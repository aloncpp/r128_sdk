/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#define TAG	"AudioHWAMP"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "audio_hw.h"
#include "AudioBase.h"

#include "local_audio_hw.h"

#include <audio_amp.h>

struct amp_attr {
	void *handle;
	audio_hw_t *ahw;
};

#ifdef CONFIG_DRIVER_SYSCONFIG
static struct ahw_params *g_amp_params[2] = {NULL, NULL};
#endif

static int amp_ahw_init(audio_hw_t *ahw)
{
	struct amp_attr *aa;
	as_pcm_config_t *pcm_config;

	aa = as_alloc(sizeof(struct amp_attr));
	if (!aa) {
		_err("no memory");
		goto err;
	}

	aa->handle = NULL;
	aa->ahw = ahw;

	audio_hw_set_private_data(ahw, aa);

	pcm_config = audio_hw_pcm_config(ahw);
#ifdef CONFIG_DRIVER_SYSCONFIG
	pcm_config = ahw_params_init("amp", g_amp_params, ahw);
#else
	if (!audio_hw_stream(ahw)) {
		/* playback */
		pcm_config->rate = AHW_DEFAULT_PB_RATE;
		pcm_config->channels = AHW_DEFAULT_PB_CHANNELS;
		pcm_config->format = AHW_DEFAULT_PB_FORMAT;
		pcm_config->period_frames = AHW_DEFAULT_PB_PERIOD_SIZE;
		pcm_config->periods = AHW_DEFAULT_PB_PERIODS;
	} else {
		/* capture */
		pcm_config->rate = AHW_DEFAULT_CAP_RATE;
		pcm_config->channels = AHW_DEFAULT_CAP_CHANNELS;
		pcm_config->format = AHW_DEFAULT_CAP_FORMAT;
		pcm_config->period_frames = AHW_DEFAULT_CAP_PERIOD_SIZE;
		pcm_config->periods = AHW_DEFAULT_CAP_PERIODS;

	}
#endif
	pcm_config->frame_bytes = format_to_bits(pcm_config->format) / 8 \
			* pcm_config->channels;
	pcm_config->buffer_frames = pcm_config->period_frames * pcm_config->periods;
	pcm_config->boundary = pcm_config->buffer_frames;
	while (pcm_config->boundary * 2 <= LONG_MAX - pcm_config->buffer_frames)
		pcm_config->boundary *= 2;

	return 0;
err:

	return -1;
}

static int amp_ahw_destroy(audio_hw_t *ahw)
{
	struct amp_attr *aa;

	aa = audio_hw_get_private_data(ahw);
	if (!aa)
		return 0;
	as_free(aa);
	audio_hw_set_private_data(ahw, NULL);

	return 0;
}

static int amp_ahw_open(struct amp_attr *aa)
{
	int ret;
	audio_hw_t *ahw = aa->ahw;
	as_pcm_config_t *pcm_config = audio_hw_pcm_config(ahw);
	struct audio_hw_elem *elem = audio_hw_elem_item(ahw);
	void *handle = NULL;
	const char *card = NULL;

	if (audio_hw_stream(ahw) == 0)
		card = elem->card_name_pb;
	else
		card = elem->card_name_cap;

	_debug("card:%s, stream:%d", card, audio_hw_stream(ahw));
	if (!audio_hw_stream(ahw))
		handle = AudioTrackCreateRM(card);
	else
		handle = AudioRecordCreateRM(card);
	if (!handle) {
		_err("at/ar create failed");
		return -1;
	}
	aa->handle = handle;

	if (!audio_hw_stream(ahw))
		ret = AudioTrackSetupRM(aa->handle, pcm_config->rate, pcm_config->channels,
					format_to_bits(pcm_config->format));
	else
		ret = AudioRecordSetupRM(aa->handle, pcm_config->rate, pcm_config->channels,
					format_to_bits(pcm_config->format));
	if (ret != 0)
		return ret;
	return 0;
}

static int amp_ahw_write(struct amp_attr *aa, struct rb_attr *rb)
{
	audio_hw_t *ahw = aa->ahw;
	tAudioTrackRM *atrm = aa->handle;
	as_pcm_config_t *pcm_config = audio_hw_pcm_config(ahw);
	snd_pcm_uframes_t frames = 0, bytes = 0, cont, offset;
	snd_pcm_sframes_t size;
	snd_pcm_uframes_t frames_count = 0;
	snd_pcm_uframes_t frames_total = pcm_config->period_frames;
	uint32_t frame_bytes = pcm_config->frame_bytes;
	void *data;

	while (1) {
		frames = frames_total - frames_count;
		offset = rb->hw_ptr % pcm_config->buffer_frames;
		cont = pcm_config->buffer_frames - offset;
		if (frames > cont)
			frames = cont;
		data = rb->rb_buf + offset * frame_bytes;
		bytes = frames * frame_bytes;
		size = AudioTrackWriteRM(atrm, data, bytes);
		/*printf("[%s] line:%d frames=%d, size=%d\n", __func__, __LINE__, frames, size);*/
		if (size < 0) {
			printf("AudioTrackWriteRM failed!!, return %ld\n", size);
			return size;
		} else if (size != bytes) {
			printf("AudioTrackWriteRM return %ld\n", size);
		}
		rb->hw_ptr += frames;
		if (rb->hw_ptr >= pcm_config->boundary)
			rb->hw_ptr -= pcm_config->boundary;
		/* fill silence data */
		memset(data, 0x0, size);
		frames_count += frames;
		if (frames_total == frames_count)
			break;
	}

	return frames_count;
}

static int amp_ahw_read(struct amp_attr *aa, struct rb_attr *rb)
{
	audio_hw_t *ahw = aa->ahw;
	tAudioRecordRM *arrm = aa->handle;
	as_pcm_config_t *pcm_config = audio_hw_pcm_config(ahw);
	snd_pcm_uframes_t frames = 0, bytes = 0, cont, offset;
	snd_pcm_sframes_t size;
	snd_pcm_uframes_t frames_count = 0;
	snd_pcm_uframes_t frames_total = pcm_config->period_frames;
	uint32_t frame_bytes = pcm_config->frame_bytes;
	void *data;

	/*printf("[%s] line:%d \n", __func__, __LINE__);*/
	while (1) {
		frames = frames_total - frames_count;
		offset = rb->hw_ptr % pcm_config->buffer_frames;
		cont = pcm_config->buffer_frames - offset;
		if (frames > cont)
			frames = cont;
		data = rb->rb_buf + offset * frame_bytes;
		bytes = frames * frame_bytes;
		/*printf("[%s] line:%d hw_ptr=%d, data=%p, ofs=%d\n", __func__, __LINE__,*/
			/*rb->hw_ptr, data, data - rb->rb_buf);*/
		size = AudioRecordReadRM(arrm, data, bytes);
		/*printf("[%s] line:%d data=%p, bytes=%u, size=%d\n", __func__, __LINE__,*/
			/*data, bytes, size);*/
		if (size < 0) {
			printf("AudioRecordReadRM failed!!, return %ld\n", size);
			return size;
		} else if (size != bytes) {
			printf("AudioRecordReadRM return %ld\n", size);
		}
		rb->hw_ptr += frames;
		if (rb->hw_ptr >= pcm_config->boundary)
			rb->hw_ptr -= pcm_config->boundary;
		frames_count += frames;
		if (frames_total == frames_count)
			break;
	}
	/*printf("[%s] line:%d frames_count=%u\n", __func__, __LINE__, frames_count);*/
	return frames_count;
}

static int amp_ahw_close(struct amp_attr *aa)
{
	int ret;
	audio_hw_t *ahw = aa->ahw;

	_debug("");
	if (!aa->handle)
		return 0;
	if (!audio_hw_stream(ahw))
		ret = AudioTrackDestroyRM(aa->handle);
	else
		ret = AudioRecordDestroyRM(aa->handle);
	if (ret < 0) {
		_err("at/ar destroy failed:%d", ret);
		return -1;
	}
	aa->handle = NULL;
	return 0;
}

/*
 * amp ahw
 * instance:	AUDIO_HW_TYPE_AMP
 * ops:		amp audio api
 * */
static struct audio_hw_ops amp_ops = {
	.ahw_open = (ahw_func)amp_ahw_open,
	.ahw_read = (ahw_func_xfer)amp_ahw_read,
	.ahw_write = (ahw_func_xfer)amp_ahw_write,
	.ahw_close = (ahw_func)amp_ahw_close,
	.ahw_init = (ahw_func)amp_ahw_init,
	.ahw_destroy = (ahw_func)amp_ahw_destroy,
};

struct audio_hw_elem g_amp_ahw = {
	.name = "amp",
	.instance = AUDIO_HW_TYPE_AMP,
	.card_name_pb = "playback",
	.card_name_cap = "capture",
	.ops = &amp_ops,
};

static struct audio_hw_ops amp_pb_ops = {
	.ahw_open = (ahw_func)amp_ahw_open,
	.ahw_write = (ahw_func_xfer)amp_ahw_write,
	.ahw_close = (ahw_func)amp_ahw_close,
	.ahw_init = (ahw_func)amp_ahw_init,
	.ahw_destroy = (ahw_func)amp_ahw_destroy,
};

struct audio_hw_elem g_amp_pb_ahw = {
	.name = "amp-pb",
	.instance = AUDIO_HW_TYPE_AMP + 1,
	.card_name_pb = "playback",
	.card_name_cap = NULL,
	.ops = &amp_pb_ops,
};

static struct audio_hw_ops amp_cap_ops = {
	.ahw_open = (ahw_func)amp_ahw_open,
	.ahw_read = (ahw_func_xfer)amp_ahw_read,
	.ahw_close = (ahw_func)amp_ahw_close,
	.ahw_init = (ahw_func)amp_ahw_init,
	.ahw_destroy = (ahw_func)amp_ahw_destroy,
};

struct audio_hw_elem g_amp_cap_ahw = {
	.name = "amp-cap",
	.instance = AUDIO_HW_TYPE_AMP + 2,
	.card_name_pb = NULL,
	.card_name_cap = "capture",
	.ops = &amp_cap_ops,
};
