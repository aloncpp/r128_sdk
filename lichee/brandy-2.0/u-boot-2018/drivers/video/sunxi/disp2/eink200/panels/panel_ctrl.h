/*
 * Allwinner SoCs eink200 driver.
 *
 * Copyright (C) 2019 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __PANEL_H__
#define __PANEL_H__

#include "../include/eink_sys_source.h"
#include "../include/timing_ctrl.h"

void EINK_OPEN_FUNC(EINK_PANEL_FUNC func, u32 delay /*ms */);
void EINK_CLOSE_FUNC(EINK_PANEL_FUNC func, u32 delay /*ms */);

struct __eink_panel {
	char name[32];
	struct eink_panel_func func;
};

extern struct __eink_panel default_eink;
int eink_panel_init(void);
s32 panel_pin_cfg(u32 en);
s32 panel_gpio_set_value(u32 io_index, u32 value);
#endif
