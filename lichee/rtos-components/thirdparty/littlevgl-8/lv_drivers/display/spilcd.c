/**
 * @file spilcd.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "spilcd.h"
#ifdef USE_SPILCD

#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hal_lcd_fb.h>
#include "sunximem.h"

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
static u32 g_screen_index = 0;
static uint32_t width;
static uint32_t height;
static uint32_t line_length;
static char *fbsmem_start = 0;

extern int bsp_disp_lcd_set_layer(unsigned int disp, struct fb_info *p_info);
extern int bsp_disp_get_screen_width(unsigned int disp);
extern int bsp_disp_get_screen_height(unsigned int disp);

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
static void fb_init(struct fb_info *p_info, uint32_t yoffset)
{
    p_info->screen_base = fbsmem_start;
    p_info->var.xres = width;
    p_info->var.yres = height;
    p_info->var.xoffset = 0;
    p_info->var.yoffset = yoffset;
    p_info->fix.line_length = line_length;
}

void spilcd_get_sizes(uint32_t *width, uint32_t *height)
{
    *width = bsp_disp_get_screen_width(g_screen_index);
    *height = bsp_disp_get_screen_height(g_screen_index);
}

void spilcd_init(uint32_t bpp, long int smem_len, char **mem_start)
{
    struct fb_info info;

    spilcd_get_sizes(&width, &height);
    LV_LOG_INFO("spilcd_init: width = %d, height = %d\n", width, height);

    line_length = width * bpp / 8;
    LV_LOG_INFO("bpp = %d, line_length = %d, smem_len = %d\n",
            bpp, line_length, smem_len);

    fbsmem_start = sunxifb_mem_alloc(smem_len, "spilcd");

    memset(fbsmem_start, 0, smem_len);
    *mem_start = fbsmem_start;
    LV_LOG_INFO("mem_start = %p\n", *mem_start);
    if((intptr_t )*mem_start == -1) {
        perror("Error: failed to get framebuffer device memory");
        return;
    }

    memset(&info, 0, sizeof(struct fb_info));
    fb_init(&info, 0);

    sunxifb_mem_flush_cache(fbsmem_start, height * line_length);
    bsp_disp_lcd_set_layer(g_screen_index, &info);
}

void spilcd_exit(void)
{
    sunxifb_mem_free((void**) &fbsmem_start, "spilcd");
}

int spilcd_pan_display(uint32_t yoffset)
{
    struct fb_info info;
    memset(&info, 0, sizeof(struct fb_info));
    fb_init(&info, yoffset);

    sunxifb_mem_flush_cache(fbsmem_start +
             yoffset * line_length, height * line_length);
    bsp_disp_lcd_set_layer(g_screen_index, &info);
    bsp_disp_lcd_wait_for_vsync(g_screen_index);

    return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif
