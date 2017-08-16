#pragma once
#include "core/logging.h"

extern logger_t s_logger;

#define LOG_DEBUG(...) logger_log(&s_logger, LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO(...) logger_info(&s_logger, __VA_ARGS__)
#define LOG_WARN(...) logger_warning(&s_logger, __VA_ARGS__)
#define LOG_ERR(...) logger_error(&s_logger, __VA_ARGS__)
#define LOG_CRIT(...) logger_critical(&s_logger, __VA_ARGS__)
#define LOG(i, ...) logger_log(&s_logger, i, __VA_ARGS__)

#if defined(_WIN32) || defined(DEBUG)
#include <stdio.h>
#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DPRINTF(...)
#endif