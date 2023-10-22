/*
 * Copyright 2019 AllWinnertech  Co., Ltd
 * frank@allwinnertech.com
 */

#ifndef __CPUFREQ_H__
#define __CPUFREQ_H__

#include <drivers/cpufreq.h>

#ifdef __cplusplus
extern "C" {
#endif

int cpufreq_info_get(unsigned int, struct cpufreq_info **);

#ifdef CONFIG_ARCH_SUN8IW18P1
#define PLATFORM_INFO sun8iw18p1_info
#include "sun8iw18p1.h"
#endif /* CONFIG_ARCH_SUN8IW18P1 */

#ifdef __cplusplus
}
#endif

#endif /* __CPUFREQ_H__ */
