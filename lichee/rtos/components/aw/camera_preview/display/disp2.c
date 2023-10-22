#include "disp2.h"
#ifdef CONFIG_DISP2_SUNXI

#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <video/sunxi_display2.h>
#include "disp_cfg_layer.h"

static u32 g_screen_index = 0;
static u32 channal_id = 0;
static u32 layer_id = 0;

extern int disp_ioctl(int cmd, void *arg);
extern int disp_release(void);
extern int disp_open(void);

static int disp2_set_layer(struct set_layer_cfg *cfg, struct camera_preview_buf dst_buf,
		uint32_t yoffset, uint32_t bits_per_pixel, uint32_t line_length)
{
	if (!cfg) {
		printf("Error: failed to get set_layer_cfg");
		return -1;
	}
    cfg->screen_id = g_screen_index;
    cfg->bpp = bits_per_pixel;
    cfg->layer_cfg.channel = channal_id;
    cfg->layer_cfg.layer_id = layer_id;
    cfg->layer_cfg.info.fb.addr[0] = (int) dst_buf.addr;
    cfg->layer_cfg.info.fb.size[0].width = dst_buf.width;
    cfg->layer_cfg.info.fb.size[0].height = dst_buf.height;
    cfg->layer_cfg.info.fb.crop.x = 0;
    cfg->layer_cfg.info.fb.crop.y = yoffset;
    cfg->layer_cfg.info.fb.crop.width = dst_buf.width;
    cfg->layer_cfg.info.fb.crop.height = dst_buf.height;
    cfg->layer_cfg.info.screen_win.x = 0;
    cfg->layer_cfg.info.screen_win.y = 0;
    cfg->layer_cfg.info.fb.flags = DISP_BF_NORMAL;
    cfg->layer_cfg.info.mode = LAYER_MODE_BUFFER;
    cfg->layer_cfg.info.alpha_mode = 0;
    cfg->layer_cfg.info.alpha_value = 0xff;
    cfg->layer_cfg.info.zorder = 16;

    hal_dcache_clean((unsigned long)(dst_buf.addr + yoffset * line_length), dst_buf.size);

    return cp_disp_cfg_layer(g_screen_index, cfg);

}

void cp_disp2_get_sizes(uint32_t *width, uint32_t *height) {
    unsigned long arg[6];

    disp_open();
    arg[0] = g_screen_index;
    *width = disp_ioctl(DISP_GET_SCN_WIDTH, (void *)arg);
    *height = disp_ioctl(DISP_GET_SCN_HEIGHT, (void *)arg);
    disp_release();
}

int cp_disp2_pan_display(struct camera_preview_buf dst_buf, int fbindex)
{
    struct set_layer_cfg cfg;
    unsigned long arg[6];
    uint32_t yoffset, bits_per_pixel, line_length;

	yoffset = dst_buf.height * fbindex;
	line_length = dst_buf.size / dst_buf.height;
	bits_per_pixel = line_length / dst_buf.width * 8;

    memset(&cfg, 0, sizeof(struct set_layer_cfg));
    disp2_set_layer(&cfg, dst_buf, yoffset, bits_per_pixel, line_length);

	//wait for vsync
    disp_open();
    arg[0] = g_screen_index;
    disp_ioctl(DISP_WAIT_VSYNC, (void *)arg);
    disp_release();

    return 0;
}

#endif
