#ifndef __LOG_BACKEND_HUB_H__
#define __LOG_BACKEND_HUB_H__

#include <stdarg.h>

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

void log_hub_vprintf(const char *format, va_list arg);
int log_hub_printf(const char *format, ...);
void log_hub_fflush(void *file);

void log_hub_process_param_change();
int log_hub_start(const char *path);
int log_hub_stop();

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif // __LOG_BACKEND_HUB_H__