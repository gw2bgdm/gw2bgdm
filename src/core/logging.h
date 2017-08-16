#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <stdint.h> // portable: uint64_t   MSVC: __int64
#include <time.h> // gmtime_r
// MSVC defines this in winsock2.h!?
#ifndef _WINSOCK2API_
typedef struct timeval {
	long tv_sec;
	long tv_usec;
} timeval;
#endif // !_WINSOCK2API_
typedef struct timezone
{
	int  tz_minuteswest; // minutes W of Greenwich
	int  tz_dsttime;     // type of dst correction
} timezone;
#else
#include <sys/time.h>
#endif


#define LOGGING_MAX_LEVEL	60

enum {
	LOG_CRITICAL = 50,
	LOG_ERROR    = 40,
	LOG_WARNING  = 30,
	LOG_INFO     = 20,
	LOG_DEBUG    = 15,
	LOG_NOTSET   = 0
};

typedef struct logger_record {
	char *name;
	int   level;
	char *levelname;
	char *message;

	time_t      seconds;
#include <stdio.h>
#if !defined(_WIN32) || !defined(_WIN64)
	suseconds_t useconds;
#else
	long useconds;
#endif
} logger_record_t;

typedef struct logger_formatter {
	char *fmt;
	char *datefmt;
} logger_formatter_t;

typedef struct logger_filter {
	char *name;
	int   minlevel;	/* include */
	int   maxlevel; /* include */

	struct logger_filter *next;
} logger_filter_t;

typedef struct logger_handler {
	char        *name;
	FILE        *file;
	logger_filter_t    *filter;
	logger_formatter_t *formatter;

	struct logger_handler *next;
} logger_handler_t;

typedef struct logger {
	char *name;
	char *level_table[LOGGING_MAX_LEVEL];

	logger_filter_t  *filter;
	logger_handler_t *handler;

	struct logger *next;
} logger_t;


#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
int gettimeofday(struct timeval * tp, struct timezone * tzp);
#endif

logger_t *
logging_getlogger(char name[]);

void
logging_setlogger(logger_t *logger);

char *
logger_getlevelname(logger_t *logger, int level);

void
logger_setlevelname(logger_t *logger, int level, char *name);

void
logger_debug(logger_t *logger, char *msg, ...);

void
logger_info(logger_t *logger, char *msg, ...);

void
logger_warning(logger_t *logger, char *msg, ...);

void
logger_error(logger_t *logger, char *msg, ...);

void
logger_critical(logger_t *logger, char *msg, ...);

void
logger_log(logger_t *logger, int level, char *msg, ...);

#ifdef __cplusplus
}
#endif


#endif /* __LOGGING_H__ */
