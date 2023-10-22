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

#define SID_PRCTL               (IOMEM_ADDR(SUNXI_SID_BASE) + 0x40)
#define SID_PRKEY               (IOMEM_ADDR(SUNXI_SID_BASE) + 0x50)
#define SID_RDKEY               (IOMEM_ADDR(SUNXI_SID_BASE) + 0x60)
#define SJTAG_AT0               (IOMEM_ADDR(SUNXI_SID_BASE) + 0x80)
#define SJTAG_AT1               (IOMEM_ADDR(SUNXI_SID_BASE) + 0x84)
#define SJTAG_S                 (IOMEM_ADDR(SUNXI_SID_BASE) + 0x88)
#define SID_EFUSE               (IOMEM_ADDR(SUNXI_SID_BASE) + 0x200)
#define SID_OP_LOCK  (0xAC)

#define EFUSE_CHIPID            (0x0)
#define EFUSE_ANTI_BRUSH		(0x10)
#define EFUSE_OEM_PROGRAM		(0x38)

#define ANTI_BRUSH_BIT_OFFSET			(31)
#define ANTI_BRUSH_MODE			(SID_EFUSE + EFUSE_ANTI_BRUSH)

/* write protect */
#define EFUSE_WRITE_PROTECT		(0x40)
/* read  protect */
#define EFUSE_READ_PROTECT		(0x44)
/* jtag security */

#define EFUSE_HUK					(0x50)
#define EFUSE_ROTPK					(0x70)
#define EFUSE_SSK					(0x90)
#define EFUSE_RSSK					(0xB0)
#define EFUSE_HDCP_HASH				(0xC0)
#define EFUSE_NV1					(0xD0)
#define EFUSE_NV2					(0xD4)
#define EFUSE_OEM_PROGRAM_SECURE	(0xE4)

#define SID_HUK_SIZE					(192)
#define SID_OEM_PROGRAM_SIZE			(64)
#define SID_SSK_SIZE					(256)
#define SID_RSSK_SIZE					(128)
#define SID_HDCP_HASH_SIZE				(128)
#define SID_OEM_PROGRAM_SECURE_SIZE		(224)

/*read protect*/
#define SCC_OEM_PROGRAM_DONTSHOW_FLAG			(4)
#define SCC_ROTPK_DONTSHOW_FLAG					(12)
#define SCC_SSK_DONTSHOW_FLAG					(13)
#define SCC_RSSK_DONTSHOW_FLAG					(14)
#define SCC_HDCP_HASH_DONTSHOW_FLAG				(15)
#define SCC_OEM_PROGRAM_SECURE_DONTSHOW_FLAG	(18)

/*write protect*/
#define SCC_OEM_PROGRAM_BURNED_FLAG				(4)
#define SCC_HUK_BURNED_FLAG						(9)
#define SCC_ROTPK_BURNED_FLAG					(12)
#define SCC_SSK_BURNED_FLAG						(13)
#define SCC_RSSK_BURNED_FLAG					(14)
#define SCC_HDCP_HASH_BURNED_FLAG				(15)
#define SCC_OEM_PROGRAM_SECURE_BURNED_FLAG		(18)

/*efuse power ctl*/
#define EFUSE_HV_SWITCH			(IOMEM_ADDR(SUNXI_RTC_BASE) + 0x204)
#endif    /*  #ifndef __SID_H__  */
