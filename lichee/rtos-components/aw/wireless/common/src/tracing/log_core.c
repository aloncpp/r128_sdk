#include "log_core.h"
#include "log_core_inner.h"
#include "log_backend_hub.h"
#include "log_parse_cmd.h"
#ifdef OS_NET_FREERTOS_OS
#else
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/epoll.h>
#endif

#define LOG_COLOR_NONE "\033[0m"
#define LOG_COLOR_RED "\033[1;31m"
#define LOG_COLOR_YELLOW "\033[1;33m"
#define LOG_COLOR_BLUE "\033[1;34m"
#define LOG_COLOR_GERRN "\033[1;32m"

const char *logleveltocolor[] = {
    LOG_COLOR_NONE, // LOG_LEVEL_MSG_NONE  = 0,
    LOG_COLOR_RED, // LOG_LEVEL_ERROR     = 1,
    LOG_COLOR_YELLOW, // LOG_LEVEL_WARNING   = 2,
    LOG_COLOR_GERRN, // LOG_LEVEL_INFO      = 3,
    LOG_COLOR_NONE, // LOG_LEVEL_DEBUG     = 4,
    LOG_COLOR_NONE, // LOG_LEVEL_MSGDUMP   = 5,
    LOG_COLOR_NONE, // LOG_LEVEL_EXCESSIVE = 6,
};

const char *logleveltoflag[] = {
    "N", // LOG_LEVEL_MSG_NONE  = 0,
    "E", // LOG_LEVEL_ERROR     = 1,
    "W", // LOG_LEVEL_WARNING   = 2,
    "I", // LOG_LEVEL_INFO      = 3,
    "D", // LOG_LEVEL_DEBUG     = 4,
    "P", // LOG_LEVEL_MSGDUMP   = 5,
    "S", // LOG_LEVEL_EXCESSIVE = 6,
};

log_type_t global_log_type = {
    .type = LOG_BACKEND_STDOUT_SYNC,
    .setting = LOG_SETTING_TIME | LOG_SETTING_COLOR | LOG_SETTING_FFLUSH,
    //    | LOG_SETTING_LINE_N,
    .level = LOG_LEVEL_INFO,
};

void log_print(const char *tag, int level, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    if (level <= global_log_type.level) {
        if (global_log_type.setting & LOG_SETTING_COLOR) {
            switch (level) {
            case LOG_LEVEL_ERROR:
            case LOG_LEVEL_WARNING:
            case LOG_LEVEL_INFO:
                log_hub_printf("%s", logleveltocolor[level]);
                break;
            default:
                break;
            }
        }
        if (global_log_type.setting & LOG_SETTING_TIME) {
            char strtimestamp[64];
            struct timeval tv;
            struct tm nowtm;
            gettimeofday(&tv, NULL);
            localtime_r(&tv.tv_sec, &nowtm);
            strftime(strtimestamp, sizeof(strtimestamp), "%Y-%m-%d %H:%M:%S", &nowtm);
            log_hub_printf("%s:%03d: ", strtimestamp, tv.tv_usec / 1000);
        }
        if (tag != NULL && strlen(tag) > 1 && strlen(tag) < 10) {
            log_hub_printf("[%s][%s]:", tag, logleveltoflag[level]);
        }
        log_hub_vprintf(fmt, ap); //main output
        if (global_log_type.setting & LOG_SETTING_COLOR) {
            switch (level) {
            case LOG_LEVEL_ERROR:
            case LOG_LEVEL_WARNING:
            case LOG_LEVEL_INFO:
                log_hub_printf(LOG_COLOR_NONE);
                break;
            default:
                break;
            }
        }
        if (global_log_type.setting & LOG_SETTING_LINE_N) {
            log_hub_printf("\n");
        } else if (global_log_type.setting & LOG_SETTING_LINE_R_N) {
            log_hub_printf("\r\n");
        }
        if (global_log_type.setting & LOG_SETTING_FFLUSH) {
            log_hub_fflush(NULL);
        }
    }
    va_end(ap);
    return;
}

int log_set_debug_level(log_level_t level)
{
    global_log_type.level = level;
    return 0;
}

log_level_t log_get_debug_level(void)
{
    return global_log_type.level;
}

int log_start(char *log_file_path, char *debug_fifo_path)
{
    log_hub_start(log_file_path);
    log_parse_cmd_start(debug_fifo_path);
    return 0;
}

int log_stop(void)
{
    log_parse_cmd_stop();
    log_hub_stop();
    return 0;
}
