#include "slab_rtt_system.h"

/**
 * This function allocates a memory block, which address is aligned to the
 * specified alignment size.
 *
 * @param size the allocated memory block size
 * @param align the alignment size
 *
 * @return the allocated memory block on successful, otherwise returns RT_NULL
 */
void *rt_malloc_align(rt_size_t size, rt_size_t align)
{
    void *ptr;
    void *align_ptr;
    int uintptr_size;
    rt_size_t align_size;

    /* sizeof pointer */
    uintptr_size = sizeof(void*);
    uintptr_size -= 1;

    /* align the alignment size to uintptr size byte */
    align = ((align + uintptr_size) & ~uintptr_size);

    /* get total aligned size */
    align_size = ((size + uintptr_size) & ~uintptr_size) + align;
    /* allocate memory block from heap */
    ptr = rt_malloc(align_size);
    if (ptr != RT_NULL)
    {
        /* the allocated memory block is aligned */
        if (((rt_ubase_t)ptr & (align - 1)) == 0)
        {
            align_ptr = (void *)((rt_ubase_t)ptr + align);
        }
        else
        {
            align_ptr = (void *)(((rt_ubase_t)ptr + (align - 1)) & ~(align - 1));
        }

        /* set the pointer before alignment pointer to the real pointer */
        *((rt_ubase_t *)((rt_ubase_t)align_ptr - sizeof(void *))) = (rt_ubase_t)ptr;

        ptr = align_ptr;
    }

    return ptr;
}

/**
 * This function release the memory block, which is allocated by
 * rt_malloc_align function and address is aligned.
 *
 * @param ptr the memory block pointer
 */
void rt_free_align(void *ptr)
{
    void *real_ptr;

    real_ptr = (void *) * (rt_ubase_t *)((rt_ubase_t)ptr - sizeof(void *));
    rt_free(real_ptr);
}
