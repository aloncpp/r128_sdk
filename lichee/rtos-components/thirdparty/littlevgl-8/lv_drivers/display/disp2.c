/**
 * @file disp2.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "disp2.h"
#ifdef USE_DISP2

#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <video/sunxi_display2.h>
#include "disp_cfg_layer.h"
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
static u32 chn = 1;
static u32 layer = 0;

static uint32_t width;
static uint32_t height;
static char *fbsmem_start = 0;
static uint32_t bits_per_pixel;

extern int disp_ioctl(int cmd, void *arg);
extern int disp_release(void);
extern int disp_open(void);

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
static int disp2_set_layer(struct set_layer_cfg *cfg, uint32_t yoffset)
{
    long int line_length;
    if (!cfg) {
        printf("Error: failed to get set_layer_cfg");
        return -1;
    }
    cfg->screen_id = g_screen_index;
    cfg->bpp = bits_per_pixel;
    cfg->layer_cfg.channel = chn;
    cfg->layer_cfg.layer_id = layer;
    cfg->layer_cfg.info.fb.addr[0] = (int)(intptr_t) fbsmem_start;
    cfg->layer_cfg.info.fb.size[0].width = width;
    cfg->layer_cfg.info.fb.size[0].height = height;
    cfg->layer_cfg.info.fb.crop.x = 0;
    cfg->layer_cfg.info.fb.crop.y = yoffset;
    cfg->layer_cfg.info.fb.crop.width = width;
    cfg->layer_cfg.info.fb.crop.height = height;
    cfg->layer_cfg.info.screen_win.x = 0;
    cfg->layer_cfg.info.screen_win.y = 0;
    cfg->layer_cfg.info.fb.flags = DISP_BF_NORMAL;
    cfg->layer_cfg.info.mode = LAYER_MODE_BUFFER;
    cfg->layer_cfg.info.alpha_mode = 0;
    cfg->layer_cfg.info.alpha_value = 0xff;
    cfg->layer_cfg.info.zorder = 16;

    line_length = width * bits_per_pixel / 8;
    sunxifb_mem_flush_cache(fbsmem_start +
             yoffset * line_length, height * line_length);

    return disp_cfg_layer(g_screen_index, cfg);

}

void disp2_get_sizes(uint32_t *width, uint32_t *height) {
    unsigned long arg[6];

    disp_open();
    arg[0] = g_screen_index;
    *width = disp_ioctl(DISP_GET_SCN_WIDTH, (void *)arg);
    *height = disp_ioctl(DISP_GET_SCN_HEIGHT, (void *)arg);
    disp_release();
}

void disp2_init(uint32_t bpp, long int smem_len, char **mem_start)
{
    struct set_layer_cfg cfg;
    bits_per_pixel = bpp;

    disp2_get_sizes(&width, &height);
    LV_LOG_INFO("disp2_init: width = %d, height = %d\n", width, height);
    LV_LOG_INFO("bpp = %d, smem_len = %d\n", bpp, smem_len);

    disp_clear_layer(g_screen_index, chn, layer);

    fbsmem_start = sunxifb_mem_alloc(smem_len, "disp2");

    memset(fbsmem_start, 0, smem_len);
    *mem_start = fbsmem_start;
    LV_LOG_INFO("mem_start = 0x%p\n", *mem_start);
    if((intptr_t )*mem_start == -1) {
        printf("Error: failed to get framebuffer device memory");
        return;
    }

    memset(&cfg, 0, sizeof(struct set_layer_cfg));
    disp2_set_layer(&cfg, 0);
}

void disp2_exit(void)
{
    disp_clear_layer(g_screen_index, chn, layer);
    usleep(1);
    sunxifb_mem_free((void**) &fbsmem_start, "disp2");
}

int disp2_pan_display(uint32_t yoffset)
{
    struct set_layer_cfg cfg;
    unsigned long arg[6];
    memset(&cfg, 0, sizeof(struct set_layer_cfg));
    disp2_set_layer(&cfg, yoffset);
    //wait for vsync
    disp_open();
    arg[0] = g_screen_index;
    disp_ioctl(DISP_WAIT_VSYNC, (void *)arg);
    disp_release();

    return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif
