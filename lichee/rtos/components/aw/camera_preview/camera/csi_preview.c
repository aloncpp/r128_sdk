/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *	1. Redistributions of source code must retain the above copyright
 *	   notice, this list of conditions and the following disclaimer.
 *	2. Redistributions in binary form must reproduce the above copyright
 *	   notice, this list of conditions and the following disclaimer in the
 *	   documentation and/or other materials provided with the
 *	   distribution.
 *	3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *	   its contributors may be used to endorse or promote products derived
 *	   from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "camera_cmd.h"

#if defined(CONFIG_CSI_CAMERA)
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <hal_mem.h>
#include <time.h>
#include <sys/time.h>
#include "csi/hal_csi_jpeg.h"
#include "jpegenc.h"

/*********************
 *      DEFINES
 *********************/

/* Color Depth: 16 (RGB565), 24 (RGB888), 32 (ARGB8888) */
#define LCD_COLOR_DEPTH 32

/* The Resolution of Camera Acquisition */
#define CSI_WIDTH  240
#define CSI_HEIGHT 320

/* Preview Size: 0 (Non-full Screen), 1 (Full Screen) */
#define FULL_SCREEN_PREVIEW 1

/* The Resolution of Non-full Screen Display */
#define PREVIEW_WIDTH  240
#define PREVIEW_HEIGHT 320

#ifdef CONFIG_DRIVERS_SPILCD
#define FB_NUM 1
#else
#define FB_NUM 2
#endif

#ifndef configAPPLICATION_NORMAL_PRIORITY
#define configAPPLICATION_NORMAL_PRIORITY (15)
#endif

/**********************
 *  STATIC VARIABLES
 **********************/
static int exit_flag = 0;
static struct camera_preview_buf src_buf;
static struct camera_preview_buf dst_buf;
static int fbindex = 0;

static int read_frame(void *addr, int size, unsigned int order)
{
	FILE* fd;
	long long res;
	char name[128];

	cp_dbg("%s line: %d addr = 0x%08x size = %d\n", __func__, __LINE__, addr, size);

	sprintf(name, "/data/csi_preview_%d.bin", order);
	fd = fopen(name, "ab");
	if (fd < 0) {
		printf("open file error %d\n", fd);
		return -1;
	}

	res = fwrite(addr, size, 1, fd);
	if (res < 0) {
		printf("write fail(%d), line%d..\n", res, __LINE__);
		fclose(fd);
		return -1;
	}
	cp_dbg("write image ok\n");

	fclose(fd);

	return 0;
}

static int csi_preview_init(void)
{
    memset(&src_buf, 0, sizeof(struct camera_preview_buf));
    memset(&dst_buf, 0, sizeof(struct camera_preview_buf));

	src_buf.width  = CSI_WIDTH;
	src_buf.height = CSI_HEIGHT;
	src_buf.fmt    = CP_FMT_NV12;
	printf("[%s]: csi_width = %d, csi_height = %d, csi format is NV12\n", __func__,
			src_buf.width, src_buf.height);

	if (FULL_SCREEN_PREVIEW)
		display_get_sizes(&(dst_buf.width), &(dst_buf.height));
	else {
		dst_buf.width  = PREVIEW_WIDTH;
		dst_buf.height = PREVIEW_HEIGHT;
	}
	if (LCD_COLOR_DEPTH == 16)
		dst_buf.fmt  = CP_FMT_RGB565;
	else if (LCD_COLOR_DEPTH == 24)
		dst_buf.fmt  = CP_FMT_RGB888;
	else
		dst_buf.fmt  = CP_FMT_ARGB8888;
	dst_buf.size = dst_buf.width * dst_buf.height * LCD_COLOR_DEPTH / 8;
	dst_buf.addr = hal_malloc_coherent(dst_buf.size * FB_NUM);
	if (dst_buf.addr == NULL) {
		printf("[%s,%d]: malloc dst_buf size :%d failed!\n", __func__, __LINE__, dst_buf.size * FB_NUM);
		return -1;
	}
	memset((void *)dst_buf.addr, 0, dst_buf.size * FB_NUM);

	printf("[%s]: lcd_width = %d, lcd_height = %d, lcd color depth is %d\n", __func__,
			dst_buf.width, dst_buf.height, LCD_COLOR_DEPTH);
	printf("[%s]: disp addr = 0x%08x, disp size = %d*%d\n", __func__, dst_buf.addr, dst_buf.size, FB_NUM);

	return 0;
}

static void lcd_show_info(uint32_t fps, int fbindex)
{
	struct ft f;
	char *str;
	int x, y;
	char b1[20], b2[20];
	sprintf(b1, "fps:%d", fps);
	sprintf(b2, "rate:%dx%d", dst_buf.width, dst_buf.height);

	f.line_width = dst_buf.size / dst_buf.height;
	f.bits_per_pixel = LCD_COLOR_DEPTH;
	f.pixel_width = f.bits_per_pixel / 8;
	f.screen_size = dst_buf.size;
	f.fbmem = dst_buf.addr + f.screen_size * fbindex;

	print_ascii(&f, b1, 0, 0);
	print_ascii(&f, b2, 0, 26);
}

static void csi_preview_to_lcd(struct csi_jpeg_mem *csi_mem, uint32_t fps)
{
	src_buf.addr = csi_mem->buf.addr;
	src_buf.size = csi_mem->buf.size;
	cp_dbg("[%s]: csi addr = 0x%08x csi size = %d\n", __func__, src_buf.addr, src_buf.size);

	format_convert(src_buf, dst_buf, fbindex);

	lcd_show_info(fps, fbindex);

	display_pan_display(dst_buf, fbindex);

	if (FB_NUM == 2)
		fbindex = !fbindex;
}

static void csi_thread_entry(void * param)
{
	struct csi_jpeg_mem *csi_mem;
	unsigned int timeout_msec = 2000;
	static struct timeval new, old;
	static uint32_t cur_fps;
	static uint32_t fps = 0;

	/*Handle csi_preview tasks (tickless mode)*/
	while(1) {
		gettimeofday(&new, NULL);
		if (new.tv_sec * 1000 - old.tv_sec * 1000 >= 1000) {
			printf("cur_fps = %d fps\n", cur_fps);
			fps = cur_fps;
			old = new;
			cur_fps = 0;
		} else {
			cur_fps++;
		}

		csi_mem = hal_csi_dqbuf(timeout_msec);
		if(exit_flag == 1)
		{
			exit_flag = 0;
			break;
		}
		if (csi_mem == NULL) {
			continue;
		}
		csi_preview_to_lcd(csi_mem, fps);
		hal_csi_qbuf();

	}
	vTaskDelete(NULL);
}

static int main_test()
{
	struct csi_jpeg_fmt fmt;
	unsigned int count;

	if (csi_preview_init()) {
		return -1;
	}

	memset(&fmt, 0, sizeof(struct csi_jpeg_fmt));
	fmt.width = CSI_WIDTH;
	fmt.height = CSI_HEIGHT;
	fmt.line_mode = OFFLINE_MODE;
	fmt.output_mode = PIX_FMT_OUT_NV12;
	hal_csi_jpeg_set_fmt(&fmt);

	count = 3;
	hal_csi_jpeg_reqbuf(count);
	hal_csi_jpeg_s_stream(1);
	printf("csi stream on!\n");

	portBASE_TYPE task_ret;
	TaskHandle_t lv_task;
	task_ret = xTaskCreate(csi_thread_entry, (signed portCHAR *) "csi_preview", 4096,
			NULL, configAPPLICATION_NORMAL_PRIORITY, &lv_task);
	if (task_ret != pdPASS) {
		printf("[%s,%d]: create csi_preview task err\n", __func__, __LINE__);
	}

	while(1)
	{
		char cRxed = 0;

		cRxed = getchar();
		if(cRxed == 'q' || cRxed == 3)
		{
			exit_flag = 1;
			hal_csi_jpeg_s_stream(0);
			hal_csi_jpeg_freebuf();
			hal_free_coherent(dst_buf.addr);
			printf("csi stream off!!\n");
			return 0;
		}
	}
}

int csi_preview_test(int argc, const char **argv)
{
	printf("camera CSI preview to lcd demo start!\n");
	hal_csi_jpeg_probe();

	if (main_test()) {
		printf("camera CSI preview to lcd demo fail!\n");
	}

	hal_csi_jpeg_remove();
	printf("camera CSI preview to lcd demo over!\n");

	return 0;
}

#endif
