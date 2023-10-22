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

 #include "type.h"
 #include "delay.h"
 #include "head.h"
 #include "resource.h"

 #include "standby_power.h"

static volatile uint32_t ext_ldo_ctrl_reg_bak;
static volatile uint32_t sys_low_pwr_stat_reg_bak;
static volatile uint32_t ble_clk32k_sw_reg0_bak;
static volatile uint32_t sys_lfclk_ctrl_bak;
static volatile uint32_t ble_rcosc_calib_reg0_bak;

/* ldo settings save */
static inline void ext_ldo_save(void)
{
	ext_ldo_ctrl_reg_bak = readl(EXT_LDO_CTRL_REG);
	sys_low_pwr_stat_reg_bak = readl(SYS_LOW_POWER_STATUS_REG);
}

/* ldo on/off */
static inline void dsp_ldo_disable(void)
{
	/* DSP_SLEEP BIT will not change if DSP is no alive */
	if (readl(SYS_LOW_POWER_STATUS_REG) & DSP_ALIVE) {

		writel(readl(SYS_LOW_POWER_CTRL_REG) & ~DSP_WUP_EN_MASK,
			SYS_LOW_POWER_CTRL_REG);

		while(!(readl(SYS_LOW_POWER_STATUS_REG) & DSP_SLEEP));
	}
}

static inline void dsp_ldo_enable(void)
{
	if (sys_low_pwr_stat_reg_bak & DSP_ALIVE) {
		writel(readl(SYS_LOW_POWER_CTRL_REG) | DSP_WUP_EN_MASK,
			SYS_LOW_POWER_CTRL_REG);

		/* wait for stability */
		while(!(readl(SYS_LOW_POWER_STATUS_REG) & DSP_ALIVE));
	}
}

static inline void sw_rv_disable(void)
{
	if (readl(SYS_LOW_POWER_STATUS_REG) & RV_ALIVE) {
		writel(readl(SYS_LOW_POWER_CTRL_REG) & ~RV_WUP_EN_MASK,
			SYS_LOW_POWER_CTRL_REG);

		while(!(readl(SYS_LOW_POWER_STATUS_REG) & RV_SLEEP));
	}
}

static inline void sw_rv_enable(void)
{
	if (readl(sys_low_pwr_stat_reg_bak) & RV_ALIVE) {
		writel(readl(SYS_LOW_POWER_CTRL_REG) | RV_WUP_EN_MASK,
			SYS_LOW_POWER_CTRL_REG);

		/* wait for stability */
		while(!(readl(SYS_LOW_POWER_STATUS_REG) & RV_ALIVE));
	}
}

static inline void ext_ldo_disable(void)
{
	/* set ext_ldo on/off same as TOP_LDO controled by PMU */
	writel((readl(EXT_LDO_CTRL_REG) & ~EXT_LDO_EN_MASK) \
		| EXT_LDO_EN_BY_PMU, EXT_LDO_CTRL_REG);
}

static inline void ext_ldo_enable(void)
{
	/* restore ext_ldo_en settings */
	writel((readl(EXT_LDO_CTRL_REG) & ~EXT_LDO_EN_MASK) \
		| (ext_ldo_ctrl_reg_bak & EXT_LDO_EN_MASK), EXT_LDO_CTRL_REG);
}

static inline void aldo_disable(void)
{
	writel(readl(AC_POWER_CTRL_REG) & ~ALDO_EN_MASK, AC_POWER_CTRL_REG);
}

static void aldo_enable(void)
{
	writel(readl(AC_POWER_CTRL_REG) | ALDO_EN_MASK, AC_POWER_CTRL_REG);

	/* wait aldo to be stable */
	udelay(1000);
}

/* set hibernation or standby mode */
void hibernation_mode_set(void)
{
	ble_clk32k_sw_reg0_bak = readl(BLE_CLK32K_SW_REG0);
	sys_lfclk_ctrl_bak = readl(SYS_LFCLK_CTRL);
	ble_rcosc_calib_reg0_bak = readl(BLE_RCOSC_CALIB_REG0);

	/* set 1 then AON_LDO will be disabled when APP system goes into sleep(as hibernation) */
	writel(readl(SYS_LOW_POWER_CTRL_REG) | STANDBY_HIBERNATION_MODE_SEL_MASK,
		SYS_LOW_POWER_CTRL_REG);

	/* AON_LDO will wait for CLK32K_AUTO_SW complete, CLK32K_AUTO_SW must be disable */
	writel(readl(BLE_CLK32K_SW_REG0) & ~CLK32K_AUTO_SW_EN_MASK, BLE_CLK32K_SW_REG0);

	/* RCCAL located in AON, RCCAL will stop in hibernation mode, which could not be the clk src of SYS32K */
	writel(readl(SYS_LFCLK_CTRL) & ~SYS_32K_SEL_MASK, SYS_LFCLK_CTRL);

	/* Stop RCOSC CALIB wakeup timer, which will wakeupsystem from hibernation */
	writel(readl(BLE_RCOSC_CALIB_REG0) & ~RCO_WUP_TIME_EN_MASK, BLE_RCOSC_CALIB_REG0);
}

void hibernation_mode_restore(void)
{
	writel(ble_rcosc_calib_reg0_bak, BLE_RCOSC_CALIB_REG0);
	writel(sys_lfclk_ctrl_bak, SYS_LFCLK_CTRL);
	writel(ble_clk32k_sw_reg0_bak, BLE_CLK32K_SW_REG0);

	/* set 0 then AON_LDO will be not disabled when APP system goes into sleep(as standby) */
	writel(readl(SYS_LOW_POWER_CTRL_REG) & ~STANDBY_HIBERNATION_MODE_SEL_MASK,
		SYS_LOW_POWER_CTRL_REG);
}

/*
 * power list:
 * 1. CLAMP/Debc/BG/POR can not be disabled.
 * 2. DCDC will be disbaled/enabled by PMU hardware.
 * 3. EXT-LDO
 * 	4. ALDO
 * 	5. DSP_LDO
 * 	6. AON-LDO will be disbaled/enabled by PMU hardware. 
 * 	7. APP-LDO will be disbaled/enabled by PMU hardware.
 *	8. RTC_LDO can not be disabled.
 * 		9. SW_RV
 */
void ldo_disable(standby_head_t *head)
{
	/* set System Low-Power Control Resgister(0x40051500) to defalut value(0x1100)
	   in case AON_LDO voltage power down follow m33A when APP system goes into sleep(as standby) */
	writel(readl(SYS_LOW_POWER_CTRL_REG) & ~STANDBY_HIBERNATION_MODE_SEL_MASK,
		SYS_LOW_POWER_CTRL_REG);
#if 0
	ext_ldo_save();
#endif
	if (!(head->pwrcfg & (1 << PM_RES_POWER_RV_LDO)))
		sw_rv_disable();

	if (!(head->pwrcfg & (1 << PM_RES_POWER_DSP_LDO)))
		dsp_ldo_disable();

#if 0
	/* ctrl by PMU hardware */
	if (!(head->pwrcfg & (1 << PM_RES_POWER_ALDO)))
		aldo_disable();

	if (!(head->pwrcfg & (1 << PM_RES_POWER_EXT_LDO)))
		ext_ldo_disable();
#endif
}

void ldo_enable(standby_head_t *head)
{
#if 0
	if (!(head->pwrcfg & (1 << PM_RES_POWER_EXT_LDO)))
		ext_ldo_enable();

	if (!(head->pwrcfg & (1 << PM_RES_POWER_ALDO)))
		aldo_enable();
#endif

	/* enable DSP_LDO when it is ready to boot DSP
	if (!(head->pwrcfg & (1 << PM_RES_POWER_DSP_LDO)))
		dsp_ldo_enable();
	*/

	/* enable SW_RV when it is ready to boot RV
	if (!(head->pwrcfg & (1 << PM_RES_POWER_RV_LDO)))
		sw_rv_enable();
	*/
}
