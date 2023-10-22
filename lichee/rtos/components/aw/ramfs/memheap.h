#ifndef _MEMHEAP_H_
#define _MEMHEAP_H_

#include <pthread.h>
#include <stdint.h>

struct memheap_item
{
    uint32_t             magic;                      /* *< magic number for memheap */
    struct memheap      *pool_ptr;                   /* *< point of pool */

    struct memheap_item *next;                       /* *< next memheap item */
    struct memheap_item *prev;                       /* *< prev memheap item */

    struct memheap_item *next_free;                  /* *< next free memheap item */
    struct memheap_item *prev_free;                  /* *< prev free memheap item */
};

struct memheap
{
    void                   *start_addr;                 /* *< pool start address and size */
    uint32_t             pool_size;                  /* *< pool size */
    uint32_t             available_size;             /* *< available size */
    uint32_t             max_used_size;              /* *< maximum allocated size */
    struct memheap_item *block_list;                 /* *< used block list */
    struct memheap_item *free_list;                  /* *< free block list */
    struct memheap_item  free_header;                /* *< free block list header */
    pthread_mutex_t     lock;                       /* *< semaphore lock */
};

int memheap_init(struct memheap *memheap,
                 const char        *name,
                 void              *start_addr,
                 size_t         size);

void *memheap_alloc(struct memheap *heap, size_t size);

void memheap_free(void *ptr);
#endif
