#include <errno.h>
#include <metal/sys.h>
#include <console.h>
#include <openamp/sunxi_helper/mem_map.h>
#include <openamp/sunxi_helper/openamp_log.h>
#include <openamp/sunxi_helper/openamp.h>

extern const struct mem_mapping *_mem_mappings;
extern const int _mem_mappings_size;

int mem_va_to_pa(unsigned long va, metal_phys_addr_t *pa, uint32_t *attr)
{
	const struct mem_mapping *map;
	int i;

	for (i = 0; i < _mem_mappings_size; ++i) {
		map = &_mem_mappings[i];
		if (va >= map->va && va < map->va + map->len) {
			*pa = va - map->va + map->pa;
			if (attr) {
				*attr = map->attr;
			}
			return 0;
		}
	}
	openamp_dbg("Invalid va %lx\n", va);
	return -EINVAL;
}

int mem_pa_to_va(unsigned long pa, metal_phys_addr_t *va, uint32_t *attr)
{
	const struct mem_mapping *map;
	int i;

	/*
	 * TODO:
	 * Maybe there are multiple VAs corresponding to one PA.
	 * Only return the first matching one?
	 */
	for (i = 0; i < _mem_mappings_size; ++i) {
		map = &_mem_mappings[i];
		if (pa >= map->pa && pa < map->pa + map->len) {
			*va = (metal_phys_addr_t)((uintptr_t)(pa - map->pa + map->va));
			if (attr) {
				*attr = map->attr;
			}
			return 0;
		}
	}
	openamp_dbg("Invalid pa 0x%lx\n", pa);
	return -EINVAL;
}

static int dump_mem_mapping(int argc, char *argv[])
{
	const struct mem_mapping *map;
	int i;

	printf("rproc memory mapping:\n");
	printf("va\t\t\t\tpa\t\t\t\tlen\n");
	for (i = 0; i < _mem_mappings_size; ++i) {
		map = &_mem_mappings[i];
		printf("0x%08zx\t\t0x%08zx\t\t0x%08x\t%s\n", (size_t)map->va,
						(size_t)map->pa, map->len,
						map->attr == MEM_CACHEABLE ? "Cacheable" : "Non-Cacheable");
	}
	printf("\n");
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(dump_mem_mapping, rproc_dump_mapping, rproc dump mapping);
