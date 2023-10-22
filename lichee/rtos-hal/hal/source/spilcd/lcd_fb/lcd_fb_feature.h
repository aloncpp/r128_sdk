/*
 * drivers/rtos-hal/hal/source/spilcd/lcd_fb/lcd_fb_feature.h
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _LCD_FB_FEATURE_H
#define _LCD_FB_FEATURE_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined (CONFIG_ARCH_SUN50IW11) || defined(CONFIG_ARCH_SUN8IW20) || defined(CONFIG_ARCH_SUN20IW1) \
	|| defined(CONFIG_ARCH_SUN20IW2)
#define SUPPORT_DBI_IF
#endif

#ifdef __cplusplus
}
#endif

#endif /*End of file*/
