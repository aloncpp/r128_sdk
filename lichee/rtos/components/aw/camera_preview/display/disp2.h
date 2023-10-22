/**
 * @file disp2.h
 *
 */

#ifndef DISP2_H
#define DISP2_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_DISP2_SUNXI

#include "camera_preview.h"

void cp_disp2_get_sizes(uint32_t *width, uint32_t *height);
int cp_disp2_pan_display(struct camera_preview_buf dst_buf, int fbindex);

#endif  /*USE_DISP2*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*DISP2_H*/
