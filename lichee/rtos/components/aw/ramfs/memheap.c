/**
 * The code is extract from rt-thread, which under the Apache License 2.0
 *
 */
#include "memheap.h"

#define MEMHEAP_MAGIC        0x1ea01ea0
#define MEMHEAP_MASK         0xfffffffe
#define MEMHEAP_USED         0x01
#define MEMHEAP_FREED        0x00

#define MEMHEAP_IS_USED(i)   ((i)->magic & MEMHEAP_USED)
#define MEMHEAP_MINIALLOC    12

#define MEMHEAP_ALIGN(size, align)           (((size) + (align) - 1) & ~((align) - 1))

#define MEMHEAP_SIZE         MEMHEAP_ALIGN(sizeof(struct memheap_item), MEMHEAP_ALIGN_SIZE)
#define MEMITEM_SIZE(item)      ((unsigned long)item->next - (unsigned long)item - MEMHEAP_SIZE)

#define MEMHEAP_ASSERT(x) configASSERT(x)
#define MEMHEAP_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))
#define MEMHEAP_ALIGN_SIZE (4)

int memheap_init(struct memheap *memheap,
                 const char        *name,
                 void              *start_addr,
                 size_t         size)
{
    struct memheap_item *item;

    MEMHEAP_ASSERT(memheap != NULL);

    memheap->start_addr     = start_addr;
    memheap->pool_size      = MEMHEAP_ALIGN_DOWN(size, MEMHEAP_ALIGN_SIZE);
    memheap->available_size = memheap->pool_size - (2 * MEMHEAP_SIZE);
    memheap->max_used_size  = memheap->pool_size - memheap->available_size;

    /* initialize the free list header */
    item            = &(memheap->free_header);
    item->magic     = MEMHEAP_MAGIC;
    item->pool_ptr  = memheap;
    item->next      = NULL;
    item->prev      = NULL;
    item->next_free = item;
    item->prev_free = item;

    /* set the free list to free list header */
    memheap->free_list = item;

    /* initialize the first big memory block */
    item            = (struct memheap_item *)start_addr;
    item->magic     = MEMHEAP_MAGIC;
    item->pool_ptr  = memheap;
    item->next      = NULL;
    item->prev      = NULL;
    item->next_free = item;
    item->prev_free = item;

    item->next = (struct memheap_item *)
                 ((uint8_t *)item + memheap->available_size + MEMHEAP_SIZE);
    item->prev = item->next;

    /* block list header */
    memheap->block_list = item;

    /* place the big memory block to free list */
    item->next_free = memheap->free_list->next_free;
    item->prev_free = memheap->free_list;
    memheap->free_list->next_free->prev_free = item;
    memheap->free_list->next_free            = item;

    /* move to the end of memory pool to build a small tailer block,
     * which prevents block merging
     */
    item = item->next;
    /* it's a used memory block */
    item->magic     = MEMHEAP_MAGIC | MEMHEAP_USED;
    item->pool_ptr  = memheap;
    item->next      = (struct memheap_item *)start_addr;
    item->prev      = (struct memheap_item *)start_addr;
    /* not in free list */
    item->next_free = item->prev_free = NULL;

    pthread_mutex_init(&(memheap->lock), NULL);

    return 0;
}

void *memheap_alloc(struct memheap *heap, size_t size)
{
    int result;
    uint32_t free_size;
    struct memheap_item *header_ptr;

    MEMHEAP_ASSERT(heap != NULL);

    /* align allocated size */
    size = MEMHEAP_ALIGN(size, MEMHEAP_ALIGN_SIZE);
    if (size < MEMHEAP_MINIALLOC)
    {
        size = MEMHEAP_MINIALLOC;
    }

    if (size < heap->available_size)
    {
        /* search on free list */
        free_size = 0;

        /* lock memheap */
        result = pthread_mutex_lock(&(heap->lock));
        if (result != 0)
        {
            return NULL;
        }

        /* get the first free memory block */
        header_ptr = heap->free_list->next_free;
        while (header_ptr != heap->free_list && free_size < size)
        {
            /* get current freed memory block size */
            free_size = MEMITEM_SIZE(header_ptr);
            if (free_size < size)
            {
                /* move to next free memory block */
                header_ptr = header_ptr->next_free;
            }
        }

        /* determine if the memory is available. */
        if (free_size >= size)
        {
            /* a block that satisfies the request has been found. */

            /* determine if the block needs to be split. */
            if (free_size >= (size + MEMHEAP_SIZE + MEMHEAP_MINIALLOC))
            {
                struct memheap_item *new_ptr;

                /* split the block. */
                new_ptr = (struct memheap_item *)
                          (((uint8_t *)header_ptr) + size + MEMHEAP_SIZE);

                /* mark the new block as a memory block and freed. */
                new_ptr->magic = MEMHEAP_MAGIC;

                /* put the pool pointer into the new block. */
                new_ptr->pool_ptr = heap;

                /* break down the block list */
                new_ptr->prev          = header_ptr;
                new_ptr->next          = header_ptr->next;
                header_ptr->next->prev = new_ptr;
                header_ptr->next       = new_ptr;

                /* remove header ptr from free list */
                header_ptr->next_free->prev_free = header_ptr->prev_free;
                header_ptr->prev_free->next_free = header_ptr->next_free;
                header_ptr->next_free = NULL;
                header_ptr->prev_free = NULL;

                /* insert new_ptr to free list */
                new_ptr->next_free = heap->free_list->next_free;
                new_ptr->prev_free = heap->free_list;
                heap->free_list->next_free->prev_free = new_ptr;
                heap->free_list->next_free            = new_ptr;

                /* decrement the available byte count.  */
                heap->available_size = heap->available_size -
                                       size -
                                       MEMHEAP_SIZE;
                if (heap->pool_size - heap->available_size > heap->max_used_size)
                {
                    heap->max_used_size = heap->pool_size - heap->available_size;
                }
            }
            else
            {
                /* decrement the entire free size from the available bytes count. */
                heap->available_size = heap->available_size - free_size;
                if (heap->pool_size - heap->available_size > heap->max_used_size)
                {
                    heap->max_used_size = heap->pool_size - heap->available_size;
                }

                header_ptr->next_free->prev_free = header_ptr->prev_free;
                header_ptr->prev_free->next_free = header_ptr->next_free;
                header_ptr->next_free = NULL;
                header_ptr->prev_free = NULL;
            }

            /* Mark the allocated block as not available. */
            header_ptr->magic |= MEMHEAP_USED;

            /* release lock */
            pthread_mutex_unlock(&(heap->lock));

            return (void *)((uint8_t *)header_ptr + MEMHEAP_SIZE);
        }

        /* release lock */
        pthread_mutex_unlock(&(heap->lock));
    }

    /* Return the completion status.  */
    return NULL;
}

void memheap_free(void *ptr)
{
    int result;
    struct memheap *heap;
    struct memheap_item *header_ptr, *new_ptr;
    uint32_t insert_header;

    if (ptr == NULL)
    {
        return;
    }

    /* set initial status as OK */
    insert_header = 1;
    new_ptr       = NULL;
    header_ptr    = (struct memheap_item *)
                    ((uint8_t *)ptr - MEMHEAP_SIZE);

    /* check magic */
    MEMHEAP_ASSERT((header_ptr->magic & MEMHEAP_MASK) == MEMHEAP_MAGIC);
    MEMHEAP_ASSERT(header_ptr->magic & MEMHEAP_USED);
    /* check whether this block of memory has been over-written. */
    MEMHEAP_ASSERT((header_ptr->next->magic & MEMHEAP_MASK) == MEMHEAP_MAGIC);

    /* get pool ptr */
    heap = header_ptr->pool_ptr;

    MEMHEAP_ASSERT(heap);

    /* lock memheap */
    result = pthread_mutex_init(&(heap->lock), NULL);
    if (result != 0)
    {
        return ;
    }

    /* Mark the memory as available. */
    header_ptr->magic &= ~MEMHEAP_USED;
    /* Adjust the available number of bytes. */
    heap->available_size = heap->available_size + MEMITEM_SIZE(header_ptr);

    /* Determine if the block can be merged with the previous neighbor. */
    if (!MEMHEAP_IS_USED(header_ptr->prev))
    {
        /* adjust the available number of bytes. */
        heap->available_size = heap->available_size + MEMHEAP_SIZE;

        /* NULL check */
        /* yes, merge block with previous neighbor. */
        (header_ptr->prev)->next = header_ptr->next;
        (header_ptr->next)->prev = header_ptr->prev;

        /* move header pointer to previous. */
        header_ptr = header_ptr->prev;
        /* don't insert header to free list */
        insert_header = 0;
    }

    /* determine if the block can be merged with the next neighbor. */
    if (!MEMHEAP_IS_USED(header_ptr->next))
    {
        /* adjust the available number of bytes. */
        heap->available_size = heap->available_size + MEMHEAP_SIZE;

        /* merge block with next neighbor. */
        new_ptr = header_ptr->next;

        new_ptr->next->prev = header_ptr;
        header_ptr->next    = new_ptr->next;

        /* remove new ptr from free list */
        new_ptr->next_free->prev_free = new_ptr->prev_free;
        new_ptr->prev_free->next_free = new_ptr->next_free;
    }

    if (insert_header)
    {
        /* no left merge, insert to free list */
        header_ptr->next_free = heap->free_list->next_free;
        header_ptr->prev_free = heap->free_list;
        heap->free_list->next_free->prev_free = header_ptr;
        heap->free_list->next_free            = header_ptr;

    }

    /* release lock */
    pthread_mutex_unlock(&(heap->lock));
}
