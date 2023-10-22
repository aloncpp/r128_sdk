#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "include/opus.h"

#define INPUT_AUDIO_SAMPLE_RATE 16000
#define INPUT_AUDIO_SAMPLE_SIZE 2
#define INPUT_AUDIO_CHANNEL     1
#define FRAME_SIZE              160 /* only support 2.5, 5, 10, 20, 40 or 60 ms */

struct inputContext {
	FILE *fp;
};

struct outputContext {
	FILE *fp;
};

struct opusDecodeContext {
	OpusDecoder *dec;
};

static uint32_t char_to_int(unsigned char ch[4])
{
	return ((uint32_t)ch[0]<<24) | ((uint32_t)ch[1]<<16)
	     | ((uint32_t)ch[2]<< 8) |  (uint32_t)ch[3];
}

/* =======================input======================= */
static struct inputContext inContext;
int opus_decode_input_init(void)
{
	inContext.fp = fopen("/data/16000_opus_encode", "rb+");
	if (inContext.fp == NULL) {
		printf("open file fail.\n");
		return -1;
	}

	return 0;
}

unsigned int opus_decode_input_data(void *buffer, unsigned int len)
{
	unsigned int act_read;
	act_read = fread(buffer, 1, len, inContext.fp);

	return act_read;
}

int opus_decode_input_deinit(void)
{
	fclose(inContext.fp);
	return 0;
}

/* =======================output======================= */
static struct outputContext outContext;
int opus_decode_output_init(void)
{
	outContext.fp = fopen("/data/16000_opus_decode.pcm", "wb+");
	if (outContext.fp == NULL) {
		printf("create file fail.\n");
		return -1;
	}

	return 0;
}

unsigned int opus_decode_output_data(void *buffer, unsigned int len)
{
	unsigned int act_write;
	act_write = fwrite(buffer, 1, len, outContext.fp);
	return act_write;
}

int opus_decode_output_deinit(void)
{
	fclose(outContext.fp);
	return 0;
}

static struct opusDecodeContext decContext;
int opus_decode_start(void)
{
	int err;

	printf("opus version:%s\n", opus_get_version_string());
	decContext.dec = opus_decoder_create(INPUT_AUDIO_SAMPLE_RATE, INPUT_AUDIO_CHANNEL, &err);
	if (err != OPUS_OK) {
		return -1;
	}

	return 0;
}

int opus_decode_data(unsigned char *sdata, int sdataLen, short *decBuffer, int decBufferLen)
{
	return opus_decode(decContext.dec, (const unsigned char *)sdata, sdataLen, decBuffer, decBufferLen, 0);
}

int opus_decode_stop(void)
{
	opus_decoder_destroy(decContext.dec);
	return 0;
}

int opus_decode_demo(void)
{
	int ret;
	int encLen;
	unsigned int len;
	int bufLen;
	unsigned char *inputBuf = NULL;
	short *decBuf = NULL;
	unsigned char ch[4];
	uint32_t dec_final_range;

	bufLen = FRAME_SIZE * INPUT_AUDIO_SAMPLE_SIZE * INPUT_AUDIO_CHANNEL;
	inputBuf = malloc(bufLen);
	if (inputBuf == NULL) {
		goto err0;
	}
	decBuf = malloc(bufLen);
	if (decBuf == NULL) {
		goto err0;
	}
	ret = opus_decode_input_init();
	if (ret) {
		goto err0;
	}
	ret = opus_decode_output_init();
	if (ret) {
		goto err1;
	}
	ret = opus_decode_start();
	if (ret) {
		goto err2;
	}
	while (1) {
		len = opus_decode_input_data(ch, 4);
		if (len != 4) {
			break;
		}
		encLen = char_to_int(ch);
		len = opus_decode_input_data(&dec_final_range, 4);
		if (len != 4) {
			break;
		}
		len = opus_decode_input_data(inputBuf, encLen);
		if (len != encLen) {
			break;
		}
		len = opus_decode_data(inputBuf, encLen, decBuf, bufLen);
		opus_decode_output_data(decBuf, len * 2);
	}
	opus_decode_stop();
err2:
	opus_decode_output_deinit();
err1:
	opus_decode_input_deinit();
err0:
	free(decBuf);
	free(inputBuf);
	return 0;
}

