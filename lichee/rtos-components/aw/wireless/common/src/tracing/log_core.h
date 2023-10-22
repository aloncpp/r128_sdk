#ifndef __LOG_CORE_H__
#define __LOG_CORE_H__

#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>

#if __cplusplus
extern "C" {
#endif

#ifndef DEFAULT_LOG_FILE
#define DEFAULT_LOG_FILE "/tmp/log_hub.txt"
#endif

#ifndef DEFAULT_LOG_CMD_FIFO
#define DEFAULT_LOG_CMD_FIFO "/tmp/log_core"
#endif

typedef enum _log_hub_type {
    LOG_BACKEND_NONE = 0,
    LOG_BACKEND_STDOUT_SYNC = 1 << 0,
    LOG_BACKEND_STDOUT_ASYNC = 1 << 1,
    LOG_BACKEND_FILE_SYNC = 1 << 2,
    LOG_BACKEND_FILE_ASYNC = 1 << 3,
    LOG_BACKEND_SYSLOG_SYNC = 1 << 4,
} log_hub_type_t;

typedef enum _log_setting {
    LOG_SETTING_NONE = 0,
    LOG_SETTING_TIME = 1 << 0,
    LOG_SETTING_COLOR = 1 << 1,
    LOG_SETTING_FFLUSH = 1 << 2,
    LOG_SETTING_LINE_NONE = 1 << 3, // have not \n
    LOG_SETTING_LINE_R_N = 1 << 4, // \r\n
    LOG_SETTING_LINE_N = 1 << 5, // \n
    LOG_SETTING_ADD_TAG = 1 << 6, // add function name and called line, not support yet
    LOG_SETTING_ERROR = 1 << 7,
} log_setting_t;

typedef enum _log_level {
    LOG_LEVEL_MSG_NONE = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_MSGDUMP = 5,
    LOG_LEVEL_EXCESSIVE = 6,
} log_level_t;

typedef struct _log_type {
    log_hub_type_t type;
    int setting;
    log_level_t level;
} log_type_t;

extern log_type_t global_log_type;

#ifdef __GNUC__
#define PRINTF_FORMAT(a,b) __attribute__ ((format (printf, (a), (b))))
#define STRUCT_PACKED __attribute__ ((packed))
#else
#define PRINTF_FORMAT(a,b)
#define STRUCT_PACKED
#endif

void log_print(const char *tag, int level, const char *fmt, ...) PRINTF_FORMAT(3, 4);
int log_set_debug_level(log_level_t level);
log_level_t log_get_debug_level(void);

int log_start(char *log_file_path, char *debug_fifo_path);
int log_stop(void);
int log_parse_cmd_register_function(int (*fun)(char *key, char*op, char*val));

#if __cplusplus
}; // extern "C"
#endif

#endif // __LOG_CORE_H__
