#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct IDirect3DDevice9;
struct GFX;
struct GFXTarget;
struct Array;
struct Character;
struct Panel;
struct State;
struct DPSData;
struct DPSTargetEx;
struct ClosePlayer;

IDirect3DDevice9* ImGui_GetDevice();
bool ImGui_GetViewportRes(uint32_t* pw, uint32_t *ph);
bool ImGui_GetFontSize(uint32_t* px, uint32_t *py);

bool ImGui_Init(void* hwnd, IDirect3DDevice9* device, void *viewport);
void ImGui_Shutdown();
void ImGui_ImplDX9_ResetInit();
bool ImGui_ImplDX9_ResetPost();
bool ImGui_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void ImGui_NewFrame();
void ImGui_Render(float ms, int64_t now);

void ImGui_FloatBarConfigLoad();
void ImGui_FloatBarConfigSave();

void ImGui_StyleSetDefaults();
void ImGui_StyleConfigLoad();
void ImGui_StyleConfigSave();

void ImGui_FloatBars(struct Panel* panel,
	struct Character *player, struct Character *target,
	int center, struct GFX* ui, struct GFXTarget* set);

void ImGui_HpAndDistance(struct Panel* panel,
	struct Character *player, struct Character *target,
	int center, struct GFX* ui, struct GFXTarget* set);

void ImGui_Compass(struct Panel* panel, int x, int y, int w);

void ImGui_RemoveGraphData(int32_t id);
void ImGui_ResetGraphData(int32_t exclude_id);
void ImGui_PersonalDps(struct Panel *panel, int x, int y,
	const struct DPSData* dps_data,
	const struct DPSTargetEx* dps_target);

void ImGui_SkillBreakdown(struct Panel *panel, int x, int y,
	const struct DPSTargetEx* dps_target);

void ImGui_GroupDps(struct Panel *panel, int x, int y,
	const struct DPSTargetEx* target,
	const struct DPSPlayer* players, uint32_t num);

void ImGui_BuffUptime(struct Panel *panel, int x, int y,
	struct ClosePlayer *players, uint32_t num);


void ImGui_CharInspect(struct Panel* panel, struct Character *c);

void ImGui_Debug(struct Panel* panel,
	struct Character *player, struct Character *target,
	struct Array* char_array, struct Array* play_array);


#ifdef __cplusplus
}
#endif