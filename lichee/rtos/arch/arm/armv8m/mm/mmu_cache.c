#include "mmu_cache.h"
#include <hal_dma.h>
#include <barrier.h>

#include <irqs.h>
#include <platform.h>
#include <memory.h>
#include <core_cm33.h>

#include <portmacro.h>
#include <cmsis_compiler.h>
#include <cachel1_armv7.h>

#define portMPU_TYPE_REG                      ( *( ( volatile uint32_t * ) 0xe000ed90 ) )
#define portMPU_CTRL_REG                      ( *( ( volatile uint32_t * ) 0xe000ed94 ) )
#define portMPU_RNR_REG                       ( *( ( volatile uint32_t * ) 0xe000ed98 ) )

#define portMPU_RBAR_REG                      ( *( ( volatile uint32_t * ) 0xe000ed9c ) )
#define portMPU_RLAR_REG                      ( *( ( volatile uint32_t * ) 0xe000eda0 ) )

#define portMPU_RBAR_A1_REG                   ( *( ( volatile uint32_t * ) 0xe000eda4 ) )
#define portMPU_RLAR_A1_REG                   ( *( ( volatile uint32_t * ) 0xe000eda8 ) )

#define portMPU_RBAR_A2_REG                   ( *( ( volatile uint32_t * ) 0xe000edac ) )
#define portMPU_RLAR_A2_REG                   ( *( ( volatile uint32_t * ) 0xe000edb0 ) )

#define portMPU_RBAR_A3_REG                   ( *( ( volatile uint32_t * ) 0xe000edb4 ) )
#define portMPU_RLAR_A3_REG                   ( *( ( volatile uint32_t * ) 0xe000edb8 ) )

#define portMPU_MAIR0_REG                     ( *( ( volatile uint32_t * ) 0xe000edc0 ) )
#define portMPU_MAIR1_REG                     ( *( ( volatile uint32_t * ) 0xe000edc4 ) )

#define portMPU_RBAR_ADDRESS_MASK             ( 0xffffffe0 ) /* Must be 32-byte aligned. */
#define portMPU_RLAR_ADDRESS_MASK             ( 0xffffffe0 ) /* Must be 32-byte aligned. */

#define portMPU_MAIR_ATTR0_POS                ( 0UL )
#define portMPU_MAIR_ATTR0_MASK               ( 0x000000ff )

#define portMPU_MAIR_ATTR1_POS                ( 8UL )
#define portMPU_MAIR_ATTR1_MASK               ( 0x0000ff00 )

#define portMPU_MAIR_ATTR2_POS                ( 16UL )
#define portMPU_MAIR_ATTR2_MASK               ( 0x00ff0000 )

#define portMPU_MAIR_ATTR3_POS                ( 24UL )
#define portMPU_MAIR_ATTR3_MASK               ( 0xff000000 )

#define portMPU_MAIR_ATTR4_POS                ( 0UL )
#define portMPU_MAIR_ATTR4_MASK               ( 0x000000ff )

#define portMPU_MAIR_ATTR5_POS                ( 8UL )
#define portMPU_MAIR_ATTR5_MASK               ( 0x0000ff00 )

#define portMPU_MAIR_ATTR6_POS                ( 16UL )
#define portMPU_MAIR_ATTR6_MASK               ( 0x00ff0000 )

#define portMPU_MAIR_ATTR7_POS                ( 24UL )
#define portMPU_MAIR_ATTR7_MASK               ( 0xff000000 )

#define portMPU_RLAR_ATTR_INDEX0              ( 0UL << 1UL )
#define portMPU_RLAR_ATTR_INDEX1              ( 1UL << 1UL )
#define portMPU_RLAR_ATTR_INDEX2              ( 2UL << 1UL )
#define portMPU_RLAR_ATTR_INDEX3              ( 3UL << 1UL )
#define portMPU_RLAR_ATTR_INDEX4              ( 4UL << 1UL )
#define portMPU_RLAR_ATTR_INDEX5              ( 5UL << 1UL )
#define portMPU_RLAR_ATTR_INDEX6              ( 6UL << 1UL )
#define portMPU_RLAR_ATTR_INDEX7              ( 7UL << 1UL )

#define portMPU_RLAR_REGION_ENABLE            ( 1UL )

/* Enable privileged access to unmapped region. */
#define portMPU_PRIV_BACKGROUND_ENABLE_BIT    ( 1UL << 2UL )

/* Enable MPU. */
#define portMPU_ENABLE_BIT                    ( 1UL << 0UL )

/* Expected value of the portMPU_TYPE register. */
#define portEXPECTED_MPU_TYPE_VALUE           ( 8UL << 8UL ) /* 8 regions, unified. */

#define portSCB_SYS_HANDLER_CTRL_STATE_REG    ( *( volatile uint32_t * ) 0xe000ed24 )
#define portSCB_MEM_FAULT_ENABLE_BIT          ( 1UL << 16UL )

void FlushTLB(void)
{
}

void FlushDcacheAll(void)
{
    SCB_CleanDCache();
}

void InvalidIcacheAll(void)
{
    SCB_InvalidateICache();
}

void FlushIcacheAll(void)
{
    InvalidIcacheAll();
}

void FlushCacheAll(void)
{
    InvalidIcacheAll();
    FlushDcacheAll();
}

void InvalidIcacheRegion(unsigned long start, unsigned int size)
{
    SCB_InvalidateICache_by_Addr((uint32_t *)start, size);
}

void FlushDcacheRegion(unsigned long start, unsigned int size)
{
    SCB_CleanDCache_by_Addr((uint32_t *)start, size);
}

void InvalidDcacheRegion(unsigned long start, unsigned int size)
{
    SCB_InvalidateDCache_by_Addr((uint32_t *)start, size);
}

void InvalidDcache(void)
{
    SCB_InvalidateDCache();
}

void hal_dcache_init(void)
{
    SCB_EnableDCache();
}

void hal_icache_init(void)
{
    SCB_EnableICache();
}

void InitNeonVFP(void)
{
}

void *dma_map_area(void * addr, size_t size, enum dma_transfer_direction dir)
{
    if (dir == DMA_DEV_TO_MEM || dir == DMA_MEM_TO_MEM) {
        InvalidDcacheRegion((unsigned long )addr, size);
    } else {
        FlushDcacheRegion((unsigned long )addr, size);
    }
    return addr;
}

void dma_unmap_area(void * addr, size_t size, enum dma_transfer_direction dir)
{
    if (dir == DMA_DEV_TO_MEM || dir == DMA_MEM_TO_MEM) {
        InvalidDcacheRegion((unsigned long )addr, size);
    }
}

#ifndef CONFIG_ARM_MPU_NREGIONS
#  define CONFIG_ARM_MPU_NREGIONS 8
#endif

struct mpu_configure_table {
	unsigned long start_addr;
	unsigned long limit_addr;
	unsigned long mem_type;
	unsigned long mem_attr;
};

/* portMPU_RLAR_ATTR_INDEX0 --- NORMAL_MEMORY_BUFFERABLE_CACHEABLE */
/* portMPU_RLAR_ATTR_INDEX1 --- NORMAL_MEMORY_NON_CACHEABLE */
/* portMPU_RLAR_ATTR_INDEX2 --- DEVICE_MEMORY_nGnRnE */

struct mpu_configure_table mpu_table [] = {
	{
		/* sram */
		.start_addr = 0x04000000,
		.limit_addr = 0x040FFFFF,
		.mem_type = portMPU_RLAR_ATTR_INDEX1,
		.mem_attr = portMPU_REGION_NON_SHAREABLE | portMPU_REGION_READ_WRITE,
	},
	{
		/* lpsram */
		.start_addr = 0x08000000,
		.limit_addr = 0x087FFFFF,
		.mem_type = portMPU_RLAR_ATTR_INDEX0,
		.mem_attr = portMPU_REGION_NON_SHAREABLE | portMPU_REGION_READ_WRITE,
	},
	{
		/* hpsram */
		.start_addr = 0x0C000000,
		.limit_addr = 0x0DFFFFFF,
		.mem_type = portMPU_RLAR_ATTR_INDEX0,
		.mem_attr = portMPU_REGION_NON_SHAREABLE | portMPU_REGION_READ_WRITE,
	},
	{
		.start_addr = 0x10000000,
		.limit_addr = 0x1FFFFFFF,
		.mem_type = portMPU_RLAR_ATTR_INDEX0,
		.mem_attr = portMPU_REGION_NON_SHAREABLE | portMPU_REGION_READ_ONLY,
	},
	{
		.start_addr = 0xA0010000,
		.limit_addr = 0xA0017FFF,
		.mem_type = portMPU_RLAR_ATTR_INDEX1,
		.mem_attr = portMPU_REGION_NON_SHAREABLE | portMPU_REGION_READ_WRITE,
	},
	{
		.start_addr = 0xA0118000,
		.limit_addr = 0xA011BFFF,
		.mem_type = portMPU_RLAR_ATTR_INDEX1,
		.mem_attr = portMPU_REGION_NON_SHAREABLE | portMPU_REGION_READ_WRITE,
	},
	{
		.start_addr = 0x68000000,
		.limit_addr = 0x68FFFFFF,
		.mem_type = portMPU_RLAR_ATTR_INDEX1,
		.mem_attr = portMPU_REGION_NON_SHAREABLE | portMPU_REGION_READ_WRITE,
	},
#if 0
	{
		.start_addr = 0x40000000,
		.limit_addr = 0xF0000000,
		.mem_type = portMPU_RLAR_ATTR_INDEX2,
		.mem_attr = portMPU_REGION_NON_SHAREABLE | portMPU_REGION_READ_WRITE |
			portMPU_REGION_EXECUTE_NEVER,
	},
#endif
};

int mpu_configure_table_region(struct mpu_configure_table *table, int region)
{
    portMPU_RNR_REG = region;
    dsb();
    isb();
    portMPU_RBAR_REG = ( ( ( uint32_t ) table->start_addr ) & portMPU_RBAR_ADDRESS_MASK ) |
                               ( table->mem_attr );
    portMPU_RLAR_REG = ( ( ( uint32_t ) table->limit_addr ) & portMPU_RLAR_ADDRESS_MASK ) |
                               ( table->mem_type | portMPU_RLAR_REGION_ENABLE);
    dsb();
    isb();
    return 0;
}

void init_mpu(void)
{
    int i;

    if (sizeof(mpu_table)/sizeof(mpu_table[0]) > CONFIG_ARM_MPU_NREGIONS)
    {
        printf("fatal error mpu table overflow!\n");
        return;
    }

    /* MAIR0 - Index 0. */
    portMPU_MAIR0_REG |= ( ( portMPU_NORMAL_MEMORY_BUFFERABLE_CACHEABLE << portMPU_MAIR_ATTR0_POS ) & portMPU_MAIR_ATTR0_MASK );
    /* MAIR0 - Index 1. */
    portMPU_MAIR0_REG |= ( (portMPU_NORMAL_MEMORY_NON_CACHEABLE << portMPU_MAIR_ATTR1_POS ) & portMPU_MAIR_ATTR1_MASK );
    /* MAIR0 - Index 2. */
    portMPU_MAIR0_REG |= ( ( portMPU_DEVICE_MEMORY_nGnRnE << portMPU_MAIR_ATTR2_POS ) & portMPU_MAIR_ATTR2_MASK );

    for (i = 0; i < sizeof(mpu_table) / sizeof(mpu_table[0]); i++)
    {
        mpu_configure_table_region(&mpu_table[i], i);
    }

    portMPU_CTRL_REG |= ( portMPU_PRIV_BACKGROUND_ENABLE_BIT | portMPU_ENABLE_BIT);
    dsb();
    isb();
}
