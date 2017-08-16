#include "core/debug.h"
#include "core/time.h"
#include "meter/game.h"
#include "meter/dps.h"
#include "meter/logging.h"
#include "meter/process.h"
#include "meter/hooks.h"
#include "meter/lru_cache.h"
#include "meter/imgui_bgdm.h"
#include "meter/ForeignFncs.h"
#include <intrin.h>		// _AddressOfReturnAddress


WndProc_t _imp_wndProc = 0;
hkGameThread_t _imp_hkGameThread = 0;
hkCombatLog_t _imp_hkCombatLog = 0;
hkCbtLogResult_t _imp_hkCbtLogResult = 0;
hkDamageResult_t _imp_hkDamageResult = 0;

extern __int64 __readRBX();

// Intercepts game client alerts
// Here we call everything that requires being called from main thread
// There functions below use the TLS (thread-local-storage) which crashes
// unless being called form the owner thread, if you're calling a function
// and crashing and you're not sure if you have the right signature, try 
// first calling it from here, if it still crahshes you got a different
// issue on your hands, GL :-)
void __fastcall hkGameThread(uintptr_t pInst, int a2, int frame_time)
{

	static volatile LONG m_lock = 0;

	// don't think thread safety is needed here but being
	// extra safe never hurt anyone and this really is
	// close to zero overhead so why not
	while (InterlockedCompareExchange(&m_lock, 1, 0)) {}

	// Update the camera data
	dps_update_cam_data();
	//if (dps_get_cam_data()->valid) {}

	// Get the contact ctx
	g_ptrs.squadContext = GetCtxSquad(g_ptrs.pfcGetSquadCtx);
	g_ptrs.contactContext = GetCtxContact(g_ptrs.pfcGetContactCtx);

	// Get the account name if we don't have it already
	// we get the account name from our own contactDef
	// which we can get from the contact context
	if (g_player.acctName[0] == 0) {
		const wchar_t *accountName = Contact_GetAccountName(CtxContact_GetContactSelf(g_ptrs.contactContext));
		if (accountName) {
			accountName++; // skip the ':'
			size_t len = wcslen(accountName);
			if (len > 0) {
				wcsncpy(g_player.acctName, accountName, len);
				g_player.acctName[ARRAYSIZE(g_player.acctName) - 1] = 0;
				DBGPRINT(TEXT("AccountName: %s"), g_player.acctName);
			}
		}
	}

	// Get missing speciesDef for targets
	// Can only be called from main thread
	dps_targets_get_species();

	// Send missing IDs/cptr's for resolving
	// Can only be done under main thread context
	lru_resolve();

	InterlockedExchange(&m_lock, 0);

	if (_imp_hkGameThread)
	{
		_imp_hkGameThread(pInst, a2, frame_time);
	}
}

// Intercepts damage result
// We onlu yse this to detect invuln phases
i64 __fastcall hkDamageResult(i64 a1, i32 type, u32 dmg, u32 a4, void* a5, i64 a6, i64 a7, u64 target)
{
	static const i32 DMG_RESULT_BLOCK = 0;
	static const i32 DMG_RESULT_INVULN_0 = 18;		// '0' dmg type of invuln
	static const i32 DMG_RESULT_INTERRUPTED = 21;	// SELF INTERRUPTED
	static const i32 DMG_RESULT_INTERRUPT = 22;
	static const i32 DMG_RESULT_INVULN = 23;
	static const i32 DMG_RESULT_MISS = 31;
	static const i32 DMG_RESULT_OBSTRUCTED = 38;
	static const i32 DMG_RESULT_OUTOFRANGE = 40;

	Array char_array = { 0 };
	process_read(g_ptrs.pCharCliCtx + OFF_CCTX_CHARS, &char_array, sizeof(char_array));

	const wchar_t* name = lru_find(CLASS_TYPE_AGENT, target, NULL, 0);
	UNREFERENCED_PARAMETER(name);

	if (dmg == 0) {

		if (type == DMG_RESULT_INVULN ||
			type == DMG_RESULT_INVULN_0) {

			u32 ti = dps_targets_insert(target, 0, &char_array, time_get());

			if (ti < MAX_TARGETS)
			{
				DPSTarget* t = dps_targets_get(ti);
				t->invuln = true;
				DBGPRINT(TEXT("[%s] INVULNERABLE"), name);
			}
		}
#ifdef _DEBUG
		else {
			if (type == DMG_RESULT_INTERRUPT) {
				DBGPRINT(TEXT("[%s] INTERRUPT"), name);
			}
			else if (type == DMG_RESULT_INTERRUPTED) {
				DBGPRINT(TEXT("[%s] INTERRUPTED"), name);
			}
			else if (type == DMG_RESULT_MISS) {
				DBGPRINT(TEXT("[%s] MISS"), name);
			}
			else if (type == DMG_RESULT_BLOCK) {
				DBGPRINT(TEXT("[%s] BLOCK"), name);
			}
			else {
				DBGPRINT(TEXT("[%s] type:%d  dmg:%d  a4:%d a5:%p a6:%p a7:%p"), name, type, dmg, a4, a5, a6, a7);
			}
		}
#endif
	}


	if (_imp_hkDamageResult)
		return _imp_hkDamageResult(a1, type, dmg, a4, a5, a6, a7, target);

	return 0;
}

i64 __fastcall hkCbtLogResult(uintptr_t a1, uintptr_t a2)
{
	// hit can be found at RSP - 0x84
	uint32_t result = process_read_u32(a1 + 0x2c);
	uintptr_t aptr_tgt = process_read_u64(a1 + 0x58);

	Array char_array = { 0 };
	process_read(g_ptrs.pCharCliCtx + OFF_CCTX_CHARS, &char_array, sizeof(char_array));

	if (result != CBT_RESULT_NORMAL)
		DBGPRINT(TEXT("%p %d"), aptr_tgt, result);

	if (result == CBT_RESULT_ABSORB) {
		u32 ti = dps_targets_insert((u64)aptr_tgt, 0, &char_array, time_get());

		if (ti < MAX_TARGETS)
		{
			DPSTarget* t = dps_targets_get(ti);
			const wchar_t* name = lru_find(CLASS_TYPE_AGENT, t->aptr, NULL, 0);
			DBGPRINT(TEXT("[%s] INVULNERABLE"), name);
			UNREFERENCED_PARAMETER(name);
		}
	}

	if (_imp_hkCbtLogResult)
		return _imp_hkCbtLogResult(a1, a2);
	return 0;
}


void __fastcall hkCombatLogProcess(int type, int hit, u64 aptr_tgt, u64 aptr_src, u64 skillDef)
{
	static i8* const CombatLogTypeStr[] = {
		"CL_CONDI_DMG_IN",
		"CL_CRIT_DMG_IN",
		"CL_GLANCE_DMG_IN",
		"CL_HEAL_IN",
		"CL_PHYS_DMG_IN",
		"CL_UNKNOWN_5",
		"CL_UNKNOWN_6",
		"CL_CONDI_DMG_OUT",
		"CL_CRIT_DMG_OUT",
		"CL_GLANCE_DMG_OUT",
		"CL_HEAL_OUT",
		"CL_PHYS_DMG_OUT",
		"CL_UNKNOWN_12",
		"CL_UNKNOWN_13"
	};

#if (defined _DEBUG) || (defined DEBUG)
	i32 efType = process_read_i32((u64)skillDef + OFF_SKILLDEF_EFFECT);
	i32 hashId = process_read_i32((u64)skillDef + OFF_SKILLDEF_HASHID);
	DBGPRINT(TEXT("%S(%d) hit=%d [target:%p] [source:%p] [skillDef:%p] [efType:%d:%#x] [hashId:%d:%#x]"),
		CombatLogTypeStr[type], type, hit, aptr_tgt, aptr_src, skillDef, efType, efType, hashId, hashId);
#endif

	if (aptr_tgt == 0 || aptr_src == 0 || hit == 0)
		return;

	i64 now = time_get();
	Player* player = &g_player;
	Array char_array = { 0 };
	process_read(g_ptrs.pCharCliCtx + OFF_CCTX_CHARS, &char_array, sizeof(char_array));

	// If the player isn't in combat yet, enter combat
	if (InterlockedAnd(&player->combat.lastKnownCombatState, 1) == 0)
	{
		DBGPRINT(TEXT("UPDATE COMBAT STATE [%s]"), &player->c.name[0]);
		Character c = { 0 };
		c.aptr = aptr_src;
		c.cptr = process_read_u64(g_ptrs.pCharCliCtx + OFF_CCTX_CONTROLLED_CHAR);
		c.pptr = process_read_u64(g_ptrs.pCharCliCtx + OFF_CCTX_CONTROLLED_PLAYER);
		read_agent(&c, c.aptr, c.cptr, NULL, now);
		read_combat_data(&c, &player->combat, now);
	}

	if (type == CL_CONDI_DMG_OUT ||
		type == CL_CRIT_DMG_OUT ||
		type == CL_GLANCE_DMG_OUT ||
		type == CL_PHYS_DMG_OUT)
	{
		u32 ti = dps_targets_insert(aptr_tgt, 0, &char_array, now);

		if (ti < MAX_TARGETS)
		{
			DPSTarget* t = dps_targets_get(ti);
			if (t)
			{
				t->invuln = false;
				t->tdmg += hit;
				t->time_update = now;
				t->time_hit = now;

				i32 hi = t->num_hits;
				if (hi < MAX_HITS)
				{
					t->hits[hi].dmg = hit;
					t->hits[hi].time = now;
					t->hits[hi].eff_id = skillDef ? process_read_u32(skillDef + OFF_SKILLDEF_EFFECT) : 0;
					t->hits[hi].eff_hash = skillDef ? process_read_u32(skillDef + OFF_SKILLDEF_HASHID) : 0;
					++t->num_hits;
				}
			}
		}
	}

	// if the player is now in combat, add hit to totals
	if (InterlockedAnd(&player->combat.lastKnownCombatState, 1))
	{
		if (type == CL_CONDI_DMG_OUT ||
			type == CL_CRIT_DMG_OUT ||
			type == CL_GLANCE_DMG_OUT ||
			type == CL_PHYS_DMG_OUT)
		{
			InterlockedExchangeAdd(&player->combat.damageOut, hit);
		}

		if (type == CL_CONDI_DMG_IN ||
			type == CL_CRIT_DMG_IN ||
			type == CL_GLANCE_DMG_IN ||
			type == CL_PHYS_DMG_IN)
		{
			InterlockedExchangeAdd(&player->combat.damageIn, hit);
		}

		if (type == CL_HEAL_OUT)
		{
			InterlockedExchangeAdd(&player->combat.healOut, hit);
		}
	}
}


uintptr_t __fastcall hkCombatLog(uintptr_t a1, uintptr_t a2, int type)
{
	u64 rsp = (u64)_AddressOfReturnAddress();
	//u64 rbx = __readRBX();
	//u64 rbx = *(int64_t*)(rsp + 0x68);
	u64 rbx = *(int64_t*)(a2 + 0x80);

	i32 hit = *(int32_t*)(rsp + 0x50);		// process_read_i32(rsp + 0x4c);
	u64 aptr_src = *(int64_t*)(rbx + 0x40);	// process_read_i64(rbx + 0x40);
	u64 aptr_tgt = *(int64_t*)(rbx + 0x58);	// process_read_i64(rbx + 0x58);
	u64 skillDef = *(int64_t*)(rbx + 0x48);	// process_read_i64(rbx + 0x48);

	hkCombatLogProcess(type, hit, aptr_tgt, aptr_src, skillDef);

	if (_imp_hkCombatLog)
		return _imp_hkCombatLog(a1, a2, type);
	return 0;
}

LRESULT CALLBACK WndProc(
	_In_ HWND   hwnd,
	_In_ UINT   uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
)
{
	bool callOrig = ImGui_WndProcHandler(hwnd, uMsg, wParam, lParam);

	if (g_state.global_on_off) {

		if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) {

			for (u32 i = 0; i < MAX_PANELS; ++i)
			{
				Panel *panel = g_state.panel_arr[i];
				if (!panel)
					continue;

				if (is_bind_down(&panel->bind))
					callOrig = false;
			}
		}
	}

	if (callOrig && _imp_wndProc)
		return _imp_wndProc(hwnd, uMsg, wParam, lParam);
	else
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
}