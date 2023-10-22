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
#include <stdlib.h>

#include "FreeRTOS_POSIX/unistd.h"

#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
#include "AudioRecord.h"
#include "AudioSystem.h"
#include "AudioTrack.h"
#else
#include <aw-alsa-lib/pcm.h>
#include <aw-alsa-lib/control.h>
#include <sound/card.h>
#endif

#include "sys_ctrl/sys_ctrl.h"
#include "kernel/os/os.h"
#include "bt_app_audio.h"
#include "xr_err.h"

#include "cmd_util.h"
#include "cmd_bt/unlock_buffer.h"
#include "cmd_bt/bt_atomic.h"

#define BT_APP_AUDIO_DEBUG(fmt, arg...)    printf("[BT_APP_AUDIO debug] <%s : %d> " fmt, __func__, __LINE__, ##arg)
#define BT_APP_AUDIO_ALERT(fmt, arg...)    printf("[BT_APP_AUDIO alert] <%s : %d> " fmt, __func__, __LINE__, ##arg)
#define BT_APP_AUDIO_ERROR(fmt, arg...)    printf("[BT_APP_AUDIO error] <%s : %d> " fmt, __func__, __LINE__, ##arg)

#define AUDIO_NAME "default"
#define BIT_DEEP   16

/* cache data before a2dp sink start */
#define BT_A2DP_CACHE_TIME         200

#define FIFO_BUFFER_SPEAKER_SIZE   (64 * 1024)
#define FIFO_BUFFER_RECODER_SIZE   (4 * 1024)

#define A2DP_SINK_THREAD_PROI      XR_OS_PRIORITY_NORMAL
#define HFP_IN_THREAD_PROI         XR_OS_PRIORITY_NORMAL
#define HFP_OUT_THREAD_PROI        XR_OS_PRIORITY_NORMAL

#define A2DP_SINK_THREAD_TASK_SIZE (1024 * 8)
#define HFP_IN_THREAD_TASK_SIZE    (1024 * 8)
#define HFP_OUT_THREAD_TASK_SIZE   (1024 * 8)

#define BT_INCOMING_PERIOD_TIME_MS 20
#define BT_OUTGOING_PERIOD_TIME_MS 10

#define TEST_DATA 1

#ifndef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
typedef struct {
	int stream;
	snd_pcm_format_t format;
	unsigned int channels, rate;
	snd_pcm_uframes_t period_size;
	snd_pcm_uframes_t buffer_size;
	snd_pcm_t *pcm;
} pcm_handle_t;

static pcm_handle_t a2dp_pcm_handle = {
	.stream      = SND_PCM_STREAM_PLAYBACK,
	.format      = SND_PCM_FORMAT_S16_LE,
	.channels    = 2,
	.rate        = 44100,
	.buffer_size = 1470 * 4,
	.period_size = 1470,
	.pcm = NULL,
};

static pcm_handle_t hfp_pcm_handle = {
	.stream      = SND_PCM_STREAM_PLAYBACK,
	.format      = SND_PCM_FORMAT_S16_LE,
	.channels    = 1,
	.rate        = 8000,
	.buffer_size = 512 * 4,
	.period_size = 512,
	.pcm = NULL,
};

static const pcm_handle_t hfp_mic_handle = {
	.stream      = SND_PCM_STREAM_CAPTURE,
	.format      = SND_PCM_FORMAT_S16_LE,
	.channels    = 1,
	.rate        = 8000,
	.buffer_size = 1024*4,
	.period_size = 1024,
	.pcm = NULL,
};
#else
typedef struct {
	unsigned int channels;
	unsigned int rate;
} pcm_handle_t;

static pcm_handle_t a2dp_pcm_handle = {
	.channels    = 2,
	.rate        = 44100,
};

static pcm_handle_t hfp_pcm_handle = {
	.channels   = 1,
	.rate       = 8000,
};

static pcm_handle_t hfp_mic_handle = {
	.channels   = 1,
	.rate       = 8000,
};

#endif

static char usr_media[50] = {0};
static char *media_list[] = {
	"/data/bt_audio.wav",
	usr_media,
};

#define BT_AV_SOURCE_FILE_NUM   (sizeof(media_list) / sizeof(media_list[0]))

static int media_file_index = 0;
static int s_media_state = APP_AV_MEDIA_STATE_IDLE;
static FILE *music_file = NULL;

typedef enum {
	BT_AUDIO_STATE_BIT_SPEAKER_ENABLE,
	BT_AUDIO_STATE_BIT_SPEAKER_THREAD_ENABLE,
	BT_AUDIO_STATE_BIT_SPEAKER_THREAD_SUSPEND,
	BT_AUDIO_STATE_BIT_SPEAKER_THREAD_CLOSE_DONE,
	BT_AUDIO_STATE_BIT_SPEAKER_THREAD_SETTING,

	BT_AUDIO_STATE_BIT_RECODER_ENABLE,
	BT_AUDIO_STATE_BIT_RECODER_THREAD_ENABLE,
	BT_AUDIO_STATE_BIT_RECODER_THREAD_SUSPEND,
	BT_AUDIO_STATE_BIT_RECODER_THREAD_CLOSE_DONE,
	BT_AUDIO_STATE_BIT_RECODER_THREAD_SETTING,
} bt_audio_state_bit;

typedef struct _bt_audio_ {
	bt_audio_type_t type;
	fifo_t fifo_incoming;//speaker
	fifo_t fifo_outgoing;//recorder
	char *fifo_incoming_buffer;//speaker
	char *fifo_outgoing_buffer;//recorder
	pcm_handle_t *pcm_handle;
	pcm_handle_t *mic_handle;
	bt_atomic_t audio_state;
	uint32_t cache_time;
	uint32_t cache_size;
} bt_audio_t;

static bt_audio_t *bt_audio_a2dp;
static bt_audio_t *bt_audio_hfp;
static XR_OS_Thread_t g_bt_audio_a2dp_thread;
static XR_OS_Thread_t g_bt_audio_hfp_i_thread;
static XR_OS_Thread_t g_bt_audio_hfp_o_thread;

static void bt_audio_incoming_thread(void* param);
static void bt_audio_outgoing_thread(void* param);

static int copy_pcm_setting(pcm_handle_t *dir,const pcm_handle_t *src)
{
#ifndef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
	dir->stream     = src->stream;
	dir->format     = src->format;
	dir->buffer_size    = src->buffer_size;
	dir->period_size    = src->period_size;
	dir->pcm            = src->pcm;
#endif
	dir->channels   = src->channels;
	dir->rate       = src->rate;

	return 0;
}

static bt_audio_t *create_audio_struct(bt_audio_type_t type)
{
	bt_audio_t* bt_audio_param;
	bt_audio_param = malloc(sizeof(bt_audio_t));
	if (bt_audio_param == NULL) {
		return NULL;
	}
	memset(bt_audio_param, 0, (sizeof(bt_audio_t)));
	bt_audio_param->type = type;
	bt_atomic_set_val(&bt_audio_param->audio_state, 0);

	if (bt_audio_param->type == BT_APP_AUDIO_TYPE_A2DP
		|| bt_audio_param->type == BT_APP_AUDIO_TYPE_HFP) {
		bt_audio_param->pcm_handle = malloc(sizeof(pcm_handle_t));
		bt_audio_param->fifo_incoming_buffer = malloc(FIFO_BUFFER_SPEAKER_SIZE);
		if (!bt_audio_param->pcm_handle ||
			!bt_audio_param->fifo_incoming_buffer) {
			goto play_create_fail;
		}
		copy_pcm_setting(bt_audio_param->pcm_handle, (bt_audio_param->type == BT_APP_AUDIO_TYPE_HFP) ?
		                         &hfp_pcm_handle : &a2dp_pcm_handle);
		fifo_init(&bt_audio_param->fifo_incoming, bt_audio_param->fifo_incoming_buffer, FIFO_BUFFER_SPEAKER_SIZE);
	}
	if (bt_audio_param->type == BT_APP_AUDIO_TYPE_HFP) {
		bt_audio_param->mic_handle = malloc(sizeof(pcm_handle_t));
		bt_audio_param->fifo_outgoing_buffer = malloc(FIFO_BUFFER_RECODER_SIZE);
		if (!bt_audio_param->mic_handle || !bt_audio_param->fifo_outgoing_buffer) {
			goto record_create_fail;
		}
		copy_pcm_setting(bt_audio_param->mic_handle, &hfp_mic_handle);
		fifo_init(&bt_audio_param->fifo_outgoing, bt_audio_param->fifo_outgoing_buffer, FIFO_BUFFER_RECODER_SIZE);
	}
	return bt_audio_param;

record_create_fail:
	if (bt_audio_param->fifo_outgoing_buffer)
		free(bt_audio_param->fifo_outgoing_buffer);
	if (bt_audio_param->mic_handle)
		free(bt_audio_param->mic_handle);

play_create_fail:
	if (bt_audio_param->fifo_incoming_buffer)
		free(bt_audio_param->fifo_incoming_buffer);
	if (bt_audio_param->pcm_handle)
		free(bt_audio_param->pcm_handle);

	if (bt_audio_param)
		free(bt_audio_param);

	return NULL;
}

static int destory_audio_struct(bt_audio_t* bt_audio_param)
{
	bt_atomic_set_val(&bt_audio_param->audio_state, 0);

	fifo_deinit(&bt_audio_param->fifo_incoming);
	if (bt_audio_param->fifo_incoming_buffer != NULL) {
		free(bt_audio_param->fifo_incoming_buffer);
		bt_audio_param->fifo_incoming_buffer = NULL;
	}
	if (bt_audio_param->pcm_handle != NULL) {
		free(bt_audio_param->pcm_handle);
		bt_audio_param->pcm_handle = NULL;
	}
	fifo_deinit(&bt_audio_param->fifo_outgoing);
	if (bt_audio_param->fifo_outgoing_buffer != NULL) {
		free(bt_audio_param->fifo_outgoing_buffer);
		bt_audio_param->fifo_outgoing_buffer = NULL;
	}
	if (bt_audio_param->mic_handle != NULL) {
		free(bt_audio_param->mic_handle);
		bt_audio_param->mic_handle = NULL;
	}

	if (bt_audio_param != NULL) {
		free(bt_audio_param);
	}

	return 0;
}

static int bt_app_audio_start(bt_audio_type_t type)//a2dp hfp etc
{
	if (type == BT_APP_AUDIO_TYPE_A2DP) {
		if (bt_audio_a2dp != NULL) {
			return -1;
		}
		bt_audio_a2dp = create_audio_struct(BT_APP_AUDIO_TYPE_A2DP);
		if (bt_audio_a2dp == NULL) {
			return -1;
		}

		memset(&g_bt_audio_a2dp_thread, 0, sizeof(g_bt_audio_a2dp_thread));
		bt_atomic_set_bit(&bt_audio_a2dp->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_ENABLE);
		XR_OS_ThreadCreate(&g_bt_audio_a2dp_thread, "a2dp_i_t", bt_audio_incoming_thread, bt_audio_a2dp, A2DP_SINK_THREAD_PROI, A2DP_SINK_THREAD_TASK_SIZE);
		bt_audio_a2dp->cache_time = BT_A2DP_CACHE_TIME;
		bt_audio_a2dp->cache_size = a2dp_pcm_handle.rate * a2dp_pcm_handle.channels * BIT_DEEP / 8 * bt_audio_a2dp->cache_time / 1000;
		bt_audio_a2dp->cache_size = (bt_audio_a2dp->cache_size > FIFO_BUFFER_SPEAKER_SIZE) ? FIFO_BUFFER_SPEAKER_SIZE : bt_audio_a2dp->cache_size;
	} else if (type == BT_APP_AUDIO_TYPE_HFP) {
		if (bt_audio_hfp != NULL) {
			return -1;
		}
		bt_audio_hfp = create_audio_struct(BT_APP_AUDIO_TYPE_HFP);
		if (bt_audio_hfp == NULL) {
			return -1;
		}
		bt_atomic_set_bit(&bt_audio_hfp->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_ENABLE);
		bt_atomic_set_bit(&bt_audio_hfp->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_ENABLE);

		memset(&g_bt_audio_hfp_o_thread, 0, sizeof(g_bt_audio_hfp_o_thread));
		memset(&g_bt_audio_hfp_i_thread, 0, sizeof(g_bt_audio_hfp_i_thread));
		XR_OS_ThreadCreate(&g_bt_audio_hfp_o_thread, "hfp_o_t", bt_audio_outgoing_thread, bt_audio_hfp, HFP_OUT_THREAD_PROI, HFP_OUT_THREAD_TASK_SIZE);
		XR_OS_ThreadCreate(&g_bt_audio_hfp_i_thread, "hfp_i_t", bt_audio_incoming_thread, bt_audio_hfp, HFP_IN_THREAD_PROI, HFP_IN_THREAD_TASK_SIZE);
		bt_audio_hfp->cache_time = 0;
		bt_audio_hfp->cache_size = 0;
	}

	return 0;
}

static int bt_app_audio_stop(bt_audio_t *param)
{
	int speaker_thread_enable = 0;
	int recoder_thread_enable = 0;
	if (bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_ENABLE)) {
		bt_atomic_clear_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_ENABLE);
		speaker_thread_enable = 1;
	}
	if (bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_ENABLE)) {
		bt_atomic_clear_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_ENABLE);
		recoder_thread_enable = 1;
	}
	if (speaker_thread_enable) {
		while (!bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_CLOSE_DONE)) {
			usleep(1000);
		}
	}
	if (recoder_thread_enable) {
		while (!bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_CLOSE_DONE)) {
			usleep(1000);
		}
	}
	destory_audio_struct(param);
	return 0;
}

#ifndef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
extern void xrun(snd_pcm_t *handle);
static int pcm_write(snd_pcm_t *_handle, char *data, snd_pcm_uframes_t frames_total, unsigned int frame_bytes)
{
	snd_pcm_sframes_t size;
	snd_pcm_uframes_t frames_loop = 400;
	snd_pcm_uframes_t frames_count = 0;
	snd_pcm_uframes_t frames = 0;
	while (1) {
		if ((frames_total - frames_count) < frames_loop)
			frames = frames_total - frames_count;
		if (frames == 0)
			frames = frames_loop;
		size = snd_pcm_writei(_handle, data, frames);
		if (size != frames) {
			BT_APP_AUDIO_ERROR("snd_pcm_writei return %ld\n", size);
		}
		if (size == -EAGAIN) {
			usleep(1000);
			continue;
		} else if (size == -EPIPE) {
			xrun(_handle);
			continue;
		} else if (size == -ESTRPIPE) {
			continue;
		} else if (size < 0) {
			BT_APP_AUDIO_ERROR("snd_pcm_writei failed!!, return %ld\n", size);
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

static int bt_app_audio_write(bt_audio_t *param, const uint8_t *data, uint32_t len)
{
	int ret = 0;
	//printf("d : bt_app_audio_write:%d\n",len);
	//ret = snd_pcm_write(&r328_pcm_config,(uint8_t *)data, len);
	if (param->pcm_handle->pcm == NULL) {
		BT_APP_AUDIO_ERROR("bt_app_audio_write handle is null\n");
		return -1;
	}
	//printf("bt_app_audio_write pcm_write\n");
	//printf("len is %d  is %d\n",snd_pcm_bytes_to_frames(handle, len),snd_pcm_frames_to_bytes(handle, 1));
	ret = pcm_write(param->pcm_handle->pcm, (char *)data,
			snd_pcm_bytes_to_frames(param->pcm_handle->pcm, len),
			snd_pcm_frames_to_bytes(param->pcm_handle->pcm, 1));
	if (ret < 0) {
		BT_APP_AUDIO_ERROR("pcm_write error:%d\n", ret);
	}

	//ret = snd_pcm_drain(handle);
	/*ret = snd_pcm_drop(handle);*/
	//if (ret < 0)
	//	printf("stop failed!, return %d\n", ret);

	return ret;
}

static int pcm_read(snd_pcm_t *handle, const char *data, snd_pcm_uframes_t frames_total, unsigned int frame_bytes)
{
	int ret = 0;
	snd_pcm_sframes_t size;
	snd_pcm_uframes_t frames_loop = 160;
	snd_pcm_uframes_t frames_count = 0;
	snd_pcm_uframes_t frames = 0;
	unsigned int offset = 0;

	while (1) {
		if ((frames_total - frames_count) < frames_loop)
			frames = frames_total - frames_count;
		if (frames == 0)
			frames = frames_loop;
		/*printf("snd_pcm_readi %ld frames\n", frames);*/
		size = snd_pcm_readi(handle, (void *)(data + offset), frames);
		if (size < 0)
			BT_APP_AUDIO_ERROR("snd_pcm_readi return %ld\n", size);
		if (size == -EAGAIN) {
			/* retry */
			usleep(1000);
			continue;
		} else if (size == -EPIPE) {
			xrun(handle);
			continue;
		} else if (size == -ESTRPIPE) {
			continue;
		} else if (size < 0) {
			BT_APP_AUDIO_ERROR("-----snd_pcm_readi failed!!, return %ld\n", size);
			ret = (int)size;
			goto err;
		}
		offset += (size * frame_bytes);
		frames_count += size;
		frames -= size;
		if (frames_total == frames_count)
			break;
		/*printf("frames_count = %ld, frames_total = %ld\n", frames_count, frames_total); */
	}

err:
	return frames_count > 0 ? frames_count : ret;
}

static int bt_app_audio_read(bt_audio_t *param, uint8_t *data, uint32_t len)
{
	int ret = 0;
	if (param->mic_handle->pcm == NULL) {
		BT_APP_AUDIO_ERROR("bt_app_audio_write handle is null\n");
		return -1;
	}
	const int drop = 6;	//channel=3 samplerate=16000 to
						//channel=1 samplerate=8000 drop=6
	ret = pcm_read(param->mic_handle->pcm, (char *)data,
				snd_pcm_bytes_to_frames(param->mic_handle->pcm, drop * len),
				snd_pcm_frames_to_bytes(param->mic_handle->pcm, 1));
	for (int i = 0; i < len / 2; i++) {
		*(short*) (data + 2 * i) = *(short*) (data + 2 * i * drop);
	}
	if (ret < 0) {
		BT_APP_AUDIO_ERROR("pcm_read error:%d\n", ret);
	}
	ret = snd_pcm_frames_to_bytes(param->mic_handle->pcm, ret) / drop;
	return ret;
}

int set_param(snd_pcm_t *handle, snd_pcm_format_t format,
			unsigned int rate, unsigned int channels,
			snd_pcm_uframes_t period_size,
			snd_pcm_uframes_t buffer_size);
#endif

static void bt_audio_incoming_thread(void *_param) //speaker
{
	int ret;
	int play_start = 0;
	bt_audio_t *param = (bt_audio_t *)_param;
	unsigned int rate;
	unsigned int channels;
	char *incoming_buffer = NULL;
	int buffer_size;

	if (param == NULL || param->fifo_incoming_buffer == NULL ||
		param->pcm_handle == NULL) {
		return;
	}

	rate = param->pcm_handle->rate;
	channels = param->pcm_handle->channels;

/*open and create*/
#ifndef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
	ret = snd_pcm_open(&param->pcm_handle->pcm, AUDIO_NAME, param->pcm_handle->stream, 0);
	if (ret < 0) {
		BT_APP_AUDIO_ERROR("speaker open error:%d\n", ret);
		goto failed;
	}
#else
	tAudioTrack *handle = NULL;
	handle = AudioTrackCreate(AUDIO_NAME);
	if (handle == NULL) {
		BT_APP_AUDIO_ERROR("speaker create error\n");
		goto failed;
	}
	AudioTrackResampleCtrl(handle, 1);
#endif

	buffer_size = rate * channels * BIT_DEEP / 8 * BT_INCOMING_PERIOD_TIME_MS / 1000;
	buffer_size = (buffer_size > FIFO_BUFFER_SPEAKER_SIZE) ? FIFO_BUFFER_SPEAKER_SIZE : buffer_size;
	incoming_buffer = malloc(buffer_size);
	if (incoming_buffer == NULL) {
		BT_APP_AUDIO_ERROR("malloc incoming buf failed\n");
		goto failed;
	}

	// xrun(param->pcm_handle->pcm);
	bt_atomic_set_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_SETTING);
	bt_atomic_set_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_ENABLE);

	while (bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_ENABLE)) {
		if (bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_SUSPEND)) {
			usleep(1000);
			play_start = 0;
			continue;
		}
		if (bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_SETTING)) {
#ifndef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
			ret = set_param(param->pcm_handle->pcm, param->pcm_handle->format, rate,
					channels, param->pcm_handle->period_size,
					param->pcm_handle->buffer_size);
			if (ret < 0) {
				goto failed;
			}
#else
			ret = AudioTrackSetup(handle, rate, channels, BIT_DEEP);

			usleep(1000 * 200);
			if (ret != 0) {
				BT_APP_AUDIO_ERROR("speaker set error:%d\n", ret);
				goto failed;
			}
#endif
			fifo_drain(&param->fifo_outgoing, 1);
			BT_APP_AUDIO_DEBUG("rate:%u, channel:%u\n", rate, channels);
			bt_atomic_clear_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_SETTING);
			continue;
		}
		if (play_start == 0) {
			if (fifo_getlen(&param->fifo_incoming) < param->cache_size) {
				usleep(1000);
				continue;
			}
			play_start = 1;
		}
		/*write*/
		if (fifo_getlen(&param->fifo_incoming) >= buffer_size) {
			fifo_pop(&param->fifo_incoming, incoming_buffer, buffer_size);
#ifndef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
			bt_app_audio_write(param, (uint8_t *)incoming_buffer, buffer_size);
#else
			ret = AudioTrackWrite(handle, incoming_buffer, buffer_size);
#endif
		}
		usleep(1000 * BT_INCOMING_PERIOD_TIME_MS >> 2);
	}

failed:
	if (incoming_buffer) {
		free(incoming_buffer);
	}

#ifndef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
	if (param->pcm_handle->pcm) {
		ret = snd_pcm_close(param->pcm_handle->pcm);
		param->pcm_handle->pcm = NULL;
		if (ret < 0) {
			BT_APP_AUDIO_ERROR("audio in close error:%d\n", ret);
		}
	}
#else
	if (handle) {
		ret = AudioTrackDestroy(handle);
		if (ret < 0) {
			BT_APP_AUDIO_ERROR("audio in close error:%d\n", ret);
		}
	}
#endif
	printf("[%s]:TaskDelete\n", __FUNCTION__);
	bt_atomic_set_bit(&param->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_THREAD_CLOSE_DONE);
	XR_OS_ThreadDelete(NULL);
	return;
}

static void bt_audio_outgoing_thread(void* _param) //microphone
{
	int ret = 0;
	bt_audio_t *param = (bt_audio_t *)_param;
	unsigned int rate;
	unsigned int channels;
	char *outgoing_buffer = NULL;
	int buffer_size;

	if (param == NULL || param->fifo_outgoing_buffer == NULL ||
		param->mic_handle == NULL) {
		return;
	}

	rate = param->mic_handle->rate;
	channels = param->mic_handle->channels;

/*open and create*/
#ifndef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
	ret = snd_pcm_open(&param->mic_handle->pcm, AUDIO_NAME, param->mic_handle->stream, 0);
	if (ret < 0) {
		BT_APP_AUDIO_ERROR("mic open error:%d\n", ret);
		goto failed;
	}
#else
	tAudioRecord *handle = NULL;
	handle = AudioRecordCreate(AUDIO_NAME);
	if (handle == NULL) {
		BT_APP_AUDIO_ERROR("mic create error\n");
		goto failed;
	}

	/*
	 * audio driver will produce a little more data, which may cause data drop.
	 * call this function to adjust it.
	 */
	//AudioRecordResampleCtrl(handle, 1);
#endif

	buffer_size = rate * channels * BIT_DEEP / 8 * BT_OUTGOING_PERIOD_TIME_MS / 1000;
	buffer_size = (buffer_size > FIFO_BUFFER_RECODER_SIZE) ? FIFO_BUFFER_RECODER_SIZE : buffer_size;
	outgoing_buffer = malloc(buffer_size);
	if (outgoing_buffer == NULL) {
		BT_APP_AUDIO_ERROR("malloc outgoing buf failed\n");
		goto failed;
	}

	bt_atomic_set_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_SETTING);
	bt_atomic_set_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_ENABLE);
	while (bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_ENABLE)) {
		if (bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_SUSPEND)) {
			usleep(1000);
			continue;
		}

		/*setting*/
		if (bt_atomic_test_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_SETTING)) {
#ifndef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
			ret = set_param(param->mic_handle->pcm, param->mic_handle->format, rate,
					channels, param->mic_handle->period_size,
					param->mic_handle->buffer_size);
			if (ret < 0) {
				goto failed;
			}
#else
			ret = AudioRecordSetup(handle, rate, channels, BIT_DEEP);
			if (ret != 0) {
				BT_APP_AUDIO_ERROR("mic set error:%d\n", ret);
				goto failed;
			}
#endif
			fifo_drain(&param->fifo_outgoing, 1);
			BT_APP_AUDIO_DEBUG("rate:%u, channel:%u\n", rate, channels);
			bt_atomic_clear_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_SETTING);
			continue;
		}

/*read*/
#ifndef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
		bt_app_audio_read(param, outgoing_buffer, buffer_size);
		fifo_push(&param->fifo_outgoing, outgoing_buffer, buffer_size);
#else
		AudioRecordRead(handle, outgoing_buffer, buffer_size);
		fifo_push(&param->fifo_outgoing, outgoing_buffer, buffer_size);
#endif
	}

failed:
	if (outgoing_buffer) {
		free(outgoing_buffer);
	}

#ifndef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
	ret = snd_pcm_close(param->mic_handle->pcm);
	param->mic_handle->pcm = NULL;
	if (ret < 0) {
		BT_APP_AUDIO_ERROR("audio out close error:%d\n", ret);
	}
#else
	ret = AudioRecordDestroy((void *)handle);
	if (ret < 0) {
		BT_APP_AUDIO_ERROR("audio out close error:%d\n", ret);
	}
#endif
	printf("[%s]:TaskDelete\n", __FUNCTION__);
	bt_atomic_set_bit(&param->audio_state, BT_AUDIO_STATE_BIT_RECODER_THREAD_CLOSE_DONE);
	XR_OS_ThreadDelete(NULL);
	return;
}

int bt_app_audio_init(void)
{
	//snd_pcm_init();
	return 0;
}

int bt_app_audio_deinit(void)
{
	return 0;
}

void bt_app_audio_config(uint32_t samplerate, uint32_t channels, bt_audio_type_t type)
{
	if (type == BT_APP_AUDIO_TYPE_A2DP) {
		a2dp_pcm_handle.rate = samplerate;
		a2dp_pcm_handle.channels = channels;
#ifndef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
		a2dp_pcm_handle.buffer_size = 1470*4;
		a2dp_pcm_handle.period_size = 1470;
#endif
	} else if(type == BT_APP_AUDIO_TYPE_HFP) {
		hfp_pcm_handle.rate = samplerate;
		hfp_pcm_handle.channels = channels;
#ifndef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
		hfp_pcm_handle.buffer_size = 512*4;
		hfp_pcm_handle.period_size = 512;
#endif
		hfp_mic_handle.rate = samplerate;
		hfp_mic_handle.channels = channels;
	}
}

void bt_app_audio_ctrl(event_msg *msg)
{
	int ret = -1;
	BT_APP_AUDIO_DEBUG("%s\n", __func__);
#if 1
	switch (msg->data) {
		case BT_APP_AUDIO_EVENTS_A2DP_START:
			bt_app_audio_start(BT_APP_AUDIO_TYPE_A2DP);
			BT_APP_AUDIO_DEBUG("bt_app_audio_ctrl BT_APP_AUDIO_EVENTS_A2DP_START\n");
			break;
		case BT_APP_AUDIO_EVENTS_HFP_START:
			bt_app_audio_start(BT_APP_AUDIO_TYPE_HFP);
			BT_APP_AUDIO_DEBUG("bt_app_audio_ctrl BT_APP_AUDIO_EVENTS_HFP_START\n");
			break;
		case BT_APP_AUDIO_EVENTS_A2DP_STOP:
			bt_app_audio_stop(bt_audio_a2dp);
			bt_audio_a2dp = NULL;
			BT_APP_AUDIO_DEBUG("bt_app_audio_ctrl BT_APP_AUDIO_EVENTS_A2DP_STOP\n");
			break;
		case BT_APP_AUDIO_EVENTS_HFP_STOP:
			bt_app_audio_stop(bt_audio_hfp);
			bt_audio_hfp = NULL;
			BT_APP_AUDIO_DEBUG("bt_app_audio_ctrl BT_APP_AUDIO_EVENTS_HFP_STOP\n");
			break;
		default:
			BT_APP_AUDIO_ERROR("bt audio request err");
			break;
	}
#endif
}

int bt_app_audio_write_unblock(const uint8_t *data, uint32_t len, bt_audio_type_t type)//flag a2dp hfp
{
	if (type == BT_APP_AUDIO_TYPE_A2DP && bt_audio_a2dp != NULL) {
		if(bt_atomic_test_bit(&bt_audio_a2dp->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_ENABLE)) {
			return fifo_push(&(bt_audio_a2dp->fifo_incoming), data, len);
		}
	} else if (type == BT_APP_AUDIO_TYPE_HFP && bt_audio_hfp != NULL) {
		if(bt_atomic_test_bit(&bt_audio_hfp->audio_state, BT_AUDIO_STATE_BIT_SPEAKER_ENABLE)) {
			// printf("%d\n",fifo_getlen(&(bt_audio_hfp->fifo_incoming)));
			return fifo_push(&(bt_audio_hfp->fifo_incoming), data, len);
		}
	}
	return -1;
}

int bt_app_audio_read_unblock(uint8_t *data, uint32_t len)
{
	if (bt_audio_hfp != NULL) {
		if(bt_atomic_test_bit(&bt_audio_hfp->audio_state, BT_AUDIO_STATE_BIT_RECODER_ENABLE)) {
			if(fifo_getlen(&bt_audio_hfp->fifo_outgoing) >= len) {
				fifo_pop(&bt_audio_hfp->fifo_outgoing, data, len);
				return len;
			}
		}
		return 0;
	}
	return -1;
}

#ifndef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
int8_t bt_app_set_system_audio_vol(uint32_t volume)
{
	snd_ctl_info_t info;
	char g_card_name[20] = "audiocodec";
	if (volume >= 0) {
#ifdef CONFIG_DRIVERS_SOUND
		if (snd_ctl_get_bynum(g_card_name, 0, &info) != 0) {
			CMD_ERR("snd_ctl_get failed\n");
			return XR_FAIL;
		}
		snd_ctl_set_bynum(g_card_name, 0, volume);
#endif
	}
	return XR_OK;
}
#else
int8_t bt_app_set_system_audio_vol(uint32_t volume)
{
	int ret = 0;
	int type = AUDIO_STREAM_SYSTEM;
	uint32_t volume_value = 0;
	uint8_t max_volume = 0;

	ret = softvol_control_with_streamtype(type, &volume_value, 2);
	if (ret != 0) {
		CMD_ERR("get softvol range failed:%d\n", ret);
		return XR_FAIL;
	}
	max_volume = (volume_value >> 16) & 0xffff;
	volume_value = (volume * max_volume / 100) & 0xffff;
	volume_value |= (volume_value << 16);
	ret = softvol_control_with_streamtype(type, &volume_value, 1);
	if (ret != 0) {
		CMD_ERR("set softvol failed:%d\n", ret);
		return XR_FAIL;
	}

	return XR_OK;
}
#endif

static int open_file(char *FILE_PATH)
{
	if (!music_file) {
		CMD_DBG("[music file]open_file:%s\n", FILE_PATH);
		music_file = fopen(FILE_PATH,"r");
		if(music_file == NULL) {
			CMD_DBG("[music file]failed to open %s\n", FILE_PATH);
			return 0;
		}
	}
	return 1;
}

static void close_file(char *FILE_PATH)
{
	if (music_file) {
		CMD_DBG("[music file]close_file:%s\n", FILE_PATH);
		fclose(music_file);
		music_file = NULL;
	}
}

void bt_app_av_media_play_state_change(bt_av_media_ctrl event)
{
	close_file(media_list[media_file_index]);
	switch (event) {
	case APP_AV_MEDIA_FORWARD:
		if ((++media_file_index) >= BT_AV_SOURCE_FILE_NUM)
			media_file_index = 0;
		if (media_list[media_file_index][0] == 0)
			media_file_index = 0;
		break;
	case APP_AV_MEDIA_BACKWARD:
		if ((--media_file_index) < 0)
			media_file_index = BT_AV_SOURCE_FILE_NUM - 1;
		if (media_list[media_file_index][0] == 0)
			media_file_index = 0;
		break;
	default :
		break;
	}
	if (open_file(media_list[media_file_index]))
		rewind(music_file);
}

void bt_app_av_media_state_change(bt_av_media_state media_state)
{
	s_media_state = media_state;

	switch (media_state) {
	case APP_AV_MEDIA_STATE_IDLE:
		break;
	case APP_AV_MEDIA_STATE_STARTING:
		open_file(media_list[media_file_index]);
		break;
	case APP_AV_MEDIA_STATE_STARTED:
		break;
	case APP_AV_MEDIA_STATE_SUSPEND:
		break;
	case APP_AV_MEDIA_STATE_STOPPING:
		if (music_file)
			rewind(music_file);
		else
			CMD_ERR("%s music file is not open\n", __func__);
		s_media_state = APP_AV_MEDIA_STATE_IDLE;
		break;
	case APP_AV_MEDIA_STATE_STOPPED:
		close_file(media_list[media_file_index]);
		s_media_state = APP_AV_MEDIA_STATE_IDLE;
		break;
	default:
		CMD_DBG("%s invalid state %d\n", __func__, media_state);
		break;
	}
}

bt_av_media_state bt_app_get_media_state(void)
{
	return s_media_state;
}

uint8_t bt_app_play_music(const char *name)
{
	int len = 0;

	if (name) {
		CMD_DBG("source play %s\n", name);
		len = strlen(name);
		if (len > sizeof(usr_media) - 1) {
			CMD_ERR("name length should shorter than %d\n", sizeof(usr_media) - 1);
			media_file_index = 0;
			return (uint8_t)(uintptr_t)CMD_STATUS_INVALID_ARG;
		}
		memset(usr_media, 0, sizeof(usr_media));
		strcpy(usr_media, name);
		media_file_index = 1;
	} else {
		media_file_index = 0;
	}
	return (uint8_t)(uintptr_t)CMD_STATUS_OK;

}

int32_t bt_app_data_cb(uint8_t *data, int32_t len)
{
	if (len < 0 || data == NULL) {
		return 0;
	}

	unsigned int writenum = 0;
	if (music_file) {
		writenum = fread(data, sizeof(uint8_t), len, music_file);
		if (writenum < len) {
			bt_app_av_media_play_state_change(APP_AV_MEDIA_REOPEN);
			return 0;
		}
#if TEST_DATA
	} else {
		int i;
		for (i = 0; i < len; i++) {
			data[i] = i;
		}
#endif
	}
	return writenum;
}

#if 0 //for test
//static void hfp_out_thread_test(void *param)
//{
//	char buf[4096];
//	int cnt = 0;
//	for (int i = 0; i < 2048; i++) {
//		double res = (double) (2000.0 * sin((double) (i*2*10) / 2048.0 * 3.14159*2));
//		short sres = (short) res;
//		*((short *) buf + 2 * i) = sres;
//		*((short *) buf + 2 * i + 1) = sres;
//	}
//	do {
//		cnt++;
//		if (fifo_getlen(&fifo_incoming) <= (2048*4)) {
//			bt_app_audio_write_unblock(buf, 2048, 0);
//		} else {
//			usleep(100);
//		}
//		if (cnt%100 == 0) {
//			printf("%d\n",fifo_getlen(&fifo_incoming));
//		}
//	} while (1);
//	printf("[%s]:TaskDelete\n", __FUNCTION__);
//	vTaskDelete(NULL);
//}

//static void bt_audio(int argc, char **argv)
//{
//	if (argc < 2) {
//		printf("para invalid\n");
//		return ;
//	}
//	if (!strcmp(argv[1], "on")) {
//		printf("bt_au on\n");
//		bt_app_audio_config(44100, 2);
//		event_msg msg;
//		msg.data = BT_APP_AUDIO_EVENTS_A2DP_START;
//		bt_app_audio_ctrl(&msg);
//		xTaskCreate(hfp_out_thread_test, "hfp_i_test", 4096, NULL, tskIDLE_PRIORITY + 2, NULL);
//		bt_app_audio_init();
//		} else if(!strcmp(argv[1], "off")) {
//			printf("bt_au off\n");
//			bt_app_audio_deinit();
//	} else {
//		printf("para invalid\n");
//		}
//	}
//FINSH_FUNCTION_EXPORT_CMD(bt_audio, bt_audio, Console bt_audio Command);
#endif

