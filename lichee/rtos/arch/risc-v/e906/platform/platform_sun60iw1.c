/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "platform_sun60iw1.h"

const char *irq_major_string[CLIC_IRQ_NUM] = {
    [0 ... 2] = NULL,
    "Machine_Software",
    [4 ... 6] = NULL,
    "CORET",
    [8 ... 10] = NULL,
    "Machine_External",
    [12 ... 15] = NULL,
    "E906_WDG",
    "E906_MBOX",
    "DSP_DoubleException",
    "DSP_PFdataIError",
    "DSP_WDG",
    "DSP_MBOX_IRQ_CPUX",
    "DSP_MBOX_IRQ_CPUS",
    "DSP_MBOX_IRQ_RISCV",
    "TZMA0",
    "DSP_TIMER0",
    "DSP_TIMER1",
    "DSP_TIMER2",
    "AHB0_HREADY_TOUT",     /* DSP AHB decoder0 timer out interrupt */
    "AHB1_HREADY_TOUT",     /* DSP AHB decoder1 timer out interrupt */
    "ADDA",     /* AUDIO codec interrupt */
    "DMIC",
    "I2S0",
    "I2S1",
    "I2S2",
    "I2S3",
    "SPDIF0",
    "DMAC1_IRQ_DSP_NS",
    "DMAC1_IRQ_DSP_S",
    "NPU",
    "TZMA1",
    "DSP_TIMER3",
    "DSP_TIMER4",
    "DSP_TIMER5",
    "DSP_PWM",
    "TZMA2",
    [46 ... 51] = NULL,
    "NMI",
    "R_PPU",
    "R_PPU1",
    "R_TWD",
    "R_WDG",
    "R_TIMER0",
    "R_TIMER1",
    "R_TIMER2",
    "R_TWI2",
    "ALARM",
    "GPIOL_S",
    "GPIOL_NS",
    "GPIOM_S",
    "GPIOM_NS",
    "R_UART0",
    "R_UART1",
    "R_TWI0",
    "R_TWI1",
    "R_CAN0",
    "R_IRRX",
    "R_PWM",
    "R_TZMA",
    "AHBS_HREADY_TOUT",
    "PCK600_CPU",
    "R_SPI",
    "R_SPINLOCK",
    "CPUS_MBOX_IRQ_CPUX",
    [79 ... 80] = NULL,
    "CPUS_MBOX_IRQ_RISCV",
    "CPUS_MBOX_IRQ_DSP",
    "INT_SCRI0(cpux_mbox_irq_rv_write)",      /* cpux_mbox_irq_rv_write */
    "INT_SCRI1(cpux_mbox_irq_rv_write)",      /* cpux_mbox_irq_rv_write */
    "INT_SCRI2(spinlock irq)",                /* spinlock irq */
    "INT_SCRI3(cpux_mbox_irq_dsp)",           /* cpux_mbox_irq_dsp */
    "INT_SCRI4(NULL)",
    "INT_SCRI5(dmac_irq1_ns)",                /* dmac_irq1_ns */
    "INT_SCRI6(dmac_irq1_s)",                 /* dmac_irq1_s */
    "INT_SCRI7",      /* gic 32 - 39 */
    "INT_SCRI8",      /* gic 40 - 47 */
    "INT_SCRI9",      /* gic 48 - 55 */
    "INT_SCRI10",     /* gic 56 - 63 */
    "INT_SCRI11",     /* gic 64 - 71 */
    "INT_SCRI12",     /* gic 72 - 79 */
    "INT_SCRI13",     /* gic 80 - 87 */
    "INT_SCRI14",     /* gic 88 - 95 */
    "INT_SCRI15",     /* gic 96 - 103 */
    "INT_SCRI16",     /* gic 104 - 111 */
    "INT_SCRI17",     /* gic 112 - 119 */
    "INT_SCRI18",     /* gic 120 - 127 */
    "INT_SCRI19",     /* gic 128 - 135 */
    "INT_SCRI20",     /* gic 136 - 143 */
    "INT_SCRI21",     /* gic 144 - 151 */
    "INT_SCRI22",     /* gic 152 - 159 */
    "INT_SCRI23",     /* gic 160 - 167 */
    "INT_SCRI24",     /* gic 168 - 175 */
    "INT_SCRI25",     /* gic 176 - 179 */
};

const char *irq_group_string[] = {
    /* group7 gic 32-39 */
    "CPUX_MBOX_R", "CPUS_MBOX_W",     "UART0",        "UART1",       "UART2",      "UART3",      "UART4",     "UART5",
    /* group8  gic 40-47 */
    "UARt6",       "UART7",           "TWI0",         "TWI1",        "TWI2",       "TWI3",       "TWI4",      "TWI5",
    /* group9  gic 48-55 */
    "SPI0",        "SPI1",            "SPI2",         "PWM0",        "SPI_FLASH",  "SPI3",       NULL,         NULL,
    /* group10 gic 56-63 */
    NULL,          NULL,              "IR_TX",        "IR_RX",       "LEDC",       "USB_OTG",    "USB_EHCI",   "USB_OHCI",
    /* group11 gic 64-71 */
    "USB1_EHCI",   "USB1_OHCI",       NULL,           "USB3.1",      "CAN0",       NULL,         "NAND",       "THS0",
    /* group12 gic 72-79 */
    "SMHC0",       "SMHC1",           "SMHC2",        "NSI",         "SMC",        NULL,         "GMAC0",      "GMAC1",
    /* group13 gic 80-87 */
    "CCU_FERR",    "AHB_HREADY_TOUT", "DMA0_CPUX_NS", "DMA0_CPUX_S", "CE_NS",      "CE_S",       "SPINLOCK",   "TIMER0",
    /* group14 gic 88-95 */
    "TIMER1",      "TIMER2",          "TIMER3",       "TIMER4",      "TIMER5",     "GPADC",      "THS1",       "WDG0",
    /* group15 gic 96-103 */
    "GPADC1",      "IOMMU",           "LRADC",        "GPIOA_NS",    "GPIOA_S",    "GPIOB_NS",   "GPIOB_S",    "GPIOC_NS",
    /* group16 gic 104-111 */
    "GPIOC_S",     "GPIOD_NS",        "GPIOD_S",      "GPIOE_NS",    "GPIOE_S",    "GPIOF_NS",   "GPIOF_S",    "GPIOG_NS",
    /* group17 gic 112-119 */
    "GPIOG_S",     "GPIOH_NS",        "GPIOH_S",      "GPIOI_NS",    "GPIOI_S",    "GPIOJ_NS",   "GPIOJ_S",    "DE",
    /* group18 gic 120-127 */
    "DE",          "HDMI_PHY",        "TCON0_LCD0",   "TV0",         "TCON0_LCD1", "HDMI",       "DSI0",       "DSI1",
    /* group19 gic 128-135 */
    "TV1",         "TCON1_LCD0",      "PCIE_EDMA0",   "PCIE_EDMA1",  "PCIE_EDMA2", "PCIE_EDMA3", "PCIE_EDMA4", "PCIE_EDMA5",
    /* group20 gic 136-143 */
    "PCIE_EDMA6",  "PCIE_EDMA7",      "PCIE_SII",     "PCIE_MSI",    "PCIE_EDMA8", "PCIE_EDMA9", "PCIE_EDMA10", "PCIE_EDMA11",
    /* group21 gic 144-151 */
    "PCIE_EDMA12", "PCIE_EDMA13",     "PCIE_EDMA14",  "PCIE_EDMA15", "GPU_EVENT",  "GPU_JOB",    "GPU_MMU",     "GPU",
    /* group22 gic 152-159 */
    "VE3",         "MEMC_DFS",        "CSI_DMA0",     "CSI_DMA1",    "CSI_DMA2",   "CSI_DMA3",    "CSI_VIPP0",  "CSI_VIPP1",
    /* group23 gic 160-167 */
    "CSI_VIPP2",   "CSI_VIPP3",       "CSI_PARSER0",  "CSI_PARSER1", "CSI_PARSER2", "CSI_ISP0",    "CSI_ISP1",   "CSI_ISP2",
    /* group24 gic 168-175 */
    "CSI_ISP3",    "CSI_CMB",         "CSI_TDM",      "CSI_TOP_PKT", "GPIOK_NS",   "GPIOK_S",     "PWM1",       "G2D",
    /* group25 gic 176-179 */
    "EDP",         "CSI_DMA4",        "CSI_DMA5",     "CSI_PARSER3", NULL,         NULL,          NULL,         NULL
};


