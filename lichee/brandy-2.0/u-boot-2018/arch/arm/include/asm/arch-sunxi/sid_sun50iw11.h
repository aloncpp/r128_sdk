/*
 * (C) Copyright 2013-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * wangwei <wangwei@allwinnertech.com>
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __SID_H__
#define __SID_H__

#include <linux/types.h>
#include <asm/arch/cpu.h>

#define SID_PRCTL               (SUNXI_SID_BASE + 0x40)
#define SID_PRKEY               (SUNXI_SID_BASE + 0x50)
#define SID_RDKEY               (SUNXI_SID_BASE + 0x60)
#define SJTAG_AT0               (SUNXI_SID_BASE + 0x80)
#define SJTAG_AT1               (SUNXI_SID_BASE + 0x84)
#define SJTAG_S                 (SUNXI_SID_BASE + 0x88)

#define SID_EFUSE               (SUNXI_SID_BASE + 0x200)
#define SID_SECURE_MODE         (SUNXI_SID_BASE + 0xA0)
#define SID_OP_LOCK  (0xAC)

#define EFUSE_CHIPID            (0x0)
#define EFUSE_ROTPK             (0x38)
/* write protect */
#define EFUSE_WRITE_PROTECT     (0x28) /* 0x28-0x2b, 32bits */
/* read  protect */
#define EFUSE_READ_PROTECT      (0x2c) /* 0x2c-0x2f, 32bits */
/* jtag security */
#define EFUSE_LCJS              (0x30)

/*efuse power ctl*/
#define EFUSE_HV_SWITCH			(SUNXI_RTC_BASE + 0x204)

#define SID_GET_SOC_VER
#define SID_SOC_VER_OFFSET	(22)
#define SID_SOC_VER_MASK	(0xF)

#define SUN50IW11P1_VERSION_C	(0xC)

#endif    /*  #ifndef __SID_H__  */
