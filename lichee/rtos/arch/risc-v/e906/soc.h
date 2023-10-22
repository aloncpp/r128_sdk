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
#ifndef _SOC_H_
#define _SOC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IHS_VALUE
#define  IHS_VALUE    (24000000)
#endif

#ifndef EHS_VALUE
#define  EHS_VALUE    (24000000)
#endif


/* -------------------------  Interrupt Number Definition  ------------------------ */
typedef enum IRQn
{
    NMI_EXPn                        =   -2,      /* NMI Exception */
    /* ----------------------  SmartL Specific Interrupt Numbers  --------------------- */
    Machine_Software_IRQn           =   3,      /* Machine software interrupt */
    CORET_IRQn                      =   7,      /* core Timer Interrupt */
    Machine_External_IRQn           =   11,     /* Machine external interrupt */
    E906_WDG_IRQn                   =   16,
    E906_MBOX_IRQn                  =   17,     /* RISCV MSGBOX READ IRQ FOR RISCV */
    DSP_DoubleException             =   18,
    DSP_PFdataIError                =   19,
	DSP_WDG_IRQn                    =   20,
	DSP_MBOX_IRQ_CPUX_IRQn          =   21,
	DSP_MBOX_IRQ_CPUS_IRQn          =   22,
	DSP_MBOX_IRQ_RISCV_IRQn         =   23,
	TZMA0_IRQn                      =   24,
	DSP_TIMER0_IRQn                 =   25,
	DSP_TIMER1_IRQn                 =   26,
	DSP_TIMER2_IRQn                 =   27,
	AHB0_HREADY_TIME_OUT_IRQn       =   28,     /* DSP AHB decoder0 timer out interrupt */
	AHB1_HREADY_TIME_OUT_IRQn       =   29,     /* DSP AHB decoder1 timer out interrupt */
	ADDA_IRQn                       =   30,     /* AUDIO codec interrupt */
	DMIC_IRQn                       =   31,
	I2S0_IRQn                       =   32,
	I2S1_IRQn                       =   33,
	I2S2_IRQn                       =   34,
	I2S3_IRQn                       =   35,
	SPDIF0_IRQn                     =   36,
	DMAC1_IRQ_DSP_NS_IRQn           =   37,
	DMAC1_IRQ_DSP_S_IRQn            =   38,
	NPU_IRQn                        =   39,
	TZMA1_IRQn                      =   40,
	DSP_TIMER3_IRQn                 =   41,
	DSP_TIMER4_IRQn                 =   42,
	DSP_TIMER5_IRQn                 =   43,
	DSP_PWM_IRQn                    =   44,
	TZMA2_IRQn                      =   45,
	NMI_IRQn                        =   52,
	R_PPU_IRQn                      =   53,
	R_PPU1_IRQn                     =   54,
	R_TWD_IRQn                      =   55,
	R_WDG_IRQn                      =   56,
	R_TIMER0_IRQn                   =   57,
	R_TIMER1_IRQn                   =   58,
	R_TIMER2_IRQn                   =   59,
	R_TWI2_IRQn                     =   60,
	ALARM_IRQn                      =   61,
	GPIOL_S_IRQn                    =   62,
	GPIOL_NS_IRQn                   =   63,
	GPIOM_S_IRQn                    =   64,
	GPIOM_NS_IRQn                   =   65,
	R_UART0_IRQn                    =   66,
	R_UART1_IRQn                    =   67,
	R_TWI0_IRQn                     =   68,
	R_TWI1_IRQn                     =   69,
	R_CAN0_IRQn                     =   70,
	R_IRRX_IRQn                     =   71,
	R_PWM_IRQn                      =   72,
	R_TZMA_IRQn                     =   73,
	AHBS_HREADY_TOUT_IRQn           =   74,
	PCK600_CPU_IRQn                 =   75,
	R_SPI_IRQn                      =   76,
	R_SPINLOCK_IRQn                 =   77,
	CPUS_MBOX_IRQ_CPUX_IRQn         =   78,
	CPUS_MBOX_IRQ_RISCV_IRQn        =   81,
	CPUS_MBOX_IRQ_DSP_IRQn          =   82,
	INT_SCRI0_IRQn                  =   83,     /* cpux_mbox_irq_rv_write */
	INT_SCRI1_IRQn                  =   84,     /* cpux_mbox_irq_rv_write */
	INT_SCRI2_IRQn                  =   85,     /* spinlock irq */
	INT_SCRI3_IRQn                  =   86,     /* cpux_mbox_irq_dsp */
	INT_SCRI4_IRQn                  =   87,
	INT_SCRI5_IRQn                  =   88,     /* dmac_irq1_ns */
	INT_SCRI6_IRQn                  =   89,     /* dmac_irq1_s */
	INT_SCRI7_IRQn                  =   90,     /* gic 32 - 39 */
	INT_SCRI8_IRQn                  =   90,     /* gic 40 - 47 */
	INT_SCRI9_IRQn                  =   90,     /* gic 48 - 55 */
	INT_SCRI10_IRQn                 =   90,     /* gic 56 - 63 */
	INT_SCRI11_IRQn                 =   90,     /* gic 64 - 71 */
	INT_SCRI12_IRQn                 =   90,     /* gic 72 - 79 */
	INT_SCRI13_IRQn                 =   90,     /* gic 80 - 87 */
	INT_SCRI14_IRQn                 =   90,     /* gic 88 - 95 */
	INT_SCRI15_IRQn                 =   90,     /* gic 96 - 103 */
	INT_SCRI16_IRQn                 =   90,     /* gic 104 - 111 */
	INT_SCRI17_IRQn                 =   90,     /* gic 112 - 119 */
	INT_SCRI18_IRQn                 =   90,     /* gic 120 - 127 */
	INT_SCRI19_IRQn                 =   90,     /* gic 128 - 135 */
	INT_SCRI20_IRQn                 =   90,     /* gic 136 - 143 */
	INT_SCRI21_IRQn                 =   90,     /* gic 144 - 151 */
	INT_SCRI22_IRQn                 =   90,     /* gic 152 - 159 */
	INT_SCRI23_IRQn                 =   90,     /* gic 160 - 167 */
	INT_SCRI24_IRQn                 =   90,     /* gic 168 - 175 */
	INT_SCRI25_IRQn                 =   90,     /* gic 176 - 179 */
}
IRQn_Type;

#ifdef __cplusplus
}
#endif

#endif  /* _SOC_H_ */
