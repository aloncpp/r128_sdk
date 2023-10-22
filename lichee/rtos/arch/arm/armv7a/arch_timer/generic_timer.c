/*
 * Allwinnertech rtos arch timer file.
 * Copyright (C) 2019  Allwinnertech Co., Ltd. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"

#include "generic_timer.h"
#include "interrupt.h"
#include "serial.h"
#include <stdio.h>

static unsigned int timer_reload_val;

static unsigned int read_cntfrq(void)
{
    unsigned int cntfrq;
    asm volatile ("mrc  p15, 0, %0, c14, c0, 0" : "=r"(cntfrq));
    return cntfrq;
}


static void write_cntp_tval(unsigned int cntp_tval)
{
    asm volatile  ("mcr p15, 0, %0, c14, c2, 0" :: "r"(cntp_tval));
}

static void write_cntp_ctl(unsigned int cntp_ctl)
{
    asm volatile ("mcr p15, 0, %0, c14, c2, 1" ::"r"(cntp_ctl));
    asm volatile ("isb");
}

static void platformSetOneshotTimer(int interval)
{
	write_cntp_tval(interval);
	write_cntp_ctl(1); /* enable timer */
}

hal_irqreturn_t platformTick(void *para)
{
	//SMP_DBG("no = %d.\n", no);
	write_cntp_ctl(0); /* disable timer */

	FreeRTOS_Tick_Handler();

	platformSetOneshotTimer(timer_reload_val);

	return HAL_IRQ_OK;
}

void armGenericTimerInit(int irq, unsigned int freq_override)
{
    	volatile unsigned char sdbbp = 1;
	unsigned int cntfrq;
	int ret;

	//ns:0 secuirty mode. ns:1 normal mode
	int irqnum, ns = 0;

	if (freq_override == 0)
	{
		cntfrq = read_cntfrq(); /* default: 24MHz */
		if (!cntfrq)
		{
			SMP_DBG("Timer frequency is zero.\n");
			return;
		}
	}
	else
	{
		cntfrq = freq_override;
	}

	//normal world, irqnum is 30
	if(ns == 1)
	{
		irqnum = 30;
	}
	else
	{
		irqnum = 29;
	}

	timer_reload_val = cntfrq / configTICK_RATE_HZ; /* 10ms */
	platformSetOneshotTimer(timer_reload_val);

	hal_request_irq(irqnum, platformTick, "arch_timer", NULL);
	ret = hal_enable_irq(irqnum);
	if (ret)
	{
		SMP_DBG("arch-timer irq enable failed.\n");
	}
	/*while(sdbbp);*/
}
