/*
 * =====================================================================================
 *
 *       Filename:  secondary.c
 *
 *    Description:  bootup secondary core on R328.
 *
 *        Version:  1.0
 *        Created:  2019年07月10日 14时32分37秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  czl init file
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stddef.h>
#include <stdint.h>
#include <io.h>
#include <FreeRTOSConfig.h>

#define RTC_BASE              0x07000000
#define sunxi_cpucfg_base     0x09010000
#define CPU1_SOFT_ENT_REG     (0x258)
#define C0_RST_CTRL           (0x00)
#define C0_CTRL_REG0          (0x10)
#define C0_CPU_STATUS         (0X80)
#define CORE_RESET_OFFSET     (0x00)
#define L1_RST_DISABLE_OFFSET (0x00)
#define STANDBYWFI_OFFSET     (0x10)

#define SPC_BASE		(0x03008000)
#define SUNXI_CCM_BASE		(0x03001000)
#define SUNXI_DMA_BASE  	(0x03002000)
#define SPC_STA_REG(x)		(SPC_BASE+0x10*(x)+0x0)
#define SPC_SET_REG(x)		(SPC_BASE+0x10*(x)+0x4)
#define SPC_CLR_REG(x)		(SPC_BASE+0x10*(x)+0x8)

#define portNOP() __asm volatile( "NOP" )

void mdelay(uint32_t ms);
extern void secondary_cpu_start(void);

static void smp_set_cpu1_boot_entry(uint32_t entry)
{
    writel(entry, RTC_BASE + CPU1_SOFT_ENT_REG);
    mdelay(10);

    isb();
    dsb();
    dmb();
}

static void smp_enable_cpu1(int cpu)
{
    unsigned int value;
    
    /* assert cpu core reset low */
    value = readl(sunxi_cpucfg_base + C0_RST_CTRL);
    value &= (~(0x1 << (CORE_RESET_OFFSET + cpu)));
    writel(value, sunxi_cpucfg_base + C0_RST_CTRL);
    
    mdelay(10);
    /* L1RSTDISABLE hold low */
    value = readl(sunxi_cpucfg_base + C0_CTRL_REG0);
    value &= (~(0x1 << (L1_RST_DISABLE_OFFSET + cpu)));
    writel(value, sunxi_cpucfg_base + C0_CTRL_REG0);
    
    mdelay(20);
    
    /* Deassert core reset high */
    value = readl(sunxi_cpucfg_base + C0_RST_CTRL);
    value |= (0x1 << (CORE_RESET_OFFSET + cpu));
    writel(value, sunxi_cpucfg_base + C0_RST_CTRL);
    mdelay(10);

    isb();
    dsb();
    dmb();
}

void sunxi_spc_set_to_ns(void)
{
    /* set master0~13 to non_secure */
    writel(0x1801e023, SPC_SET_REG(0));
    writel(0x00000005, SPC_SET_REG(2));
    writel(0x00100319, SPC_SET_REG(4));
    writel(0x00000000, SPC_SET_REG(5));
    writel(0x017D3703, SPC_SET_REG(6));
    writel(0x00000000, SPC_SET_REG(7));
    writel(0x0000030F, SPC_SET_REG(8));
    writel(0x00000000, SPC_SET_REG(9));
    writel(0x03590303, SPC_SET_REG(10));
    writel(0x00000000, SPC_SET_REG(11));
    writel(0x00000000, SPC_SET_REG(12));
 
    /*dma reset*/
    writel(readl(SUNXI_CCM_BASE+0x70c) | (1 << 16), (SUNXI_CCM_BASE+0x70c));
    udelay(20);
    /*gating clock for dma pass*/
    writel(readl(SUNXI_CCM_BASE+0x70c) | (1 << 0), (SUNXI_CCM_BASE+0x70c));
 
    /* set ccmu security switch: set mbus_sec bus_sec pll_sec to non-sec */
    writel(0x7, SUNXI_CCM_BASE+0xf00);
 
    /* set dma security switch: set DMA channel0-7 to non-sec */
    writel(0xfff, SUNXI_DMA_BASE+0x20);
}

void sencond_cpu_bootup(void)
{
    smp_set_cpu1_boot_entry((uint32_t)secondary_cpu_start);
    portNOP();
    smp_enable_cpu1(1);
    isb();
    dsb();
    dmb();
}
