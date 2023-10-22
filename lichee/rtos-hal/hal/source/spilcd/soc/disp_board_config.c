/*
 * disp_board_config.c
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <hal_clk.h>
#include <hal_gpio.h>
#include "../lcd_fb/lcd_fb_intf.h"
#include "disp_board_config.h"

#ifdef CONFIG_DRIVER_SYSCONFIG
#include <script.h>
#include <hal_cfg.h>
#endif

#define MAX_PIN_NUM 32

#ifndef CONFIG_DRIVER_SYSCONFIG
extern struct property_t g_lcd0_config[];
extern struct property_t g_lcd1_config[];
extern struct property_t g_disp_config[];
extern u32 g_disp_config_len;
extern u32 g_lcd0_config_len;
extern u32 g_lcd1_config_len;

static struct property_t *disp_get_board_config(const char *main_name, u32 *len)
{
	u32 i = 0, j = 0;
	struct property_t *cur = NULL;

	if (!main_name || !len) {
		goto OUT;
	}

	if (!strcmp(main_name, "lcd_fb0")) {
		cur = g_lcd0_config;
		*len = g_lcd0_config_len;
	} else if (!strcmp(main_name, "lcd_fb1")) {
		cur = g_lcd1_config;
		*len = g_lcd1_config_len;
	}
OUT:
	return cur;
}
#endif

u32 disp_get_property_value(const char *main_name, const char *sub_name,
		void *value, u8 type)
{
#ifdef CONFIG_DRIVER_SYSCONFIG
	int ret;
	int gpio_count = 0;
	int i;
	user_gpio_set_t gpiocfg[32] = {0};
	char sec_name[32] = {0};
	char key_name[32] = {0};

	strcpy(sec_name, main_name);
	strcpy(key_name, sub_name);

	//lcd_fb_wrn("sec_name :%s, sub_name:%s\n", sec_name, sub_name);

	if (type == PROPERTY_INTGER) {
		/* get integer */
		if (hal_cfg_get_keyvalue(sec_name, key_name, value, 1)) {
			lcd_fb_inf("of_property_read_u32_array %s.%s fail\n",
			      sec_name, key_name);
			goto not_found;
		} else
			ret = type;
		lcd_fb_inf("%s,%s: value:%d\n",sec_name,key_name,*((int*)value));
	} else if (type == PROPERTY_STRING) {
		/* get string */
		char str[40] = {0};

		if (hal_cfg_get_keyvalue(sec_name, key_name, (void *)str, sizeof(str))) {
			lcd_fb_inf("hal get value %s.%s fail\n", sec_name,
			      key_name);
			goto not_found;
		} else {
			ret = type;
			memcpy((void *)value, (void *)str, strlen(str) + 1);
		}
		lcd_fb_inf("%s,%s: value:%s\n",sec_name,key_name,value);
	} else if (type == PROPERTY_GPIO) {
		/* get gpio */
		gpio_count = hal_cfg_get_gpiosec_keycount(sec_name);
		/* get all gpio in sec_name */
		hal_cfg_get_gpiosec_data(sec_name, gpiocfg, gpio_count);
		for (i = 0; i < gpio_count; i++) {
			if (!strcmp(gpiocfg[i].name, key_name)) {
				struct disp_gpio_set_t *gpio_info = (struct disp_gpio_set_t *) value;
				gpio_info->gpio = (gpiocfg[i].port - 1) * 32 + gpiocfg[i].port_num;
				gpio_info->port = gpiocfg[i].port;
				gpio_info->port_num = gpiocfg[i].port_num;
				gpio_info->mul_sel = gpiocfg[i].mul_sel;
				gpio_info->pull = gpiocfg[i].pull;
				gpio_info->drv_level = gpiocfg[i].drv_level;
				gpio_info->data = gpiocfg[i].data;
				memcpy(gpio_info->gpio_name, key_name, strlen(key_name) + 1);
				return PROPERTY_GPIO;
			}
		}
		ret = PROPERTY_UNDEFINED;
	} else {
		ret = PROPERTY_UNDEFINED;
	}

exit:
	return ret;
not_found:
	return PROPERTY_UNDEFINED;

#else
	u32 i = 0, len = 0;
	struct property_t *cur = NULL;
	if (!main_name || !sub_name || !value)
		return 0;

	cur = disp_get_board_config(main_name, &len);
	if (!cur || !len)
		goto OUT;

	for (i = 0; i < len; ++i) {
		if(!strcmp(sub_name, cur[i].name)) {
			if (cur[i].type == PROPERTY_INTGER) {
				*(u32 *)value = cur[i].v.value;
				lcd_fb_wrn("%s,%s: value:%d\n",main_name, sub_name, cur[i].v.value);
			} else if (cur[i].type == PROPERTY_STRING) {
				memcpy(value, cur[i].v.str, strlen(cur[i].v.str) + 1);
				lcd_fb_wrn("%s,%s: value:%s\n",main_name, sub_name, value);
			} else if (cur[i].type == PROPERTY_GPIO) {
				struct disp_gpio_set_t *gpio_info =
					(struct disp_gpio_set_t *)value;
				gpio_info->gpio = cur[i].v.gpio_list.gpio;
				gpio_info->mul_sel = cur[i].v.gpio_list.mul_sel;
				gpio_info->pull = cur[i].v.gpio_list.pull;
				gpio_info->drv_level = cur[i].v.gpio_list.drv_level;
				gpio_info->data = cur[i].v.gpio_list.data;
				memcpy(gpio_info->gpio_name, sub_name, strlen(sub_name) + 1);
			} else if (cur[i].type == PROPERTY_POWER) {
				struct disp_power_t *power =
					(struct disp_power_t *)value;
				memcpy(power, &cur[i].v.power, sizeof(struct disp_power_t));
			}
			return cur[i].type;
		}
	}
OUT:
	return PROPERTY_UNDEFINED;
#endif
}

struct disp_gpio_set_t *disp_get_all_pin_property(const char *main_name,
					     u32 *list_len)
{
#ifdef CONFIG_DRIVER_SYSCONFIG
	return NULL;
#else
	static struct disp_gpio_set_t gpio_list[MAX_PIN_NUM];
	u32 i = 0, len = 0, j = 0;
	struct property_t *cur = NULL;
	memset(gpio_list, 0, 30 * sizeof(struct disp_gpio_set_t));

	if (!main_name)
		return NULL;
	cur = disp_get_board_config(main_name, &len);
	if (!cur || !len)
		return NULL;

	for (i = 0, j = 0; i < len; ++i) {
		if (cur[i].type == PROPERTY_PIN) {
			if (j == MAX_PIN_NUM)
				return NULL;
			memcpy(&gpio_list[j], &cur[i].v.gpio_list,
			       sizeof(struct disp_gpio_set_t));
			++j;
		}
	}

	*list_len = j;

	return gpio_list;
#endif
}
