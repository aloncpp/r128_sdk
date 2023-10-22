#ifndef __AWLOG_H__
#define __AWLOG_H__

#include <stdio.h>
#include <xtensa/xtutil.h>

/*
 * When XOS debugging features are enabled, it uses the libxtutil
 * library for formatted output. Thus, this library must also be
 * linked into the application. This library provides functions
 * compatible with the C standard library (such as xt_printf()
 * instead of printf()), although not all features are supported.
 * In particular, xt_printf() does not support floating point
 * formats and some other format options. The libxtutil functions
 * are lightweight versions with much smaller code and data memory
 * requirements and they are also thread-safe. For more details on
 * included functions refer to the file xtensa/xtutil.h.
 */
#define printfFromISR	xt_printf

#ifdef CONFIG_LOG_LEVEL
#define AWLOG_LVL CONFIG_LOG_LEVEL
#else
#define AWLOG_LVL 8
#endif

#ifndef pr_fmt
#define pr_fmt(fmt) fmt
#endif

#define AWLOG_LVL_ERROR                  3
#define AWLOG_LVL_WARNING                4
#define AWLOG_LVL_INFO                   6
#define AWLOG_LVL_DBG                    7

#if (AWLOG_LVL >= AWLOG_LVL_ERROR)
#define pr_err(fmt, ...) printf(pr_fmt(fmt), ##__VA_ARGS__)
#else
#define pr_err(fmt, ...)
#endif

#if (AWLOG_LVL >= AWLOG_LVL_WARNING)
#define pr_warn(fmt, ...) printf(pr_fmt(fmt), ##__VA_ARGS__)
#else
#define pr_warn(fmt, ...)
#endif

#if (AWLOG_LVL >= AWLOG_LVL_INFO)
#define pr_info(fmt, ...) printf(pr_fmt(fmt), ##__VA_ARGS__)
#else
#define pr_info(fmt, ...)
#endif

#if (AWLOG_LVL >= LOG_LEVEL_DBG) && defined(DEBUG)
#define pr_debug(fmt, ...) printf(pr_fmt(fmt), ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...)
#endif

#endif /* __AWLOG_H__ */
