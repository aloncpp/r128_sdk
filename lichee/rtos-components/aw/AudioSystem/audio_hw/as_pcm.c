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
#define TAG	"AudioHWPCM"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "audio_hw.h"
#include "AudioBase.h"
#include <AudioSystem.h>

#include "local_audio_hw.h"

#include <aw-alsa-lib/pcm.h>
#include <aw-alsa-lib/control.h>
#include <aw-alsa-lib/common.h>


struct pcm_attr {
	snd_pcm_t *handle;
	audio_hw_t *ahw;
};

#ifdef CONFIG_DRIVER_SYSCONFIG
static struct ahw_params *g_pcm_params[2] = {NULL, NULL};
#endif

static int set_param(snd_pcm_t *handle, snd_pcm_format_t format,
			unsigned int rate, unsigned int channels,
			snd_pcm_uframes_t period_size,
			snd_pcm_uframes_t buffer_size)
{
	int ret = 0;
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *sw_params;

	/* HW params */
	snd_pcm_hw_params_alloca(&params);
	ret =  snd_pcm_hw_params_any(handle, params);
	if (ret < 0) {
		printf("no configurations available\n");
		return ret;
	}
	snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	ret = snd_pcm_hw_params_set_format(handle, params, format);
	ret = snd_pcm_hw_params_set_channels(handle, params, channels);
	ret = snd_pcm_hw_params_set_rate(handle, params, rate, 0);
	ret = snd_pcm_hw_params_set_period_size(handle, params, period_size, 0);
	ret = snd_pcm_hw_params_set_buffer_size(handle, params, buffer_size);
	ret = snd_pcm_hw_params(handle, params);
	if (ret < 0) {
		printf("Unable to install hw prams!\n");
		return ret;
	}

	snd_pcm_hw_params_get_period_size(params, &period_size, NULL);
	snd_pcm_hw_params_get_buffer_size(params, &buffer_size);

	/* SW params */
	snd_pcm_sw_params_alloca(&sw_params);
	snd_pcm_sw_params_current(handle, sw_params);
	if (snd_pcm_stream(handle) == SND_PCM_STREAM_CAPTURE) {
		snd_pcm_sw_params_set_start_threshold(handle, sw_params, 1);
	} else {
		snd_pcm_uframes_t boundary = 0;
		snd_pcm_sw_params_get_boundary(sw_params, &boundary);
		snd_pcm_sw_params_set_start_threshold(handle, sw_params, buffer_size);
		/* set silence size, in order to fill silence data into ringbuffer */
		snd_pcm_sw_params_set_silence_size(handle, sw_params, boundary);
	}
	snd_pcm_sw_params_set_stop_threshold(handle, sw_params, buffer_size);
	snd_pcm_sw_params_set_avail_min(handle, sw_params, period_size);
	ret = snd_pcm_sw_params(handle ,sw_params);
	if (ret < 0) {
		printf("Unable to install sw prams!\n");
		return ret;
	}

	return ret;
}

static int pcm_ahw_init(audio_hw_t *ahw)
{
	struct pcm_attr *pa;
	as_pcm_config_t *pcm_config;

	pa = as_alloc(sizeof(struct pcm_attr));
	if (!pa) {
		_err("no memory");
		goto err;
	}

	pa->handle = NULL;
	pa->ahw = ahw;
	/*_debug("set ahw(%p) into pcm_attr", ahw);*/
	audio_hw_set_private_data(ahw, pa);

#ifdef CONFIG_DRIVER_SYSCONFIG
	pcm_config = ahw_params_init("pcm", g_pcm_params, ahw);
#else
	pcm_config = audio_hw_pcm_config(ahw);
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

#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_EQ
	int ret_eq;
	ret_eq = eq_hw_init();
	if (ret_eq != 0) {
		_err("eq_hw_init failed");
	}
#endif

#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_DRC
	int ret_drc;
	ret_drc = drc_hw_init();
	if (ret_drc != 0) {
		_err("drc_hw_init failed");
	}
#endif

	return 0;
err:

	return -1;
}

static int pcm_ahw_destroy(audio_hw_t *ahw)
{
	struct pcm_attr *pa;

	pa = audio_hw_get_private_data(ahw);
	if (!pa)
		return 0;
	as_free(pa);
	audio_hw_set_private_data(ahw, NULL);

#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_EQ
	eq_hw_destroy();
#endif

#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_DRC
	drc_hw_destroy();
#endif

	return 0;
}

static int pcm_ahw_open(struct pcm_attr *pa)
{
	int ret;
	audio_hw_t *ahw = pa->ahw;
	as_pcm_config_t *pcm_config = audio_hw_pcm_config(ahw);
	struct audio_hw_elem *elem = audio_hw_elem_item(ahw);
	const char *card = NULL;

	if (audio_hw_stream(ahw) == 0)
		card = elem->card_name_pb;
	else
		card = elem->card_name_cap;
#ifdef CONFIG_SND_PLATFORM_SUNXI_MAD
	if (audio_hw_stream(ahw) != 0) {
		snd_ctl_set("audiocodecadc", "bind mad function", 1);
		snd_ctl_set("audiocodecadc", "mad standby channel sel function", 2);
	}
#endif

	_debug("card:%s, stream:%d", card, audio_hw_stream(ahw));
	ret = snd_pcm_open(&pa->handle, card, audio_hw_stream(ahw), 0);
	if (ret < 0) {
		_err("pcm open failed:%d", ret);
		return -1;
	}

	ret = set_param(pa->handle, pcm_config->format,
		pcm_config->rate, pcm_config->channels,
		pcm_config->period_frames,
		pcm_config->period_frames * pcm_config->periods);
	if (ret != 0)
		return ret;
	return 0;
}

static int pcm_ahw_write(struct pcm_attr *pa, struct rb_attr *rb)
{
	audio_hw_t *ahw = pa->ahw;
	as_pcm_config_t *pcm_config = audio_hw_pcm_config(ahw);
	snd_pcm_t *handle = pa->handle;

	snd_pcm_sframes_t size;
	snd_pcm_uframes_t frames_total = pcm_config->period_frames;
	snd_pcm_uframes_t frames_loop = 480;
	snd_pcm_uframes_t frames_count = 0;
	snd_pcm_uframes_t frames = 0, cont, offset;
	uint32_t frame_bytes = snd_pcm_frames_to_bytes(handle, 1);
	void *data;

	while (1) {
		if ((frames_total - frames_count) < frames_loop)
			frames = frames_total - frames_count;
		if (!frames)
			frames = frames_loop;
		cont = pcm_config->buffer_frames -
			rb->hw_ptr % pcm_config->buffer_frames;
		if (frames > cont)
			frames = cont;
		offset = rb->hw_ptr % pcm_config->buffer_frames;
		data = rb->rb_buf + offset * frame_bytes;
		/*_debug("frames=%lu, count=%lu, offset:%u", frames, cont, offset);*/
		size = snd_pcm_writei(handle, data, frames);
		if (size != frames) {
			printf("snd_pcm_writei return %ld\n", size);
		}
		if (size == -EAGAIN) {
			as_msleep(10);
			continue;
		} else if (size == -EPIPE) {
			_info("ahw instance%d underrun...", audio_hw_instance(ahw));
			snd_pcm_prepare(handle);
			continue;
		} else if (size == -ESTRPIPE) {

			continue;
		} else if (size < 0) {
			printf("-----snd_pcm_writei failed!!, return %ld\n", size);
			return size;
		}
		rb->hw_ptr += size;
		if (rb->hw_ptr >= pcm_config->boundary)
			rb->hw_ptr -= pcm_config->boundary;
		/*_debug("update mixer_ptr:%u, ofs:%u", rb->hw_ptr, rb->hw_ptr % as_pcm->pcm_config.buffer_frames);*/
		/* fill silence data */
		memset(data, 0x0, size * frame_bytes);
		frames_count += size;
		frames -= size;
		if (frames_total == frames_count)
			break;
	}
	return frames_count;
}

#ifdef CONFIG_SND_SUNXI_MAD_DEBUG
#include <console.h>
static int do_mad_standby = 0;
static int cmd_as_mad(int argc, char *argv[])
{
	if (argc == 2)
		do_mad_standby = atoi(argv[1]);

	printf("do_mad_standby = %d\n", do_mad_standby);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_as_mad, as_mad, AudioSystem mad test);

extern hal_sem_t mad_sleep;
extern int sunxi_mad_schd_timeout(hal_sem_t semaphore, long ms);
#endif


static int pcm_ahw_read(struct pcm_attr *pa, struct rb_attr *rb)
{
	audio_hw_t *ahw = pa->ahw;
	as_pcm_config_t *pcm_config = audio_hw_pcm_config(ahw);
	snd_pcm_t *handle = pa->handle;

	snd_pcm_sframes_t size;
	snd_pcm_uframes_t frames_total = pcm_config->period_frames;
	snd_pcm_uframes_t frames_loop = 320;
	snd_pcm_uframes_t frames_count = 0;
	snd_pcm_uframes_t frames = 0, cont, offset;
	uint32_t frame_bytes = snd_pcm_frames_to_bytes(handle, 1);
	void *data;

	while (1) {
		if ((frames_total - frames_count) < frames_loop)
			frames = frames_total - frames_count;
		if (!frames)
			frames = frames_loop;
		cont = pcm_config->buffer_frames -
			rb->hw_ptr % pcm_config->buffer_frames;
		if (frames > cont)
			frames = cont;
		offset = rb->hw_ptr % pcm_config->buffer_frames;
		data = rb->rb_buf + offset * frame_bytes;
		/*_debug("frames=%lu, count=%lu, offset:%u", frames, cont, offset);*/
		size = snd_pcm_readi(handle, data, frames);
		if (size != frames) {
			printf("snd_pcm_readi return %ld\n", size);
		}
		if (size == -EAGAIN) {
			as_msleep(10);
			continue;
		} else if (size == -EPIPE) {
			_info("ahw instance%d overrun...", audio_hw_instance(ahw));
			snd_pcm_prepare(handle);
			continue;
		} else if (size == -ESTRPIPE) {

			continue;
		} else if (size < 0) {
			printf("-----snd_pcm_writei failed!!, return %ld\n", size);
			return size;
		}
		rb->hw_ptr += size;
		if (rb->hw_ptr >= pcm_config->boundary)
			rb->hw_ptr -= pcm_config->boundary;
		/*_debug("update mixer_ptr:%u, ofs:%u", mt->mt_rb.hw_ptr, mt->mt_rb.hw_ptr % mt->mt_pcm.pcm_config.buffer_frames);*/
		frames_count += size;
		frames -= size;
#ifdef CONFIG_SND_SUNXI_MAD_DEBUG
		if (do_mad_standby) {
			AudioSystemSetSchdTimeout(1, -1);
			printf("[%s] line:%d enter mad standby\n", __func__, __LINE__);
			snd_pcm_drop(handle);
			snd_ctl_set("audiocodecadc", "mad standby control", 1);
			sunxi_mad_schd_timeout(mad_sleep, 1000*20);
			printf("[%s] line:%d exit mad standby\n", __func__, __LINE__);
			snd_pcm_prepare(handle);
			AudioSystemSetSchdTimeout(1, 5000);
			do_mad_standby = 0;
		}
#endif
		if (frames_total == frames_count)
			break;
		/*_debug("frames_count = %ld, frames_total = %ld\n", frames_count, frames_total);*/
	}

	return frames_count;
}

static int pcm_ahw_close(struct pcm_attr *pa)
{
	int ret;

	_debug("");
	if (!pa->handle)
		return 0;
	ret = snd_pcm_close(pa->handle);
	if (ret < 0) {
		_err("pcm close failed:%d", ret);
		return -1;
	}
	pa->handle = NULL;
	return 0;
}



#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_MULTI_PCM
#define MULTI_CARD_NUM		(2)

#include "pcm_merge.c"

static struct multi_card_attr {
	const char *name;
	int16_t channels;
	int16_t rate;

} g_multi_card_attr[MULTI_CARD_NUM] = {
	{
		.name		= "hw:audiocodecadc",
		.channels	= 2,
		.rate		= 16000,
	},
	{
		.name		= "hw:snddmic",
		.channels	= 2,
		.rate		= 16000,
	},
};

static int g_channel_map[] = {
	0,	/* audiocodec ch0 --> ch0 */
	1,	/* audiocodec ch1 --> ch1 */
	2,	/* snddmic    ch0 --> ch2 */
	3,	/* snddmic    ch1 --> ch3 */
};

struct multi_pcm_attr {
	audio_hw_t *ahw;
	void *handle;
};

#define AHW_MULTI_CAP_RATE		(16000)
#define AHW_MULTI_CAP_CHANNELS		(4)
#define AHW_MULTI_CAP_FORMAT		(SND_PCM_FORMAT_S16_LE)
#define AHW_MULTI_CAP_PERIOD_SIZE	(320)
#define AHW_MULTI_CAP_PERIODS		(4)

static int pcm_ahw_multi_init(audio_hw_t *ahw)
{
	struct multi_pcm_attr *mpa;
	as_pcm_config_t *pcm_config;

	mpa = as_alloc(sizeof(struct multi_pcm_attr));
	if (!mpa) {
		_err("no memory");
		return -1;
	}

	mpa->ahw = ahw;
	audio_hw_set_private_data(ahw, mpa);


	pcm_merge_create(g_multi_card_attr[0].name, g_multi_card_attr[0].channels,
			g_multi_card_attr[1].name, g_multi_card_attr[1].channels);

	pcm_merge_set_channel_map(g_channel_map, ARRAY_SIZE(g_channel_map));

	pcm_config = audio_hw_pcm_config(ahw);
	if (!audio_hw_stream(ahw))
		goto err;
	/* support capture onlu */
	/* capture */
	pcm_config->rate = AHW_MULTI_CAP_RATE;
	pcm_config->channels = AHW_MULTI_CAP_CHANNELS;
	pcm_config->format = AHW_MULTI_CAP_FORMAT;
	pcm_config->period_frames = AHW_MULTI_CAP_PERIOD_SIZE;
	pcm_config->periods = AHW_MULTI_CAP_PERIODS;
	pcm_config->frame_bytes = format_to_bits(pcm_config->format) / 8 \
			* pcm_config->channels;
	pcm_config->buffer_frames = pcm_config->period_frames * pcm_config->periods;
	pcm_config->boundary = pcm_config->buffer_frames;
	while (pcm_config->boundary * 2 <= LONG_MAX - pcm_config->buffer_frames)
		pcm_config->boundary *= 2;

	return 0;
err:
	if (mpa)
		as_free(mpa);
	audio_hw_set_private_data(ahw, NULL);

	return -1;
}

static int pcm_ahw_multi_destroy(audio_hw_t *ahw)
{
	struct multi_pcm_attr *mpa;

	mpa = audio_hw_get_private_data(ahw);
	if (!mpa)
		return 0;
	as_free(mpa);
	audio_hw_set_private_data(ahw, NULL);

	return 0;
}

static int pcm_ahw_multi_open(struct multi_pcm_attr *mpa)
{
	int ret;
	audio_hw_t *ahw = mpa->ahw;
	as_pcm_config_t *pcm_config = audio_hw_pcm_config(ahw);

	mpa->handle = pcm_merge_open();
	if (!mpa->handle)
		return -1;
	ret = pcm_merge_hw_params(pcm_config->format, pcm_config->rate, pcm_config->channels,
				pcm_config->period_frames, pcm_config->buffer_frames);
	if (ret != 0)
		return ret;
	return 0;
}

static int pcm_ahw_multi_read(struct multi_pcm_attr *mpa, struct rb_attr *rb)
{
	audio_hw_t *ahw = mpa->ahw;
	as_pcm_config_t *pcm_config = audio_hw_pcm_config(ahw);
	snd_pcm_uframes_t offset, frames = pcm_config->period_frames;
	void *data;

	offset = rb->hw_ptr % pcm_config->buffer_frames;
	data = rb->rb_buf + offset * pcm_config->frame_bytes;
	pcm_merge_read(data, frames * pcm_config->frame_bytes);
	rb->hw_ptr += frames;
	if (rb->hw_ptr >= pcm_config->boundary)
		rb->hw_ptr -= pcm_config->boundary;

	return frames;
}

static int pcm_ahw_multi_close(struct multi_pcm_attr *mpa)
{
	if (!mpa)
		return -1;
	pcm_merge_close(mpa->handle);
	return 0;
}

#endif

#if 0
/*
 * default pcm ahw
 * instance:	AUDIO_HW_TYPE_PCM
 * ops:		support pcm_write,pcm_read
 * */
static struct audio_hw_ops pcm_ops = {
	.ahw_open = (ahw_func)pcm_ahw_open,
	.ahw_read = (ahw_func_xfer)pcm_ahw_read,
	.ahw_write = (ahw_func_xfer)pcm_ahw_write,
	.ahw_close = (ahw_func)pcm_ahw_close,
	.ahw_init = (ahw_func)pcm_ahw_init,
	.ahw_destroy = (ahw_func)pcm_ahw_destroy,
};

struct audio_hw_elem g_pcm_ahw = {
	.name = "default",
	.instance = AUDIO_HW_TYPE_PCM,
	.card_name_pb = "hw:audiocodecdac",
	.card_name_cap = "hw:audiocodecadc",
	.ops = &pcm_ops,
};

#endif

/*
 * playback ahw
 * instance:	AUDIO_HW_TYPE_PCM_PB_ONLY
 * ops:		only support pcm_write
 * */
static struct audio_hw_ops pcm_pb_ops = {
	.ahw_open = (ahw_func)pcm_ahw_open,
	.ahw_read = NULL,
	.ahw_write = (ahw_func_xfer)pcm_ahw_write,
	.ahw_close = (ahw_func)pcm_ahw_close,
	.ahw_init = (ahw_func)pcm_ahw_init,
	.ahw_destroy = (ahw_func)pcm_ahw_destroy,
};

struct audio_hw_elem g_pcm_pb_ahw = {
	.name = "playback",
	.instance = AUDIO_HW_TYPE_PCM_PB_ONLY,
#if defined(CONFIG_SND_CODEC_AUDIOCODEC_DAC)
	.card_name_pb = "hw:audiocodecdac",
#elif defined(CONFIG_SND_PLATFORM_SUNXI_DAUDIO0)
	.card_name_pb = "hw:snddaudio0",
#else
	.card_name_pb = "hw:audiocodec",
#endif
	.card_name_cap = NULL,
	.ops = &pcm_pb_ops,
};

/*
 * capture ahw
 * instance:	AUDIO_HW_TYPE_PCM_CAP_ONLY
 * ops:		only support pcm_read
 * */
static struct audio_hw_ops pcm_cap_ops = {
	.ahw_open = (ahw_func)pcm_ahw_open,
	.ahw_read = (ahw_func_xfer)pcm_ahw_read,
	.ahw_write = NULL,
	.ahw_close = (ahw_func)pcm_ahw_close,
	.ahw_init = (ahw_func)pcm_ahw_init,
	.ahw_destroy = (ahw_func)pcm_ahw_destroy,
};

struct audio_hw_elem g_pcm_cap_ahw = {
	.name = "capture",
	.instance = AUDIO_HW_TYPE_PCM_CAP_ONLY,
	.card_name_pb = NULL,
	.card_name_cap = "hw:audiocodecadc",
	.ops = &pcm_cap_ops,
};

#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_MULTI_PCM
/*
 * multicard ahw
 * instance:	AUDIO_HW_TYPE_PCM_MULTI
 * ops:		only support multi card read
 * */
static struct audio_hw_ops pcm_multi_ops = {
	.ahw_open = (ahw_func)pcm_ahw_multi_open,
	.ahw_read = (ahw_func_xfer)pcm_ahw_multi_read,
	.ahw_write = NULL,
	.ahw_close = (ahw_func)pcm_ahw_multi_close,
	.ahw_init = (ahw_func)pcm_ahw_multi_init,
	.ahw_destroy = (ahw_func)pcm_ahw_multi_destroy,
};

struct audio_hw_elem g_pcm_multi_ahw = {
	.name = "multicard",
	.instance = AUDIO_HW_TYPE_PCM_MULTI,
	.ops = &pcm_multi_ops,
};
#endif
