#include "core/types.h"
#include "core/debug.h"
#include "core/logging.h"
#include "core/helpers.h"
#include "core/time.h"
#include "core/file.h"
#include "meter/dps.h"
#include "meter/config.h"
#include "meter/game.h"
#include "meter/gfx.h"
#include "meter/utf.h"
#include "meter/network.h"
#include "meter/process.h"
#include "meter/hooks.h"
#include "meter/updater.h"
#include "meter/autolock.h"
#include "meter/ForeignFncs.h"
#include "meter/lru_cache.h"
#include "meter/localdb.h"
#include "meter/imgui_bgdm.h"
#include "meter/resource.h"
#include "hook/hook.h"
#include <Windows.h>
#include <Strsafe.h>
#pragma warning (push)
#pragma warning (disable: 4201)
#include <winternl.h>
#pragma warning (pop)
#include <TlHelp32.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <malloc.h>


// mod state
State g_state = { 0 };
GamePtrs g_ptrs = { 0 };

// game window, acquired on load
HWND g_hWnd;

// Hooked WindowsProc
// NOT USED ANYMORE
//static LONG_PTR s_oldWindProc = 0;

// Mumble link.
u32 g_build_id = 0;
LinkedMem* g_mum_mem;
static HANDLE g_mum_link;
static CamData g_cam_data;

// current player
Player g_player = { 0 };

// DPS logging.
DPSTarget* g_targets;

// Closest players.
static u64* g_closest_pap;
static u32 g_closest_num;
static wchar_t g_closest_names[MSG_CLOSEST_PLAYERS][MSG_NAME_SIZE];
static ClosePlayer g_closest_players[MSG_CLOSEST_PLAYERS];

// Group DPS.
static u32 g_group_id;
static u32 g_group_num;
static DPSPlayer g_group[MSG_CLOSEST_PLAYERS];

// Timing state.
static bool g_is_init;

// Returns the difference between two hp values, allowing for wrap around from 0 to max.
static i32 hp_diff(i32 hp_new, i32 hp_old, i32 max)
{
	i32 diff = hp_old - hp_new;
	if (diff < 0 && diff < -max / 5)
	{
		diff = hp_old + (max - hp_new);
	}

	return diff;
}

// Reads the name from a given player pointer into the given buffer.
static bool read_nameW(wchar_t* dst, u64 pptr, u64 offset)
{
	u64 nptr = process_read_u64(pptr + offset);
	if (nptr == 0)
	{
		dst[0] = 0;
		return false;
	}

	u32 size = MSG_NAME_SIZE * sizeof(wchar_t);
	memset(dst, 0, size);
	process_read(nptr, dst, size);
	dst[MSG_NAME_LAST] = 0;

	return true;
}

void read_buff_array(uintptr_t cbtBuffBar, struct BuffStacks *stacks)
{
	if (cbtBuffBar == 0) return;

	Dict buff_array = { 0 };
	struct Dict *buff_arr = &buff_array;
	process_read(cbtBuffBar + OFF_BUFFBAR_BUFF_ARR, &buff_array, sizeof(buff_array));

	if (!buff_arr || !stacks)
		return;

	assert(buff_arr->max < 1000);
	if (buff_arr->max >= 4000)
		return;

	for (u32 i = 0; i < buff_arr->max; ++i)
	{
		BuffEntry be;
		process_read(buff_arr->data + i * sizeof(BuffEntry), &be, sizeof(BuffEntry));
		if (be.pBuff)
		{
			i32 efType = process_read_i32((u64)be.pBuff + OFF_BUFF_EF_TYPE);
			switch (efType)
			{
			case(EFFECT_VIGOR):
				++stacks->vigor;
				break;
			case(EFFECT_SWIFTNESS):
				++stacks->swift;
				break;
			case(EFFECT_STABILITY):
				++stacks->stab;
				break;
			case(EFFECT_RETALIATION):
				++stacks->retal;
				break;
			case(EFFECT_RESISTANCE):
				++stacks->resist;
				break;
			case(EFFECT_REGENERATION):
				++stacks->regen;
				break;
			case(EFFECT_QUICKNESS):
				++stacks->quick;
				break;
			case(EFFECT_ALACRITY):
				++stacks->alacrity;
				break;
			case(EFFECT_PROTECTION):
				++stacks->prot;
				break;
			case(EFFECT_FURY):
				++stacks->fury;
				break;
			case(EFFECT_AEGIS):
				++stacks->aegis;
				break;
			case(EFFECT_MIGHT):
				++stacks->might;
				break;
			case(EFFECT_VAMPIRIC_AURA):
				++stacks->necVamp;
				break;
			case(EFFECT_EMPOWER_ALLIES):
				++stacks->warEA;
				break;
			case(EFFECT_BANNER_OF_TACTICS):
				++stacks->warTact;
				break;
			case(EFFECT_BANNER_OF_POWER):
				++stacks->warStr;
				break;
			case(EFFECT_BANNER_OF_DISCIPLINE):
				++stacks->warDisc;
				break;
			case(EFFECT_NATURALISTIC_RESONANCE):
				++stacks->revNR;
				break;
			case(EFFECT_ASSASSINS_PRESENCE):
				++stacks->revAP;
				break;
			case(EFFECT_SUN_SPIRIT):
				++stacks->sun;
				break;
			case(EFFECT_FROST_SPIRIT):
				++stacks->frost;
				break;
			case(EFFECT_STORM_SPIRIT):
				++stacks->storm;
				break;
			case(EFFECT_STONE_SPIRIT):
				++stacks->stone;
				break;
			case(EFFECT_SPOTTER):
				++stacks->spotter;
				break;
			case(EFFECT_GRACE_OF_THE_LAND):
			case(EFFECT_GRACE_OF_THE_LAND_CN):
				++stacks->GOTL;
				break;
			case(EFFECT_GLYPH_OF_EMPOWERMENT):
			case(EFFECT_GLYPH_OF_EMPOWERMENT_CN):
				++stacks->empGlyph;
				break;
			case(EFFECT_RITE_OF_THE_GREAT_DWARF):
				++stacks->revRite;
				break;
			case(EFFECT_SOOTHING_MIST):
				++stacks->sooMist;
				break;
			case(EFFECT_STRENGTH_IN_NUMBERS):
				++stacks->strInNum;
				break;
			case(EFFECT_PINPOINT_DISTRIBUTION):
				++stacks->engPPD;
				break;
			case(EFFECT_PORTAL_WEAVING):
			case(EFFECT_PORTAL_WEAVING2):
				++stacks->port_weave;
			}
		}
	}

	// Fix overstacking for might/gotl
	if (stacks->might > 25) stacks->might = 25;
	if (stacks->GOTL > 5) stacks->GOTL = 5;
}

// Read & calculate combat data
void read_combat_data(Character* c, CombatData *cd, i64 now)
{
	if (!c || !cd)
		return;

	static volatile LONG m_lock = 0;

	while (InterlockedCompareExchange(&m_lock, 1, 0))
	{
	}

	// Combat entry / exit times
	bool was_in_combat = InterlockedAnd(&cd->lastKnownCombatState, 1);
	if (!was_in_combat && c->in_combat)
	{
		bool combat_reenter = false;
		if (g_state.ooc_grace_period > 0) {

			// Adjsut to seconds
			int ooc_grace_period = g_state.ooc_grace_period * 1000000;
			if (now - cd->end <= ooc_grace_period)
				combat_reenter = true;
		}

		if (!combat_reenter) {
			// Combat enter
			if (cd->update) { DBGPRINT(TEXT("[%s] COMBAT ENTER"), c->name); }
			PortalData old_pd = cd->pd;
			memset(cd, 0, sizeof(CombatData));
			cd->begin = now;
			cd->pd = old_pd;
		}
		else {
			if (cd->update) { DBGPRINT(TEXT("[%s] COMBAT REENTER"), c->name); }
		}
	}
	else if (was_in_combat && !c->in_combat)
	{
		// Combat exit
		cd->end = now;
		cd->duration = (cd->end - cd->begin) / 1000.0f;

		DBGPRINT(TEXT("[%s] COMBAT EXIT  duration:%.2f"), c->name, (cd->duration / 1000.0f));
	}

	// Calc total combat time
	if (c->in_combat)
	{
		cd->duration = (now - cd->begin) / 1000.0f;
	}

	// Calc no. of downs
	bool was_downed = cd->lastKnownDownState;
	cd->lastKnownDownState = Ch_IsDowned(c->cptr);
	if (!was_downed && cd->lastKnownDownState) {
		cd->noDowned++;
	}


	// Calc buff combat uptime while
	// in combat & on combat exit
	if ( (c->in_combat || was_in_combat) &&
		cd->update &&
		c->cbptr )
	{
		f32 tick = (now - cd->update) / 1000.0f;

		// Scholar buff uptime
		if (c->hp_val > 0 &&
			c->hp_max > 0 &&
			c->hp_max >= c->hp_val) {
			u32 hpper = (u32)ceilf(c->hp_val / (f32)c->hp_max * 100.0f);
			if (hpper >= 90)
				cd->sclr_dura += tick;
		}

#define SWD_UPDATE_FREQ  500000

		if (cd->update_pos == 0)
			cd->update_pos = -SWD_UPDATE_FREQ;
		if (now - cd->update_pos >= SWD_UPDATE_FREQ)
		{
			f32 swd_tick = (now - cd->update_pos) / 1000.0f;
			cd->update_pos = now;

			// Seaweed salad buff uptime
			vec4 pos = c->apos;

			if (cd->pos.x == 0 &&
				cd->pos.y == 0 &&
				cd->pos.z == 0) {
				cd->seaw_dura += cd->duration;
				//if (c->pptr == g_player.c.pptr)
				//	DBGPRINT(TEXT("LAST POS INIT  %.2f %.2f %.2f"), pos.x, pos.y, pos.z);
			}
			else if (
				cd->pos.x != pos.x ||
				cd->pos.y != pos.y ||
				cd->pos.z != pos.z) {
				cd->seaw_dura += swd_tick;
				//if (c->pptr == g_player.c.pptr)
				//	DBGPRINT(TEXT("MOVE  dura %.2f total %.2f"), cd->seaw_dura, cd->duration);
			}
			else {
				//if (c->pptr == g_player.c.pptr)
				//	DBGPRINT(TEXT("STAND dura %.2f total %.2f "), cd->seaw_dura, cd->duration);
			}
			memcpy(&cd->pos, &pos, sizeof(cd->pos));
			cd->duration500 = cd->duration;
		}

		u64 buffBarPtr = process_read_u64(c->cbptr + OFF_CMBTNT_BUFFBAR);
		if (buffBarPtr) {

			BuffStacks stacks = { 0 };
			read_buff_array(buffBarPtr, &stacks);
			
			if (stacks.vigor)
				cd->vigor_dura += tick;
			if (stacks.swift)
				cd->swift_dura += tick;
			if (stacks.stab)
				cd->stab_dura += tick;
			if (stacks.retal)
				cd->retal_dura += tick;
			if (stacks.resist)
				cd->resist_dura += tick;
			if (stacks.regen)
				cd->regen_dura += tick;
			if (stacks.aegis)
				cd->aegis_dura += tick;
			if (stacks.necVamp)
				cd->necVA_dura += tick;
			if (stacks.warEA)
				cd->warEA_dura += tick;
			if (stacks.revNR)
				cd->revNR_dura += tick;
			if (stacks.revAP)
				cd->revAP_dura += tick;
			if (stacks.sun)
				cd->sun_dura += tick;
			if (stacks.frost)
				cd->frost_dura += tick;
			if (stacks.storm)
				cd->storm_dura += tick;
			if (stacks.stone)
				cd->stone_dura += tick;
			if (stacks.spotter)
				cd->spot_dura += tick;

			if (stacks.quick)
				cd->quik_dura += tick;
			if (stacks.alacrity)
				cd->alac_dura += tick;
			if (stacks.prot)
				cd->prot_dura += tick;
			if (stacks.fury)
				cd->fury_dura += tick;
			if (stacks.warStr)
				cd->bans_dura += tick;
			if (stacks.warDisc)
				cd->band_dura += tick;
			if (stacks.warTact)
				cd->bant_dura += tick;
			if (stacks.empGlyph)
				cd->glem_dura += tick;

			if (stacks.revRite)
				cd->revRD_dura += tick;
			if (stacks.sooMist)
				cd->eleSM_dura += tick;
			if (stacks.strInNum)
				cd->grdSIN_dura += tick;
			if (stacks.engPPD)
				cd->engPPD_dura += tick;

			cd->might_avg = (cd->might_dura * cd->might_avg + tick * stacks.might) / (cd->might_dura + tick);
			if (stacks.might)
				cd->might_dura += tick;
			if (cd->might_avg > 25)
				cd->might_avg = 25.0f;

			cd->gotl_avg = (cd->gotl_dura * cd->gotl_avg + tick * stacks.GOTL) / (cd->gotl_dura + tick);
			if (stacks.GOTL)
				cd->gotl_dura += tick;
			if (cd->gotl_avg > 5)
				cd->gotl_avg = 5.0f;
		}
	}

#if !(defined BGDM_TOS_COMPLIANT)
	if (&g_player.combat == cd) {

		u64 buffBarPtr = process_read_u64(c->cbptr + OFF_CMBTNT_BUFFBAR);
		if (buffBarPtr) {

			BuffStacks stacks = { 0 };
			read_buff_array(buffBarPtr, &stacks);

			if (stacks.port_weave && !cd->pd.is_weave) {
				cd->pd.is_weave = true;
				cd->pd.time = now;
				cd->pd.pos_trans = c->tpos;
				cd->pd.pos_agent = c->apos;
				cd->pd.map_id = g_mum_mem->map_id;
				cd->pd.shard_id = g_mum_mem->shard_id;
				cd->pd.pos_mum = g_mum_mem->avatar_pos;
			}
			else if (!stacks.port_weave) {
				cd->pd.is_weave = false;
			}
		}
	}
#endif

	// Save last combat state
	cd->update = now;
	InterlockedExchange(&cd->lastKnownCombatState, c->in_combat);

	InterlockedExchange(&m_lock, 0);
}

static void read_agent_health(Character* c)
{
	if (c->aptr == 0 && c->cptr == 0)
		return;

	if (c->aptr==0 && c->cptr) {
		c->aptr = process_read_u64(c->cptr + OFF_CHAR_AGENT);
		if (c->aptr == 0)
			return;
	}

	if (c->aptr != 0 && c->id == 0) {
		c->id = process_read_u32(c->aptr + OFF_AGENT_ID);
		c->type = process_read_u32(c->aptr + OFF_AGENT_TYPE);
	}

	u64 hptr = 0;
	if (c->type == AGENT_TYPE_ATTACK)
	{
		u64 optr = process_read_u64(c->aptr + OFF_AGENT_GADGET);
		u64 wptr = process_read_u64(optr + OFF_GADGET_WBOSS);
		process_read(wptr + OFF_WBOSS_HP, &hptr, sizeof(hptr));
	}
	else if (c->type == AGENT_TYPE_GADGET)
	{
		u64 optr = process_read_u64(c->aptr + OFF_AGENT_GADGET);
		process_read(optr + OFF_GADGET_HP, &hptr, sizeof(hptr));
	}
	else if (c->cptr)
	{
		process_read(c->cptr + OFF_CHAR_HP, &hptr, sizeof(hptr));
	}

	if (hptr)
	{
		c->hp_val = (i32)process_read_f32(hptr + OFF_HP_VAL);
		c->hp_max = (i32)process_read_f32(hptr + OFF_HP_MAX);
	}
}

// Read character common
static void read_character(Character* c, Array *pa, i64 now)
{
	if (!c || c->aptr == 0 || c->cptr == 0)
	{
		return;
	}

	u64 cptr = c->cptr;

	c->attitude = process_read_u32(cptr + OFF_CHAR_ATTITUDE);
	c->inventory = process_read_u64(cptr + OFF_CHAR_INVENTORY);

	c->bptr = process_read_u64(cptr + OFF_CHAR_BREAKBAR);
	if (c->bptr) {
		c->bb_state = process_read_i32(c->bptr + OFF_BREAKBAR_STATE);
		c->bb_value = (u32)(process_read_f32(c->bptr + OFF_BREAKBAR_VALUE) * 100.0f);
		c->bb_value = min(c->bb_value, 100);
	}

	// Is the character in combat
	c->in_combat = process_read_i32(cptr + OFF_CHAR_FLAGS) & CHAR_FLAG_IN_COMBAT ? 1 : 0;

	// Get the combatant (call the combatant VT)
	c->cbptr = Ch_GetCombatant(cptr);

	// Is character a player?
	c->is_player = Ch_IsPlayer(cptr);

	if (c->is_player) {

		// Get the player profession
		u64 sptr = process_read_u64(cptr + OFF_CHAR_STATS);
		if (sptr) {
			c->race = process_read_i8(sptr + OFF_STATS_RACE);
			c->gender = process_read_i8(sptr + OFF_STATS_GENDER);
			c->profession = process_read_i32(sptr + OFF_STATS_PROFESSION);
		}

		sptr = process_read_u64(cptr + OFF_CHAR_PROFESSION);
		if (sptr)
			c->stance = process_read_i32(sptr + OFF_PROFESSION_STANCE);

		// Can only call GetPlayer() from main thread
		// but we can call GetPlayerId() and retrieve the pptr
		// from the playerArray[agent_id]

		if (!c->pptr && pa) {
			u64 playerID = Ch_GetPlayerId(cptr);
			if (playerID && playerID < MAX_PLAYERS)
			{
				u64 pp = g_closest_pap[playerID];
				if (pp != 0)
				{
					u64 cp = process_read_u64(pp + OFF_PLAYER_CHAR);
					if (cp && cp == c->cptr) {
						c->pptr = pp;
					}
				}				
			}
		}
	}

	// Get species def for NPCs
	// Can only be called from main game thread
	/*if (!c->is_player) {
		Ch_GetSpeciesDef(c->cptr, &c->spdef);
	}*/

	// Get the contact def
	if (c->is_player && c->pptr) {
		c->ctptr = CtxContact_GetContact(g_ptrs.contactContext, c->pptr);
	}

	// if we have pptr, get the target name from the player class
	// NOTE: char name cannot be fount on non-player characters
	if (c->pptr) read_nameW(c->name, c->pptr, OFF_PLAYER_NAME);
}

// Read agent common
void read_agent(Character* c, u64 aptr, u64 cptr, Array *pa, i64 now)
{
	if (!c || (aptr == 0 && cptr == 0)) {
		return;
	}

	// if we are missing agent and have char ptr
	// get the agent from the char ptr
	if (!aptr && cptr) {
		aptr = process_read_u64(cptr + OFF_CHAR_AGENT);
		if (!aptr)
			return;
	}

	c->aptr = aptr;
	c->cptr = cptr;

	c->id = process_read_u32(aptr + OFF_AGENT_ID);
	c->type = process_read_u32(aptr + OFF_AGENT_TYPE);

	c->tptr = process_read_u64(aptr + OFF_AGENT_TRANSFORM);
	c->tpos = process_read_vec3(c->tptr + OFF_TRANSFORM_POS);

	Ag_GetPos(c->aptr, (D3DXVECTOR4*)&c->apos);

	// Read the agent HP data
	read_agent_health(c);

	if (cptr)
		read_character(c, pa, now);

	// Always get the WmAgent
	c->wmptr = Ag_GetWmAgemt(c->aptr);

	// If we don't have the name yet
	// resolve it using the game's text engine CB
	// below calls WmAgent->GetCodedName
	// and then resolves it on the main game thread

	// TODO: change to CLASS_TYPE_WMAGENT and
	// remove the redundant call to Ag_GetWmAgemt()
	// will be called by lru_resolve() on main thread
	if (c->aptr && c->name[0] == 0) {
		c->decodedName = lru_find(CLASS_TYPE_AGENT, c->aptr, c->name, ARRAYSIZE(c->name));
	}
}


// Reads player state.
static void read_player(Character* player, u64 cptr, u64 pptr, i64 now)
{
	if (cptr == 0)
	{
		return;
	}

	player->pptr = pptr;
	player->is_player = 1;
	read_agent(player, 0, cptr, NULL,  now);

	CharStats stats;
	process_read(process_read_u64(cptr + OFF_CHAR_STATS) + OFF_STATS_BASE, &stats, sizeof(stats));

	player->stats.pow = (u16)stats.pow;
	player->stats.pre = (u16)stats.pre;
	player->stats.tuf = (u16)stats.tuf;
	player->stats.vit = (u16)stats.vit;
	player->stats.fer = (u16)stats.fer;
	player->stats.hlp = (u16)stats.hlp;
	player->stats.cnd = (u16)stats.cnd;
	player->stats.con = (u16)stats.con;
	player->stats.exp = (u16)stats.exp;

	// Refresh current player stats
	// player changed or client requested reset
	if (g_player.c.aptr != player->aptr ||
		is_bind_down(&g_state.bind_dps_reset))
	{
		//memcpy(&g_player.c, player, sizeof(g_player.c));
		memset(&g_player.combat, 0, sizeof(g_player.combat));
	}

	// Always copy latest char
	memcpy(&g_player.c, player, sizeof(g_player.c));

	// Calculate combat state
	read_combat_data(player, &g_player.combat, now);
}


// Reads target state.
void read_target(Character* t, Array* ca, Array *pa, u64 aptr, i64 now)
{
	t->shard = g_mum_mem->shard_id;

	if (aptr == 0)
	{
		return;
	}

	t->id = process_read_u32(aptr + OFF_AGENT_ID);
	t->cptr = process_read_u64(ca->data + t->id * 8);

	read_agent(t, aptr, t->cptr, pa, now);
}

// Get the close players array
bool get_closest_players(ClosePlayer **pArr, u32 *pNum)
{
	if (!pArr || !pNum) return false;

	*pArr = g_closest_players;
	*pNum = g_closest_num;
	return true;
}

static bool read_closest_x(Character* player, Array* pa, i32 closest_ind[], f32 closest_len[], u32 closest_num, int mode)
{
	// If the player array is invalid, then stop.
	if (!pa || pa->data == 0 || pa->cur == 0 || pa->cur > pa->max || closest_num == 0)
	{
		return false;
	}

	for (u32 i = 0; i < closest_num; ++i)
	{
		closest_ind[i] = -1;
		closest_len[i] = FLT_MAX;
	}

	u64 player_cptr = player->cptr;
	vec3 player_pos = player->tpos;

	u32 pan = min(pa->cur, MAX_PLAYERS);
	for (u32 i = 0; i < pan; ++i)
	{
		// Verify that the player is valid.
		u64 pp = g_closest_pap[i];
		if (pp == 0)
		{
			continue;
		}

		u64 cptr = process_read_u64(pp + OFF_PLAYER_CHAR);
		if (cptr == 0 || cptr == player_cptr)
		{
			continue;
		}

		u64 aptr = process_read_u64(cptr + OFF_CHAR_AGENT);
		if (aptr == 0)
		{
			continue;
		}

		u64 tptr = process_read_u64(aptr + OFF_AGENT_TRANSFORM);
		if (tptr == 0)
		{
			continue;
		}

		u32 attitude = process_read_u32(cptr + OFF_CHAR_ATTITUDE);
		if (attitude != CHAR_ATTITUDE_FRIENDLY)
		{
			continue;
		}

		if (g_state.hideNonSquad) {
			u32 subGroup = 0;
			u64 ctptr = CtxContact_GetContact(g_ptrs.contactContext, pp);
			if (ctptr)
				subGroup = Pl_GetSquadSubgroup(g_ptrs.squadContext, ctptr);

			if (subGroup == 0)
				continue;
		}
		else if (g_state.hideNonParty) {
			if (!Pl_IsInPartyOrSquad(pp, 0, g_ptrs.contactContext))
				continue;
		}

		// Check the distance to the player, insert or remove the farthest if full.
		vec3 pos;
		process_read(tptr + OFF_TRANSFORM_POS, &pos, sizeof(pos));

		f32 dist =
			(player_pos.x - pos.x) * (player_pos.x - pos.x) +
			(player_pos.y - pos.y) * (player_pos.y - pos.y) +
			(player_pos.z - pos.z) * (player_pos.z - pos.z);

		u32 largest_ind = 0;
		f32 largest_len = closest_len[0];

		for (u32 j = 0; j < closest_num; ++j)
		{
			if (closest_len[j] > largest_len)
			{
				largest_len = closest_len[j];
				largest_ind = j;
			}
		}

		if (dist < closest_len[largest_ind])
		{
			closest_ind[largest_ind] = (i32)i;
			closest_len[largest_ind] = dist;
		}
	}

	return true;
}

u32 read_closest_x_players(Character* player, ClosePlayer closest_arr[], u32 closest_num, i64 now, int mode)
{
	// Get players from the player array.
	static i32 closest_ind[SQUAD_SIZE_MAX] = { 0 };
	static f32 closest_len[SQUAD_SIZE_MAX] = { 0 };

	Array play_array = { 0 };
	process_read(g_ptrs.pCharCliCtx + OFF_CCTX_PLAYS, &play_array, sizeof(play_array));

	memset(closest_ind, 0, sizeof(closest_ind));
	memset(closest_len, 0, sizeof(closest_len));

	if (closest_num > SQUAD_SIZE_MAX)
		closest_num = SQUAD_SIZE_MAX;

	if (!read_closest_x(player, &play_array, closest_ind, closest_len, closest_num, mode)) {
		return 0;
	}

	u32 idx = 0;
	for (u32 i = 0; i < closest_num; ++i)
	{
		if (closest_ind[i] < 0)
		{
			continue;
		}

		ClosePlayer *cp = &closest_arr[idx];
		cp->c.pptr = g_closest_pap[closest_ind[i]];
		cp->c.cptr = process_read_u64(cp->c.pptr + OFF_PLAYER_CHAR);;

		assert(cp->c.cptr);
		read_agent(&cp->c, 0, cp->c.cptr, &play_array, now);
		read_combat_data(&cp->c, &cp->cd, now);

		++idx;
	}

	return idx;
}

// Updates the closest player names list.
static void read_closest(Character* player, Array* pa, i64 now)
{
	static bool s_reset = false;
	static u32 s_closest_num;
	static ClosePlayer s_closest_players[MSG_CLOSEST_PLAYERS];

	// If the reset keybind is down reset the closest.
	if (is_bind_down(&g_state.bind_dps_reset))
	{
		DBGPRINT(TEXT("RESET CLOSEST"));
		s_reset = true;
		return;
	}

	// Only update occasionally.
	// If user requested reset, refresh immediately
	static i64 last_update = -CLOSEST_UPDATE_FREQ;
	if (!s_reset) {
		if (now - last_update < CLOSEST_UPDATE_FREQ)
		{
			return;
		}
	}

	if (s_reset)
	{
		// Reset was requested
		s_reset = false;
		s_closest_num = 0;
		memset(s_closest_players, 0, sizeof(s_closest_players));
	}
	else
	{
		// Save last update data
		last_update = now;
		s_closest_num = g_closest_num;
		memcpy(s_closest_players, g_closest_players, sizeof(s_closest_players));
	}


	// Reset the closest player list.
	g_closest_num = 0;
	memset(g_closest_names, 0, sizeof(g_closest_names));
	memset(g_closest_players, 0, sizeof(g_closest_players));

	// Get players from the player array.
	i32 closest_ind[MSG_CLOSEST_PLAYERS] = { 0 };
	f32 closest_len[MSG_CLOSEST_PLAYERS] = { 0 };

	if (!read_closest_x(player, pa, closest_ind, closest_len, MSG_CLOSEST_PLAYERS, 0)) {
		return;
	}

	// Read the names of the closest players.
	for (u32 i = 0; i < MSG_CLOSEST_PLAYERS; ++i)
	{
		if (closest_ind[i] < 0)
		{
			continue;
		}
		u64 pptr = g_closest_pap[closest_ind[i]];
		u64 cptr = process_read_u64(pptr + OFF_PLAYER_CHAR);

		ClosePlayer *cp = &g_closest_players[g_closest_num];
		bool found = false;

		// Search for the old combatant
		for (u32 j = 0; j < s_closest_num; ++j)
		{
			if (cptr == s_closest_players[j].c.cptr)
			{
				memcpy(&g_closest_players[g_closest_num], &s_closest_players[j], sizeof(g_closest_players[g_closest_num]));
				found = true;
			}
		}

		if (!found) {
			cp->c.cptr = cptr;
			cp->c.pptr = pptr;
		}

		assert(cp->c.cptr);
		read_agent(&cp->c, 0, cp->c.cptr, pa, now);
		read_combat_data(&cp->c, &cp->cd, now);

		// Fallback to get name from decodedName
		// func inside read_agent (through lru_find())
		if (cp->c.name[0] != 0)
			memcpy(g_closest_names[g_closest_num], cp->c.name, sizeof(g_closest_names[g_closest_num]));
		else if (cp->c.decodedName && cp->c.decodedName[0] != 0)
			StringCchCopyW(g_closest_names[g_closest_num], ARRAYSIZE(g_closest_names[g_closest_num]), cp->c.decodedName);

		// Skip if we have no name
		if (g_closest_names[g_closest_num][0] == 0)
			continue;

		cp->name = &g_closest_names[g_closest_num][0];

		++g_closest_num;
	}
}


// Retrieve a dps target.
DPSTarget* dps_targets_get(u32 i)
{
	if (g_targets)
		return &g_targets[i];
	else
		return NULL;
}

// Deletes a dps target.
static void dps_targets_delete(u32 i)
{
	ImGui_RemoveGraphData(g_targets[i].id);
	memset(&g_targets[i], 0, sizeof(g_targets[i]));
}

// Finds a DPS target. Returns -1 if the target doesn't exist.
static u32 dps_targets_find(u64 target)
{
	if (target == 0)
	{
		return MAX_TARGETS;
	}

	u32 id = process_read_u32(target + OFF_AGENT_ID);
	if (id == 0)
	{
		return MAX_TARGETS;
	}

	for (u32 i = 0; i < MAX_TARGETS; ++i)
	{
		if (g_targets[i].id == id)
		{
			return i;
		}
	}

	return MAX_TARGETS;
}

// Inserts a DPS target. Returns -1 on failure.
u32 dps_targets_insert(u64 target, u64 cptr, Array* ca, i64 now)
{
	if (target == 0)
	{
		return MAX_TARGETS;
	}

	i64 old_time = MAXINT64;
	u32 old_index = 0;
	u32 shard_id = g_mum_mem->shard_id;
	u32 id = process_read_u32(target + OFF_AGENT_ID);

	if (id == 0)
	{
		return MAX_TARGETS;
	}

	for (u32 i = 0; i < MAX_TARGETS; ++i)
	{
		if (g_targets[i].id == id)
		{
			g_targets[i].aptr = target;

			return i;
		}

		if (g_targets[i].time_update < old_time)
		{
			old_index = i;
			old_time = g_targets[i].time_update;
		}
	}

	dps_targets_delete(old_index);

	if (cptr == 0)
		cptr = Ag_GetChar(target, ca);

	g_targets[old_index].aptr = target;
	g_targets[old_index].cptr = cptr;
	g_targets[old_index].shard = shard_id;
	g_targets[old_index].id = id;
	g_targets[old_index].time_begin = now;
	g_targets[old_index].time_update = now;
	g_targets[old_index].hp_last = -1;
	g_targets[old_index].wmptr = Ag_GetWmAgemt(target);

	// Make sure cache doesn't have old names for this target
	lru_delete(CLASS_TYPE_AGENT, target);
	lru_delete(CLASS_TYPE_CHAR, cptr);

	Character c = { 0 };
	c.aptr = target;
	c.cptr = cptr;
	read_agent_health(&c);

	g_targets[old_index].hp_last = c.hp_val;

	return old_index;
}


void dps_targets_reset()
{
	DBGPRINT(TEXT("RESET TARGETS"));
	memset(g_targets, 0, sizeof(*g_targets)*MAX_TARGETS);
	ImGui_ResetGraphData(0);
}

void dps_targets_get_species()
{
	Array char_array;
	process_read(g_ptrs.pCharCliCtx + OFF_CCTX_CHARS, &char_array, sizeof(char_array));

	for (u32 i = 0; i < MAX_TARGETS; ++i)
	{
		DPSTarget* t = &g_targets[i];
		if (!t ||
			t->aptr == 0 ||
			t->cptr == 0 ||
			t->species_id != 0 ||
			!Ag_Validate(t->id, t->aptr, t->wmptr))
			continue;

		uint64_t speciesDef = 0;
		if (t->cptr && Ch_GetSpeciesDef(t->cptr, &speciesDef)) {
			DBGPRINT(TEXT("aptr %p cptr %p speciesDef %p"), t->aptr, t->cptr, speciesDef);
			t->npc_id = process_read_u32(speciesDef + OFF_SPECIES_DEF_ID);
			t->species_id = process_read_u32(speciesDef + OFF_SPECIES_DEF_HASHID);
		}
	}
}


static __inline void hit_array_calc_interval(u32 *dmg, const DPSHit *hit_arr, u32 hit_cnt, i64 interval, i64 now)
{
	*dmg = 0;
	for (i32 i = hit_cnt - 1; i >= 0; --i) {
		if (now - hit_arr[i].time > interval)
			break;
		*dmg += hit_arr[i].dmg;
	}
}

// Removes invalid or old targets from the array.
// Calculates targets dps numbers
static void dps_targets_update(Character* player, Array* ca, i64 now)
{
	static i64 last_update = 0;

	// No need to calc twice for the same frame
	if (now == last_update)
		return;

	// If reset was requested reset all target counters
	if (is_bind_down(&g_state.bind_dps_reset))
	{
		last_update = now;
		dps_targets_reset();
		return;
	}

	bool isPlayerDead = !Ch_IsAlive(player->cptr) && !Ch_IsDowned(player->cptr);
	bool isPlayerOOC = !InterlockedAnd(&g_player.combat.lastKnownCombatState, 1);

	for (u32 i = 0; i < MAX_TARGETS; ++i)
	{
		DPSTarget* t = &g_targets[i];
		if (!t || t->aptr == 0)
		{
			continue;
		}

		// Remove aged and invalid ptrs
		if (t->aptr == player->aptr ||
			g_mum_mem->shard_id != t->shard ||
			now - t->time_update > MAX_TARGET_AGE ||
			!Ag_Validate(t->id, t->aptr, t->wmptr))
		{
			DBGPRINT(TEXT("target delete <aptr=%p>"), t->aptr);
			dps_targets_delete(i);
			continue;
		}

		// Target is new  and was never hit, skip
		if (t->num_hits == 0)
			continue;

		// Target is already dead, skip calculation
		if (t->isDead)
			continue;

		Character c = { 0 };
		c.aptr = t->aptr;
		c.cptr = t->cptr;
		read_agent_health(&c);
		// If the mob is at full HP and it's been a while since we hit it, autoreset.
		if (c.hp_val == c.hp_max && now - t->time_hit > RESET_TIMER)
		{
			dps_targets_delete(i);
			continue;
		}

		// We don't calculate invuln time or OOC time
		if (t->invuln || isPlayerDead || isPlayerOOC) {
			goto next_target;
		}

		// Total time always starts from first hit
		if (t->duration == 0 || t->last_update == 0)
			t->duration = now - t->hits[0].time;
		else
			t->duration += now - t->last_update;

		// Update the total DPS and damage.
		if (t->hp_last >= 0)
		{
			i32 diff = hp_diff(c.hp_val, t->hp_last, c.hp_max);
			if (diff > 0) {
				// only calculate a hit if the target lost health
				// otherwise can get buggy if the target uses a healing skill
				t->hp_lost += diff;
				i32 hi = t->num_hits_team;
				if (hi < MAX_HITS)
				{
					t->hits_team[hi].dmg = diff;
					t->hits_team[hi].time = now;
					++t->num_hits_team;
				}
			}
		}

		// The game reports more damage than the target's HP on the last hit
		// for the sake of consistency, equalize both numbers if we did more dmg
		// than the target's HP, shouldn't matter much only for small targets
		// where we are the only attacker
		if (t->hp_lost < (i32)t->tdmg) {
			DBGPRINT(TEXT("target dmg adjust <aptr=%p> <hp_lost=%d> <tdmg=%d>"), t->aptr, t->hp_lost, t->tdmg);
			t->hp_lost = (i32)t->tdmg;
		}

		hit_array_calc_interval(&t->c1dmg, t->hits, t->num_hits, DPS_INTERVAL_1, now);
		hit_array_calc_interval(&t->c2dmg, t->hits, t->num_hits, DPS_INTERVAL_2, now);

		hit_array_calc_interval(&t->c1dmg_team, t->hits_team, t->num_hits_team, DPS_INTERVAL_1, now);
		hit_array_calc_interval(&t->c2dmg_team, t->hits_team, t->num_hits_team, DPS_INTERVAL_2, now);

#ifdef _DEBUG
		bool was_dead;
#endif

	next_target:

#ifdef _DEBUG
		was_dead = t->isDead;
#endif
		if (t->cptr)
			t->isDead = !Ch_IsAlive(t->cptr) && !Ch_IsDowned(t->cptr);
		else
			t->isDead = c.hp_val <= 0;
#ifdef _DEBUG
		if (!was_dead && t->isDead)
			DBGPRINT(TEXT("target is now dead <aptr=%p> <cptr=%p>"), t->aptr, t->cptr);
#endif
		if (t->hp_max==0) t->hp_max = c.hp_max;
		t->hp_last = c.hp_val;
		t->last_update = now;
	}

	last_update = now;
}

#if !(defined BGDM_TOS_COMPLIANT)

// Update inventory inspection
static void module_char_inspect(Character *target, Character *player, bool bUseLocalizedText)
{
	// Check if the target is a valid friendly player character
	if (target
		&& target->aptr
		&& target->is_player
		&& (target->attitude == CHAR_ATTITUDE_FRIENDLY)
		)
	{
		ImGui_CharInspect(&g_state.panel_gear_target, target);
	}

	if (player) {
		ImGui_CharInspect(&g_state.panel_gear_self, player);
	}
}

#endif

// Updates and draws debug information.
static void module_debug(Character* player, Character* target, Array* char_array, Array* play_array, i64 now)
{
	// Disable if not in debug mode.
	if (g_state.panel_debug.enabled == false)
	{
		return;
	}

	ImGui_Debug(&g_state.panel_debug, player, target, char_array, play_array);
}

bool get_cam_data(CamData *camData, i64 now)
{
	static volatile LONG m_lock = 0;

	while (InterlockedCompareExchange(&m_lock, 1, 0))
	{
	}

	if (!camData)
		return false;

	if (camData->valid &&
		camData->lastUpdate > 0 &&
		now > 0)
	{
		if (camData->lastUpdate == now) {
			InterlockedExchange(&m_lock, 0);
			return true;
		}
	}

	camData->valid = false;
	if (g_ptrs.pWorldView)
	{
		u64 wvctx = process_read_u64(g_ptrs.pWorldView);
		if (wvctx)
		{
			int wvctxStatus = process_read_i32(wvctx + OFF_WVCTX_STATUS);
			if (wvctxStatus == 1)
			{
				// TODO: Check why this raises an exception
				// right now it's ok as it's wrapped in try/except clause
				if (WV_GetMetrics(wvctx, 1, &camData->camPos, &camData->lookAt, &camData->upVec, &camData->fovy)) {

					D3DXVECTOR3 viewVec;
					D3DXVec3Subtract(&viewVec, (const D3DXVECTOR3*)&camData->lookAt, (const D3DXVECTOR3*)&camData->camPos);
					D3DXVec3Normalize((D3DXVECTOR3 *)&camData->viewVec, &viewVec);

					camData->curZoom = process_read_f32(wvctx + OFF_CAM_CUR_ZOOM);
					camData->minZoom = process_read_f32(wvctx + OFF_CAM_MIN_ZOOM);
					camData->maxZoom = process_read_f32(wvctx + OFF_CAM_MAX_ZOOM);

					camData->lastUpdate = now;
					camData->valid = true;
				}

				InterlockedExchange(&m_lock, 0);

				return true;
			}
		}
	}

	InterlockedExchange(&m_lock, 0);

	return false;
}

bool dps_update_cam_data()
{
	i64 now = time_get();
	return get_cam_data(&g_cam_data, now);
}

CamData* dps_get_cam_data()
{
	return &g_cam_data;
}


bool project2d(POINT* outPoint, const D3DXVECTOR3 *camPos, const D3DXVECTOR3 *viewVec, const D3DXVECTOR3 *upVec, const D3DXVECTOR3 *worldPt, float fovy)
{
	D3DXVECTOR3 lookAtPosition = { 0.f, 0.f, 0.f };
	D3DXMATRIX matProj, matView, matWorld;
	D3DXVECTOR3 screenPos;
	IDirect3DDevice9* pDevice = ImGui_GetDevice();
	D3DVIEWPORT9 vp;

	pDevice->lpVtbl->GetViewport(pDevice, &vp);

	D3DXVec3Add(&lookAtPosition, camPos, viewVec);
	D3DXMatrixLookAtLH(&matView, camPos, &lookAtPosition, upVec);
	D3DXMatrixPerspectiveFovLH(&matProj, fovy, (float)vp.Width / (float)vp.Height, 1.0f, 10000.0f);

	D3DXMatrixIdentity(&matWorld);
	D3DXVec3Project(&screenPos, worldPt, &vp, &matProj, &matView, &matWorld);

	if (outPoint)
	{
		outPoint->x = (LONG)(screenPos.x);
		outPoint->y = (LONG)(screenPos.y);
	}

	// Verify Pt is inside the screen
	if (screenPos.x < 0 || screenPos.x > vp.Width ||
		screenPos.y < 0 || screenPos.y > vp.Height || screenPos.z > 1.0f)
		return false;
	else
		return true;
}


float get_mum_fovy()
{
	float ret = 0;
	if (!g_mum_mem)
		return ret;
	wchar_t *pwcFov = wcsstr(g_mum_mem->identity, L"\"fov\":");
	if (pwcFov) {
		wchar_t *pwcEnd = NULL;
		ret = wcstof(pwcFov + 6, &pwcEnd);
	}
	return ret;
}

bool project2d_agent(POINT* outPoint, vec4* worldPt, u64 aptr, i64 now)
{
	if (!aptr || !worldPt)
		return false;

	// Update cam data
	// We only call this now from main thread hkGameThread
	// as it will randomly raise exceptions being called
	// outside of main game thread
	//get_cam_data(&g_cam_data, now);

	if (!g_cam_data.valid)
		return false;

	D3DXVECTOR3 vUpVec = { 0.0f, 0.0f, -1.0f }; // Z is UP
	return project2d(outPoint, (const D3DXVECTOR3*)&g_cam_data.camPos, (const D3DXVECTOR3*)&g_cam_data.viewVec, &vUpVec, (const D3DXVECTOR3*)worldPt, g_cam_data.fovy);
}

bool project2d_mum(POINT* outPoint, const vec3 *pos, u64 aptr, bool invertYZ)
{	
	if (!pos || get_mum_fovy() <= 0)
		return false;

	static const float inch2meter = 0.0254f;
	static const float meter2inch = 39.370078740157f;
	static const float d3dx2mum = 0.8128f;

	D3DXVECTOR3 vUpVec = { 0.0f, 0.0f, -1.0f }; // Z is UP
	D3DXVECTOR3 camPos = { 0.f, 0.f, 0.f };
	D3DXVECTOR3 viewVec = { 0.f, 0.f, 0.f };
	D3DXVECTOR3 worldPt = { 0.f, 0.f, 0.f };

	// mumble data has Y for up and an inverted Z for cam_front (at y)
	// and swiched x/y
	camPos.x = g_mum_mem->cam_pos.x;
	camPos.y = g_mum_mem->cam_pos.z;
	camPos.z = -(g_mum_mem->cam_pos.y);
	viewVec.x = g_mum_mem->cam_front.x;
	viewVec.y = g_mum_mem->cam_front.z;
	viewVec.z = -(g_mum_mem->cam_front.y);
	if (invertYZ) {
		// When using g_mum_mem->avatar_pos
		worldPt.x = pos->x;
		worldPt.y = pos->z;
		worldPt.z = -(pos->y);
	} else {
		
		// Fuck it, use the AgPos and convert inches to meters
		D3DXVECTOR4 agPos;
		if (aptr && Ag_GetPos(aptr, &agPos))
		{
			worldPt.x = agPos.x * inch2meter;
			worldPt.y = agPos.y * inch2meter;
			worldPt.z = agPos.z * inch2meter;
		}
		else
		{
			// TODO: figure out where this stupid 0.8218f is coming from
			// perhaps tan of fovy?
			worldPt.x = pos->x * d3dx2mum;
			worldPt.y = pos->y * d3dx2mum;
			worldPt.z = -(pos->z * d3dx2mum);
		}
	}	
	return project2d(outPoint, &camPos, &viewVec, &vUpVec, (const D3DXVECTOR3*)&worldPt,
		g_cam_data.valid ? g_cam_data.fovy : get_mum_fovy());
}

//
// http://stackoverflow.com/questions/14066933/direct-way-of-computing-clockwise-angle-between-2-vectors#16544330
float vec_angle_normalplane(vec3 vA, vec3 vB, vec3 vN)
{
	float dot = vA.x*vB.x + vA.y*vB.y + vA.z*vB.z;
	float det = vA.x*vB.y*vN.z + vB.x*vN.y*vA.z + vN.x*vA.y*vB.z -
		vA.z*vB.y*vN.x - vB.z*vN.y*vA.x - vN.z*vA.y*vB.x;
	float angle = (float)atan2(det, dot) * (float)(180 / M_PI);
	return angle;
}


float get_cam_direction(vec3 cam_dir)
{
	vec3 north = { 0 ,0, 1 };
	vec3 normal = { 0 ,1, 1 };
	float angle = vec_angle_normalplane(cam_dir, north, normal);
	bool to_north = 0;
	bool is_east = 0;
	if (abs((int)angle) <= 90) to_north = true;
	if (angle > 0) is_east = true;
	return (-angle);
}

static inline float round_to_multiple_of(f32 f, f32 multiple)
{
	f32 remainder = (f32)fmod(fabs(f), multiple);
	if (remainder == 0)
		return f;

	if (f < 0)
		//return -(abs(n) - remainder);
		return -((float)fabs(f) + multiple - remainder);
	else
		return f + multiple - remainder;
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

// Draw the compass 
static void module_compass(i32 default_x, i32 default_y, i32 default_w)
{

	if (!g_state.panel_compass.enabled || !g_cam_data.valid)
		return;

	// default UI location
	if (g_state.panel_compass.pos.x < 0)
		g_state.panel_compass.pos.x = default_x;
	if (g_state.panel_compass.pos.y < 0)
		g_state.panel_compass.pos.y = default_y;

	ImGui_Compass(&g_state.panel_compass, g_state.panel_compass.pos.x, g_state.panel_compass.pos.y, default_w);
}

// Calc target DPS
static DPSTargetEx* dps_target_get_pref(Character* sel_target, Array *ca, i64 now);
struct DPSTargetEx* dps_target_get_lastknown(i64 now)
{
	return dps_target_get_pref(NULL, NULL, now);
}

static DPSTargetEx* dps_target_get_pref(Character* sel_target, Array *ca, i64 now)
{
	static DPSTargetEx dps_target = { 0 };
	static i64 last_update;
	static u64 last_autolock_aptr = 0;

	if (is_bind_down(&g_state.bind_dps_reset)) {
		last_update = 0;
		last_autolock_aptr = 0;
		memset(&dps_target, 0, sizeof(dps_target));
		return NULL;
	}

	// No need to update twice in the same frame
	// GUI also uses this one to get last known target
	if (now == last_update &&
		dps_target.c.aptr &&
		dps_target.t.npc_id)
		return &dps_target;
	last_update = now;


	DPSTarget* t = NULL;
	Character* target = NULL;
	bool targetIsSelected = false;
	bool selectedTargetIsValid = false;
	bool selectedTargetAutolock = false;

	// Do we have a valid target selected
	if (sel_target &&
		sel_target->aptr &&
		(sel_target->type != AGENT_TYPE_CHAR || sel_target->attitude != CHAR_ATTITUDE_FRIENDLY)) {

		selectedTargetIsValid = true;
	}

	if (selectedTargetIsValid &&
		sel_target->aptr != last_autolock_aptr) {

		u32 ti = dps_targets_find(sel_target->aptr);
		if (ti >= MAX_TARGETS)
		{
			// Insert the target into the array
			ti = dps_targets_insert(sel_target->aptr, 0, ca, now);
			DBGPRINT(TEXT("target insert [aptr=%p] [cptr=%p] [hp_val=%i] [hp_max=%u]"),
				sel_target->aptr, sel_target->cptr, sel_target->hp_val, sel_target->hp_max);
		}
		else {
			t = g_targets + ti;
			if (t->npc_id) {
				last_autolock_aptr = sel_target->aptr;
				selectedTargetAutolock = autolock_get(t->npc_id);
			}
			if (!selectedTargetAutolock) t = NULL;
			else {
				DBGPRINT(TEXT("target autolock [aptr=%p] [cptr=%p] [id=%i]"), t->aptr, t->cptr, t->npc_id);
			}

		}
	}

	// Valid target selected and we aren't target locked?
	// or selected target needs to be auto-locked
	if (selectedTargetIsValid &&
		(selectedTargetAutolock || !dps_target.locked || (dps_target.locked && sel_target->aptr == dps_target.c.aptr))) {

		target = sel_target;
		dps_target.time_sel = now;
		targetIsSelected = true;
	}

	// We don't have a valid target, try last known target
	if (!target && dps_target.t.aptr) {
		
		target = &dps_target.c;
	}

	// Ignore invalid targets.
	if (!target || target->aptr == 0)
		return NULL;

	// Traget is selected, check for target swap
	if (targetIsSelected &&
		target->aptr != dps_target.c.aptr) {
		memset(&dps_target, 0, sizeof(dps_target));
	}

	// No target selected, linger time has passed
	// reset and hide the UI (by returning NULL)
	if (!targetIsSelected &&
		!dps_target.locked &&
		now - dps_target.time_sel >= g_state.target_retention_time)
	{
		last_update = 0;
		last_autolock_aptr = 0;
		memset(&dps_target, 0, sizeof(dps_target));
		return NULL;
	}

	// Toggle target lock
	config_toggle_bind(&g_state.bind_dps_lock, &dps_target.locked);
	if (selectedTargetAutolock) dps_target.locked = true;

	if (t == NULL) {
		// Check if the target has a DPS entry already.
		u32 ti = dps_targets_find(target->aptr);

		// If we don't have a valid target
		// display last known target
		if (ti >= MAX_TARGETS)
		{
			return &dps_target;
		}

		// Get the DPS target.
		t = g_targets + ti;
	}

	assert(t != NULL);
	t->time_update = now;
	dps_target.t = *t;
	dps_target.c = *target;

	return &dps_target;
}


// Updates DPS.
static void module_dps_solo(Character *target, Array *char_array, i64 now, i32 gui_x, i32 gui_y)
{
	DPSData dps_data;
	const DPSTargetEx *dps_target = dps_target_get_pref(target, char_array, now);

	if (dps_target) {

		dps_data.tot_dur = (f32)((u32)(dps_target->t.duration / 1000.0f) / 1000.0f);

		dps_data.l1_dur = min(dps_data.tot_dur, (DPS_INTERVAL_1 / 1000000.0f));
		dps_data.l2_dur = min(dps_data.tot_dur, (DPS_INTERVAL_2 / 1000000.0f));

		dps_data.l1_done = dps_target->t.c1dmg;
		dps_data.l2_done = dps_target->t.c2dmg;
		dps_data.l1_done_team = dps_target->t.c1dmg_team;
		dps_data.l2_done_team = dps_target->t.c2dmg_team;
		dps_data.tot_done = dps_target->t.tdmg;
		dps_data.hp_lost = dps_target->t.hp_lost;

		if (g_state.panel_dps_self.enabled) {
			ImGui_PersonalDps(&g_state.panel_dps_self, gui_x, gui_y, &dps_data, dps_target);
		}
	}
}


static void get_recent_targets(Character *target, Array* char_array, ClientTarget out_targets[], u32 arrsize, u32 *outsize, i64 now)
{
	if (!out_targets || !arrsize)
		return;

	const DPSTargetEx *dps_target = dps_target_get_pref(target, char_array, now);

	u32 total = 0;
	u32 arr_targets[MSG_TARGETS] = { 0 };

	if (arrsize > MSG_TARGETS)
		arrsize = MSG_TARGETS;

	for (u32 i = 0; i < arrsize; ++i)
	{
		arr_targets[i] = MAX_TARGETS;
	}

	for (u32 i = 0; i < MAX_TARGETS; ++i)
	{
		if (g_targets[i].aptr == 0 ||
			g_targets[i].id == 0 ||
			g_targets[i].time_update == 0)
		{
			continue;
		}

		if (total < arrsize)
		{
			arr_targets[total++] = i;
			continue;
		}

		i64 old_time = MAXINT64;
		u32 old_id = arrsize;

		for (u32 j = 0; j < arrsize; ++j)
		{
			i64 t = g_targets[arr_targets[j]].time_update;
			if (t < old_time)
			{
				old_time = t;
				old_id = j;
			}
		}

		if (old_id < arrsize && g_targets[i].time_update > old_time)
		{
			arr_targets[old_id] = i;
		}
	}

	bool arr_contains_dps_target = false;
	i64 old_time = MAXINT64;
	u32 old_idx = total;

	for (u32 i = 0; i < total; ++i)
	{
		i32 ind = arr_targets[i];
		if (ind < MAX_TARGETS)
		{
			const DPSTarget* t = g_targets + ind;
			if (t && t->aptr)
			{
				out_targets[i].id = t->id;
				out_targets[i].tdmg = t->tdmg;
				out_targets[i].invuln = t->invuln;
				out_targets[i].reserved1 = t->c1dmg;
				out_targets[i].time = (u32)(t->duration / 1000.0f);

				if (t->time_update < old_time)
				{
					old_time = t->time_update;
					old_idx = i;
				}

				if (dps_target &&
					dps_target->t.id == t->id)
					arr_contains_dps_target = true;
			}
		}
	}

	// If the array doesn't have the preferred/locked target
	// Overwrite the oldest with it
	if (dps_target &&
		!arr_contains_dps_target) {

		u32 idx = 0;
		if (total < arrsize)
			idx = total++;
		else {
			assert(old_idx < total);
			if (old_idx < total)
				idx = old_idx;
		}

		assert(idx < arrsize);

		if (idx < arrsize) {
			const DPSTarget* t = &dps_target->t;
			out_targets[idx].id = t->id;
			out_targets[idx].tdmg = t->tdmg;
			out_targets[idx].invuln = t->invuln;
			out_targets[idx].reserved1 = t->c1dmg;
			out_targets[idx].time = (u32)(t->duration / 1000.0f);
			//DBGPRINT(TEXT("added target <%d:%s> to  out_targets[%d]"), t->id, dps_target->c.decodedName, idx);
		}
	}

	if (outsize)
		*outsize = total;
}

static const char * name_by_profession(u32 prof)
{
	// TODO: need to add elites here
	// so that the qsort can sort by elites as well
	// (currently ranger/druid will be considered same, etc)
	static char* const prof_strings[] = {
		"###",
		"GRD",
		"WAR",
		"ENG",
		"RGR",
		"THF",
		"ELE",
		"MES",
		"NEC",
		"REV",
		"###",
	};

	if (prof > PROFESSION_END)
		return prof_strings[PROFESSION_END];
	else
		return prof_strings[prof];
}

// Compare buff player
static int __cdecl DPSPlayerCompare(void* pCtx, void const* pItem1, void const* pItem2)
{
	i32 retVal = 0;
	u32 sortBy = LOBYTE(*(u32*)pCtx);
	bool isAsc = HIBYTE(*(u32*)pCtx);
	const DPSPlayer* p1 = pItem1;
	const DPSPlayer* p2 = pItem2;

#define DPS_COMPARE(x) \
	if (p1->x < p2->x) retVal = -1; \
	else if (p1->x > p2->x) retVal = 1; \
	else retVal = 0;

	u32 p1time = 0;
	u32 p2time = 0;
	u32 p1dmg = 0;
	u32 p2dmg = 0;

	if (g_state.panel_dps_group.enabled > 1) {

		p1time = (u32)(p1->target_time / 1000.0f);
		p2time = (u32)(p2->target_time / 1000.0f);
		p1dmg = p1->target_dmg;
		p2dmg = p2->target_dmg;
	}
	else {

		p1time = (u32)(p1->time / 1000.0f);
		p2time = (u32)(p2->time / 1000.0f);
		p1dmg = p1->damage;
		p2dmg = p2->damage;
	}

	u32 p1dps = p1time == 0 ? 0 : (u32)(p1dmg / p1time);
	u32 p2dps = p2time == 0 ? 0 : (u32)(p2dmg / p2time);
	u32 p1hps = p1->time == 0 ? 0 : (u32)(p1->heal_out / (p1->time / 1000.0f));
	u32 p2hps = p2->time == 0 ? 0 : (u32)(p2->heal_out / (p2->time / 1000.0f));
	

	const char * profName1 = name_by_profession(p1->profession);
	const char * profName2 = name_by_profession(p2->profession);

	switch (sortBy)
	{
	case(GDPS_COL_NAME):
		if (!p1->name && p2->name)
			retVal = -1;
		else if (!p2->name && p1->name)
			retVal = 1;
		else if (!p1->name && !p2->name)
			retVal = 0;
		else
			retVal = wcscmp(p1->name, p2->name);
		break;
	case(GDPS_COL_CLS):
		retVal = strcmp(profName1, profName2);
		break;
	case(GDPS_COL_DPS):
		if (p1dps < p2dps) retVal = -1;
		else if (p1dps > p2dps) retVal = 1;
		else retVal = 0;
		break;
	case(GDPS_COL_HPS):
		if (p1hps < p2hps) retVal = -1;
		else if (p1hps > p2hps) retVal = 1;
		else retVal = 0;
		break;
	case(GDPS_COL_PER):
	case(GDPS_COL_DMGOUT):
		if (p1dmg < p2dmg) retVal = -1;
		else if (p1dmg > p2dmg) retVal = 1;
		else retVal = 0;
		break;
	case(GDPS_COL_DMGIN):
		DPS_COMPARE(damage_in);
		break;
	case(GDPS_COL_HEAL):
		DPS_COMPARE(heal_out);
		break;
	case(GDPS_COL_TIME):
	{
		if (p1time == 0 && p2time != 0) retVal = -1;
		else if (p2time == 0 && p1time != 0) retVal = 1;
		else if (p1time < p2time) retVal = -1;
		else if (p1time > p2time) retVal = 1;
		else retVal = 0;
	}
		break;
	};

	if (isAsc)
		return retVal;
	else
		return -(retVal);
}

static DPSPlayer* get_closest_arr(Character *target, Character* player, Array* char_array, i64 now, u32 *num)
{
	static DPSPlayer temp_players[MSG_CLOSEST_PLAYERS + 1];
	u32 total = 0;

	// Reset our player pool
	memset(temp_players, 0, sizeof(temp_players));

	// Find out which of the closest players matches the group dps information.
	for (u32 i = 0; i < g_group_num; ++i)
	{
		bool bFound = false;
		u32 foundIdx = 0;

		for (u32 j = 0; j < g_closest_num; ++j)
		{
			if (g_closest_names[j][0] &&
				g_group[i].name[0] &&
				wcscmp(g_closest_names[j], g_group[i].name) == 0)
			{
				bFound = true;
				foundIdx = j;
				break;
			}
		}

		if (bFound)
		{
			ClosePlayer *cp = &g_closest_players[foundIdx];
			assert(cp->c.cptr &&  cp->c.pptr);
			temp_players[total] = g_group[i];
			temp_players[total].c = &cp->c;
			temp_players[total].cd = &cp->cd;
			++total;
		}
	}

	// Add the current player into the group listing.
	get_recent_targets(target, char_array, temp_players[total].targets, MSG_TARGETS, NULL, now);
	temp_players[total].c = &g_player.c;
	temp_players[total].cd = &g_player.combat;
	temp_players[total].time = g_player.combat.duration;
	temp_players[total].damage = InterlockedCompareExchange(&g_player.combat.damageOut, 0, 0);
	temp_players[total].damage_in = InterlockedCompareExchange(&g_player.combat.damageIn, 0, 0);
	temp_players[total].heal_out = InterlockedCompareExchange(&g_player.combat.healOut, 0, 0);
	temp_players[total].stats = player->stats;
	temp_players[total].profession = player->profession;
	wmemcpy(temp_players[total].name, player->name, MSG_NAME_SIZE);
	++total;

	if (num)
		*num = total;

	return temp_players;
}


static void module_dps_group_combined(Character* player, Character* target, Array* char_array, i64 now, i32 gui_x, i32 gui_y, Panel* panel)
{
	const DPSTargetEx *dps_target = dps_target_get_pref(target, char_array, now);

	u32 total = 0;
	DPSPlayer* temp_players = get_closest_arr(target, player, char_array, now, &total);

	// Set total dmg/duration to current target
	for (u32 i = 0; i < total; i++) {

		if (dps_target && dps_target->t.id) {
			for (u32 j = 0; j < MSG_TARGETS; j++) {
				if (temp_players[i].targets[j].id == dps_target->t.id) {
					temp_players[i].target_time = (f32)temp_players[i].targets[j].time;
					temp_players[i].target_dmg = temp_players[i].targets[j].tdmg;
					break;
				}
			}
		}
	}

	// Sort players by most damage done.
	u32 sortBy = MAKEWORD(panel->cfg.sortCol, panel->cfg.asc);
	qsort_s(&temp_players[0], total, sizeof(temp_players[0]), DPSPlayerCompare, &sortBy);

	if (panel->enabled)
		ImGui_GroupDps(panel, gui_x, gui_y, dps_target, temp_players, total);
}

static void module_dps_group(Character* player, Character* target, Array* char_array, i64 now, i32 gui_x, i32 gui_y)
{
	Panel *panel = &g_state.panel_dps_group;
	module_dps_group_combined(player, target, char_array, now, gui_x, gui_y, panel);
}

static void module_dps_skills(Character* player, Character* target, Array* char_array, i64 now, i32 gui_x, i32 gui_y)
{
	Panel *panel = &g_state.panel_skills;
	const DPSTargetEx *dps_target = dps_target_get_pref(target, char_array, now);

	if (panel->enabled) {
		ImGui_SkillBreakdown(panel, gui_x, gui_y, dps_target);
	}
}

// Group DPS network module.
static void module_network_dps(Character* player, Character* target, Array* char_array, i64 now)
{
	assert(g_player.c.aptr == player->aptr);

	// User requested network DPS disable
	if (g_state.netDps_disable)
		return;
	
	// Check if we need to send a packet.
	static i64 last_update;
	if (now - last_update < MSG_FREQ_CLIENT_DAMAGE)
	{
		return;
	}

	const DPSTargetEx *dps_target = dps_target_get_pref(target, char_array, now);

	last_update = now;

	MsgClientDamageUTF8 msg = { 0 };
	msg.msg_type = MSG_TYPE_CLIENT_DAMAGE_UTF8;

	msg.shard_id = g_mum_mem->shard_id;
	msg.selected_id = dps_target ? dps_target->t.id : 0;
	msg.stats = player->stats;
	msg.in_combat = player->in_combat;
	msg.profession = player->profession;
	msg.time = g_player.combat.duration;

	//msg.damage = g_player.combat.damage_out;
	msg.damage = InterlockedCompareExchange(&g_player.combat.damageOut, 0, 0);
	msg.damage_in = InterlockedCompareExchange(&g_player.combat.damageIn, 0, 0);
	msg.heal_out = InterlockedCompareExchange(&g_player.combat.healOut, 0, 0);

	utf16_to_utf8(player->name, msg.name, sizeof(player->name));

	for (int i=0; i<ARRAYSIZE(g_closest_names); i++)
		utf16_to_utf8(g_closest_names[i], msg.closest[i].name, sizeof(msg.closest[i].name));

	// Figure out the most recent targets.
	get_recent_targets(target, char_array, msg.targets, MSG_TARGETS, NULL, now);

	// Send the message.
	network_send(&msg, sizeof(msg));
}

// Compare buff player
static int __cdecl ClosePlayerCompare(void* pCtx, void const* pItem1, void const* pItem2)
{
	i32 retVal = 0;
	u32 sortBy = LOBYTE(*(u32*)pCtx);
	bool isAsc = HIBYTE(*(u32*)pCtx);
	const  ClosePlayer* p1 = pItem1;
	const  ClosePlayer* p2 = pItem2;

#define BUFF_PERCENT(x, y) ((u32)(x->cd.y / (f32)(x->cd.duration) * 100.0f))
#define BUFF_COMPARE(x) \
		if (p1->cd.duration == 0 && p2->cd.duration != 0) retVal = -1; \
		else if (p2->cd.duration == 0 && p1->cd.duration != 0) retVal = 1; \
		else if (BUFF_PERCENT(p1, x) < BUFF_PERCENT(p2, x)) retVal = -1; \
		else if (BUFF_PERCENT(p1, x) > BUFF_PERCENT(p2, x)) retVal = 1; \
		else retVal = 0;

#define BUFF_COMPARE_NOPCT(x) \
		if (p1->cd.duration == 0 && p2->cd.duration != 0) retVal = -1; \
		else if (p2->cd.duration == 0 && p1->cd.duration != 0) retVal = 1; \
		else if (p1->cd.x < p2->cd.x) retVal = -1; \
		else if (p1->cd.x > p2->cd.x) retVal = 1; \
		else retVal = 0;


	const char * profName1 = name_by_profession(p1->c.profession);
	const char * profName2 = name_by_profession(p2->c.profession);

	switch (sortBy)
	{
	case(BUFF_COL_NAME):
		if (!p1->name && p2->name)
			retVal = -1;
		else if (!p2->name && p1->name)
			retVal = 1;
		else if (!p1->name && !p2->name)
			retVal = 0;
		else
			retVal = wcscmp(p1->name, p2->name);
		break;
	case(BUFF_COL_CLS):
		retVal = strcmp(profName1, profName2);
		break;
	case(BUFF_COL_SUB):
		if (p1->subGroup <= 0 && p2->subGroup > 0) retVal = -1;
		else if (p2->subGroup <= 0 && p1->subGroup > 0) retVal = 1;
		else if (p1->subGroup < p2->subGroup) retVal = -1;
		else if (p1->subGroup > p2->subGroup) retVal = 1;
		else retVal = 0;
		break;
	case(BUFF_COL_DOWN):
		if (p1->cd.duration == 0 && p2->cd.duration != 0) retVal = -1;
		else if (p2->cd.duration == 0 && p1->cd.duration != 0) retVal = 1;
		else if (p1->cd.noDowned < p2->cd.noDowned) retVal = -1;
		else if (p1->cd.noDowned > p2->cd.noDowned) retVal = 1;
		else retVal = 0;
		break;
	case(BUFF_COL_SCLR):
		BUFF_COMPARE(sclr_dura);
		break;
	case(BUFF_COL_SEAW):
		BUFF_COMPARE(seaw_dura);
		break;
	case(BUFF_COL_PROT):
		BUFF_COMPARE(prot_dura);
		break;
	case(BUFF_COL_QUIK):
		BUFF_COMPARE(quik_dura);
		break;
	case(BUFF_COL_ALAC):
		BUFF_COMPARE(alac_dura);
		break;
	case(BUFF_COL_FURY):
		BUFF_COMPARE(fury_dura);
		break;
	case(BUFF_COL_MIGHT):
		BUFF_COMPARE_NOPCT(might_avg);
		break;
	case(BUFF_COL_GOTL):
		BUFF_COMPARE_NOPCT(gotl_avg);
		break;
	case(BUFF_COL_GLEM):
		BUFF_COMPARE(glem_dura);
		break;
	case(BUFF_COL_VIGOR):
		BUFF_COMPARE(vigor_dura);
		break;
	case(BUFF_COL_SWIFT):
		BUFF_COMPARE(swift_dura);
		break;
	case(BUFF_COL_STAB):
		BUFF_COMPARE(stab_dura);
		break;
	case(BUFF_COL_RETAL):
		BUFF_COMPARE(retal_dura);
		break;
	case(BUFF_COL_RESIST):
		BUFF_COMPARE(resist_dura);
		break;
	case(BUFF_COL_REGEN):
		BUFF_COMPARE(regen_dura);
		break;
	case(BUFF_COL_AEGIS):
		BUFF_COMPARE(aegis_dura);
		break;
	case(BUFF_COL_BANS):
		BUFF_COMPARE(bans_dura);
		break;
	case(BUFF_COL_BAND):
		BUFF_COMPARE(band_dura);
		break;
	case(BUFF_COL_BANT):
		BUFF_COMPARE(bant_dura);
		break;
	case(BUFF_COL_EA):
		BUFF_COMPARE(warEA_dura);
		break;
	case(BUFF_COL_SPOTTER):
		BUFF_COMPARE(spot_dura);
		break;
	case(BUFF_COL_FROST):
		BUFF_COMPARE(frost_dura);
		break;
	case(BUFF_COL_SUN):
		BUFF_COMPARE(sun_dura);
		break;
	case(BUFF_COL_STORM):
		BUFF_COMPARE(storm_dura);
		break;
	case(BUFF_COL_STONE):
		BUFF_COMPARE(stone_dura);
		break;
	case(BUFF_COL_ENGPP):
		BUFF_COMPARE(engPPD_dura);
		break;
	case(BUFF_COL_REVNR):
		BUFF_COMPARE(revNR_dura);
		break;
	case(BUFF_COL_REVAP):
		BUFF_COMPARE(revAP_dura);
		break;
	case(BUFF_COL_NECVA):
		BUFF_COMPARE(necVA_dura);
		break;
	case(BUFF_COL_TIME):
	{
		u32 p1time = (u32)(p1->cd.duration / 1000.0f);
		u32 p2time = (u32)(p2->cd.duration / 1000.0f);
		if (p1time == 0 && p2time != 0) retVal = -1;
		else if (p2time == 0 && p1time != 0) retVal = 1;
		else if (p1time < p2time) retVal = -1;
		else if (p1time > p2time) retVal = 1;
		else retVal = 0;
	}
		break;
	};

	if (isAsc)
		return retVal;
	else
		return -(retVal);
}

// Group DPS module.
static void module_buff_uptime(Character* player, Array* char_array, i64 now, i32 gui_x, i32 gui_y)
{

	static ClosePlayer temp_players[MSG_CLOSEST_PLAYERS + 1];
	u32 total = 0;

	// Reset our player pool
	memset(temp_players, 0, sizeof(temp_players));

	// Add current player to our temp array
	temp_players[total].c = g_player.c;
	temp_players[total].name = &g_player.c.name[0];
	memcpy(&temp_players[total].cd, &g_player.combat, sizeof(temp_players[total].cd));
	++total;

	// Add closest player data
	for (u32 i = 0; i < g_closest_num; ++i)
	{
		temp_players[total].c = g_closest_players[i].c;
		temp_players[total].name = &g_closest_players[i].name[0];
		memcpy(&temp_players[total].cd, &g_closest_players[i].cd, sizeof(temp_players[total].cd));
		++total;
	}


	// Get squad subgroup
	for (u32 i = 0; i < total; ++i) {
		temp_players[i].subGroup = Pl_GetSquadSubgroup(g_ptrs.squadContext, temp_players[i].c.ctptr);
	}

	// Default sort: alphabetically (exclude self)
	u32 sortBy = 0;
	if (g_state.panel_buff_uptime.cfg.sortCol == BUFF_COL_NAME &&
		g_state.panel_buff_uptime.cfg.sortColLast == BUFF_COL_END &&
		g_state.panel_buff_uptime.cfg.asc == 1)
	{
		sortBy = MAKEWORD(g_state.panel_buff_uptime.cfg.sortCol, g_state.panel_buff_uptime.cfg.asc);
		qsort_s(&temp_players[1], total - 1, sizeof(temp_players[0]), ClosePlayerCompare, &sortBy);
	}
	else
	{
		sortBy = MAKEWORD(g_state.panel_buff_uptime.cfg.sortCol, g_state.panel_buff_uptime.cfg.asc);
		qsort_s(&temp_players[0], total, sizeof(temp_players[0]), ClosePlayerCompare, &sortBy);
	}


	if (g_state.panel_buff_uptime.enabled) {
		ImGui_BuffUptime(&g_state.panel_buff_uptime, gui_x, gui_y, temp_players, total);
	}
}

void mumble_link_create(void)
{
	// Open the mumble link.
	g_mum_link = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, TEXT(MUMBLE_LINK));
	if (g_mum_link == 0)
	{
		g_mum_link = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, sizeof(LinkedMem), TEXT(MUMBLE_LINK));
	}

	if (g_mum_link)
	{
		g_mum_mem = (LinkedMem*)MapViewOfFile(g_mum_link, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(LinkedMem));
	}
}

void mumble_link_destroy(void)
{
	if (g_mum_mem)
	{
		UnmapViewOfFile((LPVOID)g_mum_mem);
		g_mum_mem = NULL;
	}

	if (g_mum_link)
	{
		CloseHandle(g_mum_link);
		g_mum_link = NULL;
	}
}

u32 buildId(void)
{
	if (g_build_id > 0 && g_build_id < 200000) return g_build_id;
	if (g_mum_mem) return g_mum_mem->build_id;
	return 0;
}

void dps_create(void)
{
	memset(&g_ptrs, 0, sizeof(g_ptrs));

	if (g_state.log_dir == 0)
	{
		return;
	}

	// Allocate memory for DPS resources.
	g_targets = calloc(MAX_TARGETS, sizeof(*g_targets));
	if (g_targets == 0)
	{
		return;
	}

	g_closest_pap = calloc(MAX_PLAYERS, sizeof(*g_closest_pap));
	if (g_closest_pap == 0)
	{
		return;
	}

	// Create the logging directory.
	CreateDirectoryA(g_state.log_dir, 0);

	// Get the agent view context.
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (snap)
	{
		THREADENTRY32 te;
		te.dwSize = sizeof(te);

		for (BOOL res = Thread32First(snap, &te); res; res = Thread32Next(snap, &te))
		{
			if (te.th32OwnerProcessID != process_get_id())
			{
				continue;
			}

			HANDLE thread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
			if (thread == 0)
			{
				continue;
			}

			struct
			{
				NTSTATUS ExitStatus;
				PVOID TebBaseAddress;
				struct
				{
					HANDLE UniqueProcess;
					HANDLE UniqueThread;
				} ClientId;
				KAFFINITY AffinityMask;
				LONG Priority;
				LONG BasePriority;
			} tbi;

			if (NtQueryInformationThread(thread, (THREADINFOCLASS)0, &tbi, sizeof(tbi), 0) == 0)
			{
				u64 tls = process_read_u64((u64)tbi.TebBaseAddress + 0x58);
				u64 main = process_read_u64(tls);
				u64 slot = process_read_u64(main + OFF_TLS_SLOT);
				u64 ctx = process_read_u64(slot + OFF_CTX_CCTX);

				if (ctx)
				{
					g_ptrs.pTlsCtx = slot;
					g_ptrs.pCharCliCtx = ctx;
					break;
				}
			}

			CloseHandle(thread);
		}

		CloseHandle(snap);
	}

#if !(defined BGDM_TOS_COMPLIANT)
	// "Gw2China"
	// 47 00 77 00 32 00 43 00 68 00 69 00 6E 00 61 00  G.w.2.C.h.i.n.a.   <- didn't work, we search for the CTX instead
	// B8 6F 00 00 00 C3 CC CC CC CC CC CC CC CC CC CC 48 8D 05 ?? ?? ?? ?? C3
	u64 gw2china = process_scan(
		"\xB8\x6F\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x8D\x05\x00\x00\x00\x00\xC3",
		"xxxxxxxxxxxxxxxxxxx????x");
	if (gw2china > 0) g_state.is_gw2china = true;
#endif

	// Get the build ID
	// L"Build %u"
	// B9 01 00 00 00 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 44 8B C0
	u64 build_ptr = process_follow(
		"\xB9\x01\x00\x00\x00\xE8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x44\x8B\xC0",
		"xxxxxx????x????xxx", 0xB);

	if (build_ptr)
		g_build_id = process_read_u32(build_ptr + 0x1);

	DBGPRINT(TEXT("<GW2_buildId=%d>"), g_build_id);


	// Get the Agent context
	// "ViewAdvanceAgentSelect"
	// 48 83 C4 40 5E C3 CC CC CC CC CC CC CC CC 48 8D 05 ?? ?? ?? ?? C3
	g_ptrs.pAgentSelectionCtx = process_follow(
		"\x48\x83\xC4\x40\x5E\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x8D\x05\x00\x00\x00\x00\xC3",
		"xxxxxxxxxxxxxxxxx????x", 17);

	// Get the AgentView context
	// "ViewAdvanceAgentView"
	// 48 83 C4 40 5F C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC 48 8D 05 ?? ?? ?? ?? C3
	g_ptrs.pAgentViewCtx = process_follow(
		"\x48\x83\xC4\x40\x5F\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x8D\x05\x00\x00\x00\x00\xC3",
		"xxxxxxxxxxxxxxxxxxxxxxxx????x", 24);

	// Get the squadContext
	// search for "squadCliContext" RETx3 and look for ref to [RSI+480]
	// rsi is the ctx, find ref to RSI and you get the function that returns it
	// 48 8D 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 8B 81 ?? ?? ?? ?? 48 85 C0 74 05
	// LOL -> Turned out to be AgentView ctx ^^


	// Get the UI context.
	// "ViewAdvanceUi"
	// 48 8D 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 83 EC 28 83 F9 0B
	// -- NOT NEEDED: Searcg for "m_curScale > -10" scroll up 1 whole function till you see the context LEA
	// -- reference hit #7 for search for ctx LEA should return a function with both
	// -- "!((unsigned_ptr)this & 3)" & "!TermCheck(ptr)" x2
	g_ptrs.pUiCtx = process_follow(
		"\x48\x8D\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x83\xEC\x28\x83\xF9\x0B",
		"xxx????xxxxxxxxxxxxxxxx", 3);

	// Get the UI hiding offset.
	// 8B 15 ?? ?? ?? ?? 45 33 C9 85 D2 45 8B C1 41 0F 99 C0
	g_ptrs.pIfHide = process_follow(
		"\x8B\x15\x00\x00\x00\x00\x45\x33\xC9\x85\xD2\x45\x8B\xC1\x41\x0F\x99\xC0",
		"xx????xxxxxxxxxxxx", 2);

	// For map/ping/fps search for:
	// 'CameraSettings()->GetDebugControllerType()'
	// Get the map open offset
	// 74 0A B8 11 00 00 00
	g_ptrs.pMapOpen = process_follow(
		"\x74\x0A\xB8\x11\x00\x00\x00",
		"xxxxxxx", 0xE);
	if (g_ptrs.pMapOpen)
		g_ptrs.pMapOpen++;


	// Get the ping context
	// CC 33 C0 4C 8D 0D ?? ?? ?? ?? 4C 8B DA 45 33 C0
	g_ptrs.pPing = process_follow(
		"\xCC\x33\xC0\x4C\x8D\x0D\x00\x00\x00\x00\x4C\x8B\xDA\x45\x33\xC0",
		"xxxxxx????xxxxxx", 0x6);

	// Get the fps context
	// CC 83 0D ?? ?? ?? ?? 20 89 0D ?? ?? ?? ?? C3 CC
	g_ptrs.pFps = process_follow(
		"\xCC\x83\x0D\x00\x00\x00\x00\x20\x89\x0D\x00\x00\x00\x00\xC3\xCC",
		"xxx????xxx????xx", 0xA);

	// Get the WorldView context
	// Search for ViewAdvanceWorldView
	// E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 41 0F 28 D0 0F 28 CE
	u64 p_worldview = process_follow(
		"\xE8\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x41\x0F\x28\xD0\x0F\x28\xCE",
		"x????xxx????x????x????xxxxxxx", 0x12);

	if (p_worldview)
		g_ptrs.pWorldView = process_follow_rel_addr(p_worldview + 0x7);


	// Action cam ctx
	// search for "!s_interactiveContext->filePath[0]"
	// go to 3rd call from start of function and then roughly 3rd call again
	// 8D 50 06 44 8D 40 04
	g_ptrs.pActionCam = process_follow(
		"\x8D\x50\x06\x44\x8D\x40\x04",
		"xxxxxxx", -0x4);

	// Get wmAgent from agent ptr
	// search for "guildTagLogoFrame" and go the 2nd "agent" assert going up
	// first call after that is GetWmAgentFromAgent
	// 49 8B CF E8 ?? ?? ?? ?? 48 85 C0 74 1C 48 8B 10 48 8B C8 FF 52 70 48 85 C0
	g_ptrs.pfcGetWmAgent = process_follow(
		"\x49\x8B\xCF\xE8\x00\x00\x00\x00\x48\x85\xC0\x74\x1C\x48\x8B\x10\x48\x8B\xC8\xFF\x52\x70\x48\x85\xC0",
		"xxxx????xxxxxxxxxxxxxxxxx", 0x4);

	// Get contact ctx function
	// Search for "..\\..\\..\\Game\\Ui\\Widgets\\AgentStatus\\AsInfoBarFloating.cpp"
	// follow down a bit until you see a call with retval deref to r8 and called at VT+0x238 with RDX = pptr RCX = cptr
	// C3 CC CC CC CC CC 48 83 EC 28 E8 ?? ?? ?? ?? 48 8B 80 ?? ?? 00 00 48 83 C4 28 C3 CC CC CC CC CC CC CC CC CC CC CC 40 57
	g_ptrs.pfcGetContactCtx = process_scan(
		"\xC3\xCC\xCC\xCC\xCC\xCC\x48\x83\xEC\x28\xE8\x00\x00\x00\x00\x48\x8B\x80\x00\x00\x00\x00\x48\x83\xC4\x28\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x40\x57",
		"xxxxxxxxxxx????xxx??xxxxxxxxxxxxxxxxxxxx");

	if (g_ptrs.pfcGetContactCtx)
		g_ptrs.pfcGetContactCtx += 6;

	// Get squad context function
	// Search for "squadContext" and look at the first call up
	// 83 A7 ?? 00 00 00 EF E8 ?? ?? ?? ?? 85 C0 0F 84 ?? ?? ?? ?? E8
	g_ptrs.pfcGetSquadCtx = process_follow(
		"\x83\xA7\x00\x00\x00\x00\xEF\xE8\x00\x00\x00\x00\x85\xC0\x0F\x84\x00\x00\x00\x00\xE8",
		"xx?xxxxx????xxxx????x", 21);

	// Search for:
	// "!buffer->Count() || (CParser::Validate(buffer->Ptr(), buffer->Term(), true ) == buffer->Term())"
	// The function that has "codedString" and "codedString[0]" below the assert
	// The function above it is what we need, it will have the below assert:
	// "CParser::Validate(buffer->Ptr(), buffer->Term(), true ) == buffer->Term()"
	// 53 57 48 83 EC 48 8B D9 E8 ?? ?? ?? ?? 48 8B 48 50 E8 ?? ?? ?? ?? 44 8B 4C 24 68 48 8D 4C 24 30 48 8B F8
	// - 0xE
	g_ptrs.pfcCodedTextFromHashId = process_scan(
		"\x53\x57\x48\x83\xEC\x48\x8B\xD9\xE8\x00\x00\x00\x00\x48\x8B\x48\x50\xE8\x00\x00\x00\x00\x44\x8B\x4C\x24\x68\x48\x8D\x4C\x24\x30\x48\x8B\xF8",
		"xxxxxxxxx????xxxxx????xxxxxxxxxxxxx"
	);

	if (g_ptrs.pfcCodedTextFromHashId)
		g_ptrs.pfcCodedTextFromHashId -= 0xE;

	// Search for "resultFunc"
	// E8 ?? ?? ?? ?? FF C6 83 FE 03 72
	g_ptrs.pfcDecodeCodedText = process_follow(
		"\xE8\x00\x00\x00\x00\xFF\xC6\x83\xFE\x03\x72",
		"x????xxxxxx", 1);

	// Combat Log
	// search for "logType < UI_COMBAT_LOG_TYPES"
	// CC 48 89 5C 24 08 57 48 83 EC 20 41 8B F8 48 8B DA 41 83 F8 0E
	u64 p_cbt_log = process_scan(
		"\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x20\x41\x8B\xF8\x48\x8B\xDA\x41\x83\xF8\x0E",
		"xxxxxxxxxxxxxxxxxxxx");

	// Dmage processing function.
	// Search for assert "type < AGENT_STATUS_COMBAT_EVENT_TYPES"
	u64 p_dmg_result = process_scan(
		"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x60\x4C\x63\xFA",
		"xxxx?xxxx?xxxx?xxxxxxxxxxxxxxxx");

	if (p_cbt_log)
	{
		if (MH_CreateHook((LPVOID)p_cbt_log, (LPVOID)&hkCombatLog, (LPVOID*)&_imp_hkCombatLog) == MH_OK)
		{
			if (MH_EnableHook((LPVOID)p_cbt_log) == MH_OK) {

				DBGPRINT(TEXT("combat log hook created [p_cbt_log=0x%016llX]"), p_cbt_log);
			}
			g_ptrs.cbCombatLog = p_cbt_log;
		}
		else
		{
			DBGPRINT(TEXT("Failed to create combat log hook [p_cbt_log=0x%016llX]"), p_cbt_log);
		}
	}

	if (p_dmg_result)
	{
		if (MH_CreateHook((LPVOID)p_dmg_result, (LPVOID)&hkDamageResult, (LPVOID*)&_imp_hkDamageResult) == MH_OK)
		{
			if (MH_EnableHook((LPVOID)p_dmg_result) == MH_OK)
			{
				DBGPRINT(TEXT("Damage result hook created [p_dmg_result=0x%016llX]"), p_dmg_result);

			}

			g_ptrs.cbDmgLogResult = p_dmg_result;
		}
		else
		{
			DBGPRINT(TEXT("Failed to create damage result hook [p_dmg_result=0x%016llX]"), p_dmg_result);
		}
	}

	// Game thread context
	// "ViewAdvanceDevice" -> dereference the ptr it's pointing too and hook VT index 0
	// Problem with the above method is that the context is set after DirectX startup
	// and hence our startup (ViewAdvanceDevice ctx will be 0 at that point)
	// safer to find the VT in memory and hook the hardcoded address
	// can also be simply done by searching for "!m_fileLocks.Count()"
	// "48 89 5C 24 08 57 48 83 EC 20 83 B9 14 03 00 00 00"
	u64 p_game_thread = process_scan(
		"\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x20\x83\xB9\x14\x03\x00\x00\x00",
		"xxxxxxxxxxxxxxxxx");

	if (p_game_thread)
	{
		if (MH_CreateHook((LPVOID)p_game_thread, (LPVOID)&hkGameThread, (LPVOID*)&_imp_hkGameThread) == MH_OK)
		{
			if (MH_EnableHook((LPVOID)p_game_thread) == MH_OK) {
				DBGPRINT(TEXT("Game thread hook created [p_game_thread=0x%016llX]"), p_game_thread);
			}

			g_ptrs.pGameThread = p_game_thread;
		}
		else
		{
			DBGPRINT(TEXT("Failed to create game thread hook [p_game_thread=0x%016llX]"), p_game_thread);
		}
	}

	// NOTE: Can also replace the WndPRoc with SetWindowLogPtr() (instead of hooking)
	// However this will cause crashes with other mods that also use SetWindowLongPtr()
	// (i.e Arcdps), therefore we are now using a hook for WndProc (see below)
	/*if (g_hWnd)
	{
		//s_oldWindProc = GetWindowLongPtr(g_hWnd, GWLP_WNDPROC);
		//DBGPRINT(TEXT("GetWindowLongPtr(%p) = %p"), g_hWnd, s_oldWindProc);
		s_oldWindProc = SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
		DBGPRINT(TEXT("WindowProc hook created [g_hWnd=%p] [s_oldWindProc=%p]"), g_hWnd, s_oldWindProc);
	}*/

	// WndProc hook
	// CPU Window -> Search For -> All Modules -> Intermodular Calls
	// Search for "gw2-64.&ntdllDefWindowProc_W" (1st hit)
	// scroll up to start of function it should have calls to GetWindowLongPtrA/SetWindowLongPtrA
	// 40 53 55 56 57 41 54 41 55 41 57 48 83 EC 50 48 8B 05 ?? ?? ?? ?? 48 33 C4
	u64 pWndProc = process_scan(
		"\x40\x53\x55\x56\x57\x41\x54\x41\x55\x41\x57\x48\x83\xEC\x50\x48\x8B\x05\x00\x00\x00\x00\x48\x33\xC4",
		"xxxxxxxxxxxxxxxxxx????xxx");
	if (pWndProc)
	{
		if (MH_CreateHook((LPVOID)pWndProc, (LPVOID)&WndProc, (LPVOID*)&_imp_wndProc) == MH_OK)
		{
			if (MH_EnableHook((LPVOID)pWndProc) == MH_OK)
			{
				DBGPRINT(TEXT("WndProc hook created [p_wndProc=0x%016llX]"), pWndProc);
			}

			g_ptrs.pWndProc = pWndProc;
		}
		else
		{
			DBGPRINT(TEXT("Failed to hook WndProc [p_wndProc=0x%016llX]"), pWndProc);
		}
	}

	g_ptrs.pCam = (uintptr_t)&g_cam_data;
	g_ptrs.pMumble = (uintptr_t)g_mum_mem;
	g_ptrs.pShardId = (uintptr_t)&g_mum_mem->shard_id;
	g_ptrs.pMapId = (uintptr_t)&g_mum_mem->map_id;
	g_ptrs.pMapType = (uintptr_t)&g_mum_mem->map_type;
	g_is_init = true;
}

void dps_destroy(void)
{
	// NOT NEEDED, we are now hooking WndProc directly
	// so this essentialy does nothing (s_oldWindProc is never assigned)
	/*if (s_oldWindProc) {
		SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)s_oldWindProc);
		DBGPRINT(TEXT("WindowProc hook removed [g_hWnd=%p] [s_oldWindProc=%p]"), g_hWnd, s_oldWindProc);
		s_oldWindProc = 0;
	}*/

	if (g_ptrs.pWndProc)
	{
		MH_RemoveHook((LPVOID)g_ptrs.pWndProc);
	}

	if (g_ptrs.pGameThread)
	{
		MH_RemoveHook((LPVOID)g_ptrs.pGameThread);
	}

	if (g_ptrs.cbCombatLog)
	{
		MH_RemoveHook((LPVOID)g_ptrs.cbCombatLog);
	}

	if (g_ptrs.cbDmgLogResult)
	{
		MH_RemoveHook((LPVOID)g_ptrs.cbDmgLogResult);
	}

	free(g_closest_pap);
	free(g_targets);
}

void dps_handle_msg_damage_utf8(MsgServerDamageUTF8* msg)
{
	g_group_id = msg->selected_id;
	g_group_num = 0;
	memset(g_group, 0, sizeof(g_group));

	for (u32 i = 0; i < MSG_CLOSEST_PLAYERS; ++i)
	{
		ServerPlayerUTF8* s = msg->closest + i;

		s->name[MSG_NAME_SIZE_UTF8-1] = 0;
		if (s->name[0] == 0)
		{
			continue;
		}

		DPSPlayer* p = g_group + g_group_num;
		p->in_combat = s->in_combat;
		p->time = s->time;
		p->damage = s->damage;
		p->damage_in = s->damage_in;
		p->heal_out = s->heal_out;
		p->stats = s->stats;
		p->profession = s->profession;

		utf8_to_utf16(s->name, p->name, sizeof(p->name));

		memcpy(p->targets, s->targets, sizeof(p->targets));

		for (u32 j = 0; j < MSG_TARGETS; ++j)
		{
			// Since we hijacked the old c1dmg only value of '1' is valid
			if (p->targets[j].invuln != 1) {
				continue;
			}

			u32 id = p->targets[j].id;
			DBGPRINT(TEXT("[%s] reported target [id:%d] is invulnerable"), p->name, id);

			for (u32 x = 0; x < MAX_TARGETS; ++x)
			{
				DPSTarget* t = &g_targets[x];
				if (t->aptr == 0 || t->id == 0) {
					continue;
				}

				if (t->id == id && !t->invuln) {
					DBGPRINT(TEXT("[%d:%016I64X] is now invuln, source [%s]"), t->id, t->aptr, p->name);
					t->invuln = 1;
				}
			}

		}

		++g_group_num;
	}
}

static void default_panel_positions(GFXTarget set, uint32_t res_x, uint32_t res_y, uint32_t font_x, uint32_t font_y)
{
	uint32_t center = res_x / 2;
	if (g_state.panel_dps_self.pos.x < 0)
		g_state.panel_dps_self.pos.x = center + set.dps;
	if (g_state.panel_dps_self.pos.y < 0)
		g_state.panel_dps_self.pos.y = set.hp.y - font_y * 5;
	if (g_state.panel_dps_group.pos.x < 0)
		g_state.panel_dps_group.pos.x = center + set.dps + (u32)(42.0f*font_x);
	if (g_state.panel_dps_group.pos.y < 0)
		g_state.panel_dps_group.pos.y = set.hp.y - font_y * 5;
	if (g_state.panel_skills.pos.x < 0)
		g_state.panel_skills.pos.x = g_state.panel_dps_group.pos.x;
	if (g_state.panel_skills.pos.y < 0)
		g_state.panel_skills.pos.y = g_state.panel_dps_group.pos.y + (u32)(font_y*(MSG_CLOSEST_PLAYERS + 28));
	if (g_state.panel_buff_uptime.pos.x < 0)
		g_state.panel_buff_uptime.pos.x = g_state.panel_dps_group.pos.x;
	if (g_state.panel_buff_uptime.pos.y < 0)
		g_state.panel_buff_uptime.pos.y = g_state.panel_dps_group.pos.y + (u32)(font_y*(MSG_CLOSEST_PLAYERS + 17));
	if (g_state.panel_gear_target.pos.x < 0)
		g_state.panel_gear_target.pos.x = res_x - 400;
	if (g_state.panel_gear_target.pos.y < 0)
		g_state.panel_gear_target.pos.y = res_y - 800;
	if (g_state.panel_gear_self.pos.x < 0)
		g_state.panel_gear_self.pos.x = res_x - 400;
	if (g_state.panel_gear_self.pos.y < 0)
		g_state.panel_gear_self.pos.y = g_state.panel_gear_target.pos.y + (u32)(31.5f*font_y);
}

void dps_update(i64 now)
{
	if (g_is_init == false)
	{
		return;
	}

	if (!g_state.global_on_off)
		return;

	// Check if the UI is hidden.
	bool is_visible = (process_read_i32(g_ptrs.pIfHide) == 0) && (process_read_i32(g_ptrs.pMapOpen) == 0) && g_cam_data.valid;

	// Update UI variables.
	uint32_t res_x, res_y;
	uint32_t font_x, font_y;
	ImGui_GetViewportRes(&res_x, &res_y);
	ImGui_GetFontSize(&font_x, &font_y);
	uint32_t center = res_x / 2;

	// Read the ui scale.
	i32 ui_scale = process_read_i32(g_ptrs.pUiCtx + OFF_UCTX_SCALE);
	ui_scale = ui_scale > 3 ? 3 : ui_scale;
	ui_scale = ui_scale < 0 ? 0 : ui_scale;

	// Override for the chinese client
	// since they have only small/normal
	if (g_state.is_gw2china && ui_scale > 0) ui_scale = 2;

	// Set up the UI placement.
	GFX* ui = g_gfx + ui_scale;
	GFXTarget set;

	// check for user toggle and 
	// draw the compass if need be
	if (is_visible) 
		module_compass(center, res_y - ui->hp - 120, 320);

	// Get the character and player arrays.
	Array char_array, play_array;
	process_read(g_ptrs.pCharCliCtx + OFF_CCTX_CHARS, &char_array, sizeof(char_array));
	process_read(g_ptrs.pCharCliCtx + OFF_CCTX_PLAYS, &play_array, sizeof(play_array));

	// Read the player array
	if (play_array.data &&
		play_array.cur &&
		play_array.cur <= play_array.max)
	{
		u32 pan = min(play_array.cur, MAX_PLAYERS);
		process_read(play_array.data, g_closest_pap, pan * 8);
	}

	// Get the player and the current target data.
	Character player = { 0 };
	Character target = { 0 };

	u64 player_cptr = process_read_u64(g_ptrs.pCharCliCtx + OFF_CCTX_CONTROLLED_CHAR);
	u64 player_pptr = process_read_u64(g_ptrs.pCharCliCtx + OFF_CCTX_CONTROLLED_PLAYER);

	if (player_cptr == 0 || player_pptr == 0)
	{
		module_debug(&player, &target, &char_array, &play_array, now);
		return;
	}

	read_player(&player, player_cptr, player_pptr, now);
	read_target(&target, &char_array, &play_array, process_read_u64(g_ptrs.pAgentSelectionCtx + OFF_ACCTX_AGENT), now);
	read_closest(&player, &play_array, now);

	// Cleanup the target list.
	dps_targets_update(&player, &char_array, now);

	if (target.type == AGENT_TYPE_ATTACK)
	{
		set = ui->attack;
	}
	else if (target.type == AGENT_TYPE_GADGET)
	{
		set = ui->gadget;
	}
	else
	{
		set = ui->normal;
	}

	if (g_mum_mem->map_type >= MAP_TYPE_WVW_EB && g_mum_mem->map_type <= MAP_TYPE_WVW_EOTM2)
	{
		static const i32 wvw_offset[4] = { 27, 30, 33, 36 };;
		set.hp.y += wvw_offset[ui_scale];
		set.bb.y += wvw_offset[ui_scale];
	}
	if ((target.type == AGENT_TYPE_GADGET || target.type == AGENT_TYPE_ATTACK) && ui_scale < 3)
		set.hp.y -= 1;

	// default screen locations
	// only matters when the windows is first opened
	// and doesn't have an ImGui location saved
	// probably needs to be removed at one point
	default_panel_positions(set, res_x, res_y, font_x, font_y);

	// Draw player and target information.
	if (is_visible)
	{
		// Player HP %, target HP/breakbar & dist
#if !(defined BGDM_TOS_COMPLIANT)
		ImGui_FloatBars(&g_state.panel_float, &player, &target, center, ui, &set);
#endif
		ImGui_HpAndDistance(&g_state.panel_hp, &player, &target, center, ui, &set);

		// Update and draw DPS.
		module_dps_solo(&target, &char_array, now, g_state.panel_dps_self.pos.x, g_state.panel_dps_self.pos.y);
		module_dps_group(&player, &target, &char_array, now, g_state.panel_dps_group.pos.x, g_state.panel_dps_group.pos.y);
		module_dps_skills(&player, &target, &char_array, now, g_state.panel_skills.pos.x, g_state.panel_skills.pos.y);

		// Draw Buff uptime panel
		module_buff_uptime(&player, &char_array, now, g_state.panel_buff_uptime.pos.x, g_state.panel_buff_uptime.pos.y);

#if !(defined BGDM_TOS_COMPLIANT)
		// Update and draw inventory
		module_char_inspect(&target, &player, g_state.use_localized_text);
#endif
	}

	// Send out network data.
	module_network_dps(&player, &target, &char_array, now);

	// Draw debug information.
	module_debug(&player, &target, &char_array, &play_array, now);
}
