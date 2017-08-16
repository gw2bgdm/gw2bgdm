#pragma once
#include <windows.h>
#include "core/types.h"
#include "core/message.h"
#include "meter/game.h"
#include "meter/config.h"

// Update frequency for closest entites, in microseconds.
#define CLOSEST_UPDATE_FREQ 1000000

// DPS interval for current timing, in microseconds.
#define DPS_INTERVAL_1 10000000
#define DPS_INTERVAL_2 30000000

// Maximum inactive age in microseconds before a target is dropped from the array.
#define MAX_TARGET_AGE 180000000

// Maximum number of recorded DPS hits.
#define MAX_HITS 65536

// Maximum number of players for the player array.
#define MAX_PLAYERS 16384

// Maximum number of DPS targets.
#define MAX_TARGETS 32

// The time before a mob resets when at full HP.
#define RESET_TIMER 10000000

// Max sortable columns in a grid panel
#define MAX_COL 64

// Max no. of tab in a panel
#define MAX_TABS 6

// Max no. of panels
#define MAX_PANELS 20

enum VersionRows {
	VER_ROW_VER,
	VER_ROW_SIG,
	VER_ROW_PING,
	VER_ROW_FPS,
	VER_ROW_PROC,
	VER_ROW_SRV_TEXT,
	VER_ROW_SRV_TIME,
	VER_ROW_END
};

enum HPRows {
	HP_ROW_PLAYER_HP,
	HP_ROW_TARGET_HP,
	HP_ROW_TARGET_DIST,
	HP_ROW_TARGET_BB,
	HP_ROW_PORT_DIST,
	HP_ROW_PORT_DIST_BG,
	HP_ROW_END
};

enum TargetDpsColumns {
	TDPS_COL_DPS,
	TDPS_COL_DMG,
	TDPS_COL_TTK,
	TDPS_COL_GRAPH,
	TDPS_COL_SPECID,
	TDPS_COL_END
};

enum GroupDpsColumns {
	GDPS_COL_NAME,
	GDPS_COL_CLS,
	GDPS_COL_DPS,
	GDPS_COL_PER,
	GDPS_COL_DMGOUT,
	GDPS_COL_DMGIN,
	GDPS_COL_HPS,
	GDPS_COL_HEAL,
	GDPS_COL_TIME,
	GDPS_COL_END,
	GDPS_COL_GRAPH,
	GDPS_COL_END2
};

enum BuffUptimeColumns {
	BUFF_COL_NAME,
	BUFF_COL_CLS,
	BUFF_COL_SUB,
	BUFF_COL_DOWN,
	BUFF_COL_SCLR,
	BUFF_COL_SEAW,
	BUFF_COL_PROT,
	BUFF_COL_QUIK,
	BUFF_COL_ALAC,
	BUFF_COL_FURY,
	BUFF_COL_MIGHT,
	BUFF_COL_VIGOR,
	BUFF_COL_SWIFT,
	BUFF_COL_STAB,
	BUFF_COL_RETAL,
	BUFF_COL_RESIST,
	BUFF_COL_REGEN,
	BUFF_COL_AEGIS,
	BUFF_COL_GOTL,
	BUFF_COL_GLEM,
	BUFF_COL_BANS,
	BUFF_COL_BAND,
	BUFF_COL_BANT,
	BUFF_COL_EA,
	BUFF_COL_SPOTTER,
	BUFF_COL_FROST,
	BUFF_COL_SUN,
	BUFF_COL_STORM,
	BUFF_COL_STONE,
	BUFF_COL_ENGPP,
	BUFF_COL_REVNR,
	BUFF_COL_REVAP,
	BUFF_COL_REVRD,
	BUFF_COL_ELESM,
	BUFF_COL_GRDSN,
	BUFF_COL_NECVA,
	BUFF_COL_TIME,
	BUFF_COL_END
};

// Character data.
typedef struct Character
{
	u64 aptr;		// Agent
	u64 bptr;		// Breakbar
	u64 cptr;		// Char
	u64 tptr;		// Transform
	u64 pptr;		// Player
	//u64 spdef;	// speciesDef
	u64 cbptr;		// combatantDef
	u64 wmptr;		// wmAgent
	u64 ctptr;		// contactDef
	u32 shard;		// Instance ID
	u32 id;			// AgentID
	u32 type;		
	u32 attitude;
	u32 profession;
	u32 stance;
	i8 race;
	i8 gender;
	bool is_player;
	bool in_combat;
	u64 inventory;
	i32 hp_max;
	i32 hp_val;
	i32 bb_state;
	u32 bb_value;
	vec3 tpos;
	vec4 apos;
	ClientStats stats;
	wchar_t name[MSG_NAME_SIZE];
	const wchar_t *decodedName;		// resolved local text engine ptr
} Character;

// PortalData
typedef struct PortalData
{
	bool is_weave;
	u32 map_id;
	u32 shard_id;
	vec3 pos_trans;
	vec4 pos_agent;
	vec3 pos_mum;
	i64 time;
} PortalData;

// CombatData
typedef struct CombatData
{
	volatile long lastKnownCombatState;
	volatile long damageOut;
	volatile long damageIn;
	volatile long healOut;
	bool lastKnownDownState;
	u32 noDowned;
	i64 begin;
	i64 end;
	i64 update;
	f32 duration;
	u32 damage_out;

	// buff uptime
	f32 vigor_dura;
	f32 swift_dura;
	f32 stab_dura;
	f32 retal_dura;
	f32 resist_dura;
	f32 regen_dura;
	f32 aegis_dura;
	f32 warEA_dura;
	f32 revNR_dura;
	f32 revAP_dura;
	f32 necVA_dura;
	f32 sun_dura;
	f32 frost_dura;
	f32 storm_dura;
	f32 stone_dura;
	f32 spot_dura;
	f32 engPPD_dura;
	f32 sclr_dura;
	f32 prot_dura;
	f32 quik_dura;
	f32 alac_dura;
	f32 fury_dura;
	f32 might_dura;
	f32 might_avg;
	f32 gotl_dura;
	f32 gotl_avg;
	f32 glem_dura;
	f32 bans_dura;
	f32 band_dura;
	f32 bant_dura;
	f32 seaw_dura; // Seaweed salad food buff 
	f32 revRD_dura;
	f32 eleSM_dura;
	f32 grdSIN_dura;

	// Last combat frame position
	// for calculating seaweed buff
	// since we calculate it every 500ms
	// we need a separate duration & update vars
	f32 duration500;
	i64 update_pos;
	vec3 pos;

	// HACK needs it's own var
	// just putting it here cuz lazy
	// mesmer port data
	PortalData pd;

} CombatData;

typedef struct ClosePlayer
{
	wchar_t *name;
	Character c;
	CombatData cd;
	u32 subGroup;
} ClosePlayer;

// DPS hit.
typedef struct DPSHit
{
	i64 time;
	u32 dmg;
	u32 eff_id;
	u32 eff_hash;
} DPSHit;

// DPS target.
typedef struct DPSTarget
{
	u64 aptr;
	u64 cptr;
	u64 wmptr;
	u32 npc_id;
	u32 species_id;
	bool invuln;
	bool isDead;
	i64 duration;
	i64 last_update;
	i64 time_update;
	i64 time_begin;
	i64 time_hit;
	i32 hp_max;
	i32 hp_lost;
	i32 hp_last;
	u32 shard;
	u32 id;
	u32 tdmg;
	u32 c1dmg;
	u32 c2dmg;
	u32 c1dmg_team;
	u32 c2dmg_team;
	u32 num_hits;
	u32 num_hits_team;
	DPSHit hits[MAX_HITS];
	DPSHit hits_team[MAX_HITS];
} DPSTarget;

typedef struct DPSTargetEx
{
	bool invalid;
	bool locked;
	i64 time_sel;
	Character c;
	DPSTarget t;
} DPSTargetEx;

typedef struct DPSData
{
	f32 tot_dur;
	f32 l1_dur;
	f32 l2_dur;
	i32 hp_lost;
	i32 tot_done;
	i32 l1_done;
	i32 l2_done;
	i32 l1_done_team;
	i32 l2_done_team;
} DPSData;

// DPS player data.
typedef struct DPSPlayer
{
	Character *c;
	CombatData *cd;
	bool in_combat;
	f32 target_time;
	u32 target_dmg;
	f32 time;
	u32 damage;
	u32 damage_in;
	u32 heal_out;
	u32 profession;
	ClientStats stats;
	ClientTarget targets[MSG_TARGETS];
	wchar_t name[MSG_NAME_SIZE];
} DPSPlayer;

typedef struct
{
	Character c;
	CombatData combat;
	wchar_t acctName[MSG_NAME_SIZE*2];
} Player;


typedef struct {
	i8 *str;
	bool col_visible[MAX_COL];
	const char** col_str;
	i32 maxCol;
	i32 sortCol;
	i32 sortColLast;
	i32 asc;
	u32 tabNo;
	bool canLocalizedText;
	bool useLocalizedText;
	bool useProfColoring;
	bool lineNumbering;
} PanelConfig;

typedef struct Panel {
	i8 section[64];
	Bind bind;
	u32	enabled;
	POINT pos;
	RECT rect;
	bool init;
	bool tinyFont;
	bool autoResize;
	int mode;
	int tmp_fl;
	int tmp_fr;
	float fAlpha;
	PanelConfig cfg;
} Panel;

typedef struct State
{
	// Keybinds
	Bind bind_screenshot;
	Bind bind_on_off;
	Bind bind_input;
	Bind bind_minPanels;
	Bind bind_dps_lock;
	Bind bind_dps_reset;
	Bind bind_debug;

	// User font
	i8* default_font_path;
	u32 default_font_size;
	i8* tiny_font_path;
	u32 tiny_font_size;
	i8* imgui_ini_filename;

	// Panels
	Panel panel_hp;
	Panel panel_float;
	Panel panel_compass;
	Panel panel_dps_self;
	Panel panel_dps_group;
	Panel panel_buff_uptime;
	Panel panel_gear_self;
	Panel panel_gear_target;
	Panel panel_skills;
	Panel panel_debug;
	Panel panel_version;
	Panel *panel_arr[MAX_PANELS];

	// config
	bool is_gw2china;
	bool global_on_off;
	bool global_cap_input;
	bool show_metrics;
	bool show_metrics_bgdm;
	bool show_server;
	bool autoUpdate_disable;
	bool netDps_disable;
	bool hpCommas_disable;
	bool minPanels_enable;
	bool priortizeSquad;
	bool hideNonSquad;
	bool hideNonParty;
	bool profColor_disable;
	bool use_localized_text;
	bool use_seaweed_icon;
	bool use_downed_enemy_icon;
	i32 icon_pack_no;
	i32 target_retention_time;
	i32 ooc_grace_period;
	i8 log_dir[1024];
	i8* log_filemode;
	i32 log_minlevel;
	wchar_t lang_dir[1024];
	wchar_t lang_file[1024];

	// server
	i8* network_addr;
	u16 network_port;

	// debug version override
	i32 dbg_ver;

	// pre-update CN version
	// used to override offsets before the Chinese get
	// their patch which is usually same week Friday as NA/EU
	u32 cn_build_id_override;

} State;

typedef struct __GamePtrs
{
	uintptr_t pShardId;
	uintptr_t pMapId;
	uintptr_t pMapType;
	uintptr_t pPing;
	uintptr_t pFps;
	uintptr_t pIfHide;
	uintptr_t pMapOpen;
	uintptr_t pActionCam;
	uintptr_t pTlsCtx;
	uintptr_t pCharCliCtx;
	uintptr_t pAgentViewCtx;
	uintptr_t pWorldView;
	uintptr_t pAgentSelectionCtx;
	uintptr_t pUiCtx;
	uintptr_t pCam;
	uintptr_t pMumble;
	uintptr_t pfcGetWmAgent;
	uintptr_t pfcGetContactCtx;
	uintptr_t pfcGetSquadCtx;
	uintptr_t pfcCodedTextFromHashId;
	uintptr_t pfcDecodeCodedText;
	uintptr_t pWndProc;
	uintptr_t pGameThread;
	uintptr_t cbCombatLog;
	uintptr_t cbDmgLogResult;
	uintptr_t squadContext;
	uintptr_t contactContext;
} GamePtrs;

#ifdef __cplusplus
extern "C" {
#endif

// Our module handle
extern HANDLE g_hInstance;

// game window, acquired on load
extern HWND g_hWnd;

// global mod state
extern State g_state;

// game pointers
extern GamePtrs g_ptrs;

// Mumble data
extern LinkedMem* g_mum_mem;

// build ID (from pattern scan)
extern u32 g_build_id;

// Controlled player
extern Player g_player;

// Array forward-def
struct Array;
struct Dict;
struct BuffStacks;

// Mumble Link
void mumble_link_create(void);
void mumble_link_destroy(void);
u32 buildId(void);

// Read the .ini configuration options
void config_get_state(void);

// Creates the DPS meter.
void dps_create(void);

// Destroys the DPS meter.
void dps_destroy(void);

// Updates the DPS meter.
void dps_update(i64 now);

// Update the camera data
bool dps_update_cam_data();
CamData* dps_get_cam_data();

// Reset the DPS targets
void dps_targets_reset();

// Updte targets missing speciesDef
void dps_targets_get_species();

// Get a DPS target
DPSTarget* dps_targets_get(u32 i);

// Insert a DPS target
u32 dps_targets_insert(u64 target, u64 cptr, struct Array* ca, i64 now);

// Get last known target
struct DPSTargetEx* dps_target_get_lastknown(i64 now);

// Get the cam direction angle
float get_cam_direction(vec3 cam_dir);

// Read the charater combat state & data
void read_agent(Character* c, u64 aptr, u64 cptr, struct Array *pa, i64 now);
void read_combat_data(Character* c, CombatData *cd, i64 now);
void read_target(Character* t, struct Array* ca, struct Array *pa, u64 aptr, i64 now);
void read_buff_array(uintptr_t cbtBuffBar, struct BuffStacks *stacks);

// Cam Data & WorldToScreen projections
float get_mum_fovy();
bool project2d_agent(POINT* outPoint, vec4* worldPt, u64 aptr, i64 now);
bool project2d_mum(POINT* outPoint, const vec3 *pos, u64 aptr, bool invertYZ);

// Close players array
u32 read_closest_x_players(Character* player, ClosePlayer closest_arr[], u32 closest_num, i64 now, int mode);

// Handles damage messages from the server.
void dps_handle_msg_damage_utf8(MsgServerDamageUTF8* msg);

#ifdef __cplusplus
}
#endif
