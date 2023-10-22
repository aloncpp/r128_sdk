#include <errno.h>
#include <metal/sys.h>
#include <openamp/sunxi_helper/mem_map.h>
#include <openamp/sunxi_helper/openamp_log.h>

static const struct mem_mapping mem_mappings[] = {
	/* non-cacheable */
	/* local SRAM via external bus */
	{ .va = 0x28000, .len = 0x20000, .pa = 0x28000, .attr = MEM_NONCACHEABLE },
	/* local SRAM via internal bus */
	{ .va = 0x400000, .len = 0x10000, .pa = 0x400000, .attr = MEM_NONCACHEABLE },
	{ .va = 0x420000, .len = 0x8000, .pa = 0x420000, .attr = MEM_NONCACHEABLE },
	{ .va = 0x440000, .len = 0x8000, .pa = 0x440000, .attr = MEM_NONCACHEABLE },
	/* DDR front 256MB */
	{ .va = 0x10000000, .len = 0x10000000, .pa = 0x40000000, .attr = MEM_NONCACHEABLE },
	/* DDR front 1GB */
	{ .va = 0x40000000, .len = 0x40000000, .pa = 0x40000000, .attr = MEM_NONCACHEABLE },
	/*
	 * DDR front 1GB (cacheable configurable)
	 *   In init-sun8iw20.c it's configured non-cacheable.
	 */
	{ .va = 0x80000000, .len = 0x40000000, .pa = 0x40000000, .attr = MEM_NONCACHEABLE },

	/* cacheable */
	/* local SRAM - IRAM */
	{ .va = 0x20028000, .len = 0x10000, .pa = 0x400000, .attr = MEM_CACHEABLE },
	/* local SRAM - DRAM0 */
	{ .va = 0x20038000, .len = 0x8000, .pa = 0x420000, .attr = MEM_CACHEABLE },
	/* local SRAM - DRAM1 */
	{ .va = 0x20040000, .len = 0x8000, .pa = 0x440000, .attr = MEM_CACHEABLE },
	/* DDR front 256MB */
	{ .va = 0x30000000, .len = 0x10000000, .pa = 0x40000000, .attr = MEM_CACHEABLE },
	/* DDR front 1GB */
	{ .va = 0xC0000000, .len = 0x40000000, .pa = 0x40000000, .attr = MEM_CACHEABLE },
};

REGISTER_MEM_MAPPINGS(mem_mappings);
