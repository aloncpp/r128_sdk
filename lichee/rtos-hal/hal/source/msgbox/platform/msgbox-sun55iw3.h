/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef __MSGBOX_SUN55IW3_H
#define __MSGBOX_SUN55IW3_H

#define MSGBOX_RV    3
#define MSGBOX_RV_BASE 0x07136000

#define MSGBOX_CPUS  2
#define MSGBOX_CPUS_BASE 0x07094000

#define MSGBOX_DSP   1
#define MSGBOX_DSP_BASE 0x07120000

#define MSGBOX_ARM   0
#define MSGBOX_ARM_BASE 0x03003000

static inline void *msgbox_base_get(int *m)
{
	void *b = 0;

	switch (*m) {
	case MSGBOX_ARM:
		b = (void *)MSGBOX_ARM_BASE;
		break;
	case MSGBOX_DSP:
		b = (void *)MSGBOX_DSP_BASE;
		break;
	case MSGBOX_CPUS:
		b = (void *)MSGBOX_CPUS_BASE;
		break;
	case MSGBOX_RV:
		b = (void *)MSGBOX_RV_BASE;
		break;
	default:
		break;
	}

	return b;
}

static inline void *MSGBOX_VER_REG(int m, int n)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x10 + (n)*0x100);
}

static inline void *MSGBOX_RD_IRQ_EN_REG(int m, int n)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x20 + (n)*0x100);
}

static inline void *MSGBOX_RD_IRQ_STA_REG(int m, int n)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x24 + (n)*0x100);
}

static inline void *MSGBOX_WR_IRQ_EN_REG(int m, int n)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x30 + (n)*0x100);
}

static inline void *MSGBOX_WR_IRQ_STA_REG(int m, int n)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x34 + (n)*0x100);
}

static inline void *MSGBOX_DEBUG_REG(int m, int n)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x40 + (n)*0x100);
}

static inline void *MSGBOX_FIFO_STA_REG(int m, int n, int p)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x50 + (n)*0x100 + (p)*0x4);
}

static inline void *MSGBOX_MSG_STA_REG(int m, int n, int p)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x60 + (n)*0x100 + (p)*0x4);
}

static inline void *MSGBOX_MSG_REG(int m, int n, int p)
{
	void *msgbox_base = msgbox_base_get(&m);

	return (msgbox_base + 0x70 + (n)*0x100 + (p)*0x4);
}

#define RST_MSGBOX_TYPE	HAL_SUNXI_DSP_RESET
#define CLK_MSGBOX_TYPE	HAL_SUNXI_DSP

#if defined(CONFIG_ARCH_DSP)

#define THIS_MSGBOX_USE MSGBOX_DSP
#define IRQ_MSGBOX 3
#define RST_MSGBOX RST_BUS_DSP_MSG
#define CLK_MSGBOX CLK_BUS_DSP_MSG

#endif

#if defined(CONFIG_ARCH_RISCV)

#define THIS_MSGBOX_USE MSGBOX_RV
#define IRQ_MSGBOX MAKE_IRQn(17, 0)
#define RST_MSGBOX RST_BUS_DSP_RV_MSG
#define CLK_MSGBOX CLK_BUS_DSP_RISCV_MSG

#endif
#endif /* __MSGBOX_SUN55IW3_H */
