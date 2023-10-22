/**
 * @file sunxifb.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "sunxifb.h"
#if USE_SUNXIFB

#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "sunximem.h"

#ifdef USE_DISP2
#include "disp2.h"
#include "disp_cfg_layer.h"
#elif defined USE_SPILCD
#include "spilcd.h"
#endif

#ifdef LV_USE_SUNXIFB_DEBUG
#include <sys/time.h>
#endif /* LV_USE_SUNXIFB_DEBUG */

#ifdef USE_SUNXIFB_G2D
#include "sunxig2d.h"
#endif /* USE_SUNXIFB_G2D */

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
struct sunxifb_var_screeninfo {
    uint32_t xres;
    uint32_t yres;
    uint32_t xoffset;
    uint32_t yoffset;
    uint32_t bits_per_pixel;
};
struct sunxifb_fix_screeninfo {
    char *smem_start;
    long int smem_len;
    uint32_t line_length;
};
static struct sunxifb_var_screeninfo vinfo;
static struct sunxifb_fix_screeninfo finfo;
static char *fbp = 0;
static uint32_t fbp_w;
static uint32_t fbp_h;
static uint32_t fbp_line_length;

#ifdef USE_SUNXIFB_DOUBLE_BUFFER
    static int fbnum = 2;
#else
    static int fbnum = 1;
#endif

#ifdef USE_SUNXIFB_DOUBLE_BUFFER
struct sunxifb_info {
    char *screenfbp[2];
    uint32_t fbnum;
    uint32_t fbindex;
    volatile bool dbuf_en;
#ifdef USE_SUNXIFB_G2D
#ifdef USE_SUNXIFB_G2D_ROTATE
    g2d_blt_flags_h rotated;
    char *rotatefbp;
    uintptr_t rotatefbp_phy;
    uint32_t rotatefbp_w;
    uint32_t rotatefbp_h;
#endif /* USE_SUNXIFB_G2D_ROTATE */
#endif /* USE_SUNXIFB_G2D */
};

static struct sunxifb_info sinfo;
#endif /* USE_SUNXIFB_DOUBLE_BUFFER */

#ifdef USE_SUNXIFB_G2D_ROTATE
static void sunxifb_soft_rotate(void);
#endif /* USE_SUNXIFB_G2D_ROTATE */

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void sunxifb_get_sizes(uint32_t *width, uint32_t *height) {
#ifdef USE_DISP2
    disp2_get_sizes(width, height);
#elif defined USE_SPILCD
    spilcd_get_sizes(width, height);
#endif
}

static void fb_init(void) {
    sunxifb_get_sizes(&(vinfo.xres), &(vinfo.yres));
    vinfo.bits_per_pixel = LV_COLOR_DEPTH;
    vinfo.xoffset = 0;
    vinfo.yoffset = 0;
    finfo.line_length = vinfo.xres * vinfo.bits_per_pixel / 8;
    finfo.smem_len = finfo.line_length * vinfo.yres * fbnum;
#ifdef USE_DISP2
    disp2_init(vinfo.bits_per_pixel, finfo.smem_len, &(finfo.smem_start));
#elif defined USE_SPILCD
    spilcd_init(vinfo.bits_per_pixel, finfo.smem_len, &(finfo.smem_start));
#endif
}

void sunxifb_init(uint32_t rotated) {
    long int screensize = 0;
    fb_init();
    fbp_w = vinfo.xres;
    fbp_h = vinfo.yres;
    screensize =  finfo.smem_len; //finfo.line_length * vinfo.yres;
    fbp_line_length = finfo.line_length;

    fbp = finfo.smem_start;
    if ((intptr_t) fbp == -1) {
        perror("Error: failed to map framebuffer device to memory");
        return;
    }

#ifdef USE_SUNXIFB_DOUBLE_BUFFER
    memset(&sinfo, 0, sizeof(struct sunxifb_info));
    sinfo.dbuf_en = true;
    // Do not clear fb and pointer to back fb
    sinfo.fbnum = (uint32_t)fbnum;
    if (sinfo.fbnum > 1) {
        printf("Turn on double buffering.\n");

        sinfo.screenfbp[0] = fbp;
        sinfo.screenfbp[1] = fbp + finfo.line_length * vinfo.yres;

        if (vinfo.yoffset == 0) {
            sinfo.fbindex = !sinfo.fbindex;
            fbp = sinfo.screenfbp[sinfo.fbindex];
        }

#ifdef USE_SUNXIFB_G2D
        printf("Turn on 2d hardware acceleration.\n");

        sunxifb_g2d_init(vinfo.bits_per_pixel);

        sunxifb_mem_init();

#ifdef USE_SUNXIFB_G2D_ROTATE
        printf("Turn on 2d hardware acceleration rotate.\n");

        sinfo.rotatefbp = sunxifb_mem_alloc(finfo.line_length * vinfo.yres, "sunxifb_rotate");
        if (sinfo.rotatefbp == NULL) {
            perror("Error: cannot malloc rotate buffer");
            return;
        }

        fbp = sinfo.rotatefbp;
        sinfo.rotatefbp_phy = (uintptr_t) sunxifb_mem_get_phyaddr(
                sinfo.rotatefbp);

        switch (rotated) {
        case LV_DISP_ROT_90:
            sinfo.rotated = G2D_ROT_270;
            sinfo.rotatefbp_w = vinfo.yres;
            sinfo.rotatefbp_h = vinfo.xres;
            fbp_w = vinfo.yres;
            fbp_h = vinfo.xres;
            fbp_line_length = fbp_w * vinfo.bits_per_pixel / 8;
            break;
        case LV_DISP_ROT_180:
            sinfo.rotated = G2D_ROT_180;
            sinfo.rotatefbp_w = vinfo.xres;
            sinfo.rotatefbp_h = vinfo.yres;
            break;
        case LV_DISP_ROT_270:
            sinfo.rotated = G2D_ROT_90;
            sinfo.rotatefbp_w = vinfo.yres;
            sinfo.rotatefbp_h = vinfo.xres;
            fbp_w = vinfo.yres;
            fbp_h = vinfo.xres;
            fbp_line_length = fbp_w * vinfo.bits_per_pixel / 8;
            break;
        default:
            sinfo.rotated = G2D_ROT_0;
            sinfo.rotatefbp_w = vinfo.xres;
            sinfo.rotatefbp_h = vinfo.yres;
            break;
        }
#endif /* USE_SUNXIFB_G2D_ROTATE */
#endif /* USE_SUNXIFB_G2D */
    }
#else
    memset(fbp, 0, screensize);
#endif /* USE_SUNXIFB_DOUBLE_BUFFER */
}

void sunxifb_exit(void) {
#ifdef USE_DISP2
    disp2_exit();
#elif defined USE_SPILCD
    spilcd_exit();
#endif
#ifdef USE_SUNXIFB_DOUBLE_BUFFER
#ifdef USE_SUNXIFB_G2D
    sunxifb_g2d_deinit();

#ifdef USE_SUNXIFB_G2D_ROTATE
    sunxifb_mem_free((void **) &sinfo.rotatefbp, "sunxifb_rotate");
#endif /* USE_SUNXIFB_G2D_ROTATE */
    sunxifb_mem_deinit();
#endif /* USE_SUNXIFB_G2D */
#endif /* USE_SUNXIFB_DOUBLE_BUFFER */
}

/**
 * Flush a buffer to the marked area
 * @param drv pointer to driver where this function belongs
 * @param area an area where to copy `color_p`
 * @param color_p an array of pixel to copy to the `area` part of the screen
 */
void sunxifb_flush(lv_disp_drv_t *drv, const lv_area_t *area,
        lv_color_t *color_p) {
    if (fbp == NULL || area->x2 < 0 || area->y2 < 0
            || area->x1 > (int32_t) fbp_w - 1
            || area->y1 > (int32_t) fbp_h - 1) {
        lv_disp_flush_ready(drv);
        return;
    }

    /*Truncate the area to the screen*/
    int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
    int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
    int32_t act_x2 =
            area->x2 > (int32_t) fbp_w - 1 ? (int32_t) fbp_w - 1 : area->x2;
    int32_t act_y2 =
            area->y2 > (int32_t) fbp_h - 1 ? (int32_t) fbp_h - 1 : area->y2;

    lv_coord_t w = (act_x2 - act_x1 + 1);
    long int location = 0;
    long int byte_location = 0;
    unsigned char bit_location = 0;

    /*32 or 24 bit per pixel*/
    if (vinfo.bits_per_pixel == 32 || vinfo.bits_per_pixel == 24) {
        uint32_t *fbp32 = (uint32_t*) fbp;
        int32_t y;
        for (y = act_y1; y <= act_y2; y++) {
#ifdef USE_SUNXIFB_DOUBLE_BUFFER
            if (sinfo.fbnum > 1)
                location = act_x1 + y * fbp_line_length / 4;
            else
#endif /* USE_SUNXIFB_DOUBLE_BUFFER */
                location = (act_x1 + vinfo.xoffset)
                        + (y + vinfo.yoffset) * finfo.line_length / 4;
            memcpy(&fbp32[location], (uint32_t*) color_p,
                    (act_x2 - act_x1 + 1) * 4);
            color_p += w;
        }
    }
    /*16 bit per pixel*/
    else if (vinfo.bits_per_pixel == 16) {
        uint16_t *fbp16 = (uint16_t*) fbp;
        int32_t y;
        for (y = act_y1; y <= act_y2; y++) {
#ifdef USE_SUNXIFB_DOUBLE_BUFFER
            if (sinfo.fbnum > 1)
                location = act_x1 + y * fbp_line_length / 2;
            else
#endif /* USE_SUNXIFB_DOUBLE_BUFFER */
                location = (act_x1 + vinfo.xoffset)
                        + (y + vinfo.yoffset) * finfo.line_length / 2;
            memcpy(&fbp16[location], (uint32_t*) color_p,
                    (act_x2 - act_x1 + 1) * 2);
            color_p += w;
        }
    }
    /*8 bit per pixel*/
    else if (vinfo.bits_per_pixel == 8) {
        uint8_t *fbp8 = (uint8_t*) fbp;
        int32_t y;
        for (y = act_y1; y <= act_y2; y++) {
#ifdef USE_SUNXIFB_DOUBLE_BUFFER
            if (sinfo.fbnum > 1)
                location = act_x1 + y * fbp_line_length;
            else
#endif /* USE_SUNXIFB_DOUBLE_BUFFER */
                location = (act_x1 + vinfo.xoffset)
                        + (y + vinfo.yoffset) * finfo.line_length;
            memcpy(&fbp8[location], (uint32_t*) color_p, (act_x2 - act_x1 + 1));
            color_p += w;
        }
    }
    /*1 bit per pixel*/
    else if (vinfo.bits_per_pixel == 1) {
        uint8_t *fbp8 = (uint8_t*) fbp;
        int32_t x;
        int32_t y;
        for (y = act_y1; y <= act_y2; y++) {
            for (x = act_x1; x <= act_x2; x++) {
#ifdef USE_SUNXIFB_DOUBLE_BUFFER
                if (sinfo.fbnum > 1)
                    location = x + y * fbp_w;
                else
#endif /* USE_SUNXIFB_DOUBLE_BUFFER */
                    location = (x + vinfo.xoffset)
                            + (y + vinfo.yoffset) * vinfo.xres;
                byte_location = location / 8; /* find the byte we need to change */
                bit_location = location % 8; /* inside the byte found, find the bit we need to change */
                fbp8[byte_location] &= ~(((uint8_t)(1)) << bit_location);
                fbp8[byte_location] |= ((uint8_t)(color_p->full))
                        << bit_location;
                color_p++;
            }

            color_p += area->x2 - act_x2;
        }
    } else {
        /*Not supported bit per pixel*/
    }

    //May be some direct update command is required
    //ret = ioctl(state->fd, FBIO_UPDATE, (unsigned long)((uintptr_t)rect));

#ifdef USE_SUNXIFB_DOUBLE_BUFFER
    if (sinfo.fbnum > 1 && sinfo.dbuf_en && lv_disp_flush_is_last(drv)) {
#ifdef USE_SUNXIFB_G2D_ROTATE
        sunxifb_mem_flush_cache(sinfo.rotatefbp,
                finfo.line_length * vinfo.yres);
        if (sunxifb_g2d_blit_to_fb(sinfo.rotatefbp_phy, sinfo.rotatefbp_w,
                sinfo.rotatefbp_h, 0, 0, sinfo.rotatefbp_w,
                sinfo.rotatefbp_h, (uintptr_t)finfo.smem_start, vinfo.xres,
                vinfo.yres * fbnum, 0, sinfo.fbindex * vinfo.yres, vinfo.xres,
                vinfo.yres, sinfo.rotated) < 0) {
            sunxifb_soft_rotate();
        }
#endif /* USE_SUNXIFB_G2D_ROTATE */

        vinfo.yoffset = sinfo.fbindex * vinfo.yres;
#ifdef USE_DISP2
        disp2_pan_display(vinfo.yoffset);
#elif defined USE_SPILCD
        spilcd_pan_display(vinfo.yoffset);
#endif

#if defined(USE_SUNXIFB_G2D) && !defined(USE_SUNXIFB_G2D_ROTATE)
        if (sunxifb_g2d_blit_to_fb((uintptr_t)finfo.smem_start, vinfo.xres,
                vinfo.yres * fbnum, 0, sinfo.fbindex * vinfo.yres, vinfo.xres,
                vinfo.yres, (uintptr_t)finfo.smem_start, vinfo.xres,
                vinfo.yres * fbnum, 0, !sinfo.fbindex * vinfo.yres, vinfo.xres,
                vinfo.yres, G2D_ROT_0) < 0) {
            memcpy(sinfo.screenfbp[!sinfo.fbindex],
                    sinfo.screenfbp[sinfo.fbindex],
                    finfo.line_length * vinfo.yres);
        }
#elif !defined(USE_SUNXIFB_G2D_ROTATE)
        memcpy(sinfo.screenfbp[!sinfo.fbindex], sinfo.screenfbp[sinfo.fbindex],
                finfo.line_length * vinfo.yres);
#endif /* USE_SUNXIFB_G2D && !USE_SUNXIFB_G2D_ROTATE */

        sinfo.fbindex = !sinfo.fbindex;
#ifndef USE_SUNXIFB_G2D_ROTATE
        fbp = sinfo.screenfbp[sinfo.fbindex];
#endif /* USE_SUNXIFB_G2D_ROTATE */

#ifdef LV_USE_SUNXIFB_DEBUG
        static struct timeval new, old;
        static uint32_t cur_fps, avg_fps, max_fps, min_fps = 60, fps_cnt, first;
        gettimeofday(&new, NULL);
        if (new.tv_sec * 1000 - old.tv_sec * 1000 >= 1000) {
            if (first > 4) {
                if (first > 64) {
                    fps_cnt = 0;
                    avg_fps = 0;
                    max_fps = 0;
                    min_fps = 60;
                    first = 4;
                }
                fps_cnt++;
                avg_fps += cur_fps;
                if (max_fps < cur_fps)
                    max_fps = cur_fps;
                if (min_fps > cur_fps)
                    min_fps = cur_fps;
            }

            if (fps_cnt > 0)
                printf("sunxifb_flush fps_cnt=%u cur_fps=%u, max_fps=%u, "
                        "min_fps=%u, cur_page=%d, avg_fps=%.2f\n", fps_cnt,
                        cur_fps, max_fps, min_fps, sinfo.fbindex,
                        (float) avg_fps / (float) fps_cnt);

            first++;
            old = new;
            cur_fps = 0;
        } else {
            cur_fps++;
        }
#endif /* LV_USE_SUNXIFB_DEBUG */
    }
#endif /* USE_SUNXIFB_DOUBLE_BUFFER */

#if !defined(USE_SUNXIFB_DOUBLE_BUFFER) && defined(USE_SPILCD)
    if (fbnum = 1 && lv_disp_flush_is_last(drv)) {
        spilcd_pan_display(0);
    }
#endif

    lv_disp_flush_ready(drv);
}

void* sunxifb_alloc(size_t size, char *label) {
    return sunxifb_mem_alloc(size, label);
}

void sunxifb_free(void **data, char *label) {
    sunxifb_mem_free(data, label);
}

#ifdef USE_SUNXIFB_DOUBLE_BUFFER
bool sunxifb_get_dbuf_en() {
    return sinfo.dbuf_en;
}

int sunxifb_set_dbuf_en(lv_disp_drv_t *drv, bool dbuf_en) {
    if (sinfo.dbuf_en == dbuf_en)
        return 0;

    if (drv->draw_buf->flushing)
        return -2;

#ifdef USE_SUNXIFB_CACHE
    sinfo.screenfbp[0] = fbp;
    sinfo.screenfbp[1] = fbp + finfo.line_length * vinfo.yres;
#endif /* USE_SUNXIFB_CACHE */

    if (dbuf_en) {
        vinfo.yoffset = sinfo.fbindex * vinfo.yres;
#ifdef USE_DISP2
        disp2_pan_display(vinfo.yoffset);
#elif defined USE_SPILCD
        spilcd_pan_display(vinfo.yoffset);
#endif

#ifdef USE_SUNXIFB_G2D
        sunxifb_g2d_blit_to_fb((uintptr_t)finfo.smem_start, vinfo.xres,
                vinfo.yres * fbnum, 0, sinfo.fbindex * vinfo.yres, vinfo.xres,
                vinfo.yres, (uintptr_t)finfo.smem_start, vinfo.xres,
                vinfo.yres * fbnum, 0, !sinfo.fbindex * vinfo.yres, vinfo.xres,
                vinfo.yres, G2D_ROT_0);
#else
        memcpy(sinfo.screenfbp[!sinfo.fbindex], sinfo.screenfbp[sinfo.fbindex],
                finfo.line_length * vinfo.yres);
#endif /* USE_SUNXIFB_G2D */
    }

    sinfo.fbindex = !sinfo.fbindex;
#ifdef USE_SUNXIFB_G2D_ROTATE
    fbp = sinfo.rotatefbp;
#else
    fbp = sinfo.screenfbp[sinfo.fbindex];
#endif /* USE_SUNXIFB_G2D_ROTATE */

    sinfo.dbuf_en = dbuf_en;
    return 0;
}
#endif /* USE_SUNXIFB_DOUBLE_BUFFER */

/**********************
 *   STATIC FUNCTIONS
 **********************/
#ifdef USE_SUNXIFB_G2D_ROTATE
static void sunxifb_soft_rotate(void) {
    int i = 0, j = 0, k = 0, channel = vinfo.bits_per_pixel / 8, desW =
            vinfo.xres, desH = vinfo.yres, srcW = sinfo.rotatefbp_w, srcH =
            sinfo.rotatefbp_h;

    switch (sinfo.rotated) {
    case G2D_ROT_90:
        for (i = 0; i < desH; i++) {
            for (j = 0; j < desW; j++) {
                for (k = 0; k < channel; k++) {
                    sinfo.screenfbp[sinfo.fbindex][(i * desW + j) * channel + k] =
                            sinfo.rotatefbp[((srcH - 1 - j) * srcW + i)
                                    * channel + k];
                }
            }
        }
        break;
    case G2D_ROT_180:
        for (i = 0; i < desH; i++) {
            for (j = 0; j < desW; j++) {
                for (k = 0; k < channel; k++) {
                    sinfo.screenfbp[sinfo.fbindex][(i * desW + j) * channel + k] =
                            sinfo.rotatefbp[((srcH - 1 - i) * srcW + srcW - 1
                                    - j) * channel + k];
                }
            }
        }
        break;
    case G2D_ROT_270:
        for (i = 0; i < desH; i++) {
            for (j = 0; j < desW; j++) {
                for (k = 0; k < channel; k++) {
                    sinfo.screenfbp[sinfo.fbindex][(i * desW + j) * channel + k] =
                            sinfo.rotatefbp[(j * srcW + (srcW - i)) * channel + k];
                }
            }
        }
        break;
    default:
        memcpy(sinfo.screenfbp[sinfo.fbindex], sinfo.rotatefbp,
                finfo.line_length * vinfo.yres);
        break;
    }
}
#endif /* USE_SUNXIFB_G2D_ROTATE */

#endif
