#include <errno.h>
#include <metal/sys.h>
#include <openamp/sunxi_helper/mem_map.h>
#include <openamp/sunxi_helper/openamp_log.h>

const struct mem_mapping mem_mappings[] = {
	/* 512M */
	{ .va = 0x40000000, .len = 0x20000000, .pa = 0x40000000, .attr = MEM_CACHEABLE },
};
REGISTER_MEM_MAPPINGS(mem_mappings);
