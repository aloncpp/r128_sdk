#include <stdio.h>
#include <stdint.h>

static inline void write32(uint32_t val, uint32_t addr)
{
    *(volatile uint32_t *)addr = val;    
}

static inline uint32_t read32(uint32_t addr)
{
    return *(volatile uint32_t *)addr;
}

#define GIC_BASE                0x03020000     
#define GICC_OFFSET             0x2000                                                                                                                                                         
#define GICD_OFFSET             0x1000
#define GICD_BASE               (GIC_BASE + GICD_OFFSET)
#define GICC_BASE               (GIC_BASE + GICC_OFFSET)

#define GICD_ICENABLER(n) (0x180 + (n) * 4)
#define GICD_ISENABLER(n) (0x100 + (n) * 4)
/* GIC cpuif registers */
#define GIC_CPU_IF_CTRL         ( 0x000)
#define GIC_INT_PRIO_MASK       ( 0x004)
#define GIC_BINARY_POINT        ( 0x008)
#define GIC_INT_ACK_REG         ( 0x00c)
#define GIC_END_INT_REG         ( 0x010)
#define GIC_RUNNING_PRIO        ( 0x014)
#define GIC_HIGHEST_PENDINT     ( 0x018)
#define GIC_DEACT_INT_REG       ( 0x1000)
#define GIC_AIAR_REG            ( 0x020)
#define GIC_AEOI_REG            ( 0x024)
#define GIC_AHIGHEST_PENDINT    ( 0x028)
 
 
/* Offsets from gic.gicc_base */
#define GICC_CTLR               (0x000)
#define GICC_PMR                (0x004)
#define GICC_IAR                (0x00C)
#define GICC_EOIR               (0x010)
 
#define GICC_CTLR_ENABLEGRP0    (1 << 0)
#define GICC_CTLR_ENABLEGRP1    (1 << 1)
#define GICC_CTLR_FIQEN         (1 << 3)
 
#define EOI_MODE_NS             (1 << 10)
#define EOI_MODE_S              (1 << 9)
#define IRQ_BYP_DIS_GRP1        (1 << 8)
#define FIQ_BYP_DIS_GRP1        (1 << 7)
#define IRQ_BYP_DIS_GRP0        (1 << 6)
#define FIQ_BYP_DIS_GRP0        (1 << 5)
#define CBPR                    (1 << 4)
#define ACK_CTL                 (1 << 2)
 
#define GICD_CTLR               (0x000)
#define GICD_TYPER              (0x004)
#define GICD_IGROUPR(n)         (0x080 + (n) * 4)
#define GICD_ISENABLER(n)       (0x100 + (n) * 4)
#define GICD_ICENABLER(n)       (0x180 + (n) * 4)
#define GICD_ISPENDR(n)         (0x200 + (n) * 4)
#define GICD_ICPENDR(n)         (0x280 + (n) * 4)
#define GICD_ISACTIVER(n)       (0x300 + (n) * 4)
#define GICD_ICACTIVER(n)       (0x380 + (n) * 4)
#define GICD_IPRIORITYR(n)      (0x400 + (n) * 4)
#define GICD_ITARGETSR(n)       (0x820 + (n) * 4)
#define GICD_ICFGR(n)           (0xC00 + (n) * 4)
#define GICD_SGIR               (0xF00)
#define GICD_CTLR_ENABLEGRP0    (1 << 0)
#define GICD_CTLR_ENABLEGRP1    (1 << 1)
#define GIC_MAX_INTS (128)        
/* Number of interrupts in one register */
#define NUM_INTS_PER_REG        32
#define GIC_PRI_MASK            0xf0

void sunxi_gic_distributor_init(uint32_t gicd_base)
{       
        uint32_t cpumask = 0x01010101;
        uint32_t gic_irqs;
        uint32_t i;
        size_t max_regs;
        
        /* disable the forawrding of pending interrrupts from the distributor to the cpu interfaces */
        write32(0, gicd_base + GICD_CTLR);
        
        /* check GIC hardware configutation */
        gic_irqs = ((read32(gicd_base + GICD_TYPER) & 0x1f) + 1) * 32;
        if (gic_irqs > 1020)
                gic_irqs = 1020;
        
        if (gic_irqs < GIC_MAX_INTS)
        {
                printf("GIC parameter config error\n");
                return ;
        }
        
        /*  Set ALL interrupts as group1(non-secure) interrupts */
        max_regs = (GIC_MAX_INTS + NUM_INTS_PER_REG - 1) /
                                        NUM_INTS_PER_REG;
        for (i=1; i<max_regs; i++)
        {      
		write32(0xffffffff, gicd_base + GICD_IGROUPR(i));
        }      
        
        /* set trigger type to be level-triggered, active low */
        /* 2-bit int_config field for each interrupt*/
        max_regs = (GIC_MAX_INTS + 16 - 1) / 16;
        for (i=0; i<max_regs; i++)
        {      
                write32(0, gicd_base + GICD_ICFGR(i));
        }      
        
        /* Set priority for PPI,SGI,SPI interrupts */
        max_regs = (GIC_MAX_INTS + 4 - 1) / 4;
        for (i=0; i<max_regs; i++)
        {      
                write32(0xa0a0a0a0, gicd_base + GICD_IPRIORITYR(i));
        }      
         
        /* set processor target */
        /* 8-bit cpu targets field for each interrupt*/
        /* not include SGIs and PPIs */
        max_regs = (GIC_MAX_INTS - 32 + 4 - 1) / 4;
        for (i=0; i<max_regs; i++)
        {      
                write32(cpumask, gicd_base + GICD_ITARGETSR(i));
        }      
         
        /* disable all interrupts */
        max_regs = (GIC_MAX_INTS + 32 - 1) / 32;
        for (i=0; i<max_regs; i++)
        {      
                write32(0xffffffff, gicd_base + GICD_ICENABLER(i));
        }      
        /* clear all interrupt active state */
        max_regs = (GIC_MAX_INTS + 32 - 1) / 32;
        for (i=0; i<max_regs; i++)
        {      
                write32(0xffffffff, gicd_base + GICD_ICACTIVER(i));
        }
        /* enable group0 and group1 */
        write32(GICD_CTLR_ENABLEGRP0|GICD_CTLR_ENABLEGRP1, gicd_base + GICD_CTLR);
         
}        


void sunxi_gic_cpuif_init(uint32_t gicc_base, uint32_t gicd_base)
{       
        int i = 0;
        /*
        ¦* Deal with the banked PPI and SGI interrupts - disable all
        ¦* PPI interrupts, ensure all SGI interrupts are enabled.
        */                                                                                                                                                                                     
        write32(0, gicc_base + GICC_CTLR);

        write32(0xffffffff, gicd_base + GICD_IGROUPR(0));

        write32(0xffff0000, gicd_base + GICD_ICENABLER(0));
        write32(0x0000ffff, gicd_base + GICD_ISENABLER(0));
         
        /*interrupts with priority > 16 are signaled to the processor
        ¦  bit[3-0] = 0 : support 16 level
        */

	for (i= 0; i < 8; i++)
	{
		write32(0xa0a0a0a0, gicd_base + GICD_IPRIORITYR(i));
	}

        write32(0xff, gicc_base + GICC_PMR);
        
        /* Enable GIC */
        write32(GICC_CTLR_ENABLEGRP0 | GICC_CTLR_ENABLEGRP1 | GICC_CTLR_FIQEN,
                gicc_base + GICC_CTLR);
        
}

void set_cpuif(void)
{
    sunxi_gic_cpuif_init(GICC_BASE, GICD_BASE);
}

void set_dist(void)
{
    sunxi_gic_distributor_init(GICD_BASE);
}
void register_sgi(void);
void main_gic_init(void)
{
    sunxi_gic_cpuif_init(GICC_BASE, GICD_BASE);
    sunxi_gic_distributor_init(GICD_BASE);
    register_sgi();
}
