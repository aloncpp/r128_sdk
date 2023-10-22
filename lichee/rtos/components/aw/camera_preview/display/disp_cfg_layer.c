#ifdef CONFIG_DISP2_SUNXI

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "disp_cfg_layer.h"
int msleep(unsigned int msecs);
extern int disp_ioctl(int cmd, void *arg);
extern int disp_release(void);
extern int disp_open(void);

int cp_disp_clear_layer(u32 screen_index, u32 chn, u32 layer)
{
    struct disp_layer_config config;
    unsigned long arg[6];
    int ret = -1;
    disp_open();
    memset(&config, 0, sizeof(struct disp_layer_config));

    arg[0] = screen_index;
    arg[1] = (unsigned long)&config;
    arg[2] = 1;
    arg[3] = 0;

    config.enable = false;
    config.channel = chn;
    config.layer_id = layer;
    ret = disp_ioctl(DISP_LAYER_SET_CONFIG,
                  (void *)arg);
    msleep(10);
    if (0 != ret)
        printf("fail to set layer cfg\n");
    disp_release();

    return ret;

}

int cp_disp_clear_all_layer(u32 screen_index)
{
    struct disp_layer_config config;
    unsigned long arg[6];
    int ret = -1, i ,j;

    memset(&config, 0, sizeof(struct disp_layer_config));

    arg[0] = screen_index;
    arg[1] = (unsigned long)&config;
    arg[2] = 1;
    arg[3] = 0;

    disp_open();
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            config.enable = false;
            config.channel = i;
            config.layer_id = j;
            ret = disp_ioctl(DISP_LAYER_SET_CONFIG,
                          (void *)arg);
            msleep(10);
            if (0 != ret)
                printf("fail to set layer cfg %d\n",ret);
        }
    }

    disp_release();
    return ret;

}

int cp_disp_cfg_layer(u32 g_screen_index, struct set_layer_cfg *set_cfg)
{
    unsigned long arg[6];
    int ret, width, height, fb_width, fb_height;

    if (!set_cfg) {
        printf("Null pointer!\n");
        return -1;
    }

    if (1 == set_cfg->fullscreen) {
        /* full-screen display */
        printf("full screen display\n");
    }
    disp_open();

    arg[0] = set_cfg->screen_id;
    width = disp_ioctl(DISP_GET_SCN_WIDTH, (void *)arg);
    height = disp_ioctl(DISP_GET_SCN_HEIGHT, (void *)arg);

    fb_width = set_cfg->layer_cfg.info.fb.size[0].width;
    fb_height = set_cfg->layer_cfg.info.fb.size[0].height;

    if (set_cfg->layer_cfg.info.fb.flags & DISP_BF_STEREO_FP) {
        fb_height = fb_height / 2;
    }
    set_cfg->layer_cfg.info.fb.size[0].width = fb_width;
    set_cfg->layer_cfg.info.fb.size[0].height = fb_height;

    switch (set_cfg->bpp) {
        case 32:
            set_cfg->layer_cfg.info.fb.format = DISP_FORMAT_ARGB_8888;
            break;
        case 24:
            set_cfg->layer_cfg.info.fb.format = DISP_FORMAT_RGB_888;
            break;
        case 16:
            set_cfg->layer_cfg.info.fb.format = DISP_FORMAT_RGB_565;
            break;
        default:
            printf("invalid bits_per_pixel :%d\n",
                    set_cfg->bpp);
            set_cfg->layer_cfg.info.fb.format = 0;
            break;
    }

    /* INTERLEAVED */
    set_cfg->layer_cfg.info.fb.addr[0] =
        (int)(set_cfg->layer_cfg.info.fb.addr[0] +
          fb_width * fb_height / 3 * 0);
    set_cfg->layer_cfg.info.fb.addr[1] =
        (int)(set_cfg->layer_cfg.info.fb.addr[0] +
          fb_width * fb_height / 3 * 1);
    set_cfg->layer_cfg.info.fb.addr[2] =
        (int)(set_cfg->layer_cfg.info.fb.addr[0] +
          fb_width * fb_height / 3 * 2);
    set_cfg->layer_cfg.info.fb.trd_right_addr[0] =
        (int)(set_cfg->layer_cfg.info.fb.addr[0] +
          fb_width * fb_height * 3 / 2);
    set_cfg->layer_cfg.info.fb.trd_right_addr[1] =
        (int)(set_cfg->layer_cfg.info.fb.trd_right_addr[0] +
          fb_width * fb_height);
    set_cfg->layer_cfg.info.fb.trd_right_addr[2] =
        (int)(set_cfg->layer_cfg.info.fb.trd_right_addr[0] +
          fb_width * fb_height * 3 / 2);

    set_cfg->layer_cfg.info.fb.size[1].width =
        set_cfg->layer_cfg.info.fb.size[0].width;
    set_cfg->layer_cfg.info.fb.size[1].height =
        set_cfg->layer_cfg.info.fb.size[0].height;
    set_cfg->layer_cfg.info.fb.size[2].width =
        set_cfg->layer_cfg.info.fb.size[0].width;
    set_cfg->layer_cfg.info.fb.size[2].height =
        set_cfg->layer_cfg.info.fb.size[0].height;


    if ((0 == set_cfg->layer_cfg.info.screen_win.width) ||
        (0 == set_cfg->layer_cfg.info.screen_win.height)) {
        if (1 == set_cfg->fullscreen) {
            /* full-screen display */
            printf("full screen display\n");
            set_cfg->layer_cfg.info.screen_win.width =
                width;
            set_cfg->layer_cfg.info.screen_win.height =
                height;
        } else {
            /* origin size display */
            /* cut out-of-screen part */
            if (set_cfg->layer_cfg.info.fb.crop.width > width)
                set_cfg->layer_cfg.info.fb.crop.width = width;
            if (set_cfg->layer_cfg.info.fb.crop.height > height)
                set_cfg->layer_cfg.info.fb.crop.height = height;

            set_cfg->layer_cfg.info.screen_win.width =
                (unsigned int)
                set_cfg->layer_cfg.info.fb.crop.width;
            set_cfg->layer_cfg.info.screen_win.height =
                (unsigned int)
                set_cfg->layer_cfg.info.fb.crop.height;
        }
    }
    set_cfg->layer_cfg.info.fb.crop.x = set_cfg->layer_cfg.info.fb.crop.x
                         << 32;
    set_cfg->layer_cfg.info.fb.crop.y = set_cfg->layer_cfg.info.fb.crop.y
                         << 32;
    set_cfg->layer_cfg.info.fb.crop.width =
        set_cfg->layer_cfg.info.fb.crop.width << 32;
    set_cfg->layer_cfg.info.fb.crop.height =
        set_cfg->layer_cfg.info.fb.crop.height << 32;

    set_cfg->layer_cfg.enable = 1;
    arg[0] = set_cfg->screen_id;
    arg[1] = (unsigned long)&set_cfg->layer_cfg;
    arg[2] = 1;
    arg[3] = 0;
    ret = disp_ioctl(DISP_LAYER_SET_CONFIG, (void *)arg);
    if (0 != ret)
        printf("fail to set layer cfg %d\n",ret);
    disp_release();
    return 0;
}

#endif
