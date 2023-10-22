#include "spilcd.h"
#ifdef CONFIG_DRIVERS_SPILCD

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hal_lcd_fb.h>

static u32 g_screen_index = 0;

extern int bsp_disp_lcd_set_layer(unsigned int disp, struct fb_info *p_info);
extern int bsp_disp_get_screen_width(unsigned int disp);
extern int bsp_disp_get_screen_height(unsigned int disp);

static void fb_init(struct fb_info *p_info, struct camera_preview_buf dst_buf,
		uint32_t yoffset, uint32_t line_length)
{
    p_info->screen_base = dst_buf.addr;
    p_info->var.xres = dst_buf.width;
    p_info->var.yres = dst_buf.height;
    p_info->var.xoffset = 0;
    p_info->var.yoffset = yoffset;
    p_info->fix.line_length = line_length;
}

void cp_spilcd_get_sizes(uint32_t *width, uint32_t *height)
{
    *width = bsp_disp_get_screen_width(g_screen_index);
    *height = bsp_disp_get_screen_height(g_screen_index);
}

int cp_spilcd_pan_display(struct camera_preview_buf dst_buf, int fbindex)
{
    struct fb_info info;
    uint32_t yoffset, line_length;

	yoffset = dst_buf.height * fbindex;
	line_length = dst_buf.size / dst_buf.height;

    memset(&info, 0, sizeof(struct fb_info));
    fb_init(&info, dst_buf, yoffset, line_length);

    hal_dcache_clean((unsigned long)(dst_buf.addr + yoffset * line_length), dst_buf.size);
    bsp_disp_lcd_set_layer(g_screen_index, &info);
 
	bsp_disp_lcd_wait_for_vsync(g_screen_index);

    return 0;
}

#endif
