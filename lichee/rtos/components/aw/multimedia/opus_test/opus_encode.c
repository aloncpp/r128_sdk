#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "include/opus.h"

#define INPUT_MODE 1 /* 1:read from file system; 2:read from audio record */

#if (INPUT_MODE == 2)
#include "audio/pcm/audio_pcm.h"
#include "audio/manager/audio_manager.h"
#endif

#define INPUT_AUDIO_SAMPLE_RATE 16000
#define INPUT_AUDIO_SAMPLE_SIZE 2
#define INPUT_AUDIO_CHANNEL     1
#define FRAME_SIZE              160 /* only support 2.5, 5, 10, 20, 40 or 60 ms */

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

struct opusEncodeContext {
	OpusEncoder *enc;
};

static void int_to_char(uint32_t i, unsigned char ch[4])
{
	ch[0] = i>>24;
	ch[1] = (i>>16)&0xFF;
	ch[2] = (i>>8)&0xFF;
	ch[3] = i&0xFF;
}

/* =======================input======================= */
static struct inputContext inContext;
int opus_encode_input_init(void)
{
#if (INPUT_MODE == 1)
	inContext.fp = fopen("/data/16000.pcm", "rb+");
	if (inContext.fp == NULL) {
		printf("open file fail.\n");
		return -1;
	}

#elif (INPUT_MODE == 2)
	int ret;
	unsigned int tick;
	struct pcm_config config;

	config.channels = INPUT_AUDIO_CHANNEL;
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

unsigned int opus_encode_input_data(void *buffer, unsigned int len)
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

int opus_encode_input_deinit(void)
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
int opus_encode_output_init(void)
{
	outContext.fp = fopen("/data/16000_opus_encode", "wb+");
	if (outContext.fp == NULL) {
		printf("create file fail.\n");
		return -1;
	}

	return 0;
}

unsigned int opus_encode_output_data(void *buffer, unsigned int len)
{
	unsigned int act_write;
	act_write = fwrite(buffer, 1, len, outContext.fp);

	return act_write;
}

int opus_encode_output_deinit(void)
{
	fclose(outContext.fp);
	return 0;
}

static struct opusEncodeContext encContext;
int opus_encode_start(void)
{
	int err;
	int application;
	int complexity;
	int signal_type;
#if (INPUT_MODE == 1)
	application = OPUS_APPLICATION_AUDIO;
	complexity = 8;
	signal_type = OPUS_SIGNAL_MUSIC;
#elif (INPUT_MODE == 2)
	application = OPUS_APPLICATION_VOIP;
	complexity = 2;
	signal_type = OPUS_SIGNAL_VOICE;
#endif
	printf("opus version:%s\n", opus_get_version_string());
	encContext.enc = opus_encoder_create(INPUT_AUDIO_SAMPLE_RATE, INPUT_AUDIO_CHANNEL, application, &err);
	if (err != OPUS_OK) {
		return -1;
	}
	opus_encoder_ctl(encContext.enc, OPUS_SET_BITRATE(OPUS_AUTO));
	opus_encoder_ctl(encContext.enc, OPUS_SET_COMPLEXITY(complexity));
	opus_encoder_ctl(encContext.enc, OPUS_SET_SIGNAL(signal_type));

	return 0;
}

int opus_encode_data(short *sdata, int sdataLen, unsigned char *encBuffer, int encBufferLen)
{
	return opus_encode(encContext.enc, sdata, sdataLen, encBuffer, encBufferLen);
}

int opus_encode_stop(void)
{
	opus_encoder_destroy(encContext.enc);
	return 0;
}

/* stay the same with official demo */
int opus_encode_get_final_range(uint32_t *range)
{
	opus_encoder_ctl(encContext.enc, OPUS_GET_FINAL_RANGE(range));
	return 0;
}

int opus_encode_demo(void)
{
	int ret;
	unsigned int len;
	int bufLen;
	short *inputBuf = NULL;
	unsigned char *encBuf = NULL;
	unsigned char int_field[4];
	uint32_t enc_final_range;

	bufLen = FRAME_SIZE * INPUT_AUDIO_SAMPLE_SIZE * INPUT_AUDIO_CHANNEL;
	inputBuf = malloc(bufLen);
	if (inputBuf == NULL) {
		goto err0;
	}
	encBuf = malloc(bufLen);
	if (encBuf == NULL) {
		goto err0;
	}
	ret = opus_encode_input_init();
	if (ret) {
		goto err0;
	}
	ret = opus_encode_output_init();
	if (ret) {
		goto err1;
	}
	ret = opus_encode_start();
	if (ret) {
		goto err2;
	}
	while (1) {
		len = opus_encode_input_data(inputBuf, bufLen);
		if (len != bufLen) {
			break;
		}
		len = opus_encode_data(inputBuf, bufLen / 2, encBuf, bufLen);
		if (len < 0) {
			break;
		}
		int_to_char(len, int_field);
		opus_encode_output_data(int_field, 4);
		opus_encode_get_final_range(&enc_final_range);
		int_to_char(enc_final_range, int_field);
		opus_encode_output_data(int_field, 4);
		opus_encode_output_data(encBuf, len);
	}
	opus_encode_stop();
err2:
	opus_encode_output_deinit();
err1:
	opus_encode_input_deinit();
err0:
	free(encBuf);
	free(inputBuf);
	return 0;
}

