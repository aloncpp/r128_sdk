/**
 * @file spilcd.h
 *
 */

#ifndef SPILCD_H
#define SPILCD_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#ifndef LV_DRV_NO_CONF
#ifdef LV_CONF_INCLUDE_SIMPLE
#include "lv_drv_conf.h"
#else
#include "../../lv_drv_conf.h"
#endif
#endif

#ifdef USE_SPILCD

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void spilcd_get_sizes(uint32_t *width, uint32_t *height);
void spilcd_init(uint32_t bpp, long int smem_len, char **mem_start);
void spilcd_exit(void);
int spilcd_pan_display(uint32_t yoffset);

/**********************
 *      MACROS
 **********************/

#endif  /*USE_SPILCD*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /*SPILCD_H*/
