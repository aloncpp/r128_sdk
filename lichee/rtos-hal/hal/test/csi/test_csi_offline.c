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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "FreeRTOS/_os_semaphore.h"
#include "FreeRTOS/_os_time.h"
#include "sunxi_hal_twi.h"
#include <fcntl.h>
#include <hal_cmd.h>
#include <hal_log.h>
#include <hal_mem.h>
#include <hal_cache.h>
#include <hal_thread.h>
#include "hal_csi_jpeg.h"
#include "jpegenc.h"

static uint8_t* gaddr[3];
static uint32_t gsize[3];

typedef struct {
	char *out;
	unsigned int count;
	char *path;
} offline_command;
static offline_command cmd;

static void cmd_show_help(void)
{
	printf( "Usage:\n"
		"-h: show the help message.\n"
		"-o: [nv12/jpeg].output picture type.\n"
		"-c: count of output picture.\n"
		"-p: path of output picture.\n"
	);
}

static int read_frame(struct csi_jpeg_mem *csi_mem, unsigned int order)
{
	FILE* fd;
	long long res;
	char name[128];

	hal_log_info("%s line: %d addr = 0x%08x size = %d\n", __func__, __LINE__,
				csi_mem->buf.addr, csi_mem->buf.size);

	sprintf(name, "/%s/nv12_%d.bin", cmd.path, order);
	fd = fopen(name, "ab");
	if (fd < 0) {
		hal_log_info("open %s error %d\n", name, fd);
		return -1;
	}

	res = fwrite(csi_mem->buf.addr, csi_mem->buf.size, 1, fd);
	if (res < 0) {
		hal_log_info("write fail(%d), line%d..\n", res, __LINE__);
		fclose(fd);
		return -1;
	}
	hal_log_info("write YUV image ok, Path: %s\n", name);

	fclose(fd);

	return 0;
}

static int read_offline_jpg(void *addr, int size, unsigned int order)
{
	FILE* fd;
	long long res;
	char name[128];

	hal_log_info("%s line: %d addr = 0x%08x size = %d\n", __func__, __LINE__, addr, size);

	sprintf(name, "/%s/offline_%d.jpg", cmd.path, order);
	fd = fopen(name, "ab");
	if (fd < 0) {
		hal_log_info("open %s error %d\n", name, fd);
		return -1;
	}

	res = fwrite(addr, size, 1, fd);
	if (res < 0) {
		hal_log_info("write fail(%d), line%d..\n", res, __LINE__);
		fclose(fd);
		return -1;
	}
	hal_log_info("write JPEG image ok, Path: %s\n", name);

	fclose(fd);

	return 0;
}

static void jpeg_offline_cb(struct csi_jpeg_mem *jpeg_mem)
{
	hal_dcache_clean_invalidate((unsigned long)jpeg_mem->buf.addr,
			jpeg_mem->buf.size); /* necessary operation */
	memcpy(gaddr[jpeg_mem->index], jpeg_mem->buf.addr - JPEG_HEADER_LEN,
			jpeg_mem->buf.size + JPEG_HEADER_LEN);
	gsize[jpeg_mem->index] = jpeg_mem->buf.size + JPEG_HEADER_LEN;
}

static void main_test()
{
	struct csi_jpeg_fmt fmt;
	unsigned int count;
	struct csi_jpeg_mem *csi_mem;
	struct csi_jpeg_mem *jpeg_mem;
	unsigned int timeout_msec;
	unsigned int width, height;
	unsigned int i, order = 0;

/*	for(int i = 0x8000000; i < 0x8400000; i += 4)
		*(int *)i = 0;
	hal_log_info("lpsram init set 0x0");
	hal_msleep(2000);
*/

	hal_csi_sensor_get_sizes(&width, &height);
	hal_log_info("sensor win: width = %d, height = %d\n", width, height);

	memset(&fmt, 0, sizeof(struct csi_jpeg_fmt));
	fmt.width = width;
	fmt.height = height;
	fmt.line_mode = OFFLINE_MODE;
	if (!strcmp(cmd.out, "jpeg")) {
		fmt.output_mode = PIX_FMT_OUT_MAX;
		fmt.cb = &jpeg_offline_cb;
	} else {
		fmt.output_mode = PIX_FMT_OUT_NV12;
	}
	hal_csi_jpeg_set_fmt(&fmt);

	count = 3;

	if (hal_csi_jpeg_reqbuf(count) != 0) {
		return;
	}

	if (!strcmp(cmd.out, "jpeg")) {
		for (i = 0; i < count; i++) {
			gaddr[i] = hal_malloc(JPEG_OUTPUT_BUFF_SIZE + JPEG_HEADER_LEN);
			if (gaddr[i]) {
				hal_log_info("jpeg pic data addr = %x\n", gaddr[i]);
				memset(gaddr[i], 0 , JPEG_OUTPUT_BUFF_SIZE + JPEG_HEADER_LEN);
			} else {
				hal_log_info("[%s,%d] jpeg pic malloc fail!\n", __func__, __LINE__);
				while (i-- > 0) {
					hal_free(gaddr[i]);
				}
				hal_csi_jpeg_freebuf();
				return;
			}
		}
	}

	hal_csi_jpeg_s_stream(1);
	hal_log_info("csi stream on!");

	timeout_msec = 2000;  // recommend 2s, 10s for pm test

	if (!strcmp(cmd.out, "jpeg")) {
		while (order++ < cmd.count) {
			jpeg_mem = hal_jpeg_dqbuf(timeout_msec);
			if (jpeg_mem == NULL) {
				continue;
			}
			read_offline_jpg(gaddr[jpeg_mem->index], gsize[jpeg_mem->index], order);
			hal_jpeg_qbuf();
		}
	} else {
		while (order++ < cmd.count) {
			csi_mem = hal_csi_dqbuf(timeout_msec);
			if (csi_mem == NULL) {
				continue;
			}
			read_frame(csi_mem, order);
			hal_csi_qbuf();
		}
	}

	if (!strcmp(cmd.out, "jpeg")) {
		for (i = 0; i < count; i++) {
			hal_free(gaddr[i]);
		}
	}

	hal_csi_jpeg_s_stream(0);
	hal_csi_jpeg_freebuf();
	hal_log_info("csi stream off!!\n");

}

static void csi_offline_thread(void)
{
	hal_log_info("csi jpeg offline demo started\n");
	hal_csi_jpeg_probe();

	main_test();

	hal_csi_jpeg_remove();
	hal_log_info("csi jpeg offline demo over\n");
}

void cmd_csi_jpeg_offline_test(int argc, const char **argv)
{
    int c;

	cmd.out = "jpeg";
	cmd.count = 3;
	cmd.path = "/data/";

	if (argc < 2) {
		printf("Default: hal_csi_jpeg_offline -o jpeg -c 3 -p /data/ \n");
		printf("More Usage Message please enter: hal_csi_jpeg_offline -h \n");
	} else {
		while ((c = getopt(argc, (char *const *)argv, "ho:c:p:")) != -1) {
			printf("getopt: %c, optarg : %s \n",c, optarg);
			switch (c) {
				case 'o':
					cmd.out = optarg;
					break;
				case 'c':
					cmd.count = atoi(optarg);
					break;
				case 'p':
					cmd.path = optarg;
					break;
				case 'h':
					cmd_show_help();
					return;
				default:
					printf("Default: hal_csi_jpeg_offline -o jpeg -c 3 -p /data/ \n");
					cmd_show_help();
					break;
			}
		}
	}

	csi_offline_thread();

}

FINSH_FUNCTION_EXPORT_CMD(cmd_csi_jpeg_offline_test, hal_csi_jpeg_offline, csi jpeg offline encode test)
