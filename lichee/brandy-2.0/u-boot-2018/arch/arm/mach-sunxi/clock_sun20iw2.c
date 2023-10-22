/*
 * Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
*/

#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/clock.h>
#include <asm/arch/timer.h>
#include <asm/arch/prcm.h>

#define GPRCM_HOSC_TYPE       (SUNXI_CCMAON_BASE + 0x84)
#define GPRCM_HOSC_26M    (0)
#define GPRCM_HOSC_40M    (1)
#define GPRCM_HOSC_24M    (2)
#define GPRCM_HOSC_52M    (3)

#define SYSTEM_HFCLK26M (26000000)
#define SYSTEM_HFCLK40M (40000000)
#define SYSTEM_HFCLK24M (24000000)
#define SYSTEM_HFCLK32M (32000000)
#define SYSTEM_HFCLK24_576M (24576000)
#define SYSTEM_HFCLKUKN (0)
#define SYSTEM_LFCLK    (32768)
#define RCO_HF_CLK      (8192000)

#define TRY_HFCLK40M        (0x0)
#define TRY_HFCLK24M        (0x1)
#define TRY_HFCLK26M        (0x2)
#define TRY_HFCLK32M        (0x3)
#define TRY_HFCLK24_576M    (0x4)


#define GPRCM_USB_BIAS_CTRL				(SUNXI_PRCM_BASE + 0x064)
#define GPRCM_FAST_BOOT_INFO_REG		(SUNXI_PRCM_BASE + 0x021C)
#define CCMUAON_SYS_PLL1_OUT_CTRL		(SUNXI_CCMAON_BASE + 0xA4)


unsigned int sys_get_hosc_clk(void)
{
	u32 val, clock;
	val = readl(GPRCM_HOSC_TYPE) & 0x7;
	switch (val) {
	case 0://26M hosc
		clock = SYSTEM_HFCLK26M;
		break;
	case 1://40M hosc
		clock = SYSTEM_HFCLK40M;
		break;
	case 2://24M hosc
		clock = SYSTEM_HFCLK24M;
		break;
	case 3://32M hosc
		clock = SYSTEM_HFCLK32M;
		break;
	case 4://24.576M hosc
		clock = SYSTEM_HFCLK24_576M;
		break;
	default:
		clock = 0;
		break;
	}
    return clock;//Hz
}

static void align_clock_to_rtos(void)
{
	/*this is from rtos kernel. without this will cause read and write efuse blocked. fix me*/
	/* Use BROM DPLL1 configuration when enter USB burn mode, it will set DPLL1 to 1920M with any external crystal frequency.
	 * Note: If fraction multiplication of DPLL1 is enable(eg: external crystal frequency is 24.576M or 26M),
	 * modify the DPLL1 frequency may cause USB hareware work abnormally.
	 * writel(0xe0000301, 0x4004c48c); //dpll1
	 */
	//writel(0xc0aba00b, 0x4004c4A4);
	//writel(0x00002191, 0x4004c4e0);
}

void clock_init_uart(void)
{
	align_clock_to_rtos();

	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
#if 0
	/* uart clock source is apb2 */
	writel(APB2_CLK_SRC_OSC24M|
	       APB2_CLK_RATE_N_1|
	       APB2_CLK_RATE_M(1),
	       &ccm->apb2_cfg);
#endif

	/* open the clock for uart */
	clrbits_le32(&ccm->bus_clk_gating_ctrl0,
		     1 << (UART_GATING_SHIFT + CONFIG_CONS_INDEX - 1));
	/* udelay(2); */

	clrbits_le32(&ccm->dev_rst_ctrl0,
		     1 << (UART_RESET_SHIFT + CONFIG_CONS_INDEX - 1));
	/* udelay(2); */

	/* deassert uart reset */
	setbits_le32(&ccm->dev_rst_ctrl0,
		     1 << (UART_RESET_SHIFT + CONFIG_CONS_INDEX - 1));

	/* open the clock for uart */
	setbits_le32(&ccm->bus_clk_gating_ctrl0,
		     1 << (UART_GATING_SHIFT + CONFIG_CONS_INDEX - 1));
}

static uint clock_get_dpll3(void)
{
	u32 clock, hosc_clk, dpll_frac, dpll_ndiv, factor_m, val;

	hosc_clk = sys_get_hosc_clk() / 1000000;

	val = readl(SUNXI_CCMAON_BASE + 0x94);
	dpll_ndiv = val & 0xff0;
	dpll_ndiv = dpll_ndiv * 1000000;

	dpll_frac = 0;

	//DPLL_FRAC_CTRL enable
	if (((val >> 12) & 0x1) == 1) {
		dpll_frac = (val >> 13) & 0xffff;
		dpll_frac = (dpll_frac * 1000000) >> 16;// 2^16
	}

	factor_m = val & 0xf;
	if (factor_m == 0) {
		factor_m = 1;
	} else if (factor_m >= 8) {
		factor_m = 8;
	}
	clock = hosc_clk * (dpll_ndiv + dpll_frac) / factor_m / 1000000;

	return clock;//MHz
}

static uint clock_get_hspsram(void)
{
	u32 val, pll_sel, div, clock;

	val = readl(SUNXI_CCMAON_BASE + 0xE0);
	pll_sel = (val >> 21) & 0x01;//CKPLL_HSPSRAM_SEL
	if (pll_sel ==  0) {
		//dpll1 output as clk source
		div = (readl(SUNXI_CCMAON_BASE + 0xA4) >> 12) & 0x3;//CK1_HSPSRAM_DIV 00:M=3; 01:M=2.5; 10:M=2; 11:M=1
		switch (div) {
		case 0:
			div = 3;
			clock = 1920 / div;
			break;
		case 1:
			clock = 1920 * 2 / 5;
			break;
		case 2:
		case 3:
			div = 4 - div;
			clock = 1920 / div;
			break;
		default:
			clock = 0;
			break;
		}
	} else {
		//dpll3 output as clk source
		div = (readl(SUNXI_CCMAON_BASE + 0xA8) >> 12) & 0x3;//CK3_DEV_DIV 00:M=3; 01:M=2.5; 10:M=2; 11:M=1
		switch (div) {
		case 0:
			div = 3;
			clock = clock_get_dpll3() / div;
			break;
		case 1:
			clock = clock_get_dpll3() * 2 / 5;
			break;
		case 2:
		case 3:
			div = 4 - div;
			clock = clock_get_dpll3() / div;
			break;
		default:
			clock = 0;
			break;
		}
	}

	return clock;//MHz
}

//no this func is ok
/*uint clock_get_pll_ddr(void)
{
	//FIXME
	return 8;
}*/


uint clock_get_corepll(void)
{
	//FIXME
	//return 24;
	u32 pll_sel, div, clock;

	pll_sel = readl(SUNXI_CCMAON_BASE + 0xE0);
	pll_sel = (pll_sel >> 19) & 0x01;//CKPLL_M33_SEL
	if (pll_sel ==  0) {
		//dpll1 output as clk source
		div = readl(SUNXI_CCMAON_BASE + 0xA4) & 0x7;//CK1_M33_DIV 000:M=8; 001:M=7; 010:M=6; 011:M=5; 1xx:M=4
		div = 8 - div;
		if (div <= 4) {
			div = 4;
		}
		clock = 1920 / div;
	} else {
		//dpll3 output as clk source
		div = readl(SUNXI_CCMAON_BASE + 0xA8) & 0x7;//CK3_M33_DIV 000:M=8; 001:M=7; 010:M=6; 011:M=5; 1xx:M=4
		div = 8 - div;
		if (div <= 4) {
			div = 4;
		}
		clock = clock_get_dpll3() / div;
	}

	return clock;//MHz
}

//peripheral clock: device clk
uint clock_get_pll6(void)
{
	//FIXME
	//return 0;
	u32 val, pll_sel, div, clock;

	val = readl(SUNXI_CCMAON_BASE + 0xE0);
	pll_sel = (val >> 16) & 0x01;//CKPLL_DEV_SEL
	if (pll_sel ==  0) {
		//dpll1 output as clk source
		div = (readl(SUNXI_CCMAON_BASE + 0xA4) >> 20) & 0x3;//CK1_DEV_DIV 00:M=7; 01:M=6; 10:M=5; 11:M=4
		div = 7 - div;
		clock = 1920 / div;
	} else {
		//dpll3 output as clk source
		div = (readl(SUNXI_CCMAON_BASE + 0xA8) >> 20) & 0x3;//CK3_DEV_DIV 00:M=7; 01:M=6; 10:M=5; 11:M=4
		div = 7 - div;
		clock = clock_get_dpll3() / div;
	}

	div = (val & 0x0f) + 1;
	clock = clock / div;

	return clock;//MHz
}


uint clock_get_axi(void)
{
	//FIXME
	//return 24;
	return clock_get_corepll();
}

static uint clock_get_sys(void)
{
	u32 clock, val, div;

	val = readl(SUNXI_CCMAON_BASE + 0xE0);
	div = ((val >> 8) & 0x0f) + 1;//SYS_CLK_DIV
	clock = clock_get_corepll() / div;

	return clock;//MHz
}

static uint clock_get_rco_hf_div(void)
{
	u32 clock, val, div;

	val = readl(SUNXI_CCMAON_BASE + 0xDC);//found in ccmu_aon spec clock tree
	div = (val & 0x07) + 1;//M33_CLK_SRC_SEL
	clock = RCO_HF_CLK / div / 1000000;

	return clock;//MHz
}

static uint clock_get_ar200a_hclk(void)
{
	u32 clock, val, src;

	val = readl(SUNXI_CCMAON_BASE + 0xE0);
	src = (val >> 12) & 0x03;//M33_CLK_SRC_SEL

	switch (src) {
	case 0:
		clock = sys_get_hosc_clk() / 1000000;
		break;
	case 1:
		clock = SYSTEM_LFCLK / 1000;//32K
		break;
	case 2:
		clock = clock_get_sys();
		break;
	case 3:
		clock = clock_get_rco_hf_div();
		break;
	default:
		clock = 0;
		break;
	}

	return clock;
}

uint clock_get_ahb(void)
{
	//FIXME
	//return 24;
	return clock_get_ar200a_hclk();//AHB in ccmu_aon spec clock tree is ar200a_hcl
}


uint clock_get_apb1(void)
{
	//FIXME
	//return 24;
	u32 clock, val, src, div;

	val = readl(SUNXI_CCMAON_BASE + 0xE0);
	src = (val >> 6) & 0x03;

	switch (src) {
	case 0:
		clock = sys_get_hosc_clk() / 1000000;
		break;
	case 1:
		clock = SYSTEM_LFCLK / 1000;//32K
		break;
	case 2:
		clock = clock_get_ahb();
		break;
	case 3:
		clock = clock_get_rco_hf_div();
		break;
	default:
		clock = 0;
		break;
	}

	if (src == 3) {
		return clock;//rco_hf_div
	}

	div = 1 << ((val >> 4) & 0x03);
	clock = clock / div;

	return clock;//MHz
}

uint clock_get_apb2(void)
{
	//FIXME
	//return 24;
	struct sunxi_ccm_reg *const ccm = (struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	u32 cfg, prediv, src, div;

	cfg = readl(&ccm->apb_spc_clk_ctrl);
	src = (cfg >> 24) & 0x3;
	prediv = 1 << ((cfg >> 16) & 0x3);
	div = (cfg & 0xf) + 1;

	switch (src) {
	case 0:
		return sys_get_hosc_clk() / prediv / div / 1000000;
	case 1:
		return clock_get_pll6() / prediv / div;
	case 2:
		return SYSTEM_LFCLK / 1000;//32K
	default:
		return 0;
	}
}


uint clock_get_mbus(void)
{
	/*unsigned int clock;

	clock = clock_get_pll_ddr();
	clock = clock/4;

	return clock;*/

	return clock_get_hspsram();//mubs clk is hspsram clk?
}

static void sys_pll1_out_ctrl(u32 data, u32 offset)
{
	u32 val = 0;

	//enable hpsram_pll out
	val = readl(CCMUAON_SYS_PLL1_OUT_CTRL);
	val &= ~(0x1 << offset);
	val |= (data << offset);
	writel(val, CCMUAON_SYS_PLL1_OUT_CTRL);
	__usdelay(20);
}

static u32 get_efuse_hosc_calc(void)
{
	u32 system_cfg;
	system_cfg = readl(SUNXI_SID_SRAM_BASE + 0x44);
	return ((system_cfg & 0x000400) >> 10);
}


static u32 usb_try_get_hosc_type(void)
{
	u32 val = 0;
	u32 hosc_type = 0;

	val = readl(GPRCM_FAST_BOOT_INFO_REG);
	switch (val) {
	case(TRY_HFCLK40M):
		hosc_type = SYSTEM_HFCLK40M;
	break;
	case(TRY_HFCLK24M):
		hosc_type = SYSTEM_HFCLK24M;
	break;
	case(TRY_HFCLK26M):
		hosc_type = SYSTEM_HFCLK26M;
	break;
	case(TRY_HFCLK32M):
		hosc_type = SYSTEM_HFCLK32M;
	break;
	case(TRY_HFCLK24_576M):
		hosc_type = SYSTEM_HFCLK24_576M;
	break;
	default:
		hosc_type = SYSTEM_HFCLK40M;
	break;
	}

	//sys_set_hosc(hosc_type);
	return hosc_type;
}

static void usb_set_data(u32 addr, u32 data)
{
	u32 val = 0;

	//clear bit0 vc_clk,bit7 vc_di,bit15:8:vc_adr
	val = readl(SUNXI_USB0_BASE + 0x410);
	val &= ~((0x01 << 0) | (0xff << 8) | (0x1 << 7));
	writel(val, (SUNXI_USB0_BASE + 0x410));

	//set data
	val = readl(SUNXI_USB0_BASE + 0x410);
	val |= (addr << 8) | (data << 7);
	writel(val, (SUNXI_USB0_BASE + 0x410));

	val = readl(SUNXI_USB0_BASE + 0x410);
	val |= (0x1 << 0);
	writel(val, (SUNXI_USB0_BASE + 0x410));

	__usdelay(50);
}

static void usb_set_hosc_type(u32 r22, u32 r21, u32 r20)
{
	usb_set_data(0x20, r20);
	usb_set_data(0x21, r21);
	usb_set_data(0x22, r22);
}

/*
000---24M
001---26M
010---32M
011---40M
100---24.576M--DPLL1
101---26M---DPLL1
11x---40M
*/
static void usb_configure_hosc_type(u32 clock)
{
	switch (clock) {
	case(SYSTEM_HFCLK24M):
		usb_set_hosc_type(0, 0, 0);
	break;
	case(SYSTEM_HFCLK26M):
		usb_set_hosc_type(1, 0, 1);
	break;
	case(SYSTEM_HFCLK32M):
		usb_set_hosc_type(0, 1, 0);
	break;
	case(SYSTEM_HFCLK40M):
		usb_set_hosc_type(0, 1, 1);
	break;
	case(SYSTEM_HFCLK24_576M):
		usb_set_hosc_type(1, 0, 0);
	break;
	default:
		usb_set_hosc_type(0, 1, 1);
	break;
	}
}

static void usb_clock_init(void)
{
	u32 hosc_type;

	if (get_efuse_hosc_calc()) {
		hosc_type = usb_try_get_hosc_type();
		usb_configure_hosc_type(hosc_type);
	}

#if 0
	u32 hosc_type = SYSTEM_HFCLK40M;
	int ret;

	hosc_type = sys_get_hosc();
	if (hosc_type == SYSTEM_HFCLKUKN) {
		if (get_efuse_hosc_calc()) {
			hosc_type = usb_try_get_hosc_type();
			usb_configure_hosc_type(hosc_type);
		} else {
			/*rc detect hsoc freq*/
			hosc_type = get_hosc_type_from_rc();
			/*get hosc type*/
			if (hosc_type != SYSTEM_HFCLKUKN) {
				/*update pll*/
				switch_all_clock_to_hosc();
				/*disable usb_48M*/
				sys_pll1_out_ctrl(0, 31);
				/*set pll*/
				switch_all_clock_to_pll(hosc_type);
				/*enable usb_48M*/
				sys_pll1_out_ctrl(1, 31);
				/*configure usb hosc_type*/
				usb_configure_hosc_type(hosc_type);
			} else {
				usb_configure_hosc_type(SYSTEM_HFCLK40M);
			}
			}
	} else {
		usb_configure_hosc_type(hosc_type);
	}
#endif
}

static void disable_otg_clk_reset_gating(void)
{
	u32 reg_temp = 0;

	//disable otg clk gating
	reg_temp = readl(SUNXI_CCM_BASE + 0x4);
	reg_temp &= ~(0x01 << 27);
	writel(reg_temp, SUNXI_CCM_BASE + 0x4);

	//reset
	reg_temp = readl(SUNXI_CCM_BASE + 0xC);
	reg_temp &= ~(0x01 << 27);
	writel(reg_temp, SUNXI_CCM_BASE + 0xC);
}

//数字电路，先reset，后clk
static void enable_otg_clk_reset_gating(void)
{
	u32 reg_temp = 0;

	//enable otg reset
	reg_temp = readl(SUNXI_CCM_BASE + 0xC);
	reg_temp |= (0x01 << 27);
	writel(reg_temp, SUNXI_CCM_BASE + 0xC);

	__usdelay(100);

	//enable otg clk gating
	reg_temp = readl(SUNXI_CCM_BASE + 0x4);
	reg_temp |= (0x01 << 27);
	writel(reg_temp, SUNXI_CCM_BASE + 0x4);

	__usdelay(50);

}

static void disable_phy_clk_reset_gating(void)
{
	u32 reg_temp = 0;

	//disable phy clk gating and reset
	reg_temp = readl(SUNXI_CCM_BASE + 0xC);
	reg_temp &= ~(0x01 << 25);
	writel(reg_temp, SUNXI_CCM_BASE + 0xC);
}

//模拟电路,先clk，
static void enable_phy_clk_reset_gating(void)
{
	u32 reg_temp = 0;

	//USB 0 phy reset
	reg_temp = readl(SUNXI_CCM_BASE + 0xC);
	reg_temp |= (0x01 << 25);
	writel(reg_temp, SUNXI_CCM_BASE + 0xC);

	__usdelay(50);
}

int usb_open_clock(void)
{
	enable_otg_clk_reset_gating();
	enable_phy_clk_reset_gating();
	usb_clock_init();
	debug("SUNXI_USB0_BASE+0x410:%08x, SUNXI_USB0_BASE+0x420:%08x\n", readl(SUNXI_USB0_BASE+0x410), readl(SUNXI_USB0_BASE+0x420));
#if 0
	u32 reg_value = 0;

	reg_value = readl(SUNXI_CCM_BASE + 0xC);
	reg_value |=  (1 << 31);
	writel(reg_value, (SUNXI_CCM_BASE + 0xC));
	//delay some time
	__msdelay(1);
	reg_value = readl(SUNXI_CCM_BASE + 0xC);
	reg_value |=  (1 << 30);
	writel(reg_value, (SUNXI_CCM_BASE + 0xC));
	//delay some time
	__msdelay(1);
	reg_value = readl(SUNXI_CCM_BASE + 0xC);
	reg_value |=  (1 << 27);
	writel(reg_value, (SUNXI_CCM_BASE + 0xC));
	//delay some time
	__msdelay(1);
	reg_value = readl(SUNXI_CCM_BASE + 0xC);
	reg_value |=  (1 << 25);
	writel(reg_value, (SUNXI_CCM_BASE + 0xC));
	//delay some time
	__msdelay(1);


	reg_value = readl(SUNXI_CCM_BASE + 0x4);
	reg_value |=  (1 << 31);
	writel(reg_value, (SUNXI_CCM_BASE + 0x4));
	//delay some time
	__msdelay(1);
	reg_value = readl(SUNXI_CCM_BASE + 0x4);
	reg_value |=  (1 << 30);
	writel(reg_value, (SUNXI_CCM_BASE + 0x4));
	//delay some time
	__msdelay(1);
	reg_value = readl(SUNXI_CCM_BASE + 0x4);
	reg_value |=  (1 << 27);
	writel(reg_value, (SUNXI_CCM_BASE + 0x4));
	//delay some time
	__msdelay(1);
#endif

	return 0;
}

int usb_close_clock(void)
{
	__msdelay(500);
	u32 reg_value = 0;

	reg_value = readl(GPRCM_USB_BIAS_CTRL);
	reg_value |= 0x1;
	writel(reg_value, GPRCM_USB_BIAS_CTRL);

	/*enable usb_48M*/
	sys_pll1_out_ctrl(1, 31);

	disable_otg_clk_reset_gating();
	disable_phy_clk_reset_gating();
#if 0
	u32 reg_value = 0;

	reg_value = readl(SUNXI_CCM_BASE + 0x4);
	reg_value &= ~(1 << 31);
	writel(reg_value, (SUNXI_CCM_BASE + 0x4));
	__msdelay(1);

	reg_value = readl(SUNXI_CCM_BASE + 0x4);
	reg_value &= ~(1 << 30);
	writel(reg_value, (SUNXI_CCM_BASE + 0x4));
	__msdelay(1);

	reg_value = readl(SUNXI_CCM_BASE + 0x4);
	reg_value &= ~(1 << 27);
	writel(reg_value, (SUNXI_CCM_BASE + 0x4));
	__msdelay(1);
#endif

	return 0;
}



