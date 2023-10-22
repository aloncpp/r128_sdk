/*
 * Copyright (C) 2015 Allwinnertech, z.q <zengqi@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include "drv_tv.h"
#if defined(CONFIG_EXTCON)
#include <linux/extcon.h>
#endif

#define IS_ERR_OR_NULL(pointer) (pointer == NULL)
#define CVBSOUT_TOP     0x05600000
#define CVBSOUT         0x05604000
#define TV_CLK_NUM 4
#define TV_RST_NUM 2

static int suspend = 0;
struct tv_info_t g_tv_info;
#if defined(CONFIG_ARCH_SUN8IW7)
static unsigned int cali[4] = {512, 512, 512, 512};
#else
static unsigned int cali[4] = {625, 625, 625, 625};
#endif
static int offset[4] = {0, 0, 0, 0};
static char tv_power[25];
static u8 tv_power_enable_mask;
static struct cdev *tv_cdev;
static dev_t tv_devid ;
static struct class *tv_class;
static struct device *tv_dev;
static int tv_fake_detect;
static int tv_resync_pixel_num = -1;
static int tv_resync_line_num = -1;
static char *main_name = "tv0";
static int tv_id = 0;
unsigned int reg_base[2] = {
			CVBSOUT_TOP,
			CVBSOUT,
};

int tv_module_init(void);

static struct disp_video_timings video_timing[] = {
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_NTSC,
		.pixel_clk = 216000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 480,
		.hor_total_time = 858,
		.hor_back_porch = 60,
		.hor_front_porch = 16,
		.hor_sync_time = 62,
		.ver_total_time = 525,
		.ver_back_porch = 30,
		.ver_front_porch = 9,
		.ver_sync_time = 6,
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,

	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_PAL,
		.pixel_clk = 216000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 576,
		.hor_total_time = 864,
		.hor_back_porch = 68,
		.hor_front_porch = 12,
		.hor_sync_time = 64,
		.ver_total_time = 625,
		.ver_back_porch = 39,
		.ver_front_porch = 5,
		.ver_sync_time = 5,
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_480I,
		.pixel_clk = 216000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 480,
		.hor_total_time = 858,
		.hor_back_porch = 57,
		.hor_front_porch = 62,
		.hor_sync_time = 19,
		.ver_total_time = 525,
		.ver_back_porch = 4,
		.ver_front_porch = 1,
		.ver_sync_time = 3,
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 1,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_576I,
		.pixel_clk = 216000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 576,
		.hor_total_time = 864,
		.hor_back_porch = 69,
		.hor_front_porch = 63,
		.hor_sync_time = 12,
		.ver_total_time = 625,
		.ver_back_porch = 2,
		.ver_front_porch = 44,
		.ver_sync_time = 3,
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 1,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_480P,
		.pixel_clk = 54000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 480,
		.hor_total_time = 858,
		.hor_back_porch = 60,
		.hor_front_porch = 62,
		.hor_sync_time = 16,
		.ver_total_time = 525,
		.ver_back_porch = 9,
		.ver_front_porch = 30,
		.ver_sync_time = 6,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_576P,
		.pixel_clk = 54000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 576,
		.hor_total_time = 864,
		.hor_back_porch = 68,
		.hor_front_porch = 64,
		.hor_sync_time = 12,
		.ver_total_time = 625,
		.ver_back_porch = 5,
		.ver_front_porch = 39,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_720P_60HZ,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1280,
		.y_res = 720,
		.hor_total_time = 1650,
		.hor_back_porch = 220,
		.hor_front_porch = 40,
		.hor_sync_time = 110,
		.ver_total_time = 750,
		.ver_back_porch = 5,
		.ver_front_porch = 20,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_720P_50HZ,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1280,
		.y_res = 720,
		.hor_total_time = 1980,
		.hor_back_porch = 220,
		.hor_front_porch = 40,
		.hor_sync_time = 440,
		.ver_total_time = 750,
		.ver_back_porch = 5,
		.ver_front_porch = 20,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_1080I_60HZ,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2200,
		.hor_back_porch = 148,
		.hor_front_porch = 44,
		.hor_sync_time = 88,
		.ver_total_time = 1125,
		.ver_back_porch = 2,
		.ver_front_porch = 38,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 1,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_1080I_50HZ,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2640,
		.hor_back_porch = 148,
		.hor_front_porch = 44,
		.hor_sync_time = 528,
		.ver_total_time = 1125,
		.ver_back_porch = 2,
		.ver_front_porch = 38,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 1,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_1080P_60HZ,
		.pixel_clk = 148500000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2200,
		.hor_back_porch = 148,
		.hor_front_porch = 44,
		.hor_sync_time = 88,
		.ver_total_time = 1125,
		.ver_back_porch = 4,
		.ver_front_porch = 36,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_1080P_50HZ,
		.pixel_clk = 148500000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2640,
		.hor_back_porch = 148,
		.hor_front_porch = 44,
		.hor_sync_time = 528,
		.ver_total_time = 1125,
		.ver_back_porch = 4,
		.ver_front_porch = 36,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_1600_900P_60,
		.pixel_clk = 108000000,
		.pixel_repeat = 0,
		.x_res = 1600,
		.y_res = 900,
		.hor_total_time = 1800,
		.hor_back_porch = 96,
		.hor_front_porch = 24,
		.hor_sync_time = 80,
		.ver_total_time = 1000,
		.ver_back_porch = 96,
		.ver_front_porch = 1,
		.ver_sync_time = 3,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_1440_900P_60,
		.pixel_clk = 89000000,
		.pixel_repeat = 0,
		.x_res = 1440,
		.y_res = 900,
		.hor_total_time = 1600,
		.hor_back_porch = 80,
		.hor_front_porch = 48,
		.hor_sync_time = 32,
		.ver_total_time = 926,
		.ver_back_porch = 17,
		.ver_front_porch = 3,
		.ver_sync_time = 6,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_1366_768P_60,
		.pixel_clk = 85800000,
		.pixel_repeat = 0,
		.x_res = 1366,
		.y_res = 768,
		.hor_total_time = 1792,
		.hor_back_porch = 213,
		.hor_front_porch = 70,
		.hor_sync_time = 143,
		.ver_total_time = 798,
		.ver_back_porch = 24,
		.ver_front_porch = 3,
		.ver_sync_time = 3,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_1280_800P_60,
		.pixel_clk = 83500000,
		.pixel_repeat = 0,
		.x_res = 1280,
		.y_res = 800,
		.hor_total_time = 1680,
		.hor_back_porch = 200,
		.hor_front_porch = 72,
		.hor_sync_time = 128,
		.ver_total_time = 831,
		.ver_back_porch = 22,
		.ver_front_porch = 3,
		.ver_sync_time = 6,
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_1024_768P_60,
		.pixel_clk = 65000000,
		.pixel_repeat = 0,
		.x_res = 1024,
		.y_res = 768,
		.hor_total_time = 1344,
		.hor_back_porch = 160,
		.hor_front_porch = 24,
		.hor_sync_time = 136,
		.ver_total_time = 806,
		.ver_back_porch = 29,
		.ver_front_porch = 3,
		.ver_sync_time = 6,
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_800_600P_60,
		.pixel_clk = 40000000,
		.pixel_repeat = 0,
		.x_res = 800,
		.y_res = 600,
		.hor_total_time = 1056,
		.hor_back_porch = 88,
		.hor_front_porch = 40,
		.hor_sync_time = 128,
		.ver_total_time = 628,
		.ver_back_porch = 23,
		.ver_front_porch = 1,
		.ver_sync_time = 4,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_1280_720P_60,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1280,
		.y_res = 720,
		.hor_total_time = 1650,
		.hor_back_porch = 220,
		.hor_front_porch = 110,
		.hor_sync_time = 40,
		.ver_total_time = 750,
		.ver_back_porch = 20,
		.ver_front_porch = 5,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_1920_1080P_60,
		.pixel_clk = 148500000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2200,
		.hor_back_porch = 148,
		.hor_front_porch = 88,
		.hor_sync_time = 44,
		.ver_total_time = 1125,
		.ver_back_porch = 36,
		.ver_front_porch = 4,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
};

#define TVE_CHECK_PARAM(sel) \
	do { if (sel >= TVE_DEVICE_NUM) {\
		pr_warn("%s, sel(%d) is out of range\n", __func__, sel);\
		return -1;\
		} \
	} while (0)

#define TVDEBUG
#if defined(TVDEBUG)
#define TV_DBG(fmt, arg...)   pr_warn("%s()%d - "fmt, __func__, __LINE__, ##arg)
#else
#define TV_DBG(fmt, arg...)
#endif
#define TV_ERR(fmt, arg...)   pr_err("%s()%d - "fmt, __func__, __LINE__, ##arg)

static int tv_regulator_enable(struct regulator_dev *regulator)
{
	int ret = 0;

	if (!regulator)
		return 0;

	ret = hal_regulator_enable(regulator);
	WARN(ret, "regulator_enable failed, ret=%d\n", ret);

	return ret;
}

static int tv_regulator_disable(struct regulator_dev *regulator)
{
	int ret = 0;

	if (!regulator)
		return 0;

	ret = hal_regulator_disable(regulator);
	WARN(ret, "regulator_disable failed, ret=%d\n", ret);

	return ret;
}


#if defined(CONFIG_EXTCON)
static struct task_struct *tve_task;
static u32 tv_hpd[SCREEN_COUNT];
static struct extcon_dev *extcon_dev[SCREEN_COUNT];
static bool is_compatible_cvbs;

static const unsigned int tv_cable[] = {
	EXTCON_DISP_CVBS,
	EXTCON_NONE,
};

/* this extcon is used for the purpose of compatible platform */
static struct extcon_dev *extcon_cvbs;

static char extcon_name[20];
static ssize_t tv_faketv_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", tv_fake_detect);
}

static ssize_t tv_faketv_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = kstrtoul(buf, 10, &val);  /* strict_strtoul */
	if (err) {
		TV_ERR("Invalid size\n");
		return err;
	}

	if (val) {
		tv_fake_detect = 1;
		pr_info("fake enable tv_out\n");
	} else {
		tv_fake_detect = 0;
		pr_info("fake tv_out disable \n");
	}
	return count;
}

void tv_report_hpd_work(u32 sel, u32 hpd)
{
	if (tv_hpd[sel] == hpd)
		return;

	switch (hpd) {
	case STATUE_CLOSE:
		extcon_set_state_sync(extcon_dev[sel], EXTCON_DISP_CVBS,
				      STATUE_CLOSE);
		if (is_compatible_cvbs)
			extcon_set_state_sync(extcon_cvbs, EXTCON_DISP_CVBS,
					      STATUE_CLOSE);
		break;

	case STATUE_OPEN:
		extcon_set_state_sync(extcon_dev[sel], EXTCON_DISP_CVBS,
				      STATUE_OPEN);
		if (is_compatible_cvbs)
			extcon_set_state_sync(extcon_cvbs, EXTCON_DISP_CVBS,
					      STATUE_OPEN);
		break;

	default:
		extcon_set_state_sync(extcon_dev[sel], EXTCON_DISP_CVBS,
				      STATUE_CLOSE);
	      break;
	}
	tv_hpd[sel] = hpd;
}

s32 tv_detect_thread(void *parg)
{
	s32 hpd[SCREEN_COUNT];
	int i = 0;

	while (1) {
		if (hal_thread_should_stop())
			break;
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(20);

		if (!suspend) {
			for (i = 0; i < SCREEN_COUNT; i++) {
				if (tv_fake_detect)
					hpd[i] = 1;
				else
					hpd[i] = tve_low_get_dac_status(i);
				if (hpd[i] != tv_hpd[i]) {
					TV_DBG("hpd[%d] = %d\n", i, hpd[i]);
					tv_report_hpd_work(i, hpd[i]);
				}
			}
		}
	}
	return 0;
}

s32 tv_detect_enable(u32 sel)
{
	tve_low_dac_autocheck_enable(sel);
	/* only one thread to detect hot pluging */
	if (!tve_task) {
		tve_task = hal_thread_create(tv_detect_thread, (void *)0,
						"tve detect", HAL_THREAD_STACK_SIZE, HAL_THREAD_PRIORITY_SYS);
		if (IS_ERR(tve_task)) {
			s32 err = 0;

			err = PTR_ERR(tve_task);
			tve_task = NULL;
			return err;
		}
		TV_DBG("tve_task is ok!\n");
		hal_thread_resume(tve_task);
	}
	return 0;
}

s32 tv_detect_disable(u32 sel)
{
	if (tve_task) {
		hal_thread_stop(tve_task);
		tve_task = NULL;
		tve_low_dac_autocheck_disable(sel);
	}
	return 0;
}
#else
static hal_thread *tve_task = NULL;
static u32 tv_hpd[SCREEN_COUNT];

u32 tv_lock_data;
hal_spinlock_t tv_lock;
static bool hpd_status = 0;

int get_tve_hpg_status(void)
{
	int status = 0;

	tv_lock_data = hal_spin_lock_irqsave(&tv_lock);
	status = (int)hpd_status;
	hal_spin_unlock_irqrestore(&tv_lock, tv_lock_data);
	return status;
}

void tv_report_hpd_work(u32 sel, u32 hpd)
{
	if (tv_hpd[sel] == hpd)
		return;

	switch (hpd) {
	case STATUE_CLOSE:{
		// TV_DBG("STATUE_CLOSE \n");
		hpd_status = 0;
		break;
	}
	case STATUE_OPEN:{
		// TV_DBG("STATUE_OPEN \n");
		hpd_status = 1;
		break;
	}
	default:
		// printf("STATUE_CLOSE \n");
	      break;
	}
	tv_hpd[sel] = hpd;
}

void tv_detect_thread(void *parg)
{
	s32 hpd[SCREEN_COUNT];
	int i = 0;

	while (1) {
		disp_delay_ms(20);

		if (!suspend) {
			for (i = 0; i < SCREEN_COUNT; i++) {
				if (tv_fake_detect)
					hpd[i] = 1;
				else
					hpd[i] = tve_low_get_dac_status(i);
				if (hpd[i] != tv_hpd[i]) {
					TV_DBG("hpd[%d] = %d\n", i, hpd[i]);
					tv_lock_data = hal_spin_lock_irqsave(&tv_lock);
					tv_report_hpd_work(i, hpd[i]);
					hal_spin_unlock_irqrestore(&tv_lock, tv_lock_data);
				}
			}
		}
	}
	return;
}

s32 tv_detect_enable(u32 sel)
{
	tve_low_dac_autocheck_enable(sel);
	/* only one thread to detect hot pluging */
	if (!tve_task) {
		tve_task = hal_thread_create(tv_detect_thread, (void *)0,
						"tve detect", HAL_THREAD_STACK_SIZE, HAL_THREAD_PRIORITY_SYS);
		if (IS_ERR((long)tve_task)) {
			s32 err = 0;

			err = PTR_ERR(tve_task);
			tve_task = NULL;
			return err;
		}
		TV_DBG("tve_task is ok!\n");
		hal_thread_start(tve_task);
	}
	return 0;
}

s32 tv_detect_disable(u32 sel)
{
	if (tve_task) {
		hal_thread_stop(tve_task);
		tve_task = NULL;
		tve_low_dac_autocheck_disable(sel);
	}
	return 0;
}

static int tv_faketv_show(int argc, const char **argv)
{
	return 0;
}

static int tv_faketv_store(int argc, const char **argv)
{
	return 0;
}
#endif


static int tv_resync_pixel_store(int argc, const char **argv)
{
	int err;
	long val;

	/*
	tv_resync_pixel_num = val;
	*/

	return 0;
}

static int tv_resync_line_store(int argc, const char **argv)
{
	int err;
	long val;

	/*
	tv_resync_line_num = val;
	*/

	return 0;
}

FINSH_FUNCTION_EXPORT_ALIAS(tv_faketv_show, tv_faketv_show, tv_faketv_show cmd);
FINSH_FUNCTION_EXPORT_ALIAS(tv_faketv_store, tv_faketv_store, tv_faketv_store cmd);
FINSH_FUNCTION_EXPORT_ALIAS(tv_resync_pixel_store, tv_resync_pixel_store, tv_resync_pixel_store cmd);
FINSH_FUNCTION_EXPORT_ALIAS(tv_resync_line_store, tv_resync_line_store, tv_resync_line_store cmd);

s32 tv_get_video_info(s32 mode)
{
	s32 i, count;

	count = sizeof(video_timing)/sizeof(struct disp_video_timings);
	for (i = 0; i < count; i++) {
		if (mode == video_timing[i].tv_mode)
			return i;
	}
	return -1;
}

s32 tv_get_list_num(void)
{
	return sizeof(video_timing)/sizeof(struct disp_video_timings);
}

s32 tv_set_enhance_mode(u32 sel, u32 mode)
{
	return tve_low_enhance(sel, mode);
}

static void tve_clk_init(u32 sel)
{
	if (!g_tv_info.screen[sel].clk) {
		TV_ERR("clk is NULL, sel:%d\n", sel);
		return;
	}

	// g_tv_info.screen[sel].clk_parent = hal_clk_get_parent(g_tv_info.screen[sel].clk);
}

#if defined(TVE_TOP_SUPPORT)
static int tve_top_clk_enable(void)
{
	int ret;

	if (!IS_ERR_OR_NULL(g_tv_info.rst_bus))
		hal_reset_control_deassert(g_tv_info.rst_bus);

	if (!g_tv_info.bus_clk) {
		TV_ERR("top bus clk is NULL\n");
		return -1;
	}

	ret = hal_clock_enable(g_tv_info.bus_clk);
	if (ret != 0) {
		TV_ERR("fail to enable tve's top clk!\n");
		return ret;
	}

	return 0;
}

static int tve_top_clk_disable(void)
{
	if (!IS_ERR_OR_NULL(g_tv_info.rst_bus))
		hal_reset_control_assert(g_tv_info.rst_bus);

	if (!g_tv_info.bus_clk) {
		TV_ERR("top bus clk is NULL\n");
		return -1;
	}

	hal_clock_disable(g_tv_info.bus_clk);

	return 0;
}
#else
static int tve_top_clk_enable(void) {return 0; }
static int tve_top_clk_disable(void) {return 0; }
#endif

static int tve_clk_enable(u32 sel)
{
	int ret;

	if (!g_tv_info.screen[sel].clk) {
		TV_ERR("screen[%d] clk is NULL\n", sel);
		return -1;
	}

	ret = hal_clock_enable(g_tv_info.screen[sel].clk);
	if (ret != 0) {
		TV_ERR("fail to enable tve%d's clk!\n", sel);
		return ret;
	}

	if (!g_tv_info.screen[sel].bus_clk) {
		TV_ERR("screen[%d] bus clk is NULL\n", sel);
		return -1;
	}

	ret = hal_clock_enable(g_tv_info.screen[sel].bus_clk);
	if (ret != 0) {
		TV_ERR("fail to enable tve%d's bus clk!\n", sel);
		return ret;
	}

	if (!IS_ERR_OR_NULL(g_tv_info.screen[sel].rst_bus))
		hal_reset_control_deassert(g_tv_info.screen[sel].rst_bus);

	return 0;
}

static int tve_clk_disable(u32 sel)
{
	if (!g_tv_info.screen[sel].clk) {
		TV_ERR("screen[%d] clk is NULL\n", sel);
		return -1;
	}
	hal_clock_disable(g_tv_info.screen[sel].clk);

	if (!g_tv_info.screen[sel].bus_clk) {
		TV_ERR("screen[%d] bus clk is NULL\n", sel);
		return -1;
	}
	hal_clock_disable(g_tv_info.screen[sel].bus_clk);
	if (!IS_ERR_OR_NULL(g_tv_info.screen[sel].rst_bus))
		hal_reset_control_assert(g_tv_info.screen[sel].rst_bus);

	return 0;
}

static s32 tve_get_pixclk(unsigned long *p_rate, const int *p_tv_mode)
{
	int i = 0, list_num = 0, ret = -1;
	bool find = false;
	struct disp_video_timings *info = video_timing;

	if (!p_rate || !p_tv_mode)
		goto OUT;
	list_num = tv_get_list_num();
	for (i = 0; i < list_num; i++) {
		if (info->tv_mode == *p_tv_mode) {
			find = true;
			break;
		}
		info++;
	}
	if (!find) {
		TV_ERR("tv have no mode(%d)!\n", *p_tv_mode);
		goto OUT;
	} else {
		*p_rate = info->pixel_clk;
		ret = 0;
	}
OUT:
	return ret;
}

static void tve_clk_config(u32 sel, u32 tv_mode)
{
	int ret = 0;
	unsigned long rate = 0, prate = 0;
	unsigned long round = 0, parent_round_rate = 0;
	signed long rate_diff = 0, prate_diff = 0, accuracy = 1000000;
	unsigned int div = 1;

	ret = hal_clk_set_parent(g_tv_info.screen[sel].clk, g_tv_info.screen[sel].clk_parent);

	if (!ret) {
		printf("[err] set clk relationship failed !");
	}

	ret = tve_get_pixclk(&rate, &tv_mode);

	if (ret)
		TV_ERR("%s:tve_get_pixclk fail!\n", __func__);

	if (!g_tv_info.screen[sel].clk_parent) {
		TV_ERR("screen[%d] clk_parent is NULL\n", sel);
		return;
	}
	hal_clk_set_rate(g_tv_info.screen[sel].clk_parent, rate);

	if (!g_tv_info.screen[sel].clk) {
		TV_ERR("screen[%d] clk is NULL\n", sel);
		return;
	}
	round = hal_clk_round_rate(g_tv_info.screen[sel].clk, rate);

	rate_diff = (long)(round - rate);
	if ((rate_diff > accuracy) || (rate_diff < -accuracy)) {
		for (accuracy = 1000000; accuracy <= 5000000;
		     accuracy += 1000000) {
			for (div = 1; (rate * div) <= 984000000; div++) {
				prate = rate * div;
				parent_round_rate =
				    hal_clk_round_rate(g_tv_info.screen[sel].clk_parent, prate);
				prate_diff = (long)(parent_round_rate - prate);
				if ((prate_diff < accuracy) &&
				    (prate_diff > -accuracy)) {
					ret = hal_clk_set_rate(g_tv_info.screen[sel].clk_parent,
							   prate);
					ret += hal_clk_set_rate(
					    g_tv_info.screen[sel].clk, rate);
					if (ret)
						TV_ERR("fail to set rate(%ld) "
							"fo tve%d's clock!\n",
							rate, sel);
					else
						break;
				}
			}
			if (rate * div > 984000000) {
				TV_ERR("fail to set tve clk at %ld accuracy\n",
					accuracy);
				continue;
			}
			break;
		}
	} else {
		prate = hal_clk_get_rate(g_tv_info.screen[sel].clk_parent);
		ret = hal_clk_set_rate(g_tv_info.screen[sel].clk, rate);
		if (ret)
			TV_ERR("fail to set rate(%ld) fo tve%d's clock!\n",
				rate, sel);
	}

	TV_DBG("parent prate=%lu(%lu), rate=%lu(%lu), tv_mode=%d\n",
		hal_clk_get_rate(g_tv_info.screen[sel].clk_parent), prate,
		hal_clk_get_rate(g_tv_info.screen[sel].clk), rate, tv_mode);
}

static int tv_power_enable(char *name)
{
	int ret = -1;

	if (tv_power_enable_mask) {
		ret = 0;
		goto exit;
	}

	/*
	if (!g_tv_info.regulator)
		g_tv_info.regulator = regulator_get(
				g_tv_info.dev, name);

	if (!g_tv_info.regulator) {
		TV_ERR("regulator_get:%s failed!\n", name);
		return -1;
	}

	ret = tv_regulator_enable(g_tv_info.regulator);
	*/

	tv_power_enable_mask = 1;

exit:
	return ret;
}

static int tv_power_disable(char *name)
{
	int ret = 0;

	if (!tv_power_enable_mask) {
		ret = 0;
		goto exit;
	}
	/*
	if (!g_tv_info.regulator)
		g_tv_info.regulator = regulator_get(
				g_tv_info.dev, name);

	if (!g_tv_info.regulator) {
		TV_ERR("regulator_get:%s failed!\n", name);
		return -1;
	}

	ret = tv_regulator_disable(g_tv_info.regulator);
	regulator_put(g_tv_info.regulator);
	g_tv_info.regulator = NULL;
	*/
	tv_power_enable_mask = 0;
exit:
	return ret;
}

int tv_mutex_lock(hal_sem_t sem)
{
	return hal_sem_wait(sem);
}

int tv_mutex_unlock(hal_sem_t sem)
{
	return hal_sem_post(sem);
}

static s32 __get_offset(int i)
{
	char sub_key[20] = {0};
	s32 value = 0;
	int ret = 0;

	snprintf(sub_key, sizeof(sub_key), "dac_offset%d", i);
	ret = hal_cfg_get_keyvalue(main_name, sub_key, (u32 *)&value, sizeof(value));
	if (ret < 0) {
		TV_DBG("there is no tve dac(%d) offset value.\n", i);
	} else {
		/* Sysconfig can not use signed params, however,
		 * dac_offset as a signed param which ranges from
		 * -100 to 100, is maping sysconfig params from
		 * 0 to 200.
		*/
		if ((value > 200) || (value < 0))
			TV_ERR("dac offset is out of range.\n");
		else
			return value - 100;
	}

	return 0;
}

s32 tv_init(void)
{
	s32 i = 0, ret = 0;
	u32 cali_value = 0, sel = tv_id;
	char sub_key[20] = {0};
	unsigned int value, output_type, output_mode;
	unsigned int interface = 0;
	unsigned long rate = 0;
#if defined(CONFIG_ARCH_SUN8IW7)
	s32 sid_turn = 0;
#endif

	ret = hal_cfg_get_keyvalue(main_name, "interface",
					&interface, 1);
	if (ret < 0)
		TV_ERR("get tv interface failed!\n");
#if defined(CONFIG_EXTCON)
	/* if tve0 is the CVBS interface,
	 * than add creating another cvbs switch,
	 * which is compatible with old hardware of
	 * TVE module.
	 */

	if (interface == DISP_TV_CVBS && sel == 0) {
		extcon_cvbs =
			devm_extcon_dev_allocate(&pdev->dev, tv_cable);
		if (IS_ERR_OR_NULL(extcon_cvbs))
			return -1;
		extcon_cvbs->name = "cvbs";
		devm_extcon_dev_register(&pdev->dev, extcon_cvbs);
		is_compatible_cvbs = true;
	}

	extcon_dev[sel] = devm_extcon_dev_allocate(&pdev->dev, tv_cable);
	if (IS_ERR_OR_NULL(extcon_dev[sel]))
		return -1;
	switch (interface) {
	case DISP_TV_CVBS:
		snprintf(extcon_name, sizeof(extcon_name), "tve%d_cvbs", sel);
		break;
	case DISP_TV_YPBPR:
		snprintf(extcon_name, sizeof(extcon_name), "tve%d_ypbpr", sel);
		break;
	case DISP_TV_SVIDEO:
		snprintf(extcon_name, sizeof(extcon_name), "tve%d_svideo", sel);
		break;
	case DISP_VGA:
		snprintf(extcon_name, sizeof(extcon_name), "tve%d_vga", sel);
		break;
	default:
		break;
	}
	extcon_dev[sel]->name = extcon_name;
	devm_extcon_dev_register(&pdev->dev, extcon_dev[sel]);


#endif
		tve_top_clk_enable();
		/* get mapping dac */
		for (i = 0; i < DAC_COUNT; i++) {
			u32 value;
			u32 dac_no;

			snprintf(sub_key, sizeof(sub_key), "dac_src%d", i);
			ret = hal_cfg_get_keyvalue(main_name, sub_key,
						   &value, 4);

			TV_DBG("dac_src : %d \n", value);

			if (ret < 0) {
				TV_DBG("tve%d have no dac %d, sub_key:%s\n",
					sel, i, sub_key);
			} else {
				dac_no = value;
				g_tv_info.screen[sel].dac_no[i] = value;
				g_tv_info.screen[sel].dac_num++;
				cali_value = (u32) tve_low_get_sid(dac_no);

				pr_debug("cali_temp = %u\n", cali_value);
				/* VGA mode: 16~31 bits
				 * CVBS & YPBPR mode: 0~15 bits
				 * zero is not allow
				 */
				if (cali_value) {
					if (interface == DISP_VGA)
						cali[dac_no] =
						    (cali_value >> 16) & 0xffff;
					else {
#if defined(CONFIG_ARCH_SUN8IW7)
						if (cali_value & (1 << 9))
							sid_turn =
							    0 + (cali_value &
								 0x1ff);
						else
							sid_turn =
							    0 - (cali_value &
								 0x1ff);

						sid_turn += 91;

						if (sid_turn >= 0)
							sid_turn =
							    (1 << 9) | sid_turn;
						else
							sid_turn = 0 - sid_turn;
						cali_value = (u32)sid_turn;
#endif
						cali[dac_no] =
						    cali_value & 0xffff;
					}
				}
				offset[dac_no] =
				    __get_offset(i);
				pr_debug("cali[%u] = %u, offset[%u] = %u\n",
					 dac_no, cali[dac_no], dac_no,
					 offset[dac_no]);
			}

			snprintf(sub_key, sizeof(sub_key), "dac_type%d", i);
			ret = hal_cfg_get_keyvalue(main_name, sub_key,
						   &value, sizeof(value));
			if (ret < 0) {
				TV_DBG("tve%d have no type%d sub_key:%s\n",
					sel, i, sub_key);
				/* if do'not config type, set disabled status */
				g_tv_info.screen[sel].dac_type[i] = 7;
			} else {
				g_tv_info.screen[sel].dac_type[i] = value;
			}
		}

		/* parse boot params, but boot is not on, value should be 0 */
		value = 0;
		output_type = (value >> (sel * 16) >>  8) & 0xff;
		output_mode = (value) >> (sel * 16) & 0xff;

#if TVE_DEVICE_NUM == 1
		/*
		 * On the platform that only support 1 channel tvout,
		 * We need to check if tvout be enabled at bootloader
		 * through all paths. So here we check the disp1 part.
		 */
		if ((sel == 0) && (output_type != DISP_OUTPUT_TYPE_TV)) {
			output_type = (value >> (1 * 16) >>  8) & 0xff;
			output_mode = (value) >> (1 * 16) & 0xff;
		}
#endif

		// mutex_init(&g_tv_info.screen[sel].mlock);
		g_tv_info.screen[sel].mlock = hal_sem_create(1);
		pr_debug("[TV]:value = %d, type = %d, mode = %d\n",
				value, output_type, output_mode);
		if ((output_type == DISP_OUTPUT_TYPE_TV)
			|| (output_type == DISP_OUTPUT_TYPE_VGA)) {
			g_tv_info.screen[sel].enable = 1;
			g_tv_info.screen[sel].tv_mode = output_mode;
			pr_debug("[TV]:g_tv_info.screen[0].tv_mode = %d",
				g_tv_info.screen[sel].tv_mode);
			tve_clk_config(sel, g_tv_info.screen[sel].tv_mode);
		} else {
			g_tv_info.screen[sel].tv_mode = DISP_TV_MOD_PAL;
			ret = tve_get_pixclk(&rate,
					     (int *)&g_tv_info.screen[sel].tv_mode);
			if (ret)
				TV_ERR("%s:tve_get_pixclk fail!\n", __func__);
			ret = hal_clk_set_rate(g_tv_info.screen[sel].clk, rate);
			if (ret)
				TV_ERR(
				    "fail to set rate(%ld) fo tve%d's clock!\n",
				    rate, sel);
		}

		tve_low_set_reg_base(sel, g_tv_info.screen[sel].base_addr);
		tve_clk_init(sel);
#if !defined(CONFIG_COMMON_CLK_ENABLE_SYNCBOOT)
		tve_clk_enable(sel);
#endif

		if ((output_type != DISP_OUTPUT_TYPE_TV)
			&& (output_type != DISP_OUTPUT_TYPE_VGA)) {
			tve_low_init(sel, &g_tv_info.screen[sel].dac_no[0],
					cali, offset,
					g_tv_info.screen[sel].dac_type,
					g_tv_info.screen[sel].dac_num);
		}
		else {
			tve_low_sw_init(sel, &g_tv_info.screen[sel].dac_no[0],
					g_tv_info.screen[sel].dac_type,
					g_tv_info.screen[sel].dac_num);
		}
		tv_detect_enable(sel);
	return 0;
}

s32 tv_exit(void)
{
	s32 i;

	for (i = 0; i < g_tv_info.tv_number; i++)
		tv_detect_disable(i);

	for (i = 0; i < g_tv_info.tv_number; i++)
		tv_disable(i);

	tv_power_disable(tv_power);
	return 0;
}

s32 tv_get_mode(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	return g_tv_info.screen[sel].tv_mode;
}

s32 tv_set_mode(u32 sel, enum disp_tv_mode tv_mode)
{
	if (tv_mode >= DISP_TV_MODE_NUM)
		return -1;

	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	tv_mutex_lock(g_tv_info.screen[sel].mlock);
	g_tv_info.screen[sel].tv_mode = tv_mode;
	tv_mutex_unlock(g_tv_info.screen[sel].mlock);
	return  0;
}

s32 tv_get_input_csc(u32 sel)
{
	/* vga interface is rgb mode. */
	if (is_vga_mode(g_tv_info.screen[sel].tv_mode))
		return 0;
	else
		return 1;
}

s32 tv_get_video_timing_info(u32 sel, struct disp_video_timings **video_info)
{
	struct disp_video_timings *info;
	int ret = -1;
	int i, list_num;

	info = video_timing;

	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	list_num = tv_get_list_num();
	for (i = 0; i < list_num; i++) {
		tv_mutex_lock(g_tv_info.screen[sel].mlock);
		if (info->tv_mode == g_tv_info.screen[sel].tv_mode) {
			*video_info = info;
			ret = 0;
			tv_mutex_unlock(g_tv_info.screen[sel].mlock);
			break;
		}
		tv_mutex_unlock(g_tv_info.screen[sel].mlock);
		info++;
	}
	return ret;
}

static int __pin_config(int sel, char *name)
{
	int ret = 0;
	char type_name[10] = {0};

	/*
	snprintf(type_name, sizeof(type_name), "tv%d", sel);

	pctl = pinctrl_get(&pdev->dev);
	if (IS_ERR(pctl)) {
		TV_ERR("pinctrl_get for %s fail\n", type_name);
		ret = PTR_ERR(pctl);
		goto exit;
	}

	state = pinctrl_lookup_state(pctl, name);
	if (IS_ERR(state)) {
		TV_ERR("pinctrl_lookup_state for %s fail\n", type_name);
		ret = PTR_ERR(state);
		goto exit;
	}

	ret = pinctrl_select_state(pctl, state);
	if (ret < 0) {
		TV_ERR("pinctrl_select_state(%s)fail\n", type_name);
		goto exit;
	}
	*/
exit:
	return ret;
}

s32 tv_enable(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	if (!g_tv_info.screen[sel].enable) {
		if (is_vga_mode(g_tv_info.screen[sel].tv_mode))
			__pin_config(sel, "active");

		tve_clk_config(sel, g_tv_info.screen[sel].tv_mode);
		tve_low_set_tv_mode(sel, g_tv_info.screen[sel].tv_mode,
					cali);
		tve_low_dac_enable(sel);
		tve_adjust_resync(sel, tv_resync_pixel_num, tv_resync_line_num);
		tve_low_open(sel);
		tv_mutex_lock(g_tv_info.screen[sel].mlock);
		g_tv_info.screen[sel].enable = 1;
		tv_mutex_unlock(g_tv_info.screen[sel].mlock);
	}
	return 0;
}

s32 tv_disable(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	tv_mutex_lock(g_tv_info.screen[sel].mlock);
	if (g_tv_info.screen[sel].enable) {
		tve_low_close(sel);
		tve_low_dac_autocheck_enable(sel);
		g_tv_info.screen[sel].enable = 0;
	}
	tv_mutex_unlock(g_tv_info.screen[sel].mlock);
	if (is_vga_mode(g_tv_info.screen[sel].tv_mode))
		__pin_config(sel, "sleep");
	return 0;
}

s32 tv_suspend(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	tv_mutex_lock(g_tv_info.screen[sel].mlock);
	if (!g_tv_info.screen[sel].suspend) {
		g_tv_info.screen[sel].suspend = true;
		tv_detect_disable(sel);
		tve_clk_disable(sel);
		tve_top_clk_disable();
	}
	tv_power_disable(tv_power);
	suspend = 1;
	tv_mutex_unlock(g_tv_info.screen[sel].mlock);

	return 0;
}

s32 tv_resume(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);
	tv_mutex_lock(g_tv_info.screen[sel].mlock);
	suspend = 0;
	tv_power_enable(tv_power);
	if (g_tv_info.screen[sel].suspend) {
		g_tv_info.screen[sel].suspend = false;
		tve_top_clk_enable();
		tve_clk_enable(sel);
		tve_low_init(sel, &g_tv_info.screen[sel].dac_no[0],
				cali, offset, g_tv_info.screen[sel].dac_type,
				g_tv_info.screen[sel].dac_num);
		tv_detect_enable(sel);
	}
	tv_mutex_unlock(g_tv_info.screen[sel].mlock);

	return  0;
}

s32 tv_mode_support(u32 sel, enum disp_tv_mode mode)
{
	u32 i, list_num;
	struct disp_video_timings *info;

	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	info = video_timing;
	list_num = tv_get_list_num();
	for (i = 0; i < list_num; i++) {
		if (info->tv_mode == mode)
			return 1;
		info++;
	}
	return 0;
}

s32 tv_hot_plugging_detect(u32 state)
{
	int i = 0;

	for (i = 0; i < SCREEN_COUNT; i++) {
		if (state == STATUE_OPEN)
			return tve_low_dac_autocheck_enable(i);
		else if (state == STATUE_CLOSE)
			return tve_low_dac_autocheck_disable(i);
	}
	return 0;
}

#if defined(TVE_TOP_SUPPORT)
static int tv_top_init(hal_clk_id_t *clk_id)
{
	int ret;

	if (g_tv_info.tv_number)
		return 0;

	g_tv_info.base_addr = (void *)CVBSOUT_TOP;
	if (!g_tv_info.base_addr) {
		pr_err("unable to map tve common registers\n");
		ret = -EINVAL;
		goto err_iomap;
	}

	g_tv_info.bus_clk = hal_clock_get(HAL_SUNXI_CCU, clk_id[0]);
	g_tv_info.rst_bus = hal_reset_control_get(HAL_SUNXI_RESET, RST_BUS_TVE_TOP);

	tve_low_set_top_reg_base(g_tv_info.base_addr);

	return 0;

err_iomap:
	return ret;
}
#endif

int tv_probe(void)
{
	int index = 0;
	char str[40] = {0};
	s32 ret = 0;

	hal_reset_id_t rst_id;
	hal_reset_type_t reset_type = HAL_SUNXI_RESET;
	hal_clk_type_t clk_type = HAL_SUNXI_CCU;
	hal_clk_id_t clk_id[TV_CLK_NUM] = {
						CLK_BUS_TVE_TOP,
						CLK_TVE,
						CLK_BUS_TVE,
						CLK_PLL_VIDEO1,
	};


	TV_DBG("tv probe start!\n");

	// Regist disp_device in disp2
	tv_module_init();

	tv_fake_detect = 0;
	if (!g_tv_info.tv_number)
		memset(&g_tv_info, 0, sizeof(struct tv_info_t));
	tv_power_enable_mask = 0;
	if (hal_cfg_get_keyvalue(main_name, "tv_power", (int32_t *)str, sizeof(str))) {
		TV_ERR("of_property_read_string tv_power failed!\n");
	} else {
		memcpy((void *)tv_power, str, strlen(str) + 1);
		tv_power_enable(tv_power);
	}

#if defined(TVE_TOP_SUPPORT)
	tv_top_init(clk_id);
	index = 1;
#endif

	g_tv_info.screen[tv_id].base_addr = (void*)(intptr_t)reg_base[index];
	if (IS_ERR_OR_NULL(g_tv_info.screen[tv_id].base_addr)) {
		pr_err("fail to get addr for tve%d!\n", tv_id);
		goto err_iomap;
	}

	g_tv_info.screen[tv_id].clk = hal_clock_get(clk_type, clk_id[index]);
	if (IS_ERR_OR_NULL(g_tv_info.screen[tv_id].clk)) {
		pr_err("fail to get clk for tve%d's!\n", tv_id);
		goto err_iomap;
	}

	index++;
	g_tv_info.screen[tv_id].bus_clk = hal_clock_get(clk_type, clk_id[index]);
	if (IS_ERR_OR_NULL(g_tv_info.screen[tv_id].bus_clk)) {
		pr_err("fail to get clk for tve%d's!\n", tv_id);
		goto err_iomap;
	}

	index++;
	g_tv_info.screen[tv_id].clk_parent = hal_clock_get(clk_type, clk_id[index]);
	if (IS_ERR_OR_NULL(g_tv_info.screen[tv_id].clk_parent)) {
		pr_err("fail to get clk for tve%d's!\n", tv_id);
		goto err_iomap;
	}

	g_tv_info.screen[tv_id].rst_bus =
		hal_reset_control_get(reset_type, RST_BUS_TVE);
	 if (g_tv_info.screen[tv_id].rst_bus == NULL) {
		pr_err("get tve bus reset control  failed!\n");
		goto err_iomap;
	}

	ret = tv_init();
	if (ret)
		goto err_iomap;

	g_tv_info.tv_number++;

	TV_DBG("tv probe finished!\n");

	return 0;
err_iomap:
	return -EINVAL;
}

static int tv_remove(void)
{
	tv_exit();
	return 0;
}

int tv_open(void)
{
	return 0;
}

int tv_release(void)
{
	 return 0;
}

int tv_ioctl(int cmd, void *arg)
{
	 return 0;
}


int tv_module_init(void)
{
	struct disp_tv_func disp_func;

	memset(&disp_func, 0, sizeof(struct disp_tv_func));
	disp_func.tv_enable = tv_enable;
	disp_func.tv_disable = tv_disable;
	disp_func.tv_resume = tv_resume;
	disp_func.tv_suspend = tv_suspend;
	disp_func.tv_get_mode = tv_get_mode;
	disp_func.tv_set_mode = tv_set_mode;
	disp_func.tv_get_video_timing_info =
	    tv_get_video_timing_info;
	disp_func.tv_get_input_csc = tv_get_input_csc;
	disp_func.tv_mode_support = tv_mode_support;
	disp_func.tv_hot_plugging_detect =
	    tv_hot_plugging_detect;
	disp_func.tv_set_enhance_mode = tv_set_enhance_mode;
	bsp_disp_tv_register(&disp_func);

	return 0;
}

