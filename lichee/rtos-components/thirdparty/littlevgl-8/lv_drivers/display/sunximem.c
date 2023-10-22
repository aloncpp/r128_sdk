/**
 * @file sunximem.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "sunximem.h"

#if 1

#include <stdio.h>
#include <stdlib.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      STRUCTURES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int sunxifb_mem_init(void) {
    return 0;
}

void sunxifb_mem_deinit(void) {
}

void* sunxifb_mem_alloc(size_t size, char *label) {
    if (size == 0) {
        printf("size illegal.");
        return NULL;
    }

#ifdef USE_SUNXIFB_CACHE
    void *alloc = hal_malloc_coherent(size);
#else
    void *alloc = malloc(size);
#endif
    if (alloc == NULL) {
        printf("couldn't allocate memory (%lu bytes).", (unsigned long) size);
        return NULL;
    }

#ifdef LV_USE_SUNXIFB_DEBUG
    printf("%s: sunxifb_mem_alloc=%p size=%lu bytes\n", label, alloc, (unsigned long) size);
#endif /* LV_USE_SUNXIFB_DEBUG */

    return alloc;
}

void sunxifb_mem_free(void **data, char *label) {
    if (*data != NULL) {
#ifdef LV_USE_SUNXIFB_DEBUG
        printf("%s: sunxifb_mem_free=%p\n", label, *data);
#endif /* LV_USE_SUNXIFB_DEBUG */
#ifdef USE_SUNXIFB_CACHE
        hal_free_coherent(*data);
#else
        free(*data);
#endif
        *data = NULL;
    } else {
        printf("couldn't free memory.\n");
    }
}

void* sunxifb_mem_get_phyaddr(void *data) {
    if (data != NULL)
        return data;
    else
        return NULL;
}

void sunxifb_mem_flush_cache(void *data, size_t size) {
#ifdef USE_SUNXIFB_CACHE
    hal_dcache_clean_invalidate((unsigned long)data, size);
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif
