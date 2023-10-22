#include <FreeRTOS.h>
#include <FreeRTOSHal.h>
#include <semphr.h>
#include <queue.h>
#include <stdio.h>
#include "interrupt.h"
#include "sunxi_hal_ledc.h"
#include "./leds-sunxi.h"
#include <spinlock.h>

//#define CONFIG_ARCH_SUN8IW18P1
//#include "irqs.h"
#ifndef SUNXI_IRQ_BUS_LEDC
#define SUNXI_IRQ_BUS_LEDC 92
#endif

#ifdef LED_DEBUG
#define led_info(fmt, args...)  printf("%s()%d - "fmt, __func__, __LINE__, ##args)
#else
#define led_info(fmt, args...)
#endif

#define led_err(fmt, args...)  printf("%s()%d - "fmt, __func__, __LINE__, ##args)

static struct ledc_config ledc_config = {
	.led_count = 3,
	.reset_ns = 84,
	.t1h_ns = 800,
	.t1l_ns = 450,
	.t0h_ns = 400,
	.t0l_ns = 850,
	.wait_time0_ns = 84,
	.wait_time1_ns = 84,
	.wait_data_time_ns = 600000,
	.output_mode = "GRB",
};

static struct sunxi_led *led;
freert_spinlock_t lock;

hal_irqreturn_t sunxi_ledc_irq_handler(void *dummy)
{
	led_info("=======enter irq_handler=====\n");
	struct sunxi_led *led = (struct sunxi_led *)dummy;
	unsigned int irq_status;
	BaseType_t ret, taskwoken = pdFALSE;

	irq_status = hal_ledc_get_irq_status();
	hal_ledc_clear_all_irq();

	if (irq_status & LEDC_TRANS_FINISH_INT)
		led->result = RESULT_COMPLETE;

	if (irq_status & LEDC_WAITDATA_TIMEOUT_INT)
		led->result = RESULT_ERR;

	if (irq_status & LEDC_FIFO_OVERFLOW_INT)
		led->result = RESULT_ERR;

	led->config.length = 0;
	hal_ledc_reset();

	return HAL_IRQ_OK;
}


int sunxi_leds_get_config(struct ledc_config *config)
{
	*config = ledc_config;
	return 0;
}

void sunxi_leds_deinit(void)
{
	hal_free_irq(SUNXI_IRQ_BUS_LEDC);
	hal_ledc_deinit();
	vPortFree(led->config.data);
	vPortFree(led);
}


int sunxi_leds_init(void)
{
	int ret = -1;
	unsigned long int size;

	led_info("sunxi_leds_init\n");

	led = pvPortMalloc(sizeof(struct sunxi_led));
	if (NULL == led) {
		led_err("sunxi led malloc err\n");
		return -1;
	}

	sunxi_leds_get_config(&led->config);

	led->config.data = pvPortMalloc(sizeof(unsigned int) * led->config.led_count);
	if (NULL == led->config.data) {
		led_err("sunxi led config data malloc err\n");
		goto err1;
	}

	hal_ledc_init();

	hal_request_irq(SUNXI_IRQ_BUS_LEDC, sunxi_ledc_irq_handler, "ledc", led);

	hal_enable_irq(SUNXI_IRQ_BUS_LEDC);
	led_info("sunxi_leds_init success\n");

	return 0;

err2:
	vPortFree(led->config.data);
err1:
	vPortFree(led);

	return -1;

}

int sunxi_set_all_leds(unsigned int brightness)
{
	int i;
	freert_spin_lock(&lock);
	led->config.length = led->config.led_count;
	for(i = 0;i < led->config.led_count;i++)
		led->config.data[i] = brightness;

	hal_ledc_trans_data(&led->config);
	freert_spin_unlock(&lock);
}


int sunxi_set_leds_brightness(int led_num, unsigned int brightness)
{
	uint32_t flags = 0;
	if (NULL == led) {
		led_err("err : ledc is not init\n");
		return -1;
	}

	if (led_num > led->config.led_count) {
		led_err("has not the %d led\n", led_num);
		return -1;
	}

	freert_spin_lock(&lock);
	led->config.length = led_num;
	led->config.data[led_num-1] = brightness;
	hal_ledc_trans_data(&led->config);
	freert_spin_unlock(&lock);

	return 0;
}
