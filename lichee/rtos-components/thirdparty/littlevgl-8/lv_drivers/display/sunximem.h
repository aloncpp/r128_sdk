/**
 * @file sunximem.h
 *
 */

#ifndef SUNXIMEM_H
#define SUNXIMEM_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#if 1

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include <sunxi_hal_common.h>
#include <hal_cache.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
int sunxifb_mem_init(void);
void sunxifb_mem_deinit(void);
void* sunxifb_mem_alloc(size_t size, char *label);
void sunxifb_mem_free(void **data, char *label);
void* sunxifb_mem_get_phyaddr(void *data);
void sunxifb_mem_flush_cache(void *data, size_t size);

/**********************
 *      MACROS
 **********************/

#endif  /*USE_SUNXIFB_G2D*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*SUNXIMEM_H*/
