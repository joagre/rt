#ifndef ACRT_LOG_H
#define ACRT_LOG_H

#include <stdio.h>

// Log level constants for compile-time filtering (preprocessor needs numeric values)
#define ACRT_LOG_LEVEL_TRACE 0
#define ACRT_LOG_LEVEL_DEBUG 1
#define ACRT_LOG_LEVEL_INFO  2
#define ACRT_LOG_LEVEL_WARN  3
#define ACRT_LOG_LEVEL_ERROR 4
#define ACRT_LOG_LEVEL_NONE  5

// Log level type for runtime use
typedef int acrt_log_level_t;

// Default compile-time log level (can override with -DRT_LOG_LEVEL=...)
#ifndef ACRT_LOG_LEVEL
#define ACRT_LOG_LEVEL ACRT_LOG_LEVEL_INFO
#endif

// Core logging function (not typically called directly)
void acrt_log_write(acrt_log_level_t level, const char *file, int line,
                  const char *fmt, ...) __attribute__((format(printf, 4, 5)));

// Logging macros that compile out based on ACRT_LOG_LEVEL
#if ACRT_LOG_LEVEL <= ACRT_LOG_LEVEL_TRACE
#define ACRT_LOG_TRACE(...) acrt_log_write(ACRT_LOG_LEVEL_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#else
#define ACRT_LOG_TRACE(...) ((void)0)
#endif

#if ACRT_LOG_LEVEL <= ACRT_LOG_LEVEL_DEBUG
#define ACRT_LOG_DEBUG(...) acrt_log_write(ACRT_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#else
#define ACRT_LOG_DEBUG(...) ((void)0)
#endif

#if ACRT_LOG_LEVEL <= ACRT_LOG_LEVEL_INFO
#define ACRT_LOG_INFO(...) acrt_log_write(ACRT_LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#else
#define ACRT_LOG_INFO(...) ((void)0)
#endif

#if ACRT_LOG_LEVEL <= ACRT_LOG_LEVEL_WARN
#define ACRT_LOG_WARN(...) acrt_log_write(ACRT_LOG_LEVEL_WARN, __FILE__, __LINE__, __VA_ARGS__)
#else
#define ACRT_LOG_WARN(...) ((void)0)
#endif

#if ACRT_LOG_LEVEL <= ACRT_LOG_LEVEL_ERROR
#define ACRT_LOG_ERROR(...) acrt_log_write(ACRT_LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#else
#define ACRT_LOG_ERROR(...) ((void)0)
#endif

#endif // ACRT_LOG_H
