/*
 *  drivers/standby/mdelay.c
 *
 * Copyright (c) 2018 Allwinner.
 * 2018-09-14 Written by fanqinghua (fanqinghua@allwinnertech.com).
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "type.h"
#include "arch.h"

void udelay(u32 us)
{
//	u64 cval = get_24Mclk();

//	while((get_24Mclk() - cval) < (us * 24) )
//		;
	standby_delay_cycle(us<<2);
	return;
}

void mdelay(u32 ms)
{
	while (ms > 10) {
		udelay(10 * 1000);
		ms -= 10;
	}

	udelay(ms * 1000);
	return;
}
