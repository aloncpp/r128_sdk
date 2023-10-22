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

#define INPUT_MODE 1 /* 1:read from file system; 2:read from audio record */

#if (INPUT_MODE == 2)
#include "audio/pcm/audio_pcm.h"
#include "audio/manager/audio_manager.h"
#endif

#define INPUT_AUDIO_SAMPLE_RATE 16000

struct inputContext {
#if (INPUT_MODE == 1)
	FILE *fp;
#elif (INPUT_MODE == 2)
	unsigned int startTime;
#endif
};

struct outputContext {
	FILE *fp;
};

struct speexEncodeContext {
	void *st;
	SpeexBits ebits;
};

/* =======================input======================= */
static struct inputContext inContext;
int encode_input_init(void)
{
#if (INPUT_MODE == 1)
	inContext.fp = fopen("/data/16000.pcm", "rb+");
	if (inContext.fp == NULL) {
		printf("encode_input_init() open file fail.\n");
		return -1;
	}

#elif (INPUT_MODE == 2)
	int ret;
	unsigned int tick;
	struct pcm_config config;

	config.channels = 1;
	config.format = PCM_FORMAT_S16_LE;
	config.period_count = 2;
	config.period_size = 1024;
	config.rate = INPUT_AUDIO_SAMPLE_RATE;
	ret = snd_pcm_open(AUDIO_SND_CARD_DEFAULT, PCM_IN, &config);
	if (ret != 0) {
		printf("pcm open fail.\n");
		return -1;
	}
	audio_manager_handler(AUDIO_SND_CARD_DEFAULT, AUDIO_MANAGER_SET_VOLUME_LEVEL, AUDIO_IN_DEV_AMIC, 3);
	tick = OS_GetTicks();
	inContext.startTime = OS_TicksToMSecs(tick);
#endif

	return 0;
}

#if (INPUT_MODE == 2)
#define RECORD_DATA_DURATION_MS   20000
#endif

unsigned int encode_input_data(void *buffer, unsigned int len)
{
#if (INPUT_MODE == 1)
	unsigned int act_read;
	act_read = fread(buffer, 1, len, inContext.fp);
	return act_read;
#elif (INPUT_MODE == 2)
	unsigned int tick;
	unsigned int nowTime;

	tick = OS_GetTicks();
	nowTime = OS_TicksToMSecs(tick);
	if (((nowTime - inContext.startTime) > RECORD_DATA_DURATION_MS)) {
		return 0;
	}
	snd_pcm_read(AUDIO_SND_CARD_DEFAULT, buffer, len);
	return len;
#endif
}

int encode_input_deinit(void)
{
#if (INPUT_MODE == 1)
	fclose(inContext.fp);
#elif (INPUT_MODE == 2)
	snd_pcm_close(AUDIO_SND_CARD_DEFAULT, PCM_IN);
#endif
	return 0;
}

/* =======================output======================= */
static struct outputContext outContext;
int encode_output_init(void)
{
        outContext.fp = fopen("/data/16000_encode", "wb+");
	if (outContext.fp == NULL) {
		printf("create file fail.\n");
		return -1;
	}

	return 0;
}

unsigned int encode_output_data(void *buffer, unsigned int len)
{
	unsigned int act_write;
	act_write = fwrite(buffer, 1, len, outContext.fp);

	return act_write;
}

int encode_output_deinit(void)
{
	fclose(outContext.fp);
	return 0;
}

static struct speexEncodeContext encContext;
int speex_encode_start(void)
{
	int rate = INPUT_AUDIO_SAMPLE_RATE;
	int quality = 5;
	int complexity = 2;
	int modeID;
	const SpeexMode *mode = NULL;
	const char *speex_version;

	speex_lib_ctl(SPEEX_LIB_GET_VERSION_STRING, (void *)&speex_version);
	printf("speex version:%s\n", speex_version);

	modeID = SPEEX_MODEID_WB;
	mode = speex_lib_get_mode (modeID);
	encContext.st = speex_encoder_init(mode);
	speex_encoder_ctl(encContext.st, SPEEX_SET_COMPLEXITY, &complexity);
	speex_encoder_ctl(encContext.st, SPEEX_SET_SAMPLING_RATE, &rate);
	speex_encoder_ctl(encContext.st, SPEEX_SET_QUALITY, &quality);
	speex_bits_init(&encContext.ebits);

	return 0;
}

int speex_encode_frame_size(void)
{
	int frame_size;

	speex_encoder_ctl(encContext.st, SPEEX_GET_FRAME_SIZE, &frame_size);
	return frame_size * sizeof(short);
}

int speex_encode_data(short *sdata, int sdataLen, unsigned char *encBuffer, int encBufferLen)
{
	int nbBytes;

	speex_encode_int(encContext.st, sdata, &encContext.ebits);
	nbBytes = speex_bits_write(&encContext.ebits, (char *)encBuffer, encBufferLen);
	speex_bits_reset(&encContext.ebits);

	return nbBytes;
}

int speex_encode_stop(void)
{
	speex_encoder_destroy(encContext.st);
	speex_bits_destroy(&encContext.ebits);
	return 0;
}

int speex_encode_demo(void)
{
	int ret;
	unsigned int len;
	int bufLen;
	short *inputBuf = NULL;
	unsigned char *encBuf = NULL;

	ret = encode_input_init();
	if (ret) {
		goto err0;
	}
	ret = encode_output_init();
	if (ret) {
		goto err1;
	}
	ret = speex_encode_start();
	if (ret) {
		goto err2;
	}
	bufLen = speex_encode_frame_size();
	inputBuf = malloc(bufLen);
	if (inputBuf == NULL) {
		goto err3;
	}
	encBuf = malloc(bufLen);
	if (encBuf == NULL) {
		goto err3;
	}
	while (1) {
		len = encode_input_data(inputBuf, bufLen);
		if (len != bufLen) {
			break;
		}
		len = speex_encode_data(inputBuf, bufLen / 2, encBuf, bufLen);
		encode_output_data(&len, sizeof(len));
		encode_output_data(encBuf, len);
	}
err3:
	free(inputBuf);
	free(encBuf);
	speex_encode_stop();
err2:
	encode_output_deinit();
err1:
	encode_input_deinit();
err0:
	return 0;
}

