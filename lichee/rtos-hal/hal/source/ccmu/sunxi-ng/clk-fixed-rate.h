/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 shengduiliang@allwinnertech.com
 */

#ifndef _CLK_FIXED_RATE_H
#define _CLK_FIXED_RATE_H

#define CLK_SRC_HOSC24M	0
#define CLK_SRC_HOSC24576M 1
#define CLK_SRC_HOSC26M 2
#define CLK_SRC_HOSC32M 3
#define CLK_SRC_HOSC40M 4
#define CLK_SRC_LOSC	5
#define CLK_SRC_RC_16M	6
#define CLK_SRC_EXT_32K	7
#define CLK_SRC_RC_HF	8
#ifdef CONFIG_ARCH_SUN20IW2
#define RCOSC_CLK	9
#define CLK_RCCAL_FAKE_PARENT	10
#endif

#ifdef CONFIG_ARCH_SUN20IW2
#define CLK_SRC_NUMBER (CLK_RCCAL_FAKE_PARENT + 1)
#else
#define CLK_SRC_NUMBER (CLK_SRC_RC_HF + 1)
#endif

#endif
