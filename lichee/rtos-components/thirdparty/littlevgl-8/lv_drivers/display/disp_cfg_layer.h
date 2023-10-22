#ifndef DISP_CFG_LAYER_H
#define DISP_CFG_LAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <hal_mem.h>
#include <video/sunxi_display2.h>


struct set_layer_cfg
{
    int screen_id;
    int bpp;
    int fullscreen;//is full screen
    struct disp_layer_config layer_cfg;
};

int disp_clear_layer(u32 screen_index, u32 chn, u32 layer);
int disp_clear_all_layer(u32 screen_index);
int disp_cfg_layer(u32 g_screen_index, struct set_layer_cfg *set_cfg);

#ifdef __cplusplus
}
#endif

#endif  /*DISP_CFG_LAYER_H*/
