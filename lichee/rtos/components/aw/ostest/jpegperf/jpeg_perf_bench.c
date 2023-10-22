/*
 * Copyright (C) 2019 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include <console.h>

#include "jpeg_raw_compress.h"

#include "jpeg_perf_bench.h"

#define RAW_BUF_USE_EXT_RAM_DATA 0

#ifndef OS_GetTicks
#define OS_GetTicks()       (xTaskGetTickCount())
#endif

static int load_file(char *file_name, uint8_t *buf, int size)
{
	int ret = 0;
	int fd = -1;

	fd = open(file_name, O_RDONLY);
	if (fd <= 0) {
		printf("file open fail: %d, file %s.\n", fd, file_name);
		ret = -1;
		goto err;
	}
	printf("file open %s.\n", file_name);

	ret = read(fd, buf, size);
	if (ret != size) {
		printf("file read fail: require %d, return %d.\n", size, ret);
		goto err;
	}
	printf("image load from %s\n", file_name);

	ret = 0;
err:

	if (fd >= 0) {
		ret = close(fd);
		if (ret != 0) {
			printf("file close fail: %d\n", ret);
			ret = -1;
		}
	}
	return ret;
}

static int save_file(char *file_name, uint8_t *buf, int size)
{
	int ret = 0;
	int fd = -1;

	fd = open(file_name, O_RDWR | O_CREAT);
	if (fd <= 0) {
		printf("file open fail: %d, file %s.\n", fd, file_name);
		ret = -1;
		goto err;
	}
	printf("file create %s.\n", file_name);

	ret = write(fd, buf, size);
	if (ret != size) {
		printf("file write fail: require %d, return %d.\n", size, ret);
		goto err;
	}
	printf("image written to %s\n", file_name);

	ret = 0;

err:

	if (fd >= 0) {
		ret = close(fd);
		if (ret != 0) {
			printf("file close fail: %d\n", ret);
			ret = -1;
		}
	}
	return ret;
}

static int print_file(uint8_t *buf, int size)
{
	if (buf == NULL) {
		printf("buf is null\n");
		return -1;
	}

	printf("\n======size: %d bytes====\n\n", size);
	for (int i = 0; i < size; i++) {
		printf("%02x", buf[i]);
	}
	printf("\n\n");
	return 0;
}

int jpeg_raw_compress_test(char *file_name, int width, int height,
                           char *subsample_name, int jpeg_quality)
{
	int ret = 0;

	jpeg_compress_cfg_t img_cfg;

	uint8_t *raw_buf = NULL;
	uint32_t raw_size = 0;

	uint8_t *jpeg_buf = NULL;
	uint32_t jpeg_size = 0;

	uint32_t jpeg_compress_start = 0;
	uint32_t jpeg_compress_end = 0;

	img_cfg.width = width;
	img_cfg.height = height;
	img_cfg.quality = jpeg_quality;

	printf("jpeg compress test start\n");

	if (strcmp(subsample_name, "yuyv") == 0) {
		img_cfg.raw_type = JPEG_RAW_TYPE_YUYV;
		raw_size = width * height * 2;
	} else {
		printf("subsamp error: %s\n", subsample_name);
		ret = -1;
		goto err;
	}

#if (RAW_BUF_USE_EXT_RAM_DATA)
	if (raw_size > (RAW_BUF_SIZE_IN_EXT_RAM * RAW_BUF_MAX_NUM)) {
		printf("raw buf err: %d\n", raw_size);
		ret = -1;
		goto err;
	}
	raw_buf = &raw_buf_mem[0][0];
#else
	raw_buf = malloc(raw_size);
	if (raw_buf == NULL) {
		printf("raw buf malloc err: %d\n", raw_size);
		ret = -1;
		goto err;
	}
	printf("malloc raw_size %d.\n", raw_size);
#endif

	ret = load_file(file_name, raw_buf, raw_size);

	if (ret) {
		printf("load raw data fail.\n");
		goto err;
	}

	ret = jpeg_raw_compress_init(&img_cfg);
	if (ret) {
		printf("jpeg compress init fail.\n");
		goto err;
	}

	printf("jpeg compress init success.\n");

	jpeg_compress_start = OS_GetTicks();

	ret = jpeg_raw_compress(raw_buf, &jpeg_buf, &jpeg_size);

	jpeg_compress_end = OS_GetTicks();
	if (ret) {
		printf("jpeg compress fail: %d\n", ret);
		goto err;
	}

	printf("jpeg compress time: %d ms\n", jpeg_compress_end - jpeg_compress_start);

#if (JPEG_COMPRESS_TEST_STORE_FILE)
	char *p_char = NULL;
	int char_num = 0;
	char jpeg_file_name[256];

	p_char = strrchr(file_name, '.');
	if (p_char != NULL) {
		char_num = p_char - file_name;
		if (char_num > 255) {
			char_num = 200;
		}
		snprintf(jpeg_file_name, char_num + 1, "%s", file_name);
		snprintf(&jpeg_file_name[char_num], 255 - char_num, "_Q%d.jpg", jpeg_quality);
	}

	ret = save_file(jpeg_file_name, jpeg_buf, jpeg_size);
	if (ret) {
		printf("save jpeg file fail: %d\n", ret);
		goto err;
	}

	printf("save jpeg file quality: %d\n", jpeg_quality);
#endif

#if (JPEG_COMPRESS_TEST_PRINT_FILE)
	print_file(jpeg_buf, jpeg_size);
#endif

err:
	jpeg_raw_compress_exit();
	if (jpeg_buf) {
		free(jpeg_buf);
	}
	jpeg_buf = NULL;
	jpeg_size = 0;
#if (RAW_BUF_USE_EXT_RAM_DATA == 0)
	if (raw_buf) {
		free(raw_buf);
	}
#endif
	raw_buf = NULL;
	raw_size = 0;
	printf("jpeg compress test end(%d)\n", ret);

	return ret;
}

int cmd_jpeg_perf(int argc, char **argv)
{
	char *file_name;
	int width;
	int height;
	char *subsample_name;
	int jpeg_quality;

	if (argc < 5) {
        printf("invalid jpeg mark cmd, argc %d\n", argc);
        return -1;
	}

    file_name = argv[0];
	width = atoi(argv[1]);
	height = atoi(argv[2]);
	subsample_name = argv[3];
	jpeg_quality = atoi(argv[4]);
    jpeg_raw_compress_test(file_name, width, height, subsample_name, jpeg_quality);
	return -1;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_jpeg_perf, jpeg_perf, jpeg bench);
