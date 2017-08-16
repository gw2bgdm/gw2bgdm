#pragma once
#include "core/time.h"
#include "core/debug.h"
#include "core/message.h"
#include "core/logging.h"
#include "core/helpers.h"
#include "meter/logging.h"
#include "meter/crypto.h"
#include "meter/network.h"
#include "meter/updater.h"
#include "meter/utf.h"
#include "meter/dps.h"
#include <stdio.h>

#ifdef _MSC_VER
#include <windows.h>
#include <Shlwapi.h>
#include <Strsafe.h>
#pragma comment(lib, "shlwapi.lib")
#else
#define MAX_PATH 260
#endif

State g_state = { 0 };

logger_t s_logger;
static logger_filter_t  s_filter;
static logger_formatter_t s_formatter;
static logger_handler_t s_handler_file;
static logger_handler_t s_handler_scr;

#define PATH_METER_DLL "bgdm.dll"
#define PATH_METER_LOG "bgdm.client.log"

static i8 g_dll_path[MAX_PATH] = { 0 };
Player g_player = { 0 };


void dps_handle_msg_damage_utf8(MsgServerDamageUTF8* msg)
{
	static ServerPlayerUTF8 closest_data[MSG_CLOSEST_PLAYERS] = { 0 };
	static char closest_names[(MSG_CLOSEST_PLAYERS + 2)*MSG_NAME_SIZE_UTF8] = { 0 };

	i32 closest_len = 0;

	memset(closest_data, 0, sizeof(closest_data));
	memset(closest_names, 0, sizeof(closest_names));

	memcpy(closest_data, msg->closest, sizeof(closest_data));

	for (i32 i = 0; i < MSG_CLOSEST_PLAYERS; ++i)
	{
		if (msg->closest[i].name[0] == 0)
			break;
		if (i > 0) closest_names[closest_len++] = ':';
		strcpy(&closest_names[closest_len], msg->closest[i].name);
		closest_len += (i32)strlen(msg->closest[i].name);
	}

	char str_dmgOut[32] = { 0 };
	char str_dmgIn[32] = { 0 };
	char str_healOut[32] = { 0 };
	sprintf_num(closest_data[0].damage, str_dmgOut, sizeof(str_dmgOut));
	sprintf_num(closest_data[0].damage_in, str_dmgIn, sizeof(str_dmgIn));
	sprintf_num(closest_data[0].heal_out, str_healOut, sizeof(str_healOut));
	LOG_DEBUG("[server] update closest [%s] [dmgOut(0):%s] [dmgIn(0):%s] [heal(0):%s]", closest_names, str_dmgOut, str_dmgIn, str_healOut);
}

static bool logger_create(i8* cwd)
{
	static i8 logpath[MAX_PATH];
	FILE *logfile = NULL;

	memset(&s_logger, 0, sizeof(s_logger));
	s_logger.name = "bgdm-logger";

#ifdef _DEBUG
	s_filter.minlevel = 0;
#else
	s_filter.minlevel = 0;
#endif
	s_filter.maxlevel = LOGGING_MAX_LEVEL;
	s_filter.next = NULL;

	s_formatter.datefmt = "%Y-%m-%dT%H:%M:%SZ";
	s_formatter.fmt =
		"%(asctime)s	"
		"%(levelname)s	"
		"%(message)s";

	sprintf(logpath, "%s\\%s", cwd, PATH_METER_LOG);
	//logfile = fopen(logpath, "a");

	s_handler_scr.name = "bgdm-scr";
	s_handler_scr.file = stdout;
	s_handler_scr.filter = &s_filter;
	s_handler_scr.formatter = &s_formatter;
	s_handler_scr.next = logfile ? &s_handler_file : NULL;

	s_handler_file.name = "bgdm-file";
	s_handler_file.file = logfile;
	s_handler_file.filter = &s_filter;
	s_handler_file.formatter = &s_formatter;
	s_handler_file.next = NULL;

	s_logger.filter = &s_filter;
	s_logger.handler = &s_handler_scr;

	logging_setlogger(&s_logger);

	if (!logfile)
		LOG_WARN("[client] Unable to open %s for writing.", logpath);

	return true;
}

int wmain(int argc, wchar_t* argv[], wchar_t *envp[])
{
	static i8 cwd[MAX_PATH];

	// Get current working directory
#if defined(_WIN32) || defined(_WIN64)
	GetModuleFileNameA(NULL, cwd, MAX_PATH);
	PathRemoveFileSpecA(cwd);
	PathCombineA(g_dll_path, cwd, PATH_METER_DLL);
#else
	_getcwd(cwd, MAX_PATH);
	sprintf(g_dll_path, "%s\\%s", cwd, PATH_METER_DLL);
#endif


	// Create the logger
	if (logger_create(cwd))
		LOG_DEBUG("%s", "[client] logger started successfully");

	time_create();

	if (crypto_create() == false)
	{
		LOG_ERR("[client] error 0x%X: could not initialize crypto library", GetLastError());
		goto exit;
	}

	if (!updater_create(g_dll_path, 0))
	{
		LOG_ERR("[client] error 0x%X: %s could not be found", GetLastError(), g_dll_path);
		goto exit;
	}
	else
	{
		LOG_INFO("[client] local file version %x", updater_get_cur_version());
	}

	
	i8 srv_addr[260] = { 0 };
	u16 srv_port = MSG_NETWORK_PORT;
	if ((argc > 1))
		_snprintf(srv_addr, 260, "%S", argv[1]);
	else
		_snprintf(srv_addr, 260, "%s", MSG_NETWORK_ADDR);
	if (argc > 2 && _wtoi(argv[2]) && abs(_wtoi(argv[2]))<65536)
		srv_port = (u16)abs(_wtoi(argv[2]));

	if (!network_create(srv_addr, srv_port))
	{
		LOG_ERR("[client] could not initialize the network interface");
		goto exit;
	}

	LOG_INFO("[client] info: connecting to %s:%d", srv_addr, srv_port);

	for (;;)
	{

		// Update timing.
		i64 now = time_get();

		// Check for updates / update progress
		if (updater_update(now))
		{
			updater_create(NULL, 0);
			LOG_INFO("[client] update completed successfully [version=%x]",
				updater_get_cur_version());
		}

		// Receive queued network messages.
		static i8 buf[UDP_MAX_PKT_SIZE];
		i32 len = network_recv(buf, sizeof(buf));
		if (len < 0)
		{
			LOG_ERR("[client] error %d: internal network error", network_err());
			continue;
		}

		if (len > 0)
		{

			u32 type = *(u32*)buf;
			switch (type)
			{
			case MSG_TYPE_SERVER_DAMAGE_UTF8:
			{
				// Received a damage update message from the server.
				if (len != sizeof(MsgServerDamageUTF8))
				{
					break;
				}

				dps_handle_msg_damage_utf8((MsgServerDamageUTF8*)buf);
			} break;

			case MSG_TYPE_SERVER_UPDATE_BEGIN:
			{
				// Received an update notification from the server.
				if (len != sizeof(MsgServerUpdateBegin))
				{
					break;
				}

				updater_handle_msg_begin((MsgServerUpdateBegin*)buf);
			} break;

			case MSG_TYPE_SERVER_UPDATE_PIECE:
			{
				LOG(9, "[server] MSG_TYPE_SERVER_UPDATE_PIECE [len=%d]", len);
				// Received an update piece from the server.
				if (len != sizeof(MsgServerUpdatePiece) &&
					len != sizeof(MsgServerUpdatePieceNoFrag))
				{
					break;
				}

				updater_handle_msg_piece((MsgServerUpdatePiece*)buf);
			} break;
			}
		}


		// Send dummy dmg message to server
		// Check if we need to send a packet.
		static i64 last_update;
		static i64 update_begin = 0;
		if (now - last_update < MSG_FREQ_CLIENT_DAMAGE)
		{
			continue;
		}
		
		// Start preparing the message.
		wchar_t name[MSG_NAME_SIZE] = { 0 };
		ClientPlayerUTF8 closest[MSG_CLOSEST_PLAYERS] = { 0 };

		MsgClientDamageUTF8 msg = { 0 };
		msg.msg_type = MSG_TYPE_CLIENT_DAMAGE_UTF8;

		if (argc > 3)
			StringCchCopyW(name, ARRAYSIZE(name), argv[3]);
		else
			StringCchCopyW(name, ARRAYSIZE(name), L"test");
		if (argc > 4)
			for (i32 i = 4; i < argc; ++i)
				utf16_to_utf8(argv[i], closest[i - 4].name, sizeof(closest[i - 4].name));

		static const wchar_t *wchar_ = L"中日友好";
		//StringCchCopyW(name, ARRAYSIZE(name), wchar_);
		//utf16_to_utf8(wchar_, closest[0].name, sizeof(closest[0].name));

		StringCchPrintfW(g_player.c.name, ARRAYSIZE(g_player.c.name), L"%s", name);
		StringCchPrintfW(g_player.acctName, ARRAYSIZE(g_player.acctName), L"%s.1234", name);

		static i32 dmgOut = 1000000;
		static i32 dmgIn = 20000;
		static i32 healOut = 1000;
		dmgOut += 10000;
		dmgIn += 1000;
		healOut += 500;

		msg.shard_id = 1;
		msg.selected_id = 1;
		memset(&msg.stats, 0, sizeof(msg.stats));
		msg.in_combat = 1;
		msg.profession = 1;
		msg.time = update_begin ? (now-update_begin)/1000.0f : 0;
		msg.damage = dmgOut;
		msg.damage_in = dmgIn;
		msg.heal_out = healOut;
		utf16_to_utf8(name, msg.name, sizeof(msg.name));
		memcpy(msg.closest, closest, sizeof(msg.closest));
		if (!update_begin)
			update_begin = now;
		last_update = now;

		// Send the message.
		network_send(&msg, sizeof(msg));
	}

exit:
	network_destroy();
	crypto_destroy();

	return 0;
}
