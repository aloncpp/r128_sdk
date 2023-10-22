/*
 * (C) Copyright 2018
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangwei <wangwei@allwinnertech.com>
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef _SUNXI_CE_H
#define _SUNXI_CE_H

#include <linux/types.h>
#include <config.h>
#include <asm/arch/cpu.h>

#if defined(CONFIG_SUNXI_CE_20)
#include "ce_2.0.h"
#elif defined(CONFIG_SUNXI_CE_10)
#include "ce_1.0.h"
#elif defined(CONFIG_SUNXI_CE_21)
#include "ce_2.1.h"
#elif defined(CONFIG_SUNXI_CE_23)
#include "ce_2.3.h"
#else
#error "Unsupported plat"
#endif

#ifndef __ASSEMBLY__
void sunxi_ss_open(void);
void sunxi_ss_close(void);
int  sunxi_sha_calc(u8 *dst_addr, u32 dst_len,
					u8 *src_addr, u32 src_len);
int sunxi_md5_calc(u8 *dst_addr, u32 dst_len, u8 *src_addr, u32 src_len);

s32 sunxi_rsa_calc(u8 *n_addr,   u32 n_len,
				   u8 *e_addr,   u32 e_len,
				   u8 *dst_addr, u32 dst_len,
				   u8 *src_addr, u32 src_len);
#endif

#define SM2_SIZE_BYTE	(256)
struct sunxi_sm2_ctx_t {
	u32 mode;
	u32 sm2_size;
	u32 k_len;
	u8 *k;
	u8 *n;
	u8 *p;
	u8 *a;
	u8 *b;
	u8 *gx;
	u8 *gy;
	u8 *px;
	u8 *py;
	u8 *d;
	u8 *r;
	u8 *s;
	u8 *m;
	u32 m_len;
	u8 *cx;
	u8 *cy;
	u8 *kx;
	u8 *ky;
	u8 *out;
};

#endif /* _SUNXI_CE_H */
