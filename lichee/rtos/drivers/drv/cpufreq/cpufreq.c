/*
 * Copyright 2019 AllWinnertech  Co., Ltd
 * frank@allwinnertech.com
 */

#include "cpufreq.h"
#include <stdint.h>
#include <stdio.h>

int cpufreq_info_get(unsigned int cpu, struct cpufreq_info **info)
{
	struct cpufreq_info *cur = PLATFORM_INFO;

	while (cur->cpu) {
		unsigned int mask = cur->cpu;

		if ((cpu & mask) && !(cpu & ~mask)) {
			*info = cur;
			return 0;
		}

		cur++;
	}

	return -1;
}
