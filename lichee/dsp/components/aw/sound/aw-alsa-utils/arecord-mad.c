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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <aw-alsa-lib/common.h>
#include <aw-alsa-lib/control.h>
#include "common.h"

#include "standby.h"

#define ARECORD_MAD_COMMOND_HELP \
	"\r\narecord_mad <[-D card_num]> <-c channels> <-r rates> <-f bits>" \
	" <-p period_size> <-b buffer_size> <-t buffer seconds> " \
	" <-l loop_count>\r\n" \
	" eg: arecord_mad -D hw:audiocodec -c 4 -r 16000 -f 16 -p 1024 -b 8192 -t 5 -l 100\r\n" \
	" eg: arecord_mad -D hw:snddmic -c 2 -r 16000 -f 16 -p 1024 -b 4096 -t 4 -l 5\r\n"

static char *g_pcm_name;
static char *g_ctl_name;
static unsigned int loop_count = 5;

static unsigned int task_count = 0;

struct arecord_mad_priv {
	int mad_argc;
	char **mad_argv;
	audio_mgr_t *audio_mgr;
};

static struct arecord_mad_priv arecord_mad_priv;
static TaskHandle_t mad_voice_task = NULL;

extern SemaphoreHandle_t mad_sleep;
extern int sunxi_mad_schd_timeout(SemaphoreHandle_t semaphore, long ms);

static int suspend_then_wakeup_to_play(audio_mgr_t *audio_mgr)
{
	char *capture_data = NULL;
	snd_pcm_t *handle;
	int mode = 0;
	int ret = 0;
	unsigned int len = 0;
	snd_pcm_uframes_t period_frames = 1024, buffer_frames = 4096;

	if ((audio_mgr->capture_duration == 0) || (audio_mgr->capture_duration > 5))
		audio_mgr->capture_duration = 5;

	len = snd_pcm_format_size(audio_mgr->format,
			audio_mgr->capture_duration * audio_mgr->rate * audio_mgr->channels);
	capture_data = malloc(len);
	if (!capture_data) {
		printf("no memory\n");
		ret = -ENOMEM;
		goto err_malloc_capture_data;
	}
#ifdef CONFIG_SND_PLATFORM_SUNXI_MAD
	amixer_sset_enum_ctl(g_ctl_name, "bind mad function", "mad_bind");
#endif
	printf("\narecord start...\n");

	/* open card */
	ret = snd_pcm_open(&handle, g_pcm_name, SND_PCM_STREAM_CAPTURE, mode);
	if (ret < 0) {
		printf("audio open error:%d\n", ret);
		ret = -EFAULT;
		goto err_pcm_open;
	}

	printf("pcm_set_param...\n");
	ret = set_param(handle, audio_mgr->format, audio_mgr->rate,
		       audio_mgr->channels, period_frames, buffer_frames);
	if (ret < 0) {
		printf("audio set param error:%d\n", ret);
		ret = -EINVAL;
		goto err_set_param;
	}

	do {
		memset(capture_data, 0, len);

		ret = snd_pcm_prepare(handle);
		if (ret < 0) {
			printf("pcm_prepare error:%d\n", ret);
			//goto err_pcm_prepare;
		}
		printf("pcm_read start...\n");
#if 1
		ret = pcm_read(handle, capture_data,
			snd_pcm_bytes_to_frames(handle, len),
			snd_pcm_frames_to_bytes(handle, 1));
		if (ret < 0) {
			printf("pcm_read error:%d\n", ret);
			//goto err_pcm_read;
		}

		ret = snd_pcm_drop(handle);
		if (ret < 0) {
			printf("stop failed!, return %d\n", ret);
		//	goto err_pcm_drop;
		}
#endif
		printf("arecord stop...\n");

		/* start to playback ack */
		printf("aplay start...\n");
#if 1
		aplay_data("hw:snddaudio1", audio_mgr->format, audio_mgr->rate,
				audio_mgr->channels, capture_data, len, 1);
#endif
		printf("aplay stop...\n");
		if (!loop_count)
			break;
		loop_count--;
		/* simulator enter standby */
		amixer_sset_enum_ctl(g_ctl_name, "mad standby control", "SUSPEND");
		if (sunxi_mad_schd_timeout(mad_sleep, portMAX_DELAY) == 0)
			printf("***********wakeup by sunxi mad*************\n");
		else
			amixer_sset_enum_ctl(g_ctl_name, "mad standby control", "RESUME");
		printf("--->>loop_count:%u<<---\n", loop_count);
	} while (1);

	/* close card */
	ret = snd_pcm_close(handle);
	if (ret < 0) {
		printf("audio close error:%d\n", ret);
	}

#ifdef CONFIG_SND_PLATFORM_SUNXI_MAD
	amixer_sset_enum_ctl(g_ctl_name, "bind mad function", "mad_unbind");
#endif

	free(capture_data);
	capture_data = NULL;
	return 0;

#if 0
err_pcm_drop:
err_pcm_read:
err_pcm_prepare:
#endif
err_set_param:
	ret = snd_pcm_close(handle);
	if (ret < 0)
		printf("audio close error:%d\n", ret);
err_pcm_open:
	free(capture_data);
err_malloc_capture_data:
	capture_data = NULL;
	return ret;
}

void arecord_mad_entry(void *argv)
{
	struct arecord_mad_priv *mad_priv = argv;

	int c;
	unsigned int bits = 16;
	audio_mgr_t *audio_mgr = NULL;

	FUNCTION_THREAD_START_LINE_PRINTF(__func__);

	g_pcm_name = strdup("hw:audiocodec");
	g_ctl_name = strdup("audiocodec");
	loop_count = 5;

	mad_priv->audio_mgr = audio_mgr_create();
	if (!mad_priv->audio_mgr)
		goto err_create_audio_mgr;
	audio_mgr = mad_priv->audio_mgr;

	optind = 0;
	while ((c = getopt(mad_priv->mad_argc, mad_priv->mad_argv, "D:r:f:c:p:b:t:l:h")) != -1) {
		switch (c) {
		case 'D':
			free(g_pcm_name);
			g_pcm_name = NULL;
			g_pcm_name = strdup(optarg);
			free(g_ctl_name);
			g_ctl_name = NULL;
			g_ctl_name = strdup(g_pcm_name + 3);
			printf("pcm_name:%s, ctl_name:%s\n", g_pcm_name, g_ctl_name);
			break;
		case 'r':
			audio_mgr->rate = atoi(optarg);
			break;
		case 'f':
			bits = atoi(optarg);
			break;
		case 'c':
			audio_mgr->channels = atoi(optarg);
			break;
		case 'p':
			audio_mgr->period_size = atoi(optarg);
			break;
		case 'b':
			audio_mgr->buffer_size = atoi(optarg);
			break;
		case 't':
			audio_mgr->capture_duration = atoi(optarg);
			break;
		case 'l':
			loop_count = atoi(optarg);
			break;
		case 'h':
		default:
			printf("%s\n", ARECORD_MAD_COMMOND_HELP);
			goto err_command;
		}
	}

	switch (bits) {
	default:
		printf("%u bits not supprot, using 16bits.\n", bits);
	case 16:
		audio_mgr->format = SND_PCM_FORMAT_S16_LE;
		break;
	case 24:
		audio_mgr->format = SND_PCM_FORMAT_S24_LE;
		break;
	}

	suspend_then_wakeup_to_play(audio_mgr);

err_command:
	if (g_ctl_name)
		free(g_ctl_name);
	g_ctl_name = NULL;
	if (g_pcm_name)
		free(g_pcm_name);
	g_pcm_name = NULL;
	audio_mgr_release(audio_mgr);
	mad_priv->audio_mgr = NULL;

err_create_audio_mgr:
	mad_voice_task = NULL;

	FUNCTION_THREAD_STOP_LINE_PRINTF(__func__);

	task_count = 0;

	vTaskDelete(NULL);
}

int arecord_mad(int argc, char ** argv)
{
	BaseType_t ret = 0;

	if (task_count)
		return 0;

	task_count++;

	arecord_mad_priv.mad_argv = argv;
	arecord_mad_priv.mad_argc = argc;

	ret = xTaskCreate(arecord_mad_entry, "mad-thread", 1024,
			&arecord_mad_priv, configAPPLICATION_AUDIO_PRIORITY,
			&mad_voice_task);
	if (ret != pdPASS) {
		awalsa_err("mad-thread create failed.\n");
		return -EFAULT;
	}

	return 0;
}
#ifdef CONFIG_COMPONENTS_FREERTOS_CLI
FINSH_FUNCTION_EXPORT_CMD(arecord_mad, arecord_mad, mad test);
#endif
