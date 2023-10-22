/*
 * include/arch/riscv/mach/sun20iw2p1/irqs-sun20iw2p1.h
 *
 * Copyright (c) 2020-2021 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: huangshr <huangshr@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __IRQS_SUN20IW2P1_H
#define __IRQS_SUN20IW2P1_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AR200A_WUP_IRQn        = 16,
	CPU_WATCHDOG           = 17,
	CPU_MSGBOX_R           = 18,
	CPU2RV_MSGBOX_W        = 19,
	CPU2DSP_MSGBOX_W       = 20,

    ALARM0_IRQn            = 26,
    ALARM1_IRQn            = 27,

    DMAC0_IRQ0_S_IRQn      = 28,
    DMAC0_IRQ0_NS_IRQn     = 29,
    DMAC0_IRQ1_S_IRQn      = 30,
    DMAC0_IRQ1_NS_IRQn     = 31,
    DMAC1_IRQ_S_IRQn       = 32,
    DMAC1_IRQ_NS_IRQn      = 33,

    TIMER_IRQ0_IRQn        = 36,
    TIMER_IRQ1_IRQn        = 37,
    TIMER_IRQ2_IRQn        = 38,
    TIMER_IRQ3_IRQn        = 39,
    TIMER_IRQ4_IRQn        = 40,     /* secure timer */

    WKUP_TIMER_IRQ0_IRQn   = 44,
    WKUP_TIMER_IRQ1_IRQn   = 45,
    WKUP_TIMER_IRQ2_IRQn   = 46,

    TWD_IRQn               = 48,
    UART0_IRQn             = 49,
    UART1_IRQn             = 50,
    UART2_IRQn             = 51,

    LPUART0_IRQn           = 55,
    LPUART1_IRQn           = 56,
    TWI0_IRQn              = 57,
    TWI1_IRQn              = 58,

    SPI0_IRQn              = 61,
    SPI1_IRQn              = 62,

    PWM_IRQn               = 64,
    IR_RX_IRQn             = 65,
    IR_TX_IRQn             = 66,
    LEDC_IRQn              = 67,

    LPSRAM_IRQn            = 69,
    SPI_FLASH_IRQn         = 70,

    MAD_WAKE_IRQn          = 74,
    MAD_DATA_REQ_IRQn      = 75,

    DMIC_IRQn              = 76,
    I2S0_IRQn              = 77,
    SPDIF_IRQn             = 78,
    USB0_EHCI_IRQn         = 79,
    USB0_OHCI_IRQn         = 80,
    USB0_OTG_IRQn          = 81,

    SCR_IRQn               = 85,
    SPINLOCK_IRQn          = 86,
    SD0_IRQn               = 87,

    SRAM0_TZMA_IRQn        = 88,
    SRAM1_TZMA_IRQn        = 89,
    SRAM2_TZMA_IRQn        = 90,
    SRAM3_TZMA_IRQn        = 91,
    LPSRAM_TZMA_IRQn       = 92,
    FLASH_TZMA_IRQn        = 93,
    EXPSRAM_TZMA_IRQn      = 94,
    CE_IRQ_S_IRQn          = 95,
    CE_IRQ_NS_IRQn         = 96,
    SMC_IRQn               = 97,
    MSI_IRQn               = 98,

    G2D_IRQn               = 103,
    DE_IRQn                = 104,
    LCD0_IRQn              = 105,

    CSI_IRQn               = 109,
    VBAT_MON               = 110,

    DSP_SYS_IRQ_O0_IRQn    = 113,     /* dsp_dee */
    DSP_SYS_IRQ_O1_IRQn    = 114,     /* dsp_pe */
    DSP_SYS_IRQ_O2_IRQn    = 115,     /* dsp_wdg */
    DSP_SYS_IRQ_O3_IRQn    = 116,     /* dsp2cpu_mbox_w */
    DSP_SYS_IRQ_O4_IRQn    = 117,     /* dsp2rv_mbox_w */
    DSP_SYS_IRQ_O5_IRQn    = 118,     /* dsp tzma */

    RISCV_SYS_IRQ_O0_IRQn  = 121,     /* rv2rv_mbox_r_irq */
    RISCV_SYS_IRQ_O1_IRQn  = 122,     /* rv2dsp_mbpx_w_irq */
    RISCV_SYS_IRQ_O2_IRQn  = 123,     /* rv2cpu_mbox_w_irq */
    RISCV_SYS_IRQ_O3_IRQn  = 124,     /* rv_wdg */

    BTCOEX_IRQn            = 129,
    BLE_LL_IRQn            = 130,
    GPIOA_IRQn             = 131,
    GPIOB_IRQn             = 132,
    GPIOC_IRQn             = 133,

    WLAN_IRQn              = 135,
    RCCAL_IRQn             = 136,
    GPADC_IRQn             = 137,

} IRQn_Type;

#define SUNXI_IRQ_MAX   (140)

#ifdef __cplusplus
}
#endif

#endif    /* __IRQS_SUN20IW2P1_H */
