/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-01-13     RiceChen     the first version
 */

/*
sysconfig
[touchscreen]
tp_used                = 1
tp_int                 = port:PA13<1><0><default><default>
tp_reset               = port:PB3<1><0><default><default>
;tp_revert_mode; 0: default; 1: revert x; 2: revery y
tp_revert_mode         = 0
;tp_exchange_flag; 0: default; 1:exchange x y
tp_exchange_flag       = 0
tp_max_x               = 480
tp_max_y               = 320
;tp_addr ;0x5D or 0x14
tp_addr                = 0x5D
tp_twi_id              = 1
*/

#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <hal_timer.h>

#include "console.h"
#include "aw_types.h"
#include "hal_gpio.h"
#include "sunxi_hal_twi.h"
#include "sunxi-input.h"
#include "input-event-codes.h"
#include "gt911.h"
#include "script.h"
#include "hal_cfg.h"
#if defined(CONFIG_DRIVERS_GPIO_EX_AW9523)
#include <gpio/aw9523.h>
#endif

#define SET_TP0					"touchscreen"
#define KEY_TP_ENABLE			"tp_used"
#define KEY_TP_TWI_ID			"tp_twi_id"
#define KEY_TP_INT_GPIO			"tp_int"
#define KEY_TP_RESET_GPIO		"tp_reset"
#define KEY_TP_REVERT_MODE		"tp_revert_mode"
#define KEY_TP_EXCHANGE_FLAG	"tp_exchange_flag"
#define KEY_TP_MAX_X			"tp_max_x"
#define KEY_TP_MAX_Y			"tp_max_y"
#define KEY_TP_ADDR				"tp_addr"

#define GT911_DEV_NAME	"touchscreen"
#define INT_GPIO_MUX	0

#if defined(CONFIG_ARCH_SUN20IW2)
static struct gt911_hw_cfg gt911_cfg = {
	.twi_id = 1,
	.addr = GT911_ADDRESS_LOW,
	.int_gpio = GPIOA(13),
	.reset_gpio = GPIOB(3),
	.screen_max_x = 480,
	.screen_max_y = 320,
	.revert_x_flag = 0,
	.revert_y_flag = 0,
	.exchange_x_y_flag = 0,
};
#endif

struct gt911_drv_data *gt911_data = NULL;
// static struct rt_i2c_client gt911_data;

static inline void input_report_key_t(struct sunxi_input_dev *dev, unsigned int code, int value)
{
	sunxi_input_event(dev, EV_KEY, code, !!value);
}

static inline void input_report_abs_t(struct sunxi_input_dev *dev, unsigned int code, int value)
{
	sunxi_input_event(dev, EV_ABS, code, value);
}

static inline void input_sync_t(struct sunxi_input_dev *dev)
{
	sunxi_input_event(dev, EV_SYN, SYN_REPORT, 0);
	taskYIELD();
}

static inline void input_mt_sync_t(struct sunxi_input_dev *dev)
{
	sunxi_input_event(dev, EV_SYN, SYN_MT_REPORT, 0);
}

/* hardware section */
static uint8_t GT911_CFG_TBL[] =
{
	0x6b, 0x00, 0x04, 0x58, 0x02, 0x05, 0x0d, 0x00, 0x01, 0x0f,
	0x28, 0x0f, 0x50, 0x32, 0x03, 0x05, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8a, 0x2a, 0x0c,
	0x45, 0x47, 0x0c, 0x08, 0x00, 0x00, 0x00, 0x40, 0x03, 0x2c,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x64, 0x32, 0x00, 0x00,
	0x00, 0x28, 0x64, 0x94, 0xd5, 0x02, 0x07, 0x00, 0x00, 0x04,
	0x95, 0x2c, 0x00, 0x8b, 0x34, 0x00, 0x82, 0x3f, 0x00, 0x7d,
	0x4c, 0x00, 0x7a, 0x5b, 0x00, 0x7a, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x18, 0x16, 0x14, 0x12, 0x10, 0x0e, 0x0c, 0x0a,
	0x08, 0x06, 0x04, 0x02, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x16, 0x18, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21,
	0x22, 0x24, 0x13, 0x12, 0x10, 0x0f, 0x0a, 0x08, 0x06, 0x04,
	0x02, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x79, 0x01,
};

int gt911_i2c_read(struct gt911_drv_data *data, char *writebuf, int writelen,
			char *readbuf, int readlen)
{
	int ret = 0;
	struct twi_msg msgs[2];
	u8 i2c_port = data->config->twi_id;
	uint16_t flags = 0;

	if (readlen > 0) {
		if (writelen > 0) {
			msgs[0].addr = data->config->addr;
			msgs[0].flags = flags & TWI_M_TEN;
			msgs[0].flags &= ~(TWI_M_RD);
			msgs[0].len = writelen;
			msgs[0].buf = writebuf;

			msgs[1].addr = data->config->addr;
			msgs[1].flags = flags & TWI_M_TEN;
			msgs[1].flags |= TWI_M_RD;
			msgs[1].len = readlen;
			msgs[1].buf = readbuf;

			ret = hal_twi_xfer(i2c_port, msgs, 1);
			if (ret < 0) {
				GT911_ERR("[IIC]: i2c_transfer(1) error, addr= 0x%02x%02x!!\n",
					 writebuf[0], writebuf[1]);
				GT911_ERR("[IIC]: i2c_transfer(1) error, ret=%d, rlen=%d, wlen=%d!!\n",
					 ret, readlen, writelen);
			} else {
				ret =
					hal_twi_xfer(i2c_port, &msgs[1], 1);
				if (ret < 0) {
					GT911_ERR("[IIC]: i2c_transfer(2) error, addr= 0x%x!!\n",
						 writebuf[0]);
					GT911_ERR("[IIC]: i2c_transfer(2) error, ret=%d, rlen=%d, wlen=%d!!\n",
						 ret, readlen, writelen);
				}
			}
		} else {
			msgs[1].addr = data->config->addr;
			msgs[1].flags = TWI_M_RD;
			msgs[1].len = readlen;
			msgs[1].buf = readbuf;

			ret = hal_twi_xfer(i2c_port, &msgs[1], 1);
			if (ret < 0) {
				GT911_ERR("[IIC]: i2c_transfer(read) error, ret=%d, rlen=%d, wlen=%d!!",
					 ret, readlen, writelen);
			}
		}
	}

	return ret;
}

int gt911_i2c_write(struct gt911_drv_data *data, char *writebuf, int writelen)
{
	int ret = 0;
	u8 i2c_port = data->config->twi_id;
	struct twi_msg msgs;

	if (writelen > 0) {
		msgs.addr = data->config->addr;
		msgs.flags = 0;
		msgs.len = writelen;
		msgs.buf = writebuf;

		ret = hal_twi_xfer(i2c_port, &msgs, 1);
		if (ret < 0) {
			GT911_ERR("[IIC]: i2c_transfer(write) error, ret=%d!!\n",
				 ret);
		}
	}

	return ret;
}

static int gt911_get_product_id(struct gt911_drv_data *dev)
{
	uint8_t reg[2];
	uint8_t data[6];
	uint8_t len = 6;

	reg[0] = (uint8_t)(GT911_PRODUCT_ID >> 8);
	reg[1] = (uint8_t)(GT911_PRODUCT_ID & 0xff);

	if (gt911_i2c_read(dev, reg, 2, data, len) != 0) {
		GT911_ERR("read id failed");
		return -1;
	}
	GT911_LOG("PRODUCT_ID=%02X:%02X:%02X:%02X:%02X:%02X\n",
			data[0],data[1],data[2],data[3],data[4],data[5]);
	return 0;
}

static int gt911_get_info(struct gt911_drv_data *dev)
{
	uint8_t reg[2];
	uint8_t out_info[7];
	uint8_t out_len = 7;

	reg[0] = (uint8_t)(GT911_CONFIG_REG >> 8);
	reg[1] = (uint8_t)(GT911_CONFIG_REG & 0xFF);

	if (gt911_i2c_read(dev, reg, 2, out_info, out_len) != 0) {
		GT911_ERR("read info failed");
		return -1;
	}
	GT911_LOG("verison=%02X range_x=%d range_y=%d point_num=%d\n", out_info[0], (out_info[2] << 8) | out_info[1],
			(out_info[4] << 8) | out_info[3], out_info[5] & 0x0f);
	return 0;
}

static int gt911_config_freq(struct gt911_drv_data *dev)
{
	uint8_t buf[3];

	buf[0] = (uint8_t)(0x8056 >> 8);
	buf[1] = (uint8_t)(0x8056 & 0xFF);
	buf[2] = 0x0f;

	if (gt911_i2c_write(dev, buf, 3) != 0) {
    	GT911_ERR("soft reset failed");
    	return -1;
	}
	return 0;
}

static int gt911_init_config(struct gt911_drv_data *data)
{
	uint8_t buf[4];
	uint8_t *config;

	config = (uint8_t *)malloc(sizeof(GT911_CFG_TBL) + GT911_REGITER_LEN);
	if (config == NULL) {
		GT911_INFO("malloc config memory failed\n");
		return -1;
	}
	memcpy(&config[2], GT911_CFG_TBL, sizeof(GT911_CFG_TBL));

	config[0] = (uint8_t)((GT911_CONFIG_REG >> 8) & 0xFF);
	config[1] = (uint8_t)(GT911_CONFIG_REG & 0xFF);
	config[8] &= 0xFC; //RT_DEVICE_FLAG_INT_RX
	// config[8] |= 0x02; //RT_DEVICE_FLAG_RDONLY
	config[16] &= 0x0F; //rate
	if (gt911_i2c_write(data, config, sizeof(GT911_CFG_TBL) + GT911_ADDR_LEN) != 0) {
		GT911_INFO("send config failed");
		return -1;
	}

	buf[0] = (uint8_t)((GT911_CHECK_SUM >> 8) & 0xFF);
	buf[1] = (uint8_t)(GT911_CHECK_SUM & 0xFF);
	buf[2] = 0;

	for (int i = GT911_ADDR_LEN; i < sizeof(GT911_CFG_TBL) + GT911_ADDR_LEN; i++) {
		buf[GT911_ADDR_LEN] += config[i];
	}

	buf[2] = (~buf[2]) + 1;
	buf[3] = 0; //（0,参数不保存到flash 1,参数保存到flash）

	gt911_i2c_write(data, buf, 4);
	free(config);
	return 0;
}

//todo
static int gt911_update_data(struct gt911_drv_data *parm)
{
	struct gt911_drv_data *data = parm;
	struct gt911_hw_cfg *config = data->config;
	struct ts_event *event = &data->event;
	uint8_t point_status = 0;
	int ret = 0;
	int i = 0;
	int x, y;
	u8 tlsc_pressure, tlsc_size;
	uint8_t write_buf[3];
	uint8_t read_buf[GT911_POINT_INFO_NUM * GT911_MAX_TOUCH] = {0};
	int read_num = 0;
	static int touch_num = 0;

	/* point status register */
	write_buf[0] = (uint8_t)((GT911_READ_STATUS >> 8) & 0xFF);
	write_buf[1] = (uint8_t)(GT911_READ_STATUS & 0xFF);

	ret = gt911_i2c_read(data, write_buf, 2, &point_status, 1);
	// GT911_ERR("point_status = %02X\n", point_status);

	if (ret < 0) {
		GT911_ERR("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		goto exit_;
	}
	if (point_status == 0) { /* no data */
		read_num = 0;
		goto exit_;
	}
	if ((point_status & 0x80) == 0) { /* data is not ready */
		GT911_INFO(" data is not ready\n");
		read_num = 0;
		goto exit_;
	}
	if ((touch_num == 0 && point_status & 0x0f == 0)) {
		goto exit_;
	}
	touch_num = point_status & 0x0f;  /* get point num */

	if (touch_num > GT911_MAX_TOUCH) {/* point num is not correct */
		read_num = 0;
		goto exit_;
	}

	read_num = touch_num;

	write_buf[0] = (uint8_t)((GT911_POINT1_REG >> 8) & 0xFF);
	write_buf[1] = (uint8_t)(GT911_POINT1_REG & 0xFF);
	ret = gt911_i2c_read(data, write_buf, 2, read_buf, read_num * GT911_POINT_INFO_NUM);

	if (ret < 0) {
		GT911_ERR("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		goto exit_;
	}

	memset(event, 0, sizeof(struct ts_event));
	event->touch_point = touch_num;

	for (i = 0; i < touch_num; i++) {
		int off_set = i * 8;
		int read_id = read_buf[off_set] & 0x0F;
		x = read_buf[off_set + 1] | (read_buf[off_set + 2] << 8);	/* x */
		y = read_buf[off_set + 3] | (read_buf[off_set + 4] << 8);	/* y */
		int w = read_buf[off_set + 5] | (read_buf[off_set + 6] << 8);	/* size */
		// GT911_ERR("id=%d,x=%d,y=%d,w=%d\n", read_id, x, y, w);

		if (config->exchange_x_y_flag)
			swap(x, y);
		if (config->revert_x_flag)
			x = config->screen_max_x - x;
		if (config->revert_y_flag)
			y = config->screen_max_y - y;

		tlsc_pressure = w;
		if (tlsc_pressure > 127) {
			tlsc_pressure = 127;
		}
		tlsc_size = touch_num;

		input_report_abs_t(data->input_dev, ABS_MT_TRACKING_ID,
				read_id);
		if (y == 1500) {
			if (x == 40) {
				input_report_key_t(data->input_dev,
							KEY_MENU, 1);
			}
			if (x == 80) {
				input_report_key_t(data->input_dev,
							KEY_HOMEPAGE, 1);
			}
			if (x == 120) {
				input_report_key_t(data->input_dev,
							KEY_BACK, 1);
			}
		} else {
			input_report_abs_t(data->input_dev,
						ABS_MT_POSITION_X, x);
			input_report_abs_t(data->input_dev,
						ABS_MT_POSITION_Y, y);

			input_report_abs_t(data->input_dev,
						ABS_MT_PRESSURE, 15);
			input_report_abs_t(data->input_dev,
						ABS_MT_TOUCH_MAJOR, tlsc_size);
			input_report_key_t(data->input_dev, BTN_TOUCH, 1);
		}
		input_mt_sync_t(data->input_dev);
	}
	if (event->touch_point == 0) {
		if (y == 1500) {
			if (x == 40) {
				input_report_key_t(data->input_dev, KEY_MENU, 0);
			}
			if (x == 80) {
				input_report_key_t(data->input_dev, KEY_HOMEPAGE,
						 0);
			}
			if (x == 120) {
				input_report_key_t(data->input_dev, KEY_BACK, 0);
			}
		}
		// input_report_abs_t(data->input_dev, ABS_MT_TRACKING_ID,
		// 		0xffffffff);
		input_report_key_t(data->input_dev, BTN_TOUCH, 0);
	}
	input_sync_t(data->input_dev);

exit_:
	write_buf[0] = (uint8_t)((GT911_READ_STATUS >> 8) & 0xFF);
	write_buf[1] = (uint8_t)(GT911_READ_STATUS & 0xFF);
	write_buf[2] = 0x00;
	gt911_i2c_write(data, write_buf, 3);

	return 0;

}

void touch_event_handler(void *parm)
{

	GT911_INFO("========enter touch_event_handler=====\n");
	struct gt911_drv_data *data = (struct gt911_drv_data *)parm;

	xSemaphoreTake(data->mutex, portMAX_DELAY);

	while (1) {
		//wait irq wakeup.
		xSemaphoreTake(data->irq_sem, portMAX_DELAY);
		gt911_update_data(data);
	}

	xSemaphoreGive(data->mutex);
}

static hal_irqreturn_t gt911_irq_handler(void *parm)
{
	struct gt911_drv_data *data = (struct gt911_drv_data *)parm;
	BaseType_t sem_ret, taskwoken = pdFALSE;
	hal_irqreturn_t ret;

	GT911_INFO("========enter gt911_irq_handler=====\n");
	sem_ret = xSemaphoreGiveFromISR(data->irq_sem, &taskwoken);
	if (sem_ret == pdTRUE) {
		ret = HAL_IRQ_OK;
		return ret;
	} else {
		GT911_INFO("irq give sem err\n");
		ret = HAL_IRQ_ERR;
		return ret;
	}
}

static int gt911_tpd_reset(struct gt911_drv_data *dev)
{
	uint8_t buf[3];

	buf[0] = (uint8_t)(GT911_COMMAND_REG >> 8);
	buf[1] = (uint8_t)(GT911_COMMAND_REG & 0xFF);
	buf[2] = 0x02;

	if (gt911_i2c_write(dev, buf, 3) != 0) {
		GT911_ERR("soft reset failed");
		return -1;
	}
	return 0;
}

int32_t tp_cfg_helper_fun(char *SecName, char *KeyName, int* Value)
{
	int ret = 0;
	int value = 0;
	ret = hal_cfg_get_keyvalue(SecName, KeyName, (int32_t *)&value, 1);
	GT911_INFO("revert mode:%d, ret:%d\n", value, ret);
	if (ret) {
		GT911_ERR("script_parser_fetch %s %s fail \n", SecName, KeyName);
		return -1;
	}
	*Value = value;
	return 0;
}

int32_t gt911_pinctl_set_from_cfg(struct gt911_drv_data *dev)
{
	struct gt911_hw_cfg *cfg = dev->config;
#ifdef CONFIG_DRIVER_SYSCONFIG
	int ret = -1, value = 0;
	user_gpio_set_t gpio_set = {0};

	ret = tp_cfg_helper_fun(SET_TP0, KEY_TP_ENABLE, &(value));
	if (ret != 0) {
		GT911_INFO("touchscreen not enabled!\n");
		return -1;
	}
	tp_cfg_helper_fun(SET_TP0, KEY_TP_TWI_ID, &(cfg->twi_id));
	tp_cfg_helper_fun(SET_TP0, KEY_TP_REVERT_MODE, &(cfg->revert_x_flag));
	tp_cfg_helper_fun(SET_TP0, KEY_TP_EXCHANGE_FLAG, &(cfg->exchange_x_y_flag));
	tp_cfg_helper_fun(SET_TP0, KEY_TP_MAX_X, &(cfg->screen_max_x));
	tp_cfg_helper_fun(SET_TP0, KEY_TP_MAX_Y, &(cfg->screen_max_y));
	tp_cfg_helper_fun(SET_TP0, KEY_TP_ADDR, &(cfg->addr));

	memset(&gpio_set, 0x00, sizeof(gpio_set));
	ret = hal_cfg_get_keyvalue(SET_TP0, KEY_TP_INT_GPIO, (int32_t *)&gpio_set, sizeof(user_gpio_set_t) >> 2);
	GT911_INFO("get %s, ret:%d\n", KEY_TP_INT_GPIO, ret);
	if (ret == 0) {
		cfg->int_gpio = (gpio_set.port - 1) * 32 + gpio_set.port_num;
		GT911_INFO("%s gpio %d\n", KEY_TP_INT_GPIO, cfg->int_gpio);
	} else {
		GT911_ERR("script_parser_fetch %s %s fail \n", SET_TP0, KEY_TP_INT_GPIO);
	}
	memset(&gpio_set, 0x00, sizeof(gpio_set));
	ret = hal_cfg_get_keyvalue(SET_TP0, KEY_TP_RESET_GPIO, (int32_t *)&gpio_set, sizeof(user_gpio_set_t) >> 2);
	GT911_INFO("get %s, ret:%d\n", KEY_TP_RESET_GPIO, ret);
	if (ret == 0) {
#if defined(CONFIG_DRIVERS_GPIO_EX_AW9523)
		if (gpio_set.port >= 21) { //PU
			cfg->reset_gpio = aw9523_gpio_get_num(gpio_set.port_num);
		} else {
			cfg->reset_gpio = (gpio_set.port - 1) * 32 + gpio_set.port_num;
		}
#else
		cfg->reset_gpio = (gpio_set.port - 1) * 32 + gpio_set.port_num;
#endif
		GT911_INFO("%s gpio %d\n", KEY_TP_RESET_GPIO, cfg->reset_gpio);
	} else {
		GT911_ERR("script_parser_fetch %s %s fail \n", SET_TP0, KEY_TP_RESET_GPIO);
	}
	return ret;
#else
    GT911_INFO("unsupport sys fex, using default config\n");
	return 0;
#endif
}

static int gt911_hw_init(struct gt911_drv_data *data)
{
	int ret;
	int reset_count = 0;
	struct gt911_hw_cfg *config = data->config;

	ret = gt911_pinctl_set_from_cfg(data);
	if (ret < 0) {
		GT911_ERR("sysconfig err\n");
		return -1;
	}
	/* hw init*/
	ret = hal_gpio_set_direction(config->int_gpio, GPIO_DIRECTION_OUTPUT);
	if (ret < 0) {
		GT911_ERR("int gpio init err\n");
		return -1;
	}
	/* hw init*/
	ret = hal_gpio_set_direction(config->reset_gpio, GPIO_DIRECTION_OUTPUT);
	if (ret < 0) {
		GT911_ERR("reset gpio init err\n");
		return -1;
	}
#if 1
// When using the lvgl, this cannot be enabled
	hal_gpio_set_data(config->reset_gpio, 0);
	hal_gpio_set_data(config->int_gpio, 0);
	vTaskDelay(pdMS_TO_TICKS(100));
	hal_gpio_set_data(config->int_gpio, 1);
	vTaskDelay(pdMS_TO_TICKS(10));
	hal_gpio_set_data(config->reset_gpio, 1);
	vTaskDelay(pdMS_TO_TICKS(100));

#else
	hal_gpio_set_data(config->int_gpio, 0);
	vTaskDelay(pdMS_TO_TICKS(50));
	hal_gpio_set_data(config->int_gpio, 1);
	vTaskDelay(pdMS_TO_TICKS(50));
	hal_gpio_set_data(config->int_gpio, 0);
	vTaskDelay(pdMS_TO_TICKS(50));
#endif
	ret = hal_gpio_set_direction(config->int_gpio, GPIO_DIRECTION_INPUT);
	if (ret < 0) {
		GT911_ERR("reset gpio init err\n");
		return -1;
	}
	ret = hal_gpio_pinmux_set_function(config->int_gpio, INT_GPIO_MUX);
	if (ret < 0) {
		GT911_ERR("int gpio init err\n");
		return -1;
	}

	ret = hal_gpio_to_irq(config->int_gpio, &data->irq_num);
	if (ret < 0) {
		GT911_ERR("get irq num err\n");
		return -1;
	}
	ret = gt911_tpd_reset(data);
	if (ret < 0) {
		GT911_ERR("tpd reset err\n");
		return -1;
	}
	// gt911_init_config(data);
	gt911_config_freq(data);
	gt911_get_product_id(data);
	gt911_get_info(data);
	return 0;
}

static inline void get_gt911_cfg(struct gt911_drv_data *data)
{
	data->config = &gt911_cfg;
}

int gt911_init(void)
{
	int ret;
	struct sunxi_input_dev *input_dev;

	gt911_data = malloc(sizeof(struct gt911_drv_data));
	if (NULL == gt911_data) {
		GT911_ERR("malloc gt911_data err\n");
		return -1;
	}

	get_gt911_cfg(gt911_data);

	hal_twi_init(gt911_data->config->twi_id);
	hal_twi_control(gt911_data->config->twi_id, I2C_SLAVE,
			&gt911_data->config->addr);
	ret = gt911_hw_init(gt911_data);
	if (ret < 0) {
		GT911_ERR("gt911_hw_init err\n");
		goto err_free_data;
	}

	gt911_data->mutex = xSemaphoreCreateMutex();
	if (NULL == gt911_data->mutex) {
		GT911_ERR("mutex init err\n");
		goto err_disable_irq;
	}

	gt911_data->irq_sem = xSemaphoreCreateBinary();
	if (NULL == gt911_data->irq_sem) {
		GT911_ERR("irq_sem init err\n");
		goto err_free_mutex;
	}

	ret = hal_gpio_irq_request(gt911_data->irq_num, gt911_irq_handler,
			IRQ_TYPE_EDGE_FALLING, gt911_data);
	if (ret < 0) {
		GT911_ERR("irq request err\n");
		goto err_free_data;
	}
	ret = hal_gpio_irq_enable(gt911_data->irq_num);
	if (ret < 0) {
		GT911_ERR("irq request err\n");
		goto err_free_irq;
	}

	input_dev = sunxi_input_allocate_device();
	if (NULL == input_dev) {
		GT911_ERR("input dev alloc err\n");
		goto err_free_sem;

	}
	input_dev->name = GT911_DEV_NAME;
	input_set_capability(input_dev, EV_ABS, ABS_MT_POSITION_X);
	input_set_capability(input_dev, EV_ABS, ABS_MT_POSITION_Y);
	input_set_capability(input_dev, EV_ABS, ABS_MT_TOUCH_MAJOR);
	input_set_capability(input_dev, EV_ABS, ABS_MT_WIDTH_MAJOR);
	input_set_capability(input_dev, EV_ABS, ABS_MT_TRACKING_ID);
	input_set_capability(input_dev, EV_KEY, KEY_MENU);
	input_set_capability(input_dev, EV_KEY, BTN_TOUCH);
	input_set_capability(input_dev, EV_KEY, KEY_BACK);
	input_set_capability(input_dev, EV_KEY, KEY_HOMEPAGE);
	sunxi_input_register_device(input_dev);
	gt911_data->input_dev = input_dev;

	portBASE_TYPE task_ret;
	task_ret = xTaskCreate(touch_event_handler, (signed portCHAR *) "touch_event_handle_task", 1024, gt911_data, 0, NULL);
	if (task_ret != pdPASS) {
		GT911_ERR("input dev alloc err\n");
		goto err_free_sem;

	}

	GT911_INFO("gt911_data6x_init success\n");

	{ //temporary fix the problem of pro version borad
		vTaskDelay(pdMS_TO_TICKS(4000));
		int ret;

		/* hw init*/
		ret = hal_gpio_set_direction(gt911_cfg.int_gpio, GPIO_DIRECTION_OUTPUT);
		if (ret < 0) {
			GT911_ERR("int gpio init err\n");
			return -1;
		}

		hal_gpio_set_data(gt911_cfg.int_gpio, 0);
		vTaskDelay(pdMS_TO_TICKS(50));
		hal_gpio_set_data(gt911_cfg.int_gpio, 1);
		vTaskDelay(pdMS_TO_TICKS(50));
		hal_gpio_set_data(gt911_cfg.int_gpio, 0);
		vTaskDelay(pdMS_TO_TICKS(50));

		ret = hal_gpio_irq_request(gt911_data->irq_num, gt911_irq_handler,
				IRQ_TYPE_EDGE_FALLING, gt911_data);
		if (ret < 0) {
			GT911_ERR("irq request err\n");
		}
		ret = hal_gpio_irq_enable(gt911_data->irq_num);
		if (ret < 0) {
			GT911_ERR("irq request err\n");
		}
	}
	return 0;

err_free_sem:
	vSemaphoreDelete(gt911_data->irq_sem);

err_free_mutex:
	vSemaphoreDelete(gt911_data->mutex);

err_disable_irq:
	hal_gpio_irq_disable(gt911_data->irq_num);

err_free_irq:
	hal_gpio_irq_free(gt911_data->irq_num);

err_free_data:
	free(gt911_data);
	gt911_data = NULL;

	return -1;
}

int gt911_deinit(void)
{
	if (NULL == gt911_data) {
		GT911_ERR("gt911 is not init\n");
		return -1;
	}

	vSemaphoreDelete(gt911_data->irq_sem);

	vSemaphoreDelete(gt911_data->mutex);

	hal_gpio_irq_disable(gt911_data->irq_num);

	hal_gpio_irq_free(gt911_data->irq_num);

	free(gt911_data);

	gt911_data = NULL;

	return 0;
}

#ifdef CONFIG_DRIVERS_TEST_TOUCHSCREEN

void tp_test_task(void *parm)
{
	int fd;
	int x = -1, y = -1, z = -1;
	struct sunxi_input_event event;

	memset(&event, 0, sizeof(struct sunxi_input_event));

	fd = sunxi_input_open(GT911_DEV_NAME);
	if (fd < 0) {
		printf("gpio key open err\n");
		vTaskDelete(NULL);
	}

	while(1) {
		sunxi_input_read(fd, &event, sizeof(struct sunxi_input_event));
		//printf("read event %d %d %d", event.type, event.code, event.value);

		if (event.type == EV_ABS) {
			switch (event.code) {
				case ABS_MT_POSITION_X:
					x = event.value;
					break;
				case ABS_MT_POSITION_Y:
					y = event.value;
					break;
				case ABS_MT_TRACKING_ID:
					z = event.value;
					break;
			}
		} else if (event.type == EV_KEY) {
			if (event.code == BTN_TOUCH) {
				if (event.value == 0)
					printf("event.code == BTN_TOUCH, value = %d\n", event.value);
			}
		}
		if (x >= 0 && y >= 0) {
			printf("====press (%d, %d)====\n", x, y);
			x = -1;
			y = -1;
			z = -1;
		}
	}

	vTaskDelete(NULL);
}

static void cmd_gt911_test(int argc, char **argv)
{
	int ret = -1;
	uint16_t addr = gt911_cfg.addr;
	ret = gt911_init();

	if (ret < 0)
		printf("gt911 init fail\n");

	portBASE_TYPE task_ret;
	task_ret = xTaskCreate(tp_test_task, (signed portCHAR *) "tp_test_task", 1024, NULL, 0, NULL);
	if (task_ret != pdPASS) {
		printf("create tp read task err\n");
	}
}
FINSH_FUNCTION_EXPORT_CMD(cmd_gt911_test, gt911, gt911-test);

#endif
