/*
* Copyright (c) 2021-2027 Allwinner Technology Co., Ltd. ALL rights reserved.
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

#ifndef __TWI_SUN55IW3_H__
#define __TWI_SUN55IW3_H__

#define SUNXI_IRQ_TWI0 -1
#define SUNXI_IRQ_TWI1 -1
#define SUNXI_IRQ_TWI2 -1
#define SUNXI_IRQ_TWI3 -1
#define SUNXI_IRQ_TWI4 -1
#define SUNXI_IRQ_TWI5 -1
#if defined(CONFIG_ARCH_RISCV)
#define SUNXI_IRQ_S_TWI0  MAKE_IRQn(68, 0)
#define SUNXI_IRQ_S_TWI1  MAKE_IRQn(69, 0)
#define SUNXI_IRQ_S_TWI2  MAKE_IRQn(60, 0)
#else
#define SUNXI_IRQ_S_TWI0 13
#define SUNXI_IRQ_S_TWI1 14
#define SUNXI_IRQ_S_TWI2 15
#endif

/** the base address of TWI*/
#define SUNXI_TWI0_PBASE 0x02502200
#define SUNXI_TWI1_PBASE 0x02502400
#define SUNXI_TWI2_PBASE 0x02502800
#define SUNXI_TWI3_PBASE 0x02502c00
#define SUNXI_TWI4_PBASE 0x02503000
#define SUNXI_TWI5_PBASE 0x02503400
#define SUNXI_S_TWI0_PBASE 0x07081400
#define SUNXI_S_TWI1_PBASE 0x07081800
#define SUNXI_S_TWI2_PBASE 0x07081c00

#define TWI0_PIN_MUXSEL 2
#define TWI1_PIN_MUXSEL 2
#define TWI2_PIN_MUXSEL 2
#define TWI3_PIN_MUXSEL 2
#define TWI4_PIN_MUXSEL 2
#define TWI5_PIN_MUXSEL 2
#define S_TWI0_PIN_MUXSEL 2
#define S_TWI1_PIN_MUXSEL 2
#define S_TWI2_PIN_MUXSEL 2
#define TWI_DISABLE_PIN_MUXSEL 15
#define TWI_PULL_STATE 1
#define TWI_DRIVE_STATE 0

#define TWI0_SCK GPIOH(0)
#define TWI0_SDA GPIOH(1)
#define TWI1_SCK GPIOE(11)
#define TWI1_SDA GPIOE(12)
#define TWI2_SCK GPIOE(1)
#define TWI2_SDA GPIOE(2)
#define TWI3_SCK GPIOE(3)
#define TWI3_SDA GPIOE(4)
#define TWI4_SCK GPIOE(13)
#define TWI4_SDA GPIOE(14)
#define TWI5_SCK GPIOB(11)
#define TWI5_SDA GPIOB(12)
#define S_TWI0_SCK GPIOL(0)
#define S_TWI0_SDA GPIOL(1)
#define S_TWI1_SCK GPIOM(2)
#define S_TWI1_SDA GPIOM(3)
#define S_TWI2_SCK GPIOL(12)
#define S_TWI2_SDA GPIOL(13)

#define SUNXI_TWI_CLK_TYPE HAL_SUNXI_R_CCU
#define SUNXI_TWI_RESET_TYPE HAL_SUNXI_R_RESET

#define SUNXI_CLK_TWI(x)	CLK_TWI##x
#define SUNXI_CLK_RST_TWI(x)	RST_BUS_TWI##x
#define SUNXI_R_CLK_TWI(x)	CLK_BUS_R_TWI##x
#define SUNXI_R_CLK_RST_TWI(x)	RST_R_TWI##x

#endif /* __TWI_SUN55IW3_H__ */

