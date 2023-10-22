#include <stdlib.h>
#include "spilcd.h"
#include "disp2.h"
#include "display.h"

void display_get_sizes(uint32_t *width, uint32_t *height)
{
#ifdef CONFIG_DRIVERS_SPILCD
	cp_spilcd_get_sizes(width, height);
#elif defined(CONFIG_DISP2_SUNXI)
	cp_disp2_get_sizes(width, height);
#endif
}

void display_pan_display(struct camera_preview_buf dst_buf, int fbindex)
{
	cp_dbg("Display Image on LCD.\n");
#ifdef CONFIG_DRIVERS_SPILCD
	cp_spilcd_pan_display(dst_buf, fbindex);
#elif defined(CONFIG_DISP2_SUNXI)
	cp_disp2_pan_display(dst_buf, fbindex);
#endif
}

