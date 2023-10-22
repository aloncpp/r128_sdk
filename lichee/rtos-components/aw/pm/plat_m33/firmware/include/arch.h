/*
 * Copyright (c) 2018 Allwinner.
 * 2018-09-14 Written by fanqinghua (fanqinghua@allwinnertech.com).
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef _FW_ARCH_H_
#define _FW_ARCH_H_


#include "type.h"
#include "irqs-sun20iw2p1.h"
#include "platform-sun20iw2p1.h"
#include "core_cm33.h"

void standby_delay(u32 us);
void standby_delay_cycle(u32 cycle);

void _fw_cpu_sleep(void);
void _fw_cpu_standby(void);

#endif /* __ARM32_H__ */

