/*
 * drivers/rtos-hal/hal/source/spilcd/lcd_fb/dev_lcd_fb.c
 *
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
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
#include "include.h"
#include "./panels/panels.h"
#include "disp_lcd.h"
#include "disp_display.h"
#include "dev_lcd_fb.h"
#ifdef CONFIG_COMPONENTS_PM
#include "pm_devops.h"
#endif

struct dev_lcd_fb_t g_drv_info;
hal_workqueue *g_disp_work_queue;

extern int lcd_init(void);


static void start_work(hal_work *work, void* work_data)
{
	int i = 0;
	struct lcd_fb_device *dispdev = NULL;

	for (i = 0; i < SUPPORT_MAX_LCD; ++i) {
		dispdev = lcd_fb_device_get(i);
		if (dispdev)
			dispdev->enable(dispdev);
	}

}

static s32 start_process(void)
{
	hal_workqueue_dowork(g_disp_work_queue, &g_drv_info.start_work);
	return 0;
}

#ifdef CONFIG_COMPONENTS_PM
int lcd_fb_suspend(struct pm_device *dev, suspend_mode_t mode)
#else
int lcd_fb_suspend(void)
#endif
{
	int i = 0;
	int ret = 0;
	struct lcd_fb_device *dispdev = NULL;

	for (i = 0; i < SUPPORT_MAX_LCD; ++i) {
		dispdev = lcd_fb_device_get(i);
		if (dispdev)
			ret = dispdev->disable(dispdev);
	}
	return ret;
}

#ifdef CONFIG_COMPONENTS_PM
int lcd_fb_resume(struct pm_device *dev, suspend_mode_t mode)
#else
int lcd_fb_resume(void)
#endif
{
	int i = 0;
	int ret = 0;
	struct lcd_fb_device *dispdev = NULL;

	for (i = 0; i < SUPPORT_MAX_LCD; ++i) {
		dispdev = lcd_fb_device_get(i);
		if (dispdev)
			ret = dispdev->enable(dispdev);
	}
	return ret;
}

#ifdef CONFIG_COMPONENTS_PM
static struct pm_devops spilcd_devops = {
	.suspend = lcd_fb_suspend,
	.resume = lcd_fb_resume,
};

static struct pm_device spilcd_pm = {
	.name = "spilcd",
	.ops = &spilcd_devops,
};
#endif

int lcd_fb_probe(void)
{
	int ret = 0;

	lcd_fb_wrn();

	g_disp_work_queue = hal_workqueue_create("spilcd", 8192, 15);
	if (!g_disp_work_queue) {
		lcd_fb_wrn("Create disp work queue fail!\n");
		return -1;
	}
	hal_work_init(&g_drv_info.start_work, start_work, NULL);

	disp_init_lcd(&g_drv_info);

	lcd_init();

	start_process();

#ifdef CONFIG_COMPONENTS_PM
	pm_devops_register(&spilcd_pm);
#endif
	return ret;
}

static void lcd_fb_shutdown(void)
{
	int i = 0;
	struct lcd_fb_device *dispdev = NULL;

	for (i = 0; i < SUPPORT_MAX_LCD; ++i) {
		dispdev = lcd_fb_device_get(i);
		if (dispdev)
			dispdev->disable(dispdev);
	}
	lcd_fb_wrn("Finish\n");
}

int lcd_fb_remove(void)
{
	int ret = 0;

	lcd_fb_wrn("\n");
	lcd_fb_shutdown();
	disp_exit_lcd();
#ifdef CONFIG_COMPONENTS_PM
	pm_devops_unregister(&spilcd_pm);
#endif
	return ret;
}

int sunxi_disp_get_source_ops(struct sunxi_disp_source_ops *src_ops)
{
	memset((void *)src_ops, 0, sizeof(struct sunxi_disp_source_ops));

	src_ops->sunxi_lcd_set_panel_funs = bsp_disp_lcd_set_panel_funs;
	src_ops->sunxi_lcd_delay_ms = disp_delay_ms;
	src_ops->sunxi_lcd_delay_us = disp_delay_us;
	src_ops->sunxi_lcd_backlight_enable = bsp_disp_lcd_backlight_enable;
	src_ops->sunxi_lcd_backlight_disable = bsp_disp_lcd_backlight_disable;
	src_ops->sunxi_lcd_pwm_enable = bsp_disp_lcd_pwm_enable;
	src_ops->sunxi_lcd_pwm_disable = bsp_disp_lcd_pwm_disable;
	src_ops->sunxi_lcd_power_enable = bsp_disp_lcd_power_enable;
	src_ops->sunxi_lcd_power_disable = bsp_disp_lcd_power_disable;
	src_ops->sunxi_lcd_pin_cfg = bsp_disp_lcd_pin_cfg;
	src_ops->sunxi_lcd_gpio_set_value = bsp_disp_lcd_gpio_set_value;
	src_ops->sunxi_lcd_gpio_set_direction = bsp_disp_lcd_gpio_set_direction;
	src_ops->sunxi_lcd_cmd_write = bsp_disp_lcd_cmd_write;
	src_ops->sunxi_lcd_para_write = bsp_disp_lcd_para_write;
	src_ops->sunxi_lcd_cmd_read = bsp_disp_lcd_cmd_read;
	return 0;
}
