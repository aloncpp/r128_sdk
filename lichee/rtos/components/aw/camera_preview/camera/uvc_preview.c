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

#if defined(CONFIG_USB_CAMERA)
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "uvcvideo.h"

/*********************
 *      DEFINES
 *********************/

/* Color Depth: 16 (RGB565), 24 (RGB888), 32 (ARGB8888) */
#define LCD_COLOR_DEPTH 32

/* The Resolution of Camera Acquisition */
#define UVC_WIDTH  320
#define UVC_HEIGHT 240

/* UVC Format: 0 (YUYV), 1 (MJPEG) */
#define UVC_FMT_MJPEG  1

/* Preview Size: 0 (Non-full Screen), 1 (Full Screen) */
#define FULL_SCREEN_PREVIEW 1

/* The Resolution of Non-full Screen Display */
#define PREVIEW_WIDTH  320
#define PREVIEW_HEIGHT 240

#ifdef CONFIG_DRIVERS_SPILCD
#define FB_NUM 1
#else
#define FB_NUM 2
#endif

/**********************
 *  STATIC VARIABLES
 **********************/
static struct camera_preview_buf src_buf;
static struct camera_preview_buf dst_buf;
static int fbindex = 0;

static void *uvc_thread = NULL;
static void *file_thread = NULL;
static hal_mailbox_t uvc_mailbox = NULL;

static int uvc_preview_init(void)
{
    memset(&src_buf, 0, sizeof(struct camera_preview_buf));
    memset(&dst_buf, 0, sizeof(struct camera_preview_buf));

	src_buf.width  = UVC_WIDTH;
	src_buf.height = UVC_HEIGHT;

	if (UVC_FMT_MJPEG)
		src_buf.fmt    = CP_FMT_JPEG;
	else
		src_buf.fmt    = CP_FMT_YUYV;
	printf("[%s]: uvc_width = %d, uvc_height = %d\n", __func__,
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
	memset((void *)dst_buf.addr, 0, dst_buf.size);
	hal_dcache_clean((unsigned long)dst_buf.addr, dst_buf.size * FB_NUM);

	printf("[%s]: lcd_width = %d, lcd_height = %d, lcd color depth is %d\n", __func__,
			dst_buf.width, dst_buf.height, LCD_COLOR_DEPTH);
	printf("[%s]: disp addr = 0x%08x, disp size = %d*%d\n", __func__, dst_buf.addr, dst_buf.size, FB_NUM);

	return 0;
}

static void uvc_preview_to_lcd(void *uvc_buf, unsigned long uvc_size)
{
	src_buf.addr = uvc_buf;
	src_buf.size = uvc_size;
	cp_dbg("[%s]: uvc addr = 0x%08x uvc size = %d\n", __func__, src_buf.addr, src_buf.size);

	format_convert(src_buf, dst_buf, fbindex);

	display_pan_display(dst_buf, fbindex);

	if (FB_NUM == 2)
		fbindex = !fbindex;
}

void usb_uvc_disaply_thread(void *para)
{
    char source_data_path[64];
    unsigned int value = 0;
    struct v4l2_buffer *mailbuf;
    int np = 0;
    while(1) {
        hal_mailbox_recv(uvc_mailbox, &value, -1);
        if (value != 0) {
            mailbuf = (struct v4l2_buffer *)(uintptr_t)value;
            uvc_preview_to_lcd((uint32_t *)((int64_t)mailbuf->mem_buf), mailbuf->length);
            free((void *)((int64_t)mailbuf->mem_buf));
            free(mailbuf);
            mailbuf = NULL;
            value = 0;
        }
    }
}

void uvc_preview_test_thread(void *para)
{
	int fd;
	struct v4l2_capability cap;      /* Query device capabilities */
	struct v4l2_streamparm parms;    /* set streaming parameters */
	struct v4l2_format fmt;          /* try a format */
	struct v4l2_requestbuffers req;  /* Initiate Memory Mapping or User Pointer I/O */
	struct v4l2_buffer buf;          /* Query the status of a buffer */
    struct v4l2_buffer *mailbuf = NULL;
	enum v4l2_buf_type type;
	int n_buffers;
	// char source_data_path[64];
	// int np = 0;

	if (uvc_preview_init()) {
        printf("uvc preview init failed\n");
		return;
	}

	/* 1.open /dev/videoX node */
	fd = open("/dev/video", O_RDWR);

	/* 2.Query device capabilities */
	memset(&cap, 0, sizeof(cap));
	if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
		printf(" Query device capabilities fail!!!\n");
	} else {
		printf(" Querey device capabilities succeed\n");
		printf(" cap.driver=%s\n", cap.driver);
		printf(" cap.card=%s\n", cap.card);
		printf(" cap.bus_info=%s\n", cap.bus_info);
		printf(" cap.version=0x%08x\n", cap.version);
		printf(" cap.capabilities=0x%08x\n", cap.capabilities);
	}

	/* 7.set streaming parameters */
	memset(&parms, 0, sizeof(struct v4l2_streamparm));
	parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parms.parm.capture.timeperframe.numerator = 1;
	parms.parm.capture.timeperframe.denominator = 30;
	if (ioctl(fd, VIDIOC_S_PARM, &parms) < 0) {
		printf(" Setting streaming parameters failed, numerator:%d denominator:%d\n",
			   parms.parm.capture.timeperframe.numerator,
			   parms.parm.capture.timeperframe.denominator);
		close(fd);
		return;
	}

	/* 9.set the data format */
	memset(&fmt, 0, sizeof(struct v4l2_format));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = UVC_WIDTH;
	fmt.fmt.pix.height = UVC_HEIGHT;
	if (UVC_FMT_MJPEG)
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	else
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
		printf(" 123setting the data format failed!\n");
		close(fd);
		return;
	}

	/* 10.Initiate Memory Mapping or User Pointer I/O */
	memset(&req, 0, sizeof(struct v4l2_requestbuffers));
	req.count = 5;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
		printf(" VIDIOC_REQBUFS failed\n");
		close(fd);
		return;
	}

	/* 11.Exchange a buffer with the driver */
	for (n_buffers = 0; n_buffers < req.count; n_buffers++) {
		memset(&buf, 0, sizeof(struct v4l2_buffer));

		buf.index = n_buffers;
		if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
			printf(" VIDIOC_QBUF error\n");

			close(fd);
			return;
		}
	}

	/* streamon */
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
		printf(" VIDIOC_STREAMON error! %s\n", strerror(errno));
	} else
		printf(" stream on succeed\n");

	// np = 0;
//	while (np < 5) {
	while (1) {
		// printf(" camera capture num is [%d]\n", np);

		/* wait uvc frame */
		memset(&buf, 0, sizeof(struct v4l2_buffer));

		if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
			printf(" VIDIOC_DQBUF error\n");

			goto EXIT;
		} else {
            // printf("*****DQBUF[%d] FINISH*****\n", buf.index);
        }
#if 1
        mailbuf = malloc(sizeof(struct v4l2_buffer));
        mailbuf->mem_buf = (uint32_t)(uintptr_t)malloc(buf.length);
        if (mailbuf->mem_buf != 0) {
            memcpy((uint32_t *)((uint64_t)mailbuf->mem_buf), (uint32_t *)((uint64_t)buf.mem_buf), buf.length);
            mailbuf->length = buf.length;
            if (hal_mailbox_send_wait(uvc_mailbox, (uint32_t)(uintptr_t)mailbuf, 100) < 0) {
                printf("uvc data send failed, data lost\n");
            }
        }
#else
		// uvc_preview_to_lcd((void *)buf.mem_buf, buf.length);
#endif

		if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
			printf(" VIDIOC_QBUF error\n");

			goto EXIT;
		} else {
            // printf("************QBUF[%d] FINISH**************\n\n", buf.index);
        }
	}

	printf("\n\n Capture thread finish\n");

EXIT:
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(fd, VIDIOC_STREAMOFF, &type);

	memset(&req, 0, sizeof(struct v4l2_requestbuffers));
	req.count = 0;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	ioctl(fd, VIDIOC_REQBUFS, &req);
	hal_free_coherent(dst_buf.addr);

	close(fd);

	return;
}

int uvc_preview_test(int argc, const char **argv)
{
    uvc_mailbox = hal_mailbox_create("ucv_queue", 20);
    if (uvc_mailbox == NULL) {
        printf("mailbox create failed\n");
        goto fail_exit1;
    }
    printf("uvc_mailbox create sucess!\n");
    // usb thread is HAL_THREAD_PRIORITY_SYS,must be lower than HAL_THREAD_PRIORITY_SYS
    uvc_thread = hal_thread_create(uvc_preview_test_thread, NULL, "uvc_thread", 16 * 1024,
                                   (HAL_THREAD_PRIORITY_APP + 1));
    if (uvc_thread == NULL) {
        printf("usb_uvc_test_thread create failed\n");
        goto fail_exit2;
    }
    hal_thread_start(uvc_thread);

    file_thread = hal_thread_create(usb_uvc_disaply_thread, NULL, "uvc_display_thread", 4 * 1024,
                                   (HAL_THREAD_PRIORITY_APP));
    if (file_thread == NULL) {
        printf("uvc file thread create failed\n");
        goto fail_exit3;
    }
    hal_thread_start(file_thread);

    return 0;
fail_exit3:
    hal_thread_stop(uvc_thread);
fail_exit2:
    hal_mailbox_delete(uvc_mailbox);
fail_exit1:
    return -1;
}

#endif
