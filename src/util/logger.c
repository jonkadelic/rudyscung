#include "./logger.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

static char const* const NAMES[NUM_LOG_LEVELS] = {
    [LOG_LEVEL__DEBUG] = "DEBUG",
    [LOG_LEVEL__INFO] = "INFO",
    [LOG_LEVEL__WARN] = "WARN",
    [LOG_LEVEL__ERROR] = "ERROR"
};

static log_level_t current_log_level = LOG_LEVEL__INFO;

void logger_set_log_level(log_level_t const log_level) {
    assert(log_level >= 0 && log_level < NUM_LOG_LEVELS);

    current_log_level = log_level;
}

void logger_log(log_level_t const log_level, char const* const s, ...) {
    assert(log_level >= 0 && log_level < NUM_LOG_LEVELS);

    if (log_level >= current_log_level) {
        printf("[%s] ", NAMES[log_level]);
        va_list varargs;
        va_start(varargs, s);
        vprintf(s, varargs);
        va_end(varargs);
        printf("\n");
    }
}