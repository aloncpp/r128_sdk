#ifndef _AW_MALLOC_H
#define _AW_MALLOC_H

#ifdef CONFIG_HEAP_MULTIPLE

#define CONFIG_AW_MEM_HEAP_COUNT (4)

#define SRAM_HEAP_ID   (0)
#define LPSRAM_HEAP_ID (1)
#define HPSRAM_HEAP_ID (2)
#define DRAM_HEAP_ID   (3)

extern unsigned long __dram_heap_start;
extern unsigned long __sram_heap_start;
extern unsigned long __lpram_heap_start;
extern unsigned long __hpram_heap_start;

size_t aw_xPortGetTotalHeapSize( int heapID );
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
