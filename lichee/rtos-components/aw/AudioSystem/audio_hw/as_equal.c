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
#define TAG	"AudioHWAWEQ"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <task.h>
#include <errno.h>
#include <hal_time.h>
#include <hal_sem.h>
#include <hal_thread.h>
#include <hal_queue.h>

#include "audio_hw.h"
#include "AudioBase.h"
#include "local_audio_hw.h"

#include "eq.h"
#include "AudioEqual.h"

#include "aactd/common.h"
#include "aactd/communicate.h"

#include <aw-alsa-lib/pcm.h>
#include <aw-alsa-lib/common.h>

typedef struct  awequal{
	char *config_file[AW_EQUAL_CONFIG_NUMBER];
	as_mutex_t eq_mutex;
	int eq_enabled;
	eq_prms_t eq_prms;
	void *eq_handle;
}awequal_t;

struct aweq_attr {
	snd_pcm_t *handle;
	awequal_t *aweq_handle;
	audio_hw_t *ahw;
};

typedef struct  awequal_thread {
	hal_sem_t sync;
	hal_thread_t ctrl_task;
	hal_queue_t aw_queue;
	as_mutex_t aw_mutex;
	int verbose_level;

}awequal_thread_t;

static as_mutex_t g_aw_mutex;

static int verbose = 1;

static awequal_t *g_awequal;

#define aw_lock()	as_mutex_lock(g_aw_mutex)
#define aw_unlock()	as_mutex_unlock(g_aw_mutex)

#ifdef CONFIG_DRIVER_SYSCONFIG
static struct ahw_params *g_pcm_params[2] = {NULL, NULL};
#endif

#define LINE_STR_BUF_LEN_MAX 32

#define TUNING_COM_BUF_LEN_DEFAULT 1024

#if (defined(CONFIG_COMPONENTS_PROCESS_EQ))

static int parse_config_to_eq_prms(const char *config_file, eq_prms_t *prms, int *enabled)
{
    int ret;
    FILE *fp = NULL;
    char line_str[LINE_STR_BUF_LEN_MAX];
    int temp_int;
    int type;
    int frequency;
    int gain;
    float Q;
    int index = 0;

    if (!config_file || !prms) {
        _err("Invalid config_file or prms");
        ret = -1;
        goto out;
    }

    fp = fopen(config_file, "r");
    if (!fp) {
        _err("Failed to open %s (%s)", config_file, strerror(errno));
        ret = -1;
        goto out;
    }

    while (fgets(line_str, LINE_STR_BUF_LEN_MAX, fp)) {
        if (sscanf(line_str, "channels=%d", &temp_int) == 1) {
            prms->chan = temp_int;
        } else if (sscanf(line_str, "enabled=%d", &temp_int) == 1) {
            *enabled = temp_int;
        } else if (sscanf(line_str, "bin_num=%d", &temp_int) == 1) {
            prms->biq_num = temp_int;
        } else if (sscanf(line_str, "samplerate=%d", &temp_int) == 1) {
            prms->sampling_rate = temp_int;
        } else if (sscanf(line_str, "params=%d %d %d %f",
                    &type, &frequency, &gain, &Q) == 4) {
            prms->core_prms[index].type = type;
            prms->core_prms[index].fc = frequency;
            prms->core_prms[index].G = gain;
            prms->core_prms[index].Q = Q;
            ++index;
        }
    }
    ret = 0;

    fclose(fp);
out:
    return ret;
}

static void print_eq_prms(const eq_prms_t *prms)
{
    int i;
    for (i = 0; i < prms->biq_num; ++i) {
        const eq_core_prms_t *core_prms = &prms->core_prms[i];
        _debug(" [Biquad%02i] type: %i, freq: %d, gain: %d, Q: %.2f",
                i + 1, core_prms->type, core_prms->fc, core_prms->G, core_prms->Q);
    }
}

#endif


static void print_eq_prms_pc(const eq_prms_t_pc *prms)
{
    int i;
    for (i = 0; i < MAX_FILTER_N; ++i) {
        const eq_core_prms_t *core_prms = &prms->core_prms[i];
        _debug(" [BQ%02i] type: %i, freq: %d, gain: %d, Q: %.2f, enabled: %d",
                i + 1, core_prms->type, core_prms->fc, core_prms->G,
                core_prms->Q, prms->enable_[i]);
    }
}


static int aactd_com_eq_sw_data_to_eq_prms(struct aactd_com_eq_sw_data *data,
        eq_prms_t_pc *prms)
{
    int ret;
    int i;

    for (i = 0; i < MAX_FILTER_N; ++i) {
        prms->enable_[i] = 0;
    }

    if (data->filter_num > MAX_FILTER_N) {
        _err("Too many filters");
        ret = -1;
        goto out;
    }

    for (i = 0; i < data->filter_num; ++i) {
        prms->core_prms[i].type = data->filter_args[i].type;
        prms->core_prms[i].fc = data->filter_args[i].frequency;
        prms->core_prms[i].G = data->filter_args[i].gain;
        prms->core_prms[i].Q = (float)(data->filter_args[i].quality) / 100;
        prms->enable_[i] = data->filter_args[i].enabled;
    }

    ret = 0;
out:
    return ret;
}

static void AudioEqualThreadRun(void *arg)
{
    awequal_thread_t *aw = (awequal_thread_t *)arg;
	struct AudioEqualProcessItem item;
	hal_tick_t tick = HAL_WAIT_FOREVER;
	int ret;
	int tuning_com_buf_len = TUNING_COM_BUF_LEN_DEFAULT;
	awequal_t *awequal = NULL;

	uint8_t *remote_com_buf = NULL;
	unsigned int remote_com_actual_len;
	struct aactd_com remote_com = {
		.data = NULL,
	};
	uint8_t checksum_cal;

	struct aactd_com_eq_sw_data remote_data = {
		.filter_args = NULL,
	};
	eq_prms_t_pc remote_prms;

	remote_com_buf = as_alloc(tuning_com_buf_len);
	if (!remote_com_buf) {
		_err("Failed to allocate memory for remote_com_buf");
		goto out;
	}
	remote_com.data = remote_com_buf + sizeof(struct aactd_com_header);

	remote_data.filter_args = as_alloc(tuning_com_buf_len);
	if (!remote_data.filter_args) {
		_err("Failed to allocate memory for remote_data.filter_args");
		goto free;
	}

	while (1) {
		ret = hal_queue_recv(aw->aw_queue, &item, tick);
		if (ret != HAL_OK) {
			_err("Failed to recv queue\n");
			ret = -1;
			goto free;
		}

		_debug("receive len:%d", item.len);

		memset(remote_com_buf, 0 , tuning_com_buf_len);
		if (!memcmp(remote_com_buf, item.data, tuning_com_buf_len)) {
			 _debug("recv the memory is null, exit the thread\n");
            goto free;
		}

		/* Read header */
		memcpy(remote_com_buf, item.data, sizeof(struct aactd_com_header));
		aactd_com_buf_to_header(remote_com_buf, &remote_com.header);
        if (remote_com.header.flag != AACTD_COM_HEADER_FLAG) {
            _err("Incorrect header flag: 0x%x\n", remote_com.header.flag);
            goto wait_for_next_event;
        }

        remote_com_actual_len =
            sizeof(struct aactd_com_header) + remote_com.header.data_len + 1;

		/* Read data & checksum */
		memcpy(remote_com.data, item.data + sizeof(struct aactd_com_header), remote_com.header.data_len + 1);

		/* Verify checksum */
        remote_com.checksum = *(remote_com.data + remote_com.header.data_len);
        checksum_cal = aactd_calculate_checksum(remote_com_buf, remote_com_actual_len - 1);
        if (remote_com.checksum != checksum_cal) {
            _err("Checksum error (got: 0x%x, calculated: 0x%x), discard and try again",
                    remote_com.checksum, checksum_cal);
            goto wait_for_next_event;
        }


		switch(remote_com.header.type) {

			case EQ_SW:

				ret = aactd_com_eq_sw_buf_to_data(remote_com.data, &remote_data);
				if (ret < 0) {
					_err("Failed to convert remote data buffer to data");
					goto wait_for_next_event;
				}

				ret = aactd_com_eq_sw_data_to_eq_prms(&remote_data, &remote_prms);
				if (ret < 0) {
					_err("Failed to convert remote data to eq prms");
					goto wait_for_next_event;
				}

				if (aw->verbose_level >= 1) {
					_info("Data received from remote server:");
					aactd_com_print_content(&remote_com);
					_debug("Parameters updated:");
					print_eq_prms_pc(&remote_prms);
				}

				awequal = g_awequal;

				if (awequal) {
					as_mutex_lock(awequal->eq_mutex);

					awequal->eq_enabled = remote_data.global_enabled;
					/*
					 * NOTE: Use original "chan" and "sampling_rate", because they
					 * won't be set by remote server.
					 */
					remote_prms.chan = awequal->eq_prms.chan;
					remote_prms.sampling_rate = awequal->eq_prms.sampling_rate;
					eq_avert_parms(&awequal->eq_prms, &remote_prms);
					awequal->eq_handle = eq_setpara_reset(&awequal->eq_prms, awequal->eq_handle);
					if (!awequal->eq_handle) {
						_err("Failed to reset eq parameters");
						as_mutex_unlock(awequal->eq_mutex);
						goto wait_for_next_event;
					}
					as_mutex_unlock(awequal->eq_mutex);
				} else {
					_err("EQ parameters is null, please play first.");
					goto wait_for_next_event;
				}

				break;


			default:
				_err("Unknown command\n");
				break;
		}

wait_for_next_event:
		continue;

	}

free:

	hal_sem_post(aw->sync);

	if (remote_com_buf) {
    	as_free(remote_com_buf);
		remote_com_buf = NULL;
	}
	if (remote_data.filter_args) {
		as_free(remote_data.filter_args);
		remote_data.filter_args = NULL;
	}
out:

	_debug("finish");
	hal_thread_stop(NULL);
}

void AudioEqualThreadDestory(awequal_thread_t * aw)
{

	_debug("");

	hal_sem_wait(aw->sync);

	if (aw->sync) {
		hal_sem_delete(aw->sync);
		aw->sync = NULL;
	}

	if (aw->aw_queue) {
		hal_queue_delete(aw->aw_queue);
		aw->aw_queue = NULL;
	}

	if (aw->aw_mutex) {
		as_mutex_destroy(aw->aw_mutex);
		aw->aw_mutex = NULL;
	}

	if (aw != NULL) {
		as_free(aw);
	}

	_debug("");

	return;
}

awequal_thread_t *AudioEqualThreadCreate()
{
	awequal_thread_t *aw = NULL;

	_debug("");


	aw = as_alloc(sizeof(awequal_thread_t));
	if (!aw) {
		_err("no memory");
		goto err;
	}

	aw->sync = hal_sem_create(1);
	if (aw->sync == NULL) {
		_err("create semaphore err");
		goto err;
	}

	aw->aw_mutex = as_mutex_init();
	if (!aw->aw_mutex) {
		_err("as_mutex_init failed");
		goto err;
	}

	/* queue init  */
	aw->aw_queue = hal_queue_create("aw queue", 20, sizeof(struct AudioEqualProcessItem));
	if (!aw->aw_queue) {
		_err("hal_queue_create failed");
		goto err;
	}

	/* ctrl_task init */
	aw->ctrl_task = hal_thread_create(AudioEqualThreadRun, aw, "AudioAWEq", 4096,
			configAPPLICATION_AUDIO_PRIORITY);
	if (!aw->ctrl_task) {
		_err("ctrl_task create failed");
		goto err;
	}
	_debug("");
	return aw;

err:

	if (aw->aw_queue) {
		hal_queue_delete(aw->aw_queue);
		aw->aw_queue = NULL;
	}

	if (aw->sync) {
		hal_sem_delete(aw->sync);
		aw->sync = NULL;
	}

	if (aw->aw_mutex) {
		as_mutex_destroy(aw->aw_mutex);
		aw->aw_mutex = NULL;
	}

	if (aw != NULL) {
		as_free(aw);
		aw = NULL;
	}
	return NULL;
}

int AudioEqualThreadInit(void)
{
	g_aw_mutex = as_mutex_init();
	if (!g_aw_mutex) {
		_err("as_mutex_init failed");
		return -1;
	}
	return 0;
}

int AudioEqualThreadDeInit(void)
{

	if (!g_aw_mutex) {
		return 0;
	}

	as_mutex_destroy(g_aw_mutex);

	return 0;
}

int AudioEqualThreadRemoveAE(tAudioEqual *ae)
{
	awequal_thread_t *aw = (awequal_thread_t *)ae->priv;

	if (!aw) {
		_info("aw is null");
		return 0;
	}

	aw_lock();

	AudioEqualThreadDestory(aw);

	aw_unlock();

	return 0;
}

int AudioEqualThreadAddAE(tAudioEqual *ae)
{
	awequal_thread_t *aw;

	aw_lock();

	/*  aw create */
	aw = AudioEqualThreadCreate();
	if (!aw) {
		_err("create aw failed");
		aw_unlock();
		return  -1;
	}

	ae->ae_queue = aw->aw_queue;

	aw->verbose_level = ae->verbose_level;

	ae->priv = aw;

	aw_unlock();
	_debug("");
	return 0;
}

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

static int eq_ahw_init(audio_hw_t *ahw)
{
	struct aweq_attr *ea;
	as_pcm_config_t *pcm_config;

	ea = as_alloc(sizeof(struct aweq_attr));
	if (!ea) {
		_err("no memory");
		goto err;
	}

	ea->handle = NULL;
	ea->aweq_handle = NULL;
	ea->ahw = ahw;
	/*_debug("set ahw(%p) into aweq_attr", ahw);*/
	audio_hw_set_private_data(ahw, ea);
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
		_err("Only support playback");
		goto err;

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

static int eq_ahw_destroy(audio_hw_t *ahw)
{
	struct aweq_attr *ea;

	ea = audio_hw_get_private_data(ahw);
	if (!ea)
		return 0;
	as_free(ea);
	audio_hw_set_private_data(ahw, NULL);

	return 0;
}

static int eq_ahw_open(struct aweq_attr *ea)
{
	awequal_t *awequal = NULL;
    const char *config_file[AW_EQUAL_CONFIG_NUMBER];
    int ret = -1;
	int i = 0;
	const char *card = NULL;
	audio_hw_t *ahw = ea->ahw;
	as_pcm_config_t *pcm_config = audio_hw_pcm_config(ahw);
	struct audio_hw_elem *elem = audio_hw_elem_item(ahw);

	awequal = as_alloc(sizeof(awequal_t));
	if (!awequal) {
		_err("Failed to allocate memory for awequal_t");
		goto error;
	}

	awequal->eq_mutex = as_mutex_init();
	if (!awequal->eq_mutex) {
		_err("as_mutex_init failed");
		goto error;
	}

	ea->aweq_handle = awequal;

	if (audio_hw_stream(ahw) == 0)
		card = elem->card_name_pb;
	else
		card = elem->card_name_cap;

	_debug("card:%s, stream:%d", card, audio_hw_stream(ahw));
	ret = snd_pcm_open(&ea->handle, card, audio_hw_stream(ahw), 0);
	if (ret < 0) {
		_err("pcm open failed:%d", ret);
		goto error;
	}

	ret = set_param(ea->handle, pcm_config->format,
		pcm_config->rate, pcm_config->channels,
		pcm_config->period_frames,
		pcm_config->period_frames * pcm_config->periods);
	if (ret != 0) {
		_err("set param failed:%d", ret);
		goto error;
	}

#if (defined(CONFIG_COMPONENTS_PROCESS_EQ))

	config_file[0] = "/data/EQ.conf";
	config_file[1] = NULL;
	awequal->config_file[0] = (char *)as_alloc(strlen(config_file[0]) + 1);
	if (!awequal->config_file[0]) {
		_err("Failed to allocate memory for config_file");
		goto error;
	}
	strncpy(awequal->config_file[0], config_file[0], strlen(config_file[0]) + 1);

	awequal->eq_handle = NULL;

	ret = parse_config_to_eq_prms(awequal->config_file[0],
			   &awequal->eq_prms, &awequal->eq_enabled);
	if (ret < 0) {
	   _err("parse_config_to_eq_prms failed");
	   goto error;
	}

	awequal->eq_prms.chan = pcm_config->channels;
	awequal->eq_prms.sampling_rate = pcm_config->rate;

	if (verbose) {
		_info("Parse from config file %s", awequal->config_file[0]);
		_info("  GlobalEnable: %d", awequal->eq_enabled);
		print_eq_prms(&awequal->eq_prms);
	}

	awequal->eq_handle = eq_create(&awequal->eq_prms);
	if (!awequal->eq_handle) {
		_err("eq_create failed");
		ret = -1;
		goto error;
	}

	g_awequal = awequal;

#endif
	return 0;

error:

	if (ea->handle) {
		snd_pcm_close(ea->handle);
		ea->handle = NULL;
	}

	if (awequal->eq_handle) {
		eq_destroy(awequal->eq_handle);
		awequal->eq_handle = NULL;
	}

	for (i = 0; i < AW_EQUAL_CONFIG_NUMBER; ++i) {
		if (awequal->config_file[i]) {
			as_free(awequal->config_file[i]);
			awequal->config_file[i] = NULL;
		}
	}
	if (awequal->eq_mutex) {
		as_mutex_destroy(awequal->eq_mutex);
		awequal->eq_mutex = NULL;
	}

	if (awequal) {
		as_free(awequal);
		awequal = NULL;
		ea->aweq_handle = NULL;
		g_awequal = NULL;
	}
	return ret;

}

static int eq_ahw_close(struct aweq_attr *ea)
{
	int ret;
	int i = 0;
	awequal_t *awequal = ea->aweq_handle;

	_debug("");

	if (!ea->handle || !awequal)
		return 0;

	ret = snd_pcm_close(ea->handle);
	if (ret < 0) {
		_err("pcm close failed:%d", ret);
		return -1;
	}

	if (awequal->eq_handle) {
		eq_destroy(awequal->eq_handle);
		awequal->eq_handle = NULL;
	}

	if (awequal->eq_mutex) {
		as_mutex_destroy(awequal->eq_mutex);
		awequal->eq_mutex = NULL;
	}

	for (i = 0; i < AW_EQUAL_CONFIG_NUMBER; ++i) {
		if (awequal->config_file[i]) {
			as_free(awequal->config_file[i]);
			awequal->config_file[i] = NULL;
		}
	}

	as_free(awequal);
	awequal = NULL;
	g_awequal = NULL;
	ea->handle = NULL;
	ea->aweq_handle = NULL;
	return 0;
}

static int eq_ahw_write(struct aweq_attr *ea, struct rb_attr *rb)
{
	audio_hw_t *ahw = ea->ahw;
	as_pcm_config_t *pcm_config = audio_hw_pcm_config(ahw);
	snd_pcm_t *handle = ea->handle;
	awequal_t *awequal = ea->aweq_handle;

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

		/* eq process */
#if (defined(CONFIG_COMPONENTS_PROCESS_EQ))

		as_mutex_lock(awequal->eq_mutex);

		if (!awequal->eq_handle) {
		   _err("Invalid eq handle");
		   as_mutex_unlock(awequal->eq_mutex);
		   return 0;
		}

		if (awequal->eq_enabled) {
		   eq_process(awequal->eq_handle, (int16_t *)data, frames);
		}

		as_mutex_unlock(awequal->eq_mutex);
#endif
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

/*
 * amp ahw
 * instance:	AUDIO_HW_TYPE_EQ
 * ops:		eq(equalization) api
 * */
static struct audio_hw_ops eq_ops = {
	.ahw_open = (ahw_func)eq_ahw_open,
	.ahw_read = NULL,
	.ahw_write = (ahw_func_xfer)eq_ahw_write,
	.ahw_close = (ahw_func)eq_ahw_close,
	.ahw_init = (ahw_func)eq_ahw_init,
	.ahw_destroy = (ahw_func)eq_ahw_destroy,
};

struct audio_hw_elem g_eq_ahw = {
	.name = "playbackEQ",
	.instance = AUDIO_HW_TYPE_EQ,
	.card_name_pb = NULL,
	.card_name_cap = NULL,
	.ops = &eq_ops,
};

