#include <stddef.h>
#include <stdint.h>
#include <io.h>
#include <FreeRTOSConfig.h>

#define SUNXI_CPUXCFG_BASE    0x09010000
#define SUNXI_CPUSCFG_BASE    0x07000400
#define CPU1_SOFT_ENT_REG(x)  (0x1c4 + (x * 4))
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

extern void secondary_cpu_start(void);
void mdelay(uint32_t ms);

static void smp_set_cpu_boot_entry(int cpu, uint32_t entry)
{
    writel(entry, SUNXI_CPUSCFG_BASE + CPU1_SOFT_ENT_REG(cpu));
    mdelay(10);

    isb();
    dsb();
    dmb();
}

static void smp_enable_cpu(int cpu)
{
    unsigned int value;
    
    /* assert cpu core reset low */
    value = readl(SUNXI_CPUXCFG_BASE + C0_RST_CTRL);
    value &= (~(0x1 << (CORE_RESET_OFFSET + cpu)));
    writel(value, SUNXI_CPUXCFG_BASE + C0_RST_CTRL);

    mdelay(10);
    /* L1RSTDISABLE hold low */
    value = readl(SUNXI_CPUXCFG_BASE + C0_CTRL_REG0);
    value &= (~(0x1 << (L1_RST_DISABLE_OFFSET + cpu)));
    writel(value, SUNXI_CPUXCFG_BASE + C0_CTRL_REG0);

    mdelay(20);

    /* Deassert core reset high */
    value = readl(SUNXI_CPUXCFG_BASE + C0_RST_CTRL);
    value |= (0x1 << (CORE_RESET_OFFSET + cpu));
    writel(value, SUNXI_CPUXCFG_BASE + C0_RST_CTRL);
    mdelay(10);

    isb();
    dsb();
    dmb();
}

void sunxi_spc_set_to_ns(void)
{
}

void sencond_cpu_bootup(void)
{
    smp_set_cpu_boot_entry(1, (uint32_t)secondary_cpu_start);
    portNOP();
    smp_enable_cpu(1);
    isb();
    dsb();
    dmb();
}
