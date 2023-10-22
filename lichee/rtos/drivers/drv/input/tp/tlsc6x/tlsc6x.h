#ifndef TLSC6X_H
#define TLSC6X_H

#include <stdio.h>
#include <FreeRTOS.h>
#include <hal_timer.h>

#include "console.h"
#include "aw_types.h"
#include "sunxi-input.h"
#include "input-event-codes.h"

#ifdef DEBUG
#define tlsc_info(x...) printf("[tlsc] " x)
#define TLSC_FUNC_ENTER() printf("[tlsc]%s: Enter\n", __func__)
#define TLSC_FUNC_EXIT() printf("[tlsc]%s: Exit\n", __func__)
#else
#define tlsc_info(x...)
#define TLSC_FUNC_ENTER()
#define TLSC_FUNC_EXIT()
#endif

#define tlsc_err(x...) printf("[tlsc][error] " x)

#define swap(a, b) \
	do { typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; } while (0)

struct tlsc6x_hw_cfg {
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
struct tlsc6x_drv_data {
	struct tlsc6x_hw_cfg	*config;
	int irq_num;
	osal_timer_t tp_timer;

	xSemaphoreHandle	irq_sem;
	SemaphoreHandle_t	mutex;

	struct sunxi_input_dev *input_dev;
	struct ts_event		event;

};

int tlsc6x_i2c_read(struct tlsc6x_drv_data *data, char *writebuf, int writelen,
			char *readbuf, int readlen);

int tlsc6x_i2c_write(struct tlsc6x_drv_data *data, char *writebuf, int writelen);

int tlsc6x_tp_dect(struct tlsc6x_drv_data *data);
#endif  /*TLSC6X_H*/
