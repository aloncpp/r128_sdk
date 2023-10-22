#include "log_core.h"
#include "log_core_inner.h"
#include "log_backend_hub.h"

static FILE *out_file = NULL;
static char *last_path = NULL;
static int error_times = 0;

static int log_hub_open_file(const char *path)
{
    if (!path) {
        return 0;
    }

    if (last_path == NULL || strcmp(last_path, path) != 0) {
        /* Save our path to enable re-open */
        free(last_path);
        last_path = strdup(path);
    }

    out_file = fopen(path, "a");
    if (out_file == NULL) {
        printf("btmg_debug_open_file: Failed to open "
               "output file, using standard output");
        return -1;
    }
    error_times = 0;
    return 0;
}

static void log_hub_close_file(void)
{
    if (!out_file)
        return;
    fclose(out_file);
    out_file = NULL;
    free(last_path);
    last_path = NULL;
}

void log_hub_vprintf(const char *format, va_list args)
{
    if (global_log_type.type & LOG_BACKEND_STDOUT_SYNC) {
        vprintf(format, args);
    } else if (global_log_type.type & LOG_BACKEND_STDOUT_ASYNC) {
        // todo: not support yet
        // todo: using cas atomic concurrent queue. work queue
    }
    if (global_log_type.type & LOG_BACKEND_FILE_SYNC) {
        if (out_file) {
            // nwrite = vsnprintf(tail, size_left, format, args);
            vfprintf(out_file, format, args);
            fflush(out_file);
            // fprintf(out_file, "\n");
        } else {
            if (error_times > 20) {
                return;
            }
            if (error_times == 20) {
                error_times++;
                log_hub_open_file(DEFAULT_LOG_FILE);
                printf("error try to use default path [%s][%d]",__func__,__LINE__);
            } else if ((last_path != NULL) && (log_hub_open_file(last_path) == 0)) {
                vfprintf(out_file, format, args);
                fflush(out_file);
            } else {
                error_times++;
            }
        }
    } else if (global_log_type.type & LOG_BACKEND_FILE_ASYNC) {
        // todo: not support yet
        // todo: using cas atomic concurrent queue. work queue
    }
    if (global_log_type.type & LOG_BACKEND_SYSLOG_SYNC) {
        // todo: not support yet
    }
}

int log_hub_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format); //初始化参数指针
    log_hub_vprintf(format, args);
    va_end(args);
}

void log_hub_fflush(void *file)
{
    fflush(file);
    return;
}

int log_hub_start(const char *path)
{
    if (path == NULL) {
        return 0;
    }
    if (path && !out_file) {
        log_hub_open_file(path);
        return 0;
    } else if (!strcmp(path, last_path) && out_file){
        // path same and out_file exist nothing to do
        return 0;
    } else if (strcmp(path, last_path)) {
        // path diff reopen
        log_hub_close_file();
        log_hub_open_file(path);
        return 0;
    }
    return 0;
}

int log_hub_stop()
{
    if (out_file) {
        log_hub_close_file();
    }
    return 0;
}
