#include <stdio.h>

struct vaddr_range {
	unsigned long vstart;
	unsigned long vend;
	unsigned long offset;
};

/*
 * It's a fake mmu, just a DSP's perspective translation.
 * Only DDR addresses matter.
 */
static struct vaddr_range addr_mapping[] = {
	{ 0x10000000, 0x1fffffff, 0x30000000 },
	{ 0x30000000, 0x3fffffff, 0x10000000 },
};

unsigned long __va_to_pa(unsigned long vaddr)
{
	unsigned long paddr = vaddr;
	int i;
	int size = sizeof(addr_mapping) / sizeof(struct vaddr_range);

	for (i = 0; i < size; i++) {
		if (vaddr >= addr_mapping[i].vstart
				&& vaddr <= addr_mapping[i].vend) {
			paddr += addr_mapping[i].offset;
			break;
		}
	}

	return paddr;
}

unsigned long __pa_to_va(unsigned long paddr)
{
	/* TODO: not implement yet */
	return paddr;
}
