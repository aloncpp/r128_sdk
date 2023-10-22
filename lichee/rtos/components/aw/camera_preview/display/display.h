#ifndef DISPLAY_H
#define DISPLAY_H

#include "camera_preview.h"

void display_get_sizes(uint32_t *width, uint32_t *height);
void display_pan_display(struct camera_preview_buf dst_buf, int fbindex);

#endif  /*DISPLAY_H*/
