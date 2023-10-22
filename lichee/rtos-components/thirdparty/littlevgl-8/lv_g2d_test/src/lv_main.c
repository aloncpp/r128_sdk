#include "lvgl/lvgl.h"
#include "lv_drivers/display/sunxifb.h"
#include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <console.h>

static lv_style_t rect_style;
static lv_obj_t *rect_obj;
static lv_obj_t *canvas;

static int random_in_range(int low, int high) {
    return low + random() % (high - low + 1);
}

static void ofs_x_anim(void *img, int32_t v) {
    lv_img_set_offset_x(img, v);
}

static void ofs_y_anim(void *img, int32_t v) {
    lv_img_set_offset_y(img, v);
}

static void set_zoom_anim(void *img, int32_t v) {
    lv_img_set_zoom(img, v);
}

static void my_timer(lv_timer_t *timer) {
    if (rect_obj) {
        lv_obj_set_pos(rect_obj,
                random_in_range(0,
                        lv_disp_get_hor_res(lv_disp_get_default())
                                - lv_obj_get_width(rect_obj)),
                random_in_range(0,
                        lv_disp_get_ver_res(lv_disp_get_default())
                                - lv_obj_get_height(rect_obj)));
        lv_style_set_bg_color(&rect_style,
                lv_color_make(random_in_range(0, 255), random_in_range(0, 255),
                        random_in_range(0, 255)));
        lv_obj_invalidate(rect_obj);
    }

    if (canvas) {
        lv_obj_set_pos(canvas,
                random_in_range(0,
                        lv_disp_get_hor_res(lv_disp_get_default())
                                - lv_obj_get_self_width(canvas)),
                random_in_range(0,
                        lv_disp_get_ver_res(lv_disp_get_default())
                                - lv_obj_get_self_height(canvas)));
        lv_obj_invalidate(canvas);
    }
}

static void lv_g2d_test_gif(char *path) {
    static lv_obj_t *gif_img;
    gif_img = lv_gif_create(lv_scr_act());
    lv_gif_set_src(gif_img, path);
    lv_obj_center(gif_img);
}

static void lv_g2d_test_rect(void) {
    lv_style_init(&rect_style);
    lv_style_set_bg_color(&rect_style, lv_color_hex(0x000000));
    lv_style_set_bg_opa(&rect_style, LV_OPA_50);
    lv_style_set_border_width(&rect_style, 0);
    lv_style_set_radius(&rect_style, 0);

    rect_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(rect_obj, lv_disp_get_hor_res(lv_disp_get_default()) * 0.4,
            lv_disp_get_ver_res(lv_disp_get_default()) * 0.4);
    lv_obj_add_style(rect_obj, &rect_style, 0);
    lv_obj_align(rect_obj, LV_ALIGN_TOP_LEFT, 0, 0);
}

static void lv_g2d_test_bmp(char *path) {
    static lv_obj_t *bmp_img;
    bmp_img = lv_img_create(lv_scr_act());
    lv_img_set_src(bmp_img, path);
    lv_obj_center(bmp_img);

    if (strcmp(lv_img_get_src(bmp_img), "A:/data/lv_g2d_test/no.bmp")
            != 0) {
        static lv_anim_t bmp_anim;
        lv_anim_init(&bmp_anim);
        lv_anim_set_var(&bmp_anim, bmp_img);
        lv_anim_set_time(&bmp_anim, 3000);
        lv_anim_set_playback_time(&bmp_anim, 500);
        lv_anim_set_repeat_count(&bmp_anim, LV_ANIM_REPEAT_INFINITE);

        lv_anim_set_exec_cb(&bmp_anim, ofs_x_anim);
        lv_anim_set_values(&bmp_anim, 0, lv_obj_get_self_width(bmp_img));
        lv_anim_start(&bmp_anim);

        lv_anim_set_exec_cb(&bmp_anim, ofs_y_anim);
        lv_anim_set_values(&bmp_anim, 0, lv_obj_get_self_height(bmp_img));
        lv_anim_start(&bmp_anim);
    }
}

static void lv_g2d_test_png(char *path) {
    static lv_obj_t *png_img;
    png_img = lv_img_create(lv_scr_act());
    lv_img_set_src(png_img, path);
    lv_obj_center(png_img);

    if (strcmp(lv_img_get_src(png_img), "A:/data/lv_g2d_test/no.png")
            != 0) {
        static lv_anim_t png_anim;
        lv_anim_init(&png_anim);
        lv_anim_set_var(&png_anim, png_img);
        lv_anim_set_time(&png_anim, 3000);
        lv_anim_set_playback_time(&png_anim, 500);
        lv_anim_set_repeat_count(&png_anim, LV_ANIM_REPEAT_INFINITE);

        lv_anim_set_exec_cb(&png_anim, set_zoom_anim);
        lv_anim_set_values(&png_anim, 64, 1024);
        lv_anim_start(&png_anim);
    }
}

static void lv_g2d_test_canvas(void) {
    static lv_color_t *cbuf;
    cbuf = (lv_color_t*) sunxifb_alloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(200, 300),
            "lv_g2d_test_canvas");

    canvas = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(canvas, cbuf, 200, 300,
            LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED);
    lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_draw_img_dsc_t img_dst;
    lv_draw_img_dsc_init(&img_dst);
    lv_canvas_draw_img(canvas, 0, 0, "A:/data/lv_g2d_test/200x300.png",
            &img_dst);

    /*sunxifb_free((void**) &cbuf, "lv_g2d_test_canvas");*/

    /*Test the rotation. It requires an other buffer where the orignal image is stored.
     *So copy the current image to buffer and rotate it to the canvas*/
#if 0
    static lv_color_t *cbuf_tmp;
    cbuf_tmp = (lv_color_t*) sunxifb_alloc(
            LV_CANVAS_BUF_SIZE_TRUE_COLOR(200, 300), "lv_g2d_test_canvas");
    memcpy(cbuf_tmp, cbuf, LV_CANVAS_BUF_SIZE_TRUE_COLOR(200, 300));
    lv_img_dsc_t img;
    img.data = (void*) cbuf_tmp;
    img.header.cf = LV_IMG_CF_TRUE_COLOR;
    img.header.w = 200;
    img.header.h = 300;

    lv_canvas_transform(canvas, &img, 0, 512, 0, 0, 0, 0, true);
#endif
}

int lvgl_main(int argc, char *argv[]) {
    lv_disp_drv_t disp_drv;
    lv_disp_draw_buf_t disp_buf;
    lv_indev_drv_t indev_drv;
    uint32_t rotated = LV_DISP_ROT_NONE;
    uint32_t gif_index = 5, bmp_index = 0, png_index = 1;
    char *gif_path[] = { "A:/data/lv_g2d_test/240x320.gif",
            "A:/data/lv_g2d_test/313x235.gif",
            "A:/data/lv_g2d_test/320x480.gif",
            "A:/data/lv_g2d_test/409x409.gif",
            "A:/data/lv_g2d_test/658x494.gif",
            "A:/data/lv_g2d_test/800x480.gif",
            "A:/data/lv_g2d_test/800x600.gif",
            "A:/data/lv_g2d_test/800x1280.gif",
            "A:/data/lv_g2d_test/1280x720.gif",
            "A:/data/lv_g2d_test/1280x800.gif",
            "A:/data/lv_g2d_test/1920x1080.gif",
            "A:/data/lv_g2d_test/no.gif" };
    char *bmp_path[] = { "A:/data/lv_g2d_test/512x352.bmp",
            "A:/data/lv_g2d_test/800x480.bmp",
            "A:/data/lv_g2d_test/no.bmp" };
    char *png_path[] = { "A:/data/lv_g2d_test/47x48.png",
            "A:/data/lv_g2d_test/225x111.png",
            "A:/data/lv_g2d_test/241x148.png",
            "A:/data/lv_g2d_test/no.png"};

    printf("lv_g2d_test 0 5 0 1\n");
    printf("one num is rotate, range is 0~3\n");
    printf("tow num is gif, range is 0~11, 11 is no show gif\n");
    printf("three num is bmp, range is 0~2, 2 is no show bmp\n");
    printf("four num is png, range is 0~3, 3 is no show png\n");

    lv_disp_drv_init(&disp_drv);

    if (argc >= 2 && atoi(argv[1]) >= 0 && atoi(argv[1]) <= 4) {
        rotated = atoi(argv[1]);
#ifndef USE_SUNXIFB_G2D_ROTATE
        if (rotated != LV_DISP_ROT_NONE)
            disp_drv.sw_rotate = 1;
#endif
    }
    if (argc >= 3 && atoi(argv[2]) >= 0 && atoi(argv[2]) <= 11)
        gif_index = atoi(argv[2]);
    if (argc >= 4 && atoi(argv[3]) >= 0 && atoi(argv[3]) <= 2)
        bmp_index = atoi(argv[3]);
    if (argc >= 5 && atoi(argv[4]) >= 0 && atoi(argv[4]) <= 3)
        png_index = atoi(argv[4]);

    /*LittlevGL init*/
    lv_init();

    /*Linux frame buffer device init*/
    sunxifb_init(rotated);

    /*A buffer for LittlevGL to draw the screen's content*/
    static uint32_t width, height;
    sunxifb_get_sizes(&width, &height);

    static lv_color_t *buf;
    buf = (lv_color_t*) sunxifb_alloc(width * height * sizeof(lv_color_t),
            "lv_g2d_test");

    if (buf == NULL) {
        sunxifb_exit();
        printf("malloc draw buffer fail\n");
        return 0;
    }

    /*Initialize a descriptor for the buffer*/
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, width * height);

    /*Initialize and register a display driver*/
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = sunxifb_flush;
    disp_drv.hor_res = width;
    disp_drv.ver_res = height;
    disp_drv.rotated = rotated;
    disp_drv.screen_transp = 0;
    lv_disp_drv_register(&disp_drv);

#if USE_EVDEV
    evdev_init();
    lv_indev_drv_init(&indev_drv); /*Basic initialization*/
    indev_drv.type = LV_INDEV_TYPE_POINTER; /*See below.*/
    indev_drv.read_cb = evdev_read; /*See below.*/
    /*Register the driver in LVGL and save the created input device object*/
    lv_indev_t *evdev_indev = lv_indev_drv_register(&indev_drv);
#endif

    lv_g2d_test_gif(gif_path[gif_index]);

    lv_g2d_test_bmp(bmp_path[bmp_index]);

    lv_g2d_test_rect();

    lv_g2d_test_png(png_path[png_index]);

    lv_g2d_test_canvas();

    lv_timer_create(my_timer, 500, NULL);

    /*Handle LitlevGL tasks (tickless mode)*/
    while (1) {
        lv_task_handler();
        usleep(1000);
    }

    /*lv_timer_del(timer);*/
    /*sunxifb_free((void**) &buf, "lv_g2d_test");*/
    /*sunxifb_exit();*/
    return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void) {
    static uint64_t start_ms = 0;
    if (start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = ((uint64_t) tv_start.tv_sec * 1000000
                + (uint64_t) tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = ((uint64_t) tv_now.tv_sec * 1000000 + (uint64_t) tv_now.tv_usec)
            / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}

FINSH_FUNCTION_EXPORT_CMD(lvgl_main, lv_g2d_test, lvgl_g2d_test);
