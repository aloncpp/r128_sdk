/*
 * arch/arm/mach-sunxi/include/mach/sun8i/memory-sun8iw8p1.h
 *
 * Copyright (c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: Sugar <shuge@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __PLATFORM_SUN8I_W18P1_H
#define __PLATFORM_SUN8I_W18P1_H

/*
 *  device physical addresses
 */

#define SUNXI_SYS_CFG_PBASE      0X03000000 /* 4K */
#define SUNXI_CCMU_PBASE         0X03001000 /* 4K */
#define SUNXI_DMAC_PBASE         0X03002000 /* 4K */
#define SUNXI_HSTIMER_PBASE      0X03005000 /* 4K */
#define SUNXI_SID_PBASE          0X03006000 /* 4K */
#define SUNXI_SPC_PBASE          0X03008000 /* 1K */
#define SUNXI_TIMER_PBASE        0X03009000 /* 1K */
#define SUNXI_PWM_PBASE          0X0300A000 /* 1K */
//#define SUNXI_GPIO_PBASE         0X0300B000 /* 1K */
#define SUNXI_PSI_PBASE          0X0300C000 /* 1K */
#define SUNXI_DCU_PBASE          0X03010000 /* 64K */
#define SUNXI_GIC_PBASE          0X03020000 /* 64K */
#define SUNXI_RTC_PBASE          0X07000000 /* 1K */
#define SUNXI_NAND0_PBASE        0X04011000 /* 4K */
#define SUNXI_SMHC1_PBASE        0X04021000 /* 4K */
#define SUNXI_MSI_CTRL_PBASE     0X04002000 /* 4K */
#define SUNXI_DRAM_PHY_PBASE     0X04003000 /* 12K */
#define SUNXI_UART0_PBASE        0X05000000 /* 1K */
#define SUNXI_UART1_PBASE        0X05000400 /* 1K */
#define SUNXI_UART2_PBASE        0X05000800 /* 1K */
#define SUNXI_UART3_PBASE        0X05000c00 /* 1K */
#define SUNXI_TWI0_PBASE         0X05002000 /* 1K */
#define SUNXI_TWI1_PBASE         0X05002400 /* 1K */
#define SUNXI_SPI0_PBASE         0X05010000 /* 4K */
#define SUNXI_SPI1_PBASE         0X05011000 /* 4K */
#define SUNXI_GPADC_PBASE        0X05070000 /* 1K */
#define SUNXI_THS_PBASE          0X05070400 /* 1K */
#define SUNXI_LRADC_PBASE        0X05070800 /* 1K */
#define SUNXI_PCM0_PBASE         0X05090000 /* 4K */
#define SUNXI_PCM1_PBASE         0X05091000 /* 4K */
#define SUNXI_PCM2_PBASE         0X05092000 /* 4K */
#define SUNXI_SPDIF_PBASE        0X05093000 /* 1K */
#define SUNXI_DMIC_PBASE         0X05095000 /* 1K */
#define SUNXI_AUDIO_CODEC_PBASE  0X05096000 /* 4K */
#define SUNXI_USB_OTG_PBASE      0X05100000 /* 1M */
#define SUNXI_MAD_PBASE          0X05400000 /* 4K */
#define SUNXI_MAD_SRAM_PBASE     0X05480000 /* 128K */

#define SUNXI_LEDC_PBASE         0X06700000 /* 1K */
#define SUNXI_CPU_SYS_CFG_PBASE  0X08100000 /* 1K */
#define SUNXI_TIMESTAMP_STA_PBASE   0X08110000 /* 4K */
#define SUNXI_TIMESTAMP_CTRL_PBASE  0X08120000 /* 4K */
#define SUNXI_IDC_PBASE             0X08130000 /* 3K */
#define SUNXI_C0_CPUX_CFG_PBASE     0X09010000 /* 1K */
#define SUNXI_C0_CPUX_MBIST_PBASE   0X09020000 /* 4K */

#define SUNXI_SPI_NUM         2 /* 2 controller */
#define SUNXI_SPI_REG_SIZE    0x1000 /* controler reg sized */
#define SUNXI_I2C_NUM         2 /* 2 controller */
#define SUNXI_I2C_REG_SIZE    0x400 /* controler reg sized */

#endif    /* __PLATFORM_SUN8I_W8P1_H */
