#pragma once
#include "core/types.h"
#include <wchar.h>
#include <Windows.h>

#define CONFIG_SECTION_DBG			"Debug"
#define CONFIG_SECTION_FONT			"Font"
#define CONFIG_SECTION_STYLE		"Style"
#define CONFIG_SECTION_BIND			"KeyBinds"
#define CONFIG_SECTION_GLOBAL		"Global"
#define CONFIG_SECTION_SRV			"Server"
#define CONFIG_SECTION_LOG			"Logging"
#define CONFIG_SECTION_LANG			"Language"
#define CONFIG_SECTION_OPTIONS		"BGDM Options"
#define CONFIG_SECTION_HP			"HPPanel"
#define CONFIG_SECTION_CONMPASS		"CompassPanel"
#define CONFIG_SECTION_FLOAT		"Float Bars"
#define CONFIG_SECTION_DPS_TARGET	"Target Stats"
#define CONFIG_SECTION_DPS_GROUP	"Group Stats"
#define CONFIG_SECTION_BUFF_UPTIME	"Buff Uptime"
#define CONFIG_SECTION_SKILLS		"Skill Breakdown"
#define CONFIG_SECTION_GEAR_SELF	"Character Inspect (Self)"
#define CONFIG_SECTION_GEAR_TARGET	"Character Inspect (Target)"

// Keybinding.
typedef struct Bind
{
	u32 vk;
	u32 ctrl;
	u32 shift;
	u32 alt;
	bool toggle;
	i8 str[64];
} Bind;

#ifdef __cplusplus
extern "C" {
#endif

// Check a keybind state
bool is_key_down(u32 vk);
bool is_bind_down(Bind const* b);

// Reads a keybind.
bool config_toggle_bind(Bind* bind, bool *toggle);
Bind config_get_bind(i8 const* sec, i8 const* key, i8 const* val);
Bind config_get_bindW(const wchar_t* sec, const wchar_t* key, const wchar_t* val);

// Reads an integer value.
i32 config_get_int(i8 const* sec, i8 const* key, i32 val);
f32 config_get_float(i8 const* sec, i8 const* key, f32 val);
bool config_set_int(i8 const* sec, i8 const* key, i32 val);
bool config_set_float(i8 const* sec, i8 const* key, f32 val);

// Reads a string value.
i8* config_get_str(i8 const* sec, i8 const* key, i8 const* val);
wchar_t* config_get_strW(const wchar_t* sec, const wchar_t* key, const wchar_t* val);
bool config_set_str(i8 const* sec, i8 const* key, i8 const *val);
bool config_set_strW(const wchar_t* sec, const wchar_t* key, const wchar_t* val);

const wchar_t* config_get_my_documents();
const wchar_t* config_get_ini_file();

const i8* config_get_my_documentsA();
const i8* config_get_ini_fileA();

// Panels column names
extern const char* gdps_col_str[];
extern const char* buff_col_str[];

bool GetEmbeddedResource(int ResID, LPVOID *ppData, LPDWORD pBytes);
void config_get_state(void);
void config_test_keybinds(void);

#ifdef __cplusplus
}
#endif
