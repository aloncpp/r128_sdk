#include <stdint.h>

#define  DWT_CR      *(volatile uint32_t *)0xE0001000
#define  DWT_CYCCNT  *(volatile uint32_t *)0xE0001004
#define  DEM_CR      *(volatile uint32_t *)0xE000EDFC

#define  DEM_CR_TRCENA                   (1 << 24)
#define  DWT_CR_CYCCNTENA                (1 <<  0)

int hal_dwt_tick_init(void)
{
	DEM_CR |= (uint32_t)DEM_CR_TRCENA;
	DWT_CYCCNT = (uint32_t)0u;
	DWT_CR |= (uint32_t)DWT_CR_CYCCNTENA;

	return 0;
}

uint32_t hal_dwt_tick_read(void)
{
	return ((uint32_t)DWT_CYCCNT);
}

static uint32_t get_cpu_freq(void)
{
	/* CPU Clock Freq */
	return 192000000;
}

void hal_dwt_delay(uint32_t us)
{
	uint32_t ticks;
	uint32_t told,tnow,tcnt=0;
	uint32_t cpu_freq = get_cpu_freq();

	hal_dwt_tick_init();

	ticks = us * (cpu_freq / 1000000);
	tcnt = 0;
	told = (uint32_t)hal_dwt_tick_read();

	while(1)
	{
		tnow = (uint32_t)hal_dwt_tick_read();
		if(tnow != told)
		{
			if(tnow > told)
				tcnt += tnow - told;
			else
				tcnt += UINT32_MAX - told + tnow;
			told = tnow;
			if(tcnt >= ticks)
				break;
		}
	}
}

