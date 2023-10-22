/*
 * include/arch/arm/mach/sun20iw2p1/irqs-sun20iw2p1.h
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
    NonMaskableInt_IRQn    = -14,    /*!< 2  Cortex-M33 Non Maskable Interrupt */
    MemoryManagement_IRQn  = -12,    /*!< 4  Cortex-M33 Memory Management Interrupt */
    BusFault_IRQn          = -11,    /*!< 5  Cortex-M33 Bus Fault Interrupt */
    UsageFault_IRQn        = -10,    /*!< 6  Cortex-M33 Usage Fault Interrupt */
    SecureFault_IRQn       = -9,     /*!< 7  Cortex-M33 Secure Fault Interrupt */
    SVCall_IRQn            = -5,     /*!< 11 Cortex-M3 SV Call Interrupt */
    DebugMonitor_IRQn      = -4,     /*!< 12 Cortex-M3 Debug Monitor Interrupt */
    PendSV_IRQn            = -2,     /*!< 14 Cortex-M3 Pend SV Interrupt */
    SysTick_IRQn           = -1,     /*!< 15 Cortex-M3 System Tick Interrupt */

    AR200A_WUP_IRQn        = 0,
    CPU_SYS_IRQ_OUT0_IRQn  = 1,	     /* cpu_wdg */
    CPU_SYS_IRQ_OUT1_IRQn  = 2,      /* cpu_mbox_rd_irq */
    // CPU_SYS_IRQ_OUT2_IRQn = 3,    /* cpu2rv_mbox_w_irq */
    // CPU_SYS_IRQ_OUT3_IRQn = 4,    /* cpu2dsp_mbox_w_irq */

    BTC_SLPTMR_IRQn        =9,
    ALARM0_IRQn            = 10,
    ALARM1_IRQn            = 11,
    DMAC0_IRQ0_S_IRQn      = 12,
    DMAC0_IRQ0_NS_IRQn     = 13,
    DMAC0_IRQ1_S_IRQn      = 14,
    DMAC0_IRQ1_NS_IRQn     = 15,
    DMAC1_IRQ_S_IRQn       = 16,
    DMAC1_IRQ_NS_IRQn      = 17,

    TIMER_IRQ0_IRQn        = 20,
    TIMER_IRQ1_IRQn        = 21,
    TIMER_IRQ2_IRQn        = 22,
    TIMER_IRQ3_IRQn        = 23,
    TIMER_IRQ4_IRQn        = 24,     /* secure timer */

    WKUP_TIMER_IRQ0_IRQn   = 28,
    WKUP_TIMER_IRQ1_IRQn   = 29,
    WKUP_TIMER_IRQ2_IRQn   = 30,

    TWD_IRQn               = 32,
    UART0_IRQn             = 33,
    UART1_IRQn             = 34,
    UART2_IRQn             = 35,

    LPUART0_IRQn           = 39,
    LPUART1_IRQn           = 40,
    TWI0_IRQn              = 41,
    TWI1_IRQn              = 42,

    SPI0_IRQn              = 45,
    SPI1_IRQn              = 46,

    PWM_IRQn               = 48,
    IR_RX_IRQn             = 49,
    IR_TX_IRQn             = 50,
    LEDC_IRQn              = 51,

    LPSRAM_IRQn            = 53,
    SPI_FLASH_IRQn         = 54,

    MAD_WAKE_IRQn          = 58,
    MAD_DATA_REQ_IRQn      = 59,

    DMIC_IRQn              = 60,
    I2S0_IRQn              = 61,
    SPDIF_IRQn             = 62,
    USB0_EHCI_IRQn         = 63,
    // USB0_EHCI_IRQn      = 64,
    // USB0_EHCI_IRQn      = 65,

    SCR_IRQn               = 69,
    SPINLOCK_IRQn          = 70,
    SD0_IRQn               = 71,

    SRAM0_TZMA_IRQn        = 73,
    SRAM1_TZMA_IRQn        = 74,
    SRAM2_TZMA_IRQn        = 75,
    LPSRAM_TZMA_IRQn       = 76,
    FLASH_TZMA_IRQn        = 77,
    EXPSRAM_TZMA_IRQn      = 78,
    CE_IRQ_S_IRQn          = 79,
    CE_IRQ_NS_IRQn         = 80,
    SMC_IRQn               = 81,
    MSI_IRQn               = 82,

    G2D_IRQn               = 87,
    DE_IRQn                = 88,
    LCD0_IRQn              = 89,

    CSI_IRQn               = 93,

    DSP_SYS_IRQ_O0_IRQn    = 97,      /* dsp_dee */
    DSP_SYS_IRQ_O1_IRQn    = 98,      /* dsp_pe */
    DSP_SYS_IRQ_O2_IRQn    = 99,      /* dsp_wdg */
    DSP_SYS_IRQ_O3_IRQn    = 100,     /* dsp2cpu_mbox_w */
    // DSP_SYS_IRQ_O4_IRQn    = 101,  /* dsp2rv_mbox_w */
    DSP_SYS_IRQ_O5_IRQn    = 102,     /* dsp tzma */

    // RISCV_SYS_IRQ_O0_IRQn  = 105,  /* rv2rv_mbox_r_irq */
    // RISCV_SYS_IRQ_O1_IRQn  = 106,  /* rv2dsp_mbpx_w_irq */
    RISCV_SYS_IRQ_O2_IRQn  = 107,     /* rv2cpu_mbox_w_irq */
    RISCV_SYS_IRQ_O3_IRQn  = 108,     /* rv_wdg */

    BTC_BB_IRQn            = 110,
    BTC_DBG_IRQn           = 111,
    BTC_UNUSE_IRQn         = 112,
    BTCOEX_IRQn            = 113,
    BLE_LL_IRQn            = 114,
    GPIOA_IRQn             = 115,
    GPIOB_IRQn             = 116,
    GPIOC_IRQn             = 117,

    WLAN_IRQn              = 119,
    RCCAL_IRQn             = 120,
    GPADC_IRQn             = 121,

} IRQn_Type;

#define SUNXI_IRQ_MAX   (128)

#ifndef CONFIG_ARCH_ARM_ARMV8M_IRQ_DEFAULT_PRIORITY
#define CONFIG_ARCH_ARM_ARMV8M_IRQ_DEFAULT_PRIORITY (4)
#endif

#ifdef __cplusplus
}
#endif

#endif    /* __IRQS_SUN20IW2P1_H */
