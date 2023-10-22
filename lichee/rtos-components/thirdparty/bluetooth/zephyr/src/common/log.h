/** @file
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
//#include <offsets.h>
#include <zephyr.h>
//#include <logging/log.h>
#include <sys/__assert.h>

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
#define LOG_LEVEL LOG_LEVEL_DBG
#else
#define LOG_LEVEL CONFIG_BT_LOG_LEVEL
#endif


#if /*defined*//*Wewisetek Modified*/(SYS_LOG_DOMAIN)
#define SYS_LOG_DOMAIN "bt"
#endif

#include <logging/sys_log.h>

#define BT_DBG(fmt, ...) LOG_DBG(fmt, ##__VA_ARGS__)
#define BT_ERR(fmt, ...) LOG_ERR(fmt, ##__VA_ARGS__)
#define BT_WARN(fmt, ...) LOG_WRN(fmt, ##__VA_ARGS__)
#define BT_INFO(fmt, ...) LOG_INF(fmt, ##__VA_ARGS__)

/* Enabling debug increases stack size requirement considerably */
#define BT_STACK_DEBUG_EXTRA	300


#if defined(CONFIG_BT_ASSERT_VERBOSE)
#define BT_ASSERT_PRINT(test) __ASSERT_LOC(test)
#define BT_ASSERT_PRINT_MSG(fmt, ...) __ASSERT_MSG_INFO(fmt, ##__VA_ARGS__)
#else
#define BT_ASSERT_PRINT(test)
#define BT_ASSERT_PRINT_MSG(fmt, ...)
#endif /* CONFIG_BT_ASSERT_VERBOSE */

#if defined(CONFIG_BT_ASSERT_PANIC)
#define BT_ASSERT_DIE k_panic
#else
#define BT_ASSERT_DIE k_oops
#endif /* CONFIG_BT_ASSERT_PANIC */

#if defined(CONFIG_BT_ASSERT)
#define BT_ASSERT(cond)                          \
	do {                                     \
		if (!(cond)) {                   \
			BT_ASSERT_PRINT(cond);   \
			BT_ASSERT_DIE();         \
		}                                \
	} while (0)

#define BT_ASSERT_MSG(cond, fmt, ...)                              \
	do {                                                       \
		if (!(cond)) {                                     \
			BT_ASSERT_PRINT(cond);                     \
			BT_ASSERT_PRINT_MSG(fmt, ##__VA_ARGS__);   \
			BT_ASSERT_DIE();                           \
		}                                                  \
	} while (0)
#else
#define BT_ASSERT(cond) __ASSERT_NO_MSG(cond)
#define BT_ASSERT_MSG(cond, msg, ...) __ASSERT(cond, msg, ##__VA_ARGS__)
#endif/* CONFIG_BT_ASSERT*/

extern void print_hex_dump_bytes(const void *addr, size_t len);

#define BT_HEXDUMP_DBG(_data, _length, _str) \
	if (BT_DBG_ENABLED) { \
		BT_DBG("%s\n", _str); \
		print_hex_dump_bytes(_data, _length); \
	}

#define TP printf("<%s : %d>\n", __func__, __LINE__)


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