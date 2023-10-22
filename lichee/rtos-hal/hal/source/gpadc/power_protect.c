#include <FreeRTOS.h>
#include <stdio.h>
#include <string.h>
#include <hal_interrupt.h>
#include "sunxi_hal_gpadc.h"
#include "sunxi_hal_power_protect.h"
#include "platform/gpadc_sun20iw2.h"

#define ADC_CHANNEL                     GP_CH_8

/* #define POWER_PROTECT_DEBUG */
#ifdef POWER_PROTECT_DEBUG
#define pp_info(fmt, args...)  printf("%s()%d - "fmt, __func__, __LINE__, ##args)
#else
#define pp_info(fmt, args...)
#endif

static int flash_ret = 0;

static int gpadc_power_protect_irq_callback(uint32_t data_type, uint32_t data)
{
	uint32_t vol_data;
	uint8_t ch_num;
	static int cnt = 0;
	hal_gpadc_channel_t channal;

#ifdef POWER_PROTECT_DEBUG
	data = ((VOL_RANGE / 4096) * data); /* 12bits sample rate */
	vol_data = data / 1000;
#endif

	if (data_type == GPADC_DOWN) {
		pp_info("channel %d vol data: %u\n", ADC_CHANNEL, vol_data);
		gpadc_channel_enable_highirq(ADC_CHANNEL);
		flash_ret = 1;
	} else if (data_type == GPADC_UP) {
		pp_info("channel %d vol data: %u\n", ADC_CHANNEL, vol_data);
		flash_ret = 0;
		cnt++;
		if (cnt > 2) {
			gpadc_channel_disable_highirq(ADC_CHANNEL);
			cnt = 0;
		}
	}
	return 0;
}

int get_flash_stat()
{
	return flash_ret;
}

int sunxi_power_protect_init(void)
{
	int i, ret = -1;
	hal_gpadc_init();
	hal_gpadc_channel_init(ADC_CHANNEL);
	ret = hal_gpadc_register_callback(ADC_CHANNEL, gpadc_power_protect_irq_callback);
	gpadc_channel_enable_lowirq(ADC_CHANNEL);

	return ret;
}
