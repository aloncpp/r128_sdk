/*
 * include/arch/arm/mach/irqs.h
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

#ifndef __SUNXI_MACH_IRQS_H_
#define __SUNXI_MACH_IRQS_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined CONFIG_ARCH_SUN20IW2P1
#include "sun20iw2p1/irqs-sun20iw2p1.h"
#endif

#if defined CONFIG_ARCH_SUN55IW3
#include "sun55iw3/irqs-sun55iw3.h"
#endif

#define NR_IRQS         (SUNXI_IRQ_MAX)

#ifdef __cplusplus
}
#endif

#endif /* __SUNXI_MACH_IRQS_H_ */
