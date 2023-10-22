/*
 * FreeRTOS Kernel V10.4.3
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/*
 * A sample implementation of pvPortMalloc() and vPortFree() that combines
 * (coalescences) adjacent memory blocks as they are freed, and in so doing
 * limits memory fragmentation.
 *
 * See heap_1.c, heap_2.c and heap_3.c for alternative implementations, and the
 * memory management pages of https://www.FreeRTOS.org for more information.
 */
#include <stdlib.h>
#include <string.h>
#include <aw_malloc.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
 * all the API functions to use the MPU wrappers.  That should only be done when
 * task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

extern unsigned long _bss_end;

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
    #error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE    ( ( size_t ) ( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE         ( ( size_t ) 8 )

/* Allocate the memory for the heap. */
#if ( configAPPLICATION_ALLOCATED_HEAP == 1 )
/* The application writer has already defined the array used for the RTOS
* heap - probably so it can be placed in a special segment or address. */
    extern uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#else
#endif /* configAPPLICATION_ALLOCATED_HEAP */

/* Define the linked list structure.  This is used to link free blocks in order
 * of their memory address. */
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK * pxNextFreeBlock; /*<< The next free block in the list. */
    size_t xBlockSize;                     /*<< The size of the free block. */
} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( int heapID, BlockLink_t * pxBlockToInsert ) PRIVILEGED_FUNCTION;

/*
 * Called automatically to setup the required heap structures the first time
 * pvPortMalloc() is called.
 */
static void prvHeapInit( int heapID, unsigned long heap_start, int size) PRIVILEGED_FUNCTION;

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
 * block must by correctly byte aligned. */
static const size_t xHeapStructSize = ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );

/* Create a couple of list links to mark the start and end of the list. */
PRIVILEGED_DATA static BlockLink_t xStart[CONFIG_AW_MEM_HEAP_COUNT], * pxEnd[CONFIG_AW_MEM_HEAP_COUNT] = {NULL};

/* Keeps track of the number of calls to allocate and free memory as well as the
 * number of free bytes remaining, but says nothing about fragmentation. */
PRIVILEGED_DATA static size_t xFreeBytesRemaining[CONFIG_AW_MEM_HEAP_COUNT] = {0U};
PRIVILEGED_DATA static size_t xMinimumEverFreeBytesRemaining[CONFIG_AW_MEM_HEAP_COUNT] = {0U};
PRIVILEGED_DATA static size_t xNumberOfSuccessfulAllocations[CONFIG_AW_MEM_HEAP_COUNT] = {0};
PRIVILEGED_DATA static size_t xNumberOfSuccessfulFrees[CONFIG_AW_MEM_HEAP_COUNT] = {0};

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
 * member of an BlockLink_t structure is set then the block belongs to the
 * application.  When the bit is free the block is still part of the free heap
 * space. */
PRIVILEGED_DATA static size_t xBlockAllocatedBit[CONFIG_AW_MEM_HEAP_COUNT] = {0};

/*-----------------------------------------------------------*/

void * __internal_malloc( int heapID, size_t xWantedSize )
{
    BlockLink_t * pxBlock, * pxPreviousBlock, * pxNewBlockLink;
    void * pvReturn = NULL;

    vTaskSuspendAll();
    {
        /* If this is the first call to malloc then the heap will require
         * initialisation to setup the list of free blocks. */
        if( pxEnd[heapID] == NULL )
        {
            prvHeapInit(heapID, 0, 0);
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        /* Check the requested block size is not so large that the top bit is
         * set.  The top bit of the block size member of the BlockLink_t structure
         * is used to determine who owns the block - the application or the
         * kernel, so it must be free. */
        if( ( xWantedSize & xBlockAllocatedBit[heapID] ) == 0 )
        {
            /* The wanted size must be increased so it can contain a BlockLink_t
             * structure in addition to the requested amount of bytes. */
            if( ( xWantedSize > 0 ) &&
                ( ( xWantedSize + xHeapStructSize ) >  xWantedSize ) ) /* Overflow check */
            {
                xWantedSize += xHeapStructSize;

                /* Ensure that blocks are always aligned. */
                if( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
                {
                    /* Byte alignment required. Check for overflow. */
                    if( ( xWantedSize + ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) ) )
                            > xWantedSize )
                    {
                        xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
                        configASSERT( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) == 0 );
                    }
                    else
                    {
                        xWantedSize = 0;
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                xWantedSize = 0;
            }

            if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining[heapID] ) )
            {
                /* Traverse the list from the start	(lowest address) block until
                 * one of adequate size is found. */
                pxPreviousBlock = &xStart[heapID];
                pxBlock = xStart[heapID].pxNextFreeBlock;

                while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
                {
                    pxPreviousBlock = pxBlock;
                    pxBlock = pxBlock->pxNextFreeBlock;
                }

                /* If the end marker was reached then a block of adequate size
                 * was not found. */
                if( pxBlock != pxEnd[heapID] )
                {
                    /* Return the memory space pointed to - jumping over the
                     * BlockLink_t structure at its start. */
                    pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

                    /* This block is being returned for use so must be taken out
                     * of the list of free blocks. */
                    pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

                    /* If the block is larger than required it can be split into
                     * two. */
                    if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
                    {
                        /* This block is to be split into two.  Create a new
                         * block following the number of bytes requested. The void
                         * cast is used to prevent byte alignment warnings from the
                         * compiler. */
                        pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );
                        configASSERT( ( ( ( size_t ) pxNewBlockLink ) & portBYTE_ALIGNMENT_MASK ) == 0 );

                        /* Calculate the sizes of two blocks split from the
                         * single block. */
                        pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                        pxBlock->xBlockSize = xWantedSize;

                        /* Insert the new block into the list of free blocks. */
                        prvInsertBlockIntoFreeList( heapID, pxNewBlockLink );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    xFreeBytesRemaining[heapID] -= pxBlock->xBlockSize;

                    if( xFreeBytesRemaining[heapID] < xMinimumEverFreeBytesRemaining[heapID] )
                    {
                        xMinimumEverFreeBytesRemaining[heapID] = xFreeBytesRemaining[heapID];
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    /* The block is being returned - it is allocated and owned
                     * by the application and has no "next" block. */
                    pxBlock->xBlockSize |= xBlockAllocatedBit[heapID];
                    pxBlock->pxNextFreeBlock = NULL;
                    xNumberOfSuccessfulAllocations[heapID]++;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceMALLOC( pvReturn, xWantedSize );
    }
    ( void ) xTaskResumeAll();

    #if ( configUSE_MALLOC_FAILED_HOOK == 1 )
        {
            if( pvReturn == NULL )
            {
                extern void vApplicationMallocFailedHook( void );
                vApplicationMallocFailedHook();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
    #endif /* if ( configUSE_MALLOC_FAILED_HOOK == 1 ) */

    configASSERT( ( ( ( size_t ) pvReturn ) & ( size_t ) portBYTE_ALIGNMENT_MASK ) == 0 );
    return pvReturn;
}
/*-----------------------------------------------------------*/

void __internal_free( int heapID, void * pv )
{
    uint8_t * puc = ( uint8_t * ) pv;
    BlockLink_t * pxLink;

    if( pv != NULL )
    {
        /* The memory being freed will have an BlockLink_t structure immediately
         * before it. */
        puc -= xHeapStructSize;

        /* This casting is to keep the compiler from issuing warnings. */
        pxLink = ( void * ) puc;

        /* Check the block is actually allocated. */
        configASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit[heapID] ) != 0 );
        configASSERT( pxLink->pxNextFreeBlock == NULL );

        if( ( pxLink->xBlockSize & xBlockAllocatedBit[heapID] ) != 0 )
        {
            if( pxLink->pxNextFreeBlock == NULL )
            {
                /* The block is being returned to the heap - it is no longer
                 * allocated. */
                pxLink->xBlockSize &= ~xBlockAllocatedBit[heapID];

                vTaskSuspendAll();
                {
                    /* Add this block to the list of free blocks. */
                    xFreeBytesRemaining[heapID] += pxLink->xBlockSize;
                    traceFREE( pv, pxLink->xBlockSize );
                    prvInsertBlockIntoFreeList( heapID, ( ( BlockLink_t * ) pxLink ) );
                    xNumberOfSuccessfulFrees[heapID]++;
                }
                ( void ) xTaskResumeAll();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
}
/*-----------------------------------------------------------*/

size_t aw_xPortGetFreeHeapSize( int heapID )
{
    return xFreeBytesRemaining[heapID];
}

size_t xPortGetFreeHeapSize( void )
{
    int heapID = -1;

#ifdef CONFIG_DEFAULT_DRAM_HEAP
    heapID = DRAM_HEAP_ID;
#elif defined(CONFIG_DEFAULT_SRAM_HEAP)
    heapID = SRAM_HEAP_ID;
#elif defined(CONFIG_DEFAULT_LPSRAM_HEAP)
    heapID = LPSRAM_HEAP_ID;
#elif defined(CONFIG_DEFAULT_HPSRAM_HEAP)
    heapID = HPSRAM_HEAP_ID;
#else
#error "error heapID"
#endif

	return aw_xPortGetFreeHeapSize(heapID);
}
/*-----------------------------------------------------------*/

size_t aw_xPortGetMinimumEverFreeHeapSize( int heapID )
{
    return xMinimumEverFreeBytesRemaining[heapID];
}

size_t xPortGetMinimumEverFreeHeapSize( void )
{
    int heapID = -1;

#ifdef CONFIG_DEFAULT_DRAM_HEAP
    heapID = DRAM_HEAP_ID;
#elif defined(CONFIG_DEFAULT_SRAM_HEAP)
    heapID = SRAM_HEAP_ID;
#elif defined(CONFIG_DEFAULT_LPSRAM_HEAP)
    heapID = LPSRAM_HEAP_ID;
#elif defined(CONFIG_DEFAULT_HPSRAM_HEAP)
    heapID = HPSRAM_HEAP_ID;
#else
#error "error heapID"
#endif

    return aw_xPortGetMinimumEverFreeHeapSize(heapID);
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks( void )
{
    /* This just exists to keep the linker quiet. */
}
/*-----------------------------------------------------------*/

static void prvHeapInit( int heapID, unsigned long ucHeap, int size) /* PRIVILEGED_FUNCTION */
{
    BlockLink_t * pxFirstFreeBlock;
    uint8_t * pucAlignedHeap;
    size_t uxAddress;
    size_t xTotalHeapSize = size;

    /* Ensure the heap starts on a correctly aligned boundary. */
    uxAddress = ( size_t ) ucHeap;

    if( ( uxAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
    {
        uxAddress += ( portBYTE_ALIGNMENT - 1 );
        uxAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
        xTotalHeapSize -= uxAddress - ( size_t ) ucHeap;
    }

    pucAlignedHeap = ( uint8_t * ) uxAddress;

    /* xStart[heapID] is used to hold a pointer to the first item in the list of free
     * blocks.  The void cast is used to prevent compiler warnings. */
    xStart[heapID].pxNextFreeBlock = ( void * ) pucAlignedHeap;
    xStart[heapID].xBlockSize = ( size_t ) 0;

    /* pxEnd[heapID] is used to mark the end of the list of free blocks and is inserted
     * at the end of the heap space. */
    uxAddress = ( ( size_t ) pucAlignedHeap ) + xTotalHeapSize;
    uxAddress -= xHeapStructSize;
    uxAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
    pxEnd[heapID] = ( void * ) uxAddress;
    pxEnd[heapID]->xBlockSize = 0;
    pxEnd[heapID]->pxNextFreeBlock = NULL;

    /* To start with there is a single free block that is sized to take up the
     * entire heap space, minus the space taken by pxEnd[heapID]. */
    pxFirstFreeBlock = ( void * ) pucAlignedHeap;
    pxFirstFreeBlock->xBlockSize = uxAddress - ( size_t ) pxFirstFreeBlock;
    pxFirstFreeBlock->pxNextFreeBlock = pxEnd[heapID];

    /* Only one block exists - and it covers the entire usable heap space. */
    xMinimumEverFreeBytesRemaining[heapID] = pxFirstFreeBlock->xBlockSize;
    xFreeBytesRemaining[heapID] = pxFirstFreeBlock->xBlockSize;

    /* Work out the position of the top bit in a size_t variable. */
    xBlockAllocatedBit[heapID] = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 );
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( int heapID, BlockLink_t * pxBlockToInsert ) /* PRIVILEGED_FUNCTION */
{
    BlockLink_t * pxIterator;
    uint8_t * puc;

    /* Iterate through the list until a block is found that has a higher address
     * than the block being inserted. */
    for( pxIterator = &xStart[heapID]; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
    {
        /* Nothing to do here, just iterate to the right position. */
    }

    /* Do the block being inserted, and the block it is being inserted after
     * make a contiguous block of memory? */
    puc = ( uint8_t * ) pxIterator;

    if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
    {
        pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
        pxBlockToInsert = pxIterator;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    /* Do the block being inserted, and the block it is being inserted before
     * make a contiguous block of memory? */
    puc = ( uint8_t * ) pxBlockToInsert;

    if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextFreeBlock )
    {
        if( pxIterator->pxNextFreeBlock != pxEnd[heapID] )
        {
            /* Form one big block from the two blocks. */
            pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
            pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
        }
        else
        {
            pxBlockToInsert->pxNextFreeBlock = pxEnd[heapID];
        }
    }
    else
    {
        pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
    }

    /* If the block being inserted plugged a gab, so was merged with the block
     * before and the block after, then it's pxNextFreeBlock pointer will have
     * already been set, and should not be set here as that would make it point
     * to itself. */
    if( pxIterator != pxBlockToInsert )
    {
        pxIterator->pxNextFreeBlock = pxBlockToInsert;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }
}
/*-----------------------------------------------------------*/

void aw_vPortGetHeapStats(int heapID, HeapStats_t *pxHeapStats)
{
    BlockLink_t * pxBlock;
    size_t xBlocks = 0, xMaxSize = 0, xMinSize = portMAX_DELAY; /* portMAX_DELAY used as a portable way of getting the maximum value. */

    vTaskSuspendAll();
    {
        pxBlock = xStart[heapID].pxNextFreeBlock;

        /* pxBlock will be NULL if the heap has not been initialised.  The heap
         * is initialised automatically when the first allocation is made. */
        if( pxBlock != NULL )
        {
            do
            {
                /* Increment the number of blocks and record the largest block seen
                 * so far. */
                xBlocks++;

                if( pxBlock->xBlockSize > xMaxSize )
                {
                    xMaxSize = pxBlock->xBlockSize;
                }

                if( pxBlock->xBlockSize < xMinSize )
                {
                    xMinSize = pxBlock->xBlockSize;
                }

                /* Move to the next block in the chain until the last block is
                 * reached. */
                pxBlock = pxBlock->pxNextFreeBlock;
            } while( pxBlock != pxEnd[heapID] );
        }
    }
    ( void ) xTaskResumeAll();

    pxHeapStats->xSizeOfLargestFreeBlockInBytes = xMaxSize;
    pxHeapStats->xSizeOfSmallestFreeBlockInBytes = xMinSize;
    pxHeapStats->xNumberOfFreeBlocks = xBlocks;

    taskENTER_CRITICAL();
    {
        pxHeapStats->xAvailableHeapSpaceInBytes = xFreeBytesRemaining[heapID];
        pxHeapStats->xNumberOfSuccessfulAllocations = xNumberOfSuccessfulAllocations[heapID];
        pxHeapStats->xNumberOfSuccessfulFrees = xNumberOfSuccessfulFrees[heapID];
        pxHeapStats->xMinimumEverFreeBytesRemaining = xMinimumEverFreeBytesRemaining[heapID];
    }
    taskEXIT_CRITICAL();
}

void vPortGetHeapStats( HeapStats_t * pxHeapStats )
{
    int heapID = -1;

#ifdef CONFIG_DEFAULT_DRAM_HEAP
    heapID = DRAM_HEAP_ID;
#elif defined(CONFIG_DEFAULT_SRAM_HEAP)
    heapID = SRAM_HEAP_ID;
#elif defined(CONFIG_DEFAULT_LPSRAM_HEAP)
    heapID = LPSRAM_HEAP_ID;
#elif defined(CONFIG_DEFAULT_HPSRAM_HEAP)
    heapID = HPSRAM_HEAP_ID;
#else
#error "error heapID"
#endif

    aw_vPortGetHeapStats(heapID, pxHeapStats);
}

void *pvPortMallocAlign(size_t xWantedSize, size_t align)
{
        void *ptr;
        void *align_ptr;
        size_t uintptr_size;
        size_t align_size;

        /* sizeof pointer */
        uintptr_size = sizeof(void*);
        uintptr_size -= 1;

        /* align the alignment size to uintptr size byte */
        align = ((align + uintptr_size) & ~uintptr_size);

        /* get total aligned size */
        align_size = ((xWantedSize + uintptr_size) & ~uintptr_size) + align;
        /* allocate memory block from heap */
        ptr = pvPortMalloc(align_size);
        if (ptr != NULL)
        {
                /* the allocated memory block is aligned */
                if (((uint32_t)ptr & (align - 1)) == 0)
                {
                        align_ptr = (void *)((uint32_t)ptr + align);
                }
                else
                {
                        align_ptr = (void *)(((uint32_t)ptr + (align - 1)) & ~(align - 1));
                }

                /* set the pointer before alignment pointer to the real pointer */
                *((uint32_t *)((uint32_t)align_ptr - sizeof(void *))) = (uint32_t)ptr;

                ptr = align_ptr;
        }

        return ptr;
}

void vPortFreeAlign(void *ptr)
{
    void *real_ptr;

    real_ptr = (void *) * (uint32_t *)((uint32_t)ptr - sizeof(void *));
    vPortFree(real_ptr);
}

void *pvPortCalloc(size_t count, size_t size)
{
    void *result = NULL;

    result = pvPortMalloc(count * size);
    if(result)
    {
        memset(result, 0, count * size);
    }

    return result;
}

void *pvPortRealloc(void * old, size_t newlen)
{
    void *result = NULL;
    size_t oldsize = 0;

    if(old == NULL)
    {
        if((result = pvPortMalloc(newlen)) == NULL)
        {
            return NULL;
        }
        return result;
    }

    if(newlen == 0)
    {
        vPortFree(old);
        return result;
    }

    if((result = pvPortMalloc(newlen)) == NULL)
    {
        return result;
    }

    uint8_t *puc = (uint8_t *)old;
    BlockLink_t * pxLink = (BlockLink_t *)(puc - xHeapStructSize);
    oldsize = pxLink->xBlockSize - xHeapStructSize;

    memcpy(result, old, newlen > oldsize ? oldsize : newlen);
    vPortFree(old);

    return result;
}

static void (*memleak_malloc_hook)(void *ptr, uint32_t size);
static void (*memleak_free_hook)(void *ptr);

void memleak_malloc_sethook(void (*hook)(void *ptr, uint32_t size))
{
    memleak_malloc_hook = hook;
}

void memleak_free_sethook(void (*hook)(void *ptr))
{
    memleak_free_hook = hook;
}

void vPortFree(void *ptr)
{
    int heapID = -1;

    unsigned long ptrAddr = (unsigned long)ptr;

#ifdef CONFIG_HPSRAM_HEAP
    unsigned long _hpsram_heap_start = 0;
#endif
#ifdef CONFIG_LPSRAM_HEAP
    unsigned long _lpsram_heap_start = 0;
#endif
#ifdef CONFIG_SRAM_HEAP
    unsigned long _sram_heap_start = 0;
#endif
#ifdef CONFIG_DRAM_HEAP
    unsigned long _dram_heap_start = 0;
#endif

#if defined CONFIG_DEFAULT_DRAM_HEAP
    _dram_heap_start = (unsigned long)&_bss_end;
#elif defined CONFIG_DEFAULT_SRAM_HEAP
    _sram_heap_start = (unsigned long)&_bss_end;
#elif defined CONFIG_DEFAULT_LPSRAM_HEAP
    _lpsram_heap_start = (unsigned long)&_bss_end;
#elif defined CONFIG_DEFAULT_HPSRAM_HEAP
    _hpsram_heap_start = (unsigned long)&_bss_end;
#else
#error "The default heap need to set"
#endif

#ifdef CONFIG_SRAM_HEAP_START_ADDR
    _sram_heap_start = CONFIG_SRAM_HEAP_START_ADDR;
#endif

#ifdef CONFIG_DRAM_HEAP_START_ADDR
    _dram_heap_start = CONFIG_DRAM_HEAP_START_ADDR;
#endif

#ifdef CONFIG_LPSRAM_HEAP_START_ADDR
    _lpsram_heap_start = CONFIG_LPSRAM_HEAP_START_ADDR;
#endif

#ifdef CONFIG_HPSRAM_HEAP_START_ADDR
    _hpsram_heap_start = CONFIG_HPSRAM_HEAP_START_ADDR;
#endif

#ifdef CONFIG_DRAM_HEAP
    if (ptrAddr > _dram_heap_start
			&& ptrAddr < ((unsigned long)_dram_default_heap_start + CONFIG_DRAM_HEAP_SIZE)) {
		heapID = DRAM_HEAP_ID;
	}
#endif

#ifdef CONFIG_SRAM_HEAP
    if (ptrAddr > _sram_heap_start
			&& ptrAddr < ((unsigned long)_sram_heap_start + CONFIG_SRAM_HEAP_SIZE)) {
		heapID = SRAM_HEAP_ID;
	}
#endif

#ifdef CONFIG_LPSRAM_HEAP
    if (ptrAddr > _lpsram_heap_start
			&& ptrAddr < ((unsigned long)_lpsram_heap_start + CONFIG_LPSRAM_HEAP_SIZE)) {
		heapID = LPSRAM_HEAP_ID;
	}
#endif

#ifdef CONFIG_HPSRAM_HEAP
    if (ptrAddr > _hpsram_heap_start
			&& ptrAddr < ((unsigned long)_hpsram_heap_start + CONFIG_HPSRAM_HEAP_SIZE)) {
		heapID = HPSRAM_HEAP_ID;
	}
#endif

	if (heapID == -1) {
		return;
	}

    __internal_free(heapID, ptr);

    if(memleak_free_hook)
    {
        memleak_free_hook(ptr);
    }
}

void *pvPortMalloc(size_t size)
{
    void *ptr = NULL;

#ifdef CONFIG_DRAM_HEAP
    if (ptr == NULL)
    {
        ptr = __internal_malloc(DRAM_HEAP_ID, size);
	}
#endif

#ifdef CONFIG_SRAM_HEAP
    if (ptr == NULL)
    {
        ptr = __internal_malloc(SRAM_HEAP_ID, size);
	}
#endif

#ifdef CONFIG_LPSRAM_HEAP
    if (ptr == NULL)
    {
        ptr = __internal_malloc(LPSRAM_HEAP_ID, size);
	}
#endif

#ifdef CONFIG_HPSRAM_HEAP
    if (ptr == NULL)
    {
        ptr = __internal_malloc(HPSRAM_HEAP_ID, size);
	}
#endif

    if(ptr && memleak_malloc_hook)
    {
        memleak_malloc_hook(ptr, size);
    }
    return ptr;
}

void *aw_pvPortMalloc(int heapID, size_t size)
{
    return __internal_malloc(heapID, size);
}

void aw_vPortFree(int heapID, void *ptr)
{
    __internal_free(heapID, ptr);
}

#ifdef CONFIG_SRAM_HEAP
void *aw_sram_pvPortMalloc(size_t size)
{
    return __internal_malloc(SRAM_HEAP_ID, size);
}

void aw_sram_vPortFree(void *ptr)
{
    __internal_free(SRAM_HEAP_ID, ptr);
}
#endif

#ifdef CONFIG_DRAM_HEAP
void *aw_dram_pvPortMalloc(size_t size)
{
    return __internal_malloc(DRAM_HEAP_ID, size);
}

void aw_dram_vPortFree(void *ptr)
{
    __internal_free(DRAM_HEAP_ID, ptr);
}
#endif

#ifdef CONFIG_LPSRAM_HEAP
void *aw_lpsram_pvPortMalloc(size_t size)
{
    return __internal_malloc(LPSRAM_HEAP_ID, size);
}

void aw_lpsram_vPortFree(void *ptr)
{
    __internal_free(LPSRAM_HEAP_ID, ptr);
}
#endif

#ifdef CONFIG_HPSRAM_HEAP
void *aw_hpsram_pvPortMalloc(size_t size)
{
    return __internal_malloc(HPSRAM_HEAP_ID, size);
}

void aw_hpsram_vPortFree(void *ptr)
{
    __internal_free(HPSRAM_HEAP_ID, ptr);
}
#endif

void heap_init(void)
{
#ifdef CONFIG_HPSRAM_HEAP
    unsigned long _hpsram_heap_start = 0;
#endif
#ifdef CONFIG_LPSRAM_HEAP
    unsigned long _lpsram_heap_start = 0;
#endif
#ifdef CONFIG_SRAM_HEAP
    unsigned long _sram_heap_start = 0;
#endif
#ifdef CONFIG_DRAM_HEAP
    unsigned long _dram_heap_start = 0;
#endif

#if defined CONFIG_DEFAULT_DRAM_HEAP
    _dram_heap_start = (unsigned long)&_bss_end;
#elif defined CONFIG_DEFAULT_SRAM_HEAP
    _sram_heap_start = (unsigned long)&_bss_end;
#elif defined CONFIG_DEFAULT_LPSRAM_HEAP
    _lpsram_heap_start = (unsigned long)&_bss_end;
#elif defined CONFIG_DEFAULT_HPSRAM_HEAP
    _hpsram_heap_start = (unsigned long)&_bss_end;
#else
#error "The default heap need to set"
#endif

#ifdef CONFIG_SRAM_HEAP_START_ADDR
    _sram_heap_start = CONFIG_SRAM_HEAP_START_ADDR;
#endif

#ifdef CONFIG_DRAM_HEAP_START_ADDR
    _dram_heap_start = CONFIG_DRAM_HEAP_START_ADDR;
#endif

#ifdef CONFIG_LPSRAM_HEAP_START_ADDR
    _lpsram_heap_start = CONFIG_LPSRAM_HEAP_START_ADDR;
#endif

#ifdef CONFIG_HPSRAM_HEAP_START_ADDR
    _hpsram_heap_start = CONFIG_HPSRAM_HEAP_START_ADDR;
#endif

#ifdef CONFIG_DRAM_HEAP
	prvHeapInit(DRAM_HEAP_ID, _dram_heap_start, CONFIG_DRAM_HEAP_SIZE);
#endif

#ifdef CONFIG_SRAM_HEAP
	prvHeapInit(SRAM_HEAP_ID, _sram_heap_start, CONFIG_SRAM_HEAP_SIZE);
#endif

#ifdef CONFIG_LPSRAM_HEAP
	prvHeapInit(LPSRAM_HEAP_ID, _lpsram_heap_start, CONFIG_LPSRAM_HEAP_SIZE);
#endif

#ifdef CONFIG_HPSRAM_HEAP
	prvHeapInit(HPSRAM_HEAP_ID, _hpsram_heap_start, CONFIG_HPSRAM_HEAP_SIZE);
#endif
}
