/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY��S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS��SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY��S TECHNOLOGY.
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

#ifndef __SPI_SUN8IW19_H__
#define __SPI_SUN8IW19_H__

#define SUNXI_DBI0_PBASE         0X05010000 /* 4K */
#define SUNXI_DBI1_PBASE         0X05011000 /* 4K */
#define SUNXI_DBI2_PBASE         0X05012000 /* 4K */
#define SUNXI_IRQ_DBI0                 86
#define SUNXI_IRQ_DBI1                 87
#define SUNXI_IRQ_DBI2                 88

#define DBI_MAX_NUM 1
#define DBI0_PARAMS \
{.reg_base = SUNXI_DBI0_PBASE, .irq_num = SUNXI_IRQ_DBI0, .gpio_num = 6, \
	.gpio_clk = GPIO_PC0, .gpio_mosi = GPIO_PC2, .gpio_miso = GPIO_PC3, \
	.gpio_cs0 = GPIO_PC1, .gpio_wp = GPIO_PC4, .gpio_hold = GPIO_PC5, \
	.mux = 4, .driv_level = GPIO_DRIVING_LEVEL2}
#define DBI1_PARAMS \
{.reg_base = SUNXI_DBI1_PBASE, .irq_num = SUNXI_IRQ_DBI1, .gpio_num = 4, \
	.gpio_clk = GPIO_PH0, .gpio_mosi = GPIO_PH1, .gpio_miso = GPIO_PH2, \
	.gpio_cs0 = GPIO_PH3, .gpio_wp = 0, .gpio_hold = 0, \
	.mux = 4, .driv_level = GPIO_DRIVING_LEVEL2}
#define DBI2_PARAMS \
{.reg_base = SUNXI_DBI2_PBASE, .irq_num = SUNXI_IRQ_DBI2, .gpio_num = 4, \
	.gpio_clk = GPIO_PE18, .gpio_mosi = GPIO_PE19, .gpio_miso = GPIO_PE20, \
	.gpio_cs0 = GPIO_PE21, .gpio_wp = 0, .gpio_hold = 0, \
	.mux = 4, .driv_level = GPIO_DRIVING_LEVEL2}
#define CLK_SPI2 0//RESEVER
#define CLK_BUS_SPI2 0//RESEVER
#define RST_BUS_SPI2 0//RESEVER
#define SUNXI_CLK_SPI(x)	CLK_SPI##x
#define SUNXI_CLK_RST_SPI(x)	RST_BUS_SPI##x
#define SUNXI_CLK_BUS_SPI(x)	CLK_BUS_SPI##x
#define SUNXI_CLK_PLL_SPI CLK_PLL_PERIPH0
#endif /*__SPI_SUN8IW19_H__  */
