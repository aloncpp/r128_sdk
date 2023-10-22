/*
 * include/arch/arm/mach/sun20iw2p1/platform-sun20iw2p1.h
 *
 * Copyright (c) 2020-2021 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: huangshr <huangshr@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __PLATFORM_SUN20IW2P1_H_
#define __PLATFORM_SUN20IW2P1_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_PLATFORM_FPGA
#define LOSC_CLOCK          (32000U)
#else
#define LOSC_CLOCK          (32768U)
#endif

#define SYS_LFCLOCK         LOSC_CLOCK

#ifdef __cplusplus
}
#endif

#endif /* __PLATFORM_SUN20IW2P1_H_ */
