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

#ifndef __SPI_SUN20IW2_H__
#define __SPI_SUN20IW2_H__

#define SUNXI_SPI0_PBASE         (0x40009000ul) /* 4K */
#define SUNXI_SPI1_PBASE         (0x4000f000ul) /* 4K */

#ifdef CONFIG_ARCH_ARM_CORTEX_M33
#define SUNXI_IRQ_SPI0                 45
#define SUNXI_IRQ_SPI1                 46
#elif defined(CONFIG_ARCH_RISCV)
#define SUNXI_IRQ_SPI0                 61
#define SUNXI_IRQ_SPI1                 62
#endif

#define SPI_MAX_NUM 2

#define SPI0_PARAMS \
{	.port = 0, \
	.reg_base = SUNXI_SPI0_PBASE, .irq_num = SUNXI_IRQ_SPI0, .gpio_num = 6, \
	.pclk_pll_type = HAL_SUNXI_AON_CCU, .pclk_pll_id = CLK_DEVICE, \
	.pclk_hosc_type = HAL_SUNXI_AON_CCU, .pclk_hosc_id = CLK_HOSC, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_SPI0, \
	.mclk_type = HAL_SUNXI_CCU, .mclk_id = CLK_SPI0, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_SPI0, \
	.gpio_clk = GPIOB(6), .gpio_mosi = GPIOB(5), .gpio_miso = GPIOB(15), \
	.gpio_cs0 = GPIOB(4), .gpio_wp = GPIOB(14), .gpio_hold = GPIOB(7), \
	.mux = 4, .driv_level = GPIO_DRIVING_LEVEL2, \
}

#define SIP_SPI0_PARAMS \
{	.port = 0, \
	.reg_base = SUNXI_SPI0_PBASE, .irq_num = SUNXI_IRQ_SPI0, .gpio_num = 6, \
	.pclk_pll_type = HAL_SUNXI_AON_CCU, .pclk_pll_id = CLK_DEVICE, \
	.pclk_hosc_type = HAL_SUNXI_AON_CCU, .pclk_hosc_id = CLK_HOSC, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_SPI0, \
	.mclk_type = HAL_SUNXI_CCU, .mclk_id = CLK_SPI0, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_SPI0, \
	.gpio_clk = GPIOB(13), .gpio_mosi = GPIOB(10), .gpio_miso = GPIOB(11), \
	.gpio_cs0 = GPIOB(12), .gpio_wp = GPIOB(8), .gpio_hold = GPIOB(9), \
	.mux = 3, .driv_level = GPIO_DRIVING_LEVEL2, \
}

#define SPI1_PARAMS \
{	.port = 1, \
	.reg_base = SUNXI_SPI1_PBASE, .irq_num = SUNXI_IRQ_SPI1, .gpio_num = 6, \
	.pclk_pll_type = HAL_SUNXI_AON_CCU, .pclk_pll_id = CLK_DEVICE, \
	.pclk_hosc_type = HAL_SUNXI_AON_CCU, .pclk_hosc_id = CLK_HOSC, \
	.bus_type = HAL_SUNXI_CCU, .bus_id = CLK_BUS_SPI1, \
	.mclk_type = HAL_SUNXI_CCU, .mclk_id = CLK_SPI1, \
	.reset_type = HAL_SUNXI_RESET, .reset_id = RST_SPI1, \
	.gpio_clk = GPIOA(13), .gpio_mosi = GPIOA(18), .gpio_miso = GPIOA(21), \
	.gpio_cs0 = GPIOA(12), .gpio_wp = GPIOA(20), .gpio_hold = GPIOA(19), \
	.mux = 6, .driv_level = GPIO_DRIVING_LEVEL2, \
}

#endif /*__SPI_SUN8IW20_H__  */
