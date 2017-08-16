#include "core/time.h"
#include <sys/time.h>

static i64 g_start;

static i64 time_query(void)
{
	struct timeval now;
    gettimeofday(&now, 0);
    
    return (i64)1000000 * now.tv_sec + now.tv_usec;
}

void time_create(void)
{
	g_start = time_query();
}

i64 time_get(void)
{
	return time_query() - g_start;
}

