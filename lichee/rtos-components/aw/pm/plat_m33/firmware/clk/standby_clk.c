/* Copyright (c) 2022-2028 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 *the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
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

#include <stdio.h>
#include <stdlib.h>

#include "type.h"
#include "delay.h"
#include "head.h"
#include "resource.h"

#include "standby_clk.h"
#include "standby_power.h"

typedef enum {
	clk_disable = 0,
	clk_enable = 1,
} clk_state_t;

/* pll status */
static uint32_t volatile dpll1_ctrl_reg_bak;
static uint32_t volatile dpll1_out_cfg_reg_bak;
static uint32_t volatile dpll2_ctrl_reg_bak;
static uint32_t volatile dpll3_ctrl_reg_bak;
static uint32_t volatile dpll3_out_cfg_reg_bak;
static uint32_t volatile audpll_ctrl_reg_bak;
/* bus clk */
static uint32_t volatile sys_clk_ctrl_bak;
static uint32_t volatile sys_lfclk_ctrl_bak;

static inline void ccmu_aon_pll_save(void)
{
	dpll1_ctrl_reg_bak = readl(DPLL1_CTRL_REG);
	dpll1_out_cfg_reg_bak = readl(DPLL1_OUT_CFG_REG);

	dpll2_ctrl_reg_bak = readl(DPLL2_CTRL_REG);

	dpll3_ctrl_reg_bak = readl(DPLL3_CTRL_REG);
	dpll3_out_cfg_reg_bak = readl(DPLL3_OUT_CFG_REG);

	audpll_ctrl_reg_bak = readl(AUDIO_PLL_CTRL_REG);
}

static inline void bus_clk_save(void)
{
	sys_clk_ctrl_bak = readl(SYS_CLK_CTRL_REG);
	sys_lfclk_ctrl_bak = readl(SYS_LFCLK_CTRL_REG);
}

static void ccmu_aon_dpll1_switch(clk_state_t sta)
{
	if (sta)
		writel(readl(DPLL1_CTRL_REG) | (dpll1_ctrl_reg_bak & DPLL_NDIV_MASK) \
			| (dpll1_ctrl_reg_bak & DPLL_FACTORM_MASK), DPLL1_CTRL_REG);
	writel((readl(DPLL1_CTRL_REG) & (~DPLL_EN_MASK)) \
		| (sta << 31), DPLL1_CTRL_REG);

	if (sta) {
		while(!(readl(DPLL1_OUT_CFG_REG) & DPLL_OUT_CFG_LOCK_MASK));
		udelay(20);
		writel(dpll1_out_cfg_reg_bak, DPLL1_OUT_CFG_REG);
	}
}

static  void ccmu_aon_dpll2_switch(clk_state_t sta)
{
	if (sta)
		writel(readl(DPLL2_CTRL_REG) | (dpll2_ctrl_reg_bak & DPLL_NDIV_MASK) \
			| (dpll2_ctrl_reg_bak & DPLL_FACTORM_MASK), DPLL2_CTRL_REG);
	writel((readl(DPLL2_CTRL_REG) & (~DPLL_EN_MASK)) \
		| (sta << 31), DPLL2_CTRL_REG);
}

static  void ccmu_aon_dpll3_switch(clk_state_t sta)
{
	if (sta) {
		writel(readl(DPLL3_CTRL_REG) | (dpll3_ctrl_reg_bak & DPLL_NDIV_MASK) \
			| (dpll3_ctrl_reg_bak & DPLL_FACTORM_MASK), DPLL3_CTRL_REG);
		if (!(readl(CLK_LDO_CTRL_REG) & CLK_LDO1_EN_MASK) || !(readl(CLK_LDO_CTRL_REG) & CLK_LDO2_EN_MASK))
			writel(readl(CLK_LDO_CTRL_REG) | CLK_LDO1_EN_MASK \
				| CLK_LDO2_EN_MASK, CLK_LDO_CTRL_REG);
	}
	writel((readl(DPLL3_CTRL_REG) & (~DPLL_EN_MASK)) \
		| (sta << 31), DPLL3_CTRL_REG);
	if (sta) {
		while(!(readl(DPLL3_OUT_CFG_REG) & DPLL_OUT_CFG_LOCK_MASK));
		udelay(20);
		writel(dpll3_out_cfg_reg_bak, DPLL3_OUT_CFG_REG);
	}
}

static void  ccmu_aon_audiopll_switch(clk_state_t sta)
{
	if (sta) {
		writel(readl(AUDIO_PLL_CTRL_REG) | (audpll_ctrl_reg_bak & AUDPLL_FACTORN_MASK) \
			| (audpll_ctrl_reg_bak & AUDPLL_PRETDIVM_MASK), AUDIO_PLL_CTRL_REG);
		if (!(readl(CLK_LDO_CTRL_REG) & CLK_LDO1_EN_MASK) || !(readl(CLK_LDO_CTRL_REG) & CLK_LDO2_EN_MASK))
			writel(readl(CLK_LDO_CTRL_REG) | CLK_LDO1_EN_MASK \
				| CLK_LDO2_EN_MASK, CLK_LDO_CTRL_REG);
	}
	writel((readl(AUDIO_PLL_CTRL_REG) & (~AUDPLL_EN_MASK)) \
		| (sta << 31), AUDIO_PLL_CTRL_REG);
	if (sta) {
		writel(readl(AUDIO_PLL_CTRL_REG) | AUDPLL_LOCK_EN_MASK, AUDIO_PLL_CTRL_REG);
		while(!(readl(AUDIO_PLL_CTRL_REG) & AUDPLL_LOCK_MASK));
		udelay(150);
	} else
		writel(readl(AUDIO_PLL_CTRL_REG) & (~AUDPLL_LOCK_EN_MASK), AUDIO_PLL_CTRL_REG);
}

static void  ccmu_aon_dcxo_switch(clk_state_t sta)
{
	writel(readl(DCXO_CTRL_REG) & (~BUFFER_DPLL_EN), DCXO_CTRL_REG);
	writel((readl(DCXO_CTRL_REG) & (~DCXO_EN_MASK)) \
		| (sta << 31), DCXO_CTRL_REG);

	if (sta) {
		/* wait until the DCXO stabilizes and than start the reference clock for PLL */
		udelay(2000);
		writel(readl(DCXO_CTRL_REG) | BUFFER_DPLL_EN, DCXO_CTRL_REG);
	}
}

static void gprcm_rc_hf_swtich(uint32_t rc_hf_en)
{
	writel((readl(SYS_LFCLK_CTRL_REG) & ~RC_HF_EN_MASK) \
		| rc_hf_en, SYS_LFCLK_CTRL_REG);
	udelay(1000);
}

/* change APB clk src to SYS_32K */
static inline void APBtoSRC(uint32_t src_sel)
{
	/* change src */
	writel((readl(SYS_CLK_CTRL_REG) & ~APB_CLK_SRC_SEL_MASK) \
		| src_sel, SYS_CLK_CTRL_REG);

	/* div change to 1 */
	writel((readl(SYS_CLK_CTRL_REG) & ~APB_CLK_DIV_MASK) \
		| APB_CLK_DIV_1_SEL, SYS_CLK_CTRL_REG);
}
/* change APB clk src to HOSC */
static inline void APBtoHOSC(void)
{
	/* div restore */
	writel((readl(SYS_CLK_CTRL_REG) & ~APB_CLK_DIV_MASK) \
		| (sys_clk_ctrl_bak & APB_CLK_DIV_MASK), SYS_CLK_CTRL_REG);

	/* src change to HOSC */
	writel((readl(SYS_CLK_CTRL_REG) & ~APB_CLK_SRC_SEL_MASK) \
		| (sys_clk_ctrl_bak & APB_CLK_SRC_SEL_MASK), SYS_CLK_CTRL_REG);
}

/* change AHB clk src to SYS_32K */
static inline void AHBtoSRC(uint32_t src_sel)
{
	/* AHB and M33 share the same clk */
	writel((readl(SYS_CLK_CTRL_REG) & ~M33_CLK_SRC_SEL_MASK) \
		| src_sel, SYS_CLK_CTRL_REG);

	writel((readl(SYS_CLK_CTRL_REG) & ~SYS_CLK_DIV_MASK) \
		| M33_CLK_DIV_1_SEL, SYS_CLK_CTRL_REG);
}
/* change AHB clk src to HOSC */
static inline void AHBtoHOSC(void)
{
	/* AHB and M33 share the same clk */
	writel((readl(SYS_CLK_CTRL_REG) & ~SYS_CLK_DIV_MASK) \
		| (sys_clk_ctrl_bak & SYS_CLK_DIV_MASK), SYS_CLK_CTRL_REG);

	writel((readl(SYS_CLK_CTRL_REG) & ~M33_CLK_SRC_SEL_MASK) \
		| (sys_clk_ctrl_bak & M33_CLK_SRC_SEL_MASK), SYS_CLK_CTRL_REG);
}

/* MBUS clk src named hpsramctrl_clk is from DPLL1 or DPLL3
 * do not need to change
 */
static inline void MBUStoSYS32K(void)
{

}
static inline void MBUStoHOSC(void)
{

}

static inline void rco_wup_timer_switch(uint32_t enable)
{
	if (enable) {
		writel((readl(BLE_RCOSC_CALIB_REG0) & ~RCO_WUP_TIME_EN_MASK) \
			| RCO_WUP_TIME_EN, BLE_RCOSC_CALIB_REG0);
	} else {
		writel(readl(BLE_RCOSC_CALIB_REG0) & ~RCO_WUP_TIME_EN_MASK, BLE_RCOSC_CALIB_REG0);
	}
}

void clk_suspend(standby_head_t *head)
{
	ccmu_aon_pll_save();
	bus_clk_save();

	/* enable RC_HF */
	if ((head->clkcfg & (1 << PM_RES_CLOCK_APB_FROM_RCO_HF))
			|| (head->clkcfg & (1 << PM_RES_CLOCK_AHB_FROM_RCO_HF))) {
		gprcm_rc_hf_swtich((uint32_t)RC_HF_EN_MASK);
	}

	if (!(head->clkcfg & (1 << PM_RES_CLOCK_RCO_CALIB)))
		rco_wup_timer_switch(0);

	/* MBUStoSYS32K(); */
	if (head->clkcfg & (1 << PM_RES_CLOCK_APB_FROM_RCO_HF)) {
		APBtoSRC(APB_RCO_HF_SEL);
		udelay(60);
	} else if (!(head->clkcfg & (1 << PM_RES_CLOCK_APB_NO_NEED_TO_32K))) {
		APBtoSRC(APB_SYS_32K_SEL);
		udelay(60);
	}

#if 0
	/* audpll ctrl by audio */
	if (!(head->clkcfg & (1 << PM_RES_CLOCK_AUDPLL)))
		ccmu_aon_audiopll_switch(clk_disable);
#endif

	if (!(head->clkcfg & (1 << PM_RES_CLOCK_DPLL2)))
		ccmu_aon_dpll2_switch(clk_disable);

	/* changge m33 clk src to sys32k before close DPLL3/DPLL1 */
	if (head->clkcfg & (1 << PM_RES_CLOCK_AHB_FROM_RCO_HF)) {
		AHBtoSRC(M33_RCO_HF_SEL);
		udelay(60);
	} else if (!(head->clkcfg & (1 << PM_RES_CLOCK_AHB_NO_NEED_TO_32K))) {
		AHBtoSRC(M33_SYS_32K_SEL);
		udelay(60);
	}

	if (!(head->clkcfg & (1 << PM_RES_CLOCK_DPLL3)))
		ccmu_aon_dpll3_switch(clk_disable);

#if 0
	if (!(head->clkcfg & (1 << PM_RES_CLOCK_DPLL1)))
		ccmu_aon_dpll1_switch(clk_disable);

	/* aud_rc_hf ctrl by audio */
	if (!(head->clkcfg & (1 << PM_RES_CLOCK_RCOSC_AUD_RC_HF)))
		gprcm_rc_hf_disable();
#endif

	if (!(readl(DPLL3_CTRL_REG) & DPLL_EN_MASK) && !(readl(AUDIO_PLL_CTRL_REG) & AUDPLL_EN_MASK)) {
		/* DPLL3 or AUDIOPLL may use PLL_LDO1 and PLL_LDO2 */
		writel((readl(CLK_LDO_CTRL_REG) & (~CLK_LDO1_EN_MASK)) \
			& (~CLK_LDO2_EN_MASK), CLK_LDO_CTRL_REG);

#if 0
		/* DCXO on/off by PMU */
		if (!(readl(DPLL1_CTRL_REG) & DPLL_EN_MASK) && !(readl(DPLL2_CTRL_REG) & DPLL_EN_MASK))
			ccmu_aon_dcxo_switch(clk_disable);
#endif
	}

}

void clk_resume(standby_head_t *head)
{
#if 0
	if (!(readl(DCXO_CTRL_REG) & DCXO_EN_MASK))
		ccmu_aon_dcxo_switch(clk_enable);
#endif

	if (!(readl(DPLL3_CTRL_REG) & DPLL_EN_MASK) && !(readl(AUDIO_PLL_CTRL_REG) & AUDPLL_EN_MASK))
		writel((readl(CLK_LDO_CTRL_REG) | CLK_LDO1_EN_MASK) \
			| CLK_LDO2_EN_MASK, CLK_LDO_CTRL_REG);

#if 0
	/* aud_rc_hf ctrl by audio */
	if (!(head->clkcfg & (1 << PM_RES_CLOCK_RCOSC_AUD_RC_HF)))
		gprcm_rc_hf_enable();

	/* consider earlier execution to speed up wakeup */
	if (!(head->clkcfg & (1 << PM_RES_CLOCK_DPLL1)))
		ccmu_aon_dpll1_switch(clk_enable);
#endif

	if (!(head->clkcfg & (1 << PM_RES_CLOCK_DPLL3)))
		ccmu_aon_dpll3_switch(clk_enable);

	if (head->clkcfg & (1 << PM_RES_CLOCK_AHB_FROM_RCO_HF)) {
		AHBtoHOSC();
		udelay(60);
	} else if (!(head->clkcfg & (1 << PM_RES_CLOCK_AHB_NO_NEED_TO_32K))) {
		AHBtoHOSC();
		udelay(60);
	}

	if (!(head->clkcfg & (1 << PM_RES_CLOCK_DPLL2)))
		ccmu_aon_dpll2_switch(clk_enable);

#if 0
	/* aud_rc_hf ctrl by audio */
	if (!(head->clkcfg & (1 << PM_RES_CLOCK_AUDPLL)))
		ccmu_aon_audiopll_switch(clk_enable);
#endif

	if (head->clkcfg & (1 << PM_RES_CLOCK_APB_FROM_RCO_HF)) {
		APBtoHOSC();
		udelay(60);
	} else if (!(head->clkcfg & (1 << PM_RES_CLOCK_APB_NO_NEED_TO_32K))) {
		APBtoHOSC();
		udelay(60);
	}
	/* MBUStoHOSC(); */

	if (!(head->clkcfg & (1 << PM_RES_CLOCK_RCO_CALIB)))
		rco_wup_timer_switch(1);

	if ((head->clkcfg & (1 << PM_RES_CLOCK_APB_FROM_RCO_HF))
			|| (head->clkcfg & (1 << PM_RES_CLOCK_AHB_FROM_RCO_HF))) {
		gprcm_rc_hf_swtich(sys_lfclk_ctrl_bak & (uint32_t)RC_HF_EN_MASK);
	}
}
