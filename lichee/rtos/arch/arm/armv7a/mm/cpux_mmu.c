/*
 * =====================================================================================
 *
 *       Filename:  cpux_mmu.c
 *
 *    Description:  for mmu op. 
 *
 *        Version:  1.0
 *        Created:  2019年07月11日 13时50分18秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  czl 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stddef.h>
#include <stdint.h>
#include "drivers/hw_memmap.h"
#include "FreeRTOSConfig.h"
#include "mmu_cache.h" 
#include "cp15.h"

#define __REG32(x)               (*((volatile unsigned int *)(x)))
#define GIC_DIST_SOFTINT(hw_base) __REG32((hw_base) + 0xf00)

#define TTB_S           (1 << 1)
#define TTB_RGN_NC      (0 << 3)
#define TTB_RGN_OC_WBWA (1 << 3)
#define TTB_RGN_OC_WT   (2 << 3)
#define TTB_RGN_OC_WB   (3 << 3)
#define TTB_NOS         (1 << 5)
#define TTB_IRGN_NC     ((0 << 0) | (0 << 6))
#define TTB_IRGN_WBWA   ((0 << 0) | (1 << 6))
#define TTB_IRGN_WT     ((1 << 0) | (0 << 6))
#define TTB_IRGN_WB     ((1 << 0) | (1 << 6))
#define TTB_FLAGS_SMP   TTB_IRGN_WBWA|TTB_S|TTB_NOS|TTB_RGN_OC_WBWA

void xport_cpu_send_ipi(uint32_t ipi_no)
{
    dsb();
    // send cpu1 with IPI_NO SOFT INT.
    GIC_DIST_SOFTINT(GIC_DIST_BASE) = ((1 << 1) << 16) | ipi_no;
}

unsigned long xport_cpu_set_domain_register(unsigned long domain_val)
{
    unsigned long old_domain;
 
    asm volatile ("mrc p15, 0, %0, c3, c0\n" : "=r" (old_domain));
    asm volatile ("mcr p15, 0, %0, c3, c0\n" : :"r" (domain_val) : "memory");
 
    return old_domain;
}

//MUST Execute the i/d cache flush operation.
//Or the CPU1 would get another view of meory data.
//reasons: freertos kernel so small so that it was
//not to be re-write to the physical memory. after 
//the cpu1 triggered, would get rubbish data.

//TODO: the mmu and cache bring up shoud be re-dong 
//by CPU0. this need rewrite the uboot and freertos
//boot stage code.
//this can be removed if the after the freertos boot
//completes.
void cache_flush_invalide_for_firmware(void)
{
    xport_cpu_dcache_clean_flush();
    /*xport_cpu_dcache_disable();*/
    /*xport_cpu_icache_disable();*/
    /*xport_cpu_dcache_enable();*/
    /*xport_cpu_icache_enable();*/
}

//Create a Basic L1 page table in RAM, with 1M sections map.
//flat maping(VA=PA), need inversigate for for IOs, maybe missed.
struct mem_zone_map_items r328_page_dirs[] = 
{
#ifdef CONFIG_ARCH_SUN8IW18P1
#ifdef CONFIG_CPU_PRIVATE_DATA
    {0x48000000, 0x480FFFFF, 0x48000000, NORMAL_MEM},
#endif
	{0x40000000, 0x43FFFFFF, 0x40000000, NORMAL_MEM},
    {0x09000000, 0x090FFFFF, 0x09000000, DEVICE_MEM},
    {0x08100000, 0x081FFFFF, 0x08100000, DEVICE_MEM},
    {0x07000000, 0x070FFFFF, 0x07000000, DEVICE_MEM},
    {0x06700000, 0x067003FF, 0x06700000, DEVICE_MEM},
    {0x05480000, 0x0549FFFF, 0x05480000, DEVICE_MEM},
    {0x05400000, 0x05400FFF, 0x05400000, DEVICE_MEM},
    {0x05000000, 0x051FFFFF, 0x05000000, DEVICE_MEM},
    {0x04000000, 0x040FFFFF, 0x04000000, DEVICE_MEM},
    {0x03000000, 0x030FFFFF, 0x03000000, DEVICE_MEM},
    {0x01900000, 0x01CFFFFF, 0x01900000, DEVICE_MEM},
#elif CONFIG_ARCH_SUN8IW20
#ifdef CONFIG_CPU_PRIVATE_DATA
    {0x48000000, 0x480FFFFF, 0x48000000, NORMAL_MEM},
#endif
    {0x40000000, 0x43FFFFFF, 0x40000000, NORMAL_MEM},
    {0x09010000, 0x09020FFF, 0x09010000, DEVICE_MEM},
    {0x08100000, 0x08130FFF, 0x08100000, DEVICE_MEM},
    {0x07090000, 0x070903FF, 0x07090000, DEVICE_MEM},
    {0x07000400, 0x070403FF, 0x07000400, DEVICE_MEM},
    {0x06000000, 0x06011FFF, 0x06000000, DEVICE_MEM},
    {0x05800000, 0x05C01FFF, 0x05800000, DEVICE_MEM},
    {0x05000000, 0x05607FFF, 0x05000000, DEVICE_MEM},
    {0x04020000, 0x0450FFFF, 0x04020000, DEVICE_MEM},
    {0x03000000, 0x03301FFF, 0x03000000, DEVICE_MEM},
    {0x02500000, 0x025047FF, 0x02500000, DEVICE_MEM},
    {0x02050000, 0x02050FFF, 0x02050000, DEVICE_MEM},
    {0x02036000, 0x020363FF, 0x02036000, DEVICE_MEM},
    {0x02034000, 0x020343FF, 0x02034000, DEVICE_MEM},
    {0x02033000, 0x020333FF, 0x02033000, DEVICE_MEM},
    {0x02032000, 0x020323FF, 0x02032000, DEVICE_MEM},
    {0x02031000, 0x020313FF, 0x02031000, DEVICE_MEM},
    {0x02030000, 0x02030FFF, 0x02030000, DEVICE_MEM},
    {0x02009400, 0x0201FFFF, 0x02009400, DEVICE_MEM},
    {0x02009000, 0x020093FF, 0x02009000, DEVICE_MEM},
    {0x02008000, 0x020083FF, 0x02008000, DEVICE_MEM},
    {0x02004000, 0x020043FF, 0x02004000, DEVICE_MEM},
    {0x02003000, 0x020033FF, 0x02003000, DEVICE_MEM},
    {0x02000000, 0x02001FFF, 0x02000000, DEVICE_MEM},
    {0x01C0E000, 0x01C0FFFF, 0x01C0E000, DEVICE_MEM},
#else
#error "can not support the patform!!"
#endif
    /* boot rom address, not need to map  */
    /* {0x00000000, 0x00100000, 0x00000000, SRAM_MEM} */
}; 

volatile unsigned long cpux_mmu_table[4*1024*configNR_CPUS] __attribute__((aligned(16*1024)));

#define GET_CPUX_MMU_TABLE(cpuid)	(&cpux_mmu_table[4*1024*(cpuid)])

void xport_cpu_mmu_setpgd(uint32_t vaddrStart,
				uint32_t vaddrEnd,
				uint32_t paddrStart,
				uint32_t attr)
{
    volatile uint32_t *pdirs;
    volatile int i, nsec;
	int processor_id = cur_cpu_id();
    pdirs  = (uint32_t *)GET_CPUX_MMU_TABLE(processor_id) + (vaddrStart >> 20);
    nsec = (vaddrEnd >> 20) - (vaddrStart >> 20);
    for(i = 0; i <= nsec; i++)
    {
       *pdirs = attr | (((paddrStart >> 20) + i) << 20);
       pdirs++;
    }
    isb(); dsb(); dmb();
}

void xport_cpu_page_table_init(void)
{
    int i = 0;
#ifdef CONFIG_CPU_PRIVATE_DATA
	{
		/* pre map private data addr of each cpu */
		int processor_id = cur_cpu_id();
		extern const unsigned long cpu_private_data_paddr;
		extern const unsigned long cpu_private_data_vaddr;
		extern const unsigned long cpu_private_data_size;

		uint32_t paddr = ( (cpu_private_data_paddr - processor_id * (1024*1024)) & (~(uint32_t)(1024*1024-1)) );
		uint32_t vaddr = ( cpu_private_data_vaddr & (~(uint32_t)(1024*1024-1))  );
		uint32_t vaddr_end = (1024*1024-1) + vaddr;

		r328_page_dirs[0].vaddr_start = vaddr;
		r328_page_dirs[i].vaddr_end = vaddr_end;
		r328_page_dirs[i].paddr_start = paddr;

		//printk("cpu%d private data: vaddr:%08lx, vaddr_end:%08lx, paddr:%08lx\r\n", processor_id, vaddr, vaddr_end, paddr);
	}
#endif
    for(i = 0; i < sizeof(r328_page_dirs)/sizeof(r328_page_dirs[0]); i ++)
    {
        xport_cpu_mmu_setpgd(r328_page_dirs[i].vaddr_start, r328_page_dirs[i].vaddr_end, r328_page_dirs[i].paddr_start, r328_page_dirs[i].attr);
    }

    isb(); dsb(); dmb();
}

void xport_cpu1_cache_op_flush(void)
{
    /*xport_cpu_dcache_clean_flush();*/
    /*xport_cpu_icache_flush();*/
    xport_cpu_dcache_disable();
    xport_cpu_icache_disable();
}

void xport_cpu1_mmu_op_enable(void)
{
	int processor_id = cur_cpu_id();
    xport_cpu_mmu_disable();
    xport_cpu_page_table_init();
    xport_cpu_set_domain_register(0x55555555);

    // only ttbr0 used ,set ttbr1 to null.
    xport_cpu_tlb_set(((unsigned int)GET_CPUX_MMU_TABLE(processor_id))|TTB_FLAGS_SMP);
    xport_cpu_mmu_enable();
#ifdef CONFIG_CPU_PRIVATE_DATA
	if( 0 == processor_id ){
		extern const unsigned long cpu_private_data_paddr;
		extern const unsigned long cpu_private_data_vaddr;
		extern const unsigned long cpu_private_data_size;
		extern void vInitCpuPrivateReent( void *paddr, void *vaddr  );
		extern void vInitIrqReent( void );
		vInitIrqReent();
		for(int i = 0;i<configNR_CPUS;i++){
			vInitCpuPrivateReent( (void *)(cpu_private_data_paddr - (unsigned long)(i * (1024*1024))), (void *)cpu_private_data_vaddr);
		}
		unsigned long i=0x2000;
		while(i--);
	}
#endif
}

void xport_cpu1_cache_op_enable(void)
{
    xport_cpu_dcache_enable();
    xport_cpu_icache_enable();
}

int xport_is_valid_address(void* start_addr, void* end_addr)
{
    int flag = 0;
    int i;
    if(start_addr > end_addr && end_addr != NULL)
    {
        return 0;
    }

    for(i = 0; i < sizeof(r328_page_dirs)/sizeof(r328_page_dirs[0]); i++)
    {
        if((uint32_t)start_addr >= r328_page_dirs[i].vaddr_start && (uint32_t)start_addr <= r328_page_dirs[i].vaddr_end)
        {
            if(end_addr == NULL)
            {
                return 1;
            }
            else if((uint32_t)end_addr >= r328_page_dirs[i].vaddr_start && (uint32_t)end_addr <= r328_page_dirs[i].vaddr_end)
            {
                return 1;
            }
        }
    }
    return 0;
}
