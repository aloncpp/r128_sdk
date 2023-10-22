/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __PANEL_H__
#define __PANEL_H__
#include "../include.h"
#include "lcd_source.h"
#include "../disp_display.h"
#include "../disp_lcd.h"

struct __lcd_panel {
	char name[32];
	struct disp_lcd_panel_fun func;
};

extern struct __lcd_panel *panel_array[];

struct sunxi_lcd_drv {
	struct sunxi_disp_source_ops src_ops;
};

extern int sunxi_disp_get_source_ops(struct sunxi_disp_source_ops *src_ops);
int lcd_init(void);
#ifdef CONFIG_LCD_SUPPORT_KLD35512
extern struct __lcd_panel kld35512_panel;
#endif

#ifdef CONFIG_LCD_SUPPORT_KLD39501
extern struct __lcd_panel kld39501_panel;
#endif

#ifdef CONFIG_LCD_SUPPORT_KLD2844B
extern struct __lcd_panel kld2844b_panel;
#endif

#ifdef CONFIG_LCD_SUPPORT_NV3029S
extern struct __lcd_panel nv3029s_panel;
#endif


#endif
