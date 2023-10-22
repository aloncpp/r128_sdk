#ifndef __LOG_CORE_INNER_H__
#define __LOG_CORE_INNER_H__

#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#if __cplusplus
extern "C" {
#endif

#define LOG_ERROR(fmt, arg...)                                                                     \
    log_print("LOG_CORE", LOG_LEVEL_ERROR, "[%s:%d]:  " fmt "\n", __func__, __LINE__, ##arg)
#define LOG_WARNING(fmt, arg...)                                                                   \
    log_print("LOG_CORE", LOG_LEVEL_WARNING, "[%s:%d]:  " fmt "\n", __func__, __LINE__, ##arg)
#define LOG_INFO(fmt, arg...)                                                                      \
    log_print("LOG_CORE", LOG_LEVEL_INFO, "[%s:%d]:  " fmt "\n", __func__, __LINE__, ##arg)
#define LOG_DEBUG(fmt, arg...)                                                                     \
    log_print("LOG_CORE", LOG_LEVEL_DEBUG, "[%s:%d]:  " fmt "\n", __func__, __LINE__, ##arg)

#if __cplusplus
}; // extern "C"
#endif

#endif // __LOG_CORE_INNER_H__
