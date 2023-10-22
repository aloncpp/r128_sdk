/*
 * Allwinner SoCs display driver.
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
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

#include "include.h"
#ifndef __DISP_DISPLAY_H__
#define __DISP_DISPLAY_H__

struct disp_dev_t {
	u32 print_level;
	u32 lcd_registered[3];
};

extern struct disp_dev_t gdisp;
void LCD_OPEN_FUNC(u32 screen_id, LCD_FUNC func, u32 delay);
void LCD_CLOSE_FUNC(u32 screen_id, LCD_FUNC func, u32 delay);
s32 bsp_disp_get_screen_physical_width(u32 disp);
s32 bsp_disp_get_screen_physical_height(u32 disp);
s32 bsp_disp_get_screen_width(u32 disp);
s32 bsp_disp_get_screen_height(u32 disp);
/* lcd */
s32 bsp_disp_lcd_set_panel_funs(char *name, struct disp_lcd_panel_fun *lcd_cfg);
s32 bsp_disp_lcd_backlight_enable(u32 disp);
s32 bsp_disp_lcd_backlight_disable(u32 disp);
s32 bsp_disp_lcd_pwm_enable(u32 disp);
s32 bsp_disp_lcd_pwm_disable(u32 disp);
s32 bsp_disp_lcd_power_enable(u32 disp, u32 power_id);
s32 bsp_disp_lcd_power_disable(u32 disp, u32 power_id);
s32 bsp_disp_lcd_set_bright(u32 disp, u32 bright);
s32 bsp_disp_lcd_get_bright(u32 disp);
s32 bsp_disp_lcd_pin_cfg(u32 disp, u32 en);
s32 bsp_disp_lcd_gpio_set_value(u32 disp, u32 io_index, u32 value);
s32 bsp_disp_lcd_gpio_set_direction(u32 disp, unsigned int io_index,
				    u32 direction);
struct disp_lcd_flow *bsp_disp_lcd_get_open_flow(u32 disp);
struct disp_lcd_flow *bsp_disp_lcd_get_close_flow(u32 disp);
s32 bsp_disp_get_panel_info(u32 disp, struct disp_panel_para *info);

int bsp_disp_get_display_size(u32 disp, unsigned int *width,
			      unsigned int *height);
void *lcd_fb_dma_malloc(u32 num_bytes);
void lcd_fb_dma_free(void *addr);
struct lcd_fb_device *lcd_fb_device_get(int disp);
s32 lcd_fb_device_register(struct lcd_fb_device *dispdev);
s32 lcd_fb_device_unregister(struct lcd_fb_device *dispdev);
s32 bsp_disp_lcd_set_layer(unsigned int disp, struct fb_info *p_info);
s32 bsp_disp_lcd_blank(unsigned int disp, unsigned int en);
s32 bsp_disp_lcd_set_var(unsigned int disp, struct fb_info *p_info);
s32 bsp_disp_lcd_cmd_write(unsigned int screen_id,
					 unsigned char cmd);
s32 bsp_disp_lcd_para_write(unsigned int screen_id,
					 unsigned char para);
s32 bsp_disp_lcd_cmd_read(unsigned int screen_id, unsigned char cmd,
			  unsigned char *rx_buf, unsigned char len);
s32 bsp_disp_lcd_wait_for_vsync(unsigned int disp);

#endif
