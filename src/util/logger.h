#pragma once

#define LOG_DEBUG(s, ...) logger_log(LOG_LEVEL__DEBUG, s __VA_OPT__(,) __VA_ARGS__)
#define LOG_INFO(s, ...) logger_log(LOG_LEVEL__INFO, s __VA_OPT__(,) __VA_ARGS__)
#define LOG_WARN(s, ...) logger_log(LOG_LEVEL__WARN, s __VA_OPT__(,) __VA_ARGS__)
#define LOG_ERROR(s, ...) logger_log(LOG_LEVEL__ERROR, s __VA_OPT__(,) __VA_ARGS__)

typedef enum log_level {
    LOG_LEVEL__DEBUG,
    LOG_LEVEL__INFO,
    LOG_LEVEL__WARN,
    LOG_LEVEL__ERROR,
    NUM_LOG_LEVELS
} log_level_t;

void logger_set_log_level(log_level_t const log_level);

void logger_log(log_level_t const log_level, char const* const s, ...);