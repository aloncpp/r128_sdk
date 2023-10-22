#include "FreeRTOS.h"
#include "task.h"
#include "delay.h"
#include <xtensa/xtbsp.h>
#include "xtensa_timer.h"
#include "spinlock.h"

static unsigned int div_of_us_cycle = 400000000 / 1000000;

void set_arch_timer_ticks_per_us(uint32_t dsp_clk_freq)
{
	div_of_us_cycle = dsp_clk_freq / 1000000;
	if (!div_of_us_cycle)
		div_of_us_cycle = 1;
}

void msleep(unsigned int ms)
{
	int tick = pdMS_TO_TICKS(ms) ?: 1;

	vTaskDelay(tick);

	return;
}

int usleep(unsigned int usec)
{
	const int us_per_tick = portTICK_PERIOD_MS * 1000;
	if (usec)
	{
		vTaskDelay((usec + us_per_tick - 1) / us_per_tick);
	}

	return 0;
}

void udelay(unsigned int us)
{
	unsigned expiry = xthal_get_ccount() + div_of_us_cycle * us;
	while( (long)(expiry - xthal_get_ccount()) > 0 );

	return;
}
