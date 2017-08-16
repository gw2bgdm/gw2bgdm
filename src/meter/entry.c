#include "core/debug.h"
#include "core/logging.h"
#include "core/message.h"
#include "core/time.h"
#include "core/types.h"
#include "meter/logging.h"
#include "meter/dps.h"
#include "meter/autolock.h"
#include "meter/crypto.h"
#include "meter/network.h"
#include "meter/process.h"
#include "meter/updater.h"
#include "meter/dps.h"
#include "meter/lru_cache.h"
#include "meter/localization.h"
#include "meter/imgui_bgdm.h"
#include "hook/hook.h"
#include <Windows.h>

// The path to the meter.
#define PATH_METER_DLL ".\\bin64\\bgdm.dll"
#define PATH_METER_LOG "bgdm\\bgdm.log"

// Our module handle
HANDLE g_hInstance = 0;

logger_t s_logger;
static logger_filter_t  s_filter;
static logger_formatter_t s_formatter;
static logger_handler_t s_handler;

static bool logger_create(i8* filename)
{
	char path[1024] = {0};
	FILE *logfile;

	StringCchPrintfA(path, ARRAYSIZE(path), "%s\\%s", config_get_my_documentsA(), filename);

	memset(&s_logger, 0, sizeof(s_logger));
	s_logger.name = "bgdm-logger";

	s_filter.minlevel = g_state.log_minlevel;
	s_filter.maxlevel = LOGGING_MAX_LEVEL;
	s_filter.next = NULL;

	s_formatter.datefmt = "%Y-%m-%dT%H:%M:%SZ";
	s_formatter.fmt =
		"%(asctime)s	"
		"%(levelname)s	"
		"%(message)s";

	if (strcmp(g_state.log_filemode, "a") == 0)
		logfile = fopen(path, "a");
	else
		logfile = fopen(path, "w");

	s_handler.name = "bgdm-file";
	s_handler.file = logfile != NULL ? logfile : stdout;
	s_handler.filter = &s_filter;
	s_handler.formatter = &s_formatter;
	s_handler.next = NULL;

	s_logger.filter = &s_filter;
	s_logger.handler = &s_handler;

	logging_setlogger(&s_logger);

	if (!logfile) {
		DBGPRINT(TEXT("[logger] Unable to open %S for writing"), path);
	} else {
		DBGPRINT(TEXT("[logger] Successfully opened %S for writing [level=%d]"), path, g_state.log_minlevel);
	}

	return true;
}

bool meter_init()
{
	static bool m_init = false;
	if (m_init) return false;

	config_get_state();
	logger_create(PATH_METER_LOG);
	localtext_init(g_state.lang_dir, g_state.lang_file);
	mumble_link_create();
	m_init = true;
	return true;
}

void meter_create(void* device, void* viewport, HWND hwnd)
{
	g_hWnd = hwnd;
	meter_init();

	// Initialize hooking.
	if (MH_Initialize() != MH_OK)
	{
		MessageBox(0, TEXT("hook lib failed to initialize."), TEXT("Error"), MB_OK);
		return;
	}

	time_create();
	crypto_create();
	lru_create();
	autolock_init();
	process_create("Guild Wars 2", "ArenaNet_Dx_Window_Class");
	network_create(g_state.network_addr, g_state.network_port);

	updater_create(PATH_METER_DLL, g_state.dbg_ver);
	if (g_state.dbg_ver == 0 && updater_get_cur_version() != 0) {
		g_state.dbg_ver = updater_get_cur_version();
		config_set_int(CONFIG_SECTION_DBG, "Version", g_state.dbg_ver);
	}

	dps_create();

	if (!ImGui_Init(hwnd, device, viewport)) {
		LOG_ERR("ImGui init failed");
	}

	LOG_INFO("BGDM loaded successfully [version=%x]", updater_get_cur_version());
}

void meter_destroy(void)
{
	LOG_INFO("BGDM unload [version=%x]", updater_get_cur_version());

	// Release our hooks.
	MH_Uninitialize();
	ImGui_Shutdown();
	dps_destroy();
	updater_destroy();
	network_destroy();
	crypto_destroy();
	autolock_free();
	lru_destroy();
	mumble_link_destroy();
}

void meter_reset_init(void)
{
	ImGui_ImplDX9_ResetInit();
}

void meter_reset_post(void)
{
	ImGui_ImplDX9_ResetPost();
}

void bgdm_screenshot();
bool meter_present(bool check_for_update, bool use_sprite)
{
	// Update timing.
	i64 now = time_get();

	for (;;)
	{
		// Receive queued network messages.
		static i8 buf[UDP_MAX_PKT_SIZE];
		i32 len = network_recv(buf, sizeof(buf));
		if (len <= 0)
		{
			break;
		}

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
	
	// Test keybinds
	config_test_keybinds();

	// Start a new Imgui Frame
	ImGui_NewFrame();

	// Update the meter.
	dps_update(now);

	// Calc app processing time
	f32 ms = (time_get() - now) / 1000.0f;

	// Render experimental ImGui
	ImGui_Render(ms, now);

#ifndef BGDM_NODXSCREENSHOT
	// Check for screenshot request
	bgdm_screenshot();
#endif

	if (!check_for_update)
		return false;

	// Check and handle updates.
	bool ret = updater_update(now);
	if (ret && g_state.dbg_ver && updater_get_srv_version()) {
		config_set_int(CONFIG_SECTION_DBG, "Version", updater_get_srv_version());
	}
	return ret;
}

bool meter_update(void)
{
	return meter_present(true, true);
}

bool meter_was_updated()
{
	i64 now = time_get();
	return updater_update(now);
}

BOOL WINAPI DllMain(HANDLE instance, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
	{
		DBGPRINT(TEXT("DLL_PROCESS_ATTACH"));
		g_hInstance = instance;
	} break;

	case DLL_PROCESS_DETACH:
	{
		DBGPRINT(TEXT("DLL_PROCESS_DETACH"));
	} break;
	}

	return TRUE;
}



#ifndef BGDM_NODXSCREENSHOT
void bgdm_screenshot()
{

#define SAFE_RELEASE(p)          { if (p) { (p)->lpVtbl->Release(p); (p)=NULL; } }

	static wchar_t FileName[1024];

	// Check if we need to take a screenshot
	static bool is_toggled;

	bool key = is_bind_down(&g_state.bind_screenshot);
	if (key && is_toggled == false) {
		is_toggled = true;
	}
	else if (key == false) {
		is_toggled = false;
	}

	if (!is_toggled)
		return;

	SecureZeroMemory(FileName, sizeof(FileName));
	StringCchCopyW(FileName, ARRAYSIZE(FileName), config_get_my_documents());
	int len = lstrlenW(FileName);
	StringCchPrintfW(&FileName[len], ARRAYSIZE(FileName) - len, L"\\BGDM_Screenshot_");
	len = lstrlenW(FileName);

	time_t seconds;
	struct tm tm;
	//struct timeval tv;
	//gettimeofday(&tv, NULL);
	//seconds = tv.tv_sec;
	time(&seconds);
	localtime_s(&tm, &seconds);
	wcsftime(&FileName[len], ARRAYSIZE(FileName) - len, L"%Y-%m-%d %H.%M.%S", &tm);

	len = lstrlenW(FileName);
	StringCchPrintfW(&FileName[len], ARRAYSIZE(FileName) - len, L".jpg");

	LPDIRECT3DDEVICE9 pDev = ImGui_GetDevice();
	if (!pDev) return;
	LPDIRECT3DSURFACE9 pBackBuffer;
	HRESULT hr = pDev->lpVtbl->GetBackBuffer(pDev, 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	if (hr == D3D_OK) {
		D3DXSaveSurfaceToFile(FileName, D3DXIFF_JPG, pBackBuffer, NULL, NULL);
		SAFE_RELEASE(pBackBuffer);
	}
}
#endif