/*
 * include/arch/arm/mach/sun20iw2p1/memory-sun20iw2p1.h
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

#ifndef __MEMORY_SUN20IW2P1_H
#define __MEMORY_SUN20IW2P1_H

#ifdef __cplusplus
extern "C" {
#endif

/*!< Peripheral memory map */
#define PLAT_PHYS_OFFSET         (0x40000000U)

#define DMAC0_BASE	(PLAT_PHYS_OFFSET + 0x00000000)
#define DMAC1_BASE	(PLAT_PHYS_OFFSET + 0x00001000)
#define SPINLOCK_BASE	(PLAT_PHYS_OFFSET + 0x00003000)
#define CE_BASE		(PLAT_PHYS_OFFSET + 0x00004000)
#define SMC_BASE	(PLAT_PHYS_OFFSET + 0x00005000)
#define USB_BASE	(PLAT_PHYS_OFFSET + 0x00007000)
#define SDIO_BASE	(PLAT_PHYS_OFFSET + 0x00008000)
#define SPI0_BASE	(PLAT_PHYS_OFFSET + 0x00009000)
#define SYSCTRL_BASE	(PLAT_PHYS_OFFSET + 0x0000A000)
#define FLASHC_BASE	(PLAT_PHYS_OFFSET + 0x0000B000)
#define LPSRAM_BASE	(PLAT_PHYS_OFFSET + 0x0000D000)
#define WIFIC_BASE	(PLAT_PHYS_OFFSET + 0x0000E000)
#define SPI1_BASE	(PLAT_PHYS_OFFSET + 0x0000F000)
#define MSI_BASE	(PLAT_PHYS_OFFSET + 0x00100000)
#define CSI_BASE	(PLAT_PHYS_OFFSET + 0x00300000)

/* CPU SYS */
#define CPU_CFG_BASE	(PLAT_PHYS_OFFSET + 0x00020000)
#define CPU_WDG_BASE	(PLAT_PHYS_OFFSET + 0x00020400)
#define CPU_MBOX_BASE	(PLAT_PHYS_OFFSET + 0x00020800)

/* DSP SYS */
#define DSP_MBOX_BASE	(PLAT_PHYS_OFFSET + 0x00022000)
#define DSP_TZMA_BASE	(PLAT_PHYS_OFFSET + 0x00023000)
#define DSP_INTC_BASE	(PLAT_PHYS_OFFSET + 0x00023400)
#define DSP_WDG_BASE	(PLAT_PHYS_OFFSET + 0x00023800)
#define DSP_CFG_BASE	(PLAT_PHYS_OFFSET + 0x00023C00)

/* RISCV sys */
#define RISCV_CFG_BASE	(PLAT_PHYS_OFFSET + 0x00028000)
#define RISCV_WDG_BASE	(PLAT_PHYS_OFFSET + 0x00029000)
#define RISCV_TIMESTAMP_BASE	(PLAT_PHYS_OFFSET + 0x0002A000)
#define RISCV_MBOX_BASE	(PLAT_PHYS_OFFSET + 0x0002B000)

/* APB DEC */
#define HPSRAM_BASE	(PLAT_PHYS_OFFSET + 0x00038000)
#define CCMU_BASE	(PLAT_PHYS_OFFSET + 0x0003C000)
#define LPSRAM_TZMA_BASE	(PLAT_PHYS_OFFSET + 0x0003E000)
#define TZMA0_BASE	(PLAT_PHYS_OFFSET + 0x0003E400)
#define TZMA1_BASE	(PLAT_PHYS_OFFSET + 0x0003E800)
#define TZMA2_BASE	(PLAT_PHYS_OFFSET + 0x0003EC00)
#define TZMA3_BASE	(PLAT_PHYS_OFFSET + 0x0003F000)
#define FLASH_TZME_BASE	(PLAT_PHYS_OFFSET + 0x0003F400)
#define FLASH_ENC_BASE	(PLAT_PHYS_OFFSET + 0x0003F800)
#define EXPSRAM_TZMA_BASE	(PLAT_PHYS_OFFSET + 0x0003FC00)
#define TIMER_BASE	(PLAT_PHYS_OFFSET + 0x00043000)
#define SPC_BASE	(PLAT_PHYS_OFFSET + 0x00044000)
#define PWM_BASE	(PLAT_PHYS_OFFSET + 0x00045000)
#define SMCARD_BASE	(PLAT_PHYS_OFFSET + 0x00045400)
#define I2S_BASE	(PLAT_PHYS_OFFSET + 0x00045800)
#define SPDIF_BASE	(PLAT_PHYS_OFFSET + 0x00045C00)
#define IRRX_BASE	(PLAT_PHYS_OFFSET + 0x00046000)
#define IRTX_BASE	(PLAT_PHYS_OFFSET + 0x00046400)
#define UART0_BASE	(PLAT_PHYS_OFFSET + 0x00047000)
#define UART1_BASE	(PLAT_PHYS_OFFSET + 0x00047400)
#define UART2_BASE	(PLAT_PHYS_OFFSET + 0x00047800)
#define TWD_BASE	(PLAT_PHYS_OFFSET + 0x00047C00)
#define LEDC_BASE	(PLAT_PHYS_OFFSET + 0x00048000)
#define AHB_BW_MON0_BASE	(PLAT_PHYS_OFFSET + 0x00048400)
#define AHB_BW_MON1_BASE	(PLAT_PHYS_OFFSET + 0x00048800)
#define TRNG_BASE	(PLAT_PHYS_OFFSET + 0x00048C00)
#define TWI0_BASE	(PLAT_PHYS_OFFSET + 0x00049000)
#define TWI1_BASE	(PLAT_PHYS_OFFSET + 0x00049400)

/* AHB DEC1  aon domain*/
#define GPADC_BASE	(PLAT_PHYS_OFFSET + 0x0004A000)
#define GPIO_BASE	(PLAT_PHYS_OFFSET + 0x0004A400)
#define LPUART0_BASE	(PLAT_PHYS_OFFSET + 0x0004A800)
#define LPUART1_BASE	(PLAT_PHYS_OFFSET + 0x0004AC00)
#define CODEC_BASE	(PLAT_PHYS_OFFSET + 0x0004B000)
#define DMIC_BASE	(PLAT_PHYS_OFFSET + 0x0004C000)
#define CCMU_AON_BASE	(PLAT_PHYS_OFFSET + 0x0004C400)
#define RCOSC_CAL_BASE	(PLAT_PHYS_OFFSET + 0x0004C800)
#define WAKEUP_TIMER_BASE	(PLAT_PHYS_OFFSET + 0x0004CC00)
#define MAD_BASE	(PLAT_PHYS_OFFSET + 0x0004E000)
#define SID_BASE	(PLAT_PHYS_OFFSET + 0x0004E400)

/* AHB DEC1 rtc domain */
#define GPRCM_BASE	(PLAT_PHYS_OFFSET + 0x00050000)
#define RTC_TIMER_BASE	(PLAT_PHYS_OFFSET + 0x00051000)
#define PMC_BASE	(PLAT_PHYS_OFFSET + 0x00051400)


#ifdef __cplusplus
}
#endif

#endif    /* __MEMORY_SUN20IW2P1_H */
