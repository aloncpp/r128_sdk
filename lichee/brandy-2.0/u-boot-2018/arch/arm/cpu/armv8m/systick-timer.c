// SPDX-License-Identifier: GPL-2.0+
/*
 * ARM Cortex M3/M4/M7 SysTick timer driver
 * (C) Copyright 2017 Renesas Electronics Europe Ltd
 *
 * Based on arch/arm/mach-stm32/stm32f1/timer.c
 * (C) Copyright 2015
 * Kamil Lulko, <kamil.lulko@gmail.com>
 *
 * Copyright 2015 ATS Advanced Telematics Systems GmbH
 * Copyright 2015 Konsulko Group, Matt Porter <mporter@konsulko.com>
 *
 * The SysTick timer is a 24-bit count down timer. The clock can be either the
 * CPU clock or a reference clock. Since the timer will wrap around very quickly
 * when using the CPU clock, and we do not handle the timer interrupts, it is
 * expected that this driver is only ever used with a slow reference clock.
 *
 * The number of reference clock ticks that correspond to 10ms is normally
 * defined in the SysTick Calibration register's TENMS field. However, on some
 * devices this is wrong, so this driver allows the clock rate to be defined
 * using CONFIG_SYS_HZ_CLOCK.
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/timer.h>
#include <div64.h>

DECLARE_GLOBAL_DATA_PTR;

/* SysTick Base Address - fixed for all Cortex M3, M4 and M7 devices */
#define SYSTICK_BASE		0xE000E010

struct cm3_systick {
	uint32_t ctrl;
	uint32_t reload_val;
	uint32_t current_val;
	uint32_t calibration;
};

#define TIMER_MAX_VAL		0x00FFFFFF
#define SYSTICK_CTRL_EN		BIT(0)
/* Clock source: 0 = Ref clock, 1 = CPU clock */
#define SYSTICK_CTRL_CPU_CLK	BIT(2)
#define SYSTICK_CAL_NOREF	BIT(31)
#define SYSTICK_CAL_SKEW	BIT(30)
#define SYSTICK_CAL_TENMS_MASK	0x00FFFFFF

#ifndef CONFIG_SYS_HZ_CLOCK
#define CONFIG_SYS_HZ_CLOCK            24000000        /* Timer is clocked at 24MHz */
#endif

#ifdef CONFIG_MACH_SUN20IW2

#define CCMU_AON_BASE 0x4004C400
#define CCMU_AON_HOSC_TYPE_OFFSET 0x84

#define HOSC_TYPE_26M (0)
#define HOSC_TYPE_40M (1)
#define HOSC_TYPE_24M (2)
#define HOSC_TYPE_32M (3)
#define HOSC_TYPE_24_576M (4)

#define CCMU_BASE 0x4003C000
#define CCMU_SYSTICK_REFCLK_CTRL_OFFSET 0x40

#define REFCLK_CTRL_MCLK_ENABLE_SHIFT 31
#define REFCLK_CTRL_MCLK_ENABLE_MASK (0x1 << REFCLK_CTRL_MCLK_ENABLE_SHIFT)

#define REFCLK_CTRL_MCLK_SRC_SEL_SHIFT 24
#define REFCLK_CTRL_MCLK_SRC_SEL_MASK (0x3 << REFCLK_CTRL_MCLK_SRC_SEL_SHIFT)

#define REFCLK_CTRL_MCLK_DIV_RATIO_N_SHIFT 16
#define REFCLK_CTRL_MCLK_DIV_RATIO_N_MASK (0x3 << REFCLK_CTRL_MCLK_DIV_RATIO_N_SHIFT)

#define REFCLK_CTRL_MCLK_DIV_RATIO_M_SHIFT 0
#define REFCLK_CTRL_MCLK_DIV_RATIO_M_MASK (0xF << REFCLK_CTRL_MCLK_DIV_RATIO_M_SHIFT)
//#define CCMU_SYSTICK_CALIB_CTRL_OFFSET 0x44

#define DEFAULT_TIMER_CLK_FREQ 4000000
#define TIMER_CLK_FREQ_WHEN_HOSC_26M 3250000
#define TIMER_CLK_FREQ_WHEN_HOSC_24_576M 3072000

static __attribute__((unused)) int init_systick_timer_clk(void)
{
	uint32_t reg_data, reg_addr, timer_clk_freq = DEFAULT_TIMER_CLK_FREQ, div_m = 16;
	reg_addr = CCMU_AON_BASE + CCMU_AON_HOSC_TYPE_OFFSET;
	reg_data = readl(reg_addr);
	reg_data = (reg_data & 0x7);

	switch (reg_data) {
	case HOSC_TYPE_26M:
		div_m = 8;
		timer_clk_freq = TIMER_CLK_FREQ_WHEN_HOSC_26M;
		break;
	case HOSC_TYPE_40M:
		div_m = 10;
		break;
	case HOSC_TYPE_24M:
		div_m = 6;
		break;
	case HOSC_TYPE_32M:
		div_m = 8;
		break;
	case HOSC_TYPE_24_576M:
		div_m = 8;
		timer_clk_freq = TIMER_CLK_FREQ_WHEN_HOSC_24_576M;
		break;
	default:
		div_m = 16;
		break;
	}

	reg_addr = CCMU_BASE + CCMU_SYSTICK_REFCLK_CTRL_OFFSET;
	reg_data = readl(reg_addr);

	reg_data &= ~(REFCLK_CTRL_MCLK_SRC_SEL_MASK |
				REFCLK_CTRL_MCLK_DIV_RATIO_N_MASK |
				REFCLK_CTRL_MCLK_DIV_RATIO_M_MASK);

	div_m -= 1;
	reg_data |= (1 << REFCLK_CTRL_MCLK_ENABLE_SHIFT) |
				((div_m & REFCLK_CTRL_MCLK_DIV_RATIO_M_MASK) << REFCLK_CTRL_MCLK_DIV_RATIO_M_SHIFT);
	writel(reg_data, reg_addr);
	return timer_clk_freq;
}

static int get_systick_timer_clk(void)
{
	uint32_t reg_data, reg_addr, timer_clk_freq = DEFAULT_TIMER_CLK_FREQ;
	reg_addr = CCMU_AON_BASE + CCMU_AON_HOSC_TYPE_OFFSET;
	reg_data = readl(reg_addr);
	reg_data = (reg_data & 0x7);

	switch (reg_data) {
	case HOSC_TYPE_26M:
		timer_clk_freq = TIMER_CLK_FREQ_WHEN_HOSC_26M;
		break;
	case HOSC_TYPE_24_576M:
		timer_clk_freq = TIMER_CLK_FREQ_WHEN_HOSC_24_576M;
		break;
	default:
		break;
	}

	return timer_clk_freq;
}
#endif

/* read the 24-bit timer */
ulong read_timer(void)
{
	struct cm3_systick *systick = (struct cm3_systick *)SYSTICK_BASE;

#ifdef CONFIG_MACH_SUN20IW2
	return gd->arch.timer_reset_value - readl(&systick->current_val);
#else
	/* The timer counts down, therefore convert to an incrementing timer */
	return TIMER_MAX_VAL - readl(&systick->current_val);
#endif
}

int timer_init(void)
{
	struct cm3_systick *systick = (struct cm3_systick *)SYSTICK_BASE;

#ifdef CONFIG_MACH_SUN20IW2
	/* the systick timer has been configured by fes1 or boot0 */
	uint32_t timer_clk_freq = get_systick_timer_clk();
	gd->arch.timer_rate_hz = timer_clk_freq;

	/* use timer_reset_value variable to store the timer reload value */
	gd->arch.timer_reset_value = readl(&systick->reload_val);

	gd->arch.tbl = read_timer();
	gd->arch.tbu = 0;
	gd->arch.lastinc = gd->arch.tbl;
#else
	u32 cal;

	writel(TIMER_MAX_VAL, &systick->reload_val);
	/* Any write to current_val reg clears it to 0 */
	writel(0, &systick->current_val);

	cal = readl(&systick->calibration);
	if (cal & SYSTICK_CAL_NOREF)
		/* Use CPU clock, no interrupts */
		writel(SYSTICK_CTRL_EN | SYSTICK_CTRL_CPU_CLK, &systick->ctrl);
	else
		/* Use external clock, no interrupts */
		writel(SYSTICK_CTRL_EN, &systick->ctrl);

	/*fpga: use cpu clk, fix me*/
	//writel(SYSTICK_CTRL_EN | SYSTICK_CTRL_CPU_CLK, &systick->ctrl);

	/*
	 * If the TENMS field is inexact or wrong, specify the clock rate using
	 * CONFIG_SYS_HZ_CLOCK.
	 */
#if defined(CONFIG_SYS_HZ_CLOCK)
	gd->arch.timer_rate_hz = CONFIG_SYS_HZ_CLOCK;
#else
	gd->arch.timer_rate_hz = (cal & SYSTICK_CAL_TENMS_MASK) * 100;
#endif

	gd->arch.tbl = 0;
	gd->arch.tbu = 0;
	gd->arch.lastinc = read_timer();
#endif

	return 0;
}

unsigned long long get_ticks(void)
{
	u32 now = read_timer();

	if (now >= gd->arch.lastinc)
		gd->arch.tbl += (now - gd->arch.lastinc);
	else
#ifdef CONFIG_MACH_SUN20IW2
		gd->arch.tbl += (gd->arch.timer_reset_value - gd->arch.lastinc) + now;
#else
		gd->arch.tbl += (TIMER_MAX_VAL - gd->arch.lastinc) + now;
#endif

	gd->arch.lastinc = now;

	return gd->arch.tbl;
}

ulong get_tbclk(void)
{
	return gd->arch.timer_rate_hz;
}

/* Return timestamp in milliseconds */
unsigned long get_timer_masked(void)
{
	return get_ticks() * CONFIG_SYS_HZ / gd->arch.timer_rate_hz;
}

/* Return time interval in milliseconds after base */
ulong get_timer(ulong base)
{
	return get_timer_masked() - base;
}

void __usdelay(unsigned long us)
{
	__udelay(us);
}

void __msdelay(unsigned long ms)
{
	while (ms--)
		__udelay(1000);
}

/* get the current timestamp(ms) */
int runtime_tick(void)
{
	return get_timer_masked();
}
