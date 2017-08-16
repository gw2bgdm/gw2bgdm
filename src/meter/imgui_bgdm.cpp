#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h> // for GetCurrentWindow()->TitleBarRect9)
#include <Windows.h>
#include <Strsafe.h>
#include <intrin.h>
#include <atlconv.h>
#include <atlstr.h>
#include "dxsdk/d3dx9.h"
#include "imgui_impl_dx9.h"
#include "meter/imgui_bgdm.h"
#include "meter/imgui_bgdm_ext.h"
#pragma warning (push)
#pragma warning (disable: 4800)
#pragma warning (disable: 4456)
#pragma warning (disable: 4189)
#include "imgui_extras/imgui_colorpicker.h"
#pragma warning (pop)
#include "imgui_extras/imgui_memory_editor.h"
#include "meter/ForeignFncs.h"
#include "meter/resource.h"
#include "meter/offsets.h"
#include "meter/updater.h"
#include "meter/process.h"
#include "meter/localdb.h"
#include "meter/localization.h"
#include "meter/lru_cache.h"
#include "meter/game.h"
#include "meter/dps.h"
#include "meter/gfx.h"
#include "meter/autolock.h"
#include "core/debug.h"
#include "core/file.h"
#include "core/helpers.h"
#include "hacklib/ForeignClass.h"

#pragma intrinsic(__movsb)

#define IMGUI_WHITE			IM_COL32(255, 255, 255, 255)
#define IMGUI_GRAY			IM_COL32(255, 255, 255, 185)
#define IMGUI_BLACK			IM_COL32(0, 0, 0, 255)
#define IMGUI_RED			IM_COL32(255, 0, 0, 255)
#define IMGUI_GREEN			IM_COL32(0, 255, 0, 255)
#define IMGUI_BLUE			IM_COL32(0, 100, 255, 255)
#define IMGUI_PINK			IM_COL32(255, 0, 255, 255)
#define IMGUI_YELLOW		IM_COL32(255, 255, 0, 255)
#define IMGUI_CYAN			IM_COL32(0, 255, 255, 255)
#define IMGUI_ORANGE		IM_COL32(255, 175, 0, 255)
#define IMGUI_PURPLE		IM_COL32(150, 0, 255, 255)
#define IMGUI_LIGHT_BLUE	IM_COL32(65, 150, 190, 255)
#define IMGUI_DARK_GREEN	IM_COL32(0, 175, 80, 255)
#define IMGUI_DARK_RED		IM_COL32(150, 50, 35, 255)
#define IMGUI_LIGHT_PURPLE	IM_COL32(185, 95, 250, 255)

// ImGui has a bug where if you have noinput specified
// it ignores the size setting of the window on startup
#define IMGUI_FIX_NOINPUT_BUG {		\
	static bool INIT = false;		\
	if (!INIT) {					\
		flags &= !NO_INPUT_FLAG;	\
		INIT = true;				\
	}}

static int64_t g_now = 0;
static LPDIRECT3DDEVICE9  g_pD3D = NULL;
static D3DVIEWPORT9 g_viewPort = { 0 };
static D3DVIEWPORT9* g_pViewPort = NULL;	// d3d9.dll ViewPort

static ImFont* defaultFont = NULL;
static ImFont* tinyFont = NULL;
static ImFont* proggyTiny = NULL;
static ImFont* proggyClean = NULL;
static MemoryEditor mem_edit("Memory Editor");

//static const float DEFAULT_ALPHA = 0.4f;
static const int MIN_PANEL_FLAGS = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
static const int NO_INPUT_FLAG = ImGuiWindowFlags_NoInputs;

static bool disable_input = false;
static bool minimal_panels = false;
static bool global_use_tinyfont = false;
static bool cap_keyboard = false;
static bool cap_mouse = false;
static bool cap_input = false;
static bool show_debug = false;
static bool show_lang_editor = false;
static bool localized_text_reloaded = false;

static bool hp_bars_enable = false;
static bool hp_bars_brighten = false;

static const char*  GetProggyTinyCompressedFontDataTTFBase85();

static const int TINY_FONT_WIDTH = 10;

static const int PROF_IMAGES_COUNT = IDB_PNG_PROF_END - IDB_PNG_PROF_START;
static LPDIRECT3DTEXTURE9 PROF_IMAGES[PROF_IMAGES_COUNT] = { 0 };

static const int BUFF_ICONS_COUNT = IDB_PNG_BUFF_END - IDB_PNG_BUFF_START;
static const int STD_BUFF_ICONS_COUNT = IDB_PNG_BUFF_END - IDB_PNG_BUFF_ALACRITY;
static LPDIRECT3DTEXTURE9 BUFF_ICONS[BUFF_ICONS_COUNT] = { 0 };
static LPDIRECT3DTEXTURE9 STD_BUFF_ICONS_C[STD_BUFF_ICONS_COUNT] = { 0 };
static LPDIRECT3DTEXTURE9 STD_BUFF_ICONS_D[STD_BUFF_ICONS_COUNT] = { 0 };

static LPDIRECT3DTEXTURE9 UP_ARROW = NULL;
static LPDIRECT3DTEXTURE9 DOWN_ARROW = NULL;

IMGUI_API LRESULT ImGui_ImplDX9_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static const char* ProfessionName(uint32_t profession, bool hasElite);
static LPVOID ProfessionImage(uint32_t profession, bool hasElite);
static LPVOID ProfessionImageFromPlayer(const struct Character* player);
static ImColor ColorFromCharacter(const Character *c, bool bUseColor);
static void ShowFloatBarMenu();

static ImU32 prof_colors[2][GW2::PROFESSION_END] = { 0 };

typedef struct BGDMStyle
{
	float global_alpha = 1.0f;
	int col_edit_mode = 0;
	int round_windows = 1;
	int compass_font = 0;
	int hp_percent_mode = 0;
	int hp_fixed_mode = 0;
	int hp_comma_mode = 0;
	int hp_precision_mode = 0;
	ImU32 hp_colors[HP_ROW_END] = { 0 };
} BGDMStyle;

typedef struct BGDMColor
{
	ImU32 col;
	int idx;
	const char *cfg;
	//const char *name;
} BGDMColor;

static BGDMStyle bgdm_style;

enum BGDMColors
{
	BGDM_COL_TEXT1 = 2,
	BGDM_COL_TEXT2,
	BGDM_COL_TEXT3,
	BGDM_COL_TEXT_PLAYER,
	BGDM_COL_TEXT_PLAYER_SELF,
	BGDM_COL_TEXT_PLAYER_DEAD,
	BGDM_COL_TEXT_TARGET,
	BGDM_COL_GRAD_START,
	BGDM_COL_GRAD_END,
	BGDM_COL_TARGET_LOCK_BG,
};

static ATL::CAtlArray<BGDMColor> bgdm_colors_default;
static BGDMColor bgdm_colors[] = {
	{ 0, ImGuiCol_Text,					"ColText" },			//		"Text"},
	{ 0, ImGuiCol_TextDisabled,			"ColTextDisabled " },	//		"Text disabled" },
	{ 0, -1,							"ColText1 " },			//		"Text Panel #1" },
	{ 0, -1,							"ColText2 " },			//		"Text Panel #2" },
	{ 0, -1,							"ColText3 " },			//		"Text Panel #3" },
	{ 0, -1,							"ColTextPlayer " },		//		"Text Player" },
	{ 0, -1,							"ColTextPlayerSelf " },	//		"Text Player (self)" },
	{ 0, -1,							"ColTextPlayerDead " },	//		"Text Player (dead)" },
	{ 0, -1,							"ColTextTarget " },		//		"Text Target" },
	{ 0, -1,							"ColGradStart" },
	{ 0, -1,							"ColGradEnd" },
	{ 0, -1,							"ColBorderTLock " },	//		"Border (target-lock)" },
	{ 0, ImGuiCol_Border,				"ColBorder " },			//		"Border (lines)" },
	{ 0, ImGuiCol_WindowBg,				"ColWindowBG " },		//		"Window BG" },
	{ 0, ImGuiCol_PopupBg,				"ColPopupBG " },		//		"Popup BG" },
	{ 0, ImGuiCol_FrameBg,				"ColFrameBG " },		//		"Frame BG" },
	{ 0, ImGuiCol_FrameBgActive,		"ColFrameBGActive " },	//		"Frame BG (active)" },
	{ 0, ImGuiCol_FrameBgHovered,		"ColFrameBGHover " },	//		"Frame BG (hover)" },
	{ 0, ImGuiCol_TitleBg,				"ColTitleBG " },		//		"Title BG" },
	{ 0, ImGuiCol_TitleBgCollapsed,		"ColTitleBGCollapsed " }, //	"Title BG (collapsed)" },
	{ 0, ImGuiCol_TitleBgActive,		"ColTitleBGActive " },	//		"Title BG (active)" },
	{ 0, ImGuiCol_MenuBarBg,			"ColMenuBarBG " },		//		"Menubar BG" },
	{ 0, ImGuiCol_ComboBg,				"ColComboBG " },		//		"Combobox BG" },
	{ 0, ImGuiCol_ScrollbarBg,			"ColScrollBG " },		//		"Scrollbar BG" },
	{ 0, ImGuiCol_ScrollbarGrab,		"ColScrollGrab " },		//		"Scrollbar grab" },
	{ 0, ImGuiCol_ScrollbarGrabHovered,	"ColScrollGrabHover " },//		"Scrollbar grab (hover)" },
	{ 0, ImGuiCol_ScrollbarGrabActive,	"ColScrollGrabActive " },//		"Scrollbar grab (active)" },
	{ 0, ImGuiCol_SliderGrab,			"ColSliderGrab " },		//		"Slider grab" },
	{ 0, ImGuiCol_SliderGrabActive,		"ColSliderGrabActive " },//		"Slider grab (active)" },
	{ 0, ImGuiCol_Button,				"ColButton " },			//		"Button" },
	{ 0, ImGuiCol_ButtonActive,			"ColButtonActive " },	//		"Button (active)" },
	{ 0, ImGuiCol_ButtonHovered,		"ColButtonHover " },	//		"Button (hover)" },
	{ 0, ImGuiCol_Header,				"ColHeader " },			//		"Header" },
	{ 0, ImGuiCol_HeaderActive,			"ColHeaderActive " },	//		"Header (active)" },
	{ 0, ImGuiCol_HeaderHovered,		"ColHeaderHover " },	//		"Header (hover)" },
	{ 0, ImGuiCol_Column,				"ColColumn " },			//		"Column" },
	{ 0, ImGuiCol_ColumnActive,			"ColColumnActive " },	//		"Column (active)" },
	{ 0, ImGuiCol_ColumnHovered,		"ColColumnHover " },	//		"Column (hover)" },
	{ 0, ImGuiCol_ResizeGrip,			"ColResize " },			//		"Resize Grip" },
	{ 0, ImGuiCol_ResizeGripActive,		"ColResizeActive " },	//		"Resize Grip (active)" },
	{ 0, ImGuiCol_ResizeGripHovered,	"ColResizeHover " },	//		"Resize Grip (hover)" },
	{ 0, ImGuiCol_CloseButton,			"ColCloseBtn " },		//		"Close Button" },
	{ 0, ImGuiCol_CloseButtonActive,	"ColCloseBtnActive " }, //		"Close Button (active)" },
	{ 0, ImGuiCol_CloseButtonHovered,	"ColCloseBtnHover " },	//		"Close Button (hover)" },
	{ 0, ImGuiCol_PlotLines,			"ColPlotLines " },		//		"Plot Lines" },
	{ 0, ImGuiCol_PlotLinesHovered,		"ColPlotLinesHover " }, //		"Plot Lines (hover)" },
};

extern "C" float distance(vec3 *p, vec3 *q, bool isTransform)
{
	static const float meter2inch = 39.370078740157f;
	static const float idkwhatthisis = 0.8128f;
	float scale = isTransform ? (meter2inch * idkwhatthisis) : (1.0f);
	float sum = powf((q->x - p->x), 2) + powf((q->y - p->y), 2) + powf((q->z - p->z), 2);
	return sqrtf(sum) * scale;
}

float __inline distance(vec4* p, vec4* q, bool isTransform)
{
	return distance((vec3*)p, (vec3*)q, isTransform);
}

int IsActionCam() {
	if (g_ptrs.pActionCam)
		return *(int*)g_ptrs.pActionCam;
	return false;
}

bool IsDisableInputOrActionCam()
{
	if (disable_input || IsActionCam())
		return true;
	return false;
}

EXTERN_C bool ImGui_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplDX9_WndProcHandler(hWnd, msg, wParam, lParam)) {

		switch (msg) {
		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEWHEEL:
		case WM_MOUSEMOVE:
			return cap_mouse ? false : true;
		};

		// Only capture mouse in BGDM options
		// when cap input is disabled
		if (IsDisableInputOrActionCam())
			return true;

		switch (msg) {
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_CHAR:
			return (cap_keyboard || cap_input) ? false : true;
		}

		return true;
	}

	switch (msg) {
	case WM_SIZE:
		if (g_pD3D) {
			ImGui_ImplDX9_InvalidateDeviceObjects();
		}
		break;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU)
			break;
	}

	return true;
}

IDirect3DDevice9* ImGui_GetDevice()
{
	return g_pD3D;
}

bool ImGui_GetViewportRes(uint32_t* pw, uint32_t *ph)
{
	if (!g_pD3D) return false;
	if (pw) *pw = g_viewPort.Width;
	if (ph) *ph = g_viewPort.Height;
	return true;
}

bool ImGui_GetFontSize(uint32_t* px, uint32_t *py)
{
	if (!g_pD3D) return false;
	if (px) *px = (uint32_t)ImGui::CalcTextSize("X").x;
	if (py) *py = (uint32_t)ImGui::CalcTextSize("X").y;
	return true;
}

EXTERN_C bool ImGui_Init(void* hwnd, IDirect3DDevice9* device, void *viewport)
{
	static char IniFilename[MAX_PATH] = { 0 };
	char FontFilename[1024] = { 0 };

	if (viewport) {
		g_viewPort = *(D3DVIEWPORT9*)viewport;
		g_pViewPort = (D3DVIEWPORT9*)viewport;
	}
	g_pD3D = device;
	if (!g_pD3D) return false;

	bool bRet = ImGui_ImplDX9_Init(hwnd, device);
	ImGuiIO& io = ImGui::GetIO();
	io.MouseDoubleClickTime = 0.6f;
	io.ImeWindowHandle = hwnd;

	// Set default addr for our memory editor
	mem_edit.SetBaseAddr((UCHAR*)g_ptrs.pTlsCtx, 0x1000);

	StringCchPrintfA(IniFilename, ARRAYSIZE(IniFilename), "%s\\%s", config_get_my_documentsA(), g_state.imgui_ini_filename);
	io.IniFilename = IniFilename;

	ImFontConfig font_cfg;
	font_cfg.OversampleH = 1; //or 2 is the same
	font_cfg.OversampleV = 1;
	font_cfg.PixelSnapH = true;

	proggyClean = io.Fonts->AddFontDefault();

	if (proggyTiny == NULL) {
		SecureZeroMemory(font_cfg.Name, sizeof(font_cfg.Name));
		StringCchCopyA(font_cfg.Name, ARRAYSIZE(font_cfg.Name), "Proggy Tiny");
		const char* ttf_compressed_base85 = GetProggyTinyCompressedFontDataTTFBase85();
		proggyTiny = io.Fonts->AddFontFromMemoryCompressedBase85TTF(ttf_compressed_base85, 10.0f, &font_cfg);
	}

	if (g_state.default_font_size > 0 && 
		lstrlenA(g_state.default_font_path) > 0 &&
		file_exists(g_state.default_font_path)) {

		SecureZeroMemory(FontFilename, sizeof(FontFilename));
		SecureZeroMemory(font_cfg.Name, sizeof(font_cfg.Name));
		StringCchCopyA(font_cfg.Name, ARRAYSIZE(font_cfg.Name), "Default Font");
		StringCchPrintfA(&FontFilename[0], ARRAYSIZE(FontFilename), g_state.default_font_path);
		defaultFont = io.Fonts->AddFontFromFileTTF(FontFilename, (float)g_state.default_font_size, &font_cfg, io.Fonts->GetGlyphRangesChinese());
	}

	if (g_state.tiny_font_size > 0 &&
		lstrlenA(g_state.tiny_font_path) > 0 &&
		file_exists(g_state.tiny_font_path))
	{
		SecureZeroMemory(FontFilename, sizeof(FontFilename));
		SecureZeroMemory(font_cfg.Name, sizeof(font_cfg.Name));
		StringCchCopyA(font_cfg.Name, ARRAYSIZE(font_cfg.Name), "Tiny Font");
		StringCchPrintfA(&FontFilename[0], ARRAYSIZE(FontFilename), g_state.tiny_font_path);
		tinyFont = io.Fonts->AddFontFromFileTTF(FontFilename, (float)g_state.tiny_font_size, &font_cfg, io.Fonts->GetGlyphRangesChinese());
	}

	if (tinyFont == NULL) tinyFont = proggyTiny;
	if (defaultFont == NULL) defaultFont = io.FontDefault ? io.FontDefault : io.Fonts->Fonts[0];

	// Load Profession icons
	for (int i = 0; i < PROF_IMAGES_COUNT; i++) {
		HRESULT hr = D3DXCreateTextureFromResource(device, (HMODULE)g_hInstance, MAKEINTRESOURCE(IDB_PNG_PROF_START+i), &PROF_IMAGES[i]);
		if (FAILED(hr)) {
			DBGPRINT(TEXT("D3DXCreateTextureFromResource(%d) failed, error 0x%08x"), i, GetLastError());
		}
	}

	for (int i = 0; i < BUFF_ICONS_COUNT; i++) {
		HRESULT hr = D3DXCreateTextureFromResource(device, (HMODULE)g_hInstance, MAKEINTRESOURCE(IDB_PNG_BUFF_START + i), &BUFF_ICONS[i]);
		if (FAILED(hr)) {
			DBGPRINT(TEXT("D3DXCreateTextureFromResource(%d) failed, error 0x%08x"), i, GetLastError());
		}
	}

	for (int j = 0; j < 2; j++) {
		int start = (j == 0) ? IDB_PNG_BUFF_C_START : IDB_PNG_BUFF_D_START;
		LPDIRECT3DTEXTURE9 *arr = (j == 0) ? STD_BUFF_ICONS_C : STD_BUFF_ICONS_D;
		for (int i = 0; i < STD_BUFF_ICONS_COUNT; i++) {
			HRESULT hr = D3DXCreateTextureFromResource(device, (HMODULE)g_hInstance, MAKEINTRESOURCE(start + i), &arr[i]);
			if (FAILED(hr)) {
				//DBGPRINT(TEXT("D3DXCreateTextureFromResource(%d) failed, error 0x%08x"), i, GetLastError());
			}
		}
	}

	// Arrows
	D3DXCreateTextureFromResource(device, (HMODULE)g_hInstance, MAKEINTRESOURCE(IDB_PNG_UP_ARROW), &UP_ARROW);
	D3DXCreateTextureFromResource(device, (HMODULE)g_hInstance, MAKEINTRESOURCE(IDB_PNG_DOWN_ARROW), &DOWN_ARROW);

#if !(defined BGDM_TOS_COMPLIANT)
	// Float HP bars
	ImGui_FloatBarConfigLoad();
#endif

	// Load styling config
	ImGui_StyleConfigLoad();

	hp_bars_enable = config_get_int(CONFIG_SECTION_GLOBAL, "HPBarsEnable", hp_bars_enable) > 0;
	hp_bars_brighten = config_get_int(CONFIG_SECTION_GLOBAL, "HPBarsBrighten", hp_bars_brighten) > 0;

	if (hp_bars_enable) TR_SetHPChars(hp_bars_enable);
	if (hp_bars_brighten) TR_SetHPCharsBright(hp_bars_brighten);

	return bRet;
}

EXTERN_C void ImGui_Shutdown()
{
	ImGui_ImplDX9_Shutdown();

	if (hp_bars_enable) TR_SetHPChars(0);
	if (hp_bars_brighten) TR_SetHPCharsBright(0);
}

EXTERN_C void ImGui_ImplDX9_ResetInit()
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
}

EXTERN_C bool ImGui_ImplDX9_ResetPost()
{
	if (!g_pD3D) return false;
	if (SUCCEEDED(g_pD3D->GetViewport(&g_viewPort))) {
		// Update d3d9.dll's ViewPort
		// so we can use it on "hot" reload
		if (g_pViewPort) *g_pViewPort = g_viewPort;
	}
	return ImGui_ImplDX9_CreateDeviceObjects();
}

int IsInterfaceHidden() {
	if (g_ptrs.pIfHide)
		return *(int*)g_ptrs.pIfHide;
	return false;
}

int IsMapOpen() {
	if (g_ptrs.pMapOpen)
		return *(int*)g_ptrs.pMapOpen;
	return false;
}

//void func();
//void ShowExampleAppCustomNodeGraph(bool* opened);

static void PanelSaveColumnConfig(const struct Panel* panel)
{
	static char config_sec[32] = { 0 };
	static char config_str[256] = { 0 };

	config_set_int(panel->section, "Enabled", panel->enabled);
	config_set_int(panel->section, "TinyFont", panel->tinyFont);
	config_set_int(panel->section, "AutoResize", panel->autoResize);

	if (panel->cfg.col_str == 0) return;

	int len = 0;
	int count = 0;
	memset(config_sec, 0, sizeof(config_sec));
	memset(config_str, 0, sizeof(config_str));
	for (i32 i = 0; i < panel->cfg.maxCol; ++i)
	{
		if (panel->cfg.col_visible[i]) {
			if (count > 0)
				len += _snprintf(&config_str[len], sizeof(config_str) - len, "|");
			len += _snprintf(&config_str[len], sizeof(config_str) - len, panel->cfg.col_str[i]);
			++count;
		}
	}
	if (lstrlenA(config_str) > 0) {
		config_set_str(panel->section, "Columns", config_str);
		config_set_int(panel->section, "Sort", panel->cfg.sortCol);
		config_set_int(panel->section, "SortAsc", panel->cfg.asc);
	}
}


void ShowMetricsWindow(bool* p_open, float alpha, struct State* state, float ms)
{
	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_AlwaysAutoResize /*|
		ImGuiWindowFlags_NoSavedSettings*/;
	if (IsDisableInputOrActionCam()) flags |= NO_INPUT_FLAG;

	float spacing = 80;
	ImGui::PushFont(tinyFont);
	if (ImGui::Begin("BGDM Metrics", p_open, ImVec2(0, 0), alpha, flags))
	{
		ImColor white(ImColor(bgdm_colors[BGDM_COL_TEXT1].col));
		ImGui::PushStyleColor(ImGuiCol_Text, ImColor(bgdm_colors[BGDM_COL_TEXT1].col));

		ImGui::TextColored(white, LOCALTEXT(TEXT_METRICS_GW2_PING)); ImGui::SameLine(spacing);
		ImGui::Text("%dms", g_ptrs.pPing ? *(int*)g_ptrs.pPing : 0);

		ImGui::TextColored(white, LOCALTEXT(TEXT_METRICS_GW2_FPS)); ImGui::SameLine(spacing);
		ImGui::Text("%d", g_ptrs.pFps ? *(int*)g_ptrs.pFps : 0);

		if (g_state.show_metrics_bgdm) {
			ImGui::TextColored(white, LOCALTEXT(TEXT_METRICS_BGDM_FPS)); ImGui::SameLine(spacing);
			ImGui::Text("%.1f", ImGui::GetIO().Framerate);
			ImGui::TextColored(white, LOCALTEXT(TEXT_METRICS_BGDM_APP)); ImGui::SameLine(spacing);
			//ImGui::Text("%.3fms", 1000.0f / ImGui::GetIO().Framerate);
			ImGui::Text("%.2fms", ms);
		}

		ImGui::PopStyleColor();
	}
	ImGui::End();
	ImGui::PopFont();
}

void ShowServerWindow(bool* p_open, float alpha, struct State* state)
{
	static bool update_init = 0;

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_AlwaysAutoResize /*|
		ImGuiWindowFlags_NoSavedSettings*/;
	if (IsDisableInputOrActionCam()) flags |= NO_INPUT_FLAG;

	ImGui::PushFont(tinyFont);
	if (ImGui::Begin("Server Status", p_open, ImVec2(0, 0), alpha, flags))
	{
		ImColor color(ImColor(bgdm_colors[BGDM_COL_TEXT1].col));
		u32 update_offset, update_size;

		ImGui::PushStyleColor(ImGuiCol_Text, color);

		if (state->autoUpdate_disable) {
			ImGui::Text(LOCALTEXT(TEXT_SERVEROL_AA_DISABLED));
		}
		else {
			ImGui::Text(LOCALTEXT(TEXT_SERVEROL_AA_ENABLED));
		}

		if (updater_is_updating(&update_offset, &update_size))
		{
			ImGui::Text("%s: %.1f/%.1fKB",
				LOCALTEXT(TEXT_SERVEROL_AA_UPDATING),
				update_offset / 1024.0f, update_size / 1024.0f);
			update_init = 1;
		}
		else
		{
			if (updater_get_srv_version() != 0)
			{
				if (updater_get_srv_version() == updater_get_cur_version())
				{
					ImGui::Text(update_init ? LOCALTEXT(TEXT_SERVEROL_AA_UPDATE_OK) : LOCALTEXT(TEXT_SERVEROL_AA_UPDATED));
				}
				else
				{
					ImGui::Text("%s %x",
						update_init ? LOCALTEXT(TEXT_SERVEROL_AA_UPDATE_FAIL) : LOCALTEXT(TEXT_SERVEROL_AA_UPDATE_REQ),
						updater_get_srv_version()
					);
				}
			}
			else
			{
				ImGui::Text(LOCALTEXT(TEXT_SERVEROL_SRV_NO_CONN));
			}
		}
		ImGui::PopStyleColor();

		time_t srv_time = updater_get_srv_time();
		if (srv_time != 0)
		{
			char buff[32] = { 0 };
			struct tm tm = { 0 };
			localtime_s(&tm, &srv_time);
			strftime(&buff[0], sizeof(buff), "%Y-%m-%dT%H:%M:%SZ", &tm);
			ImGui::TextColored(ImColor(bgdm_colors[BGDM_COL_TEXT2].col), "%s", buff);
		}
	}
	ImGui::End();
	ImGui::PopFont();
}

void ShowUserGuide()
{
	ImGui::BulletText("'%s' %s", g_state.panel_version.bind.str, LOCALTEXT(TEXT_HELP_USERGUIDE_BIND));
	ImGui::BulletText(LOCALTEXT(TEXT_HELP_USERGUIDE1));
	ImGui::BulletText(LOCALTEXT(TEXT_HELP_USERGUIDE2));
	ImGui::BulletText(LOCALTEXT(TEXT_HELP_USERGUIDE3));
	ImGui::BulletText(LOCALTEXT(TEXT_HELP_USERGUIDE4));
	ImGui::BulletText(LOCALTEXT(TEXT_HELP_USERGUIDE5));
	ImGui::BulletText(LOCALTEXT(TEXT_HELP_USERGUIDE6));
	ImGui::BulletText(LOCALTEXT(TEXT_HELP_USERGUIDE7));
	ImGui::BulletText(LOCALTEXT(TEXT_HELP_USERGUIDE8));
	if (ImGui::GetIO().FontAllowUserScaling)
		ImGui::BulletText(LOCALTEXT(TEXT_HELP_USERGUIDE9));
}

void ShowKeybinds()
{
	ImGui::Columns(2, "KeyBinds", false);
	ImGui::BulletText(LOCALTEXT(TEXT_BIND_BGDM_MAIN)); ImGui::NextColumn();
	ImGui::Text(g_state.panel_version.bind.str); ImGui::NextColumn();
	ImGui::BulletText(LOCALTEXT(TEXT_BIND_GLOBAL_ONOFF)); ImGui::NextColumn();
	ImGui::Text(g_state.bind_on_off.str); ImGui::NextColumn();
	ImGui::BulletText(LOCALTEXT(TEXT_BIND_INPUTCAP_ONOFF)); ImGui::NextColumn();
	ImGui::Text(g_state.bind_input.str); ImGui::NextColumn();
	ImGui::BulletText(LOCALTEXT(TEXT_BIND_HEADER_ONOFF)); ImGui::NextColumn();
	ImGui::Text(g_state.bind_minPanels.str); ImGui::NextColumn();
	ImGui::BulletText(LOCALTEXT(TEXT_BIND_BGDM_RELOAD)); ImGui::NextColumn();
	ImGui::Text("CTRL + SHIFT + F9"); ImGui::NextColumn();
	ImGui::BulletText(LOCALTEXT(TEXT_BIND_SCREENSHOT)); ImGui::NextColumn();
	ImGui::Text(g_state.bind_screenshot.str); ImGui::NextColumn();
	ImGui::BulletText(LOCALTEXT(TEXT_BIND_LOCK_TARGET)); ImGui::NextColumn();
	ImGui::Text(g_state.bind_dps_lock.str); ImGui::NextColumn();
	ImGui::BulletText(LOCALTEXT(TEXT_BIND_DATA_RESET)); ImGui::NextColumn();
	ImGui::Text(g_state.bind_dps_reset.str); ImGui::NextColumn();
	ImGui::BulletText(LOCALTEXT(TEXT_BIND_PANEL_HP)); ImGui::NextColumn();
	ImGui::Text(g_state.panel_hp.bind.str); ImGui::NextColumn();
	ImGui::BulletText(LOCALTEXT(TEXT_BIND_PANEL_COMPASS)); ImGui::NextColumn();
	ImGui::Text(g_state.panel_compass.bind.str); ImGui::NextColumn();
	ImGui::BulletText(LOCALTEXT(TEXT_BIND_PANEL_TARGET)); ImGui::NextColumn();
	ImGui::Text(g_state.panel_dps_self.bind.str); ImGui::NextColumn();
	ImGui::BulletText(LOCALTEXT(TEXT_BIND_PANEL_GROUPDPS)); ImGui::NextColumn();
	ImGui::Text(g_state.panel_dps_group.bind.str); ImGui::NextColumn();
	ImGui::BulletText(LOCALTEXT(TEXT_BIND_PANEL_BUFFUPTIME)); ImGui::NextColumn();
	ImGui::Text(g_state.panel_buff_uptime.bind.str); ImGui::NextColumn();
	ImGui::BulletText(LOCALTEXT(TEXT_BIND_PANEL_SKILLS)); ImGui::NextColumn();
	ImGui::Text(g_state.panel_skills.bind.str); ImGui::NextColumn();

#if !(defined BGDM_TOS_COMPLIANT)
		ImGui::BulletText(LOCALTEXT(TEXT_BIND_PANEL_FLOATBARS)); ImGui::NextColumn();
		ImGui::Text(g_state.panel_float.bind.str); ImGui::NextColumn();
		ImGui::BulletText(LOCALTEXT(TEXT_BIND_PANEL_INSPECT_SELF)); ImGui::NextColumn();
		ImGui::Text(g_state.panel_gear_self.bind.str); ImGui::NextColumn();
		ImGui::BulletText(LOCALTEXT(TEXT_BIND_PANEL_INSPECT_TARGET)); ImGui::NextColumn();
		ImGui::Text(g_state.panel_gear_target.bind.str); ImGui::NextColumn();
#endif
}

void ShowAppHelp(bool *p_open, float alpha, bool use_tinyfont)
{
	ImGuiWindowFlags flags = 0;
	if (minimal_panels) flags |= MIN_PANEL_FLAGS;
	if (global_use_tinyfont || use_tinyfont) ImGui::PushFont(tinyFont);
	else ImGui::PushFont(defaultFont);
	if (IsDisableInputOrActionCam()) flags |= NO_INPUT_FLAG;
	IMGUI_FIX_NOINPUT_BUG;
	if (ImGui::Begin(LOCALTEXT(TEXT_HELP_TITLE), p_open, ImVec2(460, 250), alpha, flags)) {
		ImGui::Text(LOCALTEXT(TEXT_HELP_TITLE));
		ImGui::Spacing();
		if (ImGui::CollapsingHeader(LOCALTEXT(TEXT_HELP_BASICS_HEADER), ImGuiTreeNodeFlags_Selected|ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::TextWrapped(LOCALTEXT(TEXT_HELP_BASICS_TEXT));
			ShowUserGuide();
		}
		if (ImGui::CollapsingHeader(LOCALTEXT(TEXT_HELP_BINDS_HEADER)))
		{
			ImGui::TextWrapped(LOCALTEXT(TEXT_HELP_BINDS_TEXT));
			ShowKeybinds();
		}
	}
	ImGui::End();
	ImGui::PopFont();
}


void ShowAppAbout(bool *p_open, float alpha, bool use_tinyfont)
{
	ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
	if (global_use_tinyfont || use_tinyfont) ImGui::PushFont(tinyFont);
	else ImGui::PushFont(defaultFont);
	if (IsDisableInputOrActionCam()) flags |= NO_INPUT_FLAG;
	if (ImGui::Begin(LOCALTEXT(TEXT_ABUOT_TITLE), p_open, ImVec2(0, 0), alpha, flags)) {
		ImGui::Text("%s %s (crc:%x)", LOCALTEXT(TEXT_ABOUT_VERSION), updater_get_version_str(), updater_get_cur_version());
		ImGui::Separator();
		ImGui::Text(LOCALTEXT(TEXT_ABOUT_WHAT));
		ImGui::Text("By Bhagawan(tm), 2017");
		ImGui::Text("http://gw2bgdm.blogspot.com");
	}
	ImGui::End();
	ImGui::PopFont();
}

void ShowUnicodeTest(bool *p_open, float alpha, bool use_tinyfont)
{
	ImGuiWindowFlags flags = 0;
	if (IsDisableInputOrActionCam()) flags |= NO_INPUT_FLAG;
	IMGUI_FIX_NOINPUT_BUG;
	if (global_use_tinyfont || use_tinyfont) ImGui::PushFont(tinyFont);
	else ImGui::PushFont(defaultFont);
	ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
	if (ImGui::Begin("Unicode Test", p_open, flags)) {
		const ATL::CAtlStringW unicode = L"中日友好";
		const ATL::CAtlStringA utf8 = CW2A(unicode, CP_UTF8);
		ImGui::Text(u8"UTF8-1: %s", utf8);
		ImGui::Text(u8"UTF8-2: 我是中文");
		ImGui::Text("Kanjis: \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e (nihongo)");
	}
	ImGui::End();
	ImGui::PopFont();
}

EXTERN_C void ImGui_NewFrame() {
	if (!g_pD3D) return;
	ImGui_ImplDX9_NewFrame((void*)&g_viewPort);
}

static void ShowHelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(450.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

static __inline bool ShowColorPickerMenu(const char *label, const char *btn_text, float offset, int mode, float width, ImU32 &color, float offset_help = -1.0f, ImU32 color_default = 0)
{
	bool dirty = false;
	ImGuiColorEditFlags flags = ImGuiColorEditFlags_Alpha;

	if (mode == 1) flags |= ImGuiColorEditFlags_HSV;
	else if (mode == 2) flags |= ImGuiColorEditFlags_HEX;
	else flags |= ImGuiColorEditFlags_RGB;

	if (label && label[0] != '#') {
		ImGui::Text(label); ImGui::SameLine(offset);
	}

	ImColor col(color);
	ATL::CAtlStringA str;
	str.Format("%s##btn_%s", btn_text, label);
	ImGui::PushStyleColor(ImGuiCol_Button, col);
	if (ImGui::Button(str, ImVec2(width, 0))) {
	}
	ImGui::PopStyleColor(1);

	str.Format("##pop_%s", label);
	if (ImGui::BeginPopupContextItem(str)) {

		str.Format("##col_%s", label);
		if (ImGui::ColorPicker4(str, reinterpret_cast<float*>(&col.Value), flags)) {
			color = col;
			dirty = true;
		}
		ImGui::EndPopup();
	}

	if (color_default != 0 && color != color_default) {
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
		ImGui::SameLine();
		str.Format("%s##revert_%s", LOCALTEXT(TEXT_GLOBAL_REVERT), label);
		if (ImGui::Button(str)) {
			color = color_default;
			dirty = true;
		}
		ImGui::PopStyleVar();
	}


	if (offset_help >= 0.0f) {
		ImGui::SameLine(offset_help);  ShowHelpMarker(LOCALTEXT(TEXT_GLOBAL_COLORPICK_HELP));
	}

	return dirty;
}

bool ShowNpcIDMenuItem(const struct AutoLockTarget* target, const char* name)
{
	//const ATL::CAtlStringW unicode = lru_find(GW2::CLASS_TYPE_SPECIESID, speciesId, NULL, 0);
	ATL::CAtlStringA utf8 = name;//CW2A(target->name, CP_UTF8);
	if (utf8.GetLength() == 0) utf8.Format("%d", target->npc_id);

	bool is_selected = autolock_get(target->npc_id);
	if (ImGui::MenuItemNoPopupClose(utf8, "", &is_selected)) {
		autolock_set(target->npc_id, is_selected);
		return true;
	}
	return false;
}

static void ShowCombatMenu()
{
	static char buf[128] = { 0 };

	if (ImGui::BeginMenu(LOCALTEXT(TEXT_MENU_CBT_TITLE)))
	{
		ImGui::Text(LOCALTEXT(TEXT_MENU_CBT_OOCGRACE));
		ImGui::SameLine(); ShowHelpMarker(LOCALTEXT(TEXT_MENU_CBT_OOCGRACE_HELP));
		ImGui::PushItemWidth(100);
		if (ImGui::InputInt("##OOCGracePeriod", &g_state.ooc_grace_period)) {
			if (g_state.ooc_grace_period < 0) g_state.ooc_grace_period = 0;
			config_set_int(CONFIG_SECTION_GLOBAL, "OOCGracePeriod", g_state.ooc_grace_period);
		}
		ImGui::Separator();


		ImGui::Text(LOCALTEXT(TEXT_MENU_CBT_RETENTION));
		ImGui::SameLine(); ShowHelpMarker(LOCALTEXT(TEXT_MENU_CBT_RETENTION_HELP));

		int retention_time = (g_state.target_retention_time == INT_MAX) ? (-1) : (g_state.target_retention_time / 1000000);
		ImGui::PushItemWidth(100);
		if (ImGui::InputInt("##TargetRetention", &retention_time)) {
			if (retention_time < -1) retention_time = -1;
			// Adjsut to seconds
			g_state.target_retention_time = (retention_time >= 0) ? (retention_time * 1000000) : INT_MAX;
			config_set_int(CONFIG_SECTION_GLOBAL, "TargetRetention", retention_time);
		}

		ImGui::EndMenu();
	}


	DPSTargetEx* target = dps_target_get_lastknown(g_now);

	if (ImGui::BeginMenu(LOCALTEXT(TEXT_MENU_TLOCK_TITLE)))
	{

		if (!target)
			ImGui::TextDisabled(LOCALTEXT(TEXT_MENU_TLOCK_INVALID));
		else if (!target->locked) {
			if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_MENU_TLOCK_LOCK), g_state.bind_dps_lock.str)) {
				target->locked = true;
			}
		}
		else {
			if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_MENU_TLOCK_UNLOCK), g_state.bind_dps_lock.str)) {
				target->locked = false;
			}
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu(LOCALTEXT(TEXT_MENU_AUTOLOCK_TITLE)))
	{
		bool needsUpdate = false;
		if (target && target->t.npc_id) {
			ATL::CAtlStringA utf8;
			bool autolock = autolock_get(target->t.npc_id);
			utf8.Format("%s %s", LOCALTEXT(TEXT_MENU_AUTOLOCK_CURRENT), autolock ? LOCALTEXT(TEXT_GLOBAL_REMOVE) : LOCALTEXT(TEXT_GLOBAL_ADD));
			if (ImGui::SelectableNoPopupClose(utf8)) {
				autolock_set(target->t.npc_id, !autolock);
				target->locked = !autolock;
				needsUpdate = true;
			}
			ImGui::TextDisabled("%s %d", LOCALTEXT(TEXT_MENU_AUTOLOCK_ID), target->t.npc_id);
		}
		else {
			ImGui::TextDisabled(LOCALTEXT(TEXT_MENU_TLOCK_INVALID));
		}
		ImGui::Separator();
		if (ImGui::SelectableNoPopupClose(LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_ALL))) {
			autolock_allraid();
			needsUpdate = true;
		}
		if (ImGui::SelectableNoPopupClose(LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_CLEAR))) {
			autolock_freeraid();
			needsUpdate = true;
		}
		if (ImGui::SelectableNoPopupClose(LOCALTEXT(TEXT_MENU_AUTOLOCK_CLEAR_CUSTOM))) {
			autolock_freecustom();
			needsUpdate = true;
		}
		if (ImGui::SelectableNoPopupClose(LOCALTEXT(TEXT_MENU_AUTOLOCK_CLEAR_ALL))) {
			autolock_free();
			needsUpdate = true;
		}
		ImGui::Separator();
		if (ImGui::BeginMenu(LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING1))) {
			needsUpdate |= ShowNpcIDMenuItem(&g_raid_bosses[0], LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING1B1));
			needsUpdate |= ShowNpcIDMenuItem(&g_raid_bosses[1], LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING1B2));
			needsUpdate |= ShowNpcIDMenuItem(&g_raid_bosses[2], LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING1B3));
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING2))) {
			needsUpdate |= ShowNpcIDMenuItem(&g_raid_bosses[3], LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING2B1));
			needsUpdate |= ShowNpcIDMenuItem(&g_raid_bosses[4], LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING2B21));
			needsUpdate |= ShowNpcIDMenuItem(&g_raid_bosses[5], LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING2B22));
			needsUpdate |= ShowNpcIDMenuItem(&g_raid_bosses[6], LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING2B23));
			needsUpdate |= ShowNpcIDMenuItem(&g_raid_bosses[7], LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING2B3));
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING3))) {
			needsUpdate |= ShowNpcIDMenuItem(&g_raid_bosses[8], LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING3B2));
			// Xera has 2 diff IDs make sure we tick/untick both
			if (ShowNpcIDMenuItem(&g_raid_bosses[9], LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING3B3))) {
				bool is_selected = autolock_get(g_raid_bosses[9].npc_id);
				autolock_set(g_raid_bosses[10].npc_id, is_selected);
				needsUpdate = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING4))) {
			needsUpdate |= ShowNpcIDMenuItem(&g_raid_bosses[11], LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING4B1));
			needsUpdate |= ShowNpcIDMenuItem(&g_raid_bosses[12], LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING4B2));
			needsUpdate |= ShowNpcIDMenuItem(&g_raid_bosses[13], LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING4B3));
			needsUpdate |= ShowNpcIDMenuItem(&g_raid_bosses[14], LOCALTEXT(TEXT_MENU_AUTOLOCK_RAID_WING4B4));
			ImGui::EndMenu();
		}

		typedef struct KeyVal {
			i32 key;
			bool value;
		} KeyVal, *PKeyVal;

		if (ImGui::BeginMenu(LOCALTEXT(TEXT_MENU_AUTOLOCK_CUSTOM))) {
			ImGui::BeginChild("CustomIDs", ImVec2(0, 80), true);
			autolock_iter([](void *data) {
				ATL::CAtlStringA id;
				PKeyVal entry = (PKeyVal)data;
				id.Format("%d", entry->key);
				if (ImGui::Selectable(id, entry->value))
					entry->value ^= 1;
			}, true);
			ImGui::EndChild();
			if (ImGui::Button(LOCALTEXT(TEXT_GLOBAL_REMOVE_SEL), ImVec2(ImGui::GetContentRegionAvailWidth(), 0))) {
				autolock_iter([](void *data) {
					PKeyVal entry = (PKeyVal)data;
					if (entry->value)
						autolock_set(entry->key, false);
				}, true);
				needsUpdate = true;
			}
			ImGui::PushItemWidth(100);
			ImGui::InputText("##ID", buf, sizeof(buf), ImGuiInputTextFlags_CharsDecimal);
			ImGui::SameLine();
			if (ImGui::Button(LOCALTEXT(TEXT_GLOBAL_ADD))) {
				autolock_set(atoi(buf), true);
				needsUpdate = true;
			}
			ImGui::EndMenu();
		}

		if (needsUpdate) {
			static char config_str_raid[256] = { 0 };
			static char config_str_custom[256] = { 0 };
			static char *buff;
			static size_t size;

			for (int i = 0; i < 2; i++) {
				buff = (i == 0) ? config_str_raid : config_str_custom;
				size = (i == 0) ? sizeof(config_str_raid) : sizeof(config_str_custom);
				memset(buff, 0, size);
				autolock_iter([](void *data) {
					PKeyVal entry = (PKeyVal)data;
					int len = lstrlenA(buff);
					StringCchPrintfA(&buff[len], size - len, len > 0 ? "|%d" : "%d", entry->key);
				}, (i>0));
			}

			config_set_str(CONFIG_SECTION_GLOBAL, "AutoLockRaid", config_str_raid);
			config_set_str(CONFIG_SECTION_GLOBAL, "AutoLockCustom", config_str_custom);
		}

		ImGui::EndMenu();
	}
}

static bool ShowWindowOptionsMenu(struct Panel* panel, int *outFlags)
{
	bool bRet = false;
	if (ImGui::MenuItem(LOCALTEXT(TEXT_MENU_OPT_TINY), "", &panel->tinyFont)) {
		if (outFlags) *outFlags |= ImGuiWindowFlags_AlwaysAutoResize;
		config_set_int(panel->section, "TinyFont", panel->tinyFont);
		bRet = true;
	}
	if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_MENU_OPT_AUTO_RESIZE), "", &panel->autoResize)) {
		if (outFlags) *outFlags |= ImGuiWindowFlags_AlwaysAutoResize;
		config_set_int(panel->section, "AutoResize", panel->autoResize);
		bRet = true;
	}
	ImGui::PushAllowKeyboardFocus(false);
	if (ImGui::BeginMenu(LOCALTEXT(TEXT_MENU_OPT_ALPHA))) {
		if (ImGui::SliderFloat("##Alpha", &panel->fAlpha, 0.0f, 1.0f, "%.2f")) {
			config_set_float(panel->section, "Alpha", panel->fAlpha);
			bRet = true;
		}
		ImGui::EndMenu();
	}
	ImGui::PopAllowKeyboardFocus();
	ImGui::Separator();
	return bRet;
}

static bool ShowStyleMenu(struct Panel* panel, ImGuiStyle &style, int *outFlags)
{
	static const float color_width = 60.0f;
	static const float offset_help = 160.0f;
	static const float offset = offset_help + 25.0f;
	static const float offset_radio = offset + 60.0f;

	bool dirty = false;

	if (ImGui::BeginMenu(LOCALTEXT(TEXT_MENU_STYLE_TITLE)))
	{
		ShowWindowOptionsMenu(panel, outFlags);

		if (ImGui::Button(LOCALTEXT(TEXT_GLOBAL_RESTORE_DEF))) {
			ImGui_StyleSetDefaults();
			dirty = true;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::Text(LOCALTEXT(TEXT_GLOBAL_COLORPICK_MODE));
		ImGui::SameLine(); ShowHelpMarker(LOCALTEXT(TEXT_GLOBAL_COLORPICK_MODE_HELP));
		ImGui::SameLine();
		ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_COLORPICK_RGB, "##col_edit"), &bgdm_style.col_edit_mode, 0); ImGui::SameLine();
		ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_COLORPICK_HSV, "##col_edit"), &bgdm_style.col_edit_mode, 1); ImGui::SameLine();
		ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_COLORPICK_HEX, "##col_edit"), &bgdm_style.col_edit_mode, 2);
		ImGui::Separator();


		if (ImGui::BeginMenu(LOCALTEXT(TEXT_MENU_STYLE_GLOBAL)))
		{
			ImGui::Text(LOCALTEXT(TEXT_MENU_STYLE_WND_ROUND));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_MENU_STYLE_WND_ROUND_HELP));
			ImGui::SameLine(offset);
			dirty |= ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_YES, "##round"), &bgdm_style.round_windows, 1); ImGui::SameLine(offset_radio);
			dirty |= ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_NO, "##round"), &bgdm_style.round_windows, 0);

			ImGui::Text(LOCALTEXT(TEXT_MENU_STYLE_ALPHA));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_MENU_STYLE_ALPHA_HELP));
			ImGui::SameLine(offset);
			ImGui::PushItemWidth(-1);
			// Not exposing zero here so user doesn't "lose" the UI (zero alpha clips all widgets).
			// But application code could have a toggle to switch between zero and non-zero.
			dirty |= ImGui::SliderFloat("##Alpha", &bgdm_style.global_alpha, 0.25f, 1.0f, "%.2f");
			//dirty |= ImGui::DragFloat("##Alpha", &bgdm_style.global_alpha, 0.005f, 0.20f, 1.0f, "%.2f");
			ImGui::PopItemWidth();

			ImGui::Separator();

			for (int i=0; i < ARRAYSIZE(bgdm_colors); i++)
				dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_COL_TEXT+i), "", offset, bgdm_style.col_edit_mode, color_width, bgdm_colors[i].col, offset_help, bgdm_colors_default[i].col);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(LOCALTEXT(TEXT_MENU_STYLE_METRICS)))
		{
			ImGui::Text(LOCALTEXT(TEXT_MENU_STYLE_METRICS_BGDM));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_MENU_STYLE_METRICS_BGDM_HELP));
			ImGui::SameLine(offset);
			bool needUpdate = false;
			int mode = g_state.show_metrics_bgdm == 1;
			needUpdate |= ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_YES, "##metrics"), &mode, 1); ImGui::SameLine(offset_radio);
			needUpdate |= ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_NO, "##metrics"), &mode, 0);
			if (needUpdate) {
				g_state.show_metrics_bgdm = mode == 1;
				dirty = true;
			}
			ImGui::EndMenu();
		}

		ImGui::PopStyleVar();

		if (ImGui::BeginMenu(LOCALTEXT(TEXT_MENU_STYLE_PROFCOL)))
		{
			ImGui::Text(LOCALTEXT(TEXT_MENU_STYLE_PROFCOL_HELP));
			ImGui::Separator();
			for (int i = 0; i < 2; i++) {
				ImGui::BeginGroup();
				for (int j = GW2::PROFESSION_GUARDIAN; j < GW2::PROFESSION_END; j++) {

					LPVOID img = ProfessionImage(j, i>0);
					if (img) {
						ImGui::Image(img, ImVec2(14.0f, 14.0f));
						ImGui::SameLine();
					}

					ATL::CAtlStringA str;
					str.Format("##prof_%d%d", i, j);
					dirty |= ShowColorPickerMenu(str, ProfessionName(j, i>0), 0.0f, bgdm_style.col_edit_mode, 100.0f, prof_colors[i][j]);
				}
				ImGui::EndGroup();
				ImGui::SameLine(150);
			}
			ImGui::EndMenu();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		if (ImGui::BeginMenu(LOCALTEXT(TEXT_MENU_STYLE_HPBAR)))
		{
			ImGui::Text(LOCALTEXT(TEXT_MENU_STYLE_HP_COMMA));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_MENU_STYLE_HP_COMMA_HELP));
			ImGui::SameLine(offset);
			dirty |= ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_YES, "##comma"), &bgdm_style.hp_comma_mode, 1); ImGui::SameLine(offset_radio);
			dirty |= ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_NO, "##comma"), &bgdm_style.hp_comma_mode, 0);

			ImGui::Text(LOCALTEXT(TEXT_MENU_STYLE_HP_PER));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_MENU_STYLE_HP_PER_HELP));
			ImGui::SameLine(offset);
			dirty |= ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_YES, "##percent"), &bgdm_style.hp_percent_mode, 1); ImGui::SameLine(offset_radio);
			dirty |= ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_NO, "##percent"), &bgdm_style.hp_percent_mode, 0);

			ImGui::Text(LOCALTEXT(TEXT_MENU_STYLE_HP_PER_MODE));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_MENU_STYLE_HP_PER_MODE_HELP));
			ImGui::SameLine(offset);
			dirty |= ImGui::RadioButton(LOCALTEXT(TEXT_GLOBAL_INT), &bgdm_style.hp_precision_mode, 0); ImGui::SameLine(offset_radio);
			dirty |= ImGui::RadioButton(LOCALTEXT(TEXT_GLOBAL_FLOAT), &bgdm_style.hp_precision_mode, 1);

			ImGui::Text(LOCALTEXT(TEXT_MENU_STYLE_HP_PER_WIDTH));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_MENU_STYLE_HP_PER_WIDTH_HELP));
			ImGui::SameLine(offset);
			dirty |= ImGui::RadioButton(LOCALTEXT(TEXT_GLOBAL_FIXED), &bgdm_style.hp_fixed_mode, 1); ImGui::SameLine(offset_radio);
			dirty |= ImGui::RadioButton(LOCALTEXT(TEXT_GLOBAL_DYNAMIC), &bgdm_style.hp_fixed_mode, 0);


			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_MENU_STYLE_COL_PLAYERHP), "", offset, bgdm_style.col_edit_mode, color_width, bgdm_style.hp_colors[HP_ROW_PLAYER_HP], offset_help);
			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_MENU_STYLE_COL_TARGETHP), "", offset, bgdm_style.col_edit_mode, color_width, bgdm_style.hp_colors[HP_ROW_TARGET_HP], offset_help);
			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_MENU_STYLE_COL_TARGETDST), "", offset, bgdm_style.col_edit_mode, color_width, bgdm_style.hp_colors[HP_ROW_TARGET_DIST], offset_help);
			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_MENU_STYLE_COL_TARGETBB), "", offset, bgdm_style.col_edit_mode, color_width, bgdm_style.hp_colors[HP_ROW_TARGET_BB], offset_help);
#if !(defined BGDM_TOS_COMPLIANT)
			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_MENU_STYLE_COL_PORT_DIST), "", offset, bgdm_style.col_edit_mode, color_width, bgdm_style.hp_colors[HP_ROW_PORT_DIST], offset_help);
			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_MENU_STYLE_COL_PORT_DIST_BG), "", offset, bgdm_style.col_edit_mode, color_width, bgdm_style.hp_colors[HP_ROW_PORT_DIST_BG], offset_help);
#endif		
			ImGui::EndMenu();
		}

		ImGui::PopStyleVar();

		
#if !(defined BGDM_TOS_COMPLIANT)
		ImGui::Separator();
		ShowFloatBarMenu();
#endif

		ImGui::Separator();
		static ATL::CAtlArray<ATL::CAtlStringA> itemStrings;
		static ATL::CAtlArray<const char *> items;
		//if (items.IsEmpty()) {
			itemStrings.RemoveAll();
			itemStrings.Add(LOCALTEXT(TEXT_GLOBAL_FONT_SMALL));
			itemStrings.Add(LOCALTEXT(TEXT_GLOBAL_FONT_NORMAL));
			if (tinyFont != proggyTiny) itemStrings.Add(LOCALTEXT_FMT(TEXT_GLOBAL_FONT_SMALL, " (custom)"));
			if (defaultFont != proggyClean) itemStrings.Add(LOCALTEXT_FMT(TEXT_GLOBAL_FONT_NORMAL, " (custom)"));
			items.RemoveAll();
			items.SetCount(itemStrings.GetCount());
			for (size_t i = 0; i < itemStrings.GetCount(); ++i)
				items[i] = itemStrings[i].GetString();
			if (bgdm_style.compass_font < 0 || bgdm_style.compass_font >= (int)items.GetCount()) bgdm_style.compass_font = 0;

		//}
		ImGui::Text(LOCALTEXT_FMT(TEXT_GLOBAL_COMPASS, " %s", LOCALTEXT(TEXT_GLOBAL_FONT)));
		ImGui::SameLine(offset_help - 30.0f);
		ImGui::PushItemWidth(-1);
		dirty |= ImGui::Combo("##combo", &bgdm_style.compass_font, items.GetData(), (int)items.GetCount());
		ImGui::PopItemWidth();

		ImGui::EndMenu();
	}

	if (dirty) ImGui_StyleConfigSave();

	return dirty;
}


static bool ShowLanguageButtons(float offset, float width, bool &rescan_directory, bool &reload_file)
{
	rescan_directory = ImGui::Button(LOCALTEXT(TEXT_LOCALIZATION_RESCAN), ImVec2(width, 0));
	ATL::CAtlStringA str;
	const ATL::CAtlStringA utf8 = CW2A(g_state.lang_dir, CP_UTF8);
	str.Format("%s\n%s", LOCALTEXT(TEXT_LOCALIZATION_RESCAN_HELP), utf8);
	ImGui::SameLine(offset); ShowHelpMarker(str);

	reload_file |= ImGui::Button(LOCALTEXT(TEXT_LOCALIZATION_RELOAD), ImVec2(width, 0));
	ImGui::SameLine(offset); ShowHelpMarker(LOCALTEXT(TEXT_LOCALIZATION_RELOAD_HELP));
	
	if (reload_file) return true;
	return false;
}

static bool ShowLanguageEditor(bool use_tiny_font, float offset = 160.0f);
static bool ShowLangaugeControls(bool use_tiny_font, bool isWindow = false)
{
	static char file[1024] = { 0 };
	static const float btn_width = 120.0f;
	static const float offset = 160.0f;
	static int selected_file = 0;
	static bool reload_file = false;
	static bool rescan_directory = true;
	static ATL::CAtlArray<ATL::CAtlStringA> files;
	static ATL::CAtlArray<const char *> items;

	if (reload_file) {
		reload_file = false;
		rescan_directory = true;
		localized_text_reloaded = true;
		if (selected_file > 0 && selected_file < items.GetCount()) {
			const ATL::CAtlStringW utf16 = CA2W(items[selected_file], CP_UTF8);
			localtext_load_file(utf16);
			StringCchCopyW(g_state.lang_file, ARRAYSIZE(g_state.lang_file), utf16);
			config_set_strW(TEXT(CONFIG_SECTION_LANG), L"Default", g_state.lang_file);
		}
		else {
			localtext_load_defaults();
			memset(g_state.lang_file, 0, sizeof(g_state.lang_file));
			config_set_strW(TEXT(CONFIG_SECTION_LANG), L"Default", L"");
		}
		const ATL::CAtlStringA utf8 = CW2A(g_state.lang_file, CP_UTF8);
		StringCbCopyA(file, sizeof(file), utf8.GetString());
	}

	if (rescan_directory) {
		rescan_directory = false;
		localized_text_reloaded = true;
		items.RemoveAll();
		files.RemoveAll();
		items.Add(LOCALTEXT(TEXT_LOCALIZATION_DEFAULT));
		ATL::CAtlStringW path;
		path.Format(L"%s\\*", g_state.lang_dir);
		WIN32_FIND_DATAW FindFileData;
		HANDLE hFind = FindFirstFileW(path, &FindFileData);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if (FindFileData.dwFileAttributes != INVALID_FILE_ATTRIBUTES && !(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					files.Add(CW2A(FindFileData.cFileName, CP_UTF8));
					int i = (int)files.GetCount() - 1;
					if (files[i].GetLength() > 0 && files[i][0] != '.') {
						items.Add(files[i].GetString());
						if (lstrcmpW(g_state.lang_file, FindFileData.cFileName) == 0) {
							StringCbCopyA(file, sizeof(file), files[i].GetString());
							selected_file = i + 1;
						}
					}
				}
			} while (FindNextFileW(hFind, &FindFileData));
			FindClose(hFind);
		}
	}

	if (!isWindow) {

		ShowLanguageButtons(offset, btn_width, rescan_directory, reload_file);

		if (ImGui::Button(LOCALTEXT(TEXT_LOCALIZATION_EDITOR_OPEN), ImVec2(btn_width, 0))) {
			show_lang_editor = 1;
			ShowLanguageEditor(use_tiny_font);
		}
		ImGui::SameLine(offset); ShowHelpMarker(LOCALTEXT(TEXT_LOCALIZATION_EDITOR_HELP));
		ImGui::Separator();
	}

	ImGui::Text(LOCALTEXT(TEXT_LOCALIZATION_CURR_LANG)); ImGui::SameLine(offset);
	ImGui::Text("%s", LOCALTEXT(TEXT_LANGUAGE));

	ImGui::Text(LOCALTEXT(TEXT_LOCALIZATION_CURR_FILE)); ImGui::SameLine(offset);
	ImGui::PushItemWidth(150);
	reload_file |= ImGui::Combo("##combo", &selected_file, items.GetData(), (int)items.GetCount());
	ImGui::PopItemWidth();

	if (isWindow) {

		ImGui::Separator();
		ShowLanguageButtons(offset, btn_width, rescan_directory, reload_file);

		if (ImGui::Button(LOCALTEXT(TEXT_LOCALIZATION_SAVE), ImVec2(btn_width, 0))) {
			const ATL::CAtlStringW utf16 = CA2W(file, CP_UTF8);
			localtext_save_file(utf16.GetString());
			rescan_directory = true;
		}
		ImGui::SameLine(offset);
		ImGui::PushItemWidth(200.0f);
		ImGui::InputText("##file", file, sizeof(file));
		ImGui::SameLine(); ShowHelpMarker(LOCALTEXT(TEXT_LOCALIZATION_SAVE_HELP));
		ImGui::Separator();
	}

	return false;
}

static bool ShowLanguageEditor(bool use_tiny_font, float offset)
{
	static bool m_init = false;
	bool bRet = false;

	if (global_use_tinyfont || use_tiny_font) ImGui::PushFont(tinyFont);
	else ImGui::PushFont(defaultFont);
	if (ImGui::Begin(LOCALTEXT(TEXT_LOCALIZATION_EDITOR_TITLE), &show_lang_editor)) {
		bRet = ShowLangaugeControls(use_tiny_font, true);

		ImGui::Separator();

		ImGui::Text(LOCALTEXT(TEXT_LOCALIZATION_EDITOR_FILTER)); ImGui::SameLine(offset);
		static ImGuiTextFilter filter;
		filter.Draw("##filter", 200.0f);
		ImGui::SameLine(); ShowHelpMarker(LOCALTEXT(TEXT_LOCALIZATION_EDITOR_FILTER_HELP));
		ImGui::BeginChild("#strings", ImVec2(0, -1), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
		//ImGui::PushItemWidth(-160);

		ImGui::Columns(2);
		if (!m_init) {
			m_init = true;
			ImGui::SetColumnOffset(1, 200.0f);
		}
		for (int i = 0; i < (int)localtext_size(); ++i)
		{
			const char* name = localtext_get_name(i);
			static char buff[2048] = { 0 };
			memset(buff, 0, sizeof(buff));
			StringCbCopyA(buff, sizeof(buff), LOCALTEXT(i));
			if (!filter.PassFilter(name) && !filter.PassFilter(buff))
				continue;

			ImGui::Text(name); ImGui::NextColumn();
			ImGuiInputTextFlags flags = 0;
			if (ImGui::CalcTextSize(buff).y > ImGui::GetTextLineHeight()) flags |= ImGuiInputTextFlags_Multiline|ImGuiInputTextFlags_CtrlEnterForNewLine;
			ImGui::PushID(i);
			if (ImGui::InputTextEx("##text", buff, sizeof(buff), ImVec2(-1.0, 0.0f), flags)) {
				localtext_set(i, name, buff);
			}
			ImGui::PopID();
			ImGui::NextColumn();
		}
		//ImGui::PopItemWidth();
		ImGui::EndChild();
	}
	ImGui::End();
	ImGui::PopFont();

	return bRet;
}

static bool ShowLanguageMenu(bool use_tiny_font)
{
	bool bRet = false;
	if (ImGui::BeginMenu(LOCALTEXT(TEXT_LOCALIZATION_MENU_TITLE)))
	{
		bRet = ShowLangaugeControls(use_tiny_font);
		ImGui::EndMenu();
	}
	return bRet;
}


EXTERN_C void ImGui_Render(float ms, int64_t now) {

	static bool show_test_window = false;
	static bool show_unicode_test = false;
	static bool show_app_about = false;
	static bool show_app_help = false;
	bool show_app_metrics = g_state.show_metrics;
	bool show_server_status = g_state.show_server;

	if (!g_pD3D) return;

	g_now = now;
	Panel* panel = &g_state.panel_version;
	ImGuiStyle& style = ImGui::GetStyle();

	ImGuiIO& io = ImGui::GetIO();
	cap_keyboard = io.WantCaptureKeyboard;
	cap_mouse = io.WantCaptureMouse;
	cap_input = io.WantTextInput;

	disable_input = !g_state.global_cap_input;
	minimal_panels = g_state.minPanels_enable;

	bool use_tinyfont = panel->tinyFont;
	if (global_use_tinyfont || use_tinyfont) ImGui::PushFont(tinyFont);
	else ImGui::PushFont(defaultFont);

	if (g_state.global_on_off) {
		if (show_app_about) ShowAppAbout(&show_app_about, panel->fAlpha, use_tinyfont);
		if (show_app_help) ShowAppHelp(&show_app_help, panel->fAlpha, use_tinyfont);
		if (show_app_metrics) ShowMetricsWindow(&show_app_metrics, panel->fAlpha, &g_state, ms);
		if (show_server_status) ShowServerWindow(&show_server_status, panel->fAlpha, &g_state);
		if (show_lang_editor) { ShowLanguageEditor(use_tinyfont); }
		if (show_unicode_test) ShowUnicodeTest(&show_unicode_test, panel->fAlpha, use_tinyfont);
#ifdef _DEBUG
		if (show_test_window) {
			ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
			ImGui::ShowTestWindow(&show_test_window);
		}
#endif
		if (mem_edit.IsOpen) {
			if (global_use_tinyfont || use_tinyfont) ImGui::PushFont(proggyTiny);
			else ImGui::PushFont(proggyClean);
			mem_edit.Draw();
			ImGui::PopFont();
		}
	}

	if (panel->enabled) {

		static ImGuiWindowFlags temp_flags = 0;
		static int temp_fr_count = 0;
		ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;

		if (!panel->autoResize) temp_flags = 0;
		else if (temp_flags) {
			if (temp_fr_count++ > 1) {
				flags |= temp_flags;
				temp_flags = 0;
				temp_fr_count = 0;
			}
		}

		ImGui::SetNextWindowPos(ImVec2(300, 5), ImGuiSetCond_FirstUseEver);
		if (ImGui::Begin(LOCALTEXT(TEXT_BGDM_OPTS), (bool*)&panel->enabled, ImVec2(450, 130), panel->fAlpha, flags)) {

			// Menu
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu(LOCALTEXT(TEXT_BGDM_HELP)))
				{
					ImGui::MenuItem(LOCALTEXT(TEXT_BGDM_HELP_VIEW), NULL, &show_app_help);
					if (ImGui::MenuItem(LOCALTEXT(TEXT_BGDM_HELP_METRICS), NULL, &show_app_metrics)) {
						g_state.show_metrics = show_app_metrics;
						config_set_int(CONFIG_SECTION_OPTIONS, "ShowMetricsWindow", show_app_metrics);
					}
					if (ImGui::MenuItem(LOCALTEXT(TEXT_BGDM_HELP_SRV_STATUS), NULL, &show_server_status)) {
						g_state.show_server = show_server_status;
						config_set_int(CONFIG_SECTION_OPTIONS, "ShowServerStatus", show_server_status);
					}
					ImGui::MenuItem(LOCALTEXT(TEXT_BGDM_HELP_ABOUT), NULL, &show_app_about);
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu(LOCALTEXT(TEXT_GLOBAL_OPTIONS)))
				{
					if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_BGDM_OPTS_ENABLED), g_state.bind_on_off.str, &g_state.global_on_off)) {
						config_set_int(CONFIG_SECTION_GLOBAL, "Enabled", g_state.global_on_off);
					}
					bool temp_bool = !IsDisableInputOrActionCam();
					if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_BGDM_OPTS_INPUTCAP), g_state.bind_input.str, &temp_bool)) {
						disable_input = !temp_bool;
						g_state.global_cap_input = temp_bool;
						config_set_int(CONFIG_SECTION_GLOBAL, "CaptureInput", g_state.global_cap_input);
					}
					if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_BGDM_OPTS_HEADERS), g_state.bind_minPanels.str, &minimal_panels)) {
						config_set_int(CONFIG_SECTION_GLOBAL, "MinimalPanels", minimal_panels);
					}

					if (ImGui::BeginMenu(LOCALTEXT(TEXT_BGDM_OPTS_PROX)))
					{
						if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_BGDM_OPTS_PROX_SQUAD_PRIORITY), "", &g_state.priortizeSquad, false)) {
							config_set_int(CONFIG_SECTION_GLOBAL, "PrioritizeSquadMembers", g_state.priortizeSquad);
						}
						if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_BGDM_OPTS_PROX_NON_SQUAD_HIDE), "", &g_state.hideNonSquad)) {
							config_set_int(CONFIG_SECTION_GLOBAL, "HideNonSquadMembers", g_state.hideNonSquad);
						}
						if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_BGDM_OPTS_PROX_NON_PARTY_HIDE), "", &g_state.hideNonParty)) {
							config_set_int(CONFIG_SECTION_GLOBAL, "HideNonPartyMembers", g_state.hideNonParty);
						}
						ImGui::EndMenu();
					}

					if (ImGui::BeginMenu(LOCALTEXT(TEXT_BGDM_OPTS_SRV)))
					{
						char buf[128] = { 0 };
						StringCchPrintfA(buf, ARRAYSIZE(buf), "%s:%d", g_state.network_addr, g_state.network_port);
						ImGui::MenuItem(buf, "", false, false);
						ImGui::Separator();
						temp_bool = !g_state.autoUpdate_disable;
						if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_BGDM_OPTS_SRV_AUTO_UP), "", &temp_bool)) {
							g_state.autoUpdate_disable = !temp_bool;
							config_set_int(CONFIG_SECTION_SRV, "AutoUpdateDisable", g_state.autoUpdate_disable);
						}
						temp_bool = !g_state.netDps_disable;
						if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_BGDM_OPTS_SRV_NET_SHARE), "", &temp_bool)) {
							g_state.netDps_disable = !temp_bool;
							config_set_int(CONFIG_SECTION_SRV, "NetworkDpsDisable", g_state.netDps_disable);
						}
						ImGui::EndMenu();
					}

					ShowCombatMenu();
					if (ImGui::BeginMenu(LOCALTEXT(TEXT_BGDM_OPTS_HPBARS), TR_GetHPBarPtr() != 0))
					{
						static bool hp_bars_can_enabled = TR_SetHPChars(2);
						static bool hp_bars_can_brighten = TR_SetHPCharsBright(2);
						hp_bars_enable = TR_SetHPChars(3);
						hp_bars_brighten = TR_SetHPCharsBright(3);
						if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_BGDM_OPTS_HPBARS_ENABLED), "", &hp_bars_enable, hp_bars_can_enabled)) {
							TR_SetHPChars(hp_bars_enable);
							config_set_int(CONFIG_SECTION_GLOBAL, "HPBarsEnable", hp_bars_enable);
						}
						if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_BGDM_OPTS_HPBARS_BRIGHT), "", &hp_bars_brighten, hp_bars_can_brighten)) {
							TR_SetHPCharsBright(hp_bars_brighten);
							config_set_int(CONFIG_SECTION_GLOBAL, "HPBarsBrighten", hp_bars_brighten);
						}
						ImGui::EndMenu();
					}

#if !(defined BGDM_TOS_COMPLIANT)
					ShowFloatBarMenu();
#endif
					ImGui::EndMenu();
				}

				if (ShowStyleMenu(panel, style, &temp_flags)) {
					const ImGuiStyle default_style;
					if (bgdm_style.round_windows == 1) style.WindowRounding = default_style.WindowRounding;
					else style.WindowRounding = 0.0f;

					for (int i = 0; i < ARRAYSIZE(bgdm_colors); i++) {
						int idx = bgdm_colors[i].idx;
						if (idx >= 0 && idx < ImGuiCol_COUNT)
							style.Colors[bgdm_colors[i].idx] = ImColor(bgdm_colors[i].col);
					}

					style.Alpha = bgdm_style.global_alpha;
				}

				if (ShowLanguageMenu(use_tinyfont)) {}

				if (g_state.panel_debug.enabled) {
					if (ImGui::BeginMenu("Debug"))
					{
						ImGui::MenuItem("BGDM Debug", NULL, &show_debug);
						ImGui::MenuItem("Unicode Test", NULL, &show_unicode_test);
						ImGui::MenuItem("Memory Editor", NULL, &mem_edit.IsOpen);
#ifdef _DEBUG
						ImGui::MenuItem("ImGui Demo", NULL, &show_test_window);
#endif
						ImGui::EndMenu();
					}
				}
				ImGui::EndMenuBar();
			}

			//ImGuiStyle& style = ImGui::GetStyle();
			const ImVec4 colorActive = style.Colors[ImGuiCol_SliderGrabActive];
			const ImVec4 colorButton = style.Colors[ImGuiCol_Button];
			ImGui::PushStyleColor(ImGuiCol_SliderGrab, colorActive);
			ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, colorButton);
			ImGui::PushAllowKeyboardFocus(false);

			ImGui::Text(LOCALTEXT(TEXT_BGDM_MAIN_ONOFF)); ImGui::SameLine();
			int temp_int = g_state.global_on_off;
			ImGui::PushItemWidth(30);
			if (ImGui::SliderInt("##GlobalOnOff", &temp_int, 0, 1, NULL)) {
				g_state.global_on_off = temp_int ? true : false;
				config_set_int(CONFIG_SECTION_GLOBAL, "Enabled", g_state.global_on_off);
			}
			ImGui::SameLine(); ShowHelpMarker(LOCALTEXT(TEXT_BGDM_MAIN_ONOFF_HELP));
			ImGui::SameLine(); ImGui::Spacing(); ImGui::SameLine();
			ImGui::Text(LOCALTEXT(TEXT_BGDM_MAIN_INPUTCAP_ONOFF)); ImGui::SameLine();
			temp_int = !IsDisableInputOrActionCam();
			ImGui::PushItemWidth(30);
			if (ImGui::SliderInt("##GlobalInput", &temp_int, 0, 1, NULL)) {
				disable_input = temp_int ? false : true;
				g_state.global_cap_input = !disable_input;
				config_set_int(CONFIG_SECTION_GLOBAL, "CaptureInput", g_state.global_cap_input);
			}
			ImGui::SameLine(); ShowHelpMarker(LOCALTEXT(TEXT_BGDM_MAIN_INPUTCAP_ONOFF_HELP));

			ImGui::PopAllowKeyboardFocus();
			ImGui::PopStyleColor(2);

#if !(defined BGDM_TOS_COMPLIANT)
			bool needSaveFloat = false;
#endif
			bool needSaveHP = false;
			bool needSaveCompass = false;
			bool needSaveTarget = false;
			bool needSaveGroup = false;
			bool needSaveBuff = false;
			bool needSaveSkills = false;
			bool needSaveGearSelf = false;
			bool needSaveGearTarget = false;
			bool tmp_bool = false;
			ImGui::Separator();
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
			tmp_bool = g_state.panel_hp.enabled && g_state.panel_hp.cfg.col_visible[HP_ROW_TARGET_HP];
			if (ImGui::Checkbox(LOCALTEXT(TEXT_BGDM_MAIN_PANEL_TARGET_HP), &tmp_bool)) {
				needSaveHP = true;
				if (!g_state.panel_hp.enabled) g_state.panel_hp.enabled = tmp_bool;
				g_state.panel_hp.cfg.col_visible[HP_ROW_TARGET_HP] = tmp_bool;
			}
			ImGui::SameLine(150);
			tmp_bool = g_state.panel_hp.enabled && g_state.panel_hp.cfg.col_visible[HP_ROW_TARGET_DIST];
			if (ImGui::Checkbox(LOCALTEXT(TEXT_BGDM_MAIN_PANEL_TARGET_DIST), &tmp_bool)) {
				needSaveHP = true;
				if (!g_state.panel_hp.enabled) g_state.panel_hp.enabled = tmp_bool;
				g_state.panel_hp.cfg.col_visible[HP_ROW_TARGET_DIST] = tmp_bool;
			}
			ImGui::SameLine(300);
			tmp_bool = g_state.panel_hp.enabled && g_state.panel_hp.cfg.col_visible[HP_ROW_TARGET_BB];
			if (ImGui::Checkbox(LOCALTEXT(TEXT_BGDM_MAIN_PANEL_TARGET_BB), &tmp_bool)) {
				needSaveHP = true;
				if (!g_state.panel_hp.enabled) g_state.panel_hp.enabled = tmp_bool;
				g_state.panel_hp.cfg.col_visible[HP_ROW_TARGET_BB] = tmp_bool;
			}
			tmp_bool = g_state.panel_hp.enabled && g_state.panel_hp.cfg.col_visible[HP_ROW_PLAYER_HP];
			if (ImGui::Checkbox(LOCALTEXT(TEXT_BGDM_MAIN_PANEL_PLAYER_HP), &tmp_bool)) {
				needSaveHP = true;
				if (!g_state.panel_hp.enabled) g_state.panel_hp.enabled = tmp_bool;
				g_state.panel_hp.cfg.col_visible[HP_ROW_PLAYER_HP] = tmp_bool;
			}
			ImGui::SameLine(150);
#if !(defined BGDM_TOS_COMPLIANT)
			tmp_bool = g_state.panel_hp.enabled && g_state.panel_hp.cfg.col_visible[HP_ROW_PORT_DIST];
			if (ImGui::Checkbox(LOCALTEXT(TEXT_BGDM_MAIN_PANEL_PORT_DIST), &tmp_bool)) {
				needSaveHP = true;
				if (!g_state.panel_hp.enabled) g_state.panel_hp.enabled = tmp_bool;
				g_state.panel_hp.cfg.col_visible[HP_ROW_PORT_DIST] = tmp_bool;
			}
			ImGui::SameLine(300);
			needSaveFloat = ImGui::Checkbox(LOCALTEXT(TEXT_BGDM_MAIN_PANEL_FLOAT_BARS), (bool*)&g_state.panel_float.enabled);
#endif
			needSaveCompass = ImGui::Checkbox(LOCALTEXT(TEXT_BGDM_MAIN_PANEL_COMPASS), (bool*)&g_state.panel_compass.enabled);
			ImGui::Separator();
			needSaveTarget = ImGui::Checkbox(LOCALTEXT(TEXT_BGDM_MAIN_PANEL_TARGET_STATS), (bool*)&g_state.panel_dps_self.enabled); ImGui::SameLine(150);
			needSaveGroup = ImGui::Checkbox(LOCALTEXT(TEXT_BGDM_MAIN_PANEL_GROUP_STATS), (bool*)&g_state.panel_dps_group.enabled); ImGui::SameLine(300);
			needSaveBuff = ImGui::Checkbox(LOCALTEXT(TEXT_BGDM_MAIN_PANEL_BUFF_UPTIME), (bool*)&g_state.panel_buff_uptime.enabled);
			needSaveSkills = ImGui::Checkbox(LOCALTEXT(TEXT_BGDM_MAIN_PANEL_SKILLS), (bool*)&g_state.panel_skills.enabled);

#if !(defined BGDM_TOS_COMPLIANT)
				ImGui::SameLine(150);
				needSaveGearSelf = ImGui::Checkbox(LOCALTEXT(TEXT_BGDM_MAIN_PANEL_INSPECT_SELF), (bool*)&g_state.panel_gear_self.enabled); ImGui::SameLine(300);
				needSaveGearTarget = ImGui::Checkbox(LOCALTEXT(TEXT_BGDM_MAIN_PANEL_INSPECT_TARGET), (bool*)&g_state.panel_gear_target.enabled);
#endif
			ImGui::Separator();
			ImGui::PopStyleVar();

#if !(defined BGDM_TOS_COMPLIANT)
			if (needSaveFloat) ImGui_FloatBarConfigSave();
#endif
			if (needSaveHP) PanelSaveColumnConfig(&g_state.panel_hp);
			if (needSaveCompass) PanelSaveColumnConfig(&g_state.panel_compass);
			if (needSaveTarget) PanelSaveColumnConfig(&g_state.panel_dps_self);
			if (needSaveGroup) PanelSaveColumnConfig(&g_state.panel_dps_group);
			if (needSaveBuff) PanelSaveColumnConfig(&g_state.panel_buff_uptime);
			if (needSaveSkills) PanelSaveColumnConfig(&g_state.panel_skills);
			if (needSaveGearSelf) PanelSaveColumnConfig(&g_state.panel_gear_self);
			if (needSaveGearTarget) PanelSaveColumnConfig(&g_state.panel_gear_target);
		}
		ImGui::End();
	}
	ImGui::PopFont();

	ImGui::Render();
}

#if !(defined BGDM_TOS_COMPLIANT)
static __inline void DrawTextAndFrame(float per, ImVec2 pos, ImVec2 size, ImU32 col_text, ImU32 col_frame, float rounding = 0.0f, float thickness = 1.0f, bool draw_per = true, float dist = 0.0f)
{
	float y = (float)pos.y;
	float x = (float)pos.x - size.x / 2;
	if (per < 0.0f) per = 0.0f;
	if (per > 1.0f) per = 1.0f;

	uint32_t hp = (uint32_t)ceilf(per * 100.0f);
	hp = min(hp, 100);

	uint32_t num = 0;
	num += (hp > 9);
	num += (hp > 99);

	CAtlStringA str;
	str.Format("%u%%", hp);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	float text_height = ImGui::CalcTextSize("100%%", NULL, true).y;
	float text_y = (text_height < size.y) ? pos.y + (size.y-text_height)/2 : pos.y + 1.0f;

	if (draw_per) {
		if (dist == 0.0f)
			draw_list->AddText(ImVec2(pos.x - num*TINY_FONT_WIDTH / 2, text_y), col_text, str.GetString());
		else
			draw_list->AddText(ImVec2(x + ImGui::CalcTextSize("1").x, text_y), col_text, str.GetString());
	}
	if (thickness > 0.0f) draw_list->AddRect(ImVec2(x, y), ImVec2(x, y) + size, col_frame, rounding, -1, thickness);
	if (dist > 0.0f) {
		str.Format("%.0f ", dist);
		x = pos.x + size.x / 2 - ImGui::CalcTextSize(str.GetString()).x;
		draw_list->AddText(ImVec2(x, text_y), col_text, str.GetString());
	}
}

static void DrawRectPercentage(float per, ImVec2 pos, ImVec2 size, ImU32 col_left, ImU32 col_right, ImU32 col_frame, float rounding = 0.0f, bool draw_frame = true)
{
	
	float y = (float)pos.y;
	float x = (float)pos.x - size.x / 2;
	if (per < 0.0f) per = 0.0f;
	if (per > 1.0f) per = 1.0f;

	ImVec2 rect_size_left(size.x * per, size.y);
	ImVec2 rect_size_right(size.x * (1.0f - per), size.y);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	if (rect_size_left.x > 0.0f)
		draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x, y) + rect_size_left, col_left, rounding, -1);

	if (rect_size_right.x > 0.0f)
			draw_list->AddRectFilled(
				ImVec2(x + rect_size_left.x, y),
				ImVec2(x + rect_size_left.x, y) + rect_size_right,
				col_right, rounding, -1);

	if (draw_frame)
		draw_list->AddRect(ImVec2(x, y), ImVec2(x, y) + size, col_frame, rounding, -1);
}


enum FloatBarColors {
	FLOAT_COL_BG,
	FLOAT_COL_TEXT,
	FLOAT_COL_FRAME,
	FLOAT_COL_HP_LEFT,
	FLOAT_COL_HP_LEFT90,
	FLOAT_COL_HP_RIGHT,
	FLOAT_COL_DOWN_LEFT,
	FLOAT_COL_DOWN_RIGHT,
	FLOAT_COL_SHROUD_LEFT,
	FLOAT_COL_SHROUD_RIGHT,
	FLOAT_COL_GOTL_LEFT,
	FLOAT_COL_GOTL_RIGHT,
	FLOAT_COL_HP_END
};

typedef struct FloatBarConfig
{
	bool draw_hp;
	bool draw_cls;
	bool draw_gotl;
	int mode_self;
	int disp_mode;
	int dist_mode;
	int per_mode;
	int text_mode;
	int cls_mode;
	int col_mode;
	int closest_max;
	int frame_round_mode;
	float frame_round_default;
	float frame_round;
	float frame_thick;
	float vert_adjust;
	float icon_size;
	float rect_width;
	float rect_height_hp;
	float rect_height_gotl;
	uint32_t colors[FLOAT_COL_HP_END];
} FloatBarConfig;

static FloatBarConfig flt_config;

static __inline void FloatBarSetDefaults(FloatBarConfig &flt)
{
	flt.draw_hp = true;
	flt.draw_cls = true;
	flt.draw_gotl = true;
	flt.mode_self = 0;
	flt.disp_mode = 0;
	flt.dist_mode = 0;
	flt.per_mode = 0;
	flt.text_mode = 0;
	flt.cls_mode = 0;
	flt.col_mode = 1;
	flt.closest_max = 9;
	flt.frame_round_mode = 1;
	flt.frame_round_default = 2.0f;
	flt.frame_round = flt.frame_round_default;
	flt.frame_thick = 2.0f;
	flt.vert_adjust = 0.0f;
	flt.rect_width = 80.0f;
	flt.rect_height_hp = 13.0f;
	flt.rect_height_gotl = 9.0f;
	flt.icon_size = flt.rect_height_hp + flt.rect_height_gotl - 1.0f;

	int default_bg = -1272568280;
	flt.colors[FLOAT_COL_BG] = default_bg;
	flt.colors[FLOAT_COL_TEXT] = ImColor(IMGUI_BLACK);
	flt.colors[FLOAT_COL_FRAME] = ImColor(IMGUI_BLACK);
	flt.colors[FLOAT_COL_HP_LEFT] = ImColor(IMGUI_GREEN);
	flt.colors[FLOAT_COL_HP_LEFT90] = flt.colors[FLOAT_COL_HP_LEFT];
	flt.colors[FLOAT_COL_HP_RIGHT] = ImColor(IMGUI_RED);
	flt.colors[FLOAT_COL_DOWN_LEFT] = ImColor(IMGUI_DARK_RED);
	flt.colors[FLOAT_COL_DOWN_RIGHT] = default_bg;
	flt.colors[FLOAT_COL_SHROUD_LEFT] = ImColor(IMGUI_DARK_GREEN);
	flt.colors[FLOAT_COL_SHROUD_RIGHT] = default_bg;
	flt.colors[FLOAT_COL_GOTL_LEFT] = ImColor(IMGUI_CYAN);
	flt.colors[FLOAT_COL_GOTL_RIGHT] = default_bg;
}

FloatBarConfig* ImGui_FloatBarGet()
{
	return &flt_config;
}

void ImGui_FloatBarSetDefaults()
{
	FloatBarSetDefaults(flt_config);
}

void ImGui_FloatBarInit()
{
	if (flt_config.closest_max == 0)
		ImGui_FloatBarSetDefaults();
}

void ImGui_FloatBarConfigLoad()
{
	const struct Panel* panel = &g_state.panel_float;
	ImGui_FloatBarSetDefaults();

	FloatBarConfig &flt = flt_config;
	flt.draw_hp = config_get_int(panel->section, "DrawHP", flt.draw_hp) > 0;
	flt.draw_cls = config_get_int(panel->section, "DrawClass", flt.draw_cls) > 0;
	flt.draw_gotl = config_get_int(panel->section, "DrawGOTL", flt.draw_gotl) > 0;
	flt.mode_self = config_get_int(panel->section, "DrawSelf", flt.mode_self);
	flt.text_mode = config_get_int(panel->section, "TextMode", flt.text_mode);
	flt.cls_mode = config_get_int(panel->section, "ClassMode", flt.cls_mode);
	flt.col_mode = config_get_int(panel->section, "ColorPickerMode", flt.col_mode);
	flt.frame_round_mode = config_get_int(panel->section, "RoundMode", flt.frame_round_mode);
	flt.per_mode = config_get_int(panel->section, "PercentMode", flt.per_mode);
	flt.closest_max = config_get_int(panel->section, "PlayersMax", flt.closest_max);
	flt.frame_thick = config_get_float(panel->section, "FrameThickness", flt.frame_thick);
	flt.vert_adjust = config_get_float(panel->section, "VerticalAdjust", flt.vert_adjust);
	flt.rect_width = config_get_float(panel->section, "RectWidth", flt.rect_width);
	flt.rect_height_hp = config_get_float(panel->section, "RectHeightHP", flt.rect_height_hp);
	flt.rect_height_gotl = config_get_float(panel->section, "RectHeightGOTL", flt.rect_height_gotl);
	flt.icon_size = config_get_float(panel->section, "ClassRectSize", flt.icon_size);

	flt.colors[FLOAT_COL_BG] = config_get_int(panel->section, "ColClassBG", flt.colors[FLOAT_COL_BG]);
	flt.colors[FLOAT_COL_TEXT] = config_get_int(panel->section, "ColText", flt.colors[FLOAT_COL_TEXT]);
	flt.colors[FLOAT_COL_FRAME] = config_get_int(panel->section, "ColFrame", flt.colors[FLOAT_COL_FRAME]);
	flt.colors[FLOAT_COL_HP_LEFT] = config_get_int(panel->section, "ColHPLeft", flt.colors[FLOAT_COL_HP_LEFT]);
	flt.colors[FLOAT_COL_HP_LEFT90] = config_get_int(panel->section, "ColHPLeft90", flt.colors[FLOAT_COL_HP_LEFT]);
	flt.colors[FLOAT_COL_HP_RIGHT] = config_get_int(panel->section, "ColHPRight", flt.colors[FLOAT_COL_HP_RIGHT]);
	flt.colors[FLOAT_COL_DOWN_LEFT] = config_get_int(panel->section, "ColDownLeft", flt.colors[FLOAT_COL_DOWN_LEFT]);
	flt.colors[FLOAT_COL_DOWN_RIGHT] = config_get_int(panel->section, "ColDownRight", flt.colors[FLOAT_COL_DOWN_RIGHT]);
	flt.colors[FLOAT_COL_SHROUD_LEFT] = config_get_int(panel->section, "ColShroudLeft", flt.colors[FLOAT_COL_SHROUD_LEFT]);
	flt.colors[FLOAT_COL_SHROUD_RIGHT] = config_get_int(panel->section, "ColShroudRight", flt.colors[FLOAT_COL_SHROUD_RIGHT]);
	flt.colors[FLOAT_COL_GOTL_LEFT] = config_get_int(panel->section, "ColGOTLLeft", flt.colors[FLOAT_COL_GOTL_LEFT]);
	flt.colors[FLOAT_COL_GOTL_RIGHT] = config_get_int(panel->section, "ColGOTLRight", flt.colors[FLOAT_COL_GOTL_RIGHT]);
}

void ImGui_FloatBarConfigSave()
{
	const struct Panel* panel = &g_state.panel_float;
	config_set_int(panel->section, "Enabled", panel->enabled);

	FloatBarConfig &flt = flt_config;
	config_set_int(panel->section, "DrawHP", flt.draw_hp);
	config_set_int(panel->section, "DrawClass", flt.draw_cls);
	config_set_int(panel->section, "DrawGOTL", flt.draw_gotl);
	config_set_int(panel->section, "DrawSelf", flt.mode_self);
	config_set_int(panel->section, "TextMode", flt.text_mode);
	config_set_int(panel->section, "ClassMode", flt.cls_mode);
	config_set_int(panel->section, "ColorPickerMode", flt.col_mode);
	config_set_int(panel->section, "RoundMode", flt.frame_round_mode);
	config_set_int(panel->section, "PercentMode", flt.per_mode);
	config_set_int(panel->section, "PlayersMax", flt.closest_max);
	config_set_float(panel->section, "FrameThickness", flt.frame_thick);
	config_set_float(panel->section, "VerticalAdjust", flt.vert_adjust);
	config_set_float(panel->section, "RectWidth", flt.rect_width);
	config_set_float(panel->section, "RectHeightHP", flt.rect_height_hp);
	config_set_float(panel->section, "RectHeightGOTL", flt.rect_height_gotl);
	config_set_float(panel->section, "ClassRectSize", flt.icon_size);

	config_set_int(panel->section, "ColClassBG", flt.colors[FLOAT_COL_BG]);
	config_set_int(panel->section, "ColText", flt.colors[FLOAT_COL_TEXT]);
	config_set_int(panel->section, "ColFrame", flt.colors[FLOAT_COL_FRAME]);
	config_set_int(panel->section, "ColHPLeft", flt.colors[FLOAT_COL_HP_LEFT]);
	config_set_int(panel->section, "ColHPLeft90", flt.colors[FLOAT_COL_HP_LEFT90]);
	config_set_int(panel->section, "ColHPRight", flt.colors[FLOAT_COL_HP_RIGHT]);
	config_set_int(panel->section, "ColDownLeft", flt.colors[FLOAT_COL_DOWN_LEFT]);
	config_set_int(panel->section, "ColDownRight", flt.colors[FLOAT_COL_DOWN_RIGHT]);
	config_set_int(panel->section, "ColShroudLeft", flt.colors[FLOAT_COL_SHROUD_LEFT]);
	config_set_int(panel->section, "ColShroudRight", flt.colors[FLOAT_COL_SHROUD_RIGHT]);
	config_set_int(panel->section, "ColGOTLLeft", flt.colors[FLOAT_COL_GOTL_LEFT]);
	config_set_int(panel->section, "ColGOTLRight", flt.colors[FLOAT_COL_GOTL_RIGHT]);
}


static void DrawFloatBar(struct Character& c)
{
	POINT p;
	float y_adjust = 0.0f;
	FloatBarConfig &flt = flt_config;

	if (!flt.draw_hp && !flt.draw_cls && !flt.draw_gotl) return;
	if (c.aptr == 0 || c.cptr == 0) return;

	// Adjust the frame rouding based on user request
	if (flt.frame_round_mode == 0) flt.frame_round = 0.0f;
	else flt.frame_round = flt.frame_round_default;

	ImVec2 rect_hp = ImVec2(flt.rect_width, flt.rect_height_hp);
	ImVec2 rect_gotl = ImVec2(flt.rect_width, flt.rect_height_gotl);

	if (!project2d_agent(&p, &c.apos, c.aptr, g_now))
	//if (!project2d_mum(&p, &c.tpos, c.aptr, false))
		return;	// pt is out of screen bounds

	if (flt.vert_adjust > 0.0f)
		p.y -= (int)flt.vert_adjust;

	bool isDowned = Ch_IsDowned(c.cptr);
	bool isDead = !Ch_IsAlive(c.cptr) && !isDowned;
	if (isDead) return;

	if (flt.draw_hp) {

		y_adjust = rect_hp.y - 1.0f;
		float hp_per = (float)c.hp_val / c.hp_max;
		hp_per = ImClamp(hp_per, 0.0f, 100.0f);

		float dist = 0.0f;
		ImU32 col_hp_left = (hp_per < 0.90f) ? flt.colors[FLOAT_COL_HP_LEFT90] : flt.colors[FLOAT_COL_HP_LEFT];
		ImU32 col_hp_right = flt.colors[FLOAT_COL_HP_RIGHT];

		if (isDowned) {
			DrawRectPercentage(hp_per, ImVec2((float)p.x, (float)p.y), rect_hp,
				flt.colors[FLOAT_COL_DOWN_LEFT], flt.colors[FLOAT_COL_DOWN_RIGHT], flt.colors[FLOAT_COL_FRAME], flt.frame_round, flt.frame_thick > 0.0f);
		}
		else if (c.profession == GW2::PROFESSION_NECROMANCER && c.stance == GW2::STANCE_NECRO_SHROUD) {
			float per = hp_per;
			uintptr_t ptr = process_read_u64(c.cptr + OFF_CHAR_PROFESSION);
			if (ptr) {
				float shroud_cur = process_read_f32(ptr + OFF_PROFESSION_ENERGY_CUR);
				float shroud_max = process_read_f32(ptr + OFF_PROFESSION_ENERGY_MAX);
				per = shroud_cur / shroud_max;
			}
			ImVec2 rect_size(rect_hp.x, rect_hp.y / 2.0f);
			DrawRectPercentage(per, ImVec2((float)p.x, (float)p.y), rect_hp,
				flt.colors[FLOAT_COL_SHROUD_LEFT], flt.colors[FLOAT_COL_SHROUD_RIGHT], flt.colors[FLOAT_COL_FRAME], flt.frame_round, flt.frame_thick > 0.0f);
			DrawRectPercentage(hp_per, ImVec2((float)p.x, (float)p.y), rect_size,
				col_hp_left, col_hp_right, flt.colors[FLOAT_COL_FRAME], flt.frame_round, false);
			hp_per = per;
		}
		else {
			DrawRectPercentage(hp_per, ImVec2((float)p.x, (float)p.y), rect_hp,
				col_hp_left, col_hp_right, flt.colors[FLOAT_COL_FRAME], flt.frame_round, flt.frame_thick > 0.0f);
		}

		if (flt.text_mode == 0) ImGui::PushFont(proggyTiny);
		else ImGui::PushFont(proggyClean);
		DrawTextAndFrame(hp_per, ImVec2((float)p.x, (float)p.y), rect_hp,
			flt.colors[FLOAT_COL_TEXT], flt.colors[FLOAT_COL_FRAME],
			flt.frame_round, flt.frame_thick, flt.per_mode == 0, dist);
		ImGui::PopFont();
	}

	if (flt.draw_gotl) {

		static const int gotl_max = 5;
		uint32_t gotl_stacks = 0;
		uintptr_t buffBarPtr = process_read_u64(c.cbptr + OFF_CMBTNT_BUFFBAR);
		if (buffBarPtr) {

			BuffStacks stacks = { 0 };
			read_buff_array(buffBarPtr, &stacks);
			gotl_stacks = stacks.GOTL;
		}

		float y = (float)p.y + y_adjust;
		float per = (float)gotl_stacks / gotl_max;
		DrawRectPercentage(per, ImVec2((float)p.x, y), rect_gotl, flt.colors[FLOAT_COL_GOTL_LEFT],
			flt.colors[FLOAT_COL_GOTL_RIGHT], flt.colors[FLOAT_COL_FRAME], flt.frame_round);

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		float fraction = rect_gotl.x / gotl_max;
		float x = (float)p.x - rect_gotl.x / 2;
		for (int j = 0; j < gotl_max; j++) {
			draw_list->AddRect(ImVec2(x, y), ImVec2(x + fraction, y + rect_gotl.y), flt.colors[FLOAT_COL_FRAME], flt.frame_round, -1, flt.frame_thick);
			x += fraction;
		}

		y_adjust += rect_gotl.y;
	}

	if (flt.draw_cls) {

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 icon_size(flt.icon_size, flt.icon_size);
		float x = (flt.draw_hp || flt.draw_gotl) ? (float)p.x - rect_gotl.x / 2 - icon_size.x : (float)p.x - icon_size.x / 2;
		float y = (y_adjust < flt.icon_size) ? (float)p.y - (flt.icon_size- y_adjust)/2 : (float)p.y + (y_adjust - flt.icon_size)/2;
		ImVec2 pos(flt.frame_thick > 0.0f ? x+1.0f : x, y);

		draw_list->AddRectFilled(pos, pos + icon_size,
			flt.cls_mode == 1 ? ColorFromCharacter(&c, true) : flt.colors[FLOAT_COL_BG],
			flt.frame_round);

		if (flt.frame_thick > 0.0f)
			draw_list->AddRect(pos, pos + icon_size, flt.colors[FLOAT_COL_FRAME], flt.frame_round, -1, flt.frame_thick);

		if (flt.cls_mode != 1) {
			LPVOID img = ProfessionImageFromPlayer(&c);
			if (img) draw_list->AddImage(img, pos + ImVec2(1, 1), pos + icon_size - ImVec2(2, 2));
		}
	}
}

static void ShowFloatBarMenu()
{
	const struct Panel* panel = &g_state.panel_float;
	FloatBarConfig &flt = flt_config;
	bool dirty = false;

	if (ImGui::BeginMenu(LOCALTEXT(TEXT_FLOATBAR_TITLE)))
	{
		static const float offset_help = 160.0f;
		static const float offset = offset_help + 25.0f;
		static const float offset_radio = offset + 60.0f;
		dirty |= ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_GLOBAL_ENABLED), panel->bind.str, (bool*)&panel->enabled);
		ImGui::Separator();
		dirty |= ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_FLOATBAR_HP), "", &flt.draw_hp);
		dirty |= ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_FLOATBAR_GOTL), "", &flt.draw_gotl);
		dirty |= ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_FLOATBAR_CLASS), "", &flt.draw_cls);
		ImGui::Separator();

		if (ImGui::BeginMenu(LOCALTEXT(TEXT_GLOBAL_OPTIONS)))
		{

			if (ImGui::Button(LOCALTEXT(TEXT_GLOBAL_RESTORE_DEF))) {
				ImGui_FloatBarSetDefaults();
				dirty = true;
			}
			ImGui::Separator();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

			ImGui::Text(LOCALTEXT(TEXT_FLOATBAR_INC_SELF));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_FLOATBAR_INC_SELF_HELP));
			ImGui::SameLine(offset);
			dirty |= ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_YES, "##self"), &flt.mode_self, 1); ImGui::SameLine(offset_radio);
			dirty |= ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_NO, "##self"), &flt.mode_self, 0);
			ImGui::Separator();


			ImGui::Text(LOCALTEXT(TEXT_FLOATBAR_MAX_PL));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_FLOATBAR_MAX_PL_HELP));
			ImGui::SameLine(offset);
			if (ImGui::DragInt("##players", &flt.closest_max, 0.2f, 1, SQUAD_SIZE_MAX, "%.0f")) {
				if (flt.closest_max <= 0) flt.closest_max = 1;
				if (flt.closest_max > SQUAD_SIZE_MAX) flt.closest_max = SQUAD_SIZE_MAX;
				dirty = true;
			}

			ImGui::Text(LOCALTEXT(TEXT_FLOATBAR_BARVERT));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_FLOATBAR_BARVERT_HELP));
			ImGui::SameLine(offset);
			float slider_min = 0.0f;
			float slider_max = 500.0f;
			if (ImGui::DragFloat("##vert", &flt.vert_adjust, 1.00f, slider_min, slider_max, "%.0f")) {
				if (flt.vert_adjust <= slider_min) flt.vert_adjust = slider_min;
				if (flt.vert_adjust > slider_max) flt.vert_adjust = slider_max;
				dirty = true;
			}

			ImGui::Text(LOCALTEXT(TEXT_FLOATBAR_BARWIDTH));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_FLOATBAR_BARWIDTH_HELP));
			ImGui::SameLine(offset);
			slider_min = 30.0f;
			slider_max = 200.0f;
			if (ImGui::DragFloat("##width", &flt.rect_width, 0.8f, slider_min, slider_max, "%.0f")) {
				if (flt.rect_width <= slider_min) flt.rect_width = slider_min;
				if (flt.rect_width > slider_max) flt.rect_width = slider_max;
				dirty = true;
			}

			ImGui::Text(LOCALTEXT(TEXT_FLOATBAR_BARHEIGHT_HP));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_FLOATBAR_BARHEIGHT_HP_HELP));
			ImGui::SameLine(offset);
			slider_min = 5.0f;
			slider_max = 50.0f;
			if (ImGui::DragFloat("##hp", &flt.rect_height_hp, 0.2f, slider_min, slider_max, "%.0f")) {
				if (flt.rect_height_hp <= slider_min) flt.rect_height_hp = slider_min;
				if (flt.rect_height_hp > slider_max) flt.rect_height_hp = slider_max;
				dirty = true;
			}
			ImGui::Text(LOCALTEXT(TEXT_FLOATBAR_BARHEIGHT_GOTL));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_FLOATBAR_BARHEIGHT_GOTL_HELP));
			ImGui::SameLine(offset);
			if (ImGui::DragFloat("##gotl", &flt.rect_height_gotl, 0.2f, slider_min, slider_max, "%.0f")) {
				if (flt.rect_height_gotl <= slider_min) flt.rect_height_gotl = slider_min;
				if (flt.rect_height_gotl > slider_max) flt.rect_height_gotl = slider_max;
				dirty = true;
			}

			ImGui::Text(LOCALTEXT(TEXT_FLOATBAR_CLASS_RECT));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_FLOATBAR_CLASS_RECT_HELP));
			ImGui::SameLine(offset);
			slider_min = 5.0f;
			slider_max = 50.0f;
			if (ImGui::DragFloat("##icon", &flt.icon_size, 0.2f, slider_min, slider_max, "%.0f")) {
				if (flt.icon_size <= slider_min) flt.icon_size = slider_min;
				if (flt.icon_size > slider_max) flt.icon_size = slider_max;
				dirty = true;
			}

			ImGui::Text(LOCALTEXT(TEXT_FLOATBAR_CLASS_IND));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_FLOATBAR_CLASS_IND_HELP));
			ImGui::SameLine(offset);
			dirty |= ImGui::RadioButton(LOCALTEXT(TEXT_GLOBAL_ICON), &flt.cls_mode, 0); ImGui::SameLine(offset_radio);
			dirty |= ImGui::RadioButton(LOCALTEXT(TEXT_GLOBAL_COLOR), &flt.cls_mode, 1);

			ImGui::Text(LOCALTEXT(TEXT_FLOATBAR_TEXTPER_SIZE));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_FLOATBAR_TEXTPER_SIZE_HELP));
			ImGui::SameLine(offset);
			dirty |= ImGui::RadioButton(LOCALTEXT(TEXT_GLOBAL_FONT_SMALL), &flt.text_mode, 0); ImGui::SameLine(offset_radio);
			dirty |= ImGui::RadioButton(LOCALTEXT(TEXT_GLOBAL_FONT_NORMAL), &flt.text_mode, 1);

			ImGui::Text(LOCALTEXT(TEXT_FLOATBAR_TEXTPER_DRAW));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_FLOATBAR_TEXTPER_DRAW_HELP));
			ImGui::SameLine(offset);
			dirty |= ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_YES, "##per"), &flt.per_mode, 0); ImGui::SameLine(offset_radio);
			dirty |= ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_NO, "##per"), &flt.per_mode, 1);

			ImGui::Text(LOCALTEXT(TEXT_FLOATBAR_TEXTPER_FR_ROUND));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_FLOATBAR_TEXTPER_FR_ROUND_HELP));
			ImGui::SameLine(offset);
			dirty |= ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_YES, "##round"), &flt.frame_round_mode, 1); ImGui::SameLine(offset_radio);
			dirty |= ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_NO, "##round"), &flt.frame_round_mode, 0);

			ImGui::Text(LOCALTEXT(TEXT_FLOATBAR_TEXTPER_FR_THICK));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_FLOATBAR_TEXTPER_FR_THICK_HELP));
			ImGui::SameLine(offset);
			if (ImGui::SliderFloat("##thick", &flt.frame_thick, 0.0f, 5.0f, "%.0f")) dirty = true;

			ImGui::Separator();

			ImGui::Text(LOCALTEXT(TEXT_GLOBAL_COLORPICK_MODE));
			ImGui::SameLine(offset_help); ShowHelpMarker(LOCALTEXT(TEXT_GLOBAL_COLORPICK_MODE_HELP));
			ImGui::SameLine(offset);
			dirty |= ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_COLORPICK_RGB, "##coledit"), &flt.col_mode, 0); ImGui::SameLine(offset_radio);
			dirty |= ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_COLORPICK_HSV, "##coledit"), &flt.col_mode, 1); ImGui::SameLine(offset_radio + 60.0f);
			dirty |= ImGui::RadioButton(LOCALTEXT_FMT(TEXT_GLOBAL_COLORPICK_HEX, "##coledit"), &flt.col_mode, 2);

			static float btn_width = 60.0f;
			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_FLOATBAR_COL_FRAME), "", offset, flt.col_mode, btn_width, flt.colors[FLOAT_COL_FRAME], offset_help);
			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_FLOATBAR_COL_TEXTPER), "", offset, flt.col_mode, btn_width, flt.colors[FLOAT_COL_TEXT], offset_help);
			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_FLOATBAR_COL_CLASS_BG), "", offset, flt.col_mode, btn_width, flt.colors[FLOAT_COL_BG], offset_help);
			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_FLOATBAR_COL_HP_LEFT), "", offset, flt.col_mode, btn_width, flt.colors[FLOAT_COL_HP_LEFT], offset_help);
			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_FLOATBAR_COL_HP_90), "", offset, flt.col_mode, btn_width, flt.colors[FLOAT_COL_HP_LEFT90], offset_help);
			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_FLOATBAR_COL_HP_RIGHT), "", offset, flt.col_mode, btn_width, flt.colors[FLOAT_COL_HP_RIGHT], offset_help);
			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_FLOATBAR_COL_DOWN_LEFT), "", offset, flt.col_mode, btn_width, flt.colors[FLOAT_COL_DOWN_LEFT], offset_help);
			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_FLOATBAR_COL_DOWN_RIGHT), "", offset, flt.col_mode, btn_width, flt.colors[FLOAT_COL_DOWN_RIGHT], offset_help);
			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_FLOATBAR_COL_SHROUD_LEFT), "", offset, flt.col_mode, btn_width, flt.colors[FLOAT_COL_SHROUD_LEFT], offset_help);
			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_FLOATBAR_COL_SHROUD_RIGHT), "", offset, flt.col_mode, btn_width, flt.colors[FLOAT_COL_SHROUD_RIGHT], offset_help);
			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_FLOATBAR_COL_GOTL_LEFT), "", offset, flt.col_mode, btn_width, flt.colors[FLOAT_COL_GOTL_LEFT], offset_help);
			dirty |= ShowColorPickerMenu(LOCALTEXT(TEXT_FLOATBAR_COL_GOTL_RIGHT), "", offset, flt.col_mode, btn_width, flt.colors[FLOAT_COL_GOTL_RIGHT], offset_help);

			ImGui::PopStyleVar();
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}

	if (dirty) ImGui_FloatBarConfigSave();
}

void ImGui_FloatBars(struct Panel* panel,
	struct Character *player, struct Character *target,
	int center, struct GFX* ui, struct GFXTarget* set)
{
	if (!panel->enabled) return;

	FloatBarConfig &flt = flt_config;
	if (flt.draw_hp || flt.draw_gotl || flt.draw_cls) {

		static bool open = true;

		ImGuiWindowFlags fl = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;

		if (ImGui::Begin("##DummyWindow", &open, ImVec2((float)g_viewPort.Width, (float)g_viewPort.Height), 0.0f, fl)) {

			if (flt.mode_self) DrawFloatBar(*player);

			static ClosePlayer closest_arr[SQUAD_SIZE_MAX];
			memset(closest_arr, 0, sizeof(closest_arr));
			uint32_t closest_num = read_closest_x_players(player, closest_arr, flt_config.closest_max, g_now, flt.disp_mode);

			for (uint32_t i = 0; i < closest_num; ++i) {

				ClosePlayer *cp = &closest_arr[i];
				DrawFloatBar(cp->c);
			}
		}
		ImGui::End();
	}
}

#endif


void ImGui_HpAndDistance(struct Panel* panel,
	struct Character *player, struct Character *target,
	int center, struct GFX* ui, struct GFXTarget* set)
{
	static const int IMGUI_WINDOW_ADJUST = 8;

	if (!panel->enabled) return;

	ImGuiWindowFlags flags = NO_INPUT_FLAG |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::PushFont(proggyTiny);
	ImGui::PushStyleColor(ImGuiCol_Text, ImColor(IMGUI_WHITE));
	if (panel->cfg.col_visible[HP_ROW_PLAYER_HP] && player->hp_max > 0) {

		uint32_t hp = (uint32_t)ceilf(player->hp_val / (float)player->hp_max * 100.0f);
		hp = min(hp, 100);

		uint32_t num = 0;
		num += (hp > 9);
		num += (hp > 99);

		float x = (float)center - IMGUI_WINDOW_ADJUST - num * TINY_FONT_WIDTH / 2;
		float y = (float)(g_viewPort.Height - ui->hp - IMGUI_WINDOW_ADJUST);

		ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiSetCond_Always);
		if (ImGui::Begin("Player HP", &panel->cfg.col_visible[HP_ROW_PLAYER_HP], ImVec2(0, 0), 0.0f, flags)) {
			ImGui::TextColored(ImColor(bgdm_style.hp_colors[HP_ROW_PLAYER_HP]), "%u%%", hp);
		}
		ImGui::End();
	}

#if !(defined BGDM_TOS_COMPLIANT)
	if (panel->cfg.col_visible[HP_ROW_PORT_DIST] && g_player.combat.pd.is_weave && g_player.combat.pd.map_id == g_mum_mem->map_id) {

		//float dist = distance(g_player.combat.pd.pos_mum, g_mum_mem->avatar_pos, 0.0f);
		float dist = distance(&g_player.combat.pd.pos_agent, &player->apos, false);
		dist = min(dist, 99999);

		float x = (float)center - IMGUI_WINDOW_ADJUST + ui->utils.x;
		float y = (float)(g_viewPort.Height - IMGUI_WINDOW_ADJUST - ui->utils.y);

		ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiSetCond_Always);
		if (ImGui::Begin("Portal Distance", &panel->cfg.col_visible[HP_ROW_PORT_DIST], ImVec2(0, 0), /*panel->fAlpha*/0.0f, flags)) {

			ATL::CAtlStringA str;
			str.Format("%s %d (%.2fs)", LOCALTEXT(TEXT_PORTAL_DISTANCE), (uint32_t)dist,
				60.01f - (g_now - g_player.combat.pd.time) / 1000000.0f);

			float xpadding = 12.0f;
			float ypadding = 4.0f;
			//ImGuiStyle &style = ImGui::GetStyle();
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImVec2 size = ImGui::CalcTextSize(str.GetString());
			size.x += xpadding;
			size.y += ypadding;
			x += IMGUI_WINDOW_ADJUST - xpadding/2;
			y += IMGUI_WINDOW_ADJUST - ypadding/2;
			draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + size.x, y + size.y),
				ImColor(bgdm_style.hp_colors[HP_ROW_PORT_DIST_BG]), 0.0f/*style.WindowRounding*/);
			ImGui::TextColored(ImColor(bgdm_style.hp_colors[HP_ROW_PORT_DIST]), str.GetString());
		}
		ImGui::End();
	}
#endif

	bool drawTarget = true;
	if (!target->aptr || target->hp_max == 0)
		drawTarget = false;

	if (drawTarget && (target->is_player || Ch_IsClone(target->cptr))) {
#if (defined BGDM_TOS_COMPLIANT)
		drawTarget = false;
#else
		if (target->attitude != CHAR_ATTITUDE_FRIENDLY) {
			drawTarget = false;
		}
#endif
	}

	if (!drawTarget) {
		ImGui::PopStyleColor();
		ImGui::PopFont();
		return;
	}

	if (panel->cfg.col_visible[HP_ROW_TARGET_HP]) {

		//u32 hp = (u32)(target->hp_val / (f32)target->hp_max * 100.f);
		float x = (float)center - IMGUI_WINDOW_ADJUST + set->hp.x;
		float y = (float)(set->hp.y - IMGUI_WINDOW_ADJUST);
		float hp = (float)target->hp_val / (float)target->hp_max * 100.f;
		ImClamp(hp, 0.0f, 100.0f);

		CAtlStringA hpstr;
		if (bgdm_style.hp_percent_mode) {
			if (bgdm_style.hp_precision_mode == 1) {

				if (bgdm_style.hp_fixed_mode == 1) {
					if (hp < 10.0f) hpstr.Format("[  %.2f%%] ", hp);
					else if (hp < 100.0f) hpstr.Format("[ %.2f%%] ", hp);
					else hpstr.Format("[%.2f%%] ", hp);
				}
				else hpstr.Format("[%.2f%%] ", hp);
			}
			else {
				if (bgdm_style.hp_fixed_mode == 1) hpstr.Format("[%3d%%] ", (int)roundf(hp));
				else hpstr.Format("[%d%%] ", (int)roundf(hp));
			}
		}

		if (bgdm_style.hp_comma_mode == 1) {
			char strHpVal[32] = { 0 };
			char strHpMax[32] = { 0 };
			hpstr.AppendFormat("%s / %s",
				sprintf_num_commas(target->hp_val, strHpVal, sizeof(strHpVal)),
				sprintf_num_commas(target->hp_max, strHpMax, sizeof(strHpMax)));
		}
		else {
			hpstr.AppendFormat("%d / %d", target->hp_val, target->hp_max);
		}

		ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiSetCond_Always);
		if (ImGui::Begin("Target HP", &panel->cfg.col_visible[HP_ROW_TARGET_HP], ImVec2(0, 0), 0.0f, flags)) {
			ImGui::TextColored(ImColor(bgdm_style.hp_colors[HP_ROW_TARGET_HP]), "%s", hpstr);
		}
		ImGui::End();
	}

	if (panel->cfg.col_visible[HP_ROW_TARGET_DIST]) {

		u32 dist = (u32)distance(&player->apos, &target->apos, false);
		dist = min(dist, 9999);

		u32 num = 2;
		num += (dist > 9);
		num += (dist > 99);
		num += (dist > 999);

		float x = (float)center - IMGUI_WINDOW_ADJUST + set->dst - num * TINY_FONT_WIDTH / 2;
		float y = (float)(set->hp.y - IMGUI_WINDOW_ADJUST);

		ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiSetCond_Always);
		if (ImGui::Begin("Target Distance", &panel->cfg.col_visible[HP_ROW_TARGET_DIST], ImVec2(0, 0), 0.0f, flags)) {
			ImGui::TextColored(ImColor(bgdm_style.hp_colors[HP_ROW_TARGET_DIST]), "%d", dist);
		}
		ImGui::End();
	}


	if (panel->cfg.col_visible[HP_ROW_TARGET_BB]) {

		if (target->bptr == 0 || (target->bb_state != BREAKBAR_READY && target->bb_state != BREAKBAR_REGEN))
		{
			//return;
		}
		else
		{

			u32 bb = target->bb_value;

			u32 num = 2;
			num += (bb > 9);
			num += (bb > 99);

			float x = (float)center - IMGUI_WINDOW_ADJUST + set->bb.x - num * TINY_FONT_WIDTH / 2;
			float y = (float)(set->bb.y - IMGUI_WINDOW_ADJUST);

			ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiSetCond_Always);
			if (ImGui::Begin("Target Breakbar", &panel->cfg.col_visible[HP_ROW_TARGET_BB], ImVec2(0, 0), 0.0f, flags)) {
				ImGui::TextColored(ImColor(bgdm_style.hp_colors[HP_ROW_TARGET_BB]), "%u%%", bb);
			}
			ImGui::End();
		}
	}

	ImGui::PopStyleColor();
	ImGui::PopFont();
}


static inline float round_up_to_multiple_of(f32 f, f32 multiple)
{
	f32 remainder = (f32)fmod(fabs(f), multiple);
	if (remainder == 0)
		return f;

	if (f < 0)
		return -((float)fabs(f) - remainder);
	else
		return f + multiple - remainder;
}

void ImGui_Compass(struct Panel* panel, int x, int y, int w)
{
#define COMPASS_STR_LEN	38
	static char cardinals_str[COMPASS_STR_LEN + 1];
	static char indicator_long_str[COMPASS_STR_LEN + 1];
	static char indicator_short_str[COMPASS_STR_LEN + 1];
	static const f32 compass_inc_size = 5.0f;
	static float last_seen_angle = 181.0f;

	if (!g_ptrs.pMumble) return;

	ImColor white(ImColor(bgdm_colors[BGDM_COL_TEXT1].col));
	ImColor gray(ImColor(bgdm_colors[BGDM_COL_TEXT2].col));
	ImColor cyan(ImColor(bgdm_colors[BGDM_COL_TEXT3].col));

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
	if (IsDisableInputOrActionCam()) flags |= NO_INPUT_FLAG;

	switch (bgdm_style.compass_font) {
	case(1):
		ImGui::PushFont(proggyClean);
		break;
	case(2):
		ImGui::PushFont(tinyFont != proggyTiny ? tinyFont : defaultFont);
		break;
	case(3):
		ImGui::PushFont(defaultFont);
		break;
	default:
		ImGui::PushFont(proggyTiny);
		break;
	}

	ImGui::SetNextWindowPos(ImVec2((float)x, (float)y), ImGuiSetCond_FirstUseEver);
	if (ImGui::Begin("Compass", (bool*)&panel->enabled, ImVec2(0, 0), panel->fAlpha, flags))
	{

		f32 angle = get_cam_direction(((LinkedMem*)g_ptrs.pMumble)->cam_front);
		f32 start_angle = angle - (COMPASS_STR_LEN / 2) * compass_inc_size;
		if (start_angle < -180) {
			start_angle += 360;
		}
		f32 rounded_angle = round_up_to_multiple_of(start_angle, compass_inc_size);

		// redraw the strings only if cam changed
		if (last_seen_angle == 181.00f ||
			last_seen_angle != angle) {

			// save angle
			last_seen_angle = angle;

			// reset the current compass strings
			memset(&cardinals_str[0], ' ', COMPASS_STR_LEN); cardinals_str[COMPASS_STR_LEN] = 0;
			memset(&indicator_long_str[0], ' ', COMPASS_STR_LEN); indicator_long_str[COMPASS_STR_LEN] = 0;
			memset(&indicator_short_str[0], ' ', COMPASS_STR_LEN); indicator_short_str[COMPASS_STR_LEN] = 0;

			// round up to nearest compass inc multiple
			// and adjust the cursor relative to starting position
			f32 cur_angle = rounded_angle;
			for (int i = 0; i < COMPASS_STR_LEN; i++)
			{
				f32 next_angle = cur_angle + compass_inc_size;
				if (next_angle > 180) {
					next_angle -= 360;
				}

				if (fabs(cur_angle) == 180.00) {
					cardinals_str[i] = 'S';
				}
				else if (cur_angle == -90.00) {
					cardinals_str[i] = 'W';
				}
				else if (cur_angle == 0.00) {
					cardinals_str[i] = 'N';
				}
				else if (cur_angle == 90.00) {
					cardinals_str[i] = 'E';
				}
				else if (fmod(cur_angle, compass_inc_size * 3) == 0) {
					indicator_long_str[i] = '|';
				}
				else {
					indicator_short_str[i] = '|';
				}

				cur_angle = next_angle;
			}
		}

		ImGui::SameLine(1);
		ImGui::TextColored(cyan, cardinals_str); ImGui::SameLine(1);
		ImGui::TextColored(white, indicator_long_str); ImGui::SameLine(1);
		ImGui::TextColored(gray, indicator_short_str);
	}

	ImGui::End();
	ImGui::PopFont();
}

static __inline const char* GdpsAcronymFromColumn(int col)
{
	switch (col)
	{
	case(GDPS_COL_NAME): return LOCALTEXT(TEXT_GDPS_COLAC_NAME);
	case(GDPS_COL_CLS): return LOCALTEXT(TEXT_GDPS_COLAC_CLASS);
	case(GDPS_COL_DPS): return LOCALTEXT(TEXT_GDPS_COLAC_DPS);
	case(GDPS_COL_PER): return LOCALTEXT(TEXT_GDPS_COLAC_DMG_PER);
	case(GDPS_COL_DMGOUT): return LOCALTEXT(TEXT_GDPS_COLAC_DMG_OUT);
	case(GDPS_COL_DMGIN): return LOCALTEXT(TEXT_GDPS_COLAC_DNG_IN);
	case(GDPS_COL_HPS): return LOCALTEXT(TEXT_GDPS_COLAC_HPS);
	case(GDPS_COL_HEAL): return LOCALTEXT(TEXT_GDPS_COLAC_HEAL_OUT);
	case(GDPS_COL_TIME): return LOCALTEXT(TEXT_GDPS_COLAC_TIME);
	};
	return NULL;
}
static const char * GdpsTextFromColumn(int col)
{
	switch (col)
	{
	case(GDPS_COL_NAME): return LOCALTEXT(TEXT_GDPS_COL_NAME);
	case(GDPS_COL_CLS): return LOCALTEXT(TEXT_GDPS_COL_CLASS);
	case(GDPS_COL_DPS): return LOCALTEXT(TEXT_GDPS_COL_DPS);
	case(GDPS_COL_PER): return LOCALTEXT(TEXT_GDPS_COL_DMG_PER);
	case(GDPS_COL_DMGOUT): return LOCALTEXT(TEXT_GDPS_COL_DMG_OUT);
	case(GDPS_COL_DMGIN): return LOCALTEXT(TEXT_GDPS_COL_DNG_IN);
	case(GDPS_COL_HPS): return LOCALTEXT(TEXT_GDPS_COL_HPS);
	case(GDPS_COL_HEAL): return LOCALTEXT(TEXT_GDPS_COL_HEAL_OUT);
	case(GDPS_COL_TIME): return LOCALTEXT(TEXT_GDPS_COL_TIME);
	};
	return NULL;
}

static const char * BuffTextFromColumn(int col)
{
#define BUFF_COL_LOCALTEXT(x)	case(##x): return LOCALTEXT(TEXT_##x)
	switch (col)
	{
		BUFF_COL_LOCALTEXT(BUFF_COL_NAME);
		BUFF_COL_LOCALTEXT(BUFF_COL_CLS);
		BUFF_COL_LOCALTEXT(BUFF_COL_SUB);
		BUFF_COL_LOCALTEXT(BUFF_COL_DOWN);
		BUFF_COL_LOCALTEXT(BUFF_COL_SCLR);
		BUFF_COL_LOCALTEXT(BUFF_COL_SEAW);
		BUFF_COL_LOCALTEXT(BUFF_COL_PROT);
		BUFF_COL_LOCALTEXT(BUFF_COL_QUIK);
		BUFF_COL_LOCALTEXT(BUFF_COL_ALAC);
		BUFF_COL_LOCALTEXT(BUFF_COL_FURY);
		BUFF_COL_LOCALTEXT(BUFF_COL_MIGHT);
		BUFF_COL_LOCALTEXT(BUFF_COL_GOTL);
		BUFF_COL_LOCALTEXT(BUFF_COL_GLEM);
		BUFF_COL_LOCALTEXT(BUFF_COL_VIGOR);
		BUFF_COL_LOCALTEXT(BUFF_COL_SWIFT);
		BUFF_COL_LOCALTEXT(BUFF_COL_STAB);
		BUFF_COL_LOCALTEXT(BUFF_COL_RETAL);
		BUFF_COL_LOCALTEXT(BUFF_COL_RESIST);
		BUFF_COL_LOCALTEXT(BUFF_COL_REGEN);
		BUFF_COL_LOCALTEXT(BUFF_COL_AEGIS);
		BUFF_COL_LOCALTEXT(BUFF_COL_BANS);
		BUFF_COL_LOCALTEXT(BUFF_COL_BAND);
		BUFF_COL_LOCALTEXT(BUFF_COL_BANT);
		BUFF_COL_LOCALTEXT(BUFF_COL_EA);
		BUFF_COL_LOCALTEXT(BUFF_COL_SPOTTER);
		BUFF_COL_LOCALTEXT(BUFF_COL_FROST);
		BUFF_COL_LOCALTEXT(BUFF_COL_SUN);
		BUFF_COL_LOCALTEXT(BUFF_COL_STORM);
		BUFF_COL_LOCALTEXT(BUFF_COL_STONE);
		BUFF_COL_LOCALTEXT(BUFF_COL_ENGPP);
		BUFF_COL_LOCALTEXT(BUFF_COL_REVNR);
		BUFF_COL_LOCALTEXT(BUFF_COL_REVAP);
		BUFF_COL_LOCALTEXT(BUFF_COL_REVRD);
		BUFF_COL_LOCALTEXT(BUFF_COL_ELESM);
		BUFF_COL_LOCALTEXT(BUFF_COL_GRDSN);
		BUFF_COL_LOCALTEXT(BUFF_COL_NECVA);
		BUFF_COL_LOCALTEXT(BUFF_COL_TIME);
	}
	return NULL;
}

static LPVOID BuffIconFromColumn(int col)
{
	LPDIRECT3DTEXTURE9 *arr = BUFF_ICONS;
	if (g_state.icon_pack_no == 1)
		arr = STD_BUFF_ICONS_C;
	else if (g_state.icon_pack_no == 2)
		arr = STD_BUFF_ICONS_D;

	switch (col)
	{
	case(BUFF_COL_CLS): return BUFF_ICONS[IDB_PNG_CLASS - IDB_PNG_BUFF_START];
	case(BUFF_COL_SUB): return BUFF_ICONS[IDB_PNG_SUBGROUP - IDB_PNG_BUFF_START];
	case(BUFF_COL_DOWN): {
		if (g_state.use_downed_enemy_icon) return BUFF_ICONS[IDB_PNG_DOWNED_ENEMY - IDB_PNG_BUFF_START];
		else return BUFF_ICONS[IDB_PNG_DOWNED - IDB_PNG_BUFF_START]; }
	case(BUFF_COL_SCLR): return BUFF_ICONS[IDB_PNG_BUFF_SCHOLAR - IDB_PNG_BUFF_START];
	case(BUFF_COL_SEAW): {
		if (g_state.use_seaweed_icon) return BUFF_ICONS[IDB_PNG_BUFF_SEAWEED_SALAD - IDB_PNG_BUFF_START];
		else return arr[IDB_PNG_BUFF_MOVEMENT - IDB_PNG_BUFF_START]; }
	case(BUFF_COL_PROT): return arr[IDB_PNG_BUFF_PROTECTION - IDB_PNG_BUFF_START];
	case(BUFF_COL_QUIK): return arr[IDB_PNG_BUFF_QUICKNESS - IDB_PNG_BUFF_START];
	case(BUFF_COL_FURY): return arr[IDB_PNG_BUFF_FURY - IDB_PNG_BUFF_START];
	case(BUFF_COL_MIGHT): return arr[IDB_PNG_BUFF_MIGHT - IDB_PNG_BUFF_START];
	case(BUFF_COL_VIGOR): return arr[IDB_PNG_BUFF_VIGOR - IDB_PNG_BUFF_START];
	case(BUFF_COL_SWIFT): return arr[IDB_PNG_BUFF_SWIFTNESS - IDB_PNG_BUFF_START];
	case(BUFF_COL_STAB): return arr[IDB_PNG_BUFF_STABILITY - IDB_PNG_BUFF_START];
	case(BUFF_COL_RETAL): return arr[IDB_PNG_BUFF_RETALIATION - IDB_PNG_BUFF_START];
	case(BUFF_COL_RESIST): return arr[IDB_PNG_BUFF_RESISTANCE - IDB_PNG_BUFF_START];
	case(BUFF_COL_REGEN): return arr[IDB_PNG_BUFF_REGENERATION - IDB_PNG_BUFF_START];
	case(BUFF_COL_AEGIS): return arr[IDB_PNG_BUFF_AEGIS - IDB_PNG_BUFF_START];
	case(BUFF_COL_ALAC): return BUFF_ICONS[IDB_PNG_BUFF_ALACRITY - IDB_PNG_BUFF_START];
	case(BUFF_COL_BANS): return BUFF_ICONS[IDB_PNG_BUFF_BANNER_STRENGTH - IDB_PNG_BUFF_START];
	case(BUFF_COL_BAND): return BUFF_ICONS[IDB_PNG_BUFF_BANNER_DISCIPLINE - IDB_PNG_BUFF_START];
	case(BUFF_COL_BANT): return BUFF_ICONS[IDB_PNG_BUFF_BANNER_TACTICS - IDB_PNG_BUFF_START];
	case(BUFF_COL_EA): return BUFF_ICONS[IDB_PNG_BUFF_EMPOWER_ALLIES - IDB_PNG_BUFF_START];
	case(BUFF_COL_GOTL): return BUFF_ICONS[IDB_PNG_BUFF_GRACE_OF_THE_LAND - IDB_PNG_BUFF_START];
	case(BUFF_COL_GLEM): return BUFF_ICONS[IDB_PNG_BUFF_GLYPH_EMPOWERMENT - IDB_PNG_BUFF_START];
	case(BUFF_COL_SPOTTER): return BUFF_ICONS[IDB_PNG_BUFF_SPOTTER - IDB_PNG_BUFF_START];
	case(BUFF_COL_FROST): return BUFF_ICONS[IDB_PNG_BUFF_SPIRIT_FROST - IDB_PNG_BUFF_START];
	case(BUFF_COL_SUN): return BUFF_ICONS[IDB_PNG_BUFF_SPIRIT_SUN - IDB_PNG_BUFF_START];
	case(BUFF_COL_STORM): return BUFF_ICONS[IDB_PNG_BUFF_SPIRIT_STORM - IDB_PNG_BUFF_START];
	case(BUFF_COL_STONE): return BUFF_ICONS[IDB_PNG_BUFF_SPIRIT_STONE - IDB_PNG_BUFF_START];
	case(BUFF_COL_ENGPP): return BUFF_ICONS[IDB_PNG_BUFF_PINPOINT_DISTRIBUTION - IDB_PNG_BUFF_START];
	case(BUFF_COL_REVNR): return BUFF_ICONS[IDB_PNG_BUFF_FACET_OF_NATURE - IDB_PNG_BUFF_START];
	case(BUFF_COL_REVAP): return BUFF_ICONS[IDB_PNG_BUFF_ASSASSIN_PRESENCE - IDB_PNG_BUFF_START];
	case(BUFF_COL_REVRD): return BUFF_ICONS[IDB_PNG_BUFF_RITE_OF_GREAT_DWARF - IDB_PNG_BUFF_START];
	case(BUFF_COL_ELESM): return BUFF_ICONS[IDB_PNG_BUFF_SOOTHING_MIST - IDB_PNG_BUFF_START];
	case(BUFF_COL_GRDSN): return BUFF_ICONS[IDB_PNG_BUFF_STRENGTH_IN_NUMBERS - IDB_PNG_BUFF_START];
	case(BUFF_COL_NECVA): return BUFF_ICONS[IDB_PNG_BUFF_VAMPIRIC_AURA - IDB_PNG_BUFF_START];
	case(BUFF_COL_TIME): return BUFF_ICONS[IDB_PNG_TIME - IDB_PNG_BUFF_START];
	}
	return NULL;
}

static const char* ProfessionName(uint32_t profession, bool hasElite)
{
	const char* ret = NULL;
	switch (profession)
	{
	case(GW2::PROFESSION_GUARDIAN):
		ret = hasElite ? LOCALTEXT(TEXT_PROF_DRAGONHUNTER) : LOCALTEXT(TEXT_PROF_GUARDIAN);
		break;
	case(GW2::PROFESSION_WARRIOR):
		ret = hasElite ? LOCALTEXT(TEXT_PROF_BERSERKER) : LOCALTEXT(TEXT_PROF_WARRIOR);
		break;
	case(GW2::PROFESSION_ENGINEER):
		ret = hasElite ? LOCALTEXT(TEXT_PROF_SCRAPPER) : LOCALTEXT(TEXT_PROF_ENGINEER);
		break;
	case(GW2::PROFESSION_RANGER):
		ret = hasElite ? LOCALTEXT(TEXT_PROF_DRUID) : LOCALTEXT(TEXT_PROF_RANGER);
		break;
	case(GW2::PROFESSION_THIEF):
		ret = hasElite ? LOCALTEXT(TEXT_PROF_DAREDEVIL) : LOCALTEXT(TEXT_PROF_THIEF);
		break;
	case(GW2::PROFESSION_ELEMENTALIST):
		ret = hasElite ? LOCALTEXT(TEXT_PROF_TEMPEST) : LOCALTEXT(TEXT_PROF_ELEMENTALIST);
		break;
	case(GW2::PROFESSION_MESMER):
		ret = hasElite ? LOCALTEXT(TEXT_PROF_CHRONO) : LOCALTEXT(TEXT_PROF_MESMER);
		break;
	case(GW2::PROFESSION_NECROMANCER):
		ret = hasElite ? LOCALTEXT(TEXT_PROF_REAPER) : LOCALTEXT(TEXT_PROF_NECROMANCER);
		break;
	case(GW2::PROFESSION_REVENANT):
		ret = hasElite ? LOCALTEXT(TEXT_PROF_HERALD) : LOCALTEXT(TEXT_PROF_REVENANT);
		break;
	default:
		ret = LOCALTEXT(TEXT_GLOBAL_NONE);
	}
	return ret;
}

static LPVOID ProfessionImage(uint32_t profession, bool hasElite)
{
	LPVOID ret = NULL;
	switch (profession)
	{
	case(GW2::PROFESSION_GUARDIAN):
		ret = hasElite ?
			PROF_IMAGES[IDB_PNG_DRAGONHUNTER - IDB_PNG_PROF_START] :
			PROF_IMAGES[IDB_PNG_GUARDIAN - IDB_PNG_PROF_START];
		break;
	case(GW2::PROFESSION_WARRIOR):
		ret = hasElite ?
			PROF_IMAGES[IDB_PNG_BERSERKER - IDB_PNG_PROF_START] :
			PROF_IMAGES[IDB_PNG_WARRIOR - IDB_PNG_PROF_START];
		break;
	case(GW2::PROFESSION_ENGINEER):
		ret = hasElite ?
			PROF_IMAGES[IDB_PNG_SCRAPPER - IDB_PNG_PROF_START] :
			PROF_IMAGES[IDB_PNG_ENGINEER - IDB_PNG_PROF_START];
		break;
	case(GW2::PROFESSION_RANGER):
		ret = hasElite ?
			PROF_IMAGES[IDB_PNG_DRUID - IDB_PNG_PROF_START] :
			PROF_IMAGES[IDB_PNG_RANGER - IDB_PNG_PROF_START];
		break;
	case(GW2::PROFESSION_THIEF):
		ret = hasElite ?
			PROF_IMAGES[IDB_PNG_DAREDEVIL - IDB_PNG_PROF_START] :
			PROF_IMAGES[IDB_PNG_THIEF - IDB_PNG_PROF_START];
		break;
	case(GW2::PROFESSION_ELEMENTALIST):
		ret = hasElite ?
			PROF_IMAGES[IDB_PNG_TEMPEST - IDB_PNG_PROF_START] :
			PROF_IMAGES[IDB_PNG_ELEMENTALIST - IDB_PNG_PROF_START];
		break;
	case(GW2::PROFESSION_MESMER):
		ret = hasElite ?
			PROF_IMAGES[IDB_PNG_CHRONOMANCER - IDB_PNG_PROF_START] :
			PROF_IMAGES[IDB_PNG_MESMER - IDB_PNG_PROF_START];
		break;
	case(GW2::PROFESSION_NECROMANCER):
		ret = hasElite ?
			PROF_IMAGES[IDB_PNG_REAPER - IDB_PNG_PROF_START] :
			PROF_IMAGES[IDB_PNG_NECROMANCER - IDB_PNG_PROF_START];
		break;
	case(GW2::PROFESSION_REVENANT):
		ret = hasElite ?
			PROF_IMAGES[IDB_PNG_HERALD - IDB_PNG_PROF_START] :
			PROF_IMAGES[IDB_PNG_REVENANT - IDB_PNG_PROF_START];
		break;
	}
	return ret;
}

static LPVOID ProfessionImageFromPlayer(const struct Character* player)
{
	if (!player) return NULL;
	bool hasElite = Pl_HasEliteSpec(player->pptr);
	return ProfessionImage(player->profession, hasElite);
}

void ImGui_StyleSetDefaults()
{
	// Defaults
	bgdm_style.global_alpha = 1.0f;
	bgdm_style.col_edit_mode = 2;
	bgdm_style.round_windows = 1;
	bgdm_style.compass_font = 0;
	bgdm_style.hp_comma_mode = !g_state.hpCommas_disable;
	bgdm_style.hp_fixed_mode = 0;
	bgdm_style.hp_precision_mode = 0;
	bgdm_style.hp_percent_mode = 1;

	ImColor white(IMGUI_WHITE);
	bgdm_style.hp_colors[HP_ROW_PLAYER_HP] = white;
	bgdm_style.hp_colors[HP_ROW_TARGET_HP] = white;
	bgdm_style.hp_colors[HP_ROW_TARGET_DIST] = white;
	bgdm_style.hp_colors[HP_ROW_TARGET_BB] = white;
	bgdm_style.hp_colors[HP_ROW_PORT_DIST] = white;
	bgdm_style.hp_colors[HP_ROW_PORT_DIST_BG] = ImColor(0.0f, 0.0f, 0.0f, 0.75f);

	ImGuiStyle default_style;
	for (int i = 0; i < ARRAYSIZE(bgdm_colors); i++) {
		int idx = bgdm_colors[i].idx;
		if (idx >= 0 && idx < ImGuiCol_COUNT) {
			bgdm_colors[i].col = ImColor(default_style.Colors[idx]);
		}
	}

	bgdm_colors[BGDM_COL_TEXT1].col = ImColor(IMGUI_WHITE);
	bgdm_colors[BGDM_COL_TEXT2].col = ImColor(IMGUI_GRAY);
	bgdm_colors[BGDM_COL_TEXT3].col = ImColor(IMGUI_CYAN);
	bgdm_colors[BGDM_COL_TEXT_PLAYER].col = ImColor(IMGUI_WHITE);
	bgdm_colors[BGDM_COL_TEXT_PLAYER_SELF].col = ImColor(IMGUI_CYAN);
	bgdm_colors[BGDM_COL_TEXT_PLAYER_DEAD].col = ImColor(IMGUI_RED);
	bgdm_colors[BGDM_COL_TEXT_TARGET].col = ImColor(IMGUI_RED);
	bgdm_colors[BGDM_COL_GRAD_START].col = ImColor(IMGUI_CYAN);
	bgdm_colors[BGDM_COL_GRAD_END].col = ImColor(IMGUI_BLACK);
	bgdm_colors[BGDM_COL_TARGET_LOCK_BG].col = ImColor(IMGUI_RED);

	bgdm_colors_default.RemoveAll();
	bgdm_colors_default.SetCount(ARRAYSIZE(bgdm_colors));
	for (int i = 0; i < ARRAYSIZE(bgdm_colors); i++) {
		bgdm_colors_default[i] = bgdm_colors[i];
	}

	for (int j = 0; j < GW2::PROFESSION_END; j++) {
		switch (j)
		{
		case (GW2::PROFESSION_GUARDIAN):
			prof_colors[0][j] = ImColor(IMGUI_CYAN);
			break;
		case (GW2::PROFESSION_WARRIOR):
			prof_colors[0][j] = ImColor(IMGUI_ORANGE);
			break;
		case (GW2::PROFESSION_ENGINEER):
			prof_colors[0][j] = ImColor(IMGUI_LIGHT_PURPLE);
			break;
		case (GW2::PROFESSION_RANGER):
			prof_colors[0][j] = ImColor(IMGUI_GREEN);
			break;
		case (GW2::PROFESSION_THIEF):
			prof_colors[0][j] = ImColor(IMGUI_RED);
			break;
		case (GW2::PROFESSION_ELEMENTALIST):
			prof_colors[0][j] = ImColor(IMGUI_YELLOW);
			break;
		case (GW2::PROFESSION_MESMER):
			prof_colors[0][j] = ImColor(IMGUI_PINK);
			break;
		case (GW2::PROFESSION_NECROMANCER):
			prof_colors[0][j] = ImColor(IMGUI_DARK_GREEN);
			break;
		case (GW2::PROFESSION_REVENANT):
			prof_colors[0][j] = ImColor(IMGUI_LIGHT_BLUE);
			break;
		case (GW2::PROFESSION_NONE):
		case (GW2::PROFESSION_END):
		default:
			break;
		}
		prof_colors[1][j] = prof_colors[0][j];
	}
}

void ImGui_StyleConfigLoad()
{
	const ImGuiStyle default_style;
	ImGuiStyle &style = ImGui::GetStyle();
	ImGui_StyleSetDefaults();

	g_state.show_metrics_bgdm = config_get_int(CONFIG_SECTION_OPTIONS, "ShowMetricsBGDM", 1) == 1;

	style.Alpha = bgdm_style.global_alpha = config_get_float(CONFIG_SECTION_STYLE, "GlobalAlpha", bgdm_style.global_alpha);
	bgdm_style.col_edit_mode = config_get_int(CONFIG_SECTION_STYLE, "ColorEditMode", bgdm_style.col_edit_mode);
	bgdm_style.round_windows = config_get_int(CONFIG_SECTION_STYLE, "WindowRounding", bgdm_style.round_windows);
	if (bgdm_style.round_windows == 1) style.WindowRounding = default_style.WindowRounding;
	else style.WindowRounding = 0.0f;

	bgdm_style.compass_font = config_get_int(CONFIG_SECTION_STYLE, "CompassFont", bgdm_style.compass_font);

	bgdm_style.hp_comma_mode = config_get_int(CONFIG_SECTION_STYLE, "TargetHPComma", bgdm_style.hp_comma_mode);
	bgdm_style.hp_fixed_mode = config_get_int(CONFIG_SECTION_STYLE, "TargetHPPerFixed", bgdm_style.hp_fixed_mode);
	bgdm_style.hp_percent_mode = config_get_int(CONFIG_SECTION_STYLE, "TargetHPPerOnOff", bgdm_style.hp_percent_mode);
	bgdm_style.hp_precision_mode = config_get_int(CONFIG_SECTION_STYLE, "TargetHPPrecision", bgdm_style.hp_precision_mode);

	bgdm_style.hp_colors[HP_ROW_PLAYER_HP] = config_get_int(CONFIG_SECTION_STYLE, "PlayerHPColor", bgdm_style.hp_colors[HP_ROW_PLAYER_HP]);
	bgdm_style.hp_colors[HP_ROW_TARGET_HP] = config_get_int(CONFIG_SECTION_STYLE, "TargetHPColor", bgdm_style.hp_colors[HP_ROW_TARGET_HP]);
	bgdm_style.hp_colors[HP_ROW_TARGET_DIST] = config_get_int(CONFIG_SECTION_STYLE, "TargetBBColor", bgdm_style.hp_colors[HP_ROW_TARGET_DIST]);
	bgdm_style.hp_colors[HP_ROW_TARGET_BB] = config_get_int(CONFIG_SECTION_STYLE, "TargetDistColor", bgdm_style.hp_colors[HP_ROW_TARGET_BB]);
	bgdm_style.hp_colors[HP_ROW_PORT_DIST] = config_get_int(CONFIG_SECTION_STYLE, "PortDistColor", bgdm_style.hp_colors[HP_ROW_PORT_DIST]);
	bgdm_style.hp_colors[HP_ROW_PORT_DIST_BG] = config_get_int(CONFIG_SECTION_STYLE, "PortDistColorBG", bgdm_style.hp_colors[HP_ROW_PORT_DIST_BG]);

	for (int i = 0; i < ARRAYSIZE(bgdm_colors); i++) {
		bgdm_colors[i].col = config_get_int(CONFIG_SECTION_STYLE, bgdm_colors[i].cfg, bgdm_colors[i].col);
		int idx = bgdm_colors[i].idx;
		if (idx >= 0 && idx < ImGuiCol_COUNT)
			style.Colors[bgdm_colors[i].idx] = ImColor(bgdm_colors[i].col);
	}

	for (int i = 0; i < 2; i++) {
		for (int j = GW2::PROFESSION_GUARDIAN; j < GW2::PROFESSION_END; j++) {
			ATL::CAtlStringA str;
			str.Format("ProfColor_%d%d", i, j);
			prof_colors[i][j] = config_get_int(CONFIG_SECTION_STYLE, str.GetString(), prof_colors[i][j]);
		}
	}
}

void ImGui_StyleConfigSave()
{
	config_set_int(CONFIG_SECTION_OPTIONS, "ShowMetricsBGDM", g_state.show_metrics_bgdm);

	config_set_float(CONFIG_SECTION_STYLE, "GlobalAlpha", bgdm_style.global_alpha);
	config_set_int(CONFIG_SECTION_STYLE, "ColorEditMode", bgdm_style.col_edit_mode);
	config_set_int(CONFIG_SECTION_STYLE, "WindowRounding", bgdm_style.round_windows);

	config_set_int(CONFIG_SECTION_STYLE, "CompassFont", bgdm_style.compass_font);

	config_set_int(CONFIG_SECTION_STYLE, "TargetHPComma", bgdm_style.hp_comma_mode);
	config_set_int(CONFIG_SECTION_STYLE, "TargetHPPerFixed", bgdm_style.hp_fixed_mode);
	config_set_int(CONFIG_SECTION_STYLE, "TargetHPPerOnOff", bgdm_style.hp_percent_mode);
	config_set_int(CONFIG_SECTION_STYLE, "TargetHPPrecision", bgdm_style.hp_precision_mode);

	config_set_int(CONFIG_SECTION_STYLE, "PlayerHPColor", bgdm_style.hp_colors[HP_ROW_PLAYER_HP]);
	config_set_int(CONFIG_SECTION_STYLE, "TargetHPColor", bgdm_style.hp_colors[HP_ROW_TARGET_HP]);
	config_set_int(CONFIG_SECTION_STYLE, "TargetBBColor", bgdm_style.hp_colors[HP_ROW_TARGET_DIST]);
	config_set_int(CONFIG_SECTION_STYLE, "TargetDistColor", bgdm_style.hp_colors[HP_ROW_TARGET_BB]);
	config_set_int(CONFIG_SECTION_STYLE, "PortDistColor", bgdm_style.hp_colors[HP_ROW_PORT_DIST]);
	config_set_int(CONFIG_SECTION_STYLE, "PortDistColorBG", bgdm_style.hp_colors[HP_ROW_PORT_DIST_BG]);

	for (int i = 0; i < ARRAYSIZE(bgdm_colors); i++)
		config_set_int(CONFIG_SECTION_STYLE, bgdm_colors[i].cfg, bgdm_colors[i].col);

	for (int i = 0; i < 2; i++) {
		for (int j = GW2::PROFESSION_GUARDIAN; j < GW2::PROFESSION_END; j++) {
			ATL::CAtlStringA str;
			str.Format("ProfColor_%d%d", i, j);
			config_set_int(CONFIG_SECTION_STYLE, str.GetString(), prof_colors[i][j]);
		}
	}
}

static ImColor ColorFromCharacter(const Character *c, bool bUseColor)
{
	assert(c != NULL);
	if (!c) return ImColor(bgdm_colors[BGDM_COL_TEXT_PLAYER].col);

	if (!bUseColor) {
		bool isPlayerDead = !Ch_IsAlive(c->cptr) && !Ch_IsDowned(c->cptr);
		if (isPlayerDead)
			return ImColor(bgdm_colors[BGDM_COL_TEXT_PLAYER_DEAD].col);
		else if (g_player.c.cptr == c->cptr)
			return ImColor(bgdm_colors[BGDM_COL_TEXT_PLAYER_SELF].col);
		else
			return ImColor(bgdm_colors[BGDM_COL_TEXT_PLAYER].col);
	}

	ImColor col = ImColor(bgdm_colors[BGDM_COL_TEXT_PLAYER].col);
	int hasElite = Pl_HasEliteSpec(c->pptr) ? 1 : 0;
	if (c->profession > GW2::PROFESSION_NONE && c->profession < GW2::PROFESSION_END)
		col = ImColor(prof_colors[hasElite][c->profession]);
	return col;
}

typedef struct GraphData
{
	uint32_t id;
	bool isDead;
	bool isOOC;
	bool locked;
	float last_time;
	int last_second;
	uint32_t min_val;
	uint32_t max_val;
	ATL::CAtlArray<float> values;

	GraphData() : id(0), isDead(false), isOOC(false), locked(false), min_val(0), max_val(0), last_second(0), last_time(0.0f) {}
	GraphData(const GraphData& gd) {
		Copy(gd);
	}
	GraphData operator=(const GraphData& gd) {
		Copy(gd);
		return *this;
	}
	void Copy(const GraphData& gd) {
		id = gd.id;
		isDead = gd.isDead;
		isOOC = gd.isOOC;
		locked = gd.locked;
		min_val = gd.min_val;
		max_val = gd.max_val;
		last_time = gd.last_time;
		last_second = gd.last_second;
		values.Copy(gd.values);
	}
	void RemoveAll() {
		values.RemoveAll();
	}
	void Reset() {
		id = 0;
		isDead = false;
		isOOC = false;
		locked = false;
		last_time = 0.0f;
		last_second = 0;
		min_val = max_val = 0;
		RemoveAll();
	}
	void AddValue(uint32_t value)
	{
		values.Add((float)value);
		max_val = max(max_val, value);
		if (min_val == 0) min_val = value;
		else min_val = min(min_val, value);
	}
} GraphData;
typedef ATL::CAtlMap<uint32_t, GraphData> GraphDataMap;
static GraphData graph_data;
static GraphData graph_data_cleave;
static GraphDataMap graph_data_map;

typedef struct SkillData
{
	SkillData() : id(0), hash_id(0), dmg(0) {}

	uint32_t id;
	uint32_t hash_id;
	uint64_t dmg;
} SkillData;

//typedef ATL::CAtlList<SkillData> SkillDataList;
typedef ATL::CAtlArray<SkillData> SkillDataList;
typedef ATL::CAtlMap<uint32_t, SkillData> SkillDataMap;

static int __cdecl SkillDataCompare(void* pCtx, void const* pItem1, void const* pItem2)
{
	const SkillData* sd1 = (SkillData*)pItem1;
	const SkillData* sd2 = (SkillData*)pItem2;

	if (sd1->dmg < sd2->dmg) return 1;
	else if (sd1->dmg > sd2->dmg) return -1;
	else return 0;
}

typedef struct SkillBreakdown
{
	SkillBreakdown() { Reset(); }

	float duration;
	uint32_t tid;
	uint32_t hits;
	uint64_t dmg_tot;
	uint64_t dmg_high;
	SkillDataList sdlist;

	void Reset(bool removeAll = true) {
		tid = 0;
		hits = 0;
		duration = 0;
		dmg_tot = 0;
		dmg_high = 0;
		if (removeAll) sdlist.RemoveAll();
	}

	void RemoveAll() {
		sdlist.RemoveAll();
	}

	void AddValue(const SkillData &sd)
	{
		dmg_tot += sd.dmg;
		dmg_high = max(dmg_high, sd.dmg);

		//POSITION pos = sdlist.AddTail();
		//sdlist.GetAt(pos) = sd;
		sdlist.Add(sd);
	}

	void Sort()
	{
		qsort_s(sdlist.GetData(), sdlist.GetCount(), sizeof(sdlist[0]), SkillDataCompare, NULL);

		//for (POSITION pos = sdlist.GetHeadPosition(); pos != sdlist.GetTailPosition(); sdlist.GetNext(pos)) {
		//	for (int j = 0; j < sdlist.GetCount() - 1 - i; j++) {
		//		if (array[d] > array[d + 1])
		//		{
		//			sdlist.SwapElements()
		//			swap = array[d];
		//			array[d] = array[d + 1];
		//			array[d + 1] = swap;
		//		}
		//	}
		//}
	}

} SkillBreakdown;

static SkillBreakdown skills_target;
static SkillBreakdown skills_cleave;

void ImGui_RemoveGraphData(int32_t id)
{
	if (id <= 0) return;
	graph_data_map.RemoveKey(id);
}

void ImGui_ResetGraphData(int32_t exclude_id)
{
	skills_target.Reset();
	skills_cleave.Reset();

	if (graph_data_map.IsEmpty()) return;

	GraphDataMap::CPair *pair = NULL;
	if (exclude_id <= 0 || (pair = graph_data_map.Lookup(exclude_id)) == NULL) {
		graph_data_map.RemoveAll();
		return;
	}
	else {
		uint32_t key = pair->m_key;
		GraphData value = pair->m_value;
		graph_data_map.RemoveAll();
		graph_data_map[key] = value;
	}
}

static const GraphData* BuildGraphDataTarget(const struct DPSTargetEx* dps_target)
{
	static const float ONE_SECOND = 1000000.0f;

	if (!dps_target || dps_target->t.id == 0 || dps_target->t.num_hits == 0 ||
		(graph_data.id != 0 && dps_target->t.id != graph_data.id))
	{ graph_data.Reset(); return &graph_data; }
	if (graph_data.isDead) return &graph_data;

	GraphData &gd = graph_data_map[dps_target->t.id];
	gd.id = dps_target->t.id;
	gd.locked = dps_target->locked;
	gd.isDead = dps_target->t.isDead;

	int duration_secs = (int)(dps_target->t.duration / ONE_SECOND);
	if (dps_target->t.isDead) duration_secs++;
	if (duration_secs < gd.last_second) gd.last_second = duration_secs;	// F9 reset
	if (!dps_target->t.isDead && (duration_secs == 0 || duration_secs == gd.last_second)) { return &graph_data; }
	else gd.last_second = duration_secs;

	gd.RemoveAll();

	uint32_t damage = 0;
	uint32_t next_hit = 0;
	for (int i = 0; i < duration_secs; i++) {

		int64_t second_start = dps_target->t.hits[0].time + i*(int64_t)ONE_SECOND;
		int64_t second_end = min(second_start + (int64_t)ONE_SECOND, dps_target->t.hits[0].time + dps_target->t.duration);
		for (; next_hit < dps_target->t.num_hits &&
			dps_target->t.hits[next_hit].time >= second_start &&
			dps_target->t.hits[next_hit].time < second_end;
			++next_hit) {

			damage += dps_target->t.hits[next_hit].dmg;
		}

		float duration = (float)(second_end - dps_target->t.hits[0].time) / ONE_SECOND;
		uint32_t dps = (uint32_t)((float)damage / duration);
		gd.AddValue(dps);
		//DBGPRINT(L"dps[%d]: %d  <damage=%d> <secs: %.2f> <duration=%.2f> <isDead=%d>",
		//	i, dps, damage, duration, (float)dps_target->t.duration / ONE_SECOND, dps_target->t.isDead);
	}

	graph_data = gd;
	return &graph_data;
}

static const GraphData* BuildGraphDataCleave(const struct DPSPlayer* player)
{
	static const float ONE_SECOND = 1000.0f;

	GraphData &gd = graph_data_cleave;

	if (!player || player->time == 0.0f || player->time < gd.last_time) { gd.Reset(); return &gd; }

	bool isOOC = !InterlockedAnd(&g_player.combat.lastKnownCombatState, 1);
	if (isOOC && gd.isOOC) return &gd;
	gd.isOOC = isOOC;

	if (gd.last_time == player->time) return &gd;
	if (!isOOC && player->time - gd.last_time < ONE_SECOND) return &gd;

	float duration = player->time / ONE_SECOND;
	gd.AddValue((uint32_t)(player->damage / duration));
	gd.last_time = player->time;

	return &gd;
}

static void PlotGraph(int ui_mode, const struct DPSTargetEx* dps_target, const struct DPSPlayer* player)
{
	static char str_num[32] = { 0 };
	static const GraphData GdEmpty;
	const GraphData *pgd = &GdEmpty;

	int requested_mode = 0;
	if (player != NULL) pgd = BuildGraphDataCleave(player);
	else if (dps_target) {
		requested_mode = 1;
		pgd = BuildGraphDataTarget(dps_target);
	}

	ATL::CAtlStringA overlay;
	sprintf_num(pgd->min_val, str_num, sizeof(str_num));
	overlay.Format("%s:%s\n", LOCALTEXT(TEXT_GLOBAL_MIN), str_num);
	sprintf_num(pgd->max_val, str_num, sizeof(str_num));
	overlay.AppendFormat("%s:%s", LOCALTEXT(TEXT_GLOBAL_MAX), str_num);

	if (ui_mode != requested_mode) return;

	ImGui::Columns(1);
	ImGui::Separator();
	ImGui::PushItemWidth(-1);
	ImGui::PlotLines("##DPSGraph", pgd->values.GetData(), (int)pgd->values.GetCount(), 0, overlay.GetString(), 0.0f, FLT_MAX, ImVec2(0, 80));
	ImGui::PopItemWidth();
}

void ImGui_PersonalDps(struct Panel *panel, int x, int y, const struct DPSData* dps_data, const struct DPSTargetEx* dps_target)
{
	static char str_num[32] = { 0 };
	static ImGuiWindowFlags temp_flags = 0;
	static int temp_fr_count = 0;
	ImGuiWindowFlags flags = 0;
	if (minimal_panels) flags |= MIN_PANEL_FLAGS;
	if (IsDisableInputOrActionCam()) flags |= NO_INPUT_FLAG;
	IMGUI_FIX_NOINPUT_BUG;

	ImGuiStyle &style = ImGui::GetStyle();

	if (!panel->autoResize) temp_flags = 0;
	else if (temp_flags) {
		if (temp_fr_count++ > 1) {
			flags |= temp_flags;
			temp_flags = 0;
			temp_fr_count = 0;
		}
	}

	if (dps_target && dps_target->locked)
		flags |= ImGuiWindowFlags_ShowBorders;

	bool use_tinyfont = panel->tinyFont;
	if (global_use_tinyfont || use_tinyfont) ImGui::PushFont(tinyFont);
	else ImGui::PushFont(defaultFont);

	uint32_t old_enabled = panel->enabled;
	ImColor white(ImColor(bgdm_colors[BGDM_COL_TEXT1].col));
	ImColor old_border = style.Colors[ImGuiCol_Border];
	ImGui::PushStyleColor(ImGuiCol_Border, ImColor(bgdm_colors[BGDM_COL_TARGET_LOCK_BG].col));
	ImGui::SetNextWindowPos(ImVec2((float)x, (float)y), ImGuiSetCond_FirstUseEver);
	if (!ImGui::Begin(LOCALTEXT(TEXT_BGDM_MAIN_PANEL_TARGET_STATS), (bool*)&panel->enabled, ImVec2(245, 150), panel->fAlpha, flags))
	{
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopFont();
		return;
	}

	// was close button pressed 
	// uint32_t old_enabled = panel->enabled;
	if (old_enabled != panel->enabled)
		PanelSaveColumnConfig(panel);

	ImGui::PushStyleColor(ImGuiCol_Border, old_border);

	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1))
	{
		ImRect rect = ImGui::GetCurrentWindow()->TitleBarRect();
		if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max, false))
			ImGui::OpenPopup("CtxMenu");
	}
	if (ImGui::BeginPopup("CtxMenu"))
	{
		bool needUpdate = false;
		if (ImGui::BeginMenu(LOCALTEXT(TEXT_GLOBAL_OPTIONS)))
		{
			ShowWindowOptionsMenu(panel, &temp_flags);

			needUpdate |= ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_PANEL_TARGET_SHOW_DMG), "", &panel->cfg.col_visible[TDPS_COL_DMG]);
			needUpdate |= ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_PANEL_TARGET_SHOW_TTK), "", &panel->cfg.col_visible[TDPS_COL_TTK]);
			needUpdate |= ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_PANEL_TARGET_SHOW_GRAPH), "", &panel->cfg.col_visible[TDPS_COL_GRAPH]);

			ImGui::EndMenu();
		}

		ShowCombatMenu();

		if (g_state.panel_debug.enabled) {
			if (ImGui::BeginMenu("Debug")) {
				needUpdate |= ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_PANEL_TARGET_SHOW_NPC), "", &panel->cfg.col_visible[TDPS_COL_SPECID]);
				ImGui::EndMenu();
			}
		}

		if (needUpdate) {
			temp_flags = ImGuiWindowFlags_AlwaysAutoResize;
			PanelSaveColumnConfig(panel);
		}

		ImGui::EndPopup();
	}

	ImGui::PushStyleColor(ImGuiCol_Text, ImColor(ImColor(bgdm_colors[BGDM_COL_TEXT2].col)));

	if (dps_target->c.name[0] || dps_target->c.decodedName) {

		const ATL::CAtlStringW unicode = dps_target->c.decodedName ? dps_target->c.decodedName : dps_target->c.name;
		const ATL::CAtlStringA utf8 = CW2A(unicode, CP_UTF8);
		ImGui::TextColored(ImColor(bgdm_colors[BGDM_COL_TEXT_TARGET].col), u8"%s", utf8);
	}
	else ImGui::NewLine();

	ATL::CAtlStringA str;
	str.Format("%sXXX", LOCALTEXT(TEXT_PANEL_TARGET_TIME));
	ImGui::Columns(4, "TargetDPS", true);
	ImGui::SetColumnOffset(1, ImGui::CalcTextSize(str, NULL, true).x);
	ImGui::Separator();

	ImGui::TextColored(white, LOCALTEXT(TEXT_PANEL_TARGET_TIME)); ImGui::NextColumn();
	ImGui::Text("%dm %.2fs\n",
		(int)dps_data->tot_dur / 60,
		fmod(dps_data->tot_dur, 60));
	ImGui::NextColumn();

	ImGui::TextColored(white, LOCALTEXT(TEXT_PANEL_TARGET_L10S));  ImGui::NextColumn();
	ImGui::TextColored(white, LOCALTEXT(TEXT_PANEL_TARGET_L30S));  ImGui::NextColumn();

	ImGui::Separator();
	ImGui::TextColored(white, LOCALTEXT(TEXT_PANEL_TARGET_PDPS));  ImGui::NextColumn();
	u32 pdps = dps_data->tot_dur ? (u32)(dps_data->tot_done / dps_data->tot_dur) : 0;
	sprintf_num(pdps, str_num, sizeof(str_num));
	ImGui::Text(str_num);  ImGui::NextColumn();

	sprintf_num(dps_data->l1_dur ? (i32)(dps_data->l1_done / dps_data->l1_dur) : 0, str_num, sizeof(str_num));
	ImGui::Text(str_num);  ImGui::NextColumn();

	sprintf_num(dps_data->l2_dur ? (i32)(dps_data->l2_done / dps_data->l2_dur) : 0, str_num, sizeof(str_num));
	ImGui::Text(str_num);  ImGui::NextColumn();

	ImGui::TextColored(white, LOCALTEXT(TEXT_PANEL_TARGET_TDPS));  ImGui::NextColumn();
	u32 tdps = dps_data->tot_dur ? (u32)(dps_data->hp_lost / dps_data->tot_dur) : 0;
	sprintf_num(tdps, str_num, sizeof(str_num));
	ImGui::Text(str_num);  ImGui::NextColumn();

	sprintf_num(dps_data->l1_dur ? (i32)(dps_data->l1_done_team / dps_data->l1_dur) : 0, str_num, sizeof(str_num));
	ImGui::Text(str_num);  ImGui::NextColumn();

	sprintf_num(dps_data->l2_dur ? (i32)(dps_data->l2_done_team / dps_data->l2_dur) : 0, str_num, sizeof(str_num));
	ImGui::Text(str_num);  ImGui::NextColumn();

	if (panel->cfg.col_visible[TDPS_COL_DMG]) {

		ImGui::Separator();
		ImGui::TextColored(white, LOCALTEXT(TEXT_PANEL_TARGET_PDMG));  ImGui::NextColumn();
		sprintf_num(dps_data->tot_done, str_num, sizeof(str_num));
		ImGui::Text(str_num);  ImGui::NextColumn();
		sprintf_num(dps_data->l1_done, str_num, sizeof(str_num));
		ImGui::Text(str_num);  ImGui::NextColumn();
		sprintf_num(dps_data->l2_done, str_num, sizeof(str_num));
		ImGui::Text(str_num);  ImGui::NextColumn();

		ImGui::TextColored(white, LOCALTEXT(TEXT_PANEL_TARGET_TDMG));  ImGui::NextColumn();
		sprintf_num(dps_data->hp_lost, str_num, sizeof(str_num));
		ImGui::Text(str_num);  ImGui::NextColumn();
		sprintf_num(dps_data->l1_done_team, str_num, sizeof(str_num));
		ImGui::Text(str_num);  ImGui::NextColumn();
		sprintf_num(dps_data->l2_done_team, str_num, sizeof(str_num));
		ImGui::Text(str_num);  ImGui::NextColumn();
	}

	if (panel->cfg.col_visible[TDPS_COL_TTK]) {
		u32 ttk = (dps_target && tdps > 0) ? dps_target->c.hp_val / tdps : 0;
		ImGui::Separator();
		ImGui::TextColored(white, LOCALTEXT(TEXT_PANEL_TARGET_TTK)); ImGui::NextColumn();
		ImGui::TextColored(ImColor(bgdm_colors[BGDM_COL_TEXT_TARGET].col), "%dm %ds", ttk / 60, ttk % 60);
		ImGui::NextColumn();
	}

	if (panel->cfg.col_visible[TDPS_COL_GRAPH]) {

		PlotGraph(1, dps_target, NULL);
	}

	if (g_state.panel_debug.enabled && panel->cfg.col_visible[TDPS_COL_SPECID]) {
		ImGui::Columns(2, "", false);
		ImGui::SetColumnOffset(1, 70.0f);
		int hashID = dps_target ? dps_target->t.npc_id : 0;
		int speciesID = dps_target ? dps_target->t.species_id : 0;
		ImGui::Separator();
		ImGui::TextDisabled("NPC ID"); ImGui::NextColumn();
		ImGui::TextDisabled("%d (0x%X)", hashID, hashID);  ImGui::NextColumn();
		ImGui::TextDisabled("NPC HASH"); ImGui::NextColumn();
		ImGui::TextDisabled("%d (0x%X)", speciesID, speciesID); ImGui::NextColumn();
		if (speciesID) {
			const ATL::CAtlStringW unicode = lru_find(GW2::CLASS_TYPE_SPECIESID, speciesID, NULL, 0);
			const ATL::CAtlStringA utf8 = CW2A(unicode, CP_UTF8);
			ImGui::TextDisabled("NPC Name"); ImGui::NextColumn();
			ImGui::TextDisabled("%s", utf8);
		}
	}

	ImGui::PopStyleColor(2); // Text/Separator
	ImGui::End();
	ImGui::PopStyleColor(); // Border
	ImGui::PopFont();
	
}

static void SetColumnOffsets(const struct Panel *panel, int type)
{
	ATL::CAtlStringA str("XX"); // Spacing
	int col = 0;
	float offset = 0;
	float width = 0;
	float spacing = ImGui::CalcTextSize(str, NULL, true).x * 0.6f;
	int size = ImGui::GetColumnsDataSize();
	int nCol = panel->cfg.maxCol;
	if (type == 0) nCol = GDPS_COL_END;
	for (int i = 0; i < panel->cfg.maxCol; ++i) {
		if (!panel->cfg.col_visible[i])
			continue;

		if (i<2) {
			switch (i) {
			case(GDPS_COL_NAME):
				str.Format("XXXXXXXXXXXXXXXXXXXXX");
				if (panel->cfg.lineNumbering) str.AppendFormat("99.");
				width = ImGui::CalcTextSize(str, NULL, true).x;
				break;
			case(GDPS_COL_CLS):
				str.Format("XXXX");
				width = ImGui::CalcTextSize(str, NULL, true).x;
				break;
			}
		}
		else if ((type == 0 && i == GDPS_COL_TIME) ||
				(type == 1 && i == BUFF_COL_TIME)) {
			// Time
			str.Format("99m 99.99s");
			width = ImGui::CalcTextSize(str, NULL, true).x;
		}
		else if (type == 0) {
			switch (i) {
			case(GDPS_COL_PER):
				str.Format("100%%");
				width = ImGui::CalcTextSize(str, NULL, true).x;
				break;
			default:
				str.Format("XXXXXXXXX");
				width = ImGui::CalcTextSize(str, NULL, true).x;
				break;
			}
		}
		else if (type == 1) {
			switch (i) {
			case(BUFF_COL_MIGHT):
				str.Format("25.0");
				width = ImGui::CalcTextSize(str, NULL, true).x;
				break;
			default:
				str.Format("100%%");
				width = ImGui::CalcTextSize(str, NULL, true).x;
				break;
			}
		}

		offset += width + spacing;
		if (++col < size) ImGui::SetColumnOffset(col, offset);
	}
}

static __inline const void SkillAdd(SkillDataMap &sdmap, const DPSHit &hit)
{
	if (hit.eff_id == 0) return;
	SkillData &sd = sdmap[hit.eff_id];
	sd.dmg += hit.dmg;
	sd.id = hit.eff_id;
	sd.hash_id = hit.eff_hash;
}

static const void BuildSkillDataLists(const struct DPSTargetEx* dps_target)
{
	static bool last_ooc = false;
	static bool last_dead = false;
	bool isOOC = !InterlockedAnd(&g_player.combat.lastKnownCombatState, 1);
	if (last_ooc && isOOC) return;
	last_ooc = isOOC;

	bool build_target = true;
	bool build_cleave = true;

	if (!dps_target || dps_target->t.id == 0 || dps_target->t.num_hits == 0 ||
		(last_dead && dps_target && dps_target->t.isDead)) {
		build_target = false;
	}
	last_dead = dps_target ? dps_target->t.isDead : 0;

	if ((dps_target && dps_target->t.num_hits == 0 && skills_target.hits > 0) ||
		(dps_target && dps_target->t.id != skills_target.tid))
		skills_target.Reset();

	if (build_target) {
		
		skills_target.Reset(false);
		skills_target.duration = (float)((uint32_t)dps_target->t.duration / 1000000.0f);

		if (skills_target.tid != dps_target->t.id || skills_target.hits != dps_target->t.num_hits) {

			skills_target.tid = dps_target->t.id;
			skills_target.hits = dps_target->t.num_hits;

			SkillDataMap sdmap;
			for (uint32_t i = 0; i < dps_target->t.num_hits; i++) {
				SkillAdd(sdmap, dps_target->t.hits[i]);
			}

			skills_target.RemoveAll();
			for (POSITION pos = sdmap.GetStartPosition(); pos != NULL; sdmap.GetNext(pos)) {
				skills_target.AddValue(sdmap.GetValueAt(pos));
			}
			skills_target.Sort();
		}
	}

	if (build_cleave) {

		skills_cleave.Reset(false);
		skills_cleave.duration = g_player.combat.duration / 1000.0f;

		uint32_t num_hits = 0;
		SkillDataMap sdmap;
		for (int i = 0; i < MAX_TARGETS; i++) {
			DPSTarget* t = dps_targets_get(i);
			if (!t || t->id == 0 || t->num_hits == 0) continue;
			for (uint32_t j = 0; j < t->num_hits; j++) {
				if (t->hits[j].time >= g_player.combat.begin) {
					SkillAdd(sdmap, t->hits[j]);
					num_hits++;
				}
			}
		}

		if (num_hits == 0 || skills_cleave.hits != num_hits) {
			skills_cleave.RemoveAll();
			skills_cleave.hits = num_hits;
			for (POSITION pos = sdmap.GetStartPosition(); pos != NULL; sdmap.GetNext(pos)) {
				skills_cleave.AddValue(sdmap.GetValueAt(pos));
			}
			skills_cleave.Sort();
		}
	}
}

void ImGui_SkillBreakdown(struct Panel *panel, int x, int y, const struct DPSTargetEx* dps_target)
{
	static char str_num1[32] = { 0 };
	static char str_num2[32] = { 0 };
	static wchar_t skillName[60] = { 0 };
	static ImGuiWindowFlags temp_flags = 0;
	static int temp_fr_count = 0;
	ImGuiWindowFlags flags = 0;
	if (minimal_panels) flags |= MIN_PANEL_FLAGS;
	if (IsDisableInputOrActionCam()) flags |= NO_INPUT_FLAG;
	IMGUI_FIX_NOINPUT_BUG;

	ImGuiStyle &style = ImGui::GetStyle();

	if (!panel->autoResize) temp_flags = 0;
	else if (temp_flags) {
		if (temp_fr_count++ > 1) {
			flags |= temp_flags;
			temp_flags = 0;
			temp_fr_count = 0;
		}
	}

	int mode = panel->mode;
	panel->enabled = mode + 1;
	if (mode == 1 && dps_target && dps_target->locked)
		flags |= ImGuiWindowFlags_ShowBorders;

	bool use_tinyfont = panel->tinyFont;
	if (global_use_tinyfont || use_tinyfont) ImGui::PushFont(tinyFont);
	else ImGui::PushFont(defaultFont);

	uint32_t old_enabled = panel->enabled;
	ImColor white(ImColor(bgdm_colors[BGDM_COL_TEXT1].col));
	ImColor old_border = style.Colors[ImGuiCol_Border];
	ImGui::PushStyleColor(ImGuiCol_Border, ImColor(bgdm_colors[BGDM_COL_TARGET_LOCK_BG].col));
	ImGui::SetNextWindowPos(ImVec2((float)x, (float)y), ImGuiSetCond_FirstUseEver);
	if (!ImGui::Begin(LOCALTEXT(TEXT_BGDM_MAIN_PANEL_SKILLS), (bool*)&panel->enabled, ImVec2(425, 225), panel->fAlpha, flags))
	{
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopFont();
		return;
	}

	// was close button pressed 
	// uint32_t old_enabled = panel->enabled;
	if (old_enabled != panel->enabled)
		PanelSaveColumnConfig(panel);

	ImGui::PushStyleColor(ImGuiCol_Border, old_border);

	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1))
	{
		ImRect rect = ImGui::GetCurrentWindow()->TitleBarRect();
		if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max, false))
			ImGui::OpenPopup("CtxMenu");
	}
	if (ImGui::BeginPopup("CtxMenu"))
	{
		bool needUpdate = false;
		if (ImGui::BeginMenu(LOCALTEXT(TEXT_GLOBAL_OPTIONS)))
		{
			ShowWindowOptionsMenu(panel, &temp_flags);

			if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_PANEL_OPTS_LINENO), "", &panel->cfg.lineNumbering)) {
				config_set_int(panel->section, "LineNumbering", panel->cfg.lineNumbering);
			}

			ImGui::EndMenu();
		}

		if (needUpdate) {
			temp_flags = ImGuiWindowFlags_AlwaysAutoResize;
			PanelSaveColumnConfig(panel);
		}

		ImGui::EndPopup();
	}

	ImGui::PushStyleColor(ImGuiCol_Text, ImColor(ImColor(bgdm_colors[BGDM_COL_TEXT2].col)));

	{
		bool needUpdate = false;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		needUpdate |= ImGui::RadioButton(LOCALTEXT(TEXT_PANEL_GDPS_CLEAVE), &mode, 0); ImGui::SameLine();
		needUpdate |= ImGui::RadioButton(LOCALTEXT(TEXT_PANEL_GDPS_TARGET), &mode, 1);
		ImGui::PopStyleVar();
		if (needUpdate) {
			panel->mode = mode;
			panel->enabled = mode + 1;
			config_set_int(panel->section, "Mode", panel->mode);
		}
	}

	if (mode == 1 && dps_target) {

		const ATL::CAtlStringW unicode = dps_target->c.decodedName ? dps_target->c.decodedName : dps_target->c.name;
		const ATL::CAtlStringA utf8 = CW2A(unicode, CP_UTF8);
		ImGui::TextColored(ImColor(bgdm_colors[BGDM_COL_TEXT_TARGET].col), u8"%s", utf8);
	}

	ImGui::Separator();

	// Build the skill data
	BuildSkillDataLists(dps_target);

	SkillBreakdown &skills = mode ? skills_target : skills_cleave;
	//int i = 0;
	//for (POSITION pos = skills.sdlist.GetTailPosition(); pos != NULL; skills.sdlist.GetPrev(pos), i++) {
	for (int i = 0; i < skills.sdlist.GetCount(); i++) {

		const SkillData &sd = skills.sdlist[i];
		if (sd.id == 0) continue;

		float rect_per = skills.dmg_high ? (float)sd.dmg / skills.dmg_high : 0.0f;
		if (rect_per > 0.0f) {
			ImVec2 p = ImGui::GetCursorScreenPos();
			//ImGuiStyle &style = ImGui::GetStyle();
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			float width = ImGui::GetContentRegionAvailWidth() * rect_per;
			float height = ImGui::GetTextLineHeight() + style.WindowPadding.y/2;
			p.y -= style.WindowPadding.y / 4;
			draw_list->AddRectFilledMultiColor(ImVec2(p.x, p.y), ImVec2(p.x + width, p.y + height),
				ImColor(bgdm_colors[BGDM_COL_GRAD_START].col),
				ImColor(bgdm_colors[BGDM_COL_GRAD_END].col),
				ImColor(bgdm_colors[BGDM_COL_GRAD_END].col),
				ImColor(bgdm_colors[BGDM_COL_GRAD_START].col));
		}

		ATL::CAtlStringA str;
		if (sd.hash_id) {
			memset(skillName, 0, sizeof(skillName));
			if (lru_find(GW2::CLASS_TYPE_SKILLID, sd.hash_id, skillName, ARRAYSIZE(skillName))) {
				const ATL::CAtlStringW unicode = skillName;
				str = CW2A(unicode, CP_UTF8);
			}
		}
		if (str.IsEmpty()) str.Format("ID:%d", sd.id);
		if (panel->cfg.lineNumbering) {
			ATL::CAtlStringA saved = str;
			str.Format((i<9) ? "%d. %s" : "%d.%s", i + 1, saved);
		}
		ImGui::TextColored(white, str);

		memset(str_num1, 0, sizeof(str_num1));
		memset(str_num2, 0, sizeof(str_num2));
		sprintf_num((double)sd.dmg, str_num1, sizeof(str_num1));
		sprintf_num(skills.duration ? (double)sd.dmg / skills.duration : 0, str_num2, sizeof(str_num2));
		float per = skills.dmg_tot ? (float)sd.dmg / skills.dmg_tot : 0.0f;
		if (per > 100.0f) per = 100.0f;

		str.Format("%s (%s, %.2f%%%%)", str_num2, str_num1, per*100.0f);
		float maxw = ImGui::GetContentRegionAvailWidth();
		float textw = ImGui::CalcTextSize(str).x;
		ImGui::SameLine(max(150.0f, maxw-textw));
		ImGui::TextColored(white, str);
	}

	ImGui::PopStyleColor(2); // Text/Separator
	ImGui::End();
	ImGui::PopStyleColor(); // Border
	ImGui::PopFont();

}


void ImGui_GroupDps(struct Panel *panel, int x, int y,
	const struct DPSTargetEx* target,
	const struct DPSPlayer* players, u32 num)
{
	static char str_num[32] = { 0 };
	static ImGuiWindowFlags temp_flags = 0;
	static int temp_fr_count = 0;
	ImGuiWindowFlags flags = 0;
	if (minimal_panels) flags |= MIN_PANEL_FLAGS;
	if (IsDisableInputOrActionCam()) flags |= NO_INPUT_FLAG;
	IMGUI_FIX_NOINPUT_BUG;
	if (!panel->enabled) return;

	ImGuiStyle &style = ImGui::GetStyle();
	
	if (!panel->autoResize) temp_flags = 0;
	else if (temp_flags) {
		if (temp_fr_count++ > 1) {
			flags |= temp_flags;
			temp_flags = 0;
			temp_fr_count = 0;
		}
	}

	int mode = panel->mode;
	panel->enabled = mode + 1;
	if (mode == 1 && target && target->locked)
		flags |= ImGuiWindowFlags_ShowBorders;

	bool use_tinyfont = panel->tinyFont;
	if (global_use_tinyfont || use_tinyfont) ImGui::PushFont(tinyFont);
	else ImGui::PushFont(defaultFont);

	uint32_t old_enabled = panel->enabled;
	ImVec2 ICON_SIZE(ImGui::GetFont()->FontSize + 2, ImGui::GetFont()->FontSize);
	ImColor white(ImColor(bgdm_colors[BGDM_COL_TEXT1].col));
	ImColor old_border = style.Colors[ImGuiCol_Border];
	ImGui::PushStyleColor(ImGuiCol_Border, ImColor(bgdm_colors[BGDM_COL_TARGET_LOCK_BG].col));
	ImGui::SetNextWindowPos(ImVec2((float)x, (float)y), ImGuiSetCond_FirstUseEver);
	if (!ImGui::Begin(LOCALTEXT(TEXT_BGDM_MAIN_PANEL_GROUP_STATS), (bool*)&panel->enabled, ImVec2(400, 175), panel->fAlpha, flags))
	{
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopFont();
		return;
	}

	// was close button pressed 
	// uint32_t old_enabled = panel->enabled;
	if (old_enabled != panel->enabled)
		PanelSaveColumnConfig(panel);

	ImGui::PushStyleColor(ImGuiCol_Border, old_border);

	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1))
	{
		ImRect rect = ImGui::GetCurrentWindow()->TitleBarRect();
		if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max, false))
			ImGui::OpenPopup("CtxMenu");
	}
	if (ImGui::BeginPopup("CtxMenu"))
	{
		bool saveConfig = false;
		if (ImGui::BeginMenu(LOCALTEXT(TEXT_GLOBAL_OPTIONS)))
		{
			ShowWindowOptionsMenu(panel, &temp_flags);

			if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_PANEL_GDPS_GRAPH), "", &panel->cfg.col_visible[GDPS_COL_GRAPH])) {
				saveConfig = true;
			}
			if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_PANEL_OPTS_LINENO), "", &panel->cfg.lineNumbering)) {
				config_set_int(panel->section, "LineNumbering", panel->cfg.lineNumbering);
			}
			if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_PANEL_OPTS_PROFCOL), "", &panel->cfg.useProfColoring)) {
				config_set_int(panel->section, "ProfessionColoring", panel->cfg.useProfColoring);
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(LOCALTEXT(TEXT_GLOBAL_COLUMNS)))
		{
			if (ImGui::SelectableNoPopupClose(LOCALTEXT(TEXT_GLOBAL_ALL))) {
				for (int i = 1; i < GDPS_COL_END; i++)
					panel->cfg.col_visible[i] = true;
				saveConfig = true;
			}
			if (ImGui::SelectableNoPopupClose(LOCALTEXT(TEXT_GLOBAL_NONE))) {
				for (int i = 1; i < GDPS_COL_END; i++)
					panel->cfg.col_visible[i] = false;
				saveConfig = true;
			}
			if (ImGui::SelectableNoPopupClose(LOCALTEXT(TEXT_GLOBAL_DEFAULT))) {
				for (int i = 1; i < GDPS_COL_END; i++)
					panel->cfg.col_visible[i] = false;
				for (int i = 1; i < GDPS_COL_DMGIN; i++)
					panel->cfg.col_visible[i] = true;
				panel->cfg.col_visible[GDPS_COL_TIME] = true;
				saveConfig = true;
			}

			ImGui::Separator();
			for (int j = 1; j < GDPS_COL_END; ++j)
				if (ImGui::MenuItemNoPopupClose(GdpsTextFromColumn(j), "", &panel->cfg.col_visible[j]))
					saveConfig = true;
			ImGui::EndMenu();
		}
		ShowCombatMenu();
		ImGui::EndPopup();

		if (saveConfig) {
			PanelSaveColumnConfig(panel);
			temp_flags = ImGuiWindowFlags_AlwaysAutoResize;
		}
	}

	ImGui::PushStyleColor(ImGuiCol_Text, ImColor(bgdm_colors[BGDM_COL_TEXT2].col));

	{
		bool needUpdate = false;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		needUpdate |= ImGui::RadioButton(LOCALTEXT(TEXT_PANEL_GDPS_CLEAVE), &mode, 0); ImGui::SameLine();
		needUpdate |= ImGui::RadioButton(LOCALTEXT(TEXT_PANEL_GDPS_TARGET), &mode, 1);
		ImGui::PopStyleVar();
		if (needUpdate) {
			panel->mode = mode;
			panel->enabled = mode + 1;
			config_set_int(panel->section, "Mode", panel->mode);
		}
	}

	if (mode == 1 && target) {

		ImGui::TextColored(white, "<%s>",
			target->t.invuln ? LOCALTEXT(TEXT_PANEL_GDPS_INVULN) :
			(target->locked ? LOCALTEXT(TEXT_PANEL_GDPS_LOCKED) : LOCALTEXT(TEXT_PANEL_GDPS_UNLOCKED)));

		ImGui::SameLine(85);
		const ATL::CAtlStringW unicode = target->c.decodedName ? target->c.decodedName : target->c.name;
		const ATL::CAtlStringA utf8 = CW2A(unicode, CP_UTF8);
		ImGui::TextColored(ImColor(bgdm_colors[BGDM_COL_TEXT_TARGET].col), u8"%s", utf8);
		ImGui::Separator();
	}

	int nCol = 1;
	for (int i = 1; i < GDPS_COL_END; ++i) {
		if (panel->cfg.col_visible[i])
			nCol++;
	}

	ImGui::Columns(nCol, NULL, true);
	SetColumnOffsets(panel, 0);
	bool sortFound = false;
	for (int i = 0; i < GDPS_COL_END; ++i) {
		if (!panel->cfg.col_visible[i])
			continue;
		if (i == panel->cfg.sortCol) {
			sortFound = true;
			if (UP_ARROW && DOWN_ARROW) {
				ImGui::Image(panel->cfg.asc ? UP_ARROW : DOWN_ARROW, ICON_SIZE);
			}
			else {
				ImGui::PushFont(tinyFont);
				ImGui::TextColored(white, panel->cfg.asc ? ">" : "<");
				ImGui::PopFont();
			}
		}
		ImGui::NextColumn();
	}
	if (!sortFound) ImGui::NewLine();

	ImGui::Columns(nCol, "GroupDPS", true);
	SetColumnOffsets(panel, 0);
	for (int i = 0; i < GDPS_COL_END; ++i) {
		if (!panel->cfg.col_visible[i])
			continue;

		ImGui::PushStyleColor(ImGuiCol_Text, white);
		if (ImGui::Selectable(GdpsAcronymFromColumn(i))) {}
		if (i > 0 && ImGui::BeginPopupContextItem(gdps_col_str[i])) {
			ImGui::Text(GdpsTextFromColumn(i));
			ImGui::Separator();
			if (ImGui::Selectable(LOCALTEXT(TEXT_GLOBAL_HIDE))) {
				panel->cfg.col_visible[i] = false;
				PanelSaveColumnConfig(panel);
				temp_flags = ImGuiWindowFlags_AlwaysAutoResize;
				ImGui::EndPopup();
				ImGui::PopStyleColor();
				goto abort;
			}
			ImGui::EndPopup();
		}
		if (ImGui::IsItemClicked()) {
			panel->cfg.sortCol = i;
			if (panel->cfg.sortColLast != GDPS_COL_END &&
				panel->cfg.sortColLast == panel->cfg.sortCol)
				panel->cfg.asc ^= 1;
			else
				panel->cfg.asc = 0;
			panel->cfg.sortColLast = i;
			PanelSaveColumnConfig(panel);
		}
		ImGui::PopStyleColor();
		ImGui::NextColumn();
	}

	// Calculate total damage for the % calc
	u32 tot_dmg = 0;
	if (mode == 1 && target) {
		//if (target->t.isDead) { assert(target->t.hp_max == target->c.hp_max); }
		tot_dmg = target->t.hp_max;
	}
	else {
		for (u32 i = 0; i < num; ++i)
			tot_dmg += players[i].damage;
	}

	ImGui::Separator();
	for (u32 i = 0; i < num; ++i)
	{
		uint32_t dmg = (mode == 1) ? players[i].target_dmg : players[i].damage;
		float time = (mode == 1) ? players[i].target_time : players[i].time;
		float dur = time ? time / 1000.0f : 0;
		float dur_cleave = players[i].time ? players[i].time / 1000.0f : 0;

		for (int j = 0; j < GDPS_COL_END; ++j) {
			if (!panel->cfg.col_visible[j])
				continue;

			if (j > GDPS_COL_CLS && dur == 0 && dur_cleave == 0) {
				ImGui::NextColumn();
				continue;
			}

			switch (j) {
			case(GDPS_COL_NAME):
			{
				const ATL::CAtlStringW unicode = players[i].name;
				const ATL::CAtlStringA utf8 = CW2A(unicode, CP_UTF8);
				ATL::CAtlStringA no("");
				if (panel->cfg.lineNumbering) no.Format((i<9) ? "%d. " : "%d.", i + 1);
				ImColor plColor = ColorFromCharacter(players[i].c, panel->cfg.useProfColoring);
				// Skada UI TODO: this is not really looking good on the current panel design
				// probably need to enable/disable "skada mode" and copy the looks of the skill
				// breakdown panel which is looking good in "skada mode"
				//float per = dmg / (f32)tot_dmg;
				//if (per > 0.0f) {
				//	ImVec2 p = ImGui::GetCursorScreenPos();
				//	ImGuiStyle &style = ImGui::GetStyle();
				//	ImDrawList* draw_list = ImGui::GetWindowDrawList();
				//	float width = ImGui::GetContentRegionAvailWidth() * per;
				//	float height = ImGui::GetTextLineHeight() + style.WindowPadding.y/2;
				//	p.y -= style.WindowPadding.y / 4;
				//	draw_list->AddRectFilledMultiColor(ImVec2(p.x, p.y), ImVec2(p.x + width, p.y + height), plColor, ImColor(0, 0, 0), ImColor(0, 0, 0), plColor);
				//}
				ImGui::TextColored(plColor, u8"%s%s", no, utf8);
			} break;
			case(GDPS_COL_CLS):
			{
				LPVOID img = ProfessionImageFromPlayer(players[i].c);
				if (img) ImGui::Image(img, ICON_SIZE);
			} break;
			case(GDPS_COL_DPS):
			{
				i32 pdps = dur ? (i32)(dmg / dur) : 0;
				ImGui::Text("%s", sprintf_num(pdps, str_num, sizeof(str_num)));
			} break;
			case(GDPS_COL_PER):
			{
				u32 per = (u32)roundf(dmg / (f32)tot_dmg * 100.0f);
				per = per > 100 ? 100 : per;
				if (per>99) ImGui::SameLine(3);
				ImGui::Text("%u%%", per);
			} break;
			case (GDPS_COL_DMGOUT):
			{
				ImGui::Text("%s", sprintf_num(dmg, str_num, sizeof(str_num)));
			} break;
			case (GDPS_COL_DMGIN):
			{
				ImGui::Text("%s", sprintf_num(players[i].damage_in, str_num, sizeof(str_num)));
			} break;
			case (GDPS_COL_HPS):
			{
				i32 phps = dur_cleave ? (i32)(players[i].heal_out / dur_cleave) : 0;
				ImGui::Text("%s", sprintf_num(phps, str_num, sizeof(str_num)));
			} break;
			case (GDPS_COL_HEAL):
			{
				ImGui::Text("%s", sprintf_num(players[i].heal_out, str_num, sizeof(str_num)));
			} break;
			case (GDPS_COL_TIME):
			{
				ImGui::Text("%dm %.2fs", (int)dur / 60, fmod(dur, 60));
			} break;

			}
			ImGui::NextColumn();
		}
	}

	if (panel->cfg.col_visible[GDPS_COL_GRAPH]) {

		// Always build cleave graph
		for (u32 i = 0; i < num; ++i) {
			const struct DPSPlayer& player = players[i];
			if (player.c && player.c->cptr == g_player.c.cptr) {
				PlotGraph(mode, NULL, &player);
			}
		}
		if (mode == 1) PlotGraph(mode, target, NULL);
	}

abort:
	ImGui::PopStyleColor(2); // Text/Separator
	ImGui::End();
	ImGui::PopStyleColor(); // border
	ImGui::PopFont();
}

static bool __inline BuildBuffColumnsMenu(struct Panel *panel)
{
	bool saveConfig = false;
	if (ImGui::BeginMenu(LOCALTEXT(TEXT_GLOBAL_COLUMNS)))
	{
		if (ImGui::SelectableNoPopupClose(LOCALTEXT(TEXT_GLOBAL_ALL))) {
			for (int i = 1; i < BUFF_COL_END; i++)
				panel->cfg.col_visible[i] = true;
			saveConfig = true;
		}
		if (ImGui::SelectableNoPopupClose(LOCALTEXT(TEXT_GLOBAL_NONE))) {
			for (int i = 1; i < BUFF_COL_END; i++)
				panel->cfg.col_visible[i] = false;
			saveConfig = true;
		}
		if (ImGui::SelectableNoPopupClose(LOCALTEXT(TEXT_GLOBAL_DEFAULT))) {
			for (int i = 1; i < BUFF_COL_END; i++)
				panel->cfg.col_visible[i] = false;
			for (int i = 1; i < BUFF_COL_VIGOR; i++)
				panel->cfg.col_visible[i] = true;
			panel->cfg.col_visible[BUFF_COL_GOTL] = true;
			panel->cfg.col_visible[BUFF_COL_GLEM] = true;
			panel->cfg.col_visible[BUFF_COL_TIME] = true;
			saveConfig = true;
		}
		if (ImGui::SelectableNoPopupClose(LOCALTEXT(TEXT_GLOBAL_WVW))) {
			for (int i = 1; i < BUFF_COL_END; i++)
				panel->cfg.col_visible[i] = false;
			panel->cfg.col_visible[BUFF_COL_CLS] = true;
			panel->cfg.col_visible[BUFF_COL_SUB] = true;
			for (int i = BUFF_COL_PROT; i < BUFF_COL_AEGIS; i++)
				panel->cfg.col_visible[i] = true;
			panel->cfg.col_visible[BUFF_COL_ALAC] = false;
			panel->cfg.col_visible[BUFF_COL_RETAL] = false;
			panel->cfg.col_visible[BUFF_COL_REVNR] = true;
			panel->cfg.col_visible[BUFF_COL_REVRD] = true;
			panel->cfg.col_visible[BUFF_COL_ELESM] = true;
			panel->cfg.col_visible[BUFF_COL_TIME] = true;
			saveConfig = true;
		}
		ImGui::Separator();
		saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_CLS), "", &panel->cfg.col_visible[BUFF_COL_CLS]);
		saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_SUB), "", &panel->cfg.col_visible[BUFF_COL_SUB]);
		saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_DOWN), "", &panel->cfg.col_visible[BUFF_COL_DOWN]);
		saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_SCLR), "", &panel->cfg.col_visible[BUFF_COL_SCLR]);
		saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_SEAW), "", &panel->cfg.col_visible[BUFF_COL_SEAW]);
		saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_TIME), "", &panel->cfg.col_visible[BUFF_COL_TIME]);

		if (ImGui::BeginMenu(LOCALTEXT(TEXT_BUFF_STD)))
		{
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_PROT), "", &panel->cfg.col_visible[BUFF_COL_PROT]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_QUIK), "", &panel->cfg.col_visible[BUFF_COL_QUIK]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_ALAC), "", &panel->cfg.col_visible[BUFF_COL_ALAC]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_FURY), "", &panel->cfg.col_visible[BUFF_COL_FURY]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_MIGHT), "", &panel->cfg.col_visible[BUFF_COL_MIGHT]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_VIGOR), "", &panel->cfg.col_visible[BUFF_COL_VIGOR]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_SWIFT), "", &panel->cfg.col_visible[BUFF_COL_SWIFT]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_STAB), "", &panel->cfg.col_visible[BUFF_COL_STAB]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_RETAL), "", &panel->cfg.col_visible[BUFF_COL_RETAL]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_RESIST), "", &panel->cfg.col_visible[BUFF_COL_RESIST]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_REGEN), "", &panel->cfg.col_visible[BUFF_COL_REGEN]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_AEGIS), "", &panel->cfg.col_visible[BUFF_COL_AEGIS]);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(LOCALTEXT(TEXT_BUFF_PROF)))
		{
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_GOTL), "", &panel->cfg.col_visible[BUFF_COL_GOTL]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_GLEM), "", &panel->cfg.col_visible[BUFF_COL_GLEM]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_BANS), "", &panel->cfg.col_visible[BUFF_COL_BANS]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_BAND), "", &panel->cfg.col_visible[BUFF_COL_BAND]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_BANT), "", &panel->cfg.col_visible[BUFF_COL_BANT]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_EA), "", &panel->cfg.col_visible[BUFF_COL_EA]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_SPOTTER), "", &panel->cfg.col_visible[BUFF_COL_SPOTTER]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_FROST), "", &panel->cfg.col_visible[BUFF_COL_FROST]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_SUN), "", &panel->cfg.col_visible[BUFF_COL_SUN]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_STORM), "", &panel->cfg.col_visible[BUFF_COL_STORM]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_STONE), "", &panel->cfg.col_visible[BUFF_COL_STONE]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_ENGPP), "", &panel->cfg.col_visible[BUFF_COL_ENGPP]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_REVNR), "", &panel->cfg.col_visible[BUFF_COL_REVNR]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_REVAP), "", &panel->cfg.col_visible[BUFF_COL_REVAP]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_REVRD), "", &panel->cfg.col_visible[BUFF_COL_REVRD]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_ELESM), "", &panel->cfg.col_visible[BUFF_COL_ELESM]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_GRDSN), "", &panel->cfg.col_visible[BUFF_COL_GRDSN]);
			saveConfig |= ImGui::MenuItemNoPopupClose(BuffTextFromColumn(BUFF_COL_NECVA), "", &panel->cfg.col_visible[BUFF_COL_NECVA]);
			ImGui::EndMenu();
		}
		if (saveConfig) PanelSaveColumnConfig(panel);
		ImGui::EndMenu();
	}
	return saveConfig;
}

void ImGui_BuffUptime(struct Panel *panel, int x, int y,
	struct ClosePlayer *players, uint32_t num)
{
	static ImGuiWindowFlags temp_flags = 0;
	static int temp_fr_count = 0;
	ImGuiWindowFlags flags = 0;
	if (minimal_panels) flags |= MIN_PANEL_FLAGS;
	if (IsDisableInputOrActionCam()) flags |= NO_INPUT_FLAG;
	IMGUI_FIX_NOINPUT_BUG;

	ImGuiStyle &style = ImGui::GetStyle();

	if (!panel->autoResize) temp_flags = 0;
	else if (temp_flags) {
		if (temp_fr_count++ > 1) {
			flags |= temp_flags;
			temp_flags = 0;
			temp_fr_count = 0;
		}
	}

	bool use_tinyfont = panel->tinyFont;
	if (global_use_tinyfont || use_tinyfont) ImGui::PushFont(tinyFont);
	else ImGui::PushFont(defaultFont);

	uint32_t old_enabled = panel->enabled;
	ImVec2 ICON_SIZE(ImGui::GetFont()->FontSize+2, ImGui::GetFont()->FontSize);
	ImColor white(ImColor(bgdm_colors[BGDM_COL_TEXT1].col));
	ImColor old_border = style.Colors[ImGuiCol_Border];
	ImGui::PushStyleColor(ImGuiCol_Border, ImColor(bgdm_colors[BGDM_COL_TARGET_LOCK_BG].col));
	ImGui::SetNextWindowPos(ImVec2((float)x, (float)y), ImGuiSetCond_FirstUseEver);
	if (!ImGui::Begin(LOCALTEXT(TEXT_BGDM_MAIN_PANEL_BUFF_UPTIME), (bool*)&panel->enabled, ImVec2(575, 215), panel->fAlpha, flags))
	{
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopFont();
		return;
	}

	// was close button pressed 
	// uint32_t old_enabled = panel->enabled;
	if (old_enabled != panel->enabled)
		PanelSaveColumnConfig(panel);

	ImGui::PushStyleColor(ImGuiCol_Border, old_border);

	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1))
	{
		ImRect rect = ImGui::GetCurrentWindow()->TitleBarRect();
		if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max, false))
			ImGui::OpenPopup("CtxMenu");
	}
	if (ImGui::BeginPopup("CtxMenu"))
	{
		if (ImGui::BeginMenu(LOCALTEXT(TEXT_GLOBAL_OPTIONS)))
		{
			ShowWindowOptionsMenu(panel, &temp_flags);

			if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_PANEL_OPTS_LINENO), "", &panel->cfg.lineNumbering)) {
				config_set_int(panel->section, "LineNumbering", panel->cfg.lineNumbering);
			}
			if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_PANEL_OPTS_PROFCOL), "", &panel->cfg.useProfColoring)) {
				config_set_int(panel->section, "ProfessionColoring", panel->cfg.useProfColoring);
			}
			ImGui::EndMenu();
		}
		if (BuildBuffColumnsMenu(panel))
			temp_flags = ImGuiWindowFlags_AlwaysAutoResize;

		if (ImGui::BeginMenu(LOCALTEXT(TEXT_BUFF_OPT_ICONS)))
		{
			bool needsUpdate = false;
			bool tmp_bool = (g_state.icon_pack_no == 0);
			if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_BUFF_ICONS_STD), "", &tmp_bool)) {
				needsUpdate = true;
				g_state.icon_pack_no = 0;
			}
			tmp_bool = (g_state.icon_pack_no == 1);
			if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_BUFF_ICONS_CONTRAST), "", &tmp_bool)) {
				needsUpdate = true;
				g_state.icon_pack_no = 1;
			}
			tmp_bool = (g_state.icon_pack_no == 2);
			if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_BUFF_ICONS_DARK), "", &tmp_bool)) {
				needsUpdate = true;
				g_state.icon_pack_no = 2;
			}
			if (needsUpdate) {
				config_set_int(CONFIG_SECTION_BUFF_UPTIME, "IconPackNo", g_state.icon_pack_no);
			}
			ImGui::Separator();

			if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_BUFF_ICONS_DOWN), "", &g_state.use_downed_enemy_icon)) {
				config_set_int(CONFIG_SECTION_BUFF_UPTIME, "UseDownedEnemyIcon", g_state.use_downed_enemy_icon);
			}
			if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_BUFF_ICONS_SEAWEED), "", &g_state.use_seaweed_icon)) {
				config_set_int(CONFIG_SECTION_BUFF_UPTIME, "UseSeaweedSaladIcon", g_state.use_seaweed_icon);
			}
			ImGui::EndMenu();
		}
		ShowCombatMenu();
		ImGui::EndPopup();
	}

	ImGui::PushStyleColor(ImGuiCol_Text, ImColor(bgdm_colors[BGDM_COL_TEXT2].col));

	int nCol = 1;
	for (int i = 1; i < BUFF_COL_END; ++i) {
		if (panel->cfg.col_visible[i])
			nCol++;
	}

	ImGui::Columns(nCol, NULL, true);
	SetColumnOffsets(panel, 1);
	bool sortFound = false;
	for (int i = 0; i < BUFF_COL_END; ++i) {
		if (!panel->cfg.col_visible[i])
			continue;
		if (i == panel->cfg.sortCol) {
			sortFound = true;
			if (UP_ARROW && DOWN_ARROW) {
				ImGui::Image(panel->cfg.asc ? UP_ARROW : DOWN_ARROW, ICON_SIZE);
			}
			else {
				ImGui::PushFont(tinyFont);
				ImGui::TextColored(white, panel->cfg.asc ? ">" : "<");
				ImGui::PopFont();
			}
		}
		ImGui::NextColumn();
	}
	if (!sortFound) ImGui::NewLine();

	ImGui::Columns(nCol, "BuffUptime", true);
	SetColumnOffsets(panel, 1);
	const ImVec4 colorActive = style.Colors[ImGuiCol_HeaderActive];
	const ImVec4 colorHover = style.Colors[ImGuiCol_HeaderHovered];
	for (int i = 0; i < BUFF_COL_END; ++i) {
		if (!panel->cfg.col_visible[i])
			continue;

		bool isClicked = false;
		LPVOID img = BuffIconFromColumn(i);
		ImGui::PushStyleColor(ImGuiCol_Text, white);
		if (img) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, colorActive);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colorHover);
			int frame_padding = 0;
			ImGui::SameLine(6);
			if (ImGui::ImageButton(img, use_tinyfont ? ImVec2(18, 18) : ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), frame_padding, ImColor(0, 0, 0, 0))) {
				isClicked = true;
			}
			ImGui::PopStyleColor(3);
		}
		else if (i == 0) { ImGui::Selectable(LOCALTEXT(TEXT_GDPS_COLAC_NAME)); }
		else if (ImGui::Selectable(buff_col_str[i])) {}
		if (i > 0 && ImGui::BeginPopupContextItem(buff_col_str[i])) {
			ImGui::Text(BuffTextFromColumn(i));
			ImGui::Separator();
			if (ImGui::Selectable(LOCALTEXT(TEXT_GLOBAL_HIDE))) {
				panel->cfg.col_visible[i] = false;
				PanelSaveColumnConfig(panel);
				temp_flags = ImGuiWindowFlags_AlwaysAutoResize;
				ImGui::EndPopup();
				ImGui::PopStyleColor();
				goto abort;
			}
			ImGui::EndPopup();
		}
		isClicked = img ? isClicked : ImGui::IsItemClicked();
		if (isClicked) {
			panel->cfg.sortCol = i;
			if (panel->cfg.sortColLast != BUFF_COL_END &&
				panel->cfg.sortColLast == panel->cfg.sortCol)
				panel->cfg.asc ^= 1;
			else
				panel->cfg.asc = 0;
			panel->cfg.sortColLast = i;
			PanelSaveColumnConfig(panel);
		}
		ImGui::PopStyleColor();
		ImGui::NextColumn();
	}

	ImGui::Separator();
	for (u32 i = 0; i < num; ++i)
	{
		ClosePlayer *player = &players[i];
		float dur = player->cd.duration ? player->cd.duration / 1000.0f : 0;

		for (int j = 0; j < BUFF_COL_END; ++j) {
			if (!panel->cfg.col_visible[j])
				continue;

			if (j > BUFF_COL_SUB && dur == 0) {
				ImGui::NextColumn();
				continue;
			}
			
			switch (j) {
			case(BUFF_COL_NAME):
			{
				const ATL::CAtlStringW unicode = players[i].name;
				const ATL::CAtlStringA utf8 = CW2A(unicode, CP_UTF8);
				ATL::CAtlStringA no("");
				if (panel->cfg.lineNumbering) no.Format((i<9) ? "%d. " : "%d.", i+1);
				ImGui::TextColored(ColorFromCharacter(&player->c, panel->cfg.useProfColoring), u8"%s%s", no, utf8);
			} break;
			case(BUFF_COL_CLS):
			{
				LPVOID img = ProfessionImageFromPlayer(&players[i].c);
				if (img) ImGui::Image(img, ICON_SIZE);
			} break;
			case(BUFF_COL_SUB):
			{
				if (player->subGroup) ImGui::Text("%d", player->subGroup);
			} break;
			case (BUFF_COL_DOWN):
			{
				ImGui::Text("%d", player->cd.noDowned);
			} break;

#define PERCENT_STR(x, dura) {							\
		u32 per = (u32)(x / (f32)(dura) * 100.0f);		\
		per = per > 100 ? 100 : per;					\
		if (per>99) ImGui::SameLine(3);					\
		ImGui::Text("%u%%", per);						\
		}

			case(BUFF_COL_SCLR):
				PERCENT_STR(player->cd.sclr_dura, player->cd.duration);
				break;
			case(BUFF_COL_SEAW):
				PERCENT_STR(player->cd.seaw_dura, player->cd.duration500);
				break;
			case(BUFF_COL_PROT):
				PERCENT_STR(player->cd.prot_dura, player->cd.duration);
				break;
			case(BUFF_COL_QUIK):
				PERCENT_STR(player->cd.quik_dura, player->cd.duration);
				break;
			case(BUFF_COL_ALAC):
				PERCENT_STR(player->cd.alac_dura, player->cd.duration);
				break;
			case(BUFF_COL_FURY):
				PERCENT_STR(player->cd.fury_dura, player->cd.duration);
				break;
			case(BUFF_COL_MIGHT):
			{
				if (player->cd.might_avg >= 10.0f) ImGui::SameLine(3);
				ImGui::Text("%.1f", player->cd.might_avg);
			} break;
			case(BUFF_COL_GOTL):
			{
				ImGui::Text("%.1f", player->cd.gotl_avg);
			}break;

			case(BUFF_COL_GLEM):
				PERCENT_STR(player->cd.glem_dura, player->cd.duration);
				break;
			case(BUFF_COL_VIGOR):
				PERCENT_STR(player->cd.vigor_dura, player->cd.duration);
				break;
			case(BUFF_COL_SWIFT):
				PERCENT_STR(player->cd.swift_dura, player->cd.duration);
				break;
			case(BUFF_COL_STAB):
				PERCENT_STR(player->cd.stab_dura, player->cd.duration);
				break;
			case(BUFF_COL_RETAL):
				PERCENT_STR(player->cd.retal_dura, player->cd.duration);
				break;
			case(BUFF_COL_RESIST):
				PERCENT_STR(player->cd.resist_dura, player->cd.duration);
				break;
			case(BUFF_COL_REGEN):
				PERCENT_STR(player->cd.regen_dura, player->cd.duration);
				break;
			case(BUFF_COL_AEGIS):
				PERCENT_STR(player->cd.aegis_dura, player->cd.duration);
				break;
			case(BUFF_COL_BANS):
				PERCENT_STR(player->cd.bans_dura, player->cd.duration);
				break;
			case(BUFF_COL_BAND):
				PERCENT_STR(player->cd.band_dura, player->cd.duration);
				break;
			case(BUFF_COL_BANT):
				PERCENT_STR(player->cd.bant_dura, player->cd.duration);
				break;
			case(BUFF_COL_EA):
				PERCENT_STR(player->cd.warEA_dura, player->cd.duration);
				break;
			case(BUFF_COL_SPOTTER):
				PERCENT_STR(player->cd.spot_dura, player->cd.duration);
				break;
			case(BUFF_COL_FROST):
				PERCENT_STR(player->cd.frost_dura, player->cd.duration);
				break;
			case(BUFF_COL_SUN):
				PERCENT_STR(player->cd.sun_dura, player->cd.duration);
				break;
			case(BUFF_COL_STORM):
				PERCENT_STR(player->cd.storm_dura, player->cd.duration);
				break;
			case(BUFF_COL_STONE):
				PERCENT_STR(player->cd.stone_dura, player->cd.duration);
				break;
			case(BUFF_COL_ENGPP):
				PERCENT_STR(player->cd.engPPD_dura, player->cd.duration);
				break;
			case(BUFF_COL_REVNR):
				PERCENT_STR(player->cd.revNR_dura, player->cd.duration);
				break;
			case(BUFF_COL_REVAP):
				PERCENT_STR(player->cd.revAP_dura, player->cd.duration);
				break;
			case(BUFF_COL_REVRD):
				PERCENT_STR(player->cd.revRD_dura, player->cd.duration);
				break;
			case(BUFF_COL_ELESM):
				PERCENT_STR(player->cd.eleSM_dura, player->cd.duration);
				break;
			case(BUFF_COL_GRDSN):
				PERCENT_STR(player->cd.grdSIN_dura, player->cd.duration);
				break;
			case(BUFF_COL_NECVA):
				PERCENT_STR(player->cd.necVA_dura, player->cd.duration);
				break;
			case (BUFF_COL_TIME):
			{
				ImGui::Text("%dm %.2fs", (int)dur / 60, fmod(dur, 60));
			} break;

			}
			ImGui::NextColumn();
		}
	}

abort:
	ImGui::PopStyleColor(2); // Text/Separator
	ImGui::End();
	ImGui::PopStyleColor(); // border
	ImGui::PopFont();
}

#if !(defined BGDM_TOS_COMPLIANT)

void DrawSkin(EquipItem *item)
{
	const ATL::CAtlStringW unicode = (item->ptr && item->skin_def.name) ? item->skin_def.name : L"";
	const ATL::CAtlStringA utf8 = (item->ptr && item->skin_def.name) ? CW2A(unicode, CP_UTF8) : LOCALTEXT(TEXT_GLOBAL_NONE_CAPS);
	ImGui::Text(utf8);
}

void DrawFashion(struct Character *c, EquipItems* eq, bool bLocalizedText)
{
	ImColor white(ImColor(bgdm_colors[BGDM_COL_TEXT1].col));

	ImGui::Columns(2, "Skins", false);
	ImGui::SetColumnOffset(1, 70);
	ImGui::TextColored(white, LOCALTEXT(TEXT_GEAR_BACK)); ImGui::NextColumn();
	DrawSkin(&eq->back); ImGui::NextColumn();
	ImGui::TextColored(white, LOCALTEXT(TEXT_GEAR_HEAD)); ImGui::NextColumn();
	DrawSkin(&eq->head); ImGui::NextColumn();
	ImGui::TextColored(white, LOCALTEXT(TEXT_GEAR_SHOULDER)); ImGui::NextColumn();
	DrawSkin(&eq->shoulder); ImGui::NextColumn();
	ImGui::TextColored(white, LOCALTEXT(TEXT_GEAR_CHEST)); ImGui::NextColumn();
	DrawSkin(&eq->chest); ImGui::NextColumn();
	ImGui::TextColored(white, LOCALTEXT(TEXT_GEAR_GLOVES)); ImGui::NextColumn();
	DrawSkin(&eq->gloves); ImGui::NextColumn();
	ImGui::TextColored(white, LOCALTEXT(TEXT_GEAR_LEGS)); ImGui::NextColumn();
	DrawSkin(&eq->leggings); ImGui::NextColumn();
	ImGui::TextColored(white, LOCALTEXT(TEXT_GEAR_BOOTS)); ImGui::NextColumn();
	DrawSkin(&eq->boots); ImGui::NextColumn();
	ImGui::Separator();
	if (eq->wep1_main.ptr) {
		ImGui::TextColored(white, LOCALTEXT(TEXT_GEAR_WEP1M)); ImGui::NextColumn();
		DrawSkin(&eq->wep1_main); ImGui::NextColumn();
	}
	if (eq->wep1_off.ptr) {
		ImGui::TextColored(white, LOCALTEXT(TEXT_GEAR_WEP1O)); ImGui::NextColumn();
		DrawSkin(&eq->wep1_off); ImGui::NextColumn();
	}
	if (eq->wep2_main.ptr) {
		ImGui::TextColored(white, LOCALTEXT(TEXT_GEAR_WEP2M)); ImGui::NextColumn();
		DrawSkin(&eq->wep2_main); ImGui::NextColumn();
	}
	if (eq->wep2_off.ptr) {
		ImGui::TextColored(white, LOCALTEXT(TEXT_GEAR_WEP2O)); ImGui::NextColumn();
		DrawSkin(&eq->wep2_off); ImGui::NextColumn();
	}
}

void DrawSpec(struct Character *c, Spec* spec, bool bLocalizedText)
{
	ImColor white(ImColor(bgdm_colors[BGDM_COL_TEXT1].col));
	
	EquipItems eq = { 0 };
	if (Ch_GetInventory(c->cptr, &eq)) {

		ImGui::Columns(3, "Weapons", false);
		ImGui::SetColumnOffset(1, 100);
		ImGui::SetColumnOffset(2, 160);
		ImGui::TextColored(white, LOCALTEXT(TEXT_GEAR_WEPS));
		ImGui::NextColumn();
		if (eq.wep1_main.ptr) ImGui::Text(wep_name_from_id(eq.wep1_main.wep_type));
		if (eq.wep1_off.ptr) ImGui::Text(wep_name_from_id(eq.wep1_off.wep_type));
		ImGui::NextColumn();
		if (eq.wep2_main.ptr) ImGui::Text(wep_name_from_id(eq.wep2_main.wep_type));
		if (eq.wep2_off.ptr) ImGui::Text(wep_name_from_id(eq.wep2_off.wep_type));
		ImGui::NextColumn();
		ImGui::Separator();
	}

	ImGui::Columns(2, "Spec", false);
	ImGui::SetColumnOffset(1, 100);

	for (u32 i = 0; i < GW2::SPEC_SLOT_END; ++i)
	{
		if (bLocalizedText) {
			const ATL::CAtlStringW unicode = spec->specs[i].name;
			const ATL::CAtlStringA utf8 = CW2A(unicode, CP_UTF8);
			ImGui::TextColored(white, utf8);
		}
		else {
			ImGui::TextColored(white, spec_name_from_id(spec->specs[i].id));
		}
		ImGui::NextColumn();

		for (u32 j = 0; j < GW2::TRAIT_SLOT_END; ++j) {

			if (bLocalizedText) {
				const ATL::CAtlStringW unicode = spec->traits[i][j].name;
				const ATL::CAtlStringA utf8 = CW2A(unicode, CP_UTF8);
				ImGui::Text(utf8);
			}
			else {
				ImGui::Text(trait_name_from_id(spec->specs[i].id, spec->traits[i][j].id));
			}
		}
		ImGui::NextColumn();
	}
}


static inline bool WeaponIs2H(uint32_t type)
{
	switch (type)
	{
	case (GW2::WEP_TYPE_HAMMER):
	case (GW2::WEP_TYPE_LONGBOW):
	case (GW2::WEP_TYPE_SHORTBOW):
	case (GW2::WEP_TYPE_GREATSWORD):
	case (GW2::WEP_TYPE_RIFLE):
	case (GW2::WEP_TYPE_STAFF):
		return true;
	default:
		return false;
	}
}

static __inline ImColor ImColorFromRarity(u32 rarity)
{
	ImColor color(bgdm_colors[BGDM_COL_TEXT2].col);

	switch (rarity) {

	case (ITEM_RARITY_NONE):
		color = IMGUI_WHITE;
		break;
	case (ITEM_RARITY_FINE):
		color = IMGUI_LIGHT_BLUE;
		break;
	case (ITEM_RARITY_MASTER):
		color = IMGUI_GREEN;
		break;
	case (ITEM_RARITY_RARE):
		color = IMGUI_YELLOW;
		break;
	case (ITEM_RARITY_EXOTIC):
		color = IMGUI_ORANGE;
		break;
	case (ITEM_RARITY_ASCENDED):
		color = IMGUI_PINK;
		break;
	case (ITEM_RARITY_LEGENDARY):
		color = IMGUI_PURPLE;
		break;
	default:
		break;
	}
	return color;
}

static __inline const wchar_t *last2Words(const wchar_t *wstr)
{
	if (!wstr)
		return NULL;

	size_t len = wcslen(wstr);
	int w = 0;
	while (--len > 0) {
		if (wstr[len] == ' ')
			if (++w == 2)
				return &wstr[len + 1];
	}
	return wstr;
}

void DrawItem(const char *slot, const EquipItem *eqItem, u32* ar, bool bLocalizedText, bool use_tinyFont)
{
	ImColor white(ImColor(bgdm_colors[BGDM_COL_TEXT1].col));
	ImGui::TextColored(white, slot);
	ImGui::NextColumn();

	if (!bLocalizedText) {
		ImGui::TextColored(ImColorFromRarity(eqItem->item_def.rarity), stat_name_from_id(eqItem->stat_def.id));
	}
	else {
		const ATL::CAtlStringW unicode = eqItem->ptr ? (eqItem->stat_def.name ? eqItem->stat_def.name : L"") : L"";
		const ATL::CAtlStringA utf8 = eqItem->ptr ? (eqItem->stat_def.name ? CW2A(unicode, CP_UTF8) : LOCALTEXT(TEXT_GEAR_NOSTATS)) : LOCALTEXT(TEXT_GEAR_NOEQUIP);
		ImGui::TextColored(ImColorFromRarity(eqItem->item_def.rarity), utf8);
	}

	bool isArmor = false;
	bool isWeapon = false;
	bool isAccessory = false;

	if (eqItem->type < GW2::EQUIP_SLOT_ACCESSORY1) {
		isArmor = true;
	}
	else if (eqItem->type > GW2::EQUIP_SLOT_AMULET) {
		isWeapon = true;
	}
	else {
		isAccessory = true;
	}

	if (isWeapon) ImGui::TextColored(ImColorFromRarity(eqItem->item_def.rarity), wep_name_from_id(eqItem->wep_type));

	if (isWeapon || isArmor ||
		(isAccessory && (eqItem->item_def.rarity < GW2::RARITY_ASCENDED || eqItem->infus_len == 0)))
	{
		// Upgrade #1, skip it for ascended accessories
		// as they can't have an upgrade only infusions
		if (!bLocalizedText) {
			const char *name = isWeapon ? sigil_name_from_id(eqItem->upgrade1.id) : rune_name_from_id(eqItem->upgrade1.id);
			ImGui::TextColored(ImColorFromRarity(eqItem->upgrade1.rarity), name);
		}
		else {
			const ATL::CAtlStringW unicode = eqItem->upgrade1.name ? last2Words(eqItem->upgrade1.name) : L"";
			const ATL::CAtlStringA utf8 = eqItem->upgrade1.name ? CW2A(unicode, CP_UTF8) : LOCALTEXT(TEXT_GLOBAL_NONE_CAPS);
			ImGui::TextColored(ImColorFromRarity(eqItem->upgrade1.rarity), utf8);
		}
	}

	if (isWeapon && WeaponIs2H(eqItem->wep_type)) {

		// Upgrade #2 only valid for 2h weapons
		if (!bLocalizedText) {
			ImGui::TextColored(ImColorFromRarity(eqItem->upgrade2.rarity), sigil_name_from_id(eqItem->upgrade2.id));
		}
		else {
			const ATL::CAtlStringW unicode = eqItem->upgrade2.name ? last2Words(eqItem->upgrade2.name) : L"";
			const ATL::CAtlStringA utf8 = eqItem->upgrade2.name ? CW2A(unicode, CP_UTF8) : LOCALTEXT(TEXT_GLOBAL_NONE_CAPS);
			ImGui::TextColored(ImColorFromRarity(eqItem->upgrade2.rarity), utf8);
		}
	}

	u32 items_per_line = (isWeapon || bLocalizedText) ? 1 : 2;
	for (int i = 0; i < eqItem->infus_len; i++) {

		if (i%items_per_line == 1)
		{
			ImGui::SameLine();
			ImGui::Text("|");
			ImGui::SameLine();
		}

		if (!bLocalizedText) {
			const char* name = infusion_name_from_id(eqItem->infus_arr[i].id, ar);
			ImGui::TextColored(ImColorFromRarity(eqItem->infus_arr[i].rarity), name);
		}
		else {
			const ATL::CAtlStringW unicode = eqItem->infus_arr[i].name ? eqItem->infus_arr[i].name : L"";
			const ATL::CAtlStringA utf8 = eqItem->infus_arr[i].name ? CW2A(unicode, CP_UTF8) : LOCALTEXT(TEXT_GLOBAL_NONE_CAPS);
			ImGui::TextColored(ImColorFromRarity(eqItem->infus_arr[i].rarity), utf8);
		}
	}

	ImGui::NextColumn();
}

void DrawInventory(struct Character *c, const EquipItems* eq, bool bLocalizedText, bool use_tinyFont)
{
	u32 ar_armor = 0;
	u32 ar_wep1m = 0;
	u32 ar_wep1o = 0;
	u32 ar_wep2m = 0;
	u32 ar_wep2o = 0;

	ImGui::Spacing(); ImGui::Spacing();

	ImGui::Columns(4, "Inventory", false);

	if (!bLocalizedText) {
		ImGui::SetColumnOffset(1, 70);
		ImGui::SetColumnOffset(2, 170);
		ImGui::SetColumnOffset(3, 240);
	}
	else {
		ImGui::SetColumnOffset(1, 70);
		ImGui::SetColumnOffset(2, 210);
		ImGui::SetColumnOffset(3, 280);
	}

	DrawItem(LOCALTEXT(TEXT_GEAR_HEAD), &eq->head, &ar_armor, bLocalizedText, use_tinyFont);
	DrawItem(LOCALTEXT(TEXT_GEAR_BACK), &eq->back, &ar_armor, bLocalizedText, use_tinyFont);

	DrawItem(LOCALTEXT(TEXT_GEAR_SHOULDER), &eq->shoulder, &ar_armor, bLocalizedText, use_tinyFont);
	DrawItem(LOCALTEXT(TEXT_GEAR_EAR1), &eq->acc_ear1, &ar_armor, bLocalizedText, use_tinyFont);

	DrawItem(LOCALTEXT(TEXT_GEAR_CHEST), &eq->chest, &ar_armor, bLocalizedText, use_tinyFont);
	DrawItem(LOCALTEXT(TEXT_GEAR_EAR2), &eq->acc_ear2, &ar_armor, bLocalizedText, use_tinyFont);

	DrawItem(LOCALTEXT(TEXT_GEAR_GLOVES), &eq->gloves, &ar_armor, bLocalizedText, use_tinyFont);
	DrawItem(LOCALTEXT(TEXT_GEAR_RING1), &eq->acc_ring1, &ar_armor, bLocalizedText, use_tinyFont);

	DrawItem(LOCALTEXT(TEXT_GEAR_LEGS), &eq->leggings, &ar_armor, bLocalizedText, use_tinyFont);
	DrawItem(LOCALTEXT(TEXT_GEAR_RING2), &eq->acc_ring2, &ar_armor, bLocalizedText, use_tinyFont);

	DrawItem(LOCALTEXT(TEXT_GEAR_BOOTS), &eq->boots, &ar_armor, bLocalizedText, use_tinyFont);
	DrawItem(LOCALTEXT(TEXT_GEAR_AMULET), &eq->acc_amulet, &ar_armor, bLocalizedText, use_tinyFont);

	DrawItem(LOCALTEXT(TEXT_GEAR_WEP1M), &eq->wep1_main, &ar_wep1m, bLocalizedText, use_tinyFont);
	DrawItem(LOCALTEXT(TEXT_GEAR_WEP2M), &eq->wep2_main, &ar_wep2m, bLocalizedText, use_tinyFont);

	if (!WeaponIs2H(eq->wep1_main.wep_type))
		DrawItem(LOCALTEXT(TEXT_GEAR_WEP1O), &eq->wep1_off, &ar_wep1o, bLocalizedText, use_tinyFont);
	else { ImGui::NextColumn(); ImGui::NextColumn(); }
	if (!WeaponIs2H(eq->wep2_main.wep_type))
		DrawItem(LOCALTEXT(TEXT_GEAR_WEP2O), &eq->wep2_off, &ar_wep2o, bLocalizedText, use_tinyFont);

	u32 ar_main = ar_armor;
	u32 ar_alt = ar_armor;

	if (WeaponIs2H(eq->wep1_main.wep_type)) {
		ar_main += ar_wep1m;
	}
	else {
		if (eq->wep1_main.ptr)
			ar_main += ar_wep1m;
		else if (!WeaponIs2H(eq->wep2_main.wep_type) && eq->wep2_main.ptr)
			ar_main += ar_wep2m;
		if (eq->wep1_off.ptr)
			ar_main += ar_wep1o;
		else if (!WeaponIs2H(eq->wep2_main.wep_type) && eq->wep2_off.ptr)
			ar_main += ar_wep2o;
	}
	if (WeaponIs2H(eq->wep2_main.wep_type)) {
		ar_alt += ar_wep2m;
	}
	else {
		if (eq->wep2_main.ptr)
			ar_alt += ar_wep2m;
		else if (!WeaponIs2H(eq->wep1_main.wep_type) && eq->wep1_main.ptr)
			ar_alt += ar_wep1m;
		if (eq->wep2_off.ptr)
			ar_alt += ar_wep2o;
		else if (!WeaponIs2H(eq->wep1_main.wep_type) && eq->wep1_off.ptr)
			ar_alt += ar_wep1o;
	}

	if (ar_main > 0 || ar_alt > 0) {
		ImColor white(ImColor(bgdm_colors[BGDM_COL_TEXT1].col));
		ImColor pink(IMGUI_PINK);
		ImGui::Spacing();
		ImGui::Columns(2, NULL, false);
		ImGui::Separator();
		ImGui::SetColumnOffset(1, 70);
		ImGui::TextColored(white, LOCALTEXT(TEXT_GEAR_AR_TOTAL));
		ImGui::NextColumn();
		ImGui::TextColored(pink, "%s %d", LOCALTEXT(TEXT_GEAR_AR_MAIN), ar_main);
		ImGui::TextColored(pink, "%s %d", LOCALTEXT(TEXT_GEAR_AR_ALT), ar_alt);
	}
}

void ImGui_CharInspect(struct Panel* panel, struct Character *c)
{
	ImGuiWindowFlags flags = 0;
	if (minimal_panels) flags |= MIN_PANEL_FLAGS;
	if (IsDisableInputOrActionCam()) flags |= NO_INPUT_FLAG;

	if (!panel->enabled) return;

	ImGuiStyle &style = ImGui::GetStyle();

	if (!panel->init) {
			flags &= !NO_INPUT_FLAG;
			panel->init = true;
	}

	if (!panel->autoResize) panel->tmp_fl = 0;
	else if (panel->tmp_fl) {
		if (panel->tmp_fr++ > 1) {
			flags |= panel->tmp_fl;
			panel->tmp_fl = 0;
			panel->tmp_fr = 0;
		}
	}

	int mode = panel->mode;
	panel->enabled = mode + 1;

	const char *TITLE_SELF = LOCALTEXT(TEXT_GEAR_TITLE_SELF);
	const char *TITLE_TARGET = LOCALTEXT(TEXT_GEAR_TITLE_TARGET);
	bool isControlledPlayer = g_player.c.cptr == c->cptr;


	bool use_tinyfont = panel->tinyFont;
	if (global_use_tinyfont || use_tinyfont) ImGui::PushFont(tinyFont);
	else ImGui::PushFont(defaultFont);

	uint32_t old_enabled = panel->enabled;
	ImVec2 ICON_SIZE = (global_use_tinyfont || use_tinyfont) ? ImVec2(18, 18) : ImVec2(22, 22);
	ImColor white(ImColor(bgdm_colors[BGDM_COL_TEXT1].col));
	ImColor old_border = style.Colors[ImGuiCol_Border];
	ImGui::PushStyleColor(ImGuiCol_Border, ImColor(bgdm_colors[BGDM_COL_TARGET_LOCK_BG].col));
	ImGui::SetNextWindowPos(ImVec2((float)panel->pos.x, (float)panel->pos.y), ImGuiSetCond_FirstUseEver);
	if (!ImGui::Begin(isControlledPlayer ? TITLE_SELF : TITLE_TARGET, (bool*)&panel->enabled, ImVec2(350, 360), panel->fAlpha, flags))
	{
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopFont();
		return;
	}

	// was close button pressed 
	// uint32_t old_enabled = panel->enabled;
	if (old_enabled != panel->enabled)
		PanelSaveColumnConfig(panel);

	ImGui::PushStyleColor(ImGuiCol_Border, old_border);

	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1))
	{
		ImRect rect = ImGui::GetCurrentWindow()->TitleBarRect();
		if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max, false))
			ImGui::OpenPopup("CtxMenu");
	}
	if (ImGui::BeginPopup("CtxMenu"))
	{
		if (ImGui::BeginMenu(LOCALTEXT(TEXT_GLOBAL_OPTIONS)))
		{
			ShowWindowOptionsMenu(panel, &panel->tmp_fl);

			if (ImGui::MenuItemNoPopupClose(LOCALTEXT(TEXT_GEAR_OPT_LOCALTEXT), "", &panel->cfg.useLocalizedText)) {
				panel->tmp_fl = ImGuiWindowFlags_AlwaysAutoResize;
				config_set_int(panel->section, "LocalizedText", panel->cfg.useLocalizedText);
			}
			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}

	ImGui::PushStyleColor(ImGuiCol_Text, ImColor(bgdm_colors[BGDM_COL_TEXT2].col));

	{
		bool needUpdate = false;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		needUpdate |= ImGui::RadioButton(LOCALTEXT(TEXT_GEAR_TAB_GEAR), &mode, 0); ImGui::SameLine();
		needUpdate |= ImGui::RadioButton(LOCALTEXT(TEXT_GEAR_TAB_TRAITS), &mode, 1); ImGui::SameLine();
		needUpdate |= ImGui::RadioButton(LOCALTEXT(TEXT_GEAR_TAB_SKINS), &mode, 2);
		ImGui::PopStyleVar();
		if (needUpdate) {
			panel->mode = mode;
			panel->enabled = mode + 1;
			panel->tmp_fl = ImGuiWindowFlags_AlwaysAutoResize;
			config_set_int(panel->section, "Mode", panel->mode);
		}
	}

	ImGui::Spacing(); ImGui::Spacing();
	{
		LPVOID img = ProfessionImageFromPlayer(c);
		if (img) ImGui::Image(img, ICON_SIZE); ImGui::SameLine();

		const ATL::CAtlStringW unicode = c->name[0] ? c->name : c->decodedName;
		const ATL::CAtlStringA utf8 = CW2A(unicode, CP_UTF8);
		ImGui::TextColored(white, u8"%s", utf8);
	}

	if (mode == 2) {
		EquipItems eq = { 0 };
		if (Ch_GetInventory(c->cptr, &eq))
			DrawFashion(c, &eq, panel->cfg.useLocalizedText);
	}
	else if (mode == 1) {
		Spec spec = { 0 };
		if (Pl_GetSpec(c->pptr, &spec))
			DrawSpec(c, &spec, panel->cfg.useLocalizedText);
	}
	else {
		EquipItems eq = { 0 };
		if (Ch_GetInventory(c->cptr, &eq))
			DrawInventory(c, &eq, panel->cfg.useLocalizedText, global_use_tinyfont || use_tinyfont);
	}

	ImGui::PopStyleColor(2); // Text/Separator
	ImGui::End();
	ImGui::PopStyleColor(); // border
	ImGui::PopFont();
}
#endif


///////////////////////////
// DEBUG FUNCTIONS START //
///////////////////////////

static char	BaseAddrInput[32] = { 0 };

static __inline void CopyToClipboard(const char *str)
{
	StringCchCopyA(BaseAddrInput, sizeof(BaseAddrInput), str);

	// Commented out as it causes delays on EmptyClipboard()
	// seems to works with NULL ptr for OpenClipboard()
	//ImGuiIO& io = ImGui::GetIO();
	if (OpenClipboard((HWND)NULL/*io.ImeWindowHandle*/))
	{
		ATL::CAtlStringA source = str;
		HGLOBAL clipbuffer;
		char * buffer;
		EmptyClipboard();
		clipbuffer = GlobalAlloc(GMEM_DDESHARE, source.GetLength() + 1);
		buffer = (char*)GlobalLock(clipbuffer);
		StringCchCopyA(buffer, source.GetLength() + 1, LPCSTR(source));
		GlobalUnlock(clipbuffer);
		SetClipboardData(CF_TEXT, clipbuffer);
		CloseClipboard();
	}
}

static __inline void SelectableWithCopyHandler(const char *str)
{
	if (ImGui::Selectable(str, false, ImGuiSelectableFlags_AllowDoubleClick)) {
		if (ImGui::IsMouseDoubleClicked(0))
			CopyToClipboard(str);
	}
}
static __inline void ColumnWithCopyHandler(const char *str)
{
	SelectableWithCopyHandler(str);
	ImGui::NextColumn();
}

static __inline void PopupCtxCopy(const char *str)
{
	if (ImGui::BeginPopupContextItem("CtxClipboard"))
	{
		if (ImGui::Selectable("Copy")) {
			CopyToClipboard(str);
		}
		ImGui::EndPopup();
	}
}

static __inline void PrintGamePtr(char *name, uintptr_t ptr, const char* fmt, ...)
{
	ColumnWithCopyHandler(name);

	ATL::CAtlStringA str;
	str.Format("%p", (void *)ptr);
	if (ptr == 0) ImGui::PushStyleColor(ImGuiCol_Text, ImColor(bgdm_colors[BGDM_COL_TEXT3].col));
	ColumnWithCopyHandler(str);
	if (ptr == 0) ImGui::PopStyleColor();

	va_list args;
	va_start(args, fmt);
	str.FormatV(fmt, args);
	va_end(args);
	ColumnWithCopyHandler(str);
}

static __inline void StrWithCopyHandler(const char* fmt, ...)
{
	ATL::CAtlStringA str;
	va_list args;
	va_start(args, fmt);
	str.FormatV(fmt, args);
	va_end(args);
	SelectableWithCopyHandler(str);
}

static __inline void Str2WithCopyHandler(float spacing, const char *name, const char* fmt, ...)
{
	ATL::CAtlStringA str;
	va_list args;
	va_start(args, fmt);
	str.FormatV(fmt, args);
	va_end(args);
	ImGui::Text(name);
	//SelectableWithCopyHandler(name);
	ImGui::SameLine(spacing);
	SelectableWithCopyHandler(str);
}

static __inline void TreeNodeTextWrapped(float wrap_width, const char* name, const char *data)
{
	if (ImGui::TreeNode(name)) {

		if (wrap_width) {
			ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrap_width);
			ImGui::Text(data);
			//StrWithCopyHandler("%s", utf8.GetString());
			ImGui::PopTextWrapPos();
		} else {
			ImGui::TextWrapped(data);
		}
		ImGui::TreePop();
	}
}

static __inline void Vec3WithCopyHandler(float spacing, const char *name, const vec3* pos)
{
	Str2WithCopyHandler(spacing, name,
		"{%03.2f, %03.2f, %03.2f}",
		pos->x, pos->y, pos->z);
}

static __inline void PrintCharacter(const char* title, struct Character* c)
{
	if (!c || c->aptr == 0) {
		ImGui::NextColumn();
		return;
	}

	if (ImGui::TreeNodeEx(title, ImGuiTreeNodeFlags_DefaultOpen)) {

		float spacing = 130;
		ATL::CAtlStringA utf8 = CW2A(c->name, CP_UTF8);
		Str2WithCopyHandler(spacing, "name:", "%s", utf8.GetString());

		utf8 = CW2A(c->decodedName, CP_UTF8);
		Str2WithCopyHandler(spacing, "decodedName:", "%s", utf8.GetString());

		Str2WithCopyHandler(spacing, "aptr:", "%016I64X", c->aptr);
		Str2WithCopyHandler(spacing, "cptr:", "%016I64X", c->cptr);
		Str2WithCopyHandler(spacing, "pptr:", "%016I64X", c->pptr);
		Str2WithCopyHandler(spacing, "bptr:", "%016I64X", c->bptr);
		Str2WithCopyHandler(spacing, "tptr:", "%016I64X", c->tptr);
		Str2WithCopyHandler(spacing, "wmptr:", "%016I64X", Ag_GetWmAgemt(c->aptr));
		Str2WithCopyHandler(spacing, "cbptr:", "%016I64X", c->cbptr);
		Str2WithCopyHandler(spacing, "ctptr:", "%016I64X", c->ctptr);
		Str2WithCopyHandler(spacing, "acct:", "%S", Contact_GetAccountName(c->ctptr));
		Str2WithCopyHandler(spacing, "m_inv:", "%016I64X", c->inventory);

		Str2WithCopyHandler(spacing, "ID:", "%d", c->id);
		Str2WithCopyHandler(spacing, "shard:", "%d", c->shard);
		Str2WithCopyHandler(spacing, "type:", "%d", c->type);
		Str2WithCopyHandler(spacing, "attitude:", "%d", c->attitude);
		Str2WithCopyHandler(spacing, "profession:", "%d", c->profession);
		Str2WithCopyHandler(spacing, "stance:", "%d", c->stance);
		Str2WithCopyHandler(spacing, "race:", "%d", c->race);
		Str2WithCopyHandler(spacing, "gender:", "%d", c->gender);
		Str2WithCopyHandler(spacing, "is_player:", "%d", c->is_player);
		Str2WithCopyHandler(spacing, "is_clone:", "%d", Ch_IsClone(c->cptr));
		Str2WithCopyHandler(spacing, "is_alive:", "%d", Ch_IsAlive(c->cptr));
		Str2WithCopyHandler(spacing, "is_downed:", "%d", Ch_IsDowned(c->cptr));
		Str2WithCopyHandler(spacing, "in_combat:", "%d", c->in_combat);
		Str2WithCopyHandler(spacing, "hp_max:", "%d", c->hp_max);
		Str2WithCopyHandler(spacing, "hp_val:", "%d", c->hp_val);
		Str2WithCopyHandler(spacing, "bb_state:", "%d", c->bb_state);
		Str2WithCopyHandler(spacing, "bb_value:", "%d", c->bb_value);

		POINT pAg = { 0, 0 };
		POINT pMum = { 0, 0 };
		project2d_agent(&pAg, &c->apos, c->aptr, g_now);
		project2d_mum(&pMum, &c->tpos, c->aptr, false);

		Str2WithCopyHandler(spacing, "pos (trans):", "{%03.2f, %03.2f, %03.2f} (%d, %d)", c->tpos.x, c->tpos.y, c->tpos.z, pMum.x, pMum.y);
		Str2WithCopyHandler(spacing, "pos (agent):", "{%03.2f, %03.2f, %03.2f} (%d, %d)", c->apos.x, c->apos.y, c->apos.z, pAg.x, pAg.y);

		Spec spec = { 0 };
		if (Pl_GetSpec(c->pptr, &spec))
		{
			if (ImGui::TreeNode("spec")) {

				for (u32 i = 0; i < GW2::SPEC_SLOT_END; ++i)
				{
					StrWithCopyHandler("Line[%d]:%d", i, spec.specs[i].id);
					for (u32 j = 0; j < GW2::TRAIT_SLOT_END; ++j)
					{
						ImGui::SameLine();
						StrWithCopyHandler("Col[%d]:%d", j, spec.traits[i][j].id);
					}
				}

				ImGui::TreePop();
			}
		}

		bool needpop = false;
		__try {
			if (c->cbptr) {
				hl::ForeignClass combatant = (void*)(c->cbptr);
				if (combatant) {
					hl::ForeignClass buffBar = combatant.get<void*>(OFF_CMBTNT_BUFFBAR);
					if (buffBar) {
						auto buffs = buffBar.get<ANet::Collection<BuffEntry, false>>(OFF_BUFFBAR_BUFF_ARR);
						if (buffs.IsValid()) {

							if (ImGui::TreeNode("buffs")) {

								needpop = true;
								Str2WithCopyHandler(spacing, "buffptr:", "%p", buffBar);
								Str2WithCopyHandler(spacing, "capacity:", "%d", buffs.Capacity());
								Str2WithCopyHandler(spacing, "count:", "%d", buffs.Count());
								Str2WithCopyHandler(spacing, "data:", "%016I64X", buffs.Data());

								ATL::CAtlStringA str;
								for (uint32_t i = 0; i < buffs.Capacity(); i++) {
									BuffEntry be = buffs[i];
									hl::ForeignClass pBuff = be.pBuff;
									if (pBuff) {
										i32 efType = pBuff.get<i32>(OFF_BUFF_EF_TYPE);
										if (i > 0) str.AppendFormat("  ");
										str.AppendFormat("%d:%d", i, efType);
									}
								}

								//static float wrap_width = 350.0f;
								//ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrap_width);
								//ImGui::InputTextMultiline("##buffs", (char*)str.GetString(), str.GetLength(),
								//	ImVec2(wrap_width, ImGui::GetTextLineHeight() * 10), ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_ReadOnly);
								ImGui::TextWrapped(str);
								//ImGui::PopTextWrapPos();

								ImGui::TreePop();
								needpop = false;
							}
						}
					}
				}
			}
		}
		__except (BGDM_EXCEPTION("[PrintCharacter] access violation")) {
			if (needpop) ImGui::TreePop();
		}

		ImGui::TreePop();
	} ImGui::NextColumn();
}

static __inline void PrintTarget(int idx, DPSTarget* t)
{

	if (!t || t->aptr == 0) {
		ImGui::NextColumn(); ImGui::NextColumn(); ImGui::NextColumn();
		return;
	}

	StrWithCopyHandler("%d", idx); ImGui::NextColumn();

	ATL::CAtlStringA str;
	str.Format("%d:%016I64X", t->id, t->aptr);

	if (ImGui::TreeNode(str)) {

		float spacing = 130;

		const wchar_t* name = lru_find(GW2::CLASS_TYPE_AGENT, t->aptr, NULL, 0);
		if (name) {
			ATL::CAtlStringA utf8 = CW2A(name, CP_UTF8);
			Str2WithCopyHandler(spacing, "name:", "%s", utf8.GetString());
		}
		Str2WithCopyHandler(spacing, "aptr:", "%016I64X", t->aptr);
		Str2WithCopyHandler(spacing, "cptr:", "%016I64X", t->cptr);
		Str2WithCopyHandler(spacing, "shard:", "%d", t->shard);
		Str2WithCopyHandler(spacing, "id:", "%d", t->id);
		Str2WithCopyHandler(spacing, "npc_id:", "%d", t->npc_id);
		Str2WithCopyHandler(spacing, "species_id:", "%d", t->species_id);
		Str2WithCopyHandler(spacing, "isDead:", "%d", t->isDead);
		Str2WithCopyHandler(spacing, "invuln:", "%d", t->invuln);
		Str2WithCopyHandler(spacing, "duration:", "%lld", t->duration);
		Str2WithCopyHandler(spacing, "time_begin:", "%lld", t->time_begin);
		Str2WithCopyHandler(spacing, "time_hit:", "%lld", t->time_hit);
		Str2WithCopyHandler(spacing, "time_update:", "%lld", t->time_update);
		Str2WithCopyHandler(spacing, "hp_max:", "%d", t->hp_max);
		Str2WithCopyHandler(spacing, "hp_lost:", "%d", t->hp_lost);
		Str2WithCopyHandler(spacing, "hp_last:", "%d", t->hp_last);
		Str2WithCopyHandler(spacing, "tdmg:", "%d", t->tdmg);
		Str2WithCopyHandler(spacing, "c1dmg:", "%d", t->c1dmg);
		Str2WithCopyHandler(spacing, "c2dmg:", "%d", t->c2dmg);
		Str2WithCopyHandler(spacing, "c1dmg_team:", "%d", t->c1dmg_team);
		Str2WithCopyHandler(spacing, "c2dmg_team:", "%d", t->c2dmg_team);
		Str2WithCopyHandler(spacing, "num_hits:", "%d", t->num_hits);
		Str2WithCopyHandler(spacing, "num_hits_team:", "%d", t->num_hits_team);

		ImGui::TreePop();
	} ImGui::NextColumn();

	StrWithCopyHandler("%lld", t->time_update); ImGui::NextColumn();
}

void ImGui_Debug(struct Panel* panel,
	struct Character *player, struct Character *target,
	struct Array* char_array, struct Array* play_array)
{
	ImGuiWindowFlags flags = 0;

	if (!panel->enabled) return;
	if (minimal_panels) flags |= MIN_PANEL_FLAGS;
	if (IsDisableInputOrActionCam()) flags |= NO_INPUT_FLAG;

	if (is_bind_down(&panel->bind)) {
		show_debug ^= 1;
	}

	if (!show_debug) return;

	if (!panel->init) {
		flags &= !NO_INPUT_FLAG;
		panel->init = true;
	}

	if (!panel->autoResize) panel->tmp_fl = 0;
	else if (panel->tmp_fl) {
		if (panel->tmp_fr++ > 1) {
			flags |= panel->tmp_fl;
			panel->tmp_fl = 0;
			panel->tmp_fr = 0;
		}
	}

	bool use_tinyfont = panel->tinyFont;
	if (global_use_tinyfont || use_tinyfont) ImGui::PushFont(proggyTiny);
	else ImGui::PushFont(proggyClean);

	ImGui::SetNextWindowPos(ImVec2(720, 5), ImGuiSetCond_FirstUseEver);
	if (!ImGui::Begin("BGDM Debug", &show_debug, ImVec2(750, 400), panel->fAlpha, flags))
	{
		ImGui::End();
		ImGui::PopFont();
		return;
	}

	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1))
	{
		ImRect rect = ImGui::GetCurrentWindow()->TitleBarRect();
		if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max, false))
			ImGui::OpenPopup("CtxMenu");
	}
	if (ImGui::BeginPopup("CtxMenu"))
	{
		if (ImGui::BeginMenu("Options"))
		{
			ShowWindowOptionsMenu(panel, &panel->tmp_fl);

			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}

	ImColor white(ImColor(bgdm_colors[BGDM_COL_TEXT1].col));
	ImGui::PushStyleColor(ImGuiCol_Text, ImColor(bgdm_colors[BGDM_COL_TEXT2].col));

	ImGui::PushItemWidth(140);
	if (ImGui::InputText("##baseaddr", BaseAddrInput, sizeof(BaseAddrInput), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) { }
	ImGui::SameLine();
	if (ImGui::Button("Open in Memory Editor", ImVec2(0, 0))) {

		uintptr_t base_addr;
		if (sscanf(BaseAddrInput, "%I64X", &base_addr) == 1)
		{
			if (global_use_tinyfont || use_tinyfont) ImGui::PushFont(proggyTiny);
			else ImGui::PushFont(proggyClean);
			mem_edit.Open((unsigned char*)base_addr);
			ImGui::PopFont();
		}
	}

	if (ImGui::CollapsingHeader("Game Pointers", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Columns(3, "GamePtrs", true);
		ImGui::Separator();
		ImGui::TextColored(white, "NAME"); ImGui::NextColumn();
		ImGui::TextColored(white, "PTR"); ImGui::NextColumn();
		ImGui::TextColored(white, "VALUE"); ImGui::NextColumn();
		ImGui::Separator();

		PrintGamePtr("TlsContext", g_ptrs.pTlsCtx, "");
		PrintGamePtr("CharClientContext", g_ptrs.pCharCliCtx, "");
		PrintGamePtr("ViewAdvanceAgentSelect", g_ptrs.pAgentSelectionCtx, "");
		PrintGamePtr("ViewAdvanceAgentView", g_ptrs.pAgentViewCtx, "");
		PrintGamePtr("ViewAdvanceWorldView", g_ptrs.pWorldView, "%p", (void*)process_read_u64((uintptr_t)g_ptrs.pWorldView));
		PrintGamePtr("ViewAdvanceUi", g_ptrs.pUiCtx, "%d", g_ptrs.pUiCtx ? process_read_i32((uintptr_t)g_ptrs.pUiCtx + OFF_UCTX_SCALE) + 1 : 0);
		PrintGamePtr("Map Open Ctx", g_ptrs.pMapOpen, "%d", g_ptrs.pMapOpen ? *(int*)g_ptrs.pMapOpen : 0);
		PrintGamePtr("UI hide Ctx", g_ptrs.pIfHide, "%d", g_ptrs.pIfHide ? *(int*)g_ptrs.pIfHide : 0);
		PrintGamePtr("Action Cam Ctx", g_ptrs.pActionCam, "%d", g_ptrs.pActionCam ? *(int*)g_ptrs.pActionCam : 0);
		PrintGamePtr("Ping", g_ptrs.pPing, "%d", g_ptrs.pPing ? *(int*)g_ptrs.pPing : 0);
		PrintGamePtr("FPS", g_ptrs.pFps, "%d", g_ptrs.pFps ? *(int*)g_ptrs.pFps : 0);
		ImGui::Separator();
		PrintGamePtr("GetWmAgentFromAgent()", g_ptrs.pfcGetWmAgent, "");
		PrintGamePtr("GetContactCtx()", g_ptrs.pfcGetContactCtx, "%p:%S", (void*)g_ptrs.contactContext,
			Contact_GetAccountName(CtxContact_GetContactSelf(g_ptrs.contactContext))+1);
		PrintGamePtr("GetSquadCtx()", g_ptrs.pfcGetSquadCtx, "%p", (void*)g_ptrs.squadContext);
		PrintGamePtr("DecodeCodedText()", g_ptrs.pfcDecodeCodedText, "");
		PrintGamePtr("CodedTextFromHashId()", g_ptrs.pfcCodedTextFromHashId, "");
		ImGui::Separator();
		PrintGamePtr("WndProc", g_ptrs.pWndProc, "");
		PrintGamePtr("GameThread", g_ptrs.pGameThread, "");
		PrintGamePtr("Combat Log", g_ptrs.cbCombatLog, "");
		PrintGamePtr("Damage Result", g_ptrs.cbDmgLogResult, "");
		ImGui::Columns(1);
		ImGui::Separator();
	}

	if (ImGui::CollapsingHeader("Camera & Mumble Data"))
	{
		ImGui::Columns(2, "MumbleCam");
		ImGui::Separator();

		float spacing = 130;
		LinkedMem* pMum = (LinkedMem*)g_ptrs.pMumble;
		CamData* pCam = (CamData*)g_ptrs.pCam;

		if (!pMum) {
			ImGui::NextColumn();
		}
		else if (ImGui::TreeNodeEx("Mumble Link", ImGuiTreeNodeFlags_DefaultOpen)) {

			Str2WithCopyHandler(spacing, "p_mumble:", "%p", pMum);

			if (pMum) {
				ATL::CAtlStringA utf8 = CW2A((wchar_t*)pMum->name, CP_UTF8);
				Str2WithCopyHandler(spacing, "name:", "%s", utf8.GetString());

				Str2WithCopyHandler(spacing, "fovy:", "%f", get_mum_fovy());

				Vec3WithCopyHandler(spacing, "cam_pos:", &pMum->cam_pos);
				Vec3WithCopyHandler(spacing, "cam_front:", &pMum->cam_front);
				Vec3WithCopyHandler(spacing, "cam_top:", &pMum->cam_top);
				Vec3WithCopyHandler(spacing, "avatar_pos:", &pMum->avatar_pos);
				Vec3WithCopyHandler(spacing, "avatar_front:", &pMum->avatar_front);
				Vec3WithCopyHandler(spacing, "avatar_top:", &pMum->avatar_top);

				Str2WithCopyHandler(spacing, "ui_version:", "%d", pMum->ui_version);
				Str2WithCopyHandler(spacing, "ui_tick:", "%d", pMum->ui_tick);

				Str2WithCopyHandler(spacing, "shard_id:", "%d", pMum->shard_id);
				Str2WithCopyHandler(spacing, "map_id:", "%d", pMum->map_id);
				Str2WithCopyHandler(spacing, "map_type:", "%d", pMum->map_type);
				Str2WithCopyHandler(spacing, "instance:", "%d", pMum->instance);
				Str2WithCopyHandler(spacing, "build_id:", "%d", pMum->build_id);

				Str2WithCopyHandler(spacing, "context_len:", "%d", pMum->context_len);
				TreeNodeTextWrapped(0.0f, "context", (char*)pMum->context);

				utf8 = CW2A((wchar_t*)pMum->identity, CP_UTF8);
				TreeNodeTextWrapped(0.0f, "identity", utf8.GetString());

				utf8 = CW2A((wchar_t*)pMum->description, CP_UTF8);
				TreeNodeTextWrapped(0.0f, "description", utf8.GetString());
			}

			ImGui::TreePop();
		} ImGui::NextColumn();

		if (!pCam) {
			ImGui::NextColumn();
		}
		else {
			
			if (ImGui::TreeNodeEx("Cam Data", ImGuiTreeNodeFlags_DefaultOpen)) {

				Str2WithCopyHandler(spacing, "p_cam:", "%p", pCam);
				Str2WithCopyHandler(spacing, "valid:", "%d", pCam->valid);

				Vec3WithCopyHandler(spacing, "camPos:", (vec3*)&pCam->camPos);
				Vec3WithCopyHandler(spacing, "upVec:", (vec3*)&pCam->upVec);
				Vec3WithCopyHandler(spacing, "lookAt:", (vec3*)&pCam->lookAt);
				Vec3WithCopyHandler(spacing, "viewVec:", (vec3*)&pCam->viewVec);

				Str2WithCopyHandler(spacing, "fovy:", "%f", pCam->fovy);
				Str2WithCopyHandler(spacing, "curZoom:", "%f", pCam->curZoom);
				Str2WithCopyHandler(spacing, "minZoom:", "%f", pCam->minZoom);
				Str2WithCopyHandler(spacing, "maxZoom:", "%f", pCam->maxZoom);

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Misc Data", ImGuiTreeNodeFlags_DefaultOpen)) {

				Str2WithCopyHandler(spacing, "build_id:", "%d", g_build_id);
				Str2WithCopyHandler(spacing, "is_gw2china:", "%d", g_state.is_gw2china ? 1 : 0);
				ImGui::TreePop();
			}

		} ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::Separator();
	}

	if (ImGui::CollapsingHeader("Character Data"))
	{
		ImGui::Columns(2, "Characters");
		ImGui::Separator();
		PrintCharacter("Player", player);
		PrintCharacter("Target", target);
		ImGui::Columns(1);
		ImGui::Separator();
	}

	if (ImGui::CollapsingHeader("Target Data"))
	{
		ImGui::Columns(3, "Targets");
		ImGui::Separator();
		ImGui::SetColumnOffset(1, 30);
		ImGui::TextColored(white, "#"); ImGui::NextColumn();
		ImGui::TextColored(white, "ID:AGENT"); ImGui::NextColumn();
		ImGui::TextColored(white, "TIME"); ImGui::NextColumn();
		ImGui::Separator();

		for (int i = 0; i < MAX_TARGETS; i++) {

			DPSTarget* t = dps_targets_get(i);
			if (t) PrintTarget(i, t);
		}
		
		ImGui::Columns(1);
		ImGui::Separator();
	}

	ImGui::PopStyleColor(1); // Text
	ImGui::End();
	ImGui::PopFont();
}

/////////////////////////
// DEBUG FUNCTIONS END //
/////////////////////////


// File: 'C:\Users\ohad\Documents\fonts\ProggyTiny.ttf' (35656 bytes)
// Exported using binary_to_compressed_c.cpp
static const char proggy_tiny_ttf_compressed_data_base85[10950 + 1] =
	"7])#######LJg=:'/###[),##/l:$#Q6>##5[n42<Vh8H4,>>#/e>11NNV=Bv(*:.F?uu#(gRU.o0XGH`$vhLG1hxt9?W`#,5LsCm<]vf.r$<$u7k;hb';9C'mm?]XmKVeU2cD4Eo3R/"
	"[WB]b(MC;$jPfY.;h^`ItLw6Lh2TlS+f-s$o6Q<BaRTQrU.xfLq$N;$0iR/G0VCf_cW2p/W*q?-qmnUCLYgR`*1mTi+7.nT@C=GH?a9wps_2IH,.TQg1)Q-GL(lf(T(ofL:%SS%MS=C#"
	"jfQ$X7V$t'X#(v#Y9w0#2D$CI]V3N0PRAV3#&5>#X14,MZ[Z##UE31#J&###Q-F%b>-nw'w++GM-]u)Nx0#,M[LH>#Zsvx+6O_^#l(FS7f`C_&E?g'&kcg-6Y,/;M#@2G`Bf%=(`5LF%"
	"fv$8#,+[0#veg>$EB&sQSSDgEKnIS7EM9>Z,KO_/78pQWqJE#$nt-4$F&###E`J&#uU'B#*9D6N;@;=-:U>hL&Y5<-%A9;-Y+Z&P^)9<QYN8VQM#S/Mx?c(NdBxfMKpCEPX;*qM$Q?##"
	"&5>##._L&#awnk+ib*.-Z06X1>LcA#'rB#$o4ve6)fbA#kt72LN.72L=CG&#*iX&#90Wt(F,>>#_03:)`(@2L@^x4Sj2B@PN#[xO8QkJNRR()N@#f.Mr#)t-L5FGMm8#&#/2TkLi3n##"
	"-/5##MwQ0#EB^1v&ols-)-mTMQ@-##qlQ08*lkA#aNRV7KRYML%4_s[kNa=_0Z%7Nd4[3#S@1g_/v`W#'`Fm#<MOe#_=:n#Lx;%$b(w,$g&J1$N9>B$(Q#n$oqvc$&Svv$`,TM%,PS=%"
	"OeJE%s+]l%A=Fe%']K#&7aW5&O-Nd&q&>^&GZs1'w.bA'c>u>'B-1R'%gJ.(t1tx'_jH4(iNdc(GJ*X(l`uf(^Wqr(-=Jx(=[%5)')Gb)$1vV)57Vk),8n<*BYl/*qs%]*OI5R*Fkgb*"
	"H<+q*TQv(+Xak6+?C@H+5SaT+o2VhLKd)k+i$xl+4YW=,sJd,,C*oT,Eb:K,mSPgLsF8e,Z$=rJ[<5J`E:E&#k&bV7uVco]JfaJ2M'8L#xArJ27FJx?Zgt%uov/vZ@?Gj:Kl;,jo%*K;"
	"AL7L#7G'3/J(*t.5xO+/0r+N/%ipJ/Bq_k/A>4Y/^iwl/%K:K0[HW=04D'N0wQq_00Kjt0]NJ21?p?d1T:=Y1e*&i1HLr@28x*:29A[L2Mpd%3pFIp2igO+3aXRX3M#PN3uY$d37p2=4"
	"c,s54.3SI4v0iw4JqN65G$S*5rh<65ld7E5.IRt5.f-16A/U(6IoFR6Nj7I6Y3i[6>s#s6EF=P90>=W6-Mc##=(V$#MXI%#^3=&#nd0'#(?$(#8pm(#HJa)#X%T*#iUG+##1;,#3b.-#"
	"C<x-#Smk.#GdrI3TCR/$3Ds9)?^k-$&pG/?Hn.1#rPr.LR;NHZYu-A-muPG`uqJfLK_v>#$i0B#'2[0#s6aW-AS*wp1W,/$-pZw'%]#AOC+[]O>X`=-9_cHMN8r&MsKH##77N/)8r_G3"
	"=^x]O].[]-/(pI$^=Kn<00k-$t`%/LDK5x76,G&#$or>I?v+sQ;koJM>,CS-14,dM,Hv<-cLH?01FQ*NGx0='H9V&#;Rov$8ooX_i7d;)]]>R*sVi.Lt3NM-$@dXM:uSGMDn%>-30[b'"
	"s6Ct_.39I$3#bo7;FP&#YKh9&#d)KE$tok&L1tY-sTf2LP]K<Lsjr>&s9L]u-c4Au9*A>-<'3UN-PZL-NIV+85p0eZ3:.Q8bj1S*(h)Z$lel,MX_CH-.Nck-(veHZwdJe$ej+_frio0c"
	"KB$HFtRZ>#DiaWqFq7Q84okA#tiUi'Qumo%<]Xl8As(?@iLT[%tDn8gsDGA#hDu-$+HM3X_?@_8:N+q7v3G&#a7>0H3=t-?ZKm.HK+U58E/.`AcQV,tUd+Z-$fQ-Haotl8Zx2Fn)&UQ8"
	"c6E&docd.%&^R]u)x:p.N*wIL8+fsrk+5<MR@v58X^?xKxUi^6A``6MU-lPSgJ$##P*w,v%,[0#Rhi;-`2$I%*nhxu67Np.(AP##Y+YB]LD_K*NPG])IsiA#Dqi05siIfL;G;QMM8-##"
	"?bu&#,>###>jq:9%/v2;f`?J8fDrG%fmWw9gl'ENgjG:,EC%<-WW5x'6eaR86kf2`5alP&u]::.'a0i);c)3LN3wK#gZb19YvMa,?IggL3xoFMTK_P85<B9&NP'##mF#m8$6<QhEn>.)"
	"0xLp7gw]m_oM++.`=JfLm)1#.gGKd4N^@N%M'Np7ZO:k)VTqt%EO`gurjj;-0r%;%I<Ga>'M,W-(hdnXP4bA,%GLp75c<LYo5oMiXKh+0O>`QUWh<_&.ZoDuWmL<LKx(B6eVxZ9,V@Z7"
	"6OM=-Ke?\?.]RXk:UD`?%^FHM&LMQY-SJmDc?1&Z$gq`gMi.(58gkcA#l5#N9#9Z;%Y*K-;8K?E5#0]guh&tP8m7:f[<f568<JtpBUNiF*4db;-[s[n&9o`Y-R7B$L4*XQ8t$,?.Vqa_&"
	"fQB?-/]2u$#JUp7S+5wp=25?R6W5@MA)jB%lpNp7^'9U)jNtKNBU0I,'XFQ/&&###'><h#I[*T.73rI3#1[m%:TUv[NC90/Q]i.L(dt_/1dC_&8QFeFKgL<L+qdU2f$;R3rftK3GiIn&"
	"ddcA,CDkGM'CYcM#c[#%(MgTTc645&L(T&#b:o<<l/tDYp$M3<QQGb@vjfe$i@nEI?ZKal44)=-T4sP-u0@q$:-d9`EQjDNuagC-_1X_$PQ`&#g1iJs&h'a)J`C<-M`B-'sB1tL>CVJ+"
	"7:P&#Wj7n$+8sb<:+R.Qx7m<-T`&0%3TK<-h.oN'eSYW-g7D^6mu<W)>7Rc;:cIH%5hWHX9uCq'RC/2'GZZ(=:.$ekS>k((WP_=-,8dT%;]DeHjNJ'HOsgj-vUa$UFQO68Ic+k2HwQ'("
	"0Kgn8V=:</jUcP(Nir;JdYO&/+mZe;Cmw@^[x8IH2i<w-u$Hq7lB)KE@V)7)'R4tQc*Fv%0DTgLvgjIL%Xi.Lb+pE='Rf3M_o>*(iM]?[]-#9)#tb,;mdUw.SB+-8M*cj)1)A@;:O#ki"
	"pW<78t=vERat3KN(RZ&%0)3XJh/1q+<E3'mJ9?m'as868qukA#>'_5'r1GX`4;kNbkh&@-HCp[+c+Z68=Z9:BM#Jn$R+0Au6A)K:YXr1d^8ILE65V'#Y_%n8Mc`3r:>H9%PMhj9GVCh3"
	"F3wm81EG&#,`**<3AEYLN1pA#>q0p&(^?@'Bl+&>klY'vO%co7juS^d)a#9%=&m9.m`0i)rQNm8*GF]uI9+W-wmw5_L07xLC8qT;`9%90i^Gx'abQp7)>5wLq3n0#/U0V8+B^G3%3,0:"
	"3w<W8.p?#+Hp]p7*MXK3JhpgLE-q8P&(mW-dxr>R]4fJ(d%2N9Z-r_&7rjQS<XpW&&A-W--@;qi&29tUa5N&#gB)gL8,Ap-elVKW5TwR*0l;wPAjbS(()Eb<iU(Y&3:8,($2)49;(fvn"
	"OpTx=Jx#6qqjL<LA?*l2PS&i;d2W&HBfj.L6$[J8b(%df[2Q:8bDew'2N#k9gbY2VAHX*2L0,rBcSWf%AZbqKY9g8)k4ZQ_8dP^0#$$3:,hwW&^?Ie-:Z&Eu:RL>,sM;n9g#>ve^2SK)"
	"71JTRxD)o0@1wWA2#E;<PRZ;%>xov>0f^-QQQYVBeT+?-7kMD5d0B#QZAW0:Z<A^HCkC&Oe4LI89xAs997Um.Xi,FNt-iE)6=nm8<>jUK[OZi&61L&#>CCj;r]/RLH'(j>+$P-R9bF69"
	"`%f@[p-JZ*.hnp7;-ge$NSi?-qx8;-V<ZA+1q8N9tGRWAv9j+(7=DHFC=[Z:UVgY(=5)N<)b)OBsUeA,RgI#P76ZE,3tQnTSSff&N,76LMX[r;%'1'#(AP##r-7G;4akA[ve^@%sVi.L"
	"S_r&&;4qgL>]Z##,B?nrCn,)'(Q%a-sI^W&9'i&#SrRfL`Zwe%k.jA,xf:-%<Gf&#_:JfL:JQi*c/Z)<'7(a6g/mx'aPc689TO]uo<MU'5+WZPi.cE<g(_>$+:t>-v^)'%of?pg=`N_*"
	"o'w<LJb*=-q`6]'Fh0BI@9[&%7bI4VM$D*'C[:RaFCI<-v=B[%7hep7=wRLa#E-v.K#gmA.2(LNqLC2)bqDp7.5HZAm;&_8ekx;8FmR/&mTV:@#CTp9:td>)3(ip7XqF]uN-Fj9l=K/+"
	"sAH^*I=5qBCRt-,T163BO%ov7%,sb&T=XaZ$(#GM0#Qp%a]Cs7HNbxum=g@>wb%?7N:Fk'0PYRhUv-tLWr+P(lLM/:9N*H=KRZT'Pf2;.@2<)#pVl1MwLk0&;tUAuP3w.Le.]T/*Mc##"
	"O->>#9NCU.73rI3ZbA;%^xT3BS2L#$uLjf%53Kt-2SJMBFZ.m0cmcPS)aX%(c]Yg<^[G6;$W(8*2&$X->B+kk^$D'8E@P&#I-nT'u5pm8u;Be=AJ8F-T6po)A:&?-CPcd$rDtJjLUsv'"
	"7Hx_onecgHu78k:D#]4;tb)$-UHAm8h;2c>8J<@.(W=p&oVoY?&@+w7-)ri'bb=+<b2:*S]stQ_=5>X:<Q(Ka=4)=-+'h&,:TKs-#>#29.*DW/tNqT&QAl29xj+AuD:*+lnW]D,3l6<-"
	"PX9YYw)vX&=WuT8H=AbIs[`Am2xcW-jqbn*cZV%_t/Z&QpvGJ(i2.^==iWDurfn:Ml;-##/-U%)x$+1:lROdt*mpM=i4/)Zdr'H'P[N>-EKHl$hUvf:P'Q3`u*IM&uZA39^0F[pUB+n8"
	"hq+?I`L'-)2>Cq71g/6(?(oR&iBRiLr7w;-[HuL3u6e2&V:QjBJ:9iuF8.a<ARjp'0Abr&l&:P9L3B:^7aj(8PK:(QkKLTMsCt?'Yqkd4'DW-2%^Bq7xR[[%i_Nh11uZp7LW^G3(Y0(="
	"DRYF%#jl<-h&*u$;S^,Mw?<K**]I^FG,614Dd@N%$;Ed;0pkKNJl:a%rL@Y-&5n0#TD,##%5w.LCn^-)uH90:H;lA#;qp1(J7rLExpE]F=%,(HFI8V%095g)3fBemf@#kGO5###'5>##"
	"PHT<-.4r1&,qBE<es9B#LG'>#LK,W-fIO4kX@%%#tUB#$57>uHN^KeX'-cD)d.s*<i5qHQhe%D<KIF&#_UK]u**<gLY7<C0t#jgLqWQ>#P<Eh#$`b3(.hFQ/0rC$#P9cY#]TJLC,=5(H"
	")L2Y&SE^=-6hk.?lG8<9c6Bt+=B`&#Ee<wTFMGT&2P###3XG$>2+c&#ok:/:3l(RsPD###2d#<-%,.t$5@HgL/mu<+PXhv[Bgb4)GO;eMZQMr?,tXvIIe;t-P2l?G2j1v^)3l$'mEa68"
	"K1l@7.`V[GG#)C]Y&f;]?OM>&x]i.L(/5##BO+k9Xp0B#NS9>#+7E<-d]nl$Yw6v9YK*6(sxGug]oko$_'l_-Ai%RMq<&_8o2@2L@@AS)c8(<-c&r]$J9oq7g?(m9LIS;H-)KfF@qVI*"
	"^ACO9fKc'&6k/q7RD&Fe&*l2LSQUV$vC#W-lwf&v:'n]%D4xVH4&(^#0jg<-@r'29EQT_6Gx)Q&':nKC>s6.6*;X^ZH.->#>atJC6`hJ>NjO?-^5l-8Tj72LFIRp7,:<Q3$)F/)5Wu>-"
	".wruM;q0)(YHWp7@i('#'2,##8ZDK<cUb5A@]kA#9fRjNh6]0#&P[g)P`0i)0d](%^m[v$q)TVHKS9.v<%SR8<<9*#2<_C-7)hg'Orqs-QEsFMp=h19vO%o<M5,##n[li98-aQL[3=G%"
	"6J>M9dvR3kYKd&#V(f+MR?xK#liDE<[/RM3M7-##1na_$+q)'%xheG*DsXbN^BxK#R90%'vrIfL.r&/LT*=(&A's2;O56>>/jsX_Z_+/(NSi.L>jEG']iNUR238^&?MR49ne<Gm'IVQ("
	"((wQ8*9Xd=.H6h'lv049,uPGG:Jw_-Oghr?PvblrD)TJGlq=42p_N^-e59I$%]4d<r%wd+^1Iu'n94395H72LcojA#^QWp7i0/kXEFEm8TjjM3j^an8JUbA#FS0a=l6G]uVMHW-P-t2D"
	"pAqa)1xF&#l.HB,)'sFl=TA_%6DPT^$ok%(vc<'#Z'[0#SHW-$2W(1:C,QD.Ln5%()ocjK+uZH+o[2n<rh/:)jPHmk0S_GDMgTd4o'1c&X(ek90sTn`pij7'7_j:HY/6ElAGdvDUpK#$"
	"mgJH+.`]&=*6J_60lSc&>A>.M5N=h#=eq:T?k)/:;SF&#;k-gLXL0e,e6JdDNHj?@ihvi&wNT-;6`E.FAl):%3WK<-:EI+'^([:.+lQS%?_,c<8L[W&T7-##`QHXA=(xn&Yqbf(kg<LW"
	"VZx2&&]?68vN6pgnUIu'M6YE<i(72Lgs00:xi8u_)F.^'J5YY#)PjfL=w;/:ilAqBqSBt7r[b&#hEqhLJLvu#DO*e*),8Z>e2-g)[OksHT*bm+Do5IM<_jILK4Pv'=u5a*E#Z^FHmn),"
	"Dheq.+Sl##04kWoAl[W]rYHI%+d@a-.dm<%#1[,MtRBt)O(35&>-f;-J=<n)/on,48,Ut':/;3t&u5*P`^0d%5P'gLvk,6+)>.g:(xp/)U]W]+RCwgCbE0#Aes-h%vr_BF0;=K34Yv',"
	"/GoEYQQ-U&5&Aw>]ewGt?k,l$1oR8VCkF<%+nTH4Z8f%/M&dQ/(2###S`%/L*cS5JX&V_$iac=(LG:;$ZcRPA.$+bHU7-###c7^O1[qS%)S#qT=lI(#=,o_Q.^r#(w1I<-+PK`&,o'^#"
	"GhsWQt6j(,]_<g:%t)qK.h`u&QggR8p0S`ABAla*GK[,MtDNg8IUL^#'5>##IN]p7i4mFuXih@-t=58.1&>uu$&h2`2H:%'T):wPGJuD%5DJTIUXbA#pvRdM=WcO-uhKj%0ej?Ppr<A+"
	"6o*KWQJ,x)lE59)+HL$Gs2J?n(Y]9@3*?$(x%Id;IZO]un4WJ%ncg;-3Fd<-;Q*((VD,C'q3n0#J,,##Jw@D<aNO&#PYkb$S(Fk4:Ru&#()>uu9k,h%cwfi'B.x`=Tg^d4%45GM@iZQ'"
	"YGHP9(MGGG<CD?@lb`^)j()<-X,r]$5rJ&voRl5(4@m=&l#I]uFX9b<,#hdOsBqA.&?sI3w+K?I(kEe$PLRN9De:'#6]###QfB##:a;l9Z^:m9;pd,k#en3)wXN*<W.r_&Y]$O9+]^8i"
	"C,`l--hN,m%VO;&@RG&#]*J788ix?7HZU18T0Y(f;F@q7O51Llmr1hYQw<kkDqkjDx&AKu$-a&#2f<p^oXAZ$`([K:&tF?.T;a38igw-+BB]c;N)man%)(gL$V0`$ilcp7Clp/NCP,3)"
	"Ev$&41Nw`*@0.9@iN7rqS]Ll)H)1W-]3n0#`6%_8I;TGlGR1H-w,TRLb8jn'+.UE,fK^n&SO7m83wgDlG=](WrCeA%ioD(=;+<r7r)`0G8MT&#Dfdf.>vbG3jZJp7FAeEP&e<tDI$f[@"
	"Kn,$%6M.M:12f4V.,j80DmpKC;'+-tl;rn-okgdkq5%Y%pBmp@r,g2`PR-N9&<D^63_RR(L<*b*bP5<-s>PL'8k8k91/D?[-7(&(m@?q7kdH)cDfYN)@9TDSe;DG)uQh&#k+'p74N)^o"
	"hl=,'';[P9_kisBjgU,&g>Ok2=4'K%cl@Nii)3q-_.1U9,.QL/2&>uuF*^TVA7Bs&;W36AZ(j'muJG4M_<bc%_Q?6']Td`*<g-[-%u,N<Tcm(F,rGF>CpO4)0kNeFKG2V?'jkgNvkK<&"
	"MQU+<[xKVaY>/T@&Jp'HtA$a&5U&R8bs:RWYiYeQu4k(NgxE$%X6V&#X+3O-u_dQ8/_-ldRf1W-2dpGe*E^r7d>S^JisoC%s`^68r*d;))C[p7W?<McU=n%P4'Ho$8VG29mZvQ:H1[^&"
	"foZaE#jbxu$lZp7%2s&-9rJfL4s*C8mlB#$P:=QSF-*j$[A'aNtobb.Z'[0#kQ*n%:i4%(JN#,2#9bp7q>[6OfId2P&;Hr7cpB#$X8-l93rg996Nb4):v0<-Y7`[eEdoW*l/xNN9<&v,"
	"%nra*-?078.F8o8aP+Au]ZX2L:1Bn*fuW1;N&&3M5U#x'<w:u?\?w]RjZCNv'[c)BFnoDbPf][`*(pBQ8mcN)YW/b,'^Y(<-QIIZ*eRrP8r=24D<.#L%vXMnNG_`78f:HO*m$N$-PMR8`"
	"[9jb8tBuTW.WqG`++ho%pI<#6EZA`X;)&G;EY+Aus#XjBZXG&#j[*RBY<-El+AI':)Z.=`]4i78Z]]2`=R$m8,@^2`Qs849_LfH[(X+`)X0-2(a+@>-KA$ZAf0YS&_;AL;>g6pgV==5A"
	"6R.db1Wbs'MC9-)5u:ENe7-##H:O&=$^]712c&gL,%,cM+4(5SmwUF<qhvhO+X[b+rh(hL5:Tf$^gs]OK?Qd&kJ1>-x-*j0c-G(-9Y`N*$0_k2ece`*#JdQ8Fk=9W'&%kCjYeaHC@nL-"
	"$[d;B*<^#DuHQNYF#>Q8L*fW]8cJX/,CWR83+pVooXXk'1HCL:6I%T(`2VY(An&R8?uN]'WLJM'$/*JLXm@8JhS](6l0tv'x06oDXFC'%=7CP8fCKetbqp0%;=0iC/kRkrjK5x'qtD_O"
	"r/9x:VWF&#<r)<-rHR.'>LW`<04=q0E8/c&hItt'a8J0)kY#Q8nnV78o=6UT`-1t%-FuN:xA1J[Oq`p77%72L9$2<?vo]Q(S/,`%VaGY>-'[qp)mH^*[eL]u/o(HF0iu.-vwoW_wn=O:"
	"[3NPcDh)Zn)ex[T%LaP-VC%f$36t1CvSw,X>^>Z4q=Z58]evqT5.xP33h839>So>InZb%=w=>F%$Mdm_FjSEEwGJMN3B,-(b@mHM;uFr$r&V@-Vp;m$bF6XJbM0-'-PgQJ=Z.lBE?=x>"
	"YBPX9N3?&+0)xm'wQsH$K]MP9cmv9glREr(>=n-k;/6t$r]2@-Ips&d-8oS@pb5r@lwcQ:aum))u=KkrVv[n>Lu,@RvlOE.^Puk;v4[+9.2A2LrPn'&]/?pg&.Rq$9-vc6BUpD*8[?:B"
	"mMq*9.HFt_QSl##O->>#b7278#r%34A$;M%+=hlTsVPp'X8N&Zu/To%mDh:.,umo%5VIl90wn5F9;_OFJ?=?JbjcX($^)Rj2vao7W9Udkr[F%8:@(4F@5W5_oHOG%M4Y@G:P+JGUsRA%"
	"UeO-;Tr+OOHi8i:F$aC=K@82L(__3:>H-g)S65e;B@:xnT_x0+x,2N:rmL4)VtH#)NF7WAs,Zx'uQpE<NJEaGq^'%'j%gpB;Je(-/`%=-8`&.6X/4S-FK=f'F>U78_TX=?1s?cZYlBd'"
	"<IaN9E=Ws^iqV_,Yei68%U@9KA-Rb'2WK78hIZ;%DkE2LDfvd(M%Jn&KSC<-mSZ[$ca<@9#`'^#nx(X-BLpU@YmB#$0Q?d8/4hFco+Eu$fY%F<]%*?@FBA,;vV@-Fo:Cu047V2B18,'$"
	"Rqmr*$J4gU<7(p(Y5:wPn;v&'C(^('$9#v/1<#e+K2ta*SV0<ISF0'HPQB%oF'7F'IZ'N9$/+8Vf[VC2)&4V&7rpgL<=XD+`2aO;_((e*FKK=-J.fQ-]HGM.IhF(=2tJQ(C9ES.qL)*N"
	"pYd.:b[+Au-g([I%QL@-cVfJ8D>BugDAVB-vlc_fV5gc*s&Y9.;25##F7,W.P'OC&aTZ`*65m_&WRJM'vGl_&==(S*2)7`&27@U1G^4?-:_`=-+()t-c'ChLGF%q.0l:$#:T__&Pi68%"
	"0xi_&Zh+/(77j_&JWoF.V735&S)[R*:xFR*K5>>#`bW-?4Ne_&6Ne_&6Ne_&lM4;-xCJcM6X;uM6X;uM(.a..^2TkL%oR(#;u.T%eAr%4tJ8&><1=GHZ_+m9/#H1F^R#SC#*N=BA9(D?"
	"v[UiFk-c/>tBc/>`9IL2a)Ph#WL](#O:Jr1Btu+#TH4.#a5C/#vS3rL<1^NMowY.##t6qLw`5oL_R#.#2HwqLUwXrLp/w+#ALx>-1xRu-'*IqL@KCsLB@]qL]cYs-dpao7Om#K)l?1?%"
	";LuDNH@H>#/X-TI(;P>#,Gc>#0Su>#4`1?#8lC?#<xU?#@.i?#D:%@#HF7@#LRI@#P_[@#Tkn@#Xw*A#]-=A#a9OA#d<F&#*;G##.GY##2Sl##6`($#:l:$#>xL$#B.`$#F:r$#JF.%#"
	"NR@%#R_R%#Vke%#Zww%#_-4&#1TR-&Mglr-k'MS.o?.5/sWel/wpEM0%3'/1)K^f1-d>G21&v(35>V`39V7A4=onx4A1OY5EI0;6Ibgr6M$HS7Q<)58UT`l8Ym@M9^/x.:bGXf:f`9G;"
	"jxp(<n:Q`<rR2A=vkix=$.JY>(F+;?,_br?0wBS@49$5A8QZlAQ#]V-kw:8.o9ro.sQRP/wj320%-ki0)EKJ1-^,,21vcc258DD39P%&4=i[]4A+=>5ECtu5I[TV6Mt587Q6mo7tB'DW"
	"-fJcMxUq4S=Gj(N=eC]OkKu=Yc/;ip3#T(j:6s7R`?U+rH#5PSpL7]bIFtIqmW:YYdQqFrhod(WEH1VdDMSrZ>vViBn_t.CTp;JCbMMrdku.Sek+f4ft(XfCsOFlfOuo7[&+T.q6j<fh"
	"#+$JhxUwOoErf%OLoOcDQ@h%FSL-AF3HJ]FZndxF_6auGcH&;Hggx7I1$BSIm/YoIrVq1KXpa._D1SiKx%n.L<U=lox/Ff_)(:oDkarTCu:.T2B-5CPgW=CPh^FCPidOCPjjXCPkpbCP"
	"lvkCPm&uCPn,(DP@t>HPA$HHPB*QHPC0ZHPD6dHPD3Q-P_aQL2<j9xpG';xpG';xpG';xpG';xpG';xpG';xpG';xpG';xpG';xpG';xpG';xpG';xpG';xpCUi'%jseUCF3K29]cP.P"
	"K)uCPK)uCPK)uCPK)uCPK)uCPK)uCPK)uCPK)uCPK)uCPK)uCPK)uCPK)uCPK)uCPK)uCPT$au7ggUA5o,^<-O<eT-O<eT-O<eT-O<eT-O<eT-O<eT-O<eT-O<eT-O<eT-O<eT-O<eT-"
	"O<eT-O<eT-RWaQ.nW&##]9Pwf+($##";

static const char* GetProggyTinyCompressedFontDataTTFBase85()
{
	return proggy_tiny_ttf_compressed_data_base85;
}
