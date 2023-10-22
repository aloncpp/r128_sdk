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

#if defined CONFIG_MACH_SUN20IW2
#include "sun20iw2p1/irqs-sun20iw2p1.h"
#define AW_IRQ_DMA     13
#define AW_IRQ_TIMER0  20
#define AW_IRQ_TIMER1  21
#define AW_IRQ_USB_OTG 65
#endif

#define NR_IRQS         (SUNXI_IRQ_MAX)

#ifdef __cplusplus
}
#endif

#endif /* __SUNXI_MACH_IRQS_H_ */
