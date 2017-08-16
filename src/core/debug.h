#pragma once
#include <strsafe.h>
#include <wtypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DEBUG
#define DBGPRINT(kwszDebugFormatString, ...) _DBGPRINT(__FUNCTIONW__, __LINE__, kwszDebugFormatString, __VA_ARGS__)
VOID _DBGPRINT(LPCWSTR kwszFunction, INT iLineNumber, LPCWSTR kwszDebugFormatString, ...);
#else
#if defined(_MSC_VER)
#include <intrin.h>
#define DBGPRINT( kwszDebugFormatString, ... ) __noop(kwszDebugFormatString, __VA_ARGS__);
#else
#define DBGPRINT( kwszDebugFormatString, ... ) ;
#endif
#endif

#ifdef __cplusplus
}
#endif