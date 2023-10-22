#include <stdio.h>
#include <FreeRTOS.h>
#include <hal_timer.h>

#include "console.h"
#include "aw_types.h"
#include "hal_gpio.h"
#include "sunxi_hal_twi.h"
#include "sunxi-input.h"
#include "input-event-codes.h"
#include "tlsc6x.h"

#define TLSC6X_TEST_TASK
//#define DEBUG

#define	TS_MAX_FINGER	2

#define TLSC6X_DEV_NAME	"touchscreen"
#define INT_GPIO_MUX	0

//tp config init.
#if defined(CONFIG_ARCH_SUN8IW18P1)
struct tlsc6x_hw_cfg tlsc6x_cfg = {
	.twi_id = 1,
	.addr = 0x2e,
	.int_gpio = GPIOB(8),
	.reset_gpio =GPIOH(8),
	.screen_max_x = 240,
	.screen_max_y = 320,
	.revert_x_flag = 1,
	.revert_y_flag = 0,
	.exchange_x_y_flag = 1,
};
#endif

#if defined(CONFIG_ARCH_SUN20IW2)
struct tlsc6x_hw_cfg tlsc6x_cfg = {
	.twi_id = 0,
	.addr = 0x2e,
	.int_gpio = GPIOA(27),
	.reset_gpio = GPIOA(28),
	.screen_max_x = 240,
	.screen_max_y = 320,
	.revert_x_flag = 1,
	.revert_y_flag = 0,
	.exchange_x_y_flag = 1,
};
#endif

struct tlsc6x_drv_data *tlsc6x_data = NULL;
int g_is_telink_comp;

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
}

static inline void input_mt_sync_t(struct sunxi_input_dev *dev)
{
	sunxi_input_event(dev, EV_SYN, SYN_MT_REPORT, 0);
}

int tlsc6x_i2c_read(struct tlsc6x_drv_data *data, char *writebuf, int writelen,
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
				tlsc_err
				    ("[IIC]: i2c_transfer(1) error, addr= 0x%02x%02x!!\n",
				     writebuf[0], writebuf[1]);
				tlsc_err
				    ("[IIC]: i2c_transfer(1) error, ret=%d, rlen=%d, wlen=%d!!\n",
				     ret, readlen, writelen);
			} else {
				ret =
				    hal_twi_xfer(i2c_port, &msgs[1], 1);
				if (ret < 0) {
					tlsc_err
					    ("[IIC]: i2c_transfer(2) error, addr= 0x%x!!\n",
					     writebuf[0]);
					tlsc_err
					    ("[IIC]: i2c_transfer(2) error, ret=%d, rlen=%d, wlen=%d!!\n",
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
				tlsc_err
				    ("[IIC]: i2c_transfer(read) error, ret=%d, rlen=%d, wlen=%d!!",
				     ret, readlen, writelen);
			}
		}
	}

	return ret;
}


int tlsc6x_i2c_write(struct tlsc6x_drv_data *data, char *writebuf, int writelen)
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
			tlsc_err("[IIC]: i2c_transfer(write) error, ret=%d!!\n",
				 ret);
		}
	}

	return ret;
}

static int tlsc6x_update_data(struct tlsc6x_drv_data *parm)
{
	struct tlsc6x_drv_data *data = parm;
	struct tlsc6x_hw_cfg *config = data->config;
	struct ts_event *event = &data->event;
	u8 buf[20] = { 0 };
	int ret = -1;
	int i;
	u16 x, y;
	u8 tlsc_pressure, tlsc_size;

	ret = tlsc6x_i2c_read(data, buf, 1, buf, 18);
	if (ret < 0) {
		tlsc_err("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return ret;
	}

	memset(event, 0, sizeof(struct ts_event));
	event->touch_point = buf[2] & 0x07;


	for (i = 0; i < TS_MAX_FINGER; i++) {
		if ((buf[6 * i + 3] & 0xc0) == 0xc0) {
			continue;
		}
		x = (s16) (buf[6 * i + 3] & 0x0F) << 8 | (s16) buf[6 * i + 4];
		y = (s16) (buf[6 * i + 5] & 0x0F) << 8 | (s16) buf[6 * i + 6];

		if (config->exchange_x_y_flag)
			swap(x, y);
		if (config->revert_x_flag)
			x = config->screen_max_x - x;
		if (config->revert_y_flag)
			y = config->screen_max_y - y;

		tlsc_pressure = buf[6 * i + 7];
		if (tlsc_pressure > 127) {
			tlsc_pressure = 127;
		}
		tlsc_size = (buf[6 * i + 8] >> 4) & 0x0F;

		if ((buf[6 * i + 3] & 0x40) == 0x0) {
			input_report_abs_t(data->input_dev, ABS_MT_TRACKING_ID,
					buf[6 * i + 5] >> 4);
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
		input_report_key_t(data->input_dev, BTN_TOUCH, 0);
	}
	input_sync_t(data->input_dev);

	return 0;

}

void tp_timer_func(void *parm)
{
	struct tlsc6x_drv_data *data = (struct tlsc6x_drv_data *)parm;
	tlsc6x_update_data(data);
	/*osal_timer_delete(data->tp_timer);*/
}

void touch_event_handler(void *parm)
{

	tlsc_info("========enter touch_event_handler=====\n");
	struct tlsc6x_drv_data *data = (struct tlsc6x_drv_data *)parm;

	unsigned long time_interval = MS_TO_OSTICK(100);
	data->tp_timer = osal_timer_create("tlsc6x_timer", tp_timer_func,
					parm, time_interval,
					OSAL_TIMER_FLAG_PERIODIC);

	xSemaphoreTake(data->mutex, portMAX_DELAY);

	while (1) {
		//wait irq wakeup.
		xSemaphoreTake(data->irq_sem, portMAX_DELAY);

		tlsc6x_update_data(data);
		osal_timer_control(data->tp_timer, OSAL_TIMER_CTRL_SET_TIME, &time_interval);
	}

	xSemaphoreGive(data->mutex);
}

static hal_irqreturn_t tlsc6x_irq_handler(void *parm)
{
	struct tlsc6x_drv_data *data = (struct tlsc6x_drv_data *)parm;
	BaseType_t sem_ret, taskwoken = pdFALSE;
	hal_irqreturn_t ret;

	tlsc_info("========enter tlsc6x_irq_handler=====\n");
	sem_ret = xSemaphoreGiveFromISR(data->irq_sem, &taskwoken);
	if (sem_ret == pdTRUE) {
		portYIELD_FROM_ISR(taskwoken);
		ret = HAL_IRQ_OK;
		return ret;
	} else {
		tlsc_err("irq give sem err\n");
		ret = HAL_IRQ_ERR;
		return ret;
	}
}

static void tlsc6x_tpd_reset(int reset_gpio)
{
	hal_gpio_set_data(reset_gpio, 1);
	vTaskDelay(30/portTICK_RATE_MS);
	hal_gpio_set_data(reset_gpio, 0);
	vTaskDelay(30/portTICK_RATE_MS);
	hal_gpio_set_data(reset_gpio, 1);
	vTaskDelay(30/portTICK_RATE_MS);
}

static int tlsc6x_hw_init(struct tlsc6x_drv_data *data)
{
	int ret;
	int reset_count = 0;
	struct tlsc6x_hw_cfg *config = data->config;

	ret = hal_gpio_pinmux_set_function(config->int_gpio, INT_GPIO_MUX);
	if (ret < 0) {
		tlsc_err("int gpio init err\n");
		return -1;
	}

	ret = hal_gpio_to_irq(config->int_gpio, &data->irq_num);
	if (ret < 0) {
		tlsc_err("get irq num err\n");
		return -1;
	}

	ret = hal_gpio_set_direction(config->reset_gpio, GPIO_DIRECTION_OUTPUT);
	if (ret < 0) {
		tlsc_err("reset gpio init err\n");
		return -1;
	}

	while(reset_count++ <= 3) {
		tlsc6x_tpd_reset(config->reset_gpio);
		g_is_telink_comp = tlsc6x_tp_dect(data);
		if (g_is_telink_comp)
			break;
	}

	if (g_is_telink_comp) {
		tlsc6x_tpd_reset(config->reset_gpio);
	} else {
		tlsc_err("tlsc6x_tp_dect err\n");
		return -1;
	}



	return 0;
}

static inline void get_tlsc6x_cfg(struct tlsc6x_drv_data *data)
{
	data->config = &tlsc6x_cfg;
}


int tlsc6x_init(void)
{
	int ret;
	struct sunxi_input_dev *input_dev;

	tlsc6x_data = malloc(sizeof(struct tlsc6x_drv_data));
	if (NULL == tlsc6x_data) {
		tlsc_err("malloc tlsc6x_data err\n");
		return -1;
	}

	get_tlsc6x_cfg(tlsc6x_data);

	hal_twi_init(tlsc6x_data->config->twi_id);
	hal_twi_control(tlsc6x_data->config->twi_id, I2C_SLAVE,
			&tlsc6x_data->config->addr);
	ret = tlsc6x_hw_init(tlsc6x_data);
	if (ret < 0) {
		tlsc_err("tlsc6x_hw_init err\n");
		goto err_free_data;
	}

	ret = hal_gpio_irq_request(tlsc6x_data->irq_num, tlsc6x_irq_handler,
			IRQ_TYPE_EDGE_FALLING, tlsc6x_data);
	if (ret < 0) {
		tlsc_err("irq request err\n");
		goto err_free_data;
	}
	ret = hal_gpio_irq_enable(tlsc6x_data->irq_num);
	if (ret < 0) {
		tlsc_err("irq request err\n");
		goto err_free_irq;
	}

	tlsc6x_data->mutex = xSemaphoreCreateMutex();
	if (NULL == tlsc6x_data->mutex) {
		tlsc_err("mutex init err\n");
		goto err_disable_irq;
	}

	tlsc6x_data->irq_sem = xSemaphoreCreateBinary();
	if (NULL == tlsc6x_data->irq_sem) {
		tlsc_err("irq_sem init err\n");
		goto err_free_mutex;
	}

	input_dev = sunxi_input_allocate_device();
	if (NULL == input_dev) {
		tlsc_err("input dev alloc err\n");
		goto err_free_sem;

	}
	input_dev->name = TLSC6X_DEV_NAME;
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
	tlsc6x_data->input_dev = input_dev;

	portBASE_TYPE task_ret;
	task_ret = xTaskCreate(touch_event_handler, (signed portCHAR *) "touch_event_handle_task", 1024, tlsc6x_data, 0, NULL);
	if (task_ret != pdPASS) {
		tlsc_err("input dev alloc err\n");
		goto err_free_sem;

	}

	tlsc_info("tlsc6x_data6x_init success\n");

	return 0;

err_free_sem:
	vSemaphoreDelete(tlsc6x_data->irq_sem);

err_free_mutex:
	vSemaphoreDelete(tlsc6x_data->mutex);

err_disable_irq:
	hal_gpio_irq_disable(tlsc6x_data->irq_num);

err_free_irq:
	hal_gpio_irq_free(tlsc6x_data->irq_num);

err_free_data:
	free(tlsc6x_data);
	tlsc6x_data = NULL;

	return -1;
}

int tlsc6x_deinit(void)
{
	if (NULL == tlsc6x_data) {
		tlsc_err("tlsc6x is not init\n");
		return -1;
	}

	vSemaphoreDelete(tlsc6x_data->irq_sem);

	vSemaphoreDelete(tlsc6x_data->mutex);

	hal_gpio_irq_disable(tlsc6x_data->irq_num);

	hal_gpio_irq_free(tlsc6x_data->irq_num);

	free(tlsc6x_data);

	tlsc6x_data = NULL;

	return 0;
}


#ifdef TLSC6X_TEST_TASK

void tp_test_task(void *parm)
{
	int fd;
	int x = -1, y = -1, z = -1;
	struct sunxi_input_event event;

	memset(&event, 0, sizeof(struct sunxi_input_event));

	fd = sunxi_input_open(TLSC6X_DEV_NAME);
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

static void cmd_tlsc6x_test(int argc, char **argv)
{
	int ret = -1;
	uint16_t addr = tlsc6x_cfg.addr;
/*
	hal_twi_init(tlsc6x_cfg.twi_id);
	hal_twi_control(tlsc6x_cfg.twi_id, I2C_SLAVE, &addr);*/
	ret = tlsc6x_init();

	if (ret < 0)
		printf("tlsc6x init fail\n");

	portBASE_TYPE task_ret;
	task_ret = xTaskCreate(tp_test_task, (signed portCHAR *) "tp_test_task", 1024, NULL, 0, NULL);
	if (task_ret != pdPASS) {
		printf("create tp read task err\n");
	}
}
FINSH_FUNCTION_EXPORT_CMD(cmd_tlsc6x_test, tp_test, tlsc6x-test);

#endif

