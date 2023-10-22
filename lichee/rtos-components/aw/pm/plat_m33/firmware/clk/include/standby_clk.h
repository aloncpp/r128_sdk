#ifndef __STANDBY_CLK_H__
#define __STANDBY_CLK_H__

#include "head.h"
#include "platform.h"

/* ccmu-aon */
#define DCXO_CTRL_REG			(CCMU_AON_BASE + 0x0088)
#define DCXO_EN_MASK			(0x1 << 31)
#define BUFFER_DPLL_EN			(0x1 << 9)

#define DPLL1_CTRL_REG			(CCMU_AON_BASE + 0x008c)
#define DPLL2_CTRL_REG			(CCMU_AON_BASE + 0x0090)
#define DPLL3_CTRL_REG			(CCMU_AON_BASE + 0x0094)
#define DPLL_EN_MASK			(0x1 << 31)
#define DPLL_NDIV_MASK			(0xff << 4)
#define DPLL_FACTORM_MASK		(0xf << 0)

#define AUDIO_PLL_CTRL_REG		(CCMU_AON_BASE + 0x0098)
#define AUDPLL_EN_MASK			(0x1 << 31)
#define AUDPLL_LOCK_EN_MASK		(0x1 << 29)
#define AUDPLL_LOCK_MASK		(0x1 << 28)
#define AUDPLL_FACTORN_MASK		(0x7f << 8)
#define AUDPLL_PRETDIVM_MASK		(0x1f << 0)

#define DPLL1_OUT_CFG_REG		(CCMU_AON_BASE + 0x00a4)
#define DPLL3_OUT_CFG_REG		(CCMU_AON_BASE + 0x00a8)
#define DPLL_OUT_CFG_LOCK_MASK		(0x1 << 30)

#define CLK_LDO_CTRL_REG		(CCMU_AON_BASE + 0x00ac)
#define CLK_LDO1_EN_MASK		(0x1 << 0)
#define CLK_LDO2_EN_MASK		(0x1 << 16)

#define MODULE_CLK_EN_CTRL		(CCMU_AON_BASE + 0x00cc)
#define GPIO_BUS_CLK_GATING_MASK	(0x1 << 11)

#define SYS_CLK_CTRL_REG		(CCMU_AON_BASE + 0x00e0)
#define M33_CLK_SRC_SEL_MASK		(0x3 << 12)
#define SYS_CLK_DIV_MASK		(0xf << 8)
#define APB_CLK_SRC_SEL_MASK		(0x3 << 6)
#define APB_CLK_DIV_MASK		(0x3 << 4)

#define APB_CLK_HOSC_SEL		(0x0 << 6)
#define APB_SYS_32K_SEL			(0x1 << 6)
#define APB_RCO_HF_SEL			(0x3 << 6)
#define APB_CLK_DIV_1_SEL		(0x0 << 4)
#define M33_CLK_HOSC_SEL		(0x0 << 12)
#define M33_SYS_32K_SEL			(0x1 << 12)
#define M33_RCO_HF_SEL			(0x3 << 12)
#define M33_CLK_DIV_1_SEL		(0x0 << 8)

/* MBUS: 0 DPLL1, 1 DPLL3; DIV default 8 */
#define CKPLL_HSPSRAM_SEL_MASK		(0x1 << 21)
#define MBUS_CLK_SEL_MASK		CKPLL_HSPSRAM_SEL_MASK

/* gprcm */
#define SYSTEM_PRIV_REG3		(GPRCM_BASE + 0x020c)

#define SYS_LFCLK_CTRL_REG		(GPRCM_BASE + 0x0080)
#define RC_HF_EN_MASK			(0x1 << 29)

/* standby status */
#define STANDBY_STATUS_REG		SYSTEM_PRIV_REG3
#define STANDBY_STATUS_SUSPEND		(0x10)
#define STANDBY_STATUS_RESUME		(0x20)

static inline void save_mem_status(volatile uint32_t val)
{
	writel(val, STANDBY_STATUS_REG);
}

void clk_suspend(standby_head_t *head);
void clk_resume(standby_head_t *head);

#endif /* __STANDBY_CLK_H__ */

