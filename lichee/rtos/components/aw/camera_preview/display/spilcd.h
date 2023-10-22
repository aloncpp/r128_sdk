/**
 * @file spilcd.h
 *
 */

#ifndef SPILCD_H
#define SPILCD_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_DRIVERS_SPILCD

#include "camera_preview.h"

void cp_spilcd_get_sizes(uint32_t *width, uint32_t *height);
int cp_spilcd_pan_display(struct camera_preview_buf dst_buf, int fbindex);

#endif  /*USE_SPILCD*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /*SPILCD_H*/
