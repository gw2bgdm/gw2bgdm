#include "logging.h"

#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

#ifndef min
#define min(a,b)	((a) > (b) ? (b) : (a))
#endif

static char *
level_table[] = {
	"NOTSET",	/* 0 ~ 9 */
	"NOTSET",	/* 0 ~ 9 */
	"NOTSET",	/* 0 ~ 9 */
	"NOTSET",	/* 0 ~ 9 */
	"NOTSET",	/* 0 ~ 9 */
	"NOTSET",	/* 0 ~ 9 */
	"NOTSET",	/* 0 ~ 9 */
	"NOTSET",	/* 0 ~ 9 */
	"NOTSET",	/* 0 ~ 9 */
	"NOTSET",	/* 0 ~ 9 */

	"DBG10",	/* 10 ~ 19 */
	"DBG11",	/* 10 ~ 19 */
	"DBG12",	/* 10 ~ 19 */
	"DBG13",	/* 10 ~ 19 */
	"DBG14",	/* 10 ~ 19 */
	"DEBUG",	/* 10 ~ 19 */
	"DBG16",	/* 10 ~ 19 */
	"DBG17",	/* 10 ~ 19 */
	"DBG18",	/* 10 ~ 19 */
	"DBG19",	/* 10 ~ 19 */

	"INFO",		/* 20 ~ 29 */
	"INFO",		/* 20 ~ 29 */
	"INFO",		/* 20 ~ 29 */
	"INFO",		/* 20 ~ 29 */
	"INFO",		/* 20 ~ 29 */
	"INFO",		/* 20 ~ 29 */
	"INFO",		/* 20 ~ 29 */
	"INFO",		/* 20 ~ 29 */
	"INFO",		/* 20 ~ 29 */
	"INFO",		/* 20 ~ 29 */

	"WARN",		/* 30 ~ 39 */
	"WARN",		/* 30 ~ 39 */
	"WARN",		/* 30 ~ 39 */
	"WARN",		/* 30 ~ 39 */
	"WARN",		/* 30 ~ 39 */
	"WARN",		/* 30 ~ 39 */
	"WARN",		/* 30 ~ 39 */
	"WARN",		/* 30 ~ 39 */
	"WARN",		/* 30 ~ 39 */
	"WARN",		/* 30 ~ 39 */

	"ERROR",	/* 40 ~ 49 */
	"ERROR",	/* 40 ~ 49 */
	"ERROR",	/* 40 ~ 49 */
	"ERROR",	/* 40 ~ 49 */
	"ERROR",	/* 40 ~ 49 */
	"ERROR",	/* 40 ~ 49 */
	"ERROR",	/* 40 ~ 49 */
	"ERROR",	/* 40 ~ 49 */
	"ERROR",	/* 40 ~ 49 */
	"ERROR",	/* 40 ~ 49 */

	"CRIT",		/* 50 ~ 59 */
	"CRIT",		/* 50 ~ 59 */
	"CRIT",		/* 50 ~ 59 */
	"CRIT",		/* 50 ~ 59 */
	"CRIT",		/* 50 ~ 59 */
	"CRIT",		/* 50 ~ 59 */
	"CRIT",		/* 50 ~ 59 */
	"CRIT",		/* 50 ~ 59 */
	"CRIT",		/* 50 ~ 59 */
	"CRIT",		/* 50 ~ 59 */
};


#if defined(_WIN32) || defined(_WIN64)
int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

	tp->tv_sec = (long)((time - EPOCH) / 10000000L);
	tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
	return 0;
}

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
int gettimeofday2(struct timeval * tv, struct timezone * tz)
{
	FILETIME ft;
	unsigned __int64 tmpres = 0;
	static int tzflag;

	if (NULL != tv)
	{
		GetSystemTimeAsFileTime(&ft);

		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		/*converting file time to unix epoch*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS;
		tmpres /= 10;  /*convert into microseconds*/
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}

	if (NULL != tz)
	{
		if (!tzflag)
		{
			_tzset();
			tzflag++;
		}
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}
	return 0;
}
#endif

static logger_t *loggers = NULL;

logger_t *
logging_getlogger(char name[])
{
	logger_t *logger;

	for (logger = loggers; logger != NULL; logger = logger->next) {
		if (strcmp(logger->name, name) == 0)
			break;
	}

	return logger;
}

void
logging_setlogger(logger_t *logger)
{
	int i;

	logger_t **next;

	for (i = 0; i < sizeof(level_table)/sizeof(level_table[0]); i++) {
		if (logger->level_table[i] == NULL)
			logger->level_table[i] = level_table[i];
	}
	for (next = &loggers; *next != NULL; next = &(*next)->next) {
		if (strcmp((*next)->name, logger->name) == 0)
			break;
	}

	*next = logger;
}

char *
logger_getlevelname(logger_t *logger, int level)
{
	assert(level < LOGGING_MAX_LEVEL);

	return logger->level_table[level];
}

void
logger_setlevelname(logger_t *logger, int level, char *name)
{
	assert(level < LOGGING_MAX_LEVEL);

	logger->level_table[level] = name;
}

/* 
 * 0 not log
 * 1 log
 */
static int
filter_filter(logger_filter_t *filter, logger_record_t *record)
{
	if (record->level < filter->minlevel)
		return 0;
	if (record->level > filter->maxlevel)
		return 0;
	return 1;
}

static int
formatter_format(logger_formatter_t *formatter, logger_record_t *record,
	char buf[], int buflen)
{
	size_t i;
	size_t len;
	size_t skip;
	size_t fmtlen;

	char *fmt;

	fmt = formatter->fmt;
	fmtlen = strlen(formatter->fmt);

	len = 0;
	for (i = 0; i < fmtlen && len < buflen; i += skip) {
		if (memcmp(fmt + i, "%%", 2) == 0) {
			buf[len++] = '%';
			skip = 2;
		} else if (memcmp(fmt + i, "%(name)s", 8) == 0) {
			size_t n = strlen(record->name);
			n = min(n, buflen - len);
			memcpy(&buf[len], record->name, n);
			len += n;
			skip = 8;
		} else if (memcmp(fmt + i, "%(levelno)d", 11) == 0) {
			size_t n = snprintf(&buf[len], buflen - len, "%d", record->level);
			len += n;
			skip = 11;
		} else if (memcmp(fmt + i, "%(levelname)s", 13) == 0) {
			size_t n = strlen(record->levelname);
			n = min(n, buflen - len);
			memcpy(&buf[len], record->levelname, n);
			len += n;
			skip = 13;
		} else if (memcmp(fmt + i, "%(message)s", 11) == 0) {
			size_t n = strlen(record->message);
			n = min(n, buflen - len);
			memcpy(&buf[len], record->message, n);
			len += n;
			skip = 11;
		} else if (memcmp(fmt + i, "%(created)d", 11) == 0) {
			size_t n = snprintf(&buf[len], buflen - len, "%ld", (long)record->seconds);
			len += n;
			skip = 11;
		} else if (memcmp(fmt + i, "%(asctime)s", 11) == 0) {
			size_t n;
			struct tm tm;
#if defined(_WIN32) || defined(_WIN64)
			//gmtime_s(&tm, &record->seconds);
			localtime_s(&tm, &record->seconds);
#else
			gmtime_r(&record->seconds, &tm);
#endif
			n = strftime(&buf[len], buflen - len, formatter->datefmt, &tm);
			len += n;
			skip = 11;
		} else if (memcmp(fmt + i, "%(msecs)d", 9) == 0) {
			size_t n = snprintf(&buf[len], buflen - len, "%d", (int)record->useconds/1000);
			len += n;
			skip = 9;
		} else if (memcmp(fmt + i, "%(usecs)d", 9) == 0) {
			size_t n = snprintf(&buf[len], buflen - len, "%d", (int)record->useconds);
			len += n;
			skip = 9;
		} else {
			buf[len++] = fmt[i];
			skip = 1;
		}
	}

#undef min

	return (int)len;
}

static void
handler_emit(logger_handler_t *handler, logger_record_t *record)
{
	int  len;
	char buf[4096];
	logger_filter_t *filter;

	for (filter = handler->filter; filter != NULL; filter = filter->next) {
		if (filter_filter(filter, record) == 1)
			break;
	}
	
	if (filter == NULL)
		return;

	len = formatter_format(handler->formatter, record, buf, sizeof(buf) - 1);
	buf[len++] = '\n';
	fwrite(buf, sizeof(char), len, handler->file);
	fflush(handler->file);
}

static void
logging_log(logger_t *logger, int level, char *msg, va_list args)
{
	logger_record_t   record;
	logger_filter_t  *filter;
	logger_handler_t *handler;
	char              buf[4096];

	struct timeval tv;

	record.name      = logger->name;
	record.level     = level;
	record.levelname = logger->level_table[level];

	vsnprintf(buf, sizeof(buf), msg, args);
	record.message = buf;

    gettimeofday(&tv, NULL);
	record.seconds  = tv.tv_sec;
	record.useconds = tv.tv_usec;

	for (filter = logger->filter; filter != NULL; filter = filter->next) {
		if (filter_filter(filter, &record) == 1)
			break;
	}

	if (filter == NULL)
		return;

	for (handler = logger->handler; handler != NULL; handler = handler->next) {
		handler_emit(handler, &record);
	}
}

void
logger_debug(logger_t *logger, char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	logging_log(logger, LOG_DEBUG, msg, args);

	va_end(args);
}

void
logger_info(logger_t *logger, char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	logging_log(logger, LOG_INFO, msg, args);

	va_end(args);
}

void
logger_warning(logger_t *logger, char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	logging_log(logger, LOG_WARNING, msg, args);

	va_end(args);
}

void
logger_error(logger_t *logger, char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	logging_log(logger, LOG_ERROR, msg, args);

	va_end(args);
}

void
logger_critical(logger_t *logger, char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	logging_log(logger, LOG_CRITICAL, msg, args);

	va_end(args);
}

void
logger_log(logger_t *logger, int level, char *msg, ...)
{
	va_list args;
	va_start(args, msg);

	logging_log(logger, level, msg, args);

	va_end(args);
}
