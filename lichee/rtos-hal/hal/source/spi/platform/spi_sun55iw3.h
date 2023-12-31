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

#ifndef __SPI_SUN55IW3_H__
#define __SPI_SUN55IW3_H__

#include <hal_clk.h>

#define SUNXI_RSPI_PBASE         (0x07092000ul) /* 4K */
#ifdef CONFIG_ARCH_RISCV
#define SUNXI_IRQ_RSPI                 76
#elif defined(CONFIG_ARCH_DSP)
#define SUNXI_IRQ_RSPI                 16
#endif

#define SPI_MAX_NUM 1
#define SPI0_PARAMS \
{	.port = 0, \
	.reg_base = SUNXI_RSPI_PBASE, .irq_num = SUNXI_IRQ_RSPI, .gpio_num = 4, \
	.pclk_pll_type = HAL_SUNXI_CCU, .pclk_pll_id = CLK_PLL_PERI0_300M, \
	.pclk_hosc_type = HAL_SUNXI_FIXED_CCU, .pclk_hosc_id = CLK_SRC_HOSC24M, \
	.bus_type = HAL_SUNXI_R_CCU, .bus_id = CLK_BUS_R_SPI, \
	.mclk_type = HAL_SUNXI_R_CCU, .mclk_id = CLK_R_SPI, \
	.reset_type = HAL_SUNXI_R_RESET, .reset_id = RST_R_SPI, \
	.gpio_clk = GPIOL(11), .gpio_mosi = GPIOL(12), .gpio_miso = GPIOL(13), \
	.gpio_cs0 = GPIOL(10), .mux = 6, .driv_level = GPIO_DRIVING_LEVEL2, \
}

#endif /* __SPI_SUN55IW3_H__ */
