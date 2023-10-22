/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "pdx040wqi_rgb_480.h"

static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);

static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

#define RESET_PIN GPIOEXP1(5)
#define	SCL_PIN GPIOEXP0(7)
#define SDA_PIN GPIOEXP0(6)
#define CS_PIN GPIOEXP0(3)

#define panel_reset(sel, val)  hal_gpio_set_data(RESET_PIN, val);
#define spi_scl_1 hal_gpio_set_data(SCL_PIN, 1)
#define spi_scl_0 hal_gpio_set_data(SCL_PIN, 0)
#define spi_sdi_1 hal_gpio_set_data(SDA_PIN, 1)
#define spi_sdi_0 hal_gpio_set_data(SDA_PIN, 0)

static void SPI_SendData(unsigned char i)
{
	unsigned char n;

	for(n=0; n<8; n++) {  
		if(i&0x80) spi_sdi_1;
		else spi_sdi_0;

		i<<= 1;
		spi_scl_0;
		spi_scl_1;
	}
}
static void st7701_spi_write_cmd(unsigned char i)
{
	spi_sdi_0;

	spi_scl_0;
	spi_scl_1;

	SPI_SendData(i);
}

static void st7701_spi_write_data(unsigned char i)
{
	spi_sdi_1;

	spi_scl_0;
	spi_scl_1;

	SPI_SendData(i);
}

static void LCD_cfg_panel_info(struct panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[][2] = {
		/* {input value, corrected value} */
		{0, 0},
		{15, 15},
		{30, 30},
		{45, 45},
		{60, 60},
		{75, 75},
		{90, 90},
		{105, 105},
		{120, 120},
		{135, 135},
		{150, 150},
		{165, 165},
		{180, 180},
		{195, 195},
		{210, 210},
		{225, 225},
		{240, 240},
		{255, 255},
	};

	u32 lcd_cmap_tbl[2][3][4] = {
		{
			{LCD_CMAP_G0, LCD_CMAP_B1, LCD_CMAP_G2, LCD_CMAP_B3},
			{LCD_CMAP_B0, LCD_CMAP_R1, LCD_CMAP_B2, LCD_CMAP_R3},
			{LCD_CMAP_R0, LCD_CMAP_G1, LCD_CMAP_R2, LCD_CMAP_G3},
		},
		{
			{LCD_CMAP_B3, LCD_CMAP_G2, LCD_CMAP_B1, LCD_CMAP_G0},
			{LCD_CMAP_R3, LCD_CMAP_B2, LCD_CMAP_R1, LCD_CMAP_B0},
			{LCD_CMAP_G3, LCD_CMAP_R2, LCD_CMAP_G1, LCD_CMAP_R0},
		},
	};

	items = sizeof(lcd_gamma_tbl) / 2;
	for (i = 0; i < items - 1; i++) {
		u32 num = lcd_gamma_tbl[i + 1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value =
			    lcd_gamma_tbl[i][1] +
			    ((lcd_gamma_tbl[i + 1][1] -
			      lcd_gamma_tbl[i][1]) * j) / num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
			    (value << 16) + (value << 8) + value;
		}
	}
	info->lcd_gamma_tbl[255] =
	    (lcd_gamma_tbl[items - 1][1] << 16) +
	    (lcd_gamma_tbl[items - 1][1] << 8) + lcd_gamma_tbl[items - 1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 LCD_open_flow(u32 sel)
{
	/* open lcd power, and delay 50ms */
	LCD_OPEN_FUNC(sel, LCD_power_on, 30);
	/* open lcd power, than delay 200ms */
	LCD_OPEN_FUNC(sel, LCD_panel_init, 50);
	/* open lcd controller, and delay 100ms */
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 100);
	/* open lcd backlight, and delay 0ms */
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);

	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	/* close lcd backlight, and delay 0ms */
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 0);
	/* close lcd controller, and delay 0ms */
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);
	/* open lcd power, than delay 200ms */
	LCD_CLOSE_FUNC(sel, LCD_panel_exit, 200);
	/* close lcd power, and delay 500ms */
	LCD_CLOSE_FUNC(sel, LCD_power_off, 500);

	return 0;
}

static void LCD_power_on(u32 sel)
{
	/* config lcd_power pin to open lcd power0 */
	sunxi_lcd_power_enable(sel, 0);
	sunxi_lcd_pin_cfg(sel, 1);
}

static void LCD_power_off(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	/* config lcd_power pin to close lcd power0 */
	sunxi_lcd_power_disable(sel, 0);
}

static void LCD_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_backlight_enable(sel);
}

static void LCD_bl_close(u32 sel)
{
	/* config lcd_bl_en pin to close lcd backlight */
	sunxi_lcd_backlight_disable(sel);
	sunxi_lcd_pwm_disable(sel);
}

static void LCD_panel_init(u32 sel)
{
	hal_gpio_set_direction(RESET_PIN, GPIO_DIRECTION_OUTPUT);
	hal_gpio_set_direction(SCL_PIN, GPIO_DIRECTION_OUTPUT);
	hal_gpio_set_direction(SDA_PIN, GPIO_DIRECTION_OUTPUT);
	hal_gpio_set_direction(CS_PIN, GPIO_DIRECTION_OUTPUT);
	hal_gpio_set_data(CS_PIN, 0);
	hal_gpio_set_data(SCL_PIN, 1);
	hal_gpio_set_data(SDA_PIN, 1);

	panel_reset(0, 1);
	sunxi_lcd_delay_ms(120);
	panel_reset(0, 0);
	sunxi_lcd_delay_ms(20);
	panel_reset(0, 1);
	sunxi_lcd_delay_ms(120);

	st7701_spi_write_cmd(0x11);
	sunxi_lcd_delay_ms(120);
	st7701_spi_write_cmd(0x21);
	sunxi_lcd_delay_ms(120);

	st7701_spi_write_cmd(0xff);
	st7701_spi_write_data(0x77);
	st7701_spi_write_data(0x01);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x10);
	st7701_spi_write_cmd(0xc0);
	st7701_spi_write_data(0x3b);
	st7701_spi_write_data(0x00);
	st7701_spi_write_cmd(0xc1);
	st7701_spi_write_data(0x0d);
	st7701_spi_write_data(0x02);
	st7701_spi_write_cmd(0xc2);
	st7701_spi_write_data(0x21);
	st7701_spi_write_data(0x08);

	// RGB分布
	st7701_spi_write_cmd(0xcd);
	st7701_spi_write_data(0x08);

	st7701_spi_write_cmd(0xb0);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x11);
	st7701_spi_write_data(0x18);
	st7701_spi_write_data(0x0e);
	st7701_spi_write_data(0x11);
	st7701_spi_write_data(0x06);
	st7701_spi_write_data(0x07);
	st7701_spi_write_data(0x08);
	st7701_spi_write_data(0x07);
	st7701_spi_write_data(0x22);
	st7701_spi_write_data(0x04);
	st7701_spi_write_data(0x12);
	st7701_spi_write_data(0x0f);
	st7701_spi_write_data(0xaa);
	st7701_spi_write_data(0x31);
	st7701_spi_write_data(0x18);
	st7701_spi_write_cmd(0xb1);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x11);
	st7701_spi_write_data(0x19);
	st7701_spi_write_data(0x0e);
	st7701_spi_write_data(0x12);
	st7701_spi_write_data(0x07);
	st7701_spi_write_data(0x08);
	st7701_spi_write_data(0x08);
	st7701_spi_write_data(0x08);
	st7701_spi_write_data(0x22);
	st7701_spi_write_data(0x04);
	st7701_spi_write_data(0x11);
	st7701_spi_write_data(0x11);
	st7701_spi_write_data(0xa9);
	st7701_spi_write_data(0x32);
	st7701_spi_write_data(0x18);
	st7701_spi_write_cmd(0xff);
	st7701_spi_write_data(0x77);
	st7701_spi_write_data(0x01);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x11);
	st7701_spi_write_cmd(0xb0);
	st7701_spi_write_data(0x60);
	st7701_spi_write_cmd(0xb1);
	st7701_spi_write_data(0x30);
	st7701_spi_write_cmd(0xb2);
	st7701_spi_write_data(0x87);
	st7701_spi_write_cmd(0xb3);
	st7701_spi_write_data(0x80);
	st7701_spi_write_cmd(0xb5);
	st7701_spi_write_data(0x49);
	st7701_spi_write_cmd(0xb7);
	st7701_spi_write_data(0x85);
	st7701_spi_write_cmd(0xb8);
	st7701_spi_write_data(0x21);
	st7701_spi_write_cmd(0xc1);
	st7701_spi_write_data(0x02);
	st7701_spi_write_data(0x0a);
	st7701_spi_write_cmd(0xc2);
	st7701_spi_write_data(0x78);
	sunxi_lcd_delay_ms(20);
	st7701_spi_write_cmd(0xe0);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x1b);
	st7701_spi_write_data(0x02);
	st7701_spi_write_cmd(0xe1);
	st7701_spi_write_data(0x08);
	st7701_spi_write_data(0xa0);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x07);
	st7701_spi_write_data(0xa0);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x44);
	st7701_spi_write_data(0x44);
	st7701_spi_write_cmd(0xe2);
	st7701_spi_write_data(0x11);
	st7701_spi_write_data(0x11);
	st7701_spi_write_data(0x44);
	st7701_spi_write_data(0x44);
	st7701_spi_write_data(0xed);
	st7701_spi_write_data(0xa0);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0xec);
	st7701_spi_write_data(0xa0);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x00);
	st7701_spi_write_cmd(0xe3);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x11);
	st7701_spi_write_data(0x11);
	st7701_spi_write_cmd(0xe4);
	st7701_spi_write_data(0x44);
	st7701_spi_write_data(0x44);
	st7701_spi_write_cmd(0xe5);
	st7701_spi_write_data(0x0a);
	st7701_spi_write_data(0xe9);
	st7701_spi_write_data(0xd8);
	st7701_spi_write_data(0xa0);
	st7701_spi_write_data(0x0c);
	st7701_spi_write_data(0xeb);
	st7701_spi_write_data(0xd8);
	st7701_spi_write_data(0xa0);
	st7701_spi_write_data(0x0e);
	st7701_spi_write_data(0xed);
	st7701_spi_write_data(0xd8);
	st7701_spi_write_data(0xa0);
	st7701_spi_write_data(0x10);
	st7701_spi_write_data(0xef);
	st7701_spi_write_data(0xd8);
	st7701_spi_write_data(0xa0);
	st7701_spi_write_cmd(0xe6);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x11);
	st7701_spi_write_data(0x11);
	st7701_spi_write_cmd(0xe7);
	st7701_spi_write_data(0x44);
	st7701_spi_write_data(0x44);
	st7701_spi_write_cmd(0xe8);
	st7701_spi_write_data(0x09);
	st7701_spi_write_data(0xe8);
	st7701_spi_write_data(0xd8);
	st7701_spi_write_data(0xa0);
	st7701_spi_write_data(0x0b);
	st7701_spi_write_data(0xea);
	st7701_spi_write_data(0xd8);
	st7701_spi_write_data(0xa0);
	st7701_spi_write_data(0x0d);
	st7701_spi_write_data(0xec);
	st7701_spi_write_data(0xd8);
	st7701_spi_write_data(0xa0);
	st7701_spi_write_data(0x0f);
	st7701_spi_write_data(0xee);
	st7701_spi_write_data(0xd8);
	st7701_spi_write_data(0xa0);
	st7701_spi_write_cmd(0xeb);
	st7701_spi_write_data(0x02);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0xe4);
	st7701_spi_write_data(0xe4);
	st7701_spi_write_data(0x88);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x40);
	st7701_spi_write_cmd(0xec);
	st7701_spi_write_data(0x3c);
	st7701_spi_write_data(0x00);
	st7701_spi_write_cmd(0xed);
	st7701_spi_write_data(0xab);
	st7701_spi_write_data(0x89);
	st7701_spi_write_data(0x76);
	st7701_spi_write_data(0x54);
	st7701_spi_write_data(0x02);
	st7701_spi_write_data(0xff);
	st7701_spi_write_data(0xff);
	st7701_spi_write_data(0xff);
	st7701_spi_write_data(0xff);
	st7701_spi_write_data(0xff);
	st7701_spi_write_data(0xff);
	st7701_spi_write_data(0x20);
	st7701_spi_write_data(0x45);
	st7701_spi_write_data(0x67);
	st7701_spi_write_data(0x98);
	st7701_spi_write_data(0xba);

#if 0
	// test mode
	st7701_spi_write_cmd(0xff);
	st7701_spi_write_data(0x77);
	st7701_spi_write_data(0x01);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x12);
	st7701_spi_write_cmd(0xd1);
	st7701_spi_write_data(0x81);
	st7701_spi_write_cmd(0xd2);
	st7701_spi_write_data(0x08);
#endif

	st7701_spi_write_cmd(0xff);
	st7701_spi_write_data(0x77);
	st7701_spi_write_data(0x01);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x00);
	st7701_spi_write_data(0x00);
	st7701_spi_write_cmd(0x3a);
	st7701_spi_write_data(0x66);
	st7701_spi_write_cmd(0x36);
	st7701_spi_write_data(0x00);
	st7701_spi_write_cmd(0x29);
	sunxi_lcd_delay_ms(20);
}

static void LCD_panel_exit(u32 sel)
{
}

/* sel: 0:lcd0; 1:lcd1 */
static s32 LCD_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

struct __lcd_panel pdx040wqi_rgb_480_panel = {
	/* panel driver name, must mach the lcd_drv_name in sys_config.fex */
	.name = "pdx040wqi_rgb_480",
	.func = {
		 .cfg_panel_info = LCD_cfg_panel_info,
		 .cfg_open_flow = LCD_open_flow,
		 .cfg_close_flow = LCD_close_flow,
		 .lcd_user_defined_func = LCD_user_defined_func,
		 }
	,
};
