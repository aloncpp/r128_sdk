/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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

#include "wv_log.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "FreeRTOS/_os_time.h"
#include "sunxi_hal_twi.h"
#include <fcntl.h>
#include "csi/hal_csi_jpeg.h"
#include "jpegenc.h"
#include <hal_time.h>
#include "wv_communication.h"
#include <lwip/sockets.h>

#define CSI_JPEG_IRQn	109  // rv
static video_data *package = NULL;
int order = 0;
static TaskHandle_t jpeg_capture_thread;

static int send_jpg(struct csi_jpeg_mem *jpeg_mem)
{
	int ret = 0;
	void *addr;
	unsigned int size;
	int package_len = 0;

	addr = jpeg_mem->buf.addr - JPEG_HEADER_LEN;
	size = jpeg_mem->buf.size + JPEG_HEADER_LEN;

    if (tcp_connected == 1) {
		package_len = sizeof(video_data_header) + size;
		package = (video_data *) malloc(package_len);
		if (package == NULL) {
			LOG_E("malloc package fail");
			return -1;
		}
		memset(package, 0, package_len);
		memcpy(package->payload, addr, size);
		package->header.data_len = htonl(size);

		ret = send_package(client_sockfd, package, package_len);
		free(package);
	}

	return 0;
}

static void jpeg_process(void)
{
	int ret = 0;
	struct csi_jpeg_fmt fmt;
	unsigned int count = 3;
	struct csi_jpeg_mem *csi_mem;
	struct csi_jpeg_mem *jpeg_mem;
	unsigned int timeout_msec = 2000;
	static struct timeval new, old;
	static uint32_t cur_fps;
	static uint32_t fps = 0;

	fmt.width = 640;
	fmt.height = 480;
	fmt.line_mode = ONLINE_MODE;
	fmt.output_mode = PIX_FMT_OUT_MAX;
	fmt.cb = NULL;
	hal_csi_jpeg_set_fmt(&fmt);

	if (hal_csi_jpeg_reqbuf(count) != 0) {
		return ;
	}

	hal_csi_jpeg_s_stream(1);
	LOG_D("csi stream on!");

	while (1) {
		jpeg_mem = hal_jpeg_dqbuf(timeout_msec);
		if (jpeg_mem == NULL) {
			continue;
		}
		send_jpg(jpeg_mem);
		hal_jpeg_qbuf();
	}

	hal_csi_jpeg_s_stream(0);
	hal_csi_jpeg_freebuf();
	LOG_D("csi stream off!");
}

static void jpeg_capture_thread_func(void *args)
{
	LOG_D("jpeg capture started");

	hal_csi_jpeg_probe();
	jpeg_process();
	hal_csi_jpeg_remove();

	LOG_D("jpeg capture end");

	return;
}

int image_capture_start(void)
{
	xTaskCreate(jpeg_capture_thread_func, "jpeg_capture_thread",
				4096, NULL, 2, &jpeg_capture_thread);

	return 0;
}

int image_capture_stop(void)
{
	return 0;
}
