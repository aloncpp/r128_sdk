#ifndef __LEDS_SUNXI_H
#define __LEDS_SUNXI_H

#include <semphr.h>
#include <spinlock.h>
#include "sunxi_hal_ledc.h"

#define RESULT_COMPLETE 1
#define RESULT_ERR      2
typedef unsigned char u8;
/*struct sunxi_led {
	xSemaphoreHandle sem;
	u8 result;
	struct ledc_config config;
	freert_spinlock_t lock;
};*/

int sunxi_leds_init(void);
int sunxi_set_leds_brightness(int led_num, unsigned int brightness);

#endif
