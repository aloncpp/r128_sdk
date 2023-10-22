/*
 * Copyright 2019 AllWinnertech  Co., Ltd
 * frank@allwinnertech.com
 */

#include "cpufreq.h"
#include <hal_clk.h>

struct cpufreq_frequency_table sun8iw18p1_table[] = {
	{
		.frequency = 720000000, /* Hz */
		.target_uV = 820000,
	},
	{
		.frequency = 1008000000, /* Hz */
		.target_uV = 900000,
	},
	{
		.frequency = 1200000000, /* Hz */
		.target_uV = 1000000,
	},
	{
		/* sentinel */
	},
};

struct cpufreq_info sun8iw18p1_info[] = {
	{
		.cpu = 0x3,
		.clk = HAL_CLK_PLL_CPUX_C0,
		.freq_table = sun8iw18p1_table,
	},
	{
		/* sentinel */
	}
};
