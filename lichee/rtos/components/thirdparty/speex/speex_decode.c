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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "speex.h"

struct inputContext {
	FILE *fp;
};

struct outputContext {
	FILE *fp;
};

struct speexDecodeContext {
	void *st;
	SpeexBits dbits;
};

/* =======================input======================= */
static struct inputContext inContext;
int decode_input_init(void)
{
	inContext.fp = fopen("/data/16000_encode", "rb+");

	if (inContext.fp == NULL) {
		printf("decode_input_init() open file fail.\n");
		return -1;
	}

	return 0;
}

unsigned int decode_input_data(void *buffer, unsigned int len)
{
	unsigned int act_read;
	act_read = fread(buffer, 1, len, inContext.fp);

	return act_read;
}

int decode_input_deinit(void)
{
	fclose(inContext.fp);
	return 0;
}

/* =======================output======================= */
static struct outputContext outContext;
int decode_output_init(void)
{
        outContext.fp = fopen("/data/16000_decode.pcm", "wb+");
	if (outContext.fp == NULL) {
		printf("create file fail.\n");
		return -1;
	}

	return 0;
}

unsigned int decode_output_data(void *buffer, unsigned int len)
{
	unsigned int act_write;
	act_write = fwrite(buffer, 1, len, outContext.fp);

	return act_write;
}

int decode_output_deinit(void)
{
	fclose(outContext.fp);
	return 0;
}

static struct speexDecodeContext decContext;
int speex_decode_start(void)
{
	int enh = 1;
	const char *speex_version;
	int modeID;
	const SpeexMode *mode = NULL;

	speex_lib_ctl(SPEEX_LIB_GET_VERSION_STRING, (void *)&speex_version);
	printf("speex version:%s\n", speex_version);

	speex_bits_init(&decContext.dbits);
	modeID = SPEEX_MODEID_WB;
	mode = speex_lib_get_mode (modeID);
	decContext.st = speex_decoder_init(mode);
	speex_decoder_ctl(decContext.st, SPEEX_SET_ENH, &enh);

	return 0;
}

int speex_decode_frame_size(void)
{
	int frame_size;

	speex_decoder_ctl(decContext.st, SPEEX_GET_FRAME_SIZE, &frame_size);
	return frame_size * sizeof(short);
}

int speex_decode_data(unsigned char *sdata, int sdataLen, short *decBuffer, int decBufferLen)
{
	int frame_size;

	speex_bits_read_from(&decContext.dbits, (const char *)sdata, sdataLen);
	speex_decode_int(decContext.st, &decContext.dbits, decBuffer);
	speex_decoder_ctl(decContext.st, SPEEX_GET_FRAME_SIZE, &frame_size);

	return frame_size * sizeof(short);
}

int speex_decode_stop(void)
{
	speex_bits_destroy(&decContext.dbits);
	speex_decoder_destroy(decContext.st);
	return 0;
}

int speex_decode_demo(void)
{
	int ret;
	int encLen;
	unsigned int len;
	int bufLen;
	unsigned char *inputBuf = NULL;
	short *decBuf = NULL;

	ret = decode_input_init();
	if (ret) {
		goto err0;
	}
	ret = decode_output_init();
	if (ret) {
		goto err1;
	}
	ret = speex_decode_start();
	if (ret) {
		goto err2;
	}
	bufLen = speex_decode_frame_size();
	inputBuf = malloc(bufLen);
	if (inputBuf == NULL) {
		goto err3;
	}
	decBuf = malloc(bufLen);
	if (decBuf == NULL) {
		goto err3;
	}
	while (1) {
		len = decode_input_data(&encLen, sizeof(encLen));
		if (len != sizeof(encLen)) {
			break;
		}
		len = decode_input_data(inputBuf, encLen);
		if (len != encLen) {
			break;
		}
		len = speex_decode_data(inputBuf, encLen, decBuf, bufLen);
		decode_output_data(decBuf, len);
	}
err3:
	free(decBuf);
	free(inputBuf);
	speex_decode_stop();
err2:
	decode_output_deinit();
err1:
	decode_input_deinit();
err0:
	return 0;
}

