#ifndef __AWLOG_H__
#define __AWLOG_H__

#ifdef CONFIG_LOG_LEVEL
#define AWLOG_LVL CONFIG_LOG_LEVEL
#else
#define AWLOG_LVL 8
#endif

#ifndef pr_fmt
#define pr_fmt(fmt) fmt
#endif

#ifdef CONFIG_COMPONENT_ULOG

#define LOG_TAG pr_fmt("")
#define LOG_LVL AWLOG_LVL

#include "../components/thirdparty/ulog/ulog.h"

#define pr_err(...)	ulog_e(LOG_TAG, __VA_ARGS__)
#define pr_warn(...)	ulog_w(LOG_TAG, __VA_ARGS__)
#define pr_info(...)	ulog_i(LOG_TAG, __VA_ARGS__)
#define pr_debug(...)	ulog_d(LOG_TAG, __VA_ARGS__)

#define hexdump(buf, size) LOG_HEX("", 16, buf, size)

#else /* CONFIG_COMPONENT_ULOG */

#include <stdio.h>

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

extern int aw_hexdump(const char *buf, int bytes);
#define hexdump(buf, len) aw_hexdump(buf, len)

#endif /* CONFIG_COMPONENT_ULOG */
#endif /* __AWLOG_H__ */
