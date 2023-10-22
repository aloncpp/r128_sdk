/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.

 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.

 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.


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

#include <stdio.h>
#include "sunxi_hal_common.h"
#include "stdint.h"
#include "mbus.h"

uint32_t mbus_get_cpu_ddr(void)
{
	return MBUS_PMU->MC_CPU_BWCR;
}

uint32_t mbus_get_gpu_ddr(void)
{
#ifdef CONFIG_DRIVERS_MBUS_GPU_BW
	return MBUS_PMU->MC_GPU_BWCR;
#else
    return 0;
#endif
}

uint32_t mbus_get_rv_sys_ddr(void)
{
#ifdef CONFIG_DRIVERS_MBUS_RV_SYS_BW
	return MBUS_PMU->MC_RV_SYS_BWCR;
#else
    return 0;
#endif
}

uint32_t mbus_get_mahb_ddr(void)
{
#ifdef CONFIG_DRIVERS_MBUS_MAHB_BW
	return MBUS_PMU->MC_MAHB_BWCR;
#else
    return 0;
#endif
}

uint32_t mbus_get_dma_ddr(void)
{
#ifdef CONFIG_DRIVERS_MBUS_DMA_BW
	return MBUS_PMU->MC_DMA_BWCR;
#else
    return 0;
#endif
}

uint32_t mbus_get_dma0_ddr(void)
{
#ifdef CONFIG_DRIVERS_MBUS_DMA0_BW
	return MBUS_PMU->MC_DMA0_BWCR;
#else
    return 0;
#endif
}

uint32_t mbus_get_dma1_ddr(void)
{
#ifdef CONFIG_DRIVERS_MBUS_DMA1_BW
	return MBUS_PMU->MC_DMA1_BWCR;
#else
    return 0;
#endif
}

uint32_t mbus_get_ce_ddr(void)
{
#ifdef CONFIG_DRIVERS_MBUS_CE_BW
	return MBUS_PMU->MC_CE_BWCR;
#else
    return 0;
#endif
}

uint32_t mbus_get_tvd_ddr(void)
{
#ifdef CONFIG_DRIVERS_MBUS_TVD_BW
	return MBUS_PMU->MC_TVD_BWCR;
#else
    return 0;
#endif
}

uint32_t mbus_get_csi_ddr(void)
{
#ifdef CONFIG_DRIVERS_MBUS_CSI_BW
	return MBUS_PMU->MC_CSI_BWCR;
#else
    return 0;
#endif
}

uint32_t mbus_get_dsp_sys_ddr(void)
{
#ifdef CONFIG_DRIVERS_MBUS_DSP_SYS_BW
	return MBUS_PMU->MC_DSP_SYS_BWCR;
#else
    return 0;
#endif
}

uint32_t mbus_get_g2d_ddr(void)
{
#ifdef CONFIG_DRIVERS_MBUS_G2D_BW
	return MBUS_PMU->MC_G2D_BWCR;
#else
    return 0;
#endif
}

uint32_t mbus_get_di_ddr(void)
{
#ifdef CONFIG_DRIVERS_MBUS_DI_BW
	return MBUS_PMU->MC_DI_BWCR;
#else
    return 0;
#endif
}

uint32_t mbus_get_iommu_ddr(void)
{
#ifdef CONFIG_DRIVERS_MBUS_DI_BW
	return MBUS_PMU->MC_IOMMU_BWCR;
#else
    return 0;
#endif
}

uint32_t mbus_get_ve_ddr(void)
{
#ifdef CONFIG_DRIVERS_MBUS_VE_BW
	return MBUS_PMU->MC_VE_BWCR;
#else
    return 0;
#endif
}

uint32_t mbus_get_de_ddr(void)
{
#ifdef CONFIG_DRIVERS_MBUS_DE_BW
	return MBUS_PMU->MC_DE_BWCR;
#else
    return 0;
#endif
}

uint32_t mbus_get_disp_ddr(void)
{
#ifdef CONFIG_DRIVERS_MBUS_DISP_BW
	return MBUS_PMU->MC_DISP_BWCR;
#else
    return 0;
#endif
}

uint32_t mbus_get_oth_ddr(void)
{
	return MBUS_PMU->MC_OTHER_BWCR;
}

uint32_t mbus_get_total_ddr(void)
{
	return MBUS_PMU->MC_TOTAL_BWCR;
}

uint32_t mbus_get_window(void)
{
#ifdef MBUS_PMU_DBGCR
	uint32_t windows_us = (hal_readl(MBUS_PMU_BASE) >> 16);
	return windows_us;
#else
	return 0;
#endif
}

void mbus_pmu_enable(void)
{
	MBUS_PMU->MC_MCGCR = MBUS_PMU->MC_MCGCR & ~0x1;

/* changed for R128,20220601
#ifdef CONFIG_ARCH_SUN20IW2
	hal_writel((hal_readl(MBUS_PMU_DBGCR) & ~(0xFF<<8)) | (0x40 << 8), MBUS_PMU_DBGCR); //hardcode for fpga
	hal_writel(1, MBUS_PMU_TMR); //hardcode for 16M clk/8M psram/2M msi
#endif
*/
	//unit : 1Byte.
	//window: 50ms window for total
	MBUS_PMU->MC_MCGCR = (((50 * 1000) << 16) | 0x1);
}
