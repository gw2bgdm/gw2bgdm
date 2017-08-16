#include "core/time.h"
#include <Windows.h>

static i64 g_freq;
static i64 g_start;

void time_create(void)
{
	QueryPerformanceFrequency((LARGE_INTEGER*)&g_freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&g_start);
}

i64 time_get(void)
{
	if (!g_freq)
		return 0;

	i64 current;
	QueryPerformanceCounter((LARGE_INTEGER*)&current);
	return ((current - g_start) * 1000000) / g_freq;
}
