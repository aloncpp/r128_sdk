// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 Allwinnertech
 */
#include "ccu.h"
#include "ccu_common.h"
#include "ccu_reset.h"
#include "ccu_div.h"
#include "ccu_gate.h"
#include "ccu_mp.h"
#include "ccu_mult.h"
#include "ccu_nk.h"
#include "ccu_nkm.h"
#include "ccu_nkmp.h"
#include "ccu_nm.h"

#include "ccu-sun60iw1.h"
#include <hal_clk.h>
#include <hal_reset.h>

/* ccu_des_start */
#define SUN60IW1_PLL_CPU0_CTRL_REG   0x0000
static struct ccu_mult pll_cpu0_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.mult		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.common		= {
		.reg		= 0x0000,
		.hw.init	= CLK_HW_INIT("pll-cpu0", "dcxo24M",
					      &ccu_mult_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

/* @TODO */
#define SUN60IW1_PLL_CPU1_CTRL_REG   0x0008
#define SUN60IW1_PLL_CPU2_CTRL_REG   0x000C

#define SUN60IW1_PLL_DDR_CTRL_REG   0x0010
static struct ccu_nkmp pll_ddr_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(0, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(1, 1), /* output divider */
	.common		= {
		.reg		= 0x0010,
		.hw.init	= CLK_HW_INIT("pll-ddr", "dcxo24M",
					      &ccu_nkmp_ops,
					      CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL),
	},
};

#define SUN60IW1_PLL_PERI0_CTRL_REG   0x0020
static struct ccu_nm pll_peri0_parent_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x0020,
		.hw.init	= CLK_HW_INIT("pll-peri0-parent", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL),
	},
};

static SUNXI_CCU_M(pll_peri0_2x_clk, "pll-peri0-2x",
		   "pll-peri0-parent", 0x0020, 16, 3, 0);

static CLK_FIXED_FACTOR_HW(pll_peri0_div3_clk, "pll-peri0-div3",
			   &pll_peri0_2x_clk.common.hw,
			   6, 1, 0);

static SUNXI_CCU_M(pll_peri0_800m_clk, "pll-peri0-800m",
		   "pll-peri0-parent", 0x0020, 20, 3, 0);

static SUNXI_CCU_M(pll_peri0_480m_clk, "pll-peri0-480m",
		   "pll-peri0-parent", 0x0020, 2, 3, 0);

static CLK_FIXED_FACTOR_HW(pll_peri0_600m_clk, "pll-peri0-600m",
			   &pll_peri0_2x_clk.common.hw,
			   2, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri0_400m_clk, "pll-peri0-400m",
			   &pll_peri0_2x_clk.common.hw,
			   3, 1, 0);

static CLK_FIXED_FACTOR(pll_peri0_300m_clk, "pll-peri0-300m",
			   "pll-peri0-600m",
			   2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri0_200m_clk, "pll-peri0-200m",
			   "pll-peri0-400m",
			   2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri0_160m_clk, "pll-peri0-160m",
			   "pll-peri0-480m",
			   3, 1, 0);

static CLK_FIXED_FACTOR(pll_peri0_16m_clk, "pll-peri0-16m",
			   "pll-peri0-160m",
			   10, 1, 0);

static CLK_FIXED_FACTOR(pll_peri0_150m_clk, "pll-peri0-150m",
			   "pll-peri0-300m",
			   2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri0_25m_clk, "pll-peri0-25m",
			   "pll-peri0-150m",
			   6, 1, 0);

#define SUN60IW1_PLL_PERI1_CTRL_REG   0x0028
static struct ccu_nm pll_peri1_parent_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x0028,
		.hw.init	= CLK_HW_INIT("pll-peri1-parent", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL),
	},
};

static SUNXI_CCU_M(pll_peri1_2x_clk, "pll-peri1-2x",
		   "pll-peri1-parent", 0x0028, 16, 3, 0);

static SUNXI_CCU_M(pll_peri1_800m_clk, "pll-peri1-800m",
		   "pll-peri1-parent", 0x0028, 20, 3, 0);

static SUNXI_CCU_M(pll_peri1_480m_clk, "pll-peri1-480m",
		   "pll-peri1-parent", 0x0028, 2, 3, 0);

static CLK_FIXED_FACTOR_HW(pll_peri1_600m_clk, "pll-peri1-600m",
			   &pll_peri1_2x_clk.common.hw,
			   2, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_peri1_400m_clk, "pll-peri1-400m",
			   &pll_peri1_2x_clk.common.hw,
			   3, 1, 0);

static CLK_FIXED_FACTOR(pll_peri1_300m_clk, "pll-peri1-300m",
			   "pll-peri1-600m",
			   2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri1_200m_clk, "pll-peri1-200m",
			   "pll-peri1-400m",
			   2, 1, 0);

static CLK_FIXED_FACTOR(pll_peri1_160m_clk, "pll-peri1-160m",
			   "pll-peri1-480m",
			   3, 1, 0);

static CLK_FIXED_FACTOR(pll_peri1_150m_clk, "pll-peri1-150m",
			   "pll-peri1-300m",
			   2, 1, 0);

#define SUN60IW1_PLL_GPU_CTRL_REG   0x0030
static struct ccu_nkmp pll_gpu_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(0, 1), /* output divider */
	.common		= {
		.reg		= 0x0030,
		.hw.init	= CLK_HW_INIT("pll-gpu", "dcxo24M",
					      &ccu_nkmp_ops,
					      CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL),
	},
};

#define SUN60IW1_PLL_VIDEO0_CTRL_REG   0x0040
static struct ccu_nm pll_video0_4x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x0040,
		.hw.init	= CLK_HW_INIT("pll-video0-4x", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL),
	},
};

static CLK_FIXED_FACTOR_HW(pll_video0_2x_clk, "pll-video0-2x",
			   &pll_video0_4x_clk.common.hw,
			   2, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_video0_1x_clk, "pll-video0-1x",
			   &pll_video0_4x_clk.common.hw,
			   4, 1, 0);

#define SUN60IW1_PLL_VIDEO1_CTRL_REG   0x0048
static struct ccu_nm pll_video1_4x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(0, 1), /* input divider */
	.common		= {
		.reg		= 0x0048,
		.hw.init	= CLK_HW_INIT("pll-video1-4x", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL),
	},
};

static CLK_FIXED_FACTOR_HW(pll_video1_2x_clk, "pll-video1-2x",
			   &pll_video1_4x_clk.common.hw,
			   2, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_video1_1x_clk, "pll-video1-1x",
			   &pll_video1_4x_clk.common.hw,
			   4, 1, 0);

#define SUN60IW1_PLL_VIDEO2_CTRL_REG   0x0050
static struct ccu_nm pll_video2_4x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x0050,
		.hw.init	= CLK_HW_INIT("pll-video2-4x", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL),
	},
};

static CLK_FIXED_FACTOR_HW(pll_video2_2x_clk, "pll-video2-2x",
			   &pll_video2_4x_clk.common.hw,
			   2, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_video2_1x_clk, "pll-video2-1x",
			   &pll_video2_4x_clk.common.hw,
			   4, 1, 0);

#define SUN60IW1_PLL_VE_CTRL_REG   0x0058
static struct ccu_nkmp pll_ve_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(0, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(1, 1), /* output divider */
	.common		= {
		.reg		= 0x0058,
		.hw.init	= CLK_HW_INIT("pll-ve", "dcxo24M",
					      &ccu_nkmp_ops,
					      CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL),
	},
};

#define SUN60IW1_PLL_VIDEO3_CTRL_REG   0x0068
static struct ccu_nm pll_video3_4x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x0068,
		.hw.init	= CLK_HW_INIT("pll-video3-4x", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL),
	},
};

static CLK_FIXED_FACTOR_HW(pll_video3_2x_clk, "pll-video3-2x",
			   &pll_video3_4x_clk.common.hw,
			   2, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_video3_1x_clk, "pll-video3-1x",
			   &pll_video3_4x_clk.common.hw,
			   4, 1, 0);

#define SUN60IW1_PLL_AUDIO0_REG		0x078
static struct ccu_sdm_setting pll_audio0_sdm_table[] = {
	{ .rate = 45158400, .pattern = 0xc001bcd3, .m = 18, .n = 33 },
	{ .rate = 49152000, .pattern = 0xc001eb85, .m = 20, .n = 40 },
	{ .rate = 180633600, .pattern = 0xc001288d, .m = 3, .n = 22 },
	{ .rate = 196608000, .pattern = 0xc001eb85, .m = 5, .n = 40 },
};

static struct ccu_nm pll_audio0_4x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(16, 6),
	.fixed_post_div	= 2,
	.sdm		= _SUNXI_CCU_SDM(pll_audio0_sdm_table, BIT(24),
					 0x178, BIT(31)),
	.common		= {
		.reg		= 0x078,
		.features	= CCU_FEATURE_FIXED_POSTDIV |
				  CCU_FEATURE_SIGMA_DELTA_MOD,
		.hw.init	= CLK_HW_INIT("pll-audio0-4x", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

static CLK_FIXED_FACTOR_HW(pll_audio0_2x_clk, "pll-audio0-2x",
			   &pll_audio0_4x_clk.common.hw,
			   2, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_audio0_1x_clk, "pll-audio0-1x",
			   &pll_audio0_4x_clk.common.hw,
			   4, 1, 0);

static CLK_FIXED_FACTOR(pll_audio0_div_48m_clk, "pll-audio0-div-48m",
			   "pll-audio0-2x", 4, 1, 0);

#define SUN60IW1_PLL_NPU_CTRL_REG   0x0080
static struct ccu_nm pll_npu_4x_clk = {
	.enable		= BIT(27),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x0080,
		.hw.init	= CLK_HW_INIT("pll-npu-4x", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL),
	},
};

static CLK_FIXED_FACTOR_HW(pll_npu_2x_clk, "pll-npu-2x",
			   &pll_npu_4x_clk.common.hw,
			   2, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_npu_1x_clk, "pll-npu-1x",
			   &pll_npu_4x_clk.common.hw,
			   4, 1, 0);

static struct clk_div_table cpu0_div_table[] = {
	{ .val = 0, .div = 1 },
	{ .val = 1, .div = 2 },
	{ .val = 2, .div = 4 },
};

static SUNXI_CCU_DIV_TABLE(cpu0_div_clk, "cpu0-div", "pll-cpu0",
		0x0500, 16, 2, cpu0_div_table, CLK_SET_RATE_PARENT);

static const char * const cpu_parents[] = { "dcxo24M", "osc32k", "iosc", "cpu0-div", "pll-peri0-600m", "pll-cpu2" };

static SUNXI_CCU_MUX(cpu_clk, "cpu", cpu_parents,
		     0x0500, 24, 3, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static SUNXI_CCU_M(cpu_axi_clk, "cpu-axi",
		   "dsu", 0x0500, 1, 2, 0);

static SUNXI_CCU_M(cpu_apb_clk, "cpu-apb",
		   "dsu", 0x0500, 8, 2, 0);

static SUNXI_CCU_M(cpu_peri_clk, "cpu-peri",
		   "dsu", 0x0500, 2, 2, 0);

/* wrong clk name:0504
static SUNXI_CCU_GATE(cpu_gatin_clk, "cpu-gatin",
                      "cpu-gatin-clk-parents",
                      0x0504, BIT(16), 0);
*/
static SUNXI_CCU_GATE(dsu_clk, "dsu", "hosc",
                      0x0504, BIT(1), 0);
/*
static SUNXI_CCU_GATE(cpu0_clk_clk, "cpu0-clk",
                      "cpu0-clk-clk-parents",
                      0x0504, BIT(0), 0);
*/

static const char * const trace_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-peri0-300m", "pll-peri0-400m" };

static SUNXI_CCU_M_WITH_MUX_GATE(trace_clk, "trace",
                                 trace_parents, 0x0508,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static struct clk_div_table cpu1_div_table[] = {
	{ .val = 0, .div = 1 },
	{ .val = 1, .div = 2 },
	{ .val = 2, .div = 4 },
};

static SUNXI_CCU_DIV_TABLE(cpu1_div_clk, "cpu1-div", "pll-cpu1",
		0x050c, 16, 2, cpu1_div_table, CLK_SET_RATE_PARENT);

static const char * const dsu_parents[] = { "dcxo24M", "osc32k", "iosc", "cpu1-div", "pll-peri0-2x", "pll-peri0-600m" };

static SUNXI_CCU_MUX(dsu_parents_clk, "dsu-parents", dsu_parents,
		     0x050c, 24, 3, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static const char * const ahb_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-peri0-600m" };

SUNXI_CCU_M_WITH_MUX(ahb_clk, "ahb", ahb_parents,
		     0x0510, 0, 5, 24, 2, CLK_SET_RATE_PARENT);

static const char * const apb0_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-peri0-600m" };

SUNXI_CCU_M_WITH_MUX(apb0_clk, "apb0", apb0_parents,
		     0x0520, 0, 5, 24, 2, CLK_SET_RATE_PARENT);

static const char * const apb1_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-peri0-600m" };

SUNXI_CCU_M_WITH_MUX(apb1_clk, "apb1", apb1_parents,
		     0x0524, 0, 5, 24, 2, CLK_SET_RATE_PARENT);

static const char * const mbus_parents[] = { "pll-ddr", "pll-peri1-600m", "pll-peri1-480m", "pll-peri1-400m", "pll-peri1-150m", "hosc" };

static SUNXI_CCU_M_WITH_MUX_GATE(mbus_clk, "mbus",
                                 mbus_parents, 0x0540,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(nsi_clk, "nsi",
                      "dcxo24M",
                      0x054C, BIT(0), 0);

static const char * const gic_parents[] = { "dcxo24M", "osc32k", "pll-peri0-600m", "pll-peri0-480m" };

static SUNXI_CCU_M_WITH_MUX_GATE(gic_clk, "gic",
                                 gic_parents, 0x0550,
			         0, 5,	/* M */
			         24, 2,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static const char * const de_parents[] = { "pll-peri0-300m", "pll-peri0-400m", "pll-video0-4x", "pll-video1-4x", "pll-video2-4x", "pll-video3-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(de_clk, "de",
                                 de_parents, 0x0600,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(de0_clk, "de0",
                      "dcxo24M",
                      0x060C, BIT(0), 0);

static const char * const di_parents[] = { "pll-peri0-400m", "pll-video0-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(di_clk, "di",
                                 di_parents, 0x0620,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(bus_di_clk, "bus-di",
                      "dcxo24M",
                      0x062C, BIT(0), 0);

static const char * const g2d_parents[] = { "pll-peri0-400m", "pll-peri0-300m", "pll-video0-4x", "pll-video1-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(g2d_clk, "g2d",
                                 g2d_parents, 0x0630,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(bus_g2d_clk, "bus-g2d",
                      "dcxo24M",
                      0x063C, BIT(0), 0);

static const char * const gpu_parents[] = { "pll-peri0-800m", "pll-peri0-600m", "pll-peri0-400m", "pll-peri0-300m", "pll-peri0-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(gpu_clk, "gpu",
                                 gpu_parents, 0x0670,
			         0, 4,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(bus_gpu_clk, "bus-gpu",
                      "dcxo24M",
                      0x067C, BIT(0), 0);

static const char * const ce_parents[] = { "dcxo24M", "pll-peri0-480m", "pll-peri0-400m", "pll-peri0-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(ce_clk, "ce",
                                 ce_parents, 0x0680,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(ce_sys_clk, "ce-sys",
                      "dcxo24M",
                      0x068C, BIT(1), 0);

static SUNXI_CCU_GATE(bus_ce_clk, "bus-ce",
                      "dcxo24M",
                      0x068C, BIT(0), 0);

static const char * const ve_parents[] = { "pll-ve", "pll-peri0-480m", "pll-peri0-400m", "pll-peri0-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(ve_clk, "ve",
                                 ve_parents, 0x0690,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(bus_ve_clk, "bus-ve",
                      "dcxo24M",
                      0x069C, BIT(0), 0);

static const char * const npu_parents[] = { "pll-peri0-480m", "pll-peri0-600m", "pll-peri0-800m", "npupll4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(npu_clk, "npu",
                                 npu_parents, 0x06E0,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(dma_clk, "dma",
                      "dcxo24M",
                      0x070C, BIT(0), 0);

static SUNXI_CCU_GATE(msgbox1_clk, "msgbox1",
                      "dcxo24M",
                      0x071C, BIT(1), 0);

static SUNXI_CCU_GATE(msgbox0_clk, "msgbox0",
                      "dcxo24M",
                      0x071C, BIT(0), 0);

static SUNXI_CCU_GATE(spinlock_clk, "spinlock",
                      "dcxo24M",
                      0x072C, BIT(0), 0);

static SUNXI_CCU_GATE(timer_clk, "timer",
                      "dcxo24M",
                      0x074C, BIT(0), 0);

static SUNXI_CCU_GATE(dbgsys_clk, "dbgsys",
                      "dcxo24M",
                      0x078C, BIT(0), 0);

static SUNXI_CCU_GATE(pwm1_clk, "pwm1",
			"dcxo24M",
			0x07AC, BIT(1), 0);

static SUNXI_CCU_GATE(pwm_clk, "pwm",
			"dcxo24M",
			0x07AC, BIT(0), 0);

static SUNXI_CCU_GATE(iommu_clk, "iommu",
                      "dcxo24M",
                      0x07BC, BIT(0), 0);

static const char * const timer_parents[] = { "dcxo24M", "iosc", "clk-32k", "clk-peri0-200m" };

static struct ccu_div timer0_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0730,
		.hw.init	= CLK_HW_INIT_PARENTS("timer0", timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div timer1_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0730,
		.hw.init	= CLK_HW_INIT_PARENTS("timer1", timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div timer2_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0730,
		.hw.init	= CLK_HW_INIT_PARENTS("timer2", timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div timer3_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0730,
		.hw.init	= CLK_HW_INIT_PARENTS("timer3", timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div timer4_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0730,
		.hw.init	= CLK_HW_INIT_PARENTS("timer4", timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div timer5_clk = {
	.enable		= BIT(31),
	.div		= _SUNXI_CCU_DIV_FLAGS(0, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 3),
	.common		= {
		.reg		= 0x0730,
		.hw.init	= CLK_HW_INIT_PARENTS("timer5", timer_parents, &ccu_div_ops, 0),
	},
};

static const char * const dram_parents[] = { "pll-ddr", "pll-peri1-600m", "peri1-480m", "pll-peri1-400m", "peri1-150m" };

static SUNXI_CCU_M_WITH_MUX_GATE(dram_clk, "dram",
                                 dram_parents, 0x0800,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(usb_mbus_gate_clk, "usb-mbus-gate",
                      "dcxo24M",
                      0x0804, BIT(25), 0);

static SUNXI_CCU_GATE(gpu_mbus_gate_clk, "gpu-mbus-gate",
                      "dcxo24M",
                      0x0804, BIT(24), 0);

static SUNXI_CCU_GATE(de_mbus_gate_clk, "de-mbus-gate",
                      "dcxo24M",
                      0x0804, BIT(23), 0);

static SUNXI_CCU_GATE(nand_mbus_gate_clk, "nand-mbus-gate",
                      "dcxo24M",
                      0x0804, BIT(22), 0);

static SUNXI_CCU_GATE(npu_mbus_gate_clk, "npu-mbus-gate",
                      "dcxo24M",
                      0x0804, BIT(21), 0);

static SUNXI_CCU_GATE(vid_in_mbus_gate_clk, "vid-in-mbus-gate",
                      "dcxo24M",
                      0x0804, BIT(20), 0);

static SUNXI_CCU_GATE(vid_out_mbus_gate_clk, "vid-out-mbus-gate",
                      "dcxo24M",
                      0x0804, BIT(19), 0);

static SUNXI_CCU_GATE(ce_mbus_gate_clk, "ce-mbus-gate",
                      "dcxo24M",
                      0x0804, BIT(18), 0);

static SUNXI_CCU_GATE(ve_mbus_gate_clk, "ve-mbus-gate",
                      "dcxo24M",
                      0x0804, BIT(17), 0);

static SUNXI_CCU_GATE(dma_mbus_gate_clk, "dma-mbus-gate",
                      "dcxo24M",
                      0x0804, BIT(16), 0);

static SUNXI_CCU_GATE(bus_dram_clk, "bus-dram",
                      "dcxo24M",
                      0x080C, BIT(0), 0);

static const char * const nand0_clk0_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(nand0_clk0_clk, "nand0-clk0",
                                 nand0_clk0_parents, 0x0810,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static const char * const nand0_clk1_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_M_WITH_MUX_GATE(nand0_clk1_clk, "nand0-clk1",
                                 nand0_clk1_parents, 0x0814,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(nand0_clk, "nand0",
                      "dcxo24M",
                      0x082C, BIT(0), 0);

static const char * const smhc0_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc0_clk, "smhc0",
                                           smhc0_parents, 0x0830,
                                           0, 5,	/* M */
                                           8, 5,	/* N */
			                   24, 3,	/* mux */
					   BIT(31), 0);

static const char * const smhc1_parents[] = { "dcxo24M", "pll-peri0-400m", "pll-peri0-300m", "pll-peri1-400m", "pll-peri1-300m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc1_clk, "smhc1",
                                           smhc1_parents, 0x0834,
                                           0, 5,	/* M */
                                           8, 5,	/* N */
			                   24, 3,	/* mux */
					   BIT(31), 0);

static const char * const smhc2_parents[] = { "dcxo24M", "pll-peri0-800m", "pll-peri0-600m", "pll-peri1-800m", "pll-peri1-600m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc2_clk, "smhc2",
                                           smhc2_parents, 0x0838,
                                           0, 5,	/* M */
                                           8, 5,	/* N */
			                   24, 3,	/* mux */
					   BIT(31), 0);

static SUNXI_CCU_GATE(bus_smhc2_clk, "bus-smhc2",
                      "dcxo24M",
                      0x084C, BIT(2), 0);

static SUNXI_CCU_GATE(bus_smhc1_clk, "bus-smhc1",
                      "dcxo24M",
                      0x084C, BIT(1), 0);

static SUNXI_CCU_GATE(bus_smhc0_clk, "bus-smhc0",
                      "dcxo24M",
                      0x084C, BIT(0), 0);

static SUNXI_CCU_GATE(sysdap_clk, "sysdap",
                      "dcxo24M",
                      0x088C, BIT(0), 0);

static SUNXI_CCU_GATE(uart7_clk, "uart7",
                      "dcxo24M",
                      0x090C, BIT(7), 0);

static SUNXI_CCU_GATE(uart6_clk, "uart6",
                      "dcxo24M",
                      0x090C, BIT(6), 0);

static SUNXI_CCU_GATE(uart5_clk, "uart5",
                      "dcxo24M",
                      0x090C, BIT(5), 0);

static SUNXI_CCU_GATE(uart4_clk, "uart4",
                      "dcxo24M",
                      0x090C, BIT(4), 0);

static SUNXI_CCU_GATE(uart3_clk, "uart3",
                      "dcxo24M",
                      0x090C, BIT(3), 0);

static SUNXI_CCU_GATE(uart2_clk, "uart2",
                      "dcxo24M",
                      0x090C, BIT(2), 0);

static SUNXI_CCU_GATE(bus_uart1_clk, "bus-uart1",
                      "dcxo24M",
                      0x090C, BIT(1), 0);

static SUNXI_CCU_GATE(bus_uart0_clk, "bus-uart0",
                      "dcxo24M",
                      0x090C, BIT(0), 0);

static SUNXI_CCU_GATE(twi5_clk, "twi5",
                      "dcxo24M",
                      0x091C, BIT(5), 0);

static SUNXI_CCU_GATE(twi4_clk, "twi4",
                      "dcxo24M",
                      0x091C, BIT(4), 0);

static SUNXI_CCU_GATE(twi3_clk, "twi3",
                      "dcxo24M",
                      0x091C, BIT(3), 0);

static SUNXI_CCU_GATE(twi2_clk, "twi2",
                      "dcxo24M",
                      0x091C, BIT(2), 0);

static SUNXI_CCU_GATE(twi1_clk, "twi1",
                      "dcxo24M",
                      0x091C, BIT(1), 0);

static SUNXI_CCU_GATE(twi0_clk, "twi0",
                      "dcxo24M",
                      0x091C, BIT(0), 0);

static const char * const spi0_parents[] = { "dcxo24M", "pll-peri0-300m", "pll-peri0-200m", "pll-peri1-300m", "pll-peri1-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(spi0_clk, "spi0",
                                           spi0_parents, 0x0940,
                                           0, 5,	/* M */
			                   24, 3,	/* mux */
					   BIT(31), 0);

static const char * const spi1_parents[] = { "dcxo24M", "pll-peri0-300m", "pll-peri0-200m", "pll-peri1-300m", "pll-peri1-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(spi1_clk, "spi1",
                                           spi1_parents, 0x0944,
                                           0, 5,	/* M */
			                   24, 3,	/* mux */
					   BIT(31), 0);

static const char * const spi2_parents[] = { "dcxo24M", "pll-peri0-300m", "pll-peri0-200m", "pll-peri1-300m", "pll-peri1-200m" };

static SUNXI_CCU_M_WITH_MUX_GATE(spi2_clk, "spi2",
                                           spi2_parents, 0x0948,
                                           0, 5,	/* M */
			                   24, 3,	/* mux */
					   BIT(31), 0);

static const char * const spif_parents[] = { "dcxo24M", "pll-peri0-200m", "pll-peri0-300m", "pll-peri1-200m", "pll-peri1-300m" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(spif_clk, "spif",
                                           spif_parents, 0x0950,
                                           0, 5,	/* M */
                                           8, 5,	/* N */
			                   24, 3,	/* mux */
					   BIT(31), 0);

static SUNXI_CCU_GATE(bus_spif_clk, "bus-spif",
                      "dcxo24M",
                      0x096C, BIT(3), 0);

static SUNXI_CCU_GATE(bus_spi2_clk, "bus-spi2",
                      "dcxo24M",
                      0x096C, BIT(2), 0);

static SUNXI_CCU_GATE(bus_spi1_clk, "bus-spi1",
                      "dcxo24M",
                      0x096C, BIT(1), 0);

static SUNXI_CCU_GATE(bus_spi0_clk, "bus-spi0",
                      "dcxo24M",
                      0x096C, BIT(0), 0);

static SUNXI_CCU_GATE(gmac0_25m_clk, "gmac0-25m",
                      "dcxo24M",
                      0x0970, BIT(31), 0);

static SUNXI_CCU_GATE(gmac0_25m_clk_src_clk, "gmac0-25m-clk-src",
                      "dcxo24M",
                      0x0970, BIT(30), 0);

static SUNXI_CCU_GATE(gmac1_25m_clk, "gmac1-25m",
                      "dcxo24M",
                      0x0974, BIT(31), 0);

static SUNXI_CCU_GATE(gmac1_25m_clk_src_clk, "gmac1-25m-clk-src",
                      "dcxo24M",
                      0x0974, BIT(30), 0);

static SUNXI_CCU_GATE(gmac1_clk, "gmac1",
                      "dcxo24M",
                      0x097C, BIT(1), 0);

static SUNXI_CCU_GATE(gmac0_clk, "gmac0",
                      "dcxo24M",
                      0x097C, BIT(0), 0);

static const char * const irrx_parents[] = { "osc32k", "dcxo24M" };

static SUNXI_CCU_M_WITH_MUX_GATE(irrx_clk, "irrx",
                                 irrx_parents, 0x0990,
			         0, 5,	/* M */
			         24, 1,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(bus_irrx_clk, "bus-irrx",
                      "dcxo24M",
                      0x099C, BIT(0), 0);

static const char * const irtx_parents[] = { "dcxo24M", "pll-peri1-600m" };

static SUNXI_CCU_M_WITH_MUX_GATE(irtx_clk, "irtx",
                                 irtx_parents, 0x09C0,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(bus_irtx_clk, "bus-irtx",
                      "dcxo24M",
                      0x09CC, BIT(0), 0);

static SUNXI_CCU_GATE(bus_gpadc0_clk, "bus-gpadc0",
                      "dcxo24M",
                      0x09E0, BIT(31), 0);

static SUNXI_CCU_GATE(bus_gpadc1_clk, "bus-gpadc1",
                      "dcxo24M",
                      0x09EC, BIT(0), 0);

static SUNXI_CCU_GATE(ths_clk, "ths",
                      "dcxo24M",
                      0x09FC, BIT(0), 0);

static SUNXI_CCU_GATE(usb_clk, "usb",
                      "dcxo24M",
                      0x0A70, BIT(31), 0);

static CLK_FIXED_FACTOR(dcxo12M_clk, "dcxo12M", "dcxo24M", 2, 1, 0);

#define SUN60IW1_USB0_CTRL_REG   0x0A70
static const char * const usb_parents[] = { "pll-audio0-div-48m", "dcxo12M", "osc32k", "iosc" };

static SUNXI_CCU_MUX_WITH_GATE(usb0_clk, "usb0", usb_parents, 0x0A70,
			       24, 2, BIT(31), 0);

#define SUN60IW1_USB1_CTRL_REG   0x0A74
static SUNXI_CCU_MUX_WITH_GATE(usb1_clk, "usb1", usb_parents, 0x0A74,
			       24, 2, BIT(31), 0);

static SUNXI_CCU_GATE(usbotg0_clk, "usbotg0",
                      "dcxo24M",
                      0x0A8C, BIT(8), 0);

static SUNXI_CCU_GATE(usbehci1_clk, "usbehci1",
                      "dcxo24M",
                      0x0A8C, BIT(5), 0);

static SUNXI_CCU_GATE(usbehci0_clk, "usbehci0",
                      "dcxo24M",
                      0x0A8C, BIT(4), 0);

static SUNXI_CCU_GATE(usbohci1_clk, "usbohci1",
                      "dcxo24M",
                      0x0A8C, BIT(1), 0);

static SUNXI_CCU_GATE(usbohci0_clk, "usbohci0",
                      "dcxo24M",
                      0x0A8C, BIT(0), 0);

static SUNXI_CCU_GATE(lradc_clk, "lradc",
                      "dcxo24M",
                      0x0A9C, BIT(0), 0);

static const char * const pcie_ref_alt_parents[] = { "dcxo24M", "osc32k" };

static SUNXI_CCU_M_WITH_MUX_GATE(pcie_ref_alt_clk, "pcie-ref-alt",
                                 pcie_ref_alt_parents, 0x0AA0,
			         0, 5,	/* M */
			         24, 1,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(pcie_ref_clk, "pcie-ref",
                      "dcxo24M",
                      0x0AA4, BIT(31), 0);

static SUNXI_CCU_GATE(pcie_clk, "pcie",
                      "dcxo24M",
                      0x0AAC, BIT(0), 0);

static SUNXI_CCU_GATE(dpss_top0_clk, "dpss-top0",
                      "dcxo24M",
                      0x0ABC, BIT(0), 0);

static SUNXI_CCU_GATE(dpss_top1_clk, "dpss-top1",
                      "dcxo24M",
                      0x0ACC, BIT(0), 0);

static SUNXI_CCU_GATE(hdmi_24m_clk, "hdmi-24m",
                      "dcxo24M",
                      0x0B04, BIT(31), 0);

static const char * const hdmi_cec_parents[] = { "osc32k", "hdmi-cec-osc32k" };

static SUNXI_CCU_MUX_WITH_GATE(hdmi_cec_clk, "hdmi-cec",
                               hdmi_cec_parents, 0x0B10,
			       24, 1,	/* mux */
			       BIT(31), 0);

static SUNXI_CCU_GATE(hdmi_clk, "hdmi",
                      "dcxo24M",
                      0x0B1C, BIT(0), 0);

static const char * const dsi0_parents[] = { "dcxo24M", "pll-peri0-200m", "pll-peri0-150m" };

static SUNXI_CCU_M_WITH_MUX_GATE(dsi0_clk, "dsi0",
                                 dsi0_parents, 0x0B24,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static const char * const dsi1_parents[] = { "dcxo24M", "pll-peri0-200m", "pll-peri0-150m" };

static SUNXI_CCU_M_WITH_MUX_GATE(dsi1_clk, "dsi1",
                                 dsi1_parents, 0x0B28,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(bus_dsi1_clk, "bus-dsi1",
                      "dcxo24M",
                      0x0B4C, BIT(1), 0);

static SUNXI_CCU_GATE(bus_dsi0_clk, "bus-dsi0",
                      "dcxo24M",
                      0x0B4C, BIT(0), 0);

static const char * const vo0_tconlcd0_parents[] = { "pll-video0-4x", "pll-video1-4x", "pll-video2-4x", "pll-video3-4x", "pll-peri0-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(vo0_tconlcd0_clk, "vo0-tconlcd0",
                                 vo0_tconlcd0_parents, 0x0B60,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static const char * const vo0_tconlcd1_parents[] = { "pll-video0-4x", "pll-video1-4x", "pll-video2-4x", "pll-video3-4x", "pll-peri0-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(vo0_tconlcd1_clk, "vo0-tconlcd1",
                                 vo0_tconlcd1_parents, 0x0B64,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static const char * const vo1_tconlcd0_parents[] = { "pll-video0-4x", "pll-video1-4x", "pll-video2-4x", "pll-video3-4x", "pll-peri0-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(vo1_tconlcd0_clk, "vo1-tconlcd0",
                                 vo1_tconlcd0_parents, 0x0B68,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static const char * const combphy0_parents[] = { "pll-video0-4x", "pll-video1-4x", "pll-video2-4x", "pll-video3-4x", "pll-peri0-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(combphy0_clk, "combphy0",
                                 combphy0_parents, 0x0B6C,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static const char * const combphy1_parents[] = { "pll-video0-4x", "pll-video1-4x", "pll-video2-4x", "pll-video3-4x", "pll-peri0-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(combphy1_clk, "combphy1",
                                 combphy1_parents, 0x0B70,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(bus_vo1_tconlcd0_clk, "bus-vo1-tconlcd0",
                      "dcxo24M",
                      0x0B7C, BIT(2), 0);

static SUNXI_CCU_GATE(bus_vo0_tconlcd1_clk, "bus-vo0-tconlcd1",
                      "dcxo24M",
                      0x0B7C, BIT(1), 0);

static SUNXI_CCU_GATE(bus_vo0_tconlcd0_clk, "bus-vo0-tconlcd0",
                      "dcxo24M",
                      0x0B7C, BIT(0), 0);

static const char * const tcontv_parents[] = { "pll-video0-4x", "pll-video1-4x", "pll-video2-4x", "pll-video3-4x", "pll-peri0-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(tcontv_clk, "tcontv",
                                 tcontv_parents, 0x0B80,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static const char * const tcontv1_parents[] = { "pll-video0-4x", "pll-video1-4x", "pll-video2-4x", "pll-video3-4x", "pll-peri0-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(tcontv1_clk, "tcontv1",
                                 tcontv1_parents, 0x0B84,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(bus_tcontv1_clk, "bus-tcontv1",
                      "dcxo24M",
                      0x0B9C, BIT(1), 0);

static SUNXI_CCU_GATE(bus_tcontv_clk, "bus-tcontv",
                      "dcxo24M",
                      0x0B9C, BIT(0), 0);

static const char * const ledc_parents[] = { "dcxo24M", "pll-peri0-600m" };

static SUNXI_CCU_M_WITH_MUX_GATE(ledc_clk, "ledc",
                                           ledc_parents, 0x0BF0,
                                           0, 5,	/* M */
			                   24, 1,	/* mux */
					   BIT(31), 0);

static const char * const edp_parents[] = { "pll-video0-4x", "pll-vidio1-4x", "pll-vidio2-4x", "pll-vidio3-4x", "pll-peri0-2x" };

static SUNXI_CCU_M_WITH_MUX_GATE(edp_clk, "edp",
                                 edp_parents, 0x0BB0,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(bus_ledc_clk, "bus-ledc",
                      "dcxo24M",
                      0x0BFC, BIT(0), 0);

static const char * const csi_parents[] = { "pll-peri0-300m", "pll-peri0-400m", "pll-peri0-480m", "pll-video0-4x", "pll-video3-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(csi_clk, "csi",
                                 csi_parents, 0x0C04,
				 0, 5, /* m */
			         24, 3,	/* mux */
				 BIT(31), 0);

static const char * const csi_master0_parents[] = { "dcxo24M", "pll-video3-4x", "pll-video0-4x", "pll-video1-4x", "pll-video2-4x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master0_clk, "csi-master0",
                                           csi_master0_parents, 0x0C08,
                                           0, 5,	/* M */
                                           8, 5,	/* N */
			                   24, 3,	/* mux */
					   BIT(31), 0);

static const char * const csi_master1_parents[] = { "dcxo24M", "pll-video3-4x", "pll-video0-4x", "pll-video1-4x", "pll-video2-4x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master1_clk, "csi-master1",
                                           csi_master1_parents, 0x0C0C,
                                           0, 5,	/* M */
                                           8, 5,	/* N */
			                   24, 3,	/* mux */
					   BIT(31), 0);

static const char * const csi_master2_parents[] = { "dcxo24M", "pll-video3-4x", "pll-video0-4x", "pll-video1-4x", "pll-video2-4x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master2_clk, "csi-master2",
                                           csi_master2_parents, 0x0C10,
                                           0, 5,	/* M */
                                           8, 5,	/* N */
			                   24, 3,	/* mux */
					   BIT(31), 0);

static const char * const csi_master3_parents[] = { "dcxo24M", "pll-video3-4x", "pll-video0-4x", "pll-video1-4x", "pll-video2-4x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(csi_master3_clk, "csi-master3",
                                           csi_master3_parents, 0x0C14,
                                           0, 5,	/* M */
                                           8, 5,	/* N */
			                   24, 3,	/* mux */
					   BIT(31), 0);

static SUNXI_CCU_GATE(bus_csi_clk, "bus-csi",
                      "dcxo24M",
                      0x0C1C, BIT(0), 0);

static const char * const isp_parents[] = { "pll-peri0-300m", "pll-peri0-400m", "pll-video0-4x", "pll-video3-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(isp_clk, "isp",
                                 isp_parents, 0x0C20,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static const char * const dsp_parents[] = { "dcxo24M", "osc32k", "iosc", "pll-peri0-2x", "pll-peri0-480m" };

static SUNXI_CCU_M_WITH_MUX_GATE(dsp_clk, "dsp",
                                 dsp_parents, 0x0C70,
			         0, 5,	/* M */
			         24, 3,	/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(cpus_hclk_gate_clk, "cpus-hclk-gate",
                      "dcxo24M",
                      0x0E04, BIT(28), 0);

static SUNXI_CCU_GATE(spif_mbus_ahb_gate_clk, "spif-mbus-ahb-gate",
                      "dcxo24M",
                      0x0E04, BIT(22), 0);

static SUNXI_CCU_GATE(gmac1_mbus_ahb_gate_clk, "gmac1-mbus-ahb-gate",
                      "dcxo24M",
                      0x0E04, BIT(21), 0);

static SUNXI_CCU_GATE(gmac0_mbus_ahb_gate_clk, "gmac0-mbus-ahb-gate",
                      "dcxo24M",
                      0x0E04, BIT(20), 0);

static SUNXI_CCU_GATE(smhc2_mbus_ahb_gate_clk, "smhc2-mbus-ahb-gate",
                      "dcxo24M",
                      0x0E04, BIT(19), 0);

static SUNXI_CCU_GATE(smhc1_mbus_ahb_gate_clk, "smhc1-mbus-ahb-gate",
                      "dcxo24M",
                      0x0E04, BIT(18), 0);

static SUNXI_CCU_GATE(smhc0_mbus_ahb_gate_clk, "smhc0-mbus-ahb-gate",
                      "dcxo24M",
                      0x0E04, BIT(17), 0);

static SUNXI_CCU_GATE(usb_mbus_ahb_gate_clk, "usb-mbus-ahb-gate",
                      "dcxo24M",
                      0x0E04, BIT(16), 0);

static SUNXI_CCU_GATE(gmac1_ahb_gate_clk, "gmac1-ahb-gate",
                      "dcxo24M",
                      0x0E04, BIT(9), 0);

static SUNXI_CCU_GATE(gmac0_ahb_gate_clk, "gmac0-ahb-gate",
                      "dcxo24M",
                      0x0E04, BIT(8), 0);

static SUNXI_CCU_GATE(smhc2_ahb_gate_clk, "smhc2-ahb-gate",
                      "dcxo24M",
                      0x0E04, BIT(7), 0);

static SUNXI_CCU_GATE(smhc1_ahb_gate_clk, "smhc1-ahb-gate",
                      "dcxo24M",
                      0x0E04, BIT(6), 0);

static SUNXI_CCU_GATE(smhc0_ahb_gate_clk, "smhc0-ahb-gate",
                      "dcxo24M",
                      0x0E04, BIT(5), 0);

static SUNXI_CCU_GATE(usb_ahb_gate_clk, "usb-ahb-gate",
                      "dcxo24M",
                      0x0E04, BIT(4), 0);

static SUNXI_CCU_GATE(vid_out_ahb_gate_clk, "vid-out-ahb-gate",
                      "dcxo24M",
                      0x0E04, BIT(3), 0);

static SUNXI_CCU_GATE(vid_in_ahb_gate_clk, "vid-in-ahb-gate",
                      "dcxo24M",
                      0x0E04, BIT(2), 0);

static SUNXI_CCU_GATE(ve_ahb_gate_clk, "ve-ahb-gate",
                      "dcxo24M",
                      0x0E04, BIT(1), 0);

static SUNXI_CCU_GATE(npu_ahb_gate_clk, "npu-ahb-gate",
                      "dcxo24M",
                      0x0E04, BIT(0), 0);

static SUNXI_CCU_GATE(res_dcap_24m_clk, "res-dcap-24m",
                      "dcxo24M",
                      0x0E0C, BIT(3), 0);

static SUNXI_CCU_GATE(usb_24m_clk, "usb-24m",
                      "dcxo24M",
                      0x0E0C, BIT(0), 0);

static SUNXI_CCU_GATE(fanout_25m_clk, "fanout-25m",
                      "dcxo24M",
                      0x0F30, BIT(3), 0);

static SUNXI_CCU_GATE(fanout_16m_clk, "fanout-16m",
                      "dcxo24M",
                      0x0F30, BIT(2), 0);

static SUNXI_CCU_GATE(fanout_12m_clk, "fanout-12m",
                      "dcxo24M",
                      0x0F30, BIT(1), 0);

static SUNXI_CCU_GATE(fanout_24m_clk, "fanout-24m",
                      "dcxo24M",
                      0x0F30, BIT(0), 0);

static const char * const clk27m_fanout_parents[] = { "pll-video0-1x", "pll-video1-1x", "pll-video2-1x","pll-video3-1x" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(clk27m_fanout_clk, "clk27m_fanout",
                                           clk27m_fanout_parents, 0x0F34,
                                           0, 5,	/* M */
                                           8, 5,	/* N */
			                   24, 2,	/* mux */
					   BIT(31), 0);

static const char * const clk_fanout_parents[] = { "apb0" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(clk_fanout_clk, "clk-fanout",
                                           clk_fanout_parents, 0x0F38,
                                           0, 5,	/* M */
                                           5, 5,	/* N */
			                   24, 2,	/* mux */
					   BIT(31), 0);

static const char * const fanout_clk_parents[] = { "rtc-32k-fanout", "osc32k", "dcxo-div-12m", "pll-peri0-16m", "dcxo24M", "pll-peri0-25m", "clk27m-fanout", "clk-fanout" };

static SUNXI_CCU_MUX(fanout2_clk, "fanout2", fanout_clk_parents,
		     0x0F3C, 6, 3, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static SUNXI_CCU_MUX(fanout1_clk, "fanout1", fanout_clk_parents,
		     0x0F3C, 3, 3, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static SUNXI_CCU_MUX(fanout0_clk, "fanout0", fanout_clk_parents,
		     0x0F3C, 0, 3, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);


/* rst_def_start */
static struct ccu_reset_map SUN60IW1_ccu_resets[] =
{
	 [RST_MBUS]			= { 0x0540, BIT(30) },
	 [RST_BUS_NSI]			= { 0x054c, BIT(16) },
	 [RST_BUS_DE0]			= { 0x060c, BIT(16) },
	 [RST_BUS_DI]			= { 0x062c, BIT(16) },
	 [RST_BUS_G2D]			= { 0x063c, BIT(16) },
	 [RST_BUS_GPU]			= { 0x067c, BIT(16) },
	 [RST_BUS_CE_SY]		= { 0x068c, BIT(17) },
	 [RST_BUS_CE]			= { 0x068c, BIT(16) },
	 [RST_BUS_VE]			= { 0x069c, BIT(16) },
	 [RST_BUS_DMA]			= { 0x070c, BIT(16) },
	 [RST_BUS_MSGBOX1]		= { 0x071c, BIT(17) },
	 [RST_BUS_MSGBOX0]		= { 0x071c, BIT(16) },
	 [RST_BUS_SPINLOCK]		= { 0x072c, BIT(16) },
	 [RST_BUS_TIME]			= { 0x074c, BIT(16) },
	 [RST_BUS_DBGSY]		= { 0x078c, BIT(16) },
	 [RST_BUS_PWM1]			= { 0x07ac, BIT(17) },
	 [RST_BUS_PWM]			= { 0x07ac, BIT(16) },
	 [RST_BUS_DRAM]			= { 0x080c, BIT(16) },
	 [RST_BUS_NAND0]		= { 0x082c, BIT(16) },
	 [RST_BUS_SMHC2]		= { 0x084c, BIT(18) },
	 [RST_BUS_SMHC1]		= { 0x084c, BIT(17) },
	 [RST_BUS_SMHC0]		= { 0x084c, BIT(16) },
	 [RST_BUS_SYSDAP]		= { 0x088c, BIT(16) },
	 [RST_BUS_UART7]		= { 0x090c, BIT(23) },
	 [RST_BUS_UART6]		= { 0x090c, BIT(22) },
	 [RST_BUS_UART5]		= { 0x090c, BIT(21) },
	 [RST_BUS_UART4]		= { 0x090c, BIT(20) },
	 [RST_BUS_UART3]		= { 0x090c, BIT(19) },
	 [RST_BUS_UART2]		= { 0x090c, BIT(18) },
	 [RST_BUS_UART1]		= { 0x090c, BIT(17) },
	 [RST_BUS_UART0]		= { 0x090c, BIT(16) },
	 [RST_BUS_TWI5]			= { 0x091c, BIT(21) },
	 [RST_BUS_TWI4]			= { 0x091c, BIT(20) },
	 [RST_BUS_TWI3]			= { 0x091c, BIT(19) },
	 [RST_BUS_TWI2]			= { 0x091c, BIT(18) },
	 [RST_BUS_TWI1]			= { 0x091c, BIT(17) },
	 [RST_BUS_TWI0]			= { 0x091c, BIT(16) },
	 [RST_BUS_SPIF]			= { 0x096c, BIT(19) },
	 [RST_BUS_SPI2]			= { 0x096c, BIT(18) },
	 [RST_BUS_SPI1]			= { 0x096c, BIT(17) },
	 [RST_BUS_SPI0]			= { 0x096c, BIT(16) },
	 [RST_BUS_GMAC1]		= { 0x097c, BIT(17) },
	 [RST_BUS_GMAC0]		= { 0x097c, BIT(16) },
	 [RST_BUS_IRRX]			= { 0x099c, BIT(16) },
	 [RST_BUS_IRTX]			= { 0x09cc, BIT(16) },
	 [RST_BUS_GPADC1]		= { 0x09ec, BIT(17) },
	 [RST_BUS_GPADC0]		= { 0x09ec, BIT(16) },
	 [RST_BUS_TH]			= { 0x09fc, BIT(16) },
	 [RST_USB_PHY0_RSTN]		= { 0x0a70, BIT(30) },
	 [RST_USB_PHY1_RSTN]		= { 0x0a74, BIT(30) },
	 [RST_USB_2_PHY]		= { 0x0a8c, BIT(26) },
	 [RST_USB_2]			= { 0x0a8c, BIT(25) },
	 [RST_USB_OTG0]			= { 0x0a8c, BIT(24) },
	 [RST_USB_EHCI1]		= { 0x0a8c, BIT(21) },
	 [RST_USB_EHCI0]		= { 0x0a8c, BIT(20) },
	 [RST_USB_OHCI1]		= { 0x0a8c, BIT(17) },
	 [RST_USB_OHCI0]		= { 0x0a8c, BIT(16) },
	 [RST_BUS_LRADC]		= { 0x0a9c, BIT(16) },
	 [RST_BUS_PCIE_PE]		= { 0x0aac, BIT(18) },
	 [RST_BUS_PCIE_POWER_UP]	= { 0x0aac, BIT(17) },
	 [RST_BUS_PCIE]			= { 0x0aac, BIT(16) },
	 [RST_BUS_DPSS_TOP0]		= { 0x0abc, BIT(16) },
	 [RST_BUS_DPSS_TOP1]		= { 0x0acc, BIT(16) },
	 [RST_BUS_HDMI_SUB]		= { 0x0b1c, BIT(17) },
	 [RST_BUS_HDMI_MAIN]		= { 0x0b1c, BIT(16) },
	 [RST_BUS_DSI1]			= { 0x0b4c, BIT(17) },
	 [RST_BUS_DSI0]			= { 0x0b4c, BIT(16) },
	 [RST_BUS_VO1_TCONLCD0]		= { 0x0b7c, BIT(18) },
	 [RST_BUS_VO0_TCONLCD1]		= { 0x0b7c, BIT(17) },
	 [RST_BUS_VO0_TCONLCD0]		= { 0x0b7c, BIT(16) },
	 [RST_BUS_TCONTV1]		= { 0x0b9c, BIT(17) },
	 [RST_BUS_TCONTV]		= { 0x0b9c, BIT(16) },
	 [RST_BUS_LVDS1]		= { 0x0bac, BIT(17) },
	 [RST_BUS_LVDS0]		= { 0x0bac, BIT(16) },
	 [RST_BUS_LEDC]			= { 0x0bfc, BIT(16) },
	 [RST_BUS_CSI]			= { 0x0c1c, BIT(16) },
	 [RST_BUS_ISP]			= { 0x0c2c, BIT(16) },
	};
	/* rst_def_end */

static struct ccu_common *SUN60IW1_ccu_clks[] =
{
	&pll_cpu0_clk.common,
	&pll_ddr_clk.common,
	&pll_peri0_parent_clk.common,
	&pll_peri0_2x_clk.common,
	&pll_peri0_800m_clk.common,
	&pll_peri0_480m_clk.common,
	&pll_peri1_parent_clk.common,
	&pll_peri1_2x_clk.common,
	&pll_peri1_800m_clk.common,
	&pll_peri1_480m_clk.common,
	&pll_gpu_clk.common,
	&pll_video0_4x_clk.common,
	&pll_video1_4x_clk.common,
	&pll_video2_4x_clk.common,
	&pll_ve_clk.common,
	&pll_audio0_4x_clk.common,
	&cpu0_div_clk.common,
	&cpu_clk.common,
	&cpu_axi_clk.common,
	&cpu_apb_clk.common,
	&cpu_peri_clk.common,
	&dsu_clk.common,
	&trace_clk.common,
	&cpu1_div_clk.common,
	&dsu_parents_clk.common,
	&ahb_clk.common,
	&apb0_clk.common,
	&apb1_clk.common,
	&mbus_clk.common,
	&nsi_clk.common,
	&gic_clk.common,
	&de_clk.common,
	&de0_clk.common,
	&di_clk.common,
	&bus_di_clk.common,
	&g2d_clk.common,
	&bus_g2d_clk.common,
	&gpu_clk.common,
	&bus_gpu_clk.common,
	&ce_clk.common,
	&ce_sys_clk.common,
	&bus_ce_clk.common,
	&ve_clk.common,
	&bus_ve_clk.common,
	&npu_clk.common,
	&dma_clk.common,
	&msgbox1_clk.common,
	&msgbox0_clk.common,
	&spinlock_clk.common,
	&timer_clk.common,
	&dbgsys_clk.common,
	&pwm1_clk.common,
	&pwm_clk.common,
	&iommu_clk.common,
	&timer0_clk.common,
	&timer1_clk.common,
	&timer2_clk.common,
	&timer3_clk.common,
	&timer4_clk.common,
	&timer5_clk.common,
	&dram_clk.common,
	&usb_mbus_gate_clk.common,
	&gpu_mbus_gate_clk.common,
	&de_mbus_gate_clk.common,
	&nand_mbus_gate_clk.common,
	&npu_mbus_gate_clk.common,
	&vid_in_mbus_gate_clk.common,
	&vid_out_mbus_gate_clk.common,
	&ce_mbus_gate_clk.common,
	&ve_mbus_gate_clk.common,
	&dma_mbus_gate_clk.common,
	&bus_dram_clk.common,
	&nand0_clk.common,
	&nand0_clk0_clk.common,
	&nand0_clk.common,
	&nand0_clk.common,
	&nand0_clk1_clk.common,
	&nand0_clk.common,
	&nand0_clk.common,
	&smhc0_clk.common,
	&smhc1_clk.common,
	&smhc2_clk.common,
	&bus_smhc2_clk.common,
	&bus_smhc1_clk.common,
	&bus_smhc0_clk.common,
	&sysdap_clk.common,
	&uart7_clk.common,
	&uart6_clk.common,
	&uart5_clk.common,
	&uart4_clk.common,
	&uart3_clk.common,
	&uart2_clk.common,
	&bus_uart1_clk.common,
	&bus_uart0_clk.common,
	&twi5_clk.common,
	&twi4_clk.common,
	&twi3_clk.common,
	&twi2_clk.common,
	&twi1_clk.common,
	&twi0_clk.common,
	&spi0_clk.common,
	&spi1_clk.common,
	&spi2_clk.common,
	&spif_clk.common,
	&bus_spif_clk.common,
	&bus_spi2_clk.common,
	&bus_spi1_clk.common,
	&bus_spi0_clk.common,
	&gmac0_25m_clk.common,
	&gmac0_25m_clk_src_clk.common,
	&gmac1_25m_clk.common,
	&gmac1_25m_clk_src_clk.common,
	&gmac1_clk.common,
	&gmac0_clk.common,
	&irrx_clk.common,
	&bus_irrx_clk.common,
	&irtx_clk.common,
	&bus_irtx_clk.common,
	&bus_gpadc0_clk.common,
	&bus_gpadc1_clk.common,
	&ths_clk.common,
	&usb_clk.common,
	&usb0_clk.common,
	&usb1_clk.common,
	&usbotg0_clk.common,
	&usbehci1_clk.common,
	&usbehci0_clk.common,
	&usbohci1_clk.common,
	&usbohci0_clk.common,
	&lradc_clk.common,
	&pcie_ref_alt_clk.common,
	&pcie_ref_clk.common,
	&pcie_clk.common,
	&dpss_top0_clk.common,
	&dpss_top1_clk.common,
	&hdmi_24m_clk.common,
	&hdmi_cec_clk.common,
	&hdmi_clk.common,
	&dsi0_clk.common,
	&dsi1_clk.common,
	&bus_dsi1_clk.common,
	&bus_dsi0_clk.common,
	&vo0_tconlcd0_clk.common,
	&vo0_tconlcd1_clk.common,
	&vo1_tconlcd0_clk.common,
	&combphy0_clk.common,
	&combphy1_clk.common,
	&bus_vo1_tconlcd0_clk.common,
	&bus_vo0_tconlcd1_clk.common,
	&bus_vo0_tconlcd0_clk.common,
	&tcontv_clk.common,
	&tcontv1_clk.common,
	&bus_tcontv1_clk.common,
	&bus_tcontv_clk.common,
	&ledc_clk.common,
	&edp_clk.common,
	&bus_ledc_clk.common,
	&csi_clk.common,
	&csi_master0_clk.common,
	&csi_master1_clk.common,
	&csi_master2_clk.common,
	&csi_master3_clk.common,
	&bus_csi_clk.common,
	&isp_clk.common,
	&dsp_clk.common,
	&cpus_hclk_gate_clk.common,
	&spif_mbus_ahb_gate_clk.common,
	&gmac1_mbus_ahb_gate_clk.common,
	&gmac0_mbus_ahb_gate_clk.common,
	&smhc2_mbus_ahb_gate_clk.common,
	&smhc1_mbus_ahb_gate_clk.common,
	&smhc0_mbus_ahb_gate_clk.common,
	&usb_mbus_ahb_gate_clk.common,
	&gmac1_ahb_gate_clk.common,
	&gmac0_ahb_gate_clk.common,
	&smhc2_ahb_gate_clk.common,
	&smhc1_ahb_gate_clk.common,
	&smhc0_ahb_gate_clk.common,
	&usb_ahb_gate_clk.common,
	&vid_out_ahb_gate_clk.common,
	&vid_in_ahb_gate_clk.common,
	&ve_ahb_gate_clk.common,
	&npu_ahb_gate_clk.common,
	&res_dcap_24m_clk.common,
	&usb_24m_clk.common,
	&fanout_25m_clk.common,
	&fanout_16m_clk.common,
	&fanout_12m_clk.common,
	&fanout_24m_clk.common,
	&clk27m_fanout_clk.common,
	&clk_fanout_clk.common,
	&fanout2_clk.common,
	&fanout2_clk.common,
	&fanout1_clk.common,
	&fanout1_clk.common,
	&fanout0_clk.common,
	&fanout0_clk.common,
};
/* ccu_def_start */

static struct clk_hw_onecell_data SUN60IW1_hw_clks =
{
    .hws    = {
		[CLK_PLL_CPU0]			= &pll_cpu0_clk.common.hw,
		[CLK_PLL_DDR]			= &pll_ddr_clk.common.hw,
		[CLK_PLL_PERI0_PARENT]		= &pll_peri0_parent_clk.common.hw,
		[CLK_PLL_PERI0_2X]		= &pll_peri0_2x_clk.common.hw,
		[CLK_PERI0_DIV3]		= &pll_peri0_div3_clk.hw,
		[CLK_PLL_PERI0_800M]		= &pll_peri0_800m_clk.common.hw,
		[CLK_PLL_PERI0_480M]		= &pll_peri0_480m_clk.common.hw,
		[CLK_PLL_PERI0_600M]		= &pll_peri0_600m_clk.hw,
		[CLK_PLL_PERI0_400M]		= &pll_peri0_400m_clk.hw,
		[CLK_PLL_PERI0_300M]		= &pll_peri0_300m_clk.hw,
		[CLK_PLL_PERI0_200M]		= &pll_peri0_200m_clk.hw,
		[CLK_PLL_PERI0_160M]		= &pll_peri0_160m_clk.hw,
		[CLK_PLL_PERI0_16M]		= &pll_peri0_16m_clk.hw,
		[CLK_PLL_PERI0_150M]		= &pll_peri0_150m_clk.hw,
		[CLK_PLL_PERI0_25M]		= &pll_peri0_25m_clk.hw,
		[CLK_PLL_PERI1_PARENT]		= &pll_peri1_parent_clk.common.hw,
		[CLK_PLL_PERI1_2X]		= &pll_peri1_2x_clk.common.hw,
		[CLK_PLL_PERI1_800M]		= &pll_peri1_800m_clk.common.hw,
		[CLK_PLL_PERI1_480M]		= &pll_peri1_480m_clk.common.hw,
		[CLK_PLL_PERI1_600M]		= &pll_peri1_600m_clk.hw,
		[CLK_PLL_PERI1_400M]		= &pll_peri1_400m_clk.hw,
		[CLK_PLL_PERI1_300M]		= &pll_peri1_300m_clk.hw,
		[CLK_PLL_PERI1_200M]		= &pll_peri1_200m_clk.hw,
		[CLK_PLL_PERI1_160M]		= &pll_peri1_160m_clk.hw,
		[CLK_PLL_PERI1_150M]		= &pll_peri1_150m_clk.hw,
		[CLK_PLL_GPU]			= &pll_gpu_clk.common.hw,
		[CLK_PLL_VIDEO0_4X]		= &pll_video0_4x_clk.common.hw,
		[CLK_PLL_VIDEO0_2X]		= &pll_video0_2x_clk.hw,
		[CLK_PLL_VIDEO0_1X]		= &pll_video0_1x_clk.hw,
		[CLK_PLL_VIDEO1_4X]		= &pll_video1_4x_clk.common.hw,
		[CLK_PLL_VIDEO1_2X]		= &pll_video1_2x_clk.hw,
		[CLK_PLL_VIDEO1_1X]		= &pll_video1_1x_clk.hw,
		[CLK_PLL_VIDEO2_4X]		= &pll_video2_4x_clk.common.hw,
		[CLK_PLL_VIDEO2_2X]		= &pll_video2_2x_clk.hw,
		[CLK_PLL_VIDEO2_1X]		= &pll_video2_1x_clk.hw,
		[CLK_PLL_VE]			= &pll_ve_clk.common.hw,
		[CLK_PLL_VIDEO3_2X]		= &pll_video3_2x_clk.hw,
		[CLK_PLL_VIDEO3_1X]		= &pll_video3_1x_clk.hw,
		[CLK_PLL_AUDIO0_4X]		= &pll_audio0_4x_clk.common.hw,
		[CLK_PLL_AUDIO0_2X]		= &pll_audio0_2x_clk.hw,
		[CLK_PLL_AUDIO0_1X]		= &pll_audio0_1x_clk.hw,
		[CLK_PLL_AUDIO0_DIV_48M]	= &pll_audio0_div_48m_clk.hw,
		[CLK_PLL_NPU_2X]		= &pll_npu_2x_clk.hw,
		[CLK_PLL_NPU_1X]		= &pll_npu_1x_clk.hw,
		[CLK_CPU0_DIV]			= &cpu0_div_clk.common.hw,
		[CLK_CPU]			= &cpu_clk.common.hw,
		[CLK_CPU_AXI]			= &cpu_axi_clk.common.hw,
		[CLK_CPU_APB]			= &cpu_apb_clk.common.hw,
		[CLK_CPU_PERI]			= &cpu_peri_clk.common.hw,
		[CLK_DSU]			= &dsu_clk.common.hw,
		[CLK_TRACE]			= &trace_clk.common.hw,
		[CLK_CPU1_DIV]			= &cpu1_div_clk.common.hw,
		[CLK_DSU_PARENTS]		= &dsu_parents_clk.common.hw,
		[CLK_AHB]			= &ahb_clk.common.hw,
		[CLK_APB0]			= &apb0_clk.common.hw,
		[CLK_APB1]			= &apb1_clk.common.hw,
		[CLK_MBUS]			= &mbus_clk.common.hw,
		[CLK_NSI]			= &nsi_clk.common.hw,
		[CLK_GIC]			= &gic_clk.common.hw,
		[CLK_DE]			= &de_clk.common.hw,
		[CLK_DE0]			= &de0_clk.common.hw,
		[CLK_DI]			= &di_clk.common.hw,
		[CLK_BUS_DI]			= &bus_di_clk.common.hw,
		[CLK_G2D]			= &g2d_clk.common.hw,
		[CLK_BUS_G2D]			= &bus_g2d_clk.common.hw,
		[CLK_GPU]			= &gpu_clk.common.hw,
		[CLK_BUS_GPU]			= &bus_gpu_clk.common.hw,
		[CLK_CE]			= &ce_clk.common.hw,
		[CLK_CE_SYS]			= &ce_sys_clk.common.hw,
		[CLK_BUS_CE]			= &bus_ce_clk.common.hw,
		[CLK_VE]			= &ve_clk.common.hw,
		[CLK_BUS_VE]			= &bus_ve_clk.common.hw,
		[CLK_NPU]			= &npu_clk.common.hw,
		[CLK_DMA]			= &dma_clk.common.hw,
		[CLK_MSGBOX1]			= &msgbox1_clk.common.hw,
		[CLK_MSGBOX0]			= &msgbox0_clk.common.hw,
		[CLK_SPINLOCK]			= &spinlock_clk.common.hw,
		[CLK_TIMER]			= &timer_clk.common.hw,
		[CLK_DBGSYS]			= &dbgsys_clk.common.hw,
		[CLK_PWM1]			= &pwm1_clk.common.hw,
		[CLK_PWM]			= &pwm_clk.common.hw,
		[CLK_IOMMU]			= &iommu_clk.common.hw,
		[CLK_TIMER0]			= &timer0_clk.common.hw,
		[CLK_TIMER1]			= &timer1_clk.common.hw,
		[CLK_TIMER2]			= &timer2_clk.common.hw,
		[CLK_TIMER3]			= &timer3_clk.common.hw,
		[CLK_TIMER4]			= &timer4_clk.common.hw,
		[CLK_TIMER5]			= &timer5_clk.common.hw,
		[CLK_DRAM]			= &dram_clk.common.hw,
		[CLK_USB_MBUS_GATE]		= &usb_mbus_gate_clk.common.hw,
		[CLK_GPU_MBUS_GATE]		= &gpu_mbus_gate_clk.common.hw,
		[CLK_DE_MBUS_GATE]		= &de_mbus_gate_clk.common.hw,
		[CLK_NAND_MBUS_GATE]		= &nand_mbus_gate_clk.common.hw,
		[CLK_NPU_MBUS_GATE]		= &npu_mbus_gate_clk.common.hw,
		[CLK_VID_IN_MBUS_GATE]		= &vid_in_mbus_gate_clk.common.hw,
		[CLK_VID_OUT_MBUS_GATE]		= &vid_out_mbus_gate_clk.common.hw,
		[CLK_CE_MBUS_GATE]		= &ce_mbus_gate_clk.common.hw,
		[CLK_VE_MBUS_GATE]		= &ve_mbus_gate_clk.common.hw,
		[CLK_DMA_MBUS_GATE]		= &dma_mbus_gate_clk.common.hw,
		[CLK_BUS_DRAM]			= &bus_dram_clk.common.hw,
		[CLK_NAND0]			= &nand0_clk.common.hw,
		[CLK_NAND0_CLK0]		= &nand0_clk0_clk.common.hw,
		[CLK_NAND0_CLK1]		= &nand0_clk1_clk.common.hw,
		[CLK_SMHC0]			= &smhc0_clk.common.hw,
		[CLK_SMHC1]			= &smhc1_clk.common.hw,
		[CLK_SMHC2]			= &smhc2_clk.common.hw,
		[CLK_BUS_SMHC2]			= &bus_smhc2_clk.common.hw,
		[CLK_BUS_SMHC1]			= &bus_smhc1_clk.common.hw,
		[CLK_BUS_SMHC0]			= &bus_smhc0_clk.common.hw,
		[CLK_SYSDAP]			= &sysdap_clk.common.hw,
		[CLK_UART7]			= &uart7_clk.common.hw,
		[CLK_UART6]			= &uart6_clk.common.hw,
		[CLK_UART5]			= &uart5_clk.common.hw,
		[CLK_UART4]			= &uart4_clk.common.hw,
		[CLK_UART3]			= &uart3_clk.common.hw,
		[CLK_UART2]			= &uart2_clk.common.hw,
		[CLK_BUS_UART1]			= &bus_uart1_clk.common.hw,
		[CLK_BUS_UART0]			= &bus_uart0_clk.common.hw,
		[CLK_TWI5]			= &twi5_clk.common.hw,
		[CLK_TWI4]			= &twi4_clk.common.hw,
		[CLK_TWI3]			= &twi3_clk.common.hw,
		[CLK_TWI2]			= &twi2_clk.common.hw,
		[CLK_TWI1]			= &twi1_clk.common.hw,
		[CLK_TWI0]			= &twi0_clk.common.hw,
		[CLK_SPI0]			= &spi0_clk.common.hw,
		[CLK_SPI1]			= &spi1_clk.common.hw,
		[CLK_SPI2]			= &spi2_clk.common.hw,
		[CLK_SPIF]			= &spif_clk.common.hw,
		[CLK_BUS_SPIF]			= &bus_spif_clk.common.hw,
		[CLK_BUS_SPI2]			= &bus_spi2_clk.common.hw,
		[CLK_BUS_SPI1]			= &bus_spi1_clk.common.hw,
		[CLK_BUS_SPI0]			= &bus_spi0_clk.common.hw,
		[CLK_GMAC0_25M]			= &gmac0_25m_clk.common.hw,
		[CLK_GMAC0_25M_CLK_SRC]		= &gmac0_25m_clk_src_clk.common.hw,
		[CLK_GMAC1_25M]			= &gmac1_25m_clk.common.hw,
		[CLK_GMAC1_25M_CLK_SRC]		= &gmac1_25m_clk_src_clk.common.hw,
		[CLK_GMAC1]			= &gmac1_clk.common.hw,
		[CLK_GMAC0]			= &gmac0_clk.common.hw,
		[CLK_IRRX]			= &irrx_clk.common.hw,
		[CLK_BUS_IRRX]			= &bus_irrx_clk.common.hw,
		[CLK_IRTX]			= &irtx_clk.common.hw,
		[CLK_BUS_IRTX]			= &bus_irtx_clk.common.hw,
		[CLK_BUS_GPADC0]		= &bus_gpadc0_clk.common.hw,
		[CLK_BUS_GPADC1]		= &bus_gpadc1_clk.common.hw,
		[CLK_THS]			= &ths_clk.common.hw,
		[CLK_USB]			= &usb_clk.common.hw,
		[CLK_DCXO12M]			= &dcxo12M_clk.hw,
		[CLK_USB0]			= &usb0_clk.common.hw,
		[CLK_USB1]			= &usb1_clk.common.hw,
		[CLK_USBOTG0]			= &usbotg0_clk.common.hw,
		[CLK_USBEHCI1]			= &usbehci1_clk.common.hw,
		[CLK_USBEHCI0]			= &usbehci0_clk.common.hw,
		[CLK_USBOHCI1]			= &usbohci1_clk.common.hw,
		[CLK_USBOHCI0]			= &usbohci0_clk.common.hw,
		[CLK_LRADC]			= &lradc_clk.common.hw,
		[CLK_PCIE_REF_ALT]		= &pcie_ref_alt_clk.common.hw,
		[CLK_PCIE_REF]			= &pcie_ref_clk.common.hw,
		[CLK_PCIE]			= &pcie_clk.common.hw,
		[CLK_DPSS_TOP0]			= &dpss_top0_clk.common.hw,
		[CLK_DPSS_TOP1]			= &dpss_top1_clk.common.hw,
		[CLK_HDMI_24M]			= &hdmi_24m_clk.common.hw,
		[CLK_HDMI_CEC]			= &hdmi_cec_clk.common.hw,
		[CLK_HDMI]			= &hdmi_clk.common.hw,
		[CLK_DSI0]			= &dsi0_clk.common.hw,
		[CLK_DSI1]			= &dsi1_clk.common.hw,
		[CLK_BUS_DSI1]			= &bus_dsi1_clk.common.hw,
		[CLK_BUS_DSI0]			= &bus_dsi0_clk.common.hw,
		[CLK_VO0_TCONLCD0]		= &vo0_tconlcd0_clk.common.hw,
		[CLK_VO0_TCONLCD1]		= &vo0_tconlcd1_clk.common.hw,
		[CLK_VO1_TCONLCD0]		= &vo1_tconlcd0_clk.common.hw,
		[CLK_COMBPHY0]			= &combphy0_clk.common.hw,
		[CLK_COMBPHY1]			= &combphy1_clk.common.hw,
		[CLK_BUS_VO1_TCONLCD0]		= &bus_vo1_tconlcd0_clk.common.hw,
		[CLK_BUS_VO0_TCONLCD1]		= &bus_vo0_tconlcd1_clk.common.hw,
		[CLK_BUS_VO0_TCONLCD0]		= &bus_vo0_tconlcd0_clk.common.hw,
		[CLK_TCONTV]			= &tcontv_clk.common.hw,
		[CLK_TCONTV1]			= &tcontv1_clk.common.hw,
		[CLK_BUS_TCONTV1]		= &bus_tcontv1_clk.common.hw,
		[CLK_BUS_TCONTV]		= &bus_tcontv_clk.common.hw,
		[CLK_LEDC]			= &ledc_clk.common.hw,
		[CLK_EDP]			= &edp_clk.common.hw,
		[CLK_BUS_LEDC]			= &bus_ledc_clk.common.hw,
		[CLK_CSI]			= &csi_clk.common.hw,
		[CLK_CSI_MASTER0]		= &csi_master0_clk.common.hw,
		[CLK_CSI_MASTER1]		= &csi_master1_clk.common.hw,
		[CLK_CSI_MASTER2]		= &csi_master2_clk.common.hw,
		[CLK_CSI_MASTER3]		= &csi_master3_clk.common.hw,
		[CLK_BUS_CSI]			= &bus_csi_clk.common.hw,
		[CLK_ISP]			= &isp_clk.common.hw,
		[CLK_DSP]			= &dsp_clk.common.hw,
		[CLK_CPUS_HCLK_GATE]		= &cpus_hclk_gate_clk.common.hw,
		[CLK_SPIF_MBUS_AHB_GATE]		= &spif_mbus_ahb_gate_clk.common.hw,
		[CLK_GMAC1_MBUS_AHB_GATE]		= &gmac1_mbus_ahb_gate_clk.common.hw,
		[CLK_GMAC0_MBUS_AHB_GATE]		= &gmac0_mbus_ahb_gate_clk.common.hw,
		[CLK_SMHC2_MBUS_AHB_GATE]		= &smhc2_mbus_ahb_gate_clk.common.hw,
		[CLK_SMHC1_MBUS_AHB_GATE]		= &smhc1_mbus_ahb_gate_clk.common.hw,
		[CLK_SMHC0_MBUS_AHB_GATE]		= &smhc0_mbus_ahb_gate_clk.common.hw,
		[CLK_USB_MBUS_AHB_GATE]		= &usb_mbus_ahb_gate_clk.common.hw,
		[CLK_GMAC1_AHB_GATE]		= &gmac1_ahb_gate_clk.common.hw,
		[CLK_GMAC0_AHB_GATE]		= &gmac0_ahb_gate_clk.common.hw,
		[CLK_SMHC2_AHB_GATE]		= &smhc2_ahb_gate_clk.common.hw,
		[CLK_SMHC1_AHB_GATE]		= &smhc1_ahb_gate_clk.common.hw,
		[CLK_SMHC0_AHB_GATE]		= &smhc0_ahb_gate_clk.common.hw,
		[CLK_USB_AHB_GATE]		= &usb_ahb_gate_clk.common.hw,
		[CLK_VID_OUT_AHB_GATE]		= &vid_out_ahb_gate_clk.common.hw,
		[CLK_VID_IN_AHB_GATE]		= &vid_in_ahb_gate_clk.common.hw,
		[CLK_VE_AHB_GATE]		= &ve_ahb_gate_clk.common.hw,
		[CLK_NPU_AHB_GATE]		= &npu_ahb_gate_clk.common.hw,
		[CLK_RES_DCAP_24M]		= &res_dcap_24m_clk.common.hw,
		[CLK_USB_24M]			= &usb_24m_clk.common.hw,
		[CLK_FANOUT_25M]		= &fanout_25m_clk.common.hw,
		[CLK_FANOUT_16M]		= &fanout_16m_clk.common.hw,
		[CLK_FANOUT_12M]		= &fanout_12m_clk.common.hw,
		[CLK_FANOUT_24M]		= &fanout_24m_clk.common.hw,
		[CLK_CLK27M_FANOUT]		= &clk27m_fanout_clk.common.hw,
		[CLK_CLK_FANOUT]		= &clk_fanout_clk.common.hw,
		[CLK_FANOUT2]			= &fanout2_clk.common.hw,
		[CLK_FANOUT1]			= &fanout1_clk.common.hw,
		[CLK_FANOUT0]			= &fanout0_clk.common.hw,
    },
    .num = CLK_NUMBER,
};
/* ccu_def_end */

static const struct sunxi_ccu_desc SUN60IW1_ccu_desc =
{
    .ccu_clks   = SUN60IW1_ccu_clks,
    .num_ccu_clks   = ARRAY_SIZE(SUN60IW1_ccu_clks),

    .hw_clks    = &SUN60IW1_hw_clks,
    .clk_type   = HAL_SUNXI_CCU,

    .resets     = SUN60IW1_ccu_resets,
    .reset_type = HAL_SUNXI_RESET,
    .num_resets = ARRAY_SIZE(SUN60IW1_ccu_resets),
};

__attribute__((weak)) int sunxi_rtc_ccu_init(void)
{
    return 0;
}

int sunxi_ccu_init(void)
{
    unsigned long reg = (unsigned long)SUNXI_CCU_BASE;
    int ret;

    ret = ccu_common_init(reg, &SUN60IW1_ccu_desc);

    return ret;
}

