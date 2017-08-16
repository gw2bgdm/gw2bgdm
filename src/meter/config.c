#include "core/debug.h"
#include "core/file.h"
#include "meter/utf.h"
#include "meter/dps.h"
#include "meter/resource.h"
#include "config.h"
#include <math.h>
#include <Windows.h>
#include <Strsafe.h>
#include <Shlobj.h>

#pragma comment(lib, "shell32.lib")

// The path to the config file.
#define CONFIG_PATH L"bgdm\\bgdm.ini"
#define MAX_INIKEY_LEN 256

// Key-name mapping entry.
typedef struct KeyName
{
	wchar_t* key;
	u32 vk;
} KeyName;

// Key-name mapping table.
static KeyName CONFIG_KEYS[] = {
	{ L"BACKSPACE",	VK_BACK },
	{ L"CLEAR",		VK_CLEAR },
	{ L"DELETE",	VK_DELETE },
	{ L"DOWN",		VK_DOWN },
	{ L"END",		VK_END },
	{ L"ESC",		VK_ESCAPE },
	{ L"F1",		VK_F1 },
	{ L"F10",		VK_F10 },
	{ L"F11",		VK_F11 },
	{ L"F12",		VK_F12 },
	{ L"F2",		VK_F2 },
	{ L"F3",		VK_F3 },
	{ L"F4",		VK_F4 },
	{ L"F5",		VK_F5 },
	{ L"F6",		VK_F6 },
	{ L"F7",		VK_F7 },
	{ L"F8",		VK_F8 },
	{ L"F9",		VK_F9 },
	{ L"HOME",		VK_HOME },
	{ L"INSERT",	VK_INSERT },
	{ L"LEFT",		VK_LEFT },
	{ L"NUM0",		VK_NUMPAD0 },
	{ L"NUM1",		VK_NUMPAD1 },
	{ L"NUM2",		VK_NUMPAD2 },
	{ L"NUM3",		VK_NUMPAD3 },
	{ L"NUM4",		VK_NUMPAD4 },
	{ L"NUM5",		VK_NUMPAD5 },
	{ L"NUM6",		VK_NUMPAD6 },
	{ L"NUM7",		VK_NUMPAD7 },
	{ L"NUM8",		VK_NUMPAD8 },
	{ L"NUM9",		VK_NUMPAD9 },
	{ L"PAGEDOWN",	VK_NEXT },
	{ L"PAGEUP",	VK_PRIOR },
	{ L"PAUSE",		VK_PAUSE },
	{ L"PRINT",		VK_PRINT },
	{ L"RETURN",	VK_RETURN },
	{ L"RIGHT",		VK_RIGHT },
	{ L"SELECT",	VK_SELECT },
	{ L"SPACE",		VK_SPACE },
	{ L"TAB",		VK_TAB },
	{ L"UP",		VK_UP },
	{ L"LBUTTON",	VK_LBUTTON },
	{ L"RBUTTON",	VK_RBUTTON },
	{ L"MBUTTON",	VK_MBUTTON },
};

// Returns true if the given key is down.
bool is_key_down(u32 vk)
{
	return GetAsyncKeyState(vk) & 0x8000;
}

// Returns true if the given bind is down.
bool is_bind_down(Bind const* b)
{
	if (b->vk)
		return
		((b->ctrl == 0 && !is_key_down(VK_CONTROL)) || (b->ctrl == 1 && is_key_down(VK_CONTROL))) &&
		((b->alt == 0 && !is_key_down(VK_MENU)) || (b->alt == 1 && is_key_down(VK_MENU))) &&
		((b->shift == 0 && !is_key_down(VK_SHIFT)) || (b->shift == 1 && is_key_down(VK_SHIFT))) &&
		is_key_down(b->vk);
	else
		return
		(b->ctrl == 0 || is_key_down(VK_CONTROL)) &&
		(b->alt == 0 || is_key_down(VK_MENU)) &&
		(b->shift == 0 || is_key_down(VK_SHIFT));
}

Bind config_get_bindW(const wchar_t* sec, const wchar_t* key, const wchar_t* val)
{
	wchar_t buf[MAX_INIKEY_LEN];
	GetPrivateProfileStringW(sec, key, val, buf, ARRAYSIZE(buf), config_get_ini_file());
	_wcsupr_s(buf, ARRAYSIZE(buf));

	Bind b = { 0 };
	StringCchPrintfA(b.str, ARRAYSIZE(b.str), "%S", buf);

	wchar_t* ctx = buf;
	for (wchar_t* token = wcstok_s(buf, L" + ", &ctx); token; token = wcstok_s(NULL, L" + ", &ctx))
	{
		if (lstrlenW(token) == 1)
		{
			b.vk = token[0];
		}
		else if (lstrcmpW(token, L"ALT") == 0)
		{
			b.alt = true;
		}
		else if (lstrcmpW(token, L"CTRL") == 0)
		{
			b.ctrl = true;
		}
		else if (lstrcmpW(token, L"SHIFT") == 0)
		{
			b.shift = true;
		}
		else
		{
			for (u32 i = 0; i < ARRAYSIZE(CONFIG_KEYS); ++i)
			{
				if (lstrcmpW(token, CONFIG_KEYS[i].key) == 0)
				{
					b.vk = CONFIG_KEYS[i].vk;
					break;
				}
			}
		}
	}

	return b;
}

Bind config_get_bind(i8 const* sec, i8 const* key, i8 const* val)
{
	wchar_t args[3][MAX_INIKEY_LEN] = { 0 };
	StringCchPrintfW(args[0], ARRAYSIZE(args[0]), L"%S", sec);
	StringCchPrintfW(args[1], ARRAYSIZE(args[1]), L"%S", key);
	StringCchPrintfW(args[2], ARRAYSIZE(args[2]), L"%S", val);
	return config_get_bindW(args[0], args[1], args[2]);
}

i32 config_get_intW(const wchar_t* sec, const wchar_t* key, i32 val)
{
	return (i32)GetPrivateProfileIntW(sec, key, val, config_get_ini_file());
}

i32 config_get_int(i8 const* sec, i8 const* key, i32 val)
{
	wchar_t args[3][MAX_INIKEY_LEN] = { 0 };
	StringCchPrintfW(args[0], ARRAYSIZE(args[0]), L"%S", sec);
	StringCchPrintfW(args[1], ARRAYSIZE(args[1]), L"%S", key);
	return config_get_intW(args[0], args[1], val);
}

f32 config_get_floatW(const wchar_t* sec, const wchar_t* key, f32 val)
{
	wchar_t buf[MAX_INIKEY_LEN] = { 0 };
	f32 fRet = val;
	GetPrivateProfileStringW(sec, key, NULL, buf, ARRAYSIZE(buf), config_get_ini_file());
	if (lstrlenW(buf) > 0)
		fRet = (f32)_wtof(buf);
	return fRet;
}

f32 config_get_float(i8 const* sec, i8 const* key, f32 val)
{
	wchar_t args[3][MAX_INIKEY_LEN] = { 0 };
	StringCchPrintfW(args[0], ARRAYSIZE(args[0]), L"%S", sec);
	StringCchPrintfW(args[1], ARRAYSIZE(args[1]), L"%S", key);
	return config_get_floatW(args[0], args[1], val);
}

bool config_set_intW(const wchar_t* sec, const wchar_t* key, i32 val)
{
	wchar_t buff[256] = { 0 };
	StringCchPrintfW(buff, ARRAYSIZE(buff), L"%d", val);
	return WritePrivateProfileStringW(sec, key, buff, config_get_ini_file());
}

bool config_set_int(i8 const* sec, i8 const* key, i32 val)
{
	wchar_t args[3][MAX_INIKEY_LEN] = { 0 };
	StringCchPrintfW(args[0], ARRAYSIZE(args[0]), L"%S", sec);
	StringCchPrintfW(args[1], ARRAYSIZE(args[1]), L"%S", key);
	return config_set_intW(args[0], args[1], val);
}

bool config_set_floatW(const wchar_t* sec, const wchar_t* key, f32 val)
{
	wchar_t buff[256] = { 0 };
	StringCchPrintfW(buff, ARRAYSIZE(buff), L"%.2f", val);
	return WritePrivateProfileStringW(sec, key, buff, config_get_ini_file());
}

bool config_set_float(i8 const* sec, i8 const* key, f32 val)
{
	wchar_t args[3][MAX_INIKEY_LEN] = { 0 };
	StringCchPrintfW(args[0], ARRAYSIZE(args[0]), L"%S", sec);
	StringCchPrintfW(args[1], ARRAYSIZE(args[1]), L"%S", key);
	return config_set_floatW(args[0], args[1], val);
}

wchar_t* config_get_strW(const wchar_t* sec, const wchar_t* key, const wchar_t* val)
{
	wchar_t buf[256];
	GetPrivateProfileStringW(sec, key, val, buf, ARRAYSIZE(buf), config_get_ini_file());
	return _wcsdup(buf);
}

i8* config_get_str(i8 const* sec, i8 const* key, i8 const* val)
{
	i8 buf[256] = { 0 };
	GetPrivateProfileStringA(sec, key, val, buf, ARRAYSIZE(buf), config_get_ini_fileA());
	return _strdup(buf);
}

bool config_set_strW(const wchar_t* sec, const wchar_t* key, const wchar_t* val)
{
	return WritePrivateProfileStringW(sec, key, val, config_get_ini_file());
}

bool config_set_str(i8 const* sec, i8 const* key, i8 const *val)
{
	wchar_t args[3][MAX_INIKEY_LEN] = { 0 };
	StringCchPrintfW(args[0], ARRAYSIZE(args[0]), L"%S", sec);
	StringCchPrintfW(args[1], ARRAYSIZE(args[1]), L"%S", key);
	StringCchPrintfW(args[2], ARRAYSIZE(args[2]), L"%S", val);
	return config_set_strW(args[0], args[1], args[2]);
}


const i8* config_get_my_documentsA()
{
	static char MyDocuments[1024] = { 0 };
	if (MyDocuments[0] == 0) {
		int size = WideCharToMultiByte(CP_UTF8, 0, config_get_my_documents(), -1, NULL, 0, NULL, NULL);
		size = min(size, sizeof(MyDocuments));
		WideCharToMultiByte(CP_UTF8, 0, config_get_my_documents(), -1, MyDocuments, size, NULL, NULL);
	}
	return MyDocuments;
}

const wchar_t* config_get_my_documents()
{
	static wchar_t MyDocuments[1024] = { 0 };
	if (MyDocuments[0] == 0) {
		SecureZeroMemory(MyDocuments, sizeof(MyDocuments));
		HRESULT hr = SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, CSIDL_PERSONAL, MyDocuments);
		if (FAILED(hr)) {
			StringCchPrintfW(&MyDocuments[0], ARRAYSIZE(MyDocuments), L"bin64");
		}
	}
	return MyDocuments;
}


const i8* config_get_ini_fileA()
{
	static char IniFilename[1024] = { 0 };
	if (IniFilename[0] == 0) {
		int size = WideCharToMultiByte(CP_UTF8, 0, config_get_ini_file(), -1, NULL, 0, NULL, NULL);
		size = min(size, sizeof(IniFilename));
		WideCharToMultiByte(CP_UTF8, 0, config_get_ini_file(), -1, IniFilename, size, NULL, NULL);
	}
	return IniFilename;
}

const wchar_t* config_get_ini_file()
{
	static wchar_t IniFilename[1024] = { 0 };
	if (IniFilename[0] == 0) {
		SecureZeroMemory(IniFilename, sizeof(IniFilename));
		StringCchPrintfW(&IniFilename[0], ARRAYSIZE(IniFilename), L"%s\\%s", config_get_my_documents(), CONFIG_PATH);
	}
	return IniFilename;
}

bool config_toggle_bind(Bind* bind, bool *toggle)
{
	if (!bind || !toggle)
		return false;

	bool key = is_bind_down(bind);

	if (key && bind->toggle == false)
	{
		*toggle = !(*toggle);
		bind->toggle = true;
		return true;
	}
	else if (key == false)
	{
		bind->toggle = false;
	}
	return false;
}

static void config_toggle_bind_bool(Panel* panel)
{
	if (!panel)
		return;

	bool key = is_bind_down(&panel->bind);

	if (key && panel->bind.toggle == false)
	{
		panel->enabled = !panel->enabled;
		panel->bind.toggle = true;
		config_set_int(panel->section, "Enabled", panel->enabled);
	}
	else if (key == false)
	{
		panel->bind.toggle = false;
	}
}

static void config_toggle_bind_i32(Panel* panel)
{
	if (!panel)
		return;

	bool key = is_bind_down(&panel->bind);

	if (key && panel->bind.toggle == false)
	{
		if (++panel->enabled > panel->cfg.tabNo) {
			panel->enabled = 0;
			panel->mode = 0;
		}
		else {
			panel->mode = panel->enabled - 1;
		}
		panel->bind.toggle = true;
		config_set_int(panel->section, "Enabled", panel->enabled);
		config_set_int(panel->section, "Mode", panel->mode);
	}
	else if (key == false)
	{
		panel->bind.toggle = false;
	}
}

void config_test_keybinds(void)
{
	if (config_toggle_bind(&g_state.bind_on_off, &g_state.global_on_off)) {
		config_set_int(CONFIG_SECTION_GLOBAL, "Enabled", g_state.global_on_off);
	}

	if (config_toggle_bind(&g_state.bind_input, &g_state.global_cap_input)) {
		config_set_int(CONFIG_SECTION_GLOBAL, "CaptureInput", g_state.global_cap_input);
	}

	if (config_toggle_bind(&g_state.bind_minPanels, &g_state.minPanels_enable)) {
		config_set_int(CONFIG_SECTION_GLOBAL, "MinimalPanels", g_state.minPanels_enable);
	}

	config_toggle_bind_bool(&g_state.panel_version);
	config_toggle_bind_bool(&g_state.panel_hp);
	config_toggle_bind_bool(&g_state.panel_float);
	config_toggle_bind_i32(&g_state.panel_skills);
	config_toggle_bind_bool(&g_state.panel_buff_uptime);
	config_toggle_bind_i32(&g_state.panel_gear_self);
	config_toggle_bind_i32(&g_state.panel_gear_target);
	config_toggle_bind_i32(&g_state.panel_dps_group);
	config_toggle_bind_bool(&g_state.panel_dps_self);
	config_toggle_bind_bool(&g_state.panel_compass);
}

static const char* ver_row_str[] = {
	"VER",
	"SIG",
	"PING",
	"FPS",
	"PROC",
	"SRV_TEXT",
	"SRV_TIME",
};

static const char* hp_row_str[] = {
	"PLAYER_HP",
	"TARGET_HP",
	"TARGET_DIST",
	"TARGET_BB",
	"PORT_DIST",
	"PORT_DIST_BG",
};

static const char* tdps_col_str[] = {
	"DPS",
	"DMG",
	"TTK",
	"GRAPH",
	"SPEC_ID",
};

const char* gdps_col_str[] = {
	"NAME",
	"CLS",
	"DPS",
	"PER",
	"DMGOUT",
	"DMGIN",
	"HPS",
	"HEAL",
	"TIME",
	"END",
	"GRAPH",
};

const char* buff_col_str[] = {
	"NAME",
	"CLS",
	"SUB",
	"DWN",
	"SCL",
	"SWD",
	"PRT",
	"QCK",
	"ALC",
	"FRY",
	"MGT",
	"VIG",
	"SWI",
	"STB",
	"RTL",
	"RES",
	"REG",
	"AEG",
	"GTL",
	"GLE",
	"STR",
	"DCP",
	"TAC",
	"WEA",
	"SPO",
	"FRO",
	"SUN",
	"STM",
	"STN",
	"PPD",
	"RNR",
	"RAP",
	"RRD",
	"ESM",
	"GSN",
	"NVA",
	"TIME",
};



bool GetEmbeddedResource(int ResID, LPVOID *ppData, LPDWORD pBytes)
{
	HMODULE hMod = g_hInstance;

	HRSRC hRes = FindResource(hMod, MAKEINTRESOURCE(ResID), RT_RCDATA);
	if (!hRes)
	{
		DBGPRINT(TEXT("FindResource(%d) failed, error 0x%08x"), ResID, GetLastError());
		return false;
	}

	DWORD len = SizeofResource(hMod, hRes);
	if ((len == 0) && (GetLastError() != 0))
	{
		DBGPRINT(TEXT("SizeofResource(%d) failed, error 0x%08x"), ResID, GetLastError());
		return false;
	}

	HGLOBAL hMem = LoadResource(hMod, hRes);
	if (!hMem)
	{
		DBGPRINT(TEXT("LoadResource(%d) failed, error 0x%08x"), ResID, GetLastError());
		return false;
	}

	PVOID pData = LockResource(hMem);
	if (!pData)
	{
		DBGPRINT(TEXT("LockResource(%d) failed, error 0x%08x"), ResID, GetLastError());
		return false;
	}
	if (ppData) *ppData = pData;
	if (pBytes) *pBytes = len;
	return (pData != NULL);
}

static void config_parse_columns(Panel* panel, const char **cols, int startIdx, int endIdx)
{
	static i8 buf[1024] = { 0 };
	
	panel->cfg.col_str = cols;
	if (panel->cfg.str && panel->cfg.col_str && lstrlenA(panel->cfg.str) > 0) {

		i8* ctx = buf;
		memset(buf, 0, sizeof(buf));
		StringCchCopyA(buf, sizeof(buf), panel->cfg.str);
		for (i8* token = strtok_s(buf, "|", &ctx); token; token = strtok_s(NULL, "|", &ctx))
		{
			for (i32 i = startIdx; i < endIdx; ++i) {
				if (_strcmpi(token, panel->cfg.col_str[i]) == 0)
				{
					panel->cfg.col_visible[i] = 1;
				}
			}
		}
	}
}

void config_get_panel(Panel* panel, i8 const* section, i8 const* keybind, i8 const* keybind_default, i32 enabled)
{
	StringCchCopyA(panel->section, sizeof(panel->section), section);
	panel->enabled = config_get_int(section, "Enabled", enabled);
	panel->tinyFont = config_get_int(section, "TinyFont", 1);
	panel->autoResize = config_get_int(section, "AutoResize", 1);
	panel->mode = config_get_int(section, "Mode", 0);
	panel->pos.x = config_get_int(section, "x", -1);
	panel->pos.y = config_get_int(section, "y", -1);
	panel->fAlpha = config_get_float(section, "Alpha", 0.4f);
	panel->cfg.useLocalizedText = config_get_int(section, "LocalizedText", 0);
	panel->bind = config_get_bind(CONFIG_SECTION_BIND, keybind, keybind_default);
}

void config_get_state(void)
{
	// First check if we have an INI file
	// if not use the embedded INI
	if (!file_existsW(config_get_ini_file()))
	{
		LPVOID pIniFile = NULL;
		DWORD dwBytes = 0;
		if (GetEmbeddedResource(IDR_RT_TEXT_INIFILE, &pIniFile, &dwBytes)) {
			wchar_t dir[1024] = { 0 };
			StringCchPrintfW(dir, ARRAYSIZE(dir), L"%s\\bgdm", config_get_my_documents());
			CreateDirectoryW(dir, 0);
			file_writeW(config_get_ini_file(), pIniFile, dwBytes);
		}
	}

	// Read the configuration state.
	// Global on/off
	g_state.bind_on_off = config_get_bind(CONFIG_SECTION_BIND, "GlobalOnOff", "F7");
	g_state.bind_input = config_get_bind(CONFIG_SECTION_BIND, "GlobalInput", "Ctrl + Shift + F7");
	g_state.bind_minPanels = config_get_bind(CONFIG_SECTION_BIND, "GlobalMinPanels", "Ctrl + Shift + F8");
	g_state.bind_screenshot = config_get_bind(CONFIG_SECTION_BIND, "Screenshot", "Ctrl + Shift + F10");
	g_state.global_on_off = 1;

	// For some stuid reason CTRL+SHIFT+LBUTTON doesn't work
	// whenever CTRL+SHIFT are pressed LBUTTON is sent as RBUTTON
	// so I had to use ALT+SHIFT
	g_state.bind_dps_lock = config_get_bind(CONFIG_SECTION_BIND, "DPSLock", "F8");
	g_state.bind_dps_reset = config_get_bind(CONFIG_SECTION_BIND, "DPSReset", "F9");

	// Fonts
	g_state.default_font_path = config_get_str(CONFIG_SECTION_FONT, "DefaultFontPath", NULL);
	g_state.default_font_size = config_get_int(CONFIG_SECTION_FONT, "DefaultFontSize", 13);
	g_state.tiny_font_path = config_get_str(CONFIG_SECTION_FONT, "TinyFontPath", NULL);
	g_state.tiny_font_size = config_get_int(CONFIG_SECTION_FONT, "TinyFontSize", 10);


	// Panel configuration
	config_get_panel(&g_state.panel_debug, CONFIG_SECTION_DBG, "Debug", "F12", 0);
	config_get_panel(&g_state.panel_version, CONFIG_SECTION_OPTIONS, "OptionsToggle", "Ctrl + Shift + 0", 1);
	config_get_panel(&g_state.panel_hp, CONFIG_SECTION_HP, "HPToggle", "Ctrl + Shift + 1", 1);
	config_get_panel(&g_state.panel_compass, CONFIG_SECTION_CONMPASS, "CompassToggle", "Ctrl + Shift + 2", 1);
	config_get_panel(&g_state.panel_dps_self, CONFIG_SECTION_DPS_TARGET, "DPSToggleTarget", "Ctrl + Shift + 3", 1);
	config_get_panel(&g_state.panel_dps_group, CONFIG_SECTION_DPS_GROUP, "DPSToggleGroup", "Ctrl + Shift + 4", 1);
	config_get_panel(&g_state.panel_buff_uptime, CONFIG_SECTION_BUFF_UPTIME, "BuffUptimeToggle", "Ctrl + Shift + 5", 0);
	config_get_panel(&g_state.panel_skills, CONFIG_SECTION_SKILLS, "SkillBreakdownToggle", "Ctrl + Shift + 6", 0);
	config_get_panel(&g_state.panel_float, CONFIG_SECTION_FLOAT, "FloatBarsToggle", "Ctrl + Shift + 7", 0);
	config_get_panel(&g_state.panel_gear_self, CONFIG_SECTION_GEAR_SELF, "GearToggleSelf", "Ctrl + Shift + 8", 0);
	config_get_panel(&g_state.panel_gear_target, CONFIG_SECTION_GEAR_TARGET, "GearToggleTarget", "Ctrl + Shift + 9", 1);

	// Options panel
	g_state.show_metrics = config_get_int(CONFIG_SECTION_OPTIONS, "ShowMetricsWindow", 0);
	g_state.show_server = config_get_int(CONFIG_SECTION_OPTIONS, "ShowServerStatus", 0);

	// Target stats config
	g_state.panel_dps_self.cfg.str = config_get_str(CONFIG_SECTION_DPS_TARGET, "Columns", "DPS|DMG|TTK");
	g_state.panel_dps_self.cfg.maxCol = TDPS_COL_END;
	g_state.panel_dps_self.cfg.col_visible[TDPS_COL_DPS] = 1;

	// Sort configuration
	g_state.panel_dps_group.cfg.lineNumbering = config_get_int(CONFIG_SECTION_DPS_GROUP, "LineNumbering", 1);
	g_state.panel_dps_group.cfg.useProfColoring = config_get_int(CONFIG_SECTION_DPS_GROUP, "ProfessionColoring", 0);
	g_state.panel_dps_group.cfg.maxCol = GDPS_COL_END2;
	g_state.panel_dps_group.cfg.sortColLast = GDPS_COL_END;
	g_state.panel_dps_group.cfg.col_visible[GDPS_COL_NAME] = 1;
	g_state.panel_dps_group.cfg.sortCol = config_get_int(CONFIG_SECTION_DPS_GROUP, "Sort", 1);
	g_state.panel_dps_group.cfg.asc = config_get_int(CONFIG_SECTION_DPS_GROUP, "SortAsc", 0);
	g_state.panel_dps_group.cfg.str = config_get_str(CONFIG_SECTION_DPS_GROUP,
		"Columns", "NAME|CLS|DPS|PER|DMGOUT|DMGIN|HPS|HEAL|TIME");

	// Skills panel
	g_state.panel_skills.cfg.lineNumbering = config_get_int(CONFIG_SECTION_SKILLS, "LineNumbering", 1);

	g_state.icon_pack_no = config_get_int(CONFIG_SECTION_BUFF_UPTIME, "IconPackNo", 0);
	g_state.use_seaweed_icon = config_get_int(CONFIG_SECTION_BUFF_UPTIME, "UseSeaweedSaladIcon", 0);
	g_state.use_downed_enemy_icon = config_get_int(CONFIG_SECTION_BUFF_UPTIME, "UseDownedEnemyIcon", 0);
	g_state.panel_buff_uptime.cfg.lineNumbering = config_get_int(CONFIG_SECTION_BUFF_UPTIME, "LineNumbering", 1);
	g_state.panel_buff_uptime.cfg.useProfColoring = config_get_int(CONFIG_SECTION_BUFF_UPTIME, "ProfessionColoring", 0);
	g_state.panel_buff_uptime.cfg.maxCol = BUFF_COL_END;
	g_state.panel_buff_uptime.cfg.sortColLast = BUFF_COL_END;
	g_state.panel_buff_uptime.cfg.col_visible[BUFF_COL_NAME] = 1;
	g_state.panel_buff_uptime.cfg.sortCol = config_get_int(CONFIG_SECTION_BUFF_UPTIME, "Sort", 0);
	g_state.panel_buff_uptime.cfg.asc = config_get_int(CONFIG_SECTION_BUFF_UPTIME, "SortAsc", 0);
	g_state.panel_buff_uptime.cfg.str = config_get_str(CONFIG_SECTION_BUFF_UPTIME,
		"Columns", "NAME|CLS|SUB|DWN|SCL|SWD|PRT|QCK|ALC|FRY|MGT|AEG|GTL|GLE|TIME");

	// Row config for version & hp panel
	g_state.hpCommas_disable = config_get_int(CONFIG_SECTION_HP, "CommaSeparatorsDisable", 0);
	g_state.panel_hp.cfg.str = config_get_str(CONFIG_SECTION_HP, "Columns", "PLAYER_HP|TARGET_HP|TARGET_DIST|TARGET_BB");
	g_state.panel_hp.cfg.maxCol = HP_ROW_END;

	// Global
	g_state.global_on_off = config_get_int(CONFIG_SECTION_GLOBAL, "Enabled", 1);
	g_state.imgui_ini_filename = config_get_str(CONFIG_SECTION_GLOBAL, "ImGuiIniFilename", "bgdm\\bgdmUI.ini");
	g_state.global_cap_input = config_get_int(CONFIG_SECTION_GLOBAL, "CaptureInput", 1);
	g_state.minPanels_enable = config_get_int(CONFIG_SECTION_GLOBAL, "MinimalPanels", 0);
	g_state.priortizeSquad = config_get_int(CONFIG_SECTION_GLOBAL, "PrioritizeSquadMembers", 0);
	g_state.hideNonSquad = config_get_int(CONFIG_SECTION_GLOBAL, "HideNonSquadMembers", 0);
	g_state.hideNonParty = config_get_int(CONFIG_SECTION_GLOBAL, "HideNonPartyMembers", 0);
	g_state.ooc_grace_period = config_get_int(CONFIG_SECTION_GLOBAL, "OOCGracePeriod", 5);
	g_state.target_retention_time = config_get_int(CONFIG_SECTION_GLOBAL, "TargetRetention", -1);

	// Logging
	char *log_dir = config_get_str(CONFIG_SECTION_LOG, "Directory", "bgdm\\logs");
	StringCchPrintfA(g_state.log_dir, ARRAYSIZE(g_state.log_dir), "%s\\%s", config_get_my_documentsA(), log_dir);
	if (log_dir) free(log_dir);
	g_state.log_filemode = config_get_str(CONFIG_SECTION_LOG, "FileMode", "w");
	g_state.log_minlevel = config_get_int(CONFIG_SECTION_LOG, "MinLevel", 0);

	// Language
	wchar_t *lang_dir = config_get_strW(TEXT(CONFIG_SECTION_LANG), L"Directory", L"bgdm\\lang");
	wchar_t *lang_file = config_get_strW(TEXT(CONFIG_SECTION_LANG), L"Default", L"");
	StringCchPrintfW(g_state.lang_dir, ARRAYSIZE(g_state.lang_dir), L"%s\\%s", config_get_my_documents(), lang_dir);
	if (lang_file && lstrlenW(lang_file) > 0)
		StringCchPrintfW(g_state.lang_file, ARRAYSIZE(g_state.lang_file), L"%s", lang_file);
	if (lang_file) free(lang_file);
	if (lang_dir) free(lang_dir);

	// Server addr:port
	g_state.network_addr = config_get_str(CONFIG_SECTION_SRV, "srv_addr", MSG_NETWORK_ADDR);
	g_state.network_port = (u16)config_get_int(CONFIG_SECTION_SRV, "srv_port", MSG_NETWORK_PORT);
	g_state.autoUpdate_disable = config_get_int(CONFIG_SECTION_SRV, "AutoUpdateDisable", 0);
	g_state.netDps_disable = config_get_int(CONFIG_SECTION_SRV, "NetworkDpsDisable", 0);

	// Override the old server
	if (g_state.network_addr && strcmp(g_state.network_addr, MSG_NETWORK_ADDR_AMZN) == 0) {
		free(g_state.network_addr);
		g_state.network_addr = _strdup(MSG_NETWORK_ADDR_NFO);
		config_set_str(CONFIG_SECTION_SRV, "srv_addr", g_state.network_addr);
	}

	// Debug version override
	g_state.dbg_ver = config_get_int(CONFIG_SECTION_DBG, "Version", 0);
	g_state.cn_build_id_override = CN_BUILD_ID;

	// adjust to seconds
	if (g_state.target_retention_time >= 0)
		g_state.target_retention_time = g_state.target_retention_time * 1000000;
	else
		g_state.target_retention_time = INT_MAX;

	// Setup panels configuration
	config_parse_columns(&g_state.panel_dps_group, gdps_col_str, GDPS_COL_CLS, GDPS_COL_END2);
	config_parse_columns(&g_state.panel_dps_self, tdps_col_str, TDPS_COL_DMG, TDPS_COL_END);
	config_parse_columns(&g_state.panel_buff_uptime, buff_col_str, BUFF_COL_CLS, BUFF_COL_END);
	config_parse_columns(&g_state.panel_version, ver_row_str, VER_ROW_VER, VER_ROW_END);
	config_parse_columns(&g_state.panel_hp, hp_row_str, HP_ROW_PLAYER_HP, HP_ROW_END);

	g_state.panel_dps_self.cfg.tabNo = 2;
	g_state.panel_dps_group.cfg.tabNo = 2;
	g_state.panel_skills.cfg.tabNo = 2;


	g_state.panel_hp.cfg.col_str = hp_row_str;
	g_state.panel_version.cfg.col_str = ver_row_str;
	g_state.panel_dps_self.cfg.col_str = tdps_col_str;
	g_state.panel_dps_group.cfg.col_str = gdps_col_str;
	g_state.panel_buff_uptime.cfg.col_str = buff_col_str;

	// Add panels with close button to the array
	g_state.panel_arr[0] = &g_state.panel_gear_self;
	g_state.panel_arr[1] = &g_state.panel_gear_target;
	g_state.panel_arr[2] = &g_state.panel_dps_self;
	g_state.panel_arr[3] = &g_state.panel_dps_group;
	g_state.panel_arr[4] = &g_state.panel_buff_uptime;
	g_state.panel_arr[5] = &g_state.panel_version;
	g_state.panel_arr[6] = &g_state.panel_hp;
	g_state.panel_arr[7] = &g_state.panel_compass;
	g_state.panel_arr[8] = &g_state.panel_float;
	g_state.panel_arr[9] = &g_state.panel_skills;

	for (u32 i = 0; i<2; i++) {

		Panel *panel = g_state.panel_arr[i];
		if (!panel)
			continue;

		panel->cfg.canLocalizedText = true;
		panel->cfg.tabNo = 3;
	}
}