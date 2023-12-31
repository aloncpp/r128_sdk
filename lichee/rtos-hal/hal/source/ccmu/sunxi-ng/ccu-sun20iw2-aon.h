/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 frank@allwinnertech.com
 */

#ifndef _CCU_SUN20IW2_AON_H_
#define _CCU_SUN20IW2_AON_H_

int sunxi_ccu_aon_init();

#define SUNXI20_CCU_AON_BASE   0X4004C400

enum {
	CLK_HOSC = 0,
	CLK_HOSC_DETECT,
	CLK_DPLL1,
	CLK_DPLL2,
	CLK_DPLL3,
	RFIP0_DPLL,
	RFIP1_DPLL,
	CLK_PLL_AUDIO,
	CLK_PLL_AUDIO1X,
	CLK_PLL_AUDIO2X,
	CLK_CK1_USB,
	CLK_CK1_AUD,
	CLK_CK1_DEV,
	CLK_CK1_LSPSRAM,
	CLK_CK1_HSPSRAM_PRE,
	CLK_CK1_HSPSRAM,
	CLK_CK1_HIFI5,
	CLK_CK1_C906_PRE,
	CLK_CK1_C906,
	CLK_CK1_M33,
	CLK_CK3_DEV,
	CLK_CK3_LSPSRAM,
	CLK_CK3_HSPSRAM_PRE,
	CLK_CK3_HSPSRAM,
	CLK_CK3_HIFI5,
	CLK_CK3_C906_PRE,
	CLK_CK3_C906,
	CLK_CK3_M33,
	CLK_LDO_BYPASS,
	CLK_LDO2_EN,
	CLK_LDO1_EN,
	CLK_WLAN_BT_DEBUG_SEL0,
	CLK_WLAN_BT_DEBUG_SEL1,
	CLK_WLAN_BT_DEBUG_SEL2,
	CLK_WLAN_BT_DEBUG_SEL3,
	CLK_WLAN_BT_DEBUG_SEL4,
	CLK_WLAN_BT_DEBUG_SEL5,
	CLK_WLAN_BT_DEBUG_SEL6,
	CLK_WLAN_BT_DEBUG_SEL7,
	CLK_WLAN_SEL,
	CLK_BT_SEL,
	CLK_PFIP2_GATE,
	CLK_BLE_32M,
	CLK_BLE_48M,
	CLK_MAD_AHB_GATE,
	CLK_GPIO_GATE,
	CLK_BUS_CODEC_DAC,
	CLK_RCCAL,
	CLK_BUS_CODEC_ADC,
	CLK_MAD_APB_GATE,
	CLK_BUS_DMIC,
	CLK_GPADC,
	CLK_LPUART1_WKUP,
	CLK_LPUART0_WKUP,
	CLK_LPUART0,
	CLK_LPUART1,
	CLK_GPADC_CTRL,
	CLK_DMIC_GATE,
	CLK_SPDIF_TX,
	CLK_I2S,
	CLK_CODEC_DAC,
	CLK_CODEC_ADC_SEL1,
	CLK_CODEC_ADC_GATE,
	CLK_AUDPLL_HOSC_SEL,
	CLK_CODEC_ADC_DIV,
	CLK_CK1_AUD_DIV,
	CLK_AUD_RCO_DIV,
	CLK_CKPLL_HSPSRAM_SEL,
	CLK_CKPLL_LSPSRAM_SEL,
	CLK_CK_M33,
	CLK_CKPLL_HIFI5_SEL,
	CLK_CKPLL_C906_SEL,
	CLK_CK_DEV,
	CLK_SYS,
	CLK_AR200A_F,
	CLK_HFCLK_DIV,
	CLK_LFCLK_DIV,
	CLK_AHB_DIV,
	CLK_APB,
	CLK_DEVICE,
	CLK_MAD_LPSD,
	CLK_SPDIF_RX,
	CLK_I2S_ASRC,

	CLK_AON_NUMBER,
};

uint32_t HAL_GetHFClock(void);
#endif  /* _CCU_SUN20IW2_H_ */
