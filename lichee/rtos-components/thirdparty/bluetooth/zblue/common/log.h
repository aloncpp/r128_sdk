/** @file  log.h
 *  @brief Bluetooth subsystem logging helpers.
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_LOG_H
#define __BT_LOG_H

#include <linker/sections.h>
#if 1 //Wewisetek Modified
#else //Origin
#include <offsets.h>
#endif
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/hci.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(BT_DBG_ENABLED)
#define BT_DBG_ENABLED 1
#endif

#if BT_DBG_ENABLED
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#endif

#if 0///*defined*//*Wewisetek Modified*/(CONFIG_BT_DEBUG_MONITOR)
#include <stdio.h>

/* These defines follow the values used by syslog(2) */
#define BT_LOG_ERR      3
#define BT_LOG_WARN     4
#define BT_LOG_INFO     6
#define BT_LOG_DBG      7

__printf_like(2, 3) void bt_log(int prio, const char *fmt, ...);

#define BT_DBG(fmt, ...) \
	if (BT_DBG_ENABLED) { \
		bt_log(BT_LOG_DBG, "%s (%p): " fmt, \
		       __func__, k_current_get(), ##__VA_ARGS__); \
	}

#define BT_ERR(fmt, ...) bt_log(BT_LOG_ERR, "%s: " fmt, \
				__func__, ##__VA_ARGS__)
#define BT_WARN(fmt, ...) bt_log(BT_LOG_WARN, "%s: " fmt, \
				 __func__, ##__VA_ARGS__)
#define BT_INFO(fmt, ...) bt_log(BT_LOG_INFO, fmt, ##__VA_ARGS__)

/* Enabling debug increases stack size requirement */
#define BT_STACK_DEBUG_EXTRA	300

#else// /*defined*//*Wewisetek Modified*/(CONFIG_BT_DEBUG_LOG)

#if /*defined*//*Wewisetek Modified*/(SYS_LOG_DOMAIN)
#define SYS_LOG_DOMAIN "bt"
#endif

#include <logging/sys_log.h>

#define BT_DBG(fmt, ...) \
	if (BT_DBG_ENABLED) { \
		SYS_LOG_DBG("(%p) " fmt, /*k_current_get()*/NULL, \
			    ##__VA_ARGS__); \
	}

#define BT_ERR(fmt, ...) SYS_LOG_ERR(fmt, ##__VA_ARGS__)
#define BT_WARN(fmt, ...) SYS_LOG_WRN(fmt, ##__VA_ARGS__)
#define BT_INFO(fmt, ...) SYS_LOG_INF(fmt, ##__VA_ARGS__)

/* Enabling debug increases stack size requirement considerably */
#define BT_STACK_DEBUG_EXTRA	300
#endif

#define BT_ASSERT(cond) if (!(cond)) { \
				BT_ERR("assert: '" #cond "' failed"); \
				k_oops(); \
			}

#define BT_ASSERT_MSG __ASSERT

#define __ASSERT_NO_MSG(cond) BT_ASSERT(cond)

extern void print_hex_dump_bytes(const void *addr, size_t len);

#define BT_HEXDUMP_DBG(_data, _length, _str) \
	if (BT_DBG_ENABLED) { \
		BT_DBG("%s\n", _str); \
		print_hex_dump_bytes(_data, _length); \
	}

#define TP printf("<%s : %d>\n", __func__, __LINE__)

#if 1 //Wewisetek Modified
#else //Origin
#define BT_STACK(name, size) \
		K_THREAD_STACK_MEMBER(name, (size) + BT_STACK_DEBUG_EXTRA)
#endif
#define BT_STACK_NOINIT(name, size) \
		K_THREAD_STACK_DEFINE(name, (size) + BT_STACK_DEBUG_EXTRA)

/* NOTE: These helper functions always encodes into the same buffer storage.
 * It is the responsibility of the user of this function to copy the information
 * in this string if needed.
 *
 * NOTE: These functions are not thread-safe!
 */
const char *bt_hex_real(const void *buf, size_t len);
const char *bt_addr_str_real(const bt_addr_t *addr);
const char *bt_addr_le_str_real(const bt_addr_le_t *addr);
const char *bt_uuid_str_real(const struct bt_uuid *uuid);

/* NOTE: log_strdup does not guarantee a duplication of the string.
 * It is therefore still the responsibility of the user to handle the
 * restrictions in the underlying function call.
 */
 /* !should be optimized */
#define log_strdup(x) (x)

#define bt_hex(buf, len) log_strdup(bt_hex_real(buf, len))
#define bt_addr_str(addr) log_strdup(bt_addr_str_real(addr))
#define bt_addr_le_str(addr) log_strdup(bt_addr_le_str_real(addr))
#define bt_uuid_str(uuid) log_strdup(bt_uuid_str_real(uuid))

#ifdef __cplusplus
}
#endif

#endif /* __BT_LOG_H */

