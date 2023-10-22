#include "hw_perf.h"

int32_t hw_perf_parse_val(char *valstring) {
	int32_t retval=0;
	int32_t neg=1;
	int hexmode=0;

	if (*valstring == '-') {
		neg=-1;
		valstring++;
	}
	if ((valstring[0] == '0') && (valstring[1] == 'x')) {
		hexmode=1;
		valstring+=2;
	}
	/* first look for digits */
	if (hexmode) {
		while (((*valstring >= '0') && (*valstring <= '9')) || ((*valstring >= 'a') && (*valstring <= 'f'))) {
			int32_t digit=*valstring-'0';
			if (digit>9)
				digit=10+*valstring-'a';
			retval*=16;
			retval+=digit;
			valstring++;
		}
	} else {
		while ((*valstring >= '0') && (*valstring <= '9')) {
			int32_t digit=*valstring-'0';
			retval*=10;
			retval+=digit;
			valstring++;
		}
	}
	/* now add qualifiers */
	if ((*valstring=='K') || (*valstring=='k'))
		retval*=1024;
	if ((*valstring=='M') || (*valstring=='m'))
		retval*=1024*1024;

	retval*=neg;
	return retval;
}


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

	_t = (_t_tag - t + 23) / 24;
	_t_tag = t;
	return _t;
}

void time_perf_result(uint32_t bench_size, float time_use)
{
	float throuth_mb;

	if (time_use < 0.001) {
		printf("time_use too small\n");
		time_use = 0.001;
	}
	throuth_mb = bench_size * 1000 / 1024 / time_use / 1000;
	printf("%3d KB use:%3.3f ms, throughput:%2.3f MB/S\n",
			bench_size/1024, time_use, throuth_mb);
}
#endif
