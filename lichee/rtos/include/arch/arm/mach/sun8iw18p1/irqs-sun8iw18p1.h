/*
 * arch/arm/mach-sunxi/include/mach/sun8i/irqs-sun8iw8p1.h
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

#ifndef __IRQS_SUN8I_W18P1_H
#define __IRQS_SUN8I_W18P1_H

#include "arch/arm/mach/irqs.h"
#define SUNXI_IRQ_NMI                  (SUNXI_GIC_START + 32)  /* 64 nmi */
#define SUNXI_IRQ_DMA                  (SUNXI_GIC_START + 34)  /* 66 dma */
#define SUNXI_IRQ_MAD_WAKE             (SUNXI_GIC_START + 35)  /* 67 mad wake interrupt */
#define SUNXI_IRQ_MAD_DATA_REQ         (SUNXI_GIC_START + 36)  /* 68 mad data request interrupt */
#define SUNXI_IRQ_USBOTG               (SUNXI_GIC_START + 37)  /* 69 usb otg interrupt */
#define SUNXI_IRQ_USBEHCI0             (SUNXI_GIC_START + 38)  /* 70 usb otg ehci interrupt */
#define SUNXI_IRQ_USBOHCI0             (SUNXI_GIC_START + 39)  /* 71 usb otg ohci interrupt */
#define SUNXI_IRQ_GPIOB                (SUNXI_GIC_START + 43)  /* 75 gpiob interrupt */
#define SUNXI_IRQ_GPIOE                (SUNXI_GIC_START + 44)  /* 76 gpioe interrupt */
#define SUNXI_IRQ_GPIOG                (SUNXI_GIC_START + 46)  /* 78 gpiog interrupt */
#define SUNXI_IRQ_GPIOH                (SUNXI_GIC_START + 47)  /* 79 gpioh interrupt */
#define SUNXI_IRQ_GPADC                (SUNXI_GIC_START + 48)  /* 80 gpadc interrupt */
#define SUNXI_IRQ_THERMAL              (SUNXI_GIC_START + 49)  /* 81 thermal interrupt */
#define SUNXI_IRQ_LRADC                (SUNXI_GIC_START + 50)  /* 82 lradc interrupt */
#define SUNXI_IRQ_SPDIF                (SUNXI_GIC_START + 51)  /* 83 spdif interrupt */
#define SUNXI_IRQ_DMIC                 (SUNXI_GIC_START + 52)  /* 84 dmic interrupt */
#define SUNXI_IRQ_MSI                  (SUNXI_GIC_START + 54)  /* 86 msi interrupt */
#define SUNXI_IRQ_SMC                  (SUNXI_GIC_START + 55)  /* 87 smc interrupt */
#define SUNXI_IRQ_WDOG                 (SUNXI_GIC_START + 56)  /* 88 watchdog interrupt */
#define SUNXI_IRQ_CCU_FREE             (SUNXI_GIC_START + 57)  /* 89 ccu_free interrupt */
#define SUNXI_IRQ_BUS_TIMEOUT          (SUNXI_GIC_START + 58)  /* 90 bus timeout interrupt */
#define SUNXI_IRQ_BUS_PSI              (SUNXI_GIC_START + 59)  /* 91 psi interrupt */
#define SUNXI_IRQ_BUS_LEDC             (SUNXI_GIC_START + 60)  /* 92 ledc interrupt */
#define SUNXI_IRQ_AUDIO_CODEC_DAC      (SUNXI_GIC_START + 61)  /* 93 audio codec dac interrupt */
#define SUNXI_IRQ_AUDIO_CODEC_ADC      (SUNXI_GIC_START + 62)  /* 94 audio codec adc interrupt */
#define SUNXI_IRQ_NAND0                (SUNXI_GIC_START + 63)  /* 95 nand0 interrupt */
#define SUNXI_IRQ_CE_NS                (SUNXI_GIC_START + 65)  /* 97 ce ns interrupt */
#define SUNXI_IRQ_CE                   (SUNXI_GIC_START + 66)  /* 98 ce interrupt */
#define SUNXI_IRQ_PCM0                 (SUNXI_GIC_START + 67)  /* 99 i2s/pcm0 interrupt */
#define SUNXI_IRQ_PCM1                 (SUNXI_GIC_START + 68)  /* 100 i2s/pcm1 interrupt */
#define SUNXI_IRQ_PCM2                 (SUNXI_GIC_START + 69)  /* 101 i2s/pcm2 interrupt */
#define SUNXI_IRQ_TWI0                 (SUNXI_GIC_START + 70)  /* 102 twi0 interrupt */
#define SUNXI_IRQ_TWI1                 (SUNXI_GIC_START + 71)  /* 103 twi1 interrupt */
#define SUNXI_IRQ_SMHC0                (SUNXI_GIC_START + 74)  /* 106 smhc interrupt */
#define SUNXI_IRQ_UART0                (SUNXI_GIC_START + 76)  /* 108 uart0 interrupt */
#define SUNXI_IRQ_UART1                (SUNXI_GIC_START + 77)  /* 109 uart1 interrupt */
#define SUNXI_IRQ_UART2                (SUNXI_GIC_START + 78)  /* 110 uart2 interrupt */
#define SUNXI_IRQ_UART3                (SUNXI_GIC_START + 79)  /* 111 uart3 interrupt */
#define SUNXI_IRQ_SPI0                 (SUNXI_GIC_START + 81)  /* 113 spi0 interrupt */
#define SUNXI_IRQ_SPI1                 (SUNXI_GIC_START + 82)  /* 114 spi1 interrupt */
#define SUNXI_IRQ_HSTIMER0             (SUNXI_GIC_START + 83)  /* 115 hstimer0 interrupt */
#define SUNXI_IRQ_HSTIMER1             (SUNXI_GIC_START + 84)  /* 116 hstimer1 interrupt */
#define SUNXI_IRQ_TIMER0               (SUNXI_GIC_START + 85)  /* 117 timer0 interrupt */
#define SUNXI_IRQ_TIMER1               (SUNXI_GIC_START + 86)  /* 118 timer1 interrupt */

#define SUNXI_IRQ_MAX           (SUNXI_GIC_START + 256)

#endif    /* __IRQS_SUN8I_W18P1_H */
