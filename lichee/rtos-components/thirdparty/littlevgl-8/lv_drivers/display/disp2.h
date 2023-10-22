/**
 * @file disp2.h
 *
 */

#ifndef DISP2_H
#define DISP2_H

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

#ifdef USE_DISP2

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

void disp2_get_sizes(uint32_t *width, uint32_t *height);
void disp2_init(uint32_t bpp, long int smem_len, char **mem_start);
void disp2_exit(void);
int disp2_pan_display(uint32_t yoffset);

/**********************
 *      MACROS
 **********************/

#endif  /*USE_DISP2*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*DISP2_H*/
