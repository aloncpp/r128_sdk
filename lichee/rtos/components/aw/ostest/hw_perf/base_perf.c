#include "hw_perf.h"

#ifdef CONFIG_DRIVERS_TIMER
uint32_t time_perf_tag(void); /* based on uS */

static void hal_timer_irq_callback(void *param)
{
	hal_log_info("timer interrupt!!\n");
}

void time_perf_init(void)
{
	hal_timer_init(SUNXI_TMR0);
	hal_timer_set_oneshot(SUNXI_TMR0, 500000000, hal_timer_irq_callback, NULL);
	hal_log_info("timer only support 24MHz now!!\n");
	vTaskDelay(1);
}

void time_perf_deinit(void)
{
	hal_timer_stop(SUNXI_TMR0);
	hal_timer_uninit(SUNXI_TMR0);
}

uint32_t time_perf_tag(void) /* based on uS */
{
	static uint32_t _t_tag;
	uint32_t _t;
	uint32_t t = _perf_readl((unsigned int)_PERF_TIMER_CNTVAL_REG(SUNXI_TMR0));

#ifdef CONFIG_ARCH_SUN20IW2P1
	_t = (_t_tag - t + 23) / 40;
#else
	_t = (_t_tag - t + 23) / 24;
#endif
	_t_tag = t;
	return _t;
}

#endif
