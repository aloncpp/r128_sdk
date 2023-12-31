#ifndef _RESOURCE_H_
#define _RESOURCE_H_

typedef enum {
	PM_RES_POWER_RTC_LDO = 0,
	PM_RES_POWER_EXT_LDO,
	PM_RES_POWER_ALDO,
	PM_RES_POWER_DCDC,
	PM_RES_POWER_TOP_LDO,
	PM_RES_POWER_AON_LDO, //5
	PM_RES_POWER_APP_LDO,
	PM_RES_POWER_RV_LDO,
	PM_RES_POWER_DSP_LDO,
	PM_RES_POWER_SW,
	PM_RES_POWER_SW_VDDIO_SIP, //10
	PM_RES_POWER_SW_APP,
	PM_RES_POWER_SW_RF,
	PM_RES_POWER_SW_WF,
	PM_RES_POWER_SWM,
	PM_RES_POWER_MAX,

	PM_RES_CLOCK_OSC32K = 0,
	PM_RES_CLOCK_RC32K,
	PM_RES_CLOCK_HOSC,
	PM_RES_CLOCK_DPLL1,
	PM_RES_CLOCK_DPLL2,
	PM_RES_CLOCK_DPLL3, //5
	PM_RES_CLOCK_AUDPLL,
	PM_RES_CLOCK_DPLL_AUDPLL_LDO,
	PM_RES_CLOCK_RCOSC_AUD_RC_HF,
	PM_RES_CLOCK_APB_NO_NEED_TO_32K,
	PM_RES_CLOCK_AHB_NO_NEED_TO_32K, //10
	PM_RES_CLOCK_APB_FROM_RCO_HF,
	PM_RES_CLOCK_AHB_FROM_RCO_HF,
	PM_RES_CLOCK_RCO_CALIB,
	PM_RES_CLOCK_MAX,

	PM_RES_ANALOG_CODEC_ADC = 0,
	PM_RES_ANALOG_CODEC_DAC,
	PM_RES_ANALOG_GPADC,
	PM_RES_ANALOG_EFUSE,
	PM_RES_ANALOG_USB_PHY,
	PM_RES_ANALOG_PSRAM_PHY, //5
	PM_RES_ANALOG_RTC_GPIO,
	PM_RES_ANALOG_AON_GPIO,
	PM_RES_ANALOG_RFIP0,
	PM_RES_ANALOG_RFIP1,
	PM_RES_ANALOG_MAX, //10
} pm_resource_type_t;


#endif

