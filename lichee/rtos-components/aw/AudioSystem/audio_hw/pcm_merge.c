#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "audio_hw.h"
#include "AudioBase.h"

#include "local_audio_hw.h"

#include <aw-alsa-lib/pcm.h>
#include <aw-alsa-lib/common.h>

#define PCM_MERGE_CARD_NAME_MAX		(24)

typedef struct pcm_merge_hw_config {
	char name[PCM_MERGE_CARD_NAME_MAX];
	int chnum;
	void *tbuf;
	uint32_t tbuf_len;
	snd_pcm_t *pcm;
} pcm_merge_hw_config_t;


typedef struct pcm_merge_config {
	pcm_merge_hw_config_t main;
	pcm_merge_hw_config_t sub;
	uint32_t frame_bytes;
	uint32_t period_size;
	int channels;
	int bits;
	int chmap[16];
	bool in_use;
} pcm_merge_config_t;

pcm_merge_config_t g_pm_config = {
	.in_use = false,
};

bool pcm_merge_in_use(void)
{
	return g_pm_config.in_use;
}

void pcm_merge_set_channel_map(int *array, unsigned int size)
{
	int i;
	if (size > ARRAY_SIZE(g_pm_config.chmap))
		return;
	_info("chnanel map:");
	for (i = 0; i < size && size < ARRAY_SIZE(g_pm_config.chmap); i++) {
		g_pm_config.chmap[i] = array[i];
		_info("%d --> %d", array[i], i);
	}
	_info("-----------");
	g_pm_config.channels = size;
	g_pm_config.main.tbuf = NULL;
	g_pm_config.main.tbuf_len = 0;
	g_pm_config.sub.tbuf = NULL;
	g_pm_config.sub.tbuf_len = 0;
}

int pcm_merge_create(const char *main_name, int main_ch,
			const char *sub_name, int sub_ch)
{
	int i;
	if (g_pm_config.in_use || !main_name || !sub_name)
		return -1;

	_debug("main_name:%s, sub_name:%s\n", main_name, sub_name);
	snprintf(g_pm_config.main.name, PCM_MERGE_CARD_NAME_MAX - 1, "%s", main_name);
	snprintf(g_pm_config.sub.name, PCM_MERGE_CARD_NAME_MAX - 1, "%s", sub_name);

	g_pm_config.main.chnum = main_ch;
	g_pm_config.sub.chnum = sub_ch;

	for (i = 0; i < ARRAY_SIZE(g_pm_config.chmap); i++)
		g_pm_config.chmap[i] = -1;
	g_pm_config.channels = -1;

	return 0;
}

void *pcm_merge_open(void)
{
	int ret;

	_debug("\n");
	ret = snd_pcm_open(&g_pm_config.main.pcm, g_pm_config.main.name,
			SND_PCM_STREAM_CAPTURE, 0);
	if (ret != 0) {
		_err("open %s failed\n", g_pm_config.main.name);
		return NULL;
	}
	ret = snd_pcm_open(&g_pm_config.sub.pcm, g_pm_config.sub.name,
			SND_PCM_STREAM_CAPTURE, 0);
	if (ret != 0) {
		_err("open %s failed\n", g_pm_config.sub.name);
		snd_pcm_close(g_pm_config.main.pcm);
		g_pm_config.main.pcm = NULL;
		return NULL;
	}

	g_pm_config.in_use = true;

	return (void *)&g_pm_config;
}

int pcm_merge_hw_params(snd_pcm_format_t format,
			uint32_t rate, uint32_t channels,
			snd_pcm_uframes_t period_size,
			snd_pcm_uframes_t buffer_size)
{
	int ret = 0;
	uint32_t len;

	_debug("\n");
	g_pm_config.bits = format_to_bits(format);
	if (g_pm_config.main.chnum + g_pm_config.sub.chnum != channels) {
		_err("channels(%d) != main(%d) + sub(%d)\n",
			channels, g_pm_config.main.chnum, g_pm_config.sub.chnum);
		return -1;
	}
	if (g_pm_config.channels > 0 && g_pm_config.channels != channels) {
		_err("pm_config channels:%d, but channels=%d\n",
			g_pm_config.channels, channels);
		return -1;
	}

	ret = set_param(g_pm_config.main.pcm, format, rate, g_pm_config.main.chnum,
			period_size, buffer_size);
	if (ret != 0) {
		_err("hw_params %s failed\n", g_pm_config.main.name);
		return -1;
	}

	ret = set_param(g_pm_config.sub.pcm, format, rate, g_pm_config.sub.chnum,
			period_size, buffer_size);
	if (ret != 0) {
		_err("hw_params %s failed\n", g_pm_config.sub.name);
		return -1;
	}

	g_pm_config.frame_bytes = snd_pcm_frames_to_bytes(g_pm_config.main.pcm, 1) +
				snd_pcm_frames_to_bytes(g_pm_config.sub.pcm, 1);
	g_pm_config.period_size = period_size;

	/* main pcm */
	len = snd_pcm_frames_to_bytes(g_pm_config.main.pcm, period_size);
	if (g_pm_config.main.tbuf != NULL && g_pm_config.main.tbuf_len != len) {
		_info("tbuf_len=%u, len=%u, free it\n", g_pm_config.main.tbuf_len, len);
		free(g_pm_config.main.tbuf);
		g_pm_config.main.tbuf = NULL;
	}
	if (!g_pm_config.main.tbuf) {
		g_pm_config.main.tbuf = malloc(len);
		g_pm_config.main.tbuf_len = len;
		_info("create tbuf:%p, len=%u\n", g_pm_config.main.tbuf, len);
	}
	if (!g_pm_config.main.tbuf) {
		_err("no memory\n");
		return -1;
	}

	/* sub pcm */
	len = snd_pcm_frames_to_bytes(g_pm_config.sub.pcm, period_size);
	if (g_pm_config.sub.tbuf != NULL && g_pm_config.sub.tbuf_len != len) {
		_info("tbuf_len=%u, len=%u, free it\n", g_pm_config.sub.tbuf_len, len);
		free(g_pm_config.sub.tbuf);
		g_pm_config.sub.tbuf = NULL;
	}
	if (!g_pm_config.sub.tbuf) {
		g_pm_config.sub.tbuf = malloc(len);
		g_pm_config.sub.tbuf_len = len;
		_info("create tbuf:%p, len=%u\n", g_pm_config.main.tbuf, len);
	}
	if (!g_pm_config.sub.tbuf) {
		_err("no memory\n");
		free(g_pm_config.main.tbuf);
		g_pm_config.main.tbuf = NULL;
		g_pm_config.main.tbuf_len = 0;
		return -1;
	}

	return ret;
}

int pcm_merge_prepare(void)
{
	int ret = 0;

	_debug("\n");
	ret = snd_pcm_prepare(g_pm_config.main.pcm);
	if (ret != 0) {
		_err("prepare %s failed\n", g_pm_config.main.name);
	}

	ret = snd_pcm_prepare(g_pm_config.sub.pcm);
	if (ret != 0) {
		_err("prepare %s failed\n", g_pm_config.sub.name);
	}

	return ret;
}

static void pcm_merge_reset(snd_pcm_t *pcm1, snd_pcm_t *pcm2)
{
	/*unsigned int reg_val1, reg_val2;*/

	snd_pcm_drop(pcm1);
	snd_pcm_drop(pcm2);
	snd_pcm_prepare(pcm1);
	snd_pcm_prepare(pcm2);
	snd_pcm_start(pcm1);
	snd_pcm_start(pcm2);

}

static void do_channel_map32(uint32_t *buf)
{
	int channels = g_pm_config.channels;
	int mch = g_pm_config.main.chnum;
	int sch = g_pm_config.sub.chnum;
	int i, j;
	uint32_t *buf1 = g_pm_config.main.tbuf;
	uint32_t *buf2 = g_pm_config.sub.tbuf;
	uint32_t period_size = g_pm_config.period_size;


	for (i = 0; i < channels; i++) {
		for (j = 0; j < period_size; j++) {
			int index = g_pm_config.chmap[i];
			if (index < mch)
				buf[i + channels * j] = buf1[index + j * mch];
			else
				buf[i + channels * j] = buf2[index - mch + j * sch];
		}
	}
}

static void do_channel_map16(uint16_t *buf)
{
	int channels = g_pm_config.channels;
	int mch = g_pm_config.main.chnum;
	int sch = g_pm_config.sub.chnum;
	int i, j;
	uint16_t *buf1 = g_pm_config.main.tbuf;
	uint16_t *buf2 = g_pm_config.sub.tbuf;
	uint32_t period_size = g_pm_config.period_size;


	for (i = 0; i < channels; i++) {
		for (j = 0; j < period_size; j++) {
			int index = g_pm_config.chmap[i];
			if (index < mch)
				buf[i + channels * j] = buf1[index + j * mch];
			else
				buf[i + channels * j] = buf2[index - mch + j * sch];
		}
	}
}

/* Notice: bytes must be period_bytes*/
int pcm_merge_read(void *buf, unsigned int bytes)
{
	snd_pcm_t *pcm1 = g_pm_config.main.pcm;
	snd_pcm_t *pcm2 = g_pm_config.sub.pcm;
	snd_pcm_uframes_t size;
	snd_pcm_uframes_t frames_loop = g_pm_config.period_size;
	snd_pcm_sframes_t ret, total = bytes / g_pm_config.frame_bytes;
	snd_pcm_sframes_t frames = total;
	uint32_t offset1 = 0, offset2 = 0;
	snd_pcm_state_t st1, st2;
	void *buf1 = g_pm_config.main.tbuf;
	void *buf2 = g_pm_config.sub.tbuf;
	bool need_fill_silence_frames = false;

	st1 = snd_pcm_state(pcm1);
	st2 = snd_pcm_state(pcm2);
	if (st1 == SND_PCM_STATE_PREPARED || st2 == SND_PCM_STATE_PREPARED) {
		pcm_merge_reset(pcm1, pcm2);
		need_fill_silence_frames = true;
	} else if ((st1 != SND_PCM_STATE_RUNNING && st1 != SND_PCM_STATE_XRUN) ||
		(st2 != SND_PCM_STATE_RUNNING && st2 != SND_PCM_STATE_XRUN)) {
		_err("unexpect st1:%d, st2:%d\n", st1, st2);
	}

	while (total > 0) {
		if (total < frames_loop)
			size = total;
		else
			size = frames_loop;
		ret = snd_pcm_readi(pcm1, buf1 + offset1, size);
		if (ret != size) {
			printf("[%s] line:%d snd_pcm_readi return %ld\n",
				__func__, __LINE__, ret);
			/*snd_pcm_dump(pcm1);*/
			/*snd_pcm_dump(pcm2);*/
			pcm_merge_reset(pcm1, pcm2);
			need_fill_silence_frames = true;
			continue;
		}
		ret = snd_pcm_readi(pcm2, buf2 + offset2, size);
		if (ret != size) {
			printf("[%s] line:%d snd_pcm_readi return %ld\n",
				__func__, __LINE__, ret);
			/*snd_pcm_dump(pcm1);*/
			/*snd_pcm_dump(pcm2);*/
			pcm_merge_reset(pcm1, pcm2);
			need_fill_silence_frames = true;
			continue;
		}
		if (need_fill_silence_frames) {
			int silence_frames = 20;
			if (silence_frames > size)
				silence_frames = size;
			memset(buf1 + offset1, 0x0, snd_pcm_frames_to_bytes(pcm1, silence_frames));
			memset(buf2 + offset2, 0x0, snd_pcm_frames_to_bytes(pcm2, silence_frames));
			need_fill_silence_frames = false;
		}
		offset1 += snd_pcm_frames_to_bytes(pcm1, size);
		offset2 += snd_pcm_frames_to_bytes(pcm2, size);
		total -= size;
#if 0
		if (offset1 >= snd_pcm_frames_to_bytes(pcm1, frames_loop) ||
			offset2 >= snd_pcm_frames_to_bytes(pcm2, frames_loop)) {
			do_channel_map(buf, frames_loop);
			offset1 = 0;
			offset2 = 0;
		}
#endif
	}
	/*_debug("bytes=%u, frames=%ld, offset1=%u, offset2=%u\n", bytes, frames, offset1, offset2);*/
#if 0
	if (offset1 > 0 || offset2 > 0) {
		do_channel_map(buf, frames%frames_loop);
	}
#endif
	if (g_pm_config.bits == 16) {
		do_channel_map16(buf);
	} else if (g_pm_config.bits == 32) {
		do_channel_map32(buf);
	} else {
		fatal("unknown bits");
	}

	return frames;
}

int pcm_merge_stop(void)
{
	int ret = 0;

	_debug("\n");
	ret = snd_pcm_drop(g_pm_config.main.pcm);
	if (ret != 0) {
		_err("stop %s failed\n", g_pm_config.main.name);
	}

	ret = snd_pcm_drop(g_pm_config.sub.pcm);
	if (ret != 0) {
		_err("stop %s failed\n", g_pm_config.sub.name);
	}

	return ret;

}

int pcm_merge_close(void *handle)
{
	int ret = 0;

	_debug("\n");
	if (!handle)
		return -1;
	ret = snd_pcm_close(g_pm_config.main.pcm);
	if (ret != 0) {
		_err("close %s failed\n", g_pm_config.main.name);
	}
	g_pm_config.main.pcm = NULL;

	ret = snd_pcm_close(g_pm_config.sub.pcm);
	if (ret != 0) {
		_err("close %s failed\n", g_pm_config.sub.name);
	}
	g_pm_config.sub.pcm = NULL;

	g_pm_config.in_use = false;

	return 0;
}
