/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
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

#ifndef __MSGBOX_SUN50IW11_H__
#define __MSGBOX_SUN50IW11_H__

/* config for DSP */
#if defined(CONFIG_CORE_DSP0)
#include <interrupt.h>
#include <hal_prcm.h>

#define MSGBOX0_CPU_DSP0	0x03003000
#define MSGBOX1_CPU_DSP1	0x03008000
#define MSGBOXR_DSP0_DSP1	0x07092000

#if defined(CONFIG_CORE_DSP0)	/* DSP0 */
#define MSGBOX_CPU_DSP		MSGBOX0_CPU_DSP0
#define MSGBOX_DSP_DSP		MSGBOXR_DSP0_DSP1

#define MSGBOX_CPU_DSP_USER	(1)
#define MSGBOX_DSP_DSP_USER	(0)

#elif defined(CONFIG_CORE_DSP1)	/* DSP1 */
#define MSGBOX_CPU_DSP		MSGBOX1_CPU_DSP1
#define MSGBOX_DSP_DSP		MSGBOXR_DSP0_DSP1

#define MSGBOX_CPU_DSP_USER	(1)
#define MSGBOX_DSP_DSP_USER	(1)

#else				/* error */
#error "must select dsp core"
#endif

/* for prcm and ccmu compatibility */
#define HAL_CLK_PERIPH_MSGBOX0	CCU_MOD_CLK_MSGBOX0
#define HAL_CLK_PERIPH_MSGBOX1	CCU_MOD_CLK_MSGBOX1
#define HAL_CLK_PERIPH_MSGBOXR	CCU_MOD_CLK_MSGBOXR
#endif /* CONFIG_CORE_DSP0 */

#if defined(CONFIG_CORE_DSP0)
#define SUNXI_DSP_IRQ_R_MSGBOX_DSP	3
#define SUNXI_DSP_IRQ_MSGBOX0_DSP	4
#else
#define SUNXI_DSP_IRQ_R_MSGBOX_DSP	3
#define SUNXI_DSP_IRQ_MSGBOX0_DSP	4
#endif


#endif /*__MSGBOX_SUN50IW11_H__  */
