#include "updater.h"
#include "core/crc32.h"
#include "core/file.h"
#include "core/debug.h"
#include "core/logging.h"
#include "meter/logging.h"
#include "meter/crypto.h"
#include "meter/network.h"
#include "meter/dps.h"
#include "meter/utf.h"
#include <stdlib.h>
#include <Strsafe.h>
#include <winver.h>
#include <time.h>


// Update state.
static i8 g_update_path[260];
static u8 g_update_sig[MSG_SIG_SIZE];
static u8* g_update_data = 0;
static u64 g_update_sequence = 0;
static u32 g_update_size = 0;
static u32 g_update_offset = 0;
static u32 g_update_version = 0;
static u32 g_current_version = 0;
static time_t g_srv_lastKnowntime = { 0 };
static bool g_is_updating = 0;

bool updater_create(i8 const* path, i32 version)
{
	if (path) {
		memset(g_update_path, 0, sizeof(g_update_path));
		StringCchCopyA(g_update_path, sizeof(g_update_path), path);
	}

	if (version != 0) {
		g_current_version = version;
		return true;
	}

	u32 size;
	u8* data = file_read(g_update_path, &size);
	
	if (data)
	{
		g_current_version = crc32(data, size);
		free(data);
		return true;
	}

	return false;
}

void updater_destroy(void)
{
	free(g_update_data);
}

u32 updater_get_cur_version(void)
{
	return g_current_version;
}

u32 updater_get_srv_version(void)
{
	return g_update_version;
}

time_t updater_get_srv_time(void)
{
	return g_srv_lastKnowntime;
}

i8 const *updater_get_version_str(void)
{
	static i8 str_ver[32] = { 0 };
	static i8 buff[1024] = { 0 };

	if (str_ver[0] == 0)
	{
		UINT uSize = 0;
		LPBYTE lpBuffer = NULL;
		DWORD dwHandle = 0;
		DWORD dwCount = GetFileVersionInfoSizeA(g_update_path, &dwHandle);
		if (dwCount)
		{
			dwCount = min(dwCount, 1024);
			if (GetFileVersionInfoA(g_update_path, 0/*dwHandle*/, dwCount, buff) != 0)
			{
				if (VerQueryValueA(buff, "\\", (VOID FAR* FAR*)&lpBuffer, &uSize))
				{
					if (uSize)
					{
						VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
						StringCchPrintfA(str_ver, ARRAYSIZE(str_ver),
#if (defined BGDM_TOS_COMPLIANT)
#if (defined IMGUI_FREETYPE_STATIC) || (defined IMGUI_FREETYPE_DLL)
							"%d.%d.%d.%d_FreeType",
#else
							"%d.%d.%d.%d",
#endif
#else
							"%d.%d.%d.%d_NonTOS",
#endif
							(verInfo->dwFileVersionMS >> 16) & 0xffff,
							(verInfo->dwFileVersionMS >> 0) & 0xffff,
							(verInfo->dwFileVersionLS >> 16) & 0xffff,
							(verInfo->dwFileVersionLS >> 0) & 0xffff
						);
					}
				}
			}
		}
	}

	return str_ver;
}

bool updater_is_updating(u32* offset, u32* size)
{
	if (g_is_updating == false)
	{
		return false;
	}

	*offset = g_update_offset;
	*size = g_update_size;

	return true;
}

void updater_handle_msg_begin(MsgServerUpdateBegin* msg)
{
	LOG_DEBUG(
		"[server] ServerUpdateBegin: [seq=%x] [version=%x] [size=%d] [pubkey_len=%d]",
		msg->sequence,
		msg->data_version,
		msg->data_size,
		msg->pubkey_len
	);

	time(&g_srv_lastKnowntime);

	DBGPRINT(TEXT("[sequence=%x] [version=%x] [size=%d] [pubkey_len=%d]"),
		msg->sequence, msg->data_version, msg->data_size, msg->pubkey_len);

	if (msg->data_size == 0)
	{
		// Server sent reset signal
		g_is_updating = false;
		g_update_offset = 0;
		return;
	}

	if (!crypto_initialized() ||
		msg->data_size == 0 ||
		msg->sequence != g_update_sequence ||
		msg->data_version == g_current_version ||
		g_state.autoUpdate_disable)
	{
		if (!g_state.autoUpdate_disable)
			LOG_INFO("[client] Client has latest version, no update required [version=%x].", updater_get_cur_version());
		else
			LOG_INFO("[client] Auto-update disabled [localVersion=%x] [serverVersion=%x].", updater_get_cur_version(), msg->data_version);
		// Since we're using the update begin as ping
		// update the latest srv ver and exit so we
		// can display correct info on version panel
		g_update_version = msg->data_version;
		return;
	}

	if (g_update_data)
	{
		free(g_update_data);
		g_update_data = 0;
	}

	g_update_data = malloc(msg->data_size);
	if (g_update_data == 0)
	{
		return;
	}

	if (!crypto_set_pubkey(msg->pubkey_data, msg->pubkey_len))
	{
		DBGPRINT(TEXT("ServerUpdateBegin invalid public key [len=%d]"), msg->pubkey_len);
		return;
	}
	else
	{
		DBGPRINT(TEXT("ServerUpdateBegin public key loaded successfully [len=%d]"), msg->pubkey_len);
	}

	memcpy(g_update_sig, msg->sig, MSG_SIG_SIZE);
	g_update_version = msg->data_version;
	g_update_size = msg->data_size;
	g_update_offset = 0;
	g_is_updating = true;

	LOG_INFO(
		"[client] WILL UPDATE TO VER %x (%.1fKB)",
		msg->data_version, msg->data_size / 1024.0f);
}

void updater_handle_msg_piece(MsgServerUpdatePiece* msg)
{
	LOG_DEBUG(
		"[server] ServerUpdatePiece: [seq=%x] [version=%x] [size=%d] [offset=%d]",
		msg->sequence,
		msg->data_version,
		msg->data_size,
		msg->data_offset
	);

	time(&g_srv_lastKnowntime);

	if (!crypto_initialized() ||
		g_is_updating == false ||
		msg->sequence != g_update_sequence ||
		msg->data_version != g_update_version ||
		msg->data_offset != g_update_offset ||
		g_update_offset + msg->data_size > g_update_size)
	{
		return;
	}

	memcpy(g_update_data + msg->data_offset, msg->data_buffer, msg->data_size);
	g_update_offset += msg->data_size;

	crypto_random(&g_update_sequence, sizeof(g_update_sequence));

	DBGPRINT(TEXT("ServerUpdatePiece [sequence=%x] [version=%x] [offset=%x] [size=%d]"),
		msg->sequence, msg->data_version, msg->data_offset, msg->data_size);

	LOG_INFO(
		"[client] UPDATED OFFSET: %.1fKB @ %.1f/%.1fKB",
		msg->data_size / 1024.0f, msg->data_offset / 1024.0f, g_update_size / 1024.0f);

}

bool updater_update(i64 now)
{
	// If not updating, then check for updates.
	if (g_is_updating == false)
	{
		static i64 last_check = -MSG_FREQ_CLIENT_UPDATE_BEGIN;
		if (now - last_check >= MSG_FREQ_CLIENT_UPDATE_BEGIN)
		{
			last_check = now;
			MsgClientUpdateBeginUTF8 msg = { 0 };
			msg.msg_type = MSG_TYPE_CLIENT_UPDATE_BEGIN_UTF8;
			msg.data_version = g_current_version;
			crypto_random(&msg.sequence, sizeof(msg.sequence));
			StringCchCopyA(msg.version, ARRAYSIZE(msg.version), updater_get_version_str());
			if (g_player.c.aptr && g_player.c.id) {
				utf16_to_utf8(g_player.c.name[0] != 0 ? g_player.c.name : g_player.c.decodedName, msg.charName, sizeof(msg.charName));
				utf16_to_utf8(g_player.acctName, msg.acctName, sizeof(msg.acctName));
			}
			network_send(&msg, sizeof(msg));
			g_update_sequence = msg.sequence;
		}

		return false;
	}

	// Check if we're finished updating.
	if (g_update_data &&
		g_update_size > 0 &&
		g_update_offset == g_update_size)
	{
		LOG_INFO("[client] update download complete [version=%x] [size=%d]", g_update_version, g_update_size);
		DBGPRINT(TEXT("update download complete [version=%x] [size=%d]"), g_update_version, g_update_size);

		u32 crc = crc32(g_update_data, g_update_size);
		if (crc != g_update_version || crypto_verify(g_update_sig, sizeof(g_update_sig), g_update_data, g_update_size) == false)
		{
			LOG_ERR("[client] update verification failed [version=%x] [size=%d]", crc, g_update_size);
			DBGPRINT(TEXT("update verification failed [version=%x] [size=%d]"), crc, g_update_size);
			g_is_updating = false;
			g_update_offset = 0;
			return false;
		}

		if (file_write(g_update_path, g_update_data, g_update_size) == false)
		{
			LOG_ERR("[client] update save failed error=0x%x [path=%s]", GetLastError(), g_update_path);
			DBGPRINT(TEXT("update save failed error=0x%x [path=%S]"), GetLastError(), g_update_path);
			return false;
		}

		LOG_INFO("[client] update completed succefully [version=%x] [size=%d]", g_update_version, g_update_size);
		DBGPRINT(TEXT("update completed succefully [version=%x] [size=%d]"), g_update_version, g_update_size);

		g_is_updating = false;
		g_update_offset = 0;
		return true;
	}

	// Ask for new update pieces.
	static i64 last_piece = -MSG_FREQ_CLIENT_UPDATE_PIECE;
	if (now - last_piece >= MSG_FREQ_CLIENT_UPDATE_PIECE)
	{
		last_piece = now;

		u32 size = (g_update_size - g_update_offset);
		size = min(size, MSG_UPDATE_SIZE);

		MsgClientUpdatePiece msg = { 0 };
		msg.msg_type = MSG_TYPE_CLIENT_UPDATE_PIECE;
		msg.data_version = g_update_version;
		msg.data_offset = g_update_offset;
		msg.data_size = size;
		msg.sequence = g_update_sequence;
		network_send(&msg, sizeof(msg));

		LOG_INFO(
			"[client] UPDATE REQ SENT: %.1fKB at offset %.1f/%.1fKB [seq=%x]",
			msg.data_size / 1024.0f, msg.data_offset / 1024.0f, g_update_size / 1024.0f, msg.sequence);
	}

	return false;
}
