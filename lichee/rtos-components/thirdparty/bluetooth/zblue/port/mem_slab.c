/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
//#include <kernel_structs.h>
//#include <debug/object_tracing_common.h>
#include <toolchain.h>
//#include <linker/sections.h>
//#include <wait_q.h>
#include <misc/dlist.h>
//#include <ksched.h>
//#include <init.h>
//#include <sys/check.h>

#ifdef CONFIG_OBJECT_TRACING
struct k_mem_slab *_trace_list_k_mem_slab;
#endif	/* CONFIG_OBJECT_TRACING */

/**
 * @brief Initialize kernel memory slab subsystem.
 *
 * Perform any initialization of memory slabs that wasn't done at build time.
 * Currently this just involves creating the list of free blocks for each slab.
 *
 * @return N/A
 */
static int create_free_list(struct k_mem_slab *slab)
{
	uint32_t j;
	char *p;

	/* blocks must be word aligned */
	CHECKIF(((slab->block_size | (uintptr_t)slab->buffer) &
				(sizeof(void *) - 1)) != 0) {
		return -EINVAL;
	}

	slab->free_list = NULL;
	p = slab->buffer;

	for (j = 0U; j < slab->num_blocks; j++) {
		*(char **)p = slab->free_list;
		slab->free_list = p;
		p += slab->block_size;
	}
	return 0;
}

#if defined(CONFIG_BLEHOST_Z_ITERABLE_SECTION)
/**
 * @brief Complete initialization of statically defined memory slabs.
 *
 * Perform any initialization that wasn't done at build time.
 *
 * @return N/A
 */
int init_mem_slab_module(void)
{
	int rc = 0;

	Z_STRUCT_SECTION_FOREACH(k_mem_slab, slab) {
#if defined(CONFIG_BT_VAR_MEM_DYNC_ALLOC)
		rc = mem_slab_mem_alloc(slab);
		if (rc < 0) {
			goto out;
		}
#endif
		rc = create_free_list(slab);
		if (rc < 0) {
			goto out;
		}
		z_waitq_init_n(&slab->wait_q, slab->num_blocks);
		SYS_TRACING_OBJ_INIT(k_mem_slab, slab);
		z_object_init(slab);
	}

out:
	return rc;
}

#if defined(CONFIG_BT_DEINIT)
int deinit_mem_slab_module(void)
{
	int rc = 0;

	Z_STRUCT_SECTION_FOREACH(k_mem_slab, slab) {
		rc = z_waitq_deinit_n(&slab->wait_q);
		if (rc < 0) {
			return rc;
		}
		slab->num_used = 0;
		slab->free_list = NULL;
#if defined(CONFIG_BT_VAR_MEM_DYNC_ALLOC)
		mem_slab_mem_free(slab);
#else
		slab->buffer = NULL;
#endif
	}

	return rc;
}

void mem_slab_var_deinit(struct k_mem_slab *mem_slab, size_t block_size,
            uint32_t num_blocks)
{
	mem_slab->num_blocks = num_blocks;
	mem_slab->block_size = block_size;
	mem_slab->num_used = 0;
	SYS_TRACING_OBJ_INIT(k_mem_slab, mem_slab);
	z_object_init(mem_slab);
}
#endif
#endif /* CONFIG_BLEHOST_Z_ITERABLE_SECTION */

#if defined(CONFIG_BT_VAR_MEM_DYNC_ALLOC)
int mem_slab_mem_alloc(struct k_mem_slab *mem_slab)
{
	size_t alloc_size;

	alloc_size = mem_slab->num_blocks * mem_slab->block_size;

	mem_slab->buffer = (char *)k_malloc(alloc_size);
	if (mem_slab->buffer == NULL) {
		return -ENOMEM;
	}
	memset(mem_slab->buffer, 0, alloc_size);
	return 0;
}

void mem_slab_mem_free(struct k_mem_slab *mem_slab)
{
	k_free(mem_slab->buffer);
	mem_slab->buffer = NULL;
}
#endif

int k_mem_slab_init(struct k_mem_slab *slab, void *buffer,
		    size_t block_size, uint32_t num_blocks)
{
	int rc = 0;

	slab->num_blocks = num_blocks;
	slab->block_size = block_size;
	slab->buffer = buffer;
	slab->num_used = 0U;
	rc = create_free_list(slab);
	if (rc < 0) {
		goto out;
	}
	z_waitq_init_n(&slab->wait_q, slab->num_blocks);
	SYS_TRACING_OBJ_INIT(k_mem_slab, slab);

	z_object_init(slab);

out:
	return rc;
}

#if defined(CONFIG_BT_DEINIT)
int k_mem_slab_deinit(struct k_mem_slab *slab)
{
	int rc = 0;

	rc = z_waitq_deinit_n(&slab->wait_q);
	if (rc < 0) {
		return rc;
	}
	slab->num_blocks = 0;
	slab->block_size = 0;
#if !defined(CONFIG_BT_VAR_MEM_DYNC_ALLOC)
	slab->buffer = NULL;
#endif
	slab->num_used = 0;
	slab->free_list = NULL;

	return 0;
}
#endif

#if !defined(K_MEM_SLAB_BY_HEAP)
int k_mem_slab_alloc(struct k_mem_slab *slab, void **mem, k_timeout_t timeout)
{
	unsigned int key = irq_lock();
	int result;

	/* wait for a free block or timeout */
	z_pend_curr(NULL, key, &slab->wait_q, timeout);

	key = irq_lock();

	if (slab->free_list != NULL) {
		/* take a free block */
		*mem = slab->free_list;
		slab->free_list = *(char **)(slab->free_list);
		slab->num_used++;
		result = 0;
	} else {
		/* don't wait for a free block to become available */
		*mem = NULL;
		result = -ENOMEM;
	}

	irq_unlock(key);

	return result;
}

void k_mem_slab_free(struct k_mem_slab *slab, void **mem)
{
	unsigned int key = irq_lock();

	**(char ***)mem = slab->free_list;
	slab->free_list = *(char **)mem;
	slab->num_used--;

	struct k_thread *pending_thread = z_unpend_first_thread(&slab->wait_q);
	if (pending_thread != NULL) {
		z_ready_thread(pending_thread);
		z_reschedule(key);
	} else {
		irq_unlock(key);
	}
}
#endif /* K_MEM_SLAB_BY_HEAP */
