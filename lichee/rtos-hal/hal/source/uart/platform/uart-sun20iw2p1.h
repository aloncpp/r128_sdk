/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY¡¯S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS¡¯SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY¡¯S TECHNOLOGY.
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

#ifndef __UART_SUN20IW2_H__
#define __UART_SUN20IW2_H__

#include <interrupt.h>

#define UART_SCLK CLK_PCLK_SPC
#define UART_PCLK CLK_DEVICE

#define SUNXI_CLK_UART0 CLK_BUS_UART0
#define SUNXI_RST_UART0 RST_UART0

#define SUNXI_CLK_UART1 CLK_BUS_UART1
#define SUNXI_RST_UART1 RST_UART1

#define SUNXI_CLK_UART2 CLK_BUS_UART2
#define SUNXI_RST_UART2 RST_UART2

#define SUNXI_CLK_UART3 0
#define SUNXI_RST_UART3 0

#define SUNXI_CLK_UART4 0 /* no support */
#define SUNXI_RST_UART4 0

#define SUNXI_CLK_UART5 0 /* no support */
#define SUNXI_RST_UART5 0


#if defined(CONFIG_ARCH_RISCV_C906)
#define SUNXI_IRQ_UART0		(49)  /* 49 uart0 interrupt */
#define SUNXI_IRQ_UART1		(50)  /* 50 uart1 interrupt */
#define SUNXI_IRQ_UART2		(51)  /* 51 uart2 interrupt */
/* not used */
#define SUNXI_IRQ_UART3		(0)
#define SUNXI_IRQ_UART4		(0)
#define SUNXI_IRQ_UART5		(0)
#elif defined(CONFIG_ARCH_DSP)
#define SUNXI_IRQ_UART0		(RINTC_IRQ_MASK | 34)  /* 34 uart0 interrupt */
#define SUNXI_IRQ_UART1		(RINTC_IRQ_MASK | 35)  /* 35 uart1 interrupt */
#define SUNXI_IRQ_UART2		(RINTC_IRQ_MASK | 36)  /* 36 uart2 interrupt */
/* not used */
#define SUNXI_IRQ_UART3		(0)
#define SUNXI_IRQ_UART4		(0)
#define SUNXI_IRQ_UART5		(0)
#else
#define SUNXI_IRQ_UART0		(33)  /* 33 uart0 interrupt */
#define SUNXI_IRQ_UART1		(34)  /* 34 uart1 interrupt */
#define SUNXI_IRQ_UART2		(35)  /* 35 uart2 interrupt */
/* not used */
#define SUNXI_IRQ_UART3		(0)
#define SUNXI_IRQ_UART4		(0)
#define SUNXI_IRQ_UART5		(0)
#endif

/* base register infomation */
#define SUNXI_UART0_BASE	(0x40047000)
#define SUNXI_UART1_BASE	(0x40047400)
#define SUNXI_UART2_BASE	(0x40047800)
/* not used */
#define SUNXI_UART3_BASE	(0xffffffff)
#define SUNXI_UART4_BASE	(0xffffffff) /* no support */
#define SUNXI_UART5_BASE	(0xffffffff) /* no support */

#define UART_FIFO_SIZE		(64)
#define UART0_GPIO_FUNCTION	(5)
#define UART1_GPIO_FUNCTION	(2)
#define UART2_GPIO_FUNCTION	(2)
#define UART3_GPIO_FUNCTION	(2)
#define UART4_GPIO_FUNCTION	(2) /* no support */
#define UART5_GPIO_FUNCTION	(2) /* no support */

#define UART0_TX	GPIOA(16)
#define UART0_RX	GPIOA(17)

#define UART1_TX	GPIOB(14)
#define UART1_RX	GPIOB(15)

#define UART2_RTS	GPIOA(10)
#define UART2_CTS	GPIOA(11)
#define UART2_TX	GPIOA(12)
#define UART2_RX	GPIOA(13)

/* not used */
#define UART3_TX	GPIOA(12)
#define UART3_RX	GPIOA(13)

#define UART4_TX	GPIOA(12) /* no support */
#define UART4_RX	GPIOA(13) /* no support */

#define UART5_TX	GPIOA(12) /* no support */
#define UART5_RX	GPIOA(13) /* no support */


#endif /*__UART_SUN20IW2_P1__  */
