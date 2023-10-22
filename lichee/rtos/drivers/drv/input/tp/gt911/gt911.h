/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-01-13     RiceChen     the first version
 */

#ifndef __GT911_H__
#define __GT911_H__

#include <stdio.h>
#include <FreeRTOS.h>
#include <hal_timer.h>

#include "console.h"
#include "aw_types.h"
#include "sunxi-input.h"
#include "input-event-codes.h"
// #define DEBUG

#ifdef DEBUG
#define GT911_INFO(x...) printf("[gt911] " x)
#define GT911_FUNC_ENTER() printf("[gt911]%s: Enter\n", __func__)
#define GT911_FUNC_EXIT() printf("[gt911]%s: Exit\n", __func__)
#else
#define GT911_INFO(x...)
#define GT911_FUNC_ENTER()
#define GT911_FUNC_EXIT()
#endif

#define GT911_ERR(x...) printf("[gt911][error] " x)
#define GT911_LOG(x...) printf("[gt911][log] " x)

#define GT911_ADDR_LEN          2
#define GT911_REGITER_LEN       2
#define GT911_MAX_TOUCH         5
#define GT911_POINT_INFO_NUM    8

#define GT911_ADDRESS_HIGH      0x5D
#define GT911_ADDRESS_LOW       0x14

#define GT911_COMMAND_REG       0x8040
#define GT911_CONFIG_REG        0x8047

#define GT911_PRODUCT_ID        0x8140
#define GT911_VENDOR_ID         0x814A
#define GT911_READ_STATUS       0x814E

#define GT911_POINT1_REG        0x814F
#define GT911_POINT2_REG        0x8157
#define GT911_POINT3_REG        0x815F
#define GT911_POINT4_REG        0x8167
#define GT911_POINT5_REG        0x816F

#define GT911_CHECK_SUM         0x80FF

#define GT911_TOUCH_CTRL_GET_ID			0
#define GT911_TOUCH_CTRL_GET_INFO		1
#define GT911_TOUCH_CTRL_SET_X_RANGE	2
#define GT911_TOUCH_CTRL_SET_Y_RANGE	3
#define GT911_TOUCH_CTRL_SET_X_TO_Y		4
#define GT911_TOUCH_CTRL_SET_MODE		5

#define swap(a, b) \
	do { typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; } while (0)

struct gt911_hw_cfg {
	int twi_id;
	int addr;
	int int_gpio;
	int reset_gpio;
	int screen_max_x;
	int screen_max_y;
	int revert_x_flag;
	int revert_y_flag;
	int exchange_x_y_flag;
};

struct ts_event {
	u16 x1;
	u16 y1;
	u16 x2;
	u16 y2;
	u16 x3;
	u16 y3;
	u16 x4;
	u16 y4;
	u16 x5;
	u16 y5;
	u16 pressure;
	u8 touch_point;
};

struct gt911_drv_data {
	struct gt911_hw_cfg *config;
	int irq_num;

	xSemaphoreHandle irq_sem;
	SemaphoreHandle_t mutex;

	struct sunxi_input_dev *input_dev;
	struct ts_event event;
};

#endif /* gt911.h */