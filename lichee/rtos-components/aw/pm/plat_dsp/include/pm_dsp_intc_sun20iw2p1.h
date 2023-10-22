#ifndef __PM_DSP_INTC_SUN20IW2P1_H__
#define __PM_DSP_INTC_SUN20IW2P1_H__

#include <irqs.h>

typedef enum {
	AR200A_WUP_IRQn		= RINTC_IRQ_MASK | 1,

	ALARM0_IRQn		= RINTC_IRQ_MASK | 11,
	ALARM1_IRQn		= RINTC_IRQ_MASK | 12,

	WKUP_TIMER_IRQ0_IRQn	= RINTC_IRQ_MASK | 29,
	WKUP_TIMER_IRQ1_IRQn	= RINTC_IRQ_MASK | 30,
	WKUP_TIMER_IRQ2_IRQn	= RINTC_IRQ_MASK | 31,

	LPUART0_IRQn		= RINTC_IRQ_MASK | 40,
	LPUART1_IRQn		= RINTC_IRQ_MASK | 41,

	MAD_WAKE_IRQn		= RINTC_IRQ_MASK | 59,

	BTCOEX_IRQn		= RINTC_IRQ_MASK | 114,
	BLE_LL_IRQn		= RINTC_IRQ_MASK | 115,

	GPIOA_IRQn		= RINTC_IRQ_MASK | 116,
	GPIOB_IRQn		= RINTC_IRQ_MASK | 117,
	GPIOC_IRQn		= RINTC_IRQ_MASK | 118,

	WLAN_IRQn		= RINTC_IRQ_MASK | 120,

	GPADC_IRQn		= RINTC_IRQ_MASK | 122,
} IRQn_Type;

#endif /* __PM_DSP_INTC_SUN20IW2P1_H__ */