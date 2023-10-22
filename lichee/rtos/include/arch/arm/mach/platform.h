/*
 * include/arch/arm/mach/platform.h
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

#ifndef __SUNXI_PLATFORM_H_
#define __SUNXI_PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined CONFIG_ARCH_SUN8IW18P1
#include "sun8iw18p1/platform-sun8iw18p1.h"
#endif

#if defined CONFIG_ARCH_SUN20IW2P1
#include "sun20iw2p1/platform-sun20iw2p1.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* __SUNXI_PLATFORM_H_ */
