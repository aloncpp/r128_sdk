#include <errno.h>
#include <metal/sys.h>
#include <openamp/sunxi_helper/mem_map.h>
#include <openamp/sunxi_helper/openamp_log.h>

const struct mem_mapping mem_mappings[] = {
	/* DSP RAM */
	{ .va = 0x20000, .len = 0x20000, .pa = 0x20000, .attr = MEM_NONCACHEABLE },
	/* SRAM A2 */
	{ .va = 0x40000, .len = 0x24000, .pa = 0x40000, .attr = MEM_NONCACHEABLE },
	/* DDR */
	{ .va = 0x8000000, .len = 0x37f00000, .pa = 0x8000000, .attr = MEM_NONCACHEABLE },
	/* SRAM SPACE 1 */
	{ .va = 0x3ffc0000, .len = 0x40000, .pa = 0x07280000, .attr = MEM_CACHEABLE },
	/* SRAM SPACE 2 && DRAM SPACE 1 */
	{ .va = 0x40000000, .len = 0x40000, .pa = 0x072c0000, .attr = MEM_CACHEABLE },
	/* DRAM SPACE 2 */
	{ .va = 0x40040000, .len = 0x3ffc0000, .pa = 0x40040000, .attr = MEM_CACHEABLE },
};
REGISTER_MEM_MAPPINGS(mem_mappings);
