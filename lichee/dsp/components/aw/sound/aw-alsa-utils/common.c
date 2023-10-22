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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <FreeRTOS.h>
#include <task.h>
#include <aw-alsa-lib/pcm.h>
#include <aw-alsa-lib/control.h>
#include "common.h"

unsigned int g_verbose = 0;

int set_param(snd_pcm_t *handle, snd_pcm_format_t format,
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

	if (g_verbose)
		snd_pcm_dump(handle);

	return ret;
}

void xrun(snd_pcm_t *handle)
{
	int ret;

	printf("Xrun...\n");
	ret = snd_pcm_prepare(handle);
	if (ret < 0) {
		printf("prepare failed in xrun. return %d\n", ret);
	}
}

void do_pause(snd_pcm_t *handle)
{
	int ret = 0;
	ret = snd_pcm_pause(handle, 1);
	if (ret < 0)
		printf("pause failed!, return %d\n", ret);
	//sleep(5);
	vTaskDelay(pdMS_TO_TICKS(5 * 1000));
	ret = snd_pcm_pause(handle, 0);
	if (ret < 0)
		printf("pause release failed!, return %d\n", ret);
}

void do_other_test(snd_pcm_t *handle)
{
	do_pause(handle);
	return;
}

int pcm_read(snd_pcm_t *handle, const char *data, snd_pcm_uframes_t frames_total,
		unsigned int frame_bytes)
{
	int ret = 0;
	snd_pcm_sframes_t size;
	snd_pcm_uframes_t frames_loop = 512;
	snd_pcm_uframes_t frames_count = 0;
	snd_pcm_uframes_t frames = 0;
	unsigned int offset = 0;

	if ((handle == NULL) || (data == NULL) || (frames_total == 0)) {
		printf("pcm_read params is null.\n");
		return -EFAULT;
	}

	while (1) {
		if ((frames_total - frames_count) < frames_loop)
			frames = frames_total - frames_count;
		if (frames == 0)
			frames = frames_loop;
		/*usleep(500000);*/
		/*printf("snd_pcm_readi %ld frames\n", frames);*/
		size = snd_pcm_readi(handle, (void *)(data + offset), frames);
		if (size < 0)
			printf("snd_pcm_readi return %ld\n", size);
		if (size == -EAGAIN) {
			/* retry */
			//usleep(10000);
			vTaskDelay(pdMS_TO_TICKS(10));
			continue;
		} else if (size == -EPIPE) {
			xrun(handle);
			continue;
		} else if (size == -ESTRPIPE) {

			continue;
		} else if (size < 0) {
			printf("-----snd_pcm_readi failed!!, return %ld\n", size);
			ret = (int)size;
			goto err_func;
		}
		offset += (size * frame_bytes);
		frames_count += size;
		frames -= size;
		if (frames_total == frames_count)
			break;
		/*printf("frames_count = %ld, frames_total = %ld\n", frames_count, frames_total);*/
	}
err_func:
	return frames_count > 0 ? frames_count : ret;
}

/*
 * arg0: arecord
 * arg1: card
 * arg2: format
 * arg3: rate
 * arg4: channels
 * arg5: data
 * arg6: len
 */
int arecord_data(const char *card_name, snd_pcm_format_t format, unsigned int rate,
		unsigned int channels, const void *data, unsigned int datalen)
{
	int ret = 0;
	unsigned long long i = 0;
	snd_pcm_t *handle;
	int mode = 0;
	unsigned int loop_count = 1;
	snd_pcm_uframes_t period_frames = 1024, buffer_frames = 4096;

	printf("dump args:\n");
	printf("card:      %s\n", card_name);
	printf("format:    %u\n", format);
	printf("rate:      %u\n", rate);
	printf("channels:  %u\n", channels);
	printf("data:      %p\n", data);
	printf("datalen:   %u\n", datalen);

	/* open card */
	ret = snd_pcm_open(&handle, card_name, SND_PCM_STREAM_CAPTURE, mode);
	if (ret < 0) {
		printf("audio open error:%d\n", ret);
		return -1;
	}

	ret = set_param(handle, format, rate, channels, period_frames, buffer_frames);
	if (ret < 0) {
		printf("audio set param error:%d\n", ret);
		goto err_func;
	}

	do {
		printf("pcm_read start...\n");
		ret = pcm_read(handle, data,
			snd_pcm_bytes_to_frames(handle, datalen),
			snd_pcm_frames_to_bytes(handle, 1));
		if (ret < 0) {
			printf("playback error:%d\n", ret);
			goto err_func;
		}
	} while (++i < loop_count);

	ret = snd_pcm_drop(handle);
	if (ret < 0)
		printf("stop failed!, return %d\n", ret);

err_func:
	/* close card */
	ret = snd_pcm_close(handle);
	if (ret < 0) {
		printf("audio close error:%d\n", ret);
		return ret;
	}
	return ret;
}

int pcm_write(snd_pcm_t *handle, char *data, snd_pcm_uframes_t frames_total,
		unsigned int frame_bytes)
{
	snd_pcm_sframes_t size;
	snd_pcm_uframes_t frames_loop = 512;
	snd_pcm_uframes_t frames_count = 0;
	snd_pcm_uframes_t frames = 0;

	if ((handle == NULL) || (data == NULL) || (frames_total == 0)) {
		printf("pcm_write params is null.\n");
		return -EFAULT;
	}

	while (1) {
		if ((frames_total - frames_count) < frames_loop)
			frames = frames_total - frames_count;
		if (frames == 0)
			frames = frames_loop;
		/*usleep(500000);*/
		size = snd_pcm_writei(handle, data, frames);
		if (size != frames) {
			printf("snd_pcm_writei return %ld\n", size);
		}
		if (size == -EAGAIN) {
			//usleep(10000);
			vTaskDelay(pdMS_TO_TICKS(10));
			continue;
		} else if (size == -EPIPE) {
			xrun(handle);
			continue;
		} else if (size == -ESTRPIPE) {
			continue;
		} else if (size < 0) {
			printf("-----snd_pcm_writei failed!!, return %ld\n", size);
			return size;
		}
		data += (size * frame_bytes);
		frames_count += size;
		frames -= size;
		if (frames_total == frames_count)
			break;
		/*printf("frames_count = %ld, frames_total = %ld\n", frames_count, frames_total);*/
	}

	return frames_count;
}

/*
 * arg0: aplay
 * arg1: card
 * arg2: format
 * arg3: rate
 * arg4: channels
 * arg5: data
 * arg6: len
 */
int aplay_data(const char *card_name, snd_pcm_format_t format, unsigned int rate,
			unsigned int channels, const char *data,
			unsigned int datalen, unsigned int loop_count)
{
	int ret = 0;
	snd_pcm_t *handle;
	int mode = 0;
	unsigned long long i = 0;
	snd_pcm_uframes_t period_frames = 1024, buffer_frames = 4096;

	printf("dump args:\n");
	printf("card:	     %s\n", card_name);
	printf("format:      %u\n", format);
	printf("rate:	     %u\n", rate);
	printf("channels:    %u\n", channels);
	printf("data:	     %p\n", data);
	printf("datalen:     %u\n", datalen);
	printf("period_size: %lu\n", period_frames);
	printf("buffer_size: %lu\n", buffer_frames);

	/* open card */
	ret = snd_pcm_open(&handle, card_name, SND_PCM_STREAM_PLAYBACK, mode);
	if (ret < 0) {
		printf("audio open error:%d\n", ret);
		return -1;
	}

	ret = set_param(handle, format, rate, channels, period_frames, buffer_frames);
	if (ret < 0) {
		printf("audio set param error:%d\n", ret);
		goto err_func;
	}

	i = 0;
	do {
		ret = pcm_write(handle, (char *)data,
			snd_pcm_bytes_to_frames(handle, datalen),
			snd_pcm_frames_to_bytes(handle, 1));
		if (ret < 0) {
			printf("pcm_write error:%d\n", ret);
			goto err_func;
		}
	} while (++i < loop_count);

	ret = snd_pcm_drain(handle);
	if (ret < 0)
		printf("stop failed!, return %d\n", ret);
err_func:
	/* close card */
	ret = snd_pcm_close(handle);
	if (ret < 0) {
		printf("audio close error:%d\n", ret);
		return ret;
	}
	return ret;
}

audio_mgr_t *audio_mgr_create(void)
{
	audio_mgr_t *audio_mgr = NULL;

	audio_mgr = malloc(sizeof(audio_mgr_t));
	if (!audio_mgr) {
		printf("no memory\n");
		return NULL;
	}
	memset(audio_mgr, 0, sizeof(audio_mgr_t));
	audio_mgr->format = SND_PCM_FORMAT_S16_LE;
	audio_mgr->rate = 16000;
	audio_mgr->channels = 2;
	audio_mgr->period_size = 1024;
	audio_mgr->buffer_size = 8192;
	audio_mgr->chunk_size = audio_mgr->period_size;
	return audio_mgr;
}

void audio_mgr_dump_args(audio_mgr_t *audio_mgr)
{
	if (!audio_mgr) {
		printf(AW_ALSA_LOG_COLOR_RED"audio_mgr is null.\n"
			AW_ALSA_LOG_COLOR_NONE);
		return;
	}
	printf(AW_ALSA_LOG_COLOR_BLUE"format:           %u\n"
		AW_ALSA_LOG_COLOR_NONE, audio_mgr->format);
	printf(AW_ALSA_LOG_COLOR_BLUE"rate:             %u\n"
		AW_ALSA_LOG_COLOR_NONE, audio_mgr->rate);
	printf(AW_ALSA_LOG_COLOR_BLUE"channels:         %u\n"
		AW_ALSA_LOG_COLOR_NONE, audio_mgr->channels);
	printf(AW_ALSA_LOG_COLOR_BLUE"period_size:      %lu\n"
		AW_ALSA_LOG_COLOR_NONE, audio_mgr->period_size);
	printf(AW_ALSA_LOG_COLOR_BLUE"buffer_size:      %lu\n"
		AW_ALSA_LOG_COLOR_NONE, audio_mgr->buffer_size);
	printf(AW_ALSA_LOG_COLOR_BLUE"frame_bytes:      %lu\n"
		AW_ALSA_LOG_COLOR_NONE, audio_mgr->frame_bytes);
	printf(AW_ALSA_LOG_COLOR_BLUE"chunk_size:       %lu\n"
		AW_ALSA_LOG_COLOR_NONE, audio_mgr->chunk_size);
	printf(AW_ALSA_LOG_COLOR_BLUE"in_aborting:      %u\n"
		AW_ALSA_LOG_COLOR_NONE, audio_mgr->in_aborting);
	printf(AW_ALSA_LOG_COLOR_BLUE"capture_duration: %u\n"
		AW_ALSA_LOG_COLOR_NONE, audio_mgr->capture_duration);
}

void audio_mgr_release(audio_mgr_t *mgr)
{
	if (!mgr) {
		printf("%s: mgr null !\n", __func__);
		return;
	}
	free(mgr);
}

int amixer_sset_enum_ctl(const char *card_name, const char *ctl_name,
			const char *ctl_val)
{
	int ret = 0;
	int i= 0;
	snd_ctl_info_t info;

	if ((card_name == NULL) || (!strcmp(card_name, "default")))
		card_name = "audiocodec";

	ret = snd_ctl_get(card_name, ctl_name, &info);
	if (ret < 0) {
		return ret;
	}

	if (info.type == SND_CTL_ELEM_TYPE_ENUMERATED) {
		for (i = 0; i < info.items; i++) {
			if (!strcmp(info.texts[i], ctl_val)) {
				ret = snd_ctl_set(card_name, ctl_name, i);
				if (ret < 0) {
					return ret;
				}
				break;
			}
		}
	}
	return 0;
}

void FUNCTION_THREAD_STOP_LINE_PRINTF(const char *string)
{
	printf(AW_ALSA_LOG_COLOR_YELLOW
		"------> %s Stop <------\n"
		AW_ALSA_LOG_COLOR_NONE, string);
}

void FUNCTION_THREAD_LINE_PRINTF(const char *string, const unsigned int line)
{
	printf(AW_ALSA_LOG_COLOR_YELLOW
		"------> %s %u <------\n"
		AW_ALSA_LOG_COLOR_NONE, string, line);
}

void FUNCTION_THREAD_START_LINE_PRINTF(const char *string)
{
	printf(AW_ALSA_LOG_COLOR_YELLOW
		"------> %s Start <------\n"
		AW_ALSA_LOG_COLOR_NONE, string);
}

