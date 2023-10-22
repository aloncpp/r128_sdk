/* drivers/video/sunxi/disp2/disp/lcd/kd070d57.c
 *
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: anruliu <anruliu@allwinnertech.com>
 *
 * kd070d57 panel driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

lcd_used            = <1>;

lcd_driver_name     = "kd070d57";
lcd_backlight       = <50>;
lcd_if              = <4>;

lcd_x               = <600>;
lcd_y               = <1024>;
lcd_width           = <89>;
lcd_height          = <152>;
lcd_dclk_freq       = <47>;

lcd_pwm_used        = <1>;
lcd_pwm_ch          = <3>;
lcd_pwm_freq        = <50000>;
lcd_pwm_pol         = <0>;
lcd_pwm_max_limit   = <255>;

lcd_hbp             = <64>;
lcd_ht              = <742>;
lcd_hspw            = <4>;
lcd_vbp             = <28>;
lcd_vt              = <1086>;
lcd_vspw            = <4>;

lcd_dsi_lane        = <4>;
lcd_lvds_if         = <0>;
lcd_lvds_colordepth = <0>;
lcd_lvds_mode       = <0>;
lcd_frm             = <0>;
lcd_hv_clk_phase    = <0>;
lcd_hv_sync_polarity= <0>;
lcd_gamma_en        = <0>;
lcd_bright_curve_en = <0>;
lcd_cmap_en         = <0>;
lcd_fsync_en        = <0>;
lcd_fsync_act_time  = <1000>;
lcd_fsync_dis_time  = <1000>;
lcd_fsync_pol       = <0>;

deu_mode            = <0>;
lcdgamma4iep        = <22>;
smart_color         = <90>;

lcd_pin_power = "cldo2";
lcd_pin_power1 = "bldo4";

lcd_power = "dc1sw";
lcd_gpio_0 = <&pio PD 19 1 0 3 1>;
pinctrl-0 = <&dsi4lane_pins_a>;
pinctrl-1 = <&dsi4lane_pins_b>;

 */
#include "kd070d57.h"

static void lcd_power_on(u32 sel);
static void lcd_power_off(u32 sel);
static void lcd_bl_open(u32 sel);
static void lcd_bl_close(u32 sel);

static void lcd_panel_init(u32 sel);
static void lcd_panel_exit(u32 sel);

#define panel_reset(sel, val) sunxi_lcd_gpio_set_value(sel, 0, val)

static void lcd_cfg_panel_info(panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[][2] = { { 0, 0 }, { 15, 15 }, { 30, 30 }, { 45, 45 }, {
			60, 60 }, { 75, 75 }, { 90, 90 }, { 105, 105 }, { 120, 120 }, { 135,
			135 }, { 150, 150 }, { 165, 165 }, { 180, 180 }, { 195, 195 }, {
			210, 210 }, { 225, 225 }, { 240, 240 }, { 255, 255 }, };

	u32 lcd_cmap_tbl[2][3][4] = { { { LCD_CMAP_G0, LCD_CMAP_B1, LCD_CMAP_G2,
			LCD_CMAP_B3 },
			{ LCD_CMAP_B0, LCD_CMAP_R1, LCD_CMAP_B2, LCD_CMAP_R3 }, {
					LCD_CMAP_R0, LCD_CMAP_G1, LCD_CMAP_R2, LCD_CMAP_G3 }, }, { {
			LCD_CMAP_B3, LCD_CMAP_G2, LCD_CMAP_B1, LCD_CMAP_G0 }, { LCD_CMAP_R3,
			LCD_CMAP_B2, LCD_CMAP_R1, LCD_CMAP_B0 }, { LCD_CMAP_G3, LCD_CMAP_R2,
			LCD_CMAP_G1, LCD_CMAP_R0 }, }, };

	items = sizeof(lcd_gamma_tbl) / 2;
	for (i = 0; i < items - 1; i++) {
		u32 num = lcd_gamma_tbl[i + 1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1]
					+ ((lcd_gamma_tbl[i + 1][1] - lcd_gamma_tbl[i][1]) * j)
							/ num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] = (value << 16)
					+ (value << 8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items - 1][1] << 16)
			+ (lcd_gamma_tbl[items - 1][1] << 8) + lcd_gamma_tbl[items - 1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 lcd_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, lcd_power_on, 10);
	LCD_OPEN_FUNC(sel, lcd_panel_init, 10);
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 50);
	LCD_OPEN_FUNC(sel, lcd_bl_open, 0);

	return 0;
}

static s32 lcd_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, lcd_bl_close, 0);
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);
	LCD_CLOSE_FUNC(sel, lcd_panel_exit, 200);
	LCD_CLOSE_FUNC(sel, lcd_power_off, 500);

	return 0;
}

static void lcd_power_on(u32 sel)
{
	sunxi_lcd_power_enable(sel, 0);
	sunxi_lcd_delay_ms(10);
	sunxi_lcd_pin_cfg(sel, 1);
	sunxi_lcd_delay_ms(50);
	panel_reset(sel, 1);
	sunxi_lcd_delay_ms(20);
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(20);
	panel_reset(sel, 1);
	sunxi_lcd_delay_ms(20);

}

static void lcd_power_off(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	sunxi_lcd_delay_ms(20);
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 0);
}

static void lcd_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	/* sunxi_lcd_backlight_enable(sel); */
}

static void lcd_bl_close(u32 sel)
{
	/* sunxi_lcd_backlight_disable(sel); */
	sunxi_lcd_pwm_disable(sel);
}

#define REGFLAG_DELAY 0XFC
#define REGFLAG_END_OF_TABLE 0xFD /* END OF REGISTERS MARKER */

struct LCM_setting_table {
	u8 cmd;
	u32 count;
	u8 para_list[64];
};

/*add panel initialization below*/

static struct LCM_setting_table lcm_kd070d57_setting[] = {
		{ 0x11, 0, { } },    // SLPOUT
		{ REGFLAG_DELAY, REGFLAG_DELAY, {120} },
		{ 0x29, 0, { } },
		{ REGFLAG_DELAY, REGFLAG_DELAY, {20} },
		{ REGFLAG_END_OF_TABLE, REGFLAG_END_OF_TABLE, {0x00} }
};

static void lcd_panel_init(u32 sel)
{
	__u32 i;
	sunxi_lcd_dsi_clk_enable(sel);
	sunxi_lcd_delay_ms(10);

	for (i = 0;; i++) {
		if (lcm_kd070d57_setting[i].count == REGFLAG_END_OF_TABLE)
			break;
		else if (lcm_kd070d57_setting[i].count == REGFLAG_DELAY) {
			sunxi_lcd_delay_ms(lcm_kd070d57_setting[i].para_list[0]);
		} else
			dsi_dcs_wr(sel, lcm_kd070d57_setting[i].cmd,
					lcm_kd070d57_setting[i].para_list,
					lcm_kd070d57_setting[i].count);
		/* break; */
	}


}

static void lcd_panel_exit(u32 sel)
{
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x10);
	sunxi_lcd_delay_ms(80);
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x28);
	sunxi_lcd_delay_ms(50);

}

/*sel: 0:lcd0; 1:lcd1*/
static s32 lcd_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

__lcd_panel_t kd070d57_panel = {
/* panel driver name, must mach the name of
 * lcd_drv_name in sys_config.fex
 */
	.name = "kd070d57",
	.func = {
		.cfg_panel_info = lcd_cfg_panel_info,
		.cfg_open_flow = lcd_open_flow,
		.cfg_close_flow = lcd_close_flow,
		.lcd_user_defined_func = lcd_user_defined_func,
	},
};
