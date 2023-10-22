/*
 * Allwinner Sun20iw2 clock register definitions
 *
 * (C) Copyright 2017  <wangwei@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _SUNXI_CLOCK_SUN20IW2_H
#define _SUNXI_CLOCK_SUN20IW2_H

struct sunxi_ccm_reg {
	u32 reserved_0x000[1];			/* 0x0000 */
	u32 bus_clk_gating_ctrl0;
	u32 bus_clk_gating_ctrl1;
	u32 dev_rst_ctrl0;
	u32 dev_rst_ctrl1;			/* 0x0010 */
	u32 cpu_dsp_rv_clk_gating_ctrl;
	u32 cpu_dsp_rv_rst_ctrl;
	u32 mbus_clk_gating_ctrl;
	u32 spi0_clk_ctrl;			/* 0x0020 */
	u32 spi1_clk_ctrl;
	u32 sdc_clk_ctrl;
	u32 ss_clk_ctrl;
	u32 csi_dclk_ctrl;			/* 0x0030 */
	u32 ledc_clk_ctrl;
	u32 irrx_clk_ctrl;
	u32 irtx_clk_ctrl;
	u32 systick_refclk_ctrl;		/* 0x0040 */
	u32 systick_calib_ctrl;
	u32 reserved_0x048[2];
	u32 csi_out_mclk_ctrl;			/* 0x0050 */
	u32 flashc_mclk_ctrl;
	u32 sqpi_psramc_clk_ctrl;
	u32 apb_spc_clk_ctrl;
	u32 usb_clk_ctrl;			/* 0x0060 */
	u32 riscv_clk_ctrl;
	u32 dsp_clk_ctrl;
	u32 hpsram_clk_ctrl;
	u32 lpsram_clk_ctrl;			/* 0x0070 */
	u32 g2d_clk_ctrl;
	u32 de_clk_ctrl;
	u32 lcd_clk_ctrl;
	u32 reserved_0x080[0x100-0x80];		/* 0x0080 */
	u32 reset_source_recode;		/* 0x0100 */
};

#define UART_RESET_SHIFT		(6)
#define UART_GATING_SHIFT		(6)

#define SPI_RESET_SHIFT			(0)
#define SPI_GATING_SHIFT		(0)

/* pll1 bit field */
#define CCM_PLL1_CTRL_EN		BIT(31)
#define CCM_PLL1_LOCK_EN		BIT(29)
#define CCM_PLL1_LOCK			BIT(28)
#define CCM_PLL1_CLOCK_TIME_2		(2 << 24)
#define CCM_PLL1_CTRL_P(p)		((p) << 16)
#define CCM_PLL1_CTRL_N(n)		((n) << 8)

/* pll5 bit field */
#define CCM_PLL5_CTRL_EN		BIT(31)
#define CCM_PLL5_LOCK_EN		BIT(29)
#define CCM_PLL5_LOCK			BIT(28)
#define CCM_PLL5_CTRL_N(n)		((n) << 8)
#define CCM_PLL5_CTRL_DIV1(div1)	((div1) << 0)
#define CCM_PLL5_CTRL_DIV2(div0)	((div0) << 1)

/* pll6 bit field */
#define CCM_PLL6_CTRL_EN		BIT(31)
#define CCM_PLL6_LOCK_EN		BIT(29)
#define CCM_PLL6_LOCK			BIT(28)
#define CCM_PLL6_CTRL_N_SHIFT		8
#define CCM_PLL6_CTRL_N_MASK		(0xff << CCM_PLL6_CTRL_N_SHIFT)
#define CCM_PLL6_CTRL_DIV1_SHIFT	0
#define CCM_PLL6_CTRL_DIV1_MASK		(0x1 << CCM_PLL6_CTRL_DIV1_SHIFT)
#define CCM_PLL6_CTRL_DIV2_SHIFT	1
#define CCM_PLL6_CTRL_DIV2_MASK		(0x1 << CCM_PLL6_CTRL_DIV2_SHIFT)
#define CCM_PLL6_DEFAULT		0xa0006300

/* cpu_axi bit field*/
#define CCM_CPU_AXI_MUX_MASK		(0x3 << 24)
#define CCM_CPU_AXI_MUX_OSC24M		(0x0 << 24)
#define CCM_CPU_AXI_MUX_PLL_CPUX	(0x3 << 24)

/* psi_ahb1_ahb2 bit field */
#define CCM_PSI_AHB1_AHB2_DEFAULT	0x03000102

/* ahb3 bit field */
#define CCM_AHB3_DEFAULT		0x03000002

/* apb1 bit field */
#define CCM_APB1_DEFAULT		0x03000102

/* apb2 bit field */
#define APB2_CLK_SRC_OSC24M		(0x0 << 24)
#define APB2_CLK_SRC_OSC32K		(0x1 << 24)
#define APB2_CLK_SRC_PSI		(0x2 << 24)
#define APB2_CLK_SRC_PLL6		(0x3 << 24)
#define APB2_CLK_SRC_MASK		(0x3 << 24)
#define APB2_CLK_RATE_N_1		(0x0 << 8)
#define APB2_CLK_RATE_N_2		(0x1 << 8)
#define APB2_CLK_RATE_N_4		(0x2 << 8)
#define APB2_CLK_RATE_N_8		(0x3 << 8)
#define APB2_CLK_RATE_N_MASK		(3 << 8)
#define APB2_CLK_RATE_M(m)		(((m)-1) << 0)
#define APB2_CLK_RATE_M_MASK            (3 << 0)

/* MBUS clock bit field */
#define MBUS_ENABLE			BIT(31)
#define MBUS_RESET			BIT(30)
#define MBUS_CLK_SRC_MASK		GENMASK(25, 24)
#define MBUS_CLK_SRC_OSCM24		(0 << 24)
#define MBUS_CLK_SRC_PLL6X2		(1 << 24)
#define MBUS_CLK_SRC_PLL5		(2 << 24)
#define MBUS_CLK_SRC_PLL6X4		(3 << 24)
#define MBUS_CLK_M(m)			(((m)-1) << 0)

/* Module gate/reset shift*/
#define RESET_SHIFT			(16)
#define GATING_SHIFT			(0)

/* MMC clock bit field */
#define CCM_MMC_CTRL_M(x)		((x) - 1)
#define CCM_MMC_CTRL_N(x)		((x) << 16)
#define CCM_MMC_CTRL_OSCM24		(0x0 << 24)
#define CCM_MMC_CTRL_PLL6X2		(0x2 << 24)
#define CCM_MMC_CTRL_PLL_PERIPH2X2	(0x2 << 24)
#define CCM_MMC_CTRL_ENABLE		(0x1 << 31)
/* if doesn't have these delays */
#define CCM_MMC_CTRL_OCLK_DLY(a)	((void) (a), 0)
#define CCM_MMC_CTRL_SCLK_DLY(a)	((void) (a), 0)

/*CE*/
#define CE_CLK_SRC_MASK                   (0x3)
#define CE_CLK_SRC_SEL_BIT                (24)
#define CE_CLK_SRC                        (0x01)

#define CE_CLK_DIV_RATION_N_BIT           (8)
#define CE_CLK_DIV_RATION_N_MASK          (0x3)
#define CE_CLK_DIV_RATION_N               (0)

#define CE_CLK_DIV_RATION_M_BIT           (0)
#define CE_CLK_DIV_RATION_M_MASK          (0xF)
#define CE_CLK_DIV_RATION_M               (2)

#define CE_SCLK_ONOFF_BIT                 (31)
#define CE_SCLK_ON                        (1)

#define CE_GATING_BASE                    CCMU_CE_BGR_REG
#define CE_GATING_PASS                    (1)
#define CE_GATING_BIT                     (0)

#define CE_RST_REG_BASE                   CCMU_CE_BGR_REG
#define CE_RST_BIT                        (16)
#define CE_DEASSERT                       (1)

#define CE_MBUS_GATING_MASK               (1)
#define CE_MBUS_GATING_BIT		  (2)
#define CE_MBUS_GATING			  (1)
#endif /* _SUNXI_CLOCK_SUN20IW2_H */
