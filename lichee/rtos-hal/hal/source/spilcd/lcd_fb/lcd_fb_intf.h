/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _LCD_FB_INTF_
#define _LCD_FB_INTF_

#include "include.h"
#include <sunxi_hal_regulator.h>
#include <hal_gpio.h>

#define SYS_CONFIG_USE 0

struct disp_gpio_set_t {
	char gpio_name[32];
	int port;
	int port_num;
	int mul_sel;
	int pull;
	int drv_level;
	int data;
	int gpio;
};

struct disp_pwm_dev {
	u32 pwm_channel_id;
	struct pwm_config cfg;
	bool enable;
};

/**
 * disp_power_t
 */

struct disp_power_t {
	char power_name[32];
	/*see sunxi_hal_regulator.h */
	enum REGULATOR_TYPE_ENUM power_type;
	enum REGULATOR_ID_ENUM power_id;
	/*unit:uV, 1V=1000000uV */
	u32 power_vol;
	bool always_on;
};

#define DISP_IRQ_RETURN IRQ_HANDLED
#define DISP_PIN_STATE_ACTIVE "active"
#define DISP_PIN_STATE_SLEEP "sleep"

int lcd_fb_register_irq(u32 IrqNo, u32 Flags, void *Handler, void *pArg,
			  u32 DataSize, u32 Prio);
void lcd_fb_unregister_irq(u32 IrqNo, void *Handler, void *pArg);
void lcd_fb_disable_irq(u32 IrqNo);
void lcd_fb_enable_irq(u32 IrqNo);

/* returns: 0:invalid, 1: int; 2:str, 3: gpio */
int lcd_fb_script_get_item(char *main_name, char *sub_name, int value[],
			     int count);

int lcd_fb_get_ic_ver(void);

int lcd_fb_gpio_request(struct disp_gpio_set_t *gpio_list,
			  u32 group_count_max);
int lcd_fb_gpio_request_simple(struct disp_gpio_set_t *gpio_list,
				 u32 group_count_max);
int lcd_fb_gpio_release(int p_handler, s32 if_release_to_default_status);

/* direction: 0:input, 1:output */
int lcd_fb_gpio_set_direction(u32 p_handler, u32 direction,
				const char *gpio_name);
int lcd_fb_gpio_get_value(u32 p_handler, const char *gpio_name);
int lcd_fb_gpio_set_value(u32 p_handler, u32 value_to_gpio,
			    const char *gpio_name);
int lcd_fb_pin_set_state(char *dev_name, char *name);

int lcd_fb_power_enable(char *name);
int lcd_fb_power_disable(char *name);
void *lcd_fb_malloc(u32 size);
void lcd_fb_free(void *ptr);
int lcd_fb_mutex_lock(hal_sem_t *sem);
int lcd_fb_mutex_unlock(hal_sem_t *sem);
int lcd_fb_mutex_init(hal_sem_t *lock);

uintptr_t lcd_fb_pwm_request(u32 pwm_id);
int lcd_fb_pwm_free(uintptr_t p_handler);
int lcd_fb_pwm_enable(uintptr_t p_handler);
int lcd_fb_pwm_disable(uintptr_t p_handler);
int lcd_fb_pwm_config(uintptr_t p_handler, int duty_ns, int period_ns);
int lcd_fb_pwm_set_polarity(uintptr_t p_handler, int polarity);
s32 disp_delay_ms(u32 ms);
s32 disp_delay_us(u32 us);

#endif
