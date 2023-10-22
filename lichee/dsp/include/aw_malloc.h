#ifndef _AW_MALLOC_H
#define _AW_MALLOC_H

#ifdef CONFIG_MEMMANG_HEAP_MULTIPLE

#define CONFIG_AW_MEM_HEAP_COUNT (4)

#define SRAM_HEAP_ID   (0)
#define LPSRAM_HEAP_ID (1)
#define HPSRAM_HEAP_ID (2)
#define DRAM_HEAP_ID   (3)

/* Used to pass information about the heap out of vPortGetHeapStats(). */
typedef struct xHeapStats
{
    size_t xAvailableHeapSpaceInBytes;          /* The total heap size currently available - this is the sum of all the free blocks, not the largest block that can be allocated. */
    size_t xSizeOfLargestFreeBlockInBytes;      /* The maximum size, in bytes, of all the free blocks within the heap at the time vPortGetHeapStats() is called. */
    size_t xSizeOfSmallestFreeBlockInBytes;     /* The minimum size, in bytes, of all the free blocks within the heap at the time vPortGetHeapStats() is called. */
    size_t xNumberOfFreeBlocks;                 /* The number of free memory blocks within the heap at the time vPortGetHeapStats() is called. */
    size_t xMinimumEverFreeBytesRemaining;      /* The minimum amount of total free memory (sum of all free blocks) there has been in the heap since the system booted. */
    size_t xNumberOfSuccessfulAllocations;      /* The number of calls to pvPortMalloc() that have returned a valid memory block. */
    size_t xNumberOfSuccessfulFrees;            /* The number of calls to vPortFree() that has successfully freed a block of memory. */
} HeapStats_t;

extern unsigned long __dram_heap_start;
extern unsigned long __sram_heap_start;
extern unsigned long __lpram_heap_start;
extern unsigned long __hpram_heap_start;

size_t aw_xPortGetFreeHeapSize( int heapID );
size_t aw_xPortGetMinimumEverFreeHeapSize( int heapID );
void aw_vPortGetHeapStats( int heapID, HeapStats_t * pxHeapStats );

void *aw_pvPortMalloc(int heapID, size_t size);
void aw_vPortFree(int heapID, void *ptr);

void *aw_sram_pvPortMalloc(size_t size);
void aw_sram_vPortFree(void *ptr);

void *aw_dram_pvPortMalloc(size_t size);
void aw_dram_vPortFree(void *ptr);
void *aw_lpsram_pvPortMalloc(size_t size);
void aw_lpsram_vPortFree(void *ptr);
void *aw_hpsram_pvPortMalloc(size_t size);
void aw_hpsram_vPortFree(void *ptr);

#endif
#endif
