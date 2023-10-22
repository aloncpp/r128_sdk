#ifdef OS_NET_FREERTOS_OS
#include <stdio.h>
#include <wifi_log.h>
#else
#include <syslog.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdint.h>
#include <stdbool.h>
#include <wifi_log.h>
#endif

#include <stdarg.h>
#include <sys/time.h>
#include <time.h>

typedef struct {
	wmg_log_level_t wmg_debug_level;
	int wmg_log_para;
} global_log_type_t;

static global_log_type_t global_log = {
	.wmg_debug_level = WMG_MSG_INFO,
	.wmg_log_para = 0,
};

static int wmg_printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

void wmg_print(int level, int para_enable, const char *tag, const char *file, const char *func, int line, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	if(level <= global_log.wmg_debug_level) {
#ifndef OS_NET_FREERTOS_OS
		if(para_enable && (global_log.wmg_log_para != 0)) {
			char strtimestamp[64];
			struct timeval tv;
			struct tm nowtm;
			gettimeofday(&tv, NULL);
			localtime_r(&tv.tv_sec, &nowtm);
			strftime(strtimestamp, sizeof(strtimestamp), "%Y-%m-%d %H:%M:%S", &nowtm);
			wmg_printf("%s:%03d: ", strtimestamp, tv.tv_usec / 1000);
		}
#endif

		if(tag != NULL) {
			wmg_printf("%s", tag);
		}

#ifndef OS_NET_FREERTOS_OS
		if(para_enable && (global_log.wmg_log_para != 0)) {
			if(file != NULL) {
				wmg_printf("[%s:%s:%d]", file, func, line);
			}
		}
#endif

		vprintf(fmt, ap); //main output
		fflush(NULL);
	}

	va_end(ap);
	return;
}

void wmg_set_debug_level(wmg_log_level_t level)
{
	global_log.wmg_debug_level = level;
}

int wmg_get_debug_level()
{
	return global_log.wmg_debug_level;
}

void wmg_set_log_para(int log_para)
{
	global_log.wmg_log_para = log_para;
}

int wmg_get_log_para()
{
	return global_log.wmg_log_para;
}
