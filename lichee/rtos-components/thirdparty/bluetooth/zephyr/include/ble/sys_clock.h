/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Variables needed needed for system clock
 *
 *
 * Declare variables used by both system timer device driver and kernel
 * components that use timer functionality.
 */

#ifndef ZEPHYR_INCLUDE_SYS_CLOCK_H_
#define ZEPHYR_INCLUDE_SYS_CLOCK_H_

#include <ble/sys/util.h>
#include <ble/sys/dlist.h>

#include <toolchain.h>
//#include <zephyr/types.h>

//#include <sys/time_units.h>

#include "kernel/os/os.h"

#define OS_GetTicks XR_OS_GetTicks

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup clock_apis
 * @{
 */

/**
 * @brief Tick precision used in timeout APIs
 *
 * This type defines the word size of the timeout values used in
 * k_timeout_t objects, and thus defines an upper bound on maximum
 * timeout length (or equivalently minimum tick duration).  Note that
 * this does not affect the size of the system uptime counter, which
 * is always a 64 bit count of ticks.
 */
#ifdef CONFIG_TIMEOUT_64BIT
typedef int64_t k_ticks_t;
#else
typedef uint32_t k_ticks_t;
#endif

#define K_TICKS_FOREVER ((k_ticks_t) -1)

/**
 * @brief Kernel timeout type
 *
 * Timeout arguments presented to kernel APIs are stored in this
 * opaque type, which is capable of representing times in various
 * formats and units.  It should be constructed from application data
 * using one of the macros defined for this purpose (e.g. `K_MSEC()`,
 * `K_TIMEOUT_ABS_TICKS()`, etc...), or be one of the two constants
 * K_NO_WAIT or K_FOREVER.  Applications should not inspect the
 * internal data once constructed.  Timeout values may be compared for
 * equality with the `K_TIMEOUT_EQ()` macro.
 */
#define SYS_FOREVER_MS (-1)

/** @brief System-wide macro to convert milliseconds to kernel timeouts
 */
#define SYS_TIMEOUT_MS(ms) ((ms) == SYS_FOREVER_MS ? K_FOREVER : K_MSEC(ms))

/* Legacy timeout API */
typedef int32_t k_timeout_t;
#define K_TIMEOUT_EQ(a, b) ((a) == (b))
#define Z_TIMEOUT_NO_WAIT 0
#define Z_TIMEOUT_TICKS(t) OS_TicksToMSecs(t)
#define Z_FOREVER K_TICKS_FOREVER
#define Z_TIMEOUT_MS(t) OS_MSecsToTicks(t)
#define Z_TIMEOUT_US(t) ((999 + OS_MSecsToTicks(t)) / 1000)
#define Z_TIMEOUT_NS(t) ((999999 + OS_MSecsToTicks(t)) / 1000000)
#define Z_TIMEOUT_CYC(t) k_cyc_to_ms_ceil32(MAX((t), 0))

/** @} */

#ifdef CONFIG_TICKLESS_KERNEL
extern int _sys_clock_always_on;
extern void z_enable_sys_clock(void);
#endif

#define CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC OS_HZ

#if defined(CONFIG_SYS_CLOCK_EXISTS) && \
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 0)
//TODO: check the usage of CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
//#error "SYS_CLOCK_HW_CYCLES_PER_SEC must be non-zero!"
#endif

/* number of nsec per usec */
#ifndef NSEC_PER_USEC
#define NSEC_PER_USEC 1000U
#endif

/* number of microseconds per millisecond */
#ifndef USEC_PER_MSEC
#define USEC_PER_MSEC 1000U
#endif

/* number of milliseconds per second */
#ifndef MSEC_PER_SEC
#define MSEC_PER_SEC 1000U
#endif

/* number of microseconds per second */
#ifndef USEC_PER_SEC
#define USEC_PER_SEC ((USEC_PER_MSEC) * (MSEC_PER_SEC))
#endif

/* number of nanoseconds per second */
#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC ((NSEC_PER_USEC) * (USEC_PER_MSEC) * (MSEC_PER_SEC))
#endif

/* kernel clocks */

#define __ticks_to_ms(t) __DEPRECATED_MACRO \
	k_ticks_to_ms_floor64((uint64_t)(t))
#define z_ms_to_ticks(t) \
	((int32_t)k_ms_to_ticks_ceil32((uint32_t)(t)))

/* added tick needed to account for tick in progress */
#define _TICK_ALIGN 1

/*
 * SYS_CLOCK_HW_CYCLES_TO_NS_AVG converts CPU clock cycles to nanoseconds
 * and calculates the average cycle time
 */
#define SYS_CLOCK_HW_CYCLES_TO_NS_AVG(X, NCYCLES) \
	(uint32_t)(k_cyc_to_ns_floor64(X) / NCYCLES)

/**
 * @defgroup clock_apis Kernel Clock APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @} end defgroup clock_apis
 */

/**
 *
 * @brief Return the lower part of the current system tick count
 *
 * @return the current system tick count
 *
 */
static inline uint32_t z_tick_get_32(void)
{
	return (uint32_t)OS_GetTicks();
}


/**
 *
 * @brief Return the current system tick count
 *
 * @return the current system tick count
 *
 */
static inline int64_t z_tick_get(void)
{
	return (int64_t)OS_GetTicks();
}


#ifndef CONFIG_SYS_CLOCK_EXISTS
#define z_tick_get() (0)
#define z_tick_get_32() (0)
#endif

uint64_t z_timeout_end_calc(k_timeout_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_CLOCK_H_ */
