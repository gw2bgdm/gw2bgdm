#pragma once
#include "core/types.h"
#include "meter/enums.h"
#include "meter/offsets.h"
#pragma warning (push)
#pragma warning (disable: 4201)
#include "dxsdk/d3dx9math.h"
#pragma warning (pop)

// Agent types.
#define AGENT_TYPE_CHAR 0
#define AGENT_TYPE_GADGET 10
#define AGENT_TYPE_ATTACK 11
#define AGENT_TYPE_ITEM 15

// Breakbar states.
#define BREAKBAR_NONE -1
#define BREAKBAR_READY 0
#define BREAKBAR_REGEN 1
#define BREAKBAR_BLOCK 2

// Char attitudes.
#define CHAR_ATTITUDE_FRIENDLY 0
#define CHAR_ATTITUDE_HOSTILE 1
#define CHAR_ATTITUDE_INDIFFERENT 2
#define CHAR_ATTITUDE_NEUTRAL 3

// Char statuses.
#define CHAR_STATUS_ALIVE 0
#define CHAR_STATUS_DEAD 1
#define CHAR_STATUS_DOWN 2

// Char Inventory slots.
#define CHAR_INVENTORY_INVALID_SLOT -1	

// Map types.
#define MAP_TYPE_REDIRECT 0
#define MAP_TYPE_CREATE 1
#define MAP_TYPE_PVP 2
#define MAP_TYPE_GVG 3
#define MAP_TYPE_INSTANCE 4
#define MAP_TYPE_PUBLIC 5
#define MAP_TYPE_TOURNAMENT 6
#define MAP_TYPE_TUTORIAL 7
#define MAP_TYPE_WVW_EB 8
#define MAP_TYPE_WVW_BB 9
#define MAP_TYPE_WVW_GB 10
#define MAP_TYPE_WVW_RB 11
#define MAP_TYPE_WVW_FV 12
#define MAP_TYPE_WVW_OS 13
#define MAP_TYPE_WVW_EOTM 14
#define MAP_TYPE_WVW_EOTM2 15

#define MAP_ID_GILDED_HOLLOW 1121
#define MAP_ID_LOST_PRECIPICE 1124

// Squad information
#define SQUAD_SIZE_MAX 50
#define SQUAD_MAX_SUBGROUPS 15


// A resizable array.
typedef struct Array
{
	u64 data;
	u32 max;
	u32 cur;
} Array;

// Buffs have a different type of array
typedef struct Dict
{
	u32 max;
	u32 cur;
	u64 data;
} Dict;

// Buff bar dictionary entry
typedef struct BuffEntry {
	size_t buffId;
	uintptr_t *pBuff;
	size_t hash;
} BuffEntry;

// Buff stacks information
typedef struct BuffStacks {
	u16 vigor;
	u16 swift;
	u16 stab;
	u16 retal;
	u16 resist;
	u16 regen;
	u16 quick;
	u16 alacrity;
	u16 prot;
	u16 fury;
	u16 aegis;
	u16 might;
	u16 necVamp;	// Necro Vampiric Aura
	u16 warEA;		// Warrior Empower Allies
	u16 warTact;	// Warrior Banner of Tactics
	u16 warStr;		// Warrior Banner of Strength
	u16 warDisc;	// Warrior Banner of Discipline
	u16 revNR;		// Revenant Naturalistic Resonance
	u16 revAP;		// Revenant Assasin's Presence
	u16 revRite;	// Revenant Rite of great Dwarf
	u16 sun;		// Ranger Sun Spirit
	u16 frost;		// Ranger Frost Spirit
	u16 storm;		// Ranger Storm Spirit
	u16 stone;		// Ranger Stone Spirit
	u16 spotter;	// Ranger Spotter
	u16 GOTL;		// Druid Grace of the Land
	u16 empGlyph;	// Druid Glyph of Empowerment
	u16 sooMist;	// Elementalist soothing mist
	u16 strInNum;	// Guard Strength in numbers
	u16 engPPD;		// Engineer Pinpoint Distribution
	u16 port_weave;
} BuffStacks;

// Character stat structure.
typedef struct CharStats
{
	i32 pow;
	i32 pre;
	i32 tuf;
	i32 vit;
	i32 fer;
	i32 hlp;
	i32 cnd;
	i32 con;
	i32 exp;
} CharStats;

// Infusions
typedef struct __ItemDef
{
	u64 ptr;
	u32 id;
	i32 rarity;
	u32 level;
	const wchar_t *name;
} ItemDef;

// Maximum number of infusions
#define MAX_UPGRADES 10

// EquippedItems
typedef struct __EquipItem
{
	const wchar_t *name;
	u64 ptr;
	i32 type;
	bool is_wep;
	i32 wep_type;
	ItemDef stat_def;
	ItemDef item_def;
	ItemDef skin_def;
	ItemDef upgrade1;
	ItemDef upgrade2;
	ItemDef infus_arr[MAX_UPGRADES];
	i32 infus_len;
} EquipItem;

typedef struct __EquipItems
{
	EquipItem head_aqua;
	EquipItem back;
	EquipItem chest;
	EquipItem boots;
	EquipItem gloves;
	EquipItem head;
	EquipItem leggings;
	EquipItem shoulder;
	EquipItem acc_ear1;
	EquipItem acc_ear2;
	EquipItem acc_ring1;
	EquipItem acc_ring2;
	EquipItem acc_amulet;
	EquipItem wep1_aqua;
	EquipItem wep2_aqua;
	EquipItem wep1_main;
	EquipItem wep1_off;
	EquipItem wep2_main;
	EquipItem wep2_off;
	EquipItem tool_foraging;
	EquipItem tool_logging;
	EquipItem tool_mining;
} EquipItems;

typedef struct __TraitDef
{
	u64 ptr;
	u32 id;
	u32 hash;
	const wchar_t *name;
} TraitDef;

typedef struct __Spec
{
#ifdef __cplusplus
	TraitDef specs[GW2::SPEC_SLOT_END];
	TraitDef traits[GW2::SPEC_SLOT_END][GW2::TRAIT_SLOT_END];
#else
	TraitDef specs[SPEC_SLOT_END];
	TraitDef traits[SPEC_SLOT_END][TRAIT_SLOT_END];
#endif
} Spec;

// Mumble linked memory.
typedef struct LinkedMem
{
	u32	ui_version;
	u32	ui_tick;
	vec3 avatar_pos;
	vec3 avatar_front;
	vec3 avatar_top;
	u16	name[256];
	vec3 cam_pos;
	vec3 cam_front;
	vec3 cam_top;
	u16	identity[256];
	u32	context_len;
	u8 server[28];
	u32 map_id;
	u32 map_type;
	u32 shard_id;
	u32 instance;
	u32 build_id;
	u8 context[208];
	u16 description[2048];
} LinkedMem;


// Camera data
typedef struct _CamData
{
	bool valid;
	i64 lastUpdate;
	D3DXVECTOR3 camPos;
	D3DXVECTOR3 upVec;
	D3DXVECTOR3 lookAt;
	D3DXVECTOR3 viewVec;
	float fovy;
	float curZoom;
	float minZoom;
	float maxZoom;
} CamData;


// Mumble data
#ifdef __cplusplus
extern "C" LinkedMem* g_mum_mem;
#else
extern LinkedMem* g_mum_mem;
#endif