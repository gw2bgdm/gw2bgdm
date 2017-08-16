#pragma once
#include <stdint.h>
#include "core/types.h"

// Hooked functions.
typedef void(__fastcall *hkGameThread_t)(uintptr_t, int, int);
typedef i64(__fastcall *hkCbtLogResult_t)(u64 a1, u64 a2);
typedef i64(__fastcall *hkDamageResult_t)(i64 a1, i32 a2, u32 a3, u32 a4, void* a5, i64 a6, i64 a7, u64 a8);
typedef uintptr_t(__fastcall *hkCombatLog_t)(uintptr_t, uintptr_t, int);

extern void hkGameThread(uintptr_t pInst, int a2, int frame_time);
extern uintptr_t hkCombatLog(uintptr_t a1, uintptr_t a2, int type);
extern i64 hkCbtLogResult(uintptr_t a1, uintptr_t a2);
extern i64  hkDamageResult(i64 a1, i32 type, u32 dmg, u32 a4, void* a5, i64 a6, i64 a7, u64 target);

extern hkGameThread_t _imp_hkGameThread;
extern hkCombatLog_t _imp_hkCombatLog;
extern hkCbtLogResult_t _imp_hkCbtLogResult;
extern hkDamageResult_t _imp_hkDamageResult;

typedef LRESULT(CALLBACK *WndProc_t)(
	_In_ HWND   hwnd,
	_In_ UINT   uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
	);

LRESULT CALLBACK WndProc(
	_In_ HWND   hwnd,
	_In_ UINT   uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
);

extern WndProc_t _imp_wndProc;