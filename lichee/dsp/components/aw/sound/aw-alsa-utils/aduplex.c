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
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <aw-alsa-lib/pcm.h>
#include <aw-alsa-lib/control.h>
#include "common.h"

#define ADUPLEX_TIME_MAX        (48 * 3600U)
#define ADUPLEX_PCM_QUEUE_SIZE  16U
#define ADUPLEX_WAIT_QUEUE_SIZE  2U
#define ADUPLEX_DATA_MODE 0x34353637 //0x01234567

#define ADUPLEX_COMMOND_HELP \
	"\r\naduplex <[-D card_num]> <-c channels> <-r rates> <-f bits>" \
	" <-p period_size> <-b buffer_size>" \
	" <-t seconds> <-H playback_card>\r\n" \
	" eg: aduplex -D hw:snddaudio0 -c 8 -r 48000 -f 16 -p 1024 -b 4096 -t 2 -H hw:snddaudio0\r\n"

static unsigned int audio_task_exit = 0;

enum aduplex_task_cmd {
	ADUPLEX_TASK_NULL = 0,
	ADUPLEX_TASK_EXIT = 1,
	ADUPLEX_PCM_QUEUE_ADD = 2,
	ADUPLEX_PCM_QUEUE_REMOVE = 3,
};

struct duplex_pcm_queue {
	void *pcm_data;
	unsigned int data_size;
	unsigned int using;
	struct list_head list;
};

static struct duplex_pcm_queue capture_pcm_queue[ADUPLEX_PCM_QUEUE_SIZE];
static struct duplex_pcm_queue playback_pcm_queue[ADUPLEX_PCM_QUEUE_SIZE];

struct duplex_priv {
	QueueHandle_t list_semaph;
	QueueHandle_t wait_queue;
	QueueHandle_t pcm_queue;
	struct list_head pcm_list;
	audio_mgr_t *playback_mgr;
	audio_mgr_t *capture_mgr;
	TaskHandle_t record_task;
	TaskHandle_t playback_task;
	unsigned int task_exit;
	char *pcm_name;
	char *hpcm_name;
};

static struct duplex_priv duplex_priv[2];
static int capture_stop;
static int playback_stop;
extern unsigned int g_verbose;

static QueueHandle_t aduplex_mutex_init()
{
	return xSemaphoreCreateMutex();
}

static int aduplex_mutex_lock(QueueHandle_t semaphore)
{
	int ret = 0;
	const TickType_t timeout = pdMS_TO_TICKS(1000);

	ret = xSemaphoreTake(semaphore, timeout);
	if (ret == pdFAIL) {
		awalsa_err("\n");
		return -EFAULT;
	}
	return 0;
}

static int aduplex_mutex_unlock(QueueHandle_t semaphore)
{
	int ret = 0;

	ret = xSemaphoreGive(semaphore);
	if (ret == pdFAIL) {
		awalsa_err("\n");
		return -EFAULT;
	}
	return 0;
}

static void aduplex_mutex_delete(QueueHandle_t semaphore)
{
	if (!semaphore)
		return;
	vSemaphoreDelete(semaphore);
}

static int duplex_pcm_queue_init(struct duplex_pcm_queue *pcm_queue, size_t data_size)
{
	int i = 0;

	for (i = 0; i < ADUPLEX_PCM_QUEUE_SIZE; i++) {
		pcm_queue[i].data_size = data_size;
		pcm_queue[i].pcm_data = malloc(pcm_queue[i].data_size);
		if (!pcm_queue[i].pcm_data) {
			awalsa_err("pcm_queue[%d]->pcm_data malloc failed.\n", i);
			goto err_pcm_queue_malloc;
		}
	}

	return 0;

err_pcm_queue_malloc:
	for (i = 0; i < ADUPLEX_PCM_QUEUE_SIZE; i++) {
		if (pcm_queue[i].pcm_data) {
			free(pcm_queue[i].pcm_data);
			pcm_queue[i].pcm_data = NULL;
		}
	}
	return -ENOMEM;
}

static int duplex_pcm_queue_set_using(struct duplex_pcm_queue *pcm_queue,
				unsigned int enable)
{
	if (!pcm_queue) {
		awalsa_err("\n");
		return -EFAULT;
	}
	pcm_queue->using = enable;

	return 0;
}

static int duplex_pcm_queue_get_using(struct duplex_pcm_queue *pcm_queue,
				unsigned int *enable)
{
	if (!pcm_queue) {
		awalsa_err("\n");
		return -EFAULT;
	}
	*enable = pcm_queue->using;

	return 0;
}

static struct duplex_pcm_queue *duplex_pcm_queue_get_idle_pcm_queue(
		struct duplex_pcm_queue *pcm_queue)
{
	int i = 0;
	unsigned int enable = 0;

	for (i = 0; i < ADUPLEX_PCM_QUEUE_SIZE; i++) {
		duplex_pcm_queue_get_using(&pcm_queue[i], &enable);
		if (enable == 0) {
			return &pcm_queue[i];
		}
	}

	return NULL;
}

static int duplex_pcm_queue_free(struct duplex_pcm_queue *pcm_queue)
{
	int i = 0;

	for (i = 0; i < ADUPLEX_PCM_QUEUE_SIZE; i++) {
		if (pcm_queue[i].pcm_data) {
			free(pcm_queue[i].pcm_data);
			pcm_queue[i].pcm_data = NULL;
			memset(&pcm_queue[i], 0, sizeof(struct duplex_pcm_queue));
		}
	}
	return 0;
}

static int duplex_pcm_list_init(struct duplex_priv *duplex_priv)
{
	if (!duplex_priv) {
		awalsa_err("\n");
		return -EFAULT;
	}

	aduplex_mutex_lock(duplex_priv->list_semaph);
	INIT_LIST_HEAD(&duplex_priv->pcm_list);
	aduplex_mutex_unlock(duplex_priv->list_semaph);

	return 0;
}

static int aduplex_pcm_list_add_pcm_queue(struct duplex_priv *duplex_priv,
			    struct duplex_pcm_queue *pcm_queue)
{
	struct list_head *new = NULL;
	struct list_head *head = NULL;

	if (!duplex_priv) {
		awalsa_err("\n");
		return -EFAULT;
	}
	if (!pcm_queue) {
		awalsa_err("\n");
		return -EFAULT;
	}

	aduplex_mutex_lock(duplex_priv->list_semaph);
	head = &duplex_priv->pcm_list;
	new = &pcm_queue->list;
	if (new && head) {
		duplex_pcm_queue_set_using(pcm_queue, 1);
		list_add_tail(new, head);
		aduplex_mutex_unlock(duplex_priv->list_semaph);
		return 0;
	}
	aduplex_mutex_unlock(duplex_priv->list_semaph);

	return -EFAULT;
}

static int pcm_list_get_head(struct duplex_priv *duplex_priv,
				struct duplex_pcm_queue **pcm_queue)
{
	struct duplex_pcm_queue *c = NULL;

	if (!duplex_priv) {
		awalsa_err("\n");
		return -EFAULT;
	}
	if (!pcm_queue) {
		awalsa_err("\n");
		return -EFAULT;
	}

	aduplex_mutex_lock(duplex_priv->list_semaph);
	if (list_empty(&duplex_priv->pcm_list)) {
		awalsa_err("\n");
		*pcm_queue = NULL;
		aduplex_mutex_unlock(duplex_priv->list_semaph);
		return -EFAULT;
	}

	c = list_first_entry(&duplex_priv->pcm_list, struct duplex_pcm_queue, list);
	if (!c) {
		awalsa_err("\n");
		*pcm_queue = NULL;
		aduplex_mutex_unlock(duplex_priv->list_semaph);
		return -EFAULT;
	}
	*pcm_queue = c;
	aduplex_mutex_unlock(duplex_priv->list_semaph);

	return 0;
}

static int duplex_pcm_list_remove_pcm_queue(struct duplex_priv *duplex_priv,
			struct duplex_pcm_queue *pcm_queue)
{
	struct duplex_pcm_queue *c = NULL;
	struct duplex_pcm_queue *tmp = NULL;

	if (!duplex_priv) {
		awalsa_err("\n");
		return -EFAULT;
	}
	if (!pcm_queue) {
		awalsa_err("\n");
		return -EFAULT;
	}

	aduplex_mutex_lock(duplex_priv->list_semaph);
	if (list_empty(&duplex_priv->pcm_list)) {
		awalsa_err("\n");
		aduplex_mutex_unlock(duplex_priv->list_semaph);
		return -EFAULT;
	}
	list_for_each_entry_safe(c, tmp, &duplex_priv->pcm_list, list) {
		if (c == pcm_queue) {
			list_del(&c->list);
			duplex_pcm_queue_set_using(c, 0);
			aduplex_mutex_unlock(duplex_priv->list_semaph);
			return 0;
		}
	}
	awalsa_err("\n");
	aduplex_mutex_unlock(duplex_priv->list_semaph);
	return -EFAULT;
}

static int duplex_pcm_list_delete_all_pcm_queue(struct duplex_priv *duplex_priv)
{
	struct duplex_pcm_queue *pcm_queue = NULL;
	struct duplex_pcm_queue *tmp = NULL;

	if (!duplex_priv) {
		awalsa_err("\n");
		return -EFAULT;
	}

	aduplex_mutex_lock(duplex_priv->list_semaph);
	if (list_empty(&duplex_priv->pcm_list)) {
		awalsa_err("\n");
		aduplex_mutex_unlock(duplex_priv->list_semaph);
		return -EFAULT;
	}

	list_for_each_entry_safe(pcm_queue, tmp, &duplex_priv->pcm_list, list) {
		list_del(&pcm_queue->list);
		duplex_pcm_queue_set_using(pcm_queue, 0);
	}

	list_del_init(&duplex_priv->pcm_list);
	aduplex_mutex_unlock(duplex_priv->list_semaph);

	return 0;
}

/* FIXME */
static int duplex_pcm_list_get_pcm_queue(struct duplex_priv *duplex_priv,
				struct duplex_pcm_queue **pcm_queue)
{
	int ret = 0;
	int i = 0;
	BaseType_t xStatus = 0;
	const TickType_t xTicksToWait = pdMS_TO_TICKS(1000);
	unsigned int receive_val = 0;

	for (i = 0; i < 10; i++) {
		if((ret = uxQueueMessagesWaiting(duplex_priv->pcm_queue)) != 0) {
		//	printf("%s QM Waiting:%d!\n", __func__, ret);
		}

		xStatus = xQueueReceive(duplex_priv->pcm_queue, &receive_val, xTicksToWait);
		if( xStatus == pdPASS ) {
			//printf("%s Received = %d\n", __func__, receive_val);
			switch (receive_val) {
			case ADUPLEX_PCM_QUEUE_ADD:/* get pcm data */
				ret = pcm_list_get_head(duplex_priv, pcm_queue);
				if (ret != 0) {
					printf("%s i=%d get_head failed.\n",
						__func__, i);
				} else {
					//printf("+++%s %d+++\n", __func__, __LINE__);
					return 0;
				}
				break;
			case ADUPLEX_TASK_EXIT:
				printf("%s task exit!\n", __func__);
				return -EFAULT;
			default:
				printf("%s unsupport cmd val:%d\n", __func__, receive_val);
				break;
			}
		} else {
			printf("%s timeout[%d]\n", __func__, i);
			continue;
		}
	}
	if (i >= 10) {
		ret = -EFAULT;
		*pcm_queue = NULL;
	}

	return ret;
}

#if 0
static int duplex_pcm_queue_fill_data(audio_mgr_t *audio_mgr,
				struct duplex_pcm_queue *pcm_queue, int k)
{
	unsigned int pcm_data_frames = 0;
	int i = 0;
	int j = 0;

	pcm_data_frames = pcm_queue->data_size / snd_pcm_frames_to_bytes(audio_mgr->handle, 1);

	if (pcm_queue == NULL) {
		awalsa_err("\n");
		return -ENOMEM;
	}

	memset(pcm_queue->pcm_data, 0, pcm_queue->data_size);

	/* for every frame */
	for (i = 0; i < pcm_data_frames; i++) {
		/* for every channel data */
		for (j = 0; j < audio_mgr->channels; j++) {
			switch (audio_mgr->format) {
			default:
			case SND_PCM_FORMAT_S16_LE:
				*((unsigned short *)pcm_queue->pcm_data + (i * audio_mgr->channels) + j) =
						ADUPLEX_DATA_MODE + j + i;
						//ADUPLEX_DATA_MODE + j - k;
				break;
			/* FIXME: s24 should write 32bits data. */
			case SND_PCM_FORMAT_S24_LE:
			case SND_PCM_FORMAT_S32_LE:
				*((unsigned int *)pcm_queue->pcm_data + (i * audio_mgr->channels) + j) =
						ADUPLEX_DATA_MODE + j - k;
				break;
			}
		}
	}
//	aduplex_show_pcm_data(audio_mgr, pcm_queue->pcm_data, pcm_queue->data_size);
	return 0;
}

static void aduplex_show_pcm_data(audio_mgr_t *mgr, void *pcm_data,
			unsigned int pcm_data_size)
{
	int i = 0;
	int j = 0;
	unsigned int pcm_data_frames = 0;

	if (pcm_data == NULL) {
		printf("pcm_data is null.\n");
		return;
	}
	if (pcm_data_size == 0) {
		printf("pcm_data_size is 0.\n");
		return;
	}
	pcm_data_frames = pcm_data_size / snd_pcm_frames_to_bytes(mgr->handle, 1);

	/* for every frame */
	for (i = 0; i < pcm_data_frames; i++) {
		printf("\n0x%x: ", i * mgr->channels);
		/* for every channel data */
		for (j = 0; j < mgr->channels; j++) {
			switch (mgr->format) {
			default:
			case SND_PCM_FORMAT_S16_LE:
				printf("0x%x ", *((unsigned short *)pcm_data + (i * mgr->channels) + j));
				break;
			/* FIXME: s24 should write 32bits data. */
			case SND_PCM_FORMAT_S24_LE:
			case SND_PCM_FORMAT_S32_LE:
				printf("0x%x ", *((unsigned int *)pcm_data + (i * mgr->channels) + j));
				break;
			}
		}
	}
	printf("\n");
}
#endif

static int arecord_cthread(void *argv, const char *card_name, audio_mgr_t *audio_mgr)
{
	int ret = 0;
	int mode = 0;
	unsigned long i = 0;
	struct duplex_pcm_queue *pcm_queue = NULL;
	struct duplex_priv *duplex_priv = argv;
	unsigned int pvItemToQueue = 0;
	unsigned int chunk_ms = 0;
	unsigned long capture_count = 0;
	const TickType_t timeout = pdMS_TO_TICKS(20);

	/* open card */
	ret = snd_pcm_open(&audio_mgr->handle, card_name, SND_PCM_STREAM_CAPTURE, mode);
	if (ret < 0) {
		awalsa_err("open error:%d\n", ret);
		return -1;
	}
	ret = set_param(audio_mgr->handle, audio_mgr->format,
			audio_mgr->rate, audio_mgr->channels,
			audio_mgr->period_size, audio_mgr->buffer_size);
	if (ret < 0) {
		awalsa_err("set param error:%d\n", ret);
		goto err_set_pcm_param;
	}

	printf("pcm_read start...\n");
	capture_stop = 0;

	chunk_ms = audio_mgr->chunk_size * 1000 / audio_mgr->rate;
	capture_count = audio_mgr->capture_duration * 1000 / chunk_ms;

	/* FIXME: for fill playback to start */
	for (i = 0; i < (audio_mgr->buffer_size/audio_mgr->period_size) + 2; i++) {
		/* get idle pcm_data */
		pcm_queue = duplex_pcm_queue_get_idle_pcm_queue(capture_pcm_queue);
		if (pcm_queue == NULL)
			awalsa_err("pcm_queue is null.\n");
		/* add buffer list */
		aduplex_pcm_list_add_pcm_queue(duplex_priv, pcm_queue);
		pvItemToQueue = ADUPLEX_PCM_QUEUE_ADD;
		ret = xQueueSendToBack(duplex_priv->pcm_queue, (void *)&pvItemToQueue, timeout);
		if (ret == errQUEUE_FULL)
			awalsa_err("SendToBack failed.\n");
	}

	i = 0;
	do {
		/* get idle pcm_data */
		pcm_queue = duplex_pcm_queue_get_idle_pcm_queue(capture_pcm_queue);
		if (pcm_queue == NULL) {
			awalsa_err("pcm_queue is null. i = %lu. capture_count:%lu\n", i, capture_count);
			break;
		}

		ret = pcm_read(audio_mgr->handle, pcm_queue->pcm_data,
			audio_mgr->chunk_size,
			snd_pcm_frames_to_bytes(audio_mgr->handle, 1));
		if (ret < 0) {
			awalsa_err("capture error:%d\n", ret);
			goto err_pcm_read;
		}

//		duplex_pcm_queue_fill_data(audio_mgr, pcm_queue, i);
		/* add buffer list */
		aduplex_pcm_list_add_pcm_queue(duplex_priv, pcm_queue);

		pvItemToQueue = ADUPLEX_PCM_QUEUE_ADD;
		ret = xQueueSendToBack(duplex_priv->pcm_queue, (void *)&pvItemToQueue, timeout);
		if (ret == errQUEUE_FULL)
			awalsa_err("SendToBack failed.\n");
	} while (++i < capture_count);

	capture_stop = 1;

	ret = snd_pcm_drop(audio_mgr->handle);
	if (ret < 0)
		awalsa_err("stop failed!, return %d\n", ret);

	/* close card */
	ret = snd_pcm_close(audio_mgr->handle);
	if (ret < 0)
		awalsa_err("close error:%d\n", ret);

	return 0;

err_pcm_read:
	ret = snd_pcm_drop(audio_mgr->handle);
	if (ret < 0)
		awalsa_err("stop failed!, return %d\n", ret);

err_set_pcm_param:
	capture_stop = 1;

	/* close card */
	ret = snd_pcm_close(audio_mgr->handle);
	if (ret < 0)
		awalsa_err("close error:%d\n", ret);

	pvItemToQueue = ADUPLEX_TASK_EXIT,
	ret = xQueueSendToBack(duplex_priv->pcm_queue, (void *)&pvItemToQueue, timeout);
	if (ret == errQUEUE_FULL)
		awalsa_err("SendToBack failed.\n");
	return ret;
}

static int aplay_cthread(void *argv, const char *card_name, audio_mgr_t *audio_mgr)
{
	int ret = 0;
	int mode = 0;
	unsigned int pvItemToQueue = 0;
	unsigned int chunk_ms = 0;
	struct duplex_priv *duplex_priv = argv;
	const TickType_t timeout = pdMS_TO_TICKS(20);
	struct duplex_pcm_queue *pcm_queue = NULL;
	unsigned long long i = 0;

	/* open card */
	ret = snd_pcm_open(&audio_mgr->handle, card_name, SND_PCM_STREAM_PLAYBACK, mode);
	if (ret < 0) {
		awalsa_err("open error:%d\n", ret);
		return -1;
	}

	ret = set_param(audio_mgr->handle, audio_mgr->format,
			audio_mgr->rate, audio_mgr->channels,
			audio_mgr->period_size, audio_mgr->buffer_size);
	if (ret < 0) {
		awalsa_err("set param error:%d\n", ret);
		goto err_set_pcm_param;
	}

	chunk_ms = audio_mgr->chunk_size * 1000 / audio_mgr->rate;
	printf("pcm_write start.\n");

	do {
		ret = duplex_pcm_list_get_pcm_queue(duplex_priv, &pcm_queue);
		if (ret != 0) {
			awalsa_err("i=%llu\n", i);
			break;
		}
//		duplex_pcm_queue_fill_data(audio_mgr, pcm_queue, i);

		ret = pcm_write(audio_mgr->handle, pcm_queue->pcm_data,
			snd_pcm_bytes_to_frames(audio_mgr->handle, pcm_queue->data_size),
			snd_pcm_frames_to_bytes(audio_mgr->handle, 1));
		if (ret < 0) {
			awalsa_err("pcm_write error:%d\n", ret);
			goto err_pcm_write;
		}

		/* remove buffer list*/
		duplex_pcm_list_remove_pcm_queue(duplex_priv, pcm_queue);
		i++;
	} while (!capture_stop); /* buffer list is null. */

	ret = snd_pcm_drop(audio_mgr->handle);
	if (ret < 0)
		awalsa_err("failed!, return %d\n", ret);

	/* close card */
	ret = snd_pcm_close(audio_mgr->handle);
	if (ret < 0)
		awalsa_err("close error:%d\n", ret);

	pvItemToQueue = ADUPLEX_TASK_EXIT;
	ret = xQueueSendToBack(duplex_priv->wait_queue, (void *)&pvItemToQueue, timeout);
	if (ret == errQUEUE_FULL)
		awalsa_err("SendToBack failed.\n");

	return 0;

err_pcm_write:
err_set_pcm_param:
	/* close card */
	ret = snd_pcm_close(audio_mgr->handle);
	if (ret < 0)
		awalsa_err("close error:%d\n", ret);

	pvItemToQueue = ADUPLEX_TASK_EXIT;
	ret = xQueueSendToBack(duplex_priv->wait_queue, (void *)&pvItemToQueue, timeout);
	if (ret == errQUEUE_FULL)
		awalsa_err("SendToBack failed.\n");

	return ret;
}

static int arecord_pthread(void *argv, const char *card_name, audio_mgr_t *audio_mgr)
{
	int ret = 0;
	int mode = 0;
	unsigned long long i = 0;
	struct duplex_pcm_queue *pcm_queue = NULL;
	struct duplex_priv *duplex_priv = argv;
	unsigned int pvItemToQueue = 0;
	unsigned int chunk_ms = 0;
	unsigned int capture_count = 0;
	const TickType_t timeout = pdMS_TO_TICKS(20);

	/* open card */
	ret = snd_pcm_open(&audio_mgr->handle, card_name, SND_PCM_STREAM_CAPTURE, mode);
	if (ret < 0) {
		awalsa_err("open error:%d\n", ret);
		return -1;
	}
	ret = set_param(audio_mgr->handle, audio_mgr->format,
			audio_mgr->rate, audio_mgr->channels,
			audio_mgr->period_size, audio_mgr->buffer_size);
	if (ret < 0) {
		awalsa_err("set param error:%d\n", ret);
		goto err_set_pcm_param;
	}

	printf("pcm_read start...\n");
	playback_stop = 0;

	chunk_ms = audio_mgr->chunk_size * 1000 / audio_mgr->rate;
	capture_count = audio_mgr->capture_duration * 1000 / chunk_ms;

	/* FIXME: for fill playback to start */
	for (i = 0; i < (audio_mgr->buffer_size/audio_mgr->period_size) + 2; i++) {
		/* get idle pcm_data */
		pcm_queue = duplex_pcm_queue_get_idle_pcm_queue(playback_pcm_queue);
		if (pcm_queue == NULL)
			awalsa_err("pcm_queue is null.\n");
		/* add buffer list */
		aduplex_pcm_list_add_pcm_queue(duplex_priv, pcm_queue);
		pvItemToQueue = ADUPLEX_PCM_QUEUE_ADD;
		ret = xQueueSendToBack(duplex_priv->pcm_queue, (void *)&pvItemToQueue, timeout);
		if (ret == errQUEUE_FULL)
			awalsa_err("SendToBack failed.\n");
	}

	i = 0;
	do {
		/* get idle pcm_data */
		pcm_queue = duplex_pcm_queue_get_idle_pcm_queue(playback_pcm_queue);
		if (pcm_queue == NULL) {
			awalsa_err("pcm_queue is null.\n");
			break;
		}

		ret = pcm_read(audio_mgr->handle, pcm_queue->pcm_data,
			audio_mgr->chunk_size,
			snd_pcm_frames_to_bytes(audio_mgr->handle, 1));
		if (ret < 0) {
			awalsa_err("capture error:%d\n", ret);
			goto err_pcm_read;
		}

		/* add buffer list */
		aduplex_pcm_list_add_pcm_queue(duplex_priv, pcm_queue);

		pvItemToQueue = ADUPLEX_PCM_QUEUE_ADD;
		ret = xQueueSendToBack(duplex_priv->pcm_queue, (void *)&pvItemToQueue, timeout);
		if (ret == errQUEUE_FULL)
			awalsa_err("SendToBack failed.\n");
	} while (++i < capture_count);

	playback_stop = 1;

	ret = snd_pcm_drop(audio_mgr->handle);
	if (ret < 0)
		awalsa_err("stop failed!, return %d\n", ret);

	/* close card */
	ret = snd_pcm_close(audio_mgr->handle);
	if (ret < 0)
		awalsa_err("close error:%d\n", ret);

	return 0;

err_pcm_read:
	ret = snd_pcm_drop(audio_mgr->handle);
	if (ret < 0)
		awalsa_err("stop failed!, return %d\n", ret);

err_set_pcm_param:
	playback_stop = 1;

	/* close card */
	ret = snd_pcm_close(audio_mgr->handle);
	if (ret < 0)
		awalsa_err("close error:%d\n", ret);

	pvItemToQueue = ADUPLEX_TASK_EXIT,
	ret = xQueueSendToBack(duplex_priv->pcm_queue, (void *)&pvItemToQueue, timeout);
	if (ret == errQUEUE_FULL)
		awalsa_err("SendToBack failed.\n");
	return ret;
}

static int aplay_pthread(void *argv, const char *card_name, audio_mgr_t *audio_mgr)
{
	int ret = 0;
	int mode = 0;
	unsigned int pvItemToQueue = 0;
	unsigned int chunk_ms = 0;
	struct duplex_priv *duplex_priv = argv;
	const TickType_t timeout = pdMS_TO_TICKS(20);
	struct duplex_pcm_queue *pcm_queue = NULL;
	unsigned long long i = 0;

	/* open card */
	ret = snd_pcm_open(&audio_mgr->handle, card_name, SND_PCM_STREAM_PLAYBACK, mode);
	if (ret < 0) {
		awalsa_err("open error:%d\n", ret);
		return -1;
	}

	ret = set_param(audio_mgr->handle, audio_mgr->format,
			audio_mgr->rate, audio_mgr->channels,
			audio_mgr->period_size, audio_mgr->buffer_size);
	if (ret < 0) {
		awalsa_err("set param error:%d\n", ret);
		goto err_set_pcm_param;
	}

	chunk_ms = audio_mgr->chunk_size * 1000 / audio_mgr->rate;

	do {
		ret = duplex_pcm_list_get_pcm_queue(duplex_priv, &pcm_queue);
		if (ret != 0) {
			printf("===%s %d i=%llu===\n", __func__, __LINE__, i);
			break;
		}
//		duplex_pcm_queue_fill_data(audio_mgr, pcm_queue, i);

		ret = pcm_write(audio_mgr->handle, pcm_queue->pcm_data,
			snd_pcm_bytes_to_frames(audio_mgr->handle, pcm_queue->data_size),
			snd_pcm_frames_to_bytes(audio_mgr->handle, 1));
		if (ret < 0) {
			awalsa_err("pcm_write error:%d\n", ret);
			goto err_pcm_write;
		}

		/* remove buffer list*/
		duplex_pcm_list_remove_pcm_queue(duplex_priv, pcm_queue);
		i++;
	} while (!playback_stop); /* buffer list is null. */

	ret = snd_pcm_drop(audio_mgr->handle);
	if (ret < 0)
		awalsa_err("failed!, return %d\n", ret);

	/* close card */
	ret = snd_pcm_close(audio_mgr->handle);
	if (ret < 0)
		awalsa_err("close error:%d\n", ret);

	pvItemToQueue = ADUPLEX_TASK_EXIT;
	ret = xQueueSendToBack(duplex_priv->wait_queue, (void *)&pvItemToQueue, timeout);
	if (ret == errQUEUE_FULL)
		awalsa_err("SendToBack failed.\n");

	return 0;

err_pcm_write:
err_set_pcm_param:
	/* close card */
	ret = snd_pcm_close(audio_mgr->handle);
	if (ret < 0)
		awalsa_err("close error:%d\n", ret);

	pvItemToQueue = ADUPLEX_TASK_EXIT;
	ret = xQueueSendToBack(duplex_priv->wait_queue, (void *)&pvItemToQueue, timeout);
	if (ret == errQUEUE_FULL)
		awalsa_err("SendToBack failed.\n");

	return ret;
}

static void arecord_centry(void * arg)
{
	struct duplex_priv *duplex_priv = arg;

	FUNCTION_THREAD_START_LINE_PRINTF(__func__);
	/* do something */
	arecord_cthread(duplex_priv, duplex_priv->pcm_name, duplex_priv->capture_mgr);
	FUNCTION_THREAD_STOP_LINE_PRINTF(__func__);
	duplex_priv->task_exit |= 0x1 << SND_PCM_STREAM_CAPTURE;
	audio_task_exit |= 0x1 << SND_PCM_STREAM_CAPTURE;

	/* must be call at here */
	vTaskDelete(NULL);
}

static void aplay_centry(void * arg)
{
	struct duplex_priv *duplex_priv = arg;

	FUNCTION_THREAD_START_LINE_PRINTF(__func__);
	/* do something */
	aplay_cthread(duplex_priv, duplex_priv->hpcm_name, duplex_priv->playback_mgr);
	FUNCTION_THREAD_STOP_LINE_PRINTF(__func__);
	duplex_priv->task_exit |= 0x1 << SND_PCM_STREAM_PLAYBACK;
	audio_task_exit |= 0x1 << SND_PCM_STREAM_PLAYBACK;

	/* must be call at here */
	vTaskDelete(NULL);
}

static void arecord_pentry(void * arg)
{
	struct duplex_priv *duplex_priv = arg;

	FUNCTION_THREAD_START_LINE_PRINTF(__func__);
	/* do something */
	arecord_pthread(duplex_priv, duplex_priv->pcm_name, duplex_priv->capture_mgr);
	FUNCTION_THREAD_STOP_LINE_PRINTF(__func__);
	duplex_priv->task_exit |= 0x1 << SND_PCM_STREAM_CAPTURE;

	/* must be call at here */
	vTaskDelete(NULL);
}

static void aplay_pentry(void * arg)
{
	struct duplex_priv *duplex_priv = arg;

	FUNCTION_THREAD_START_LINE_PRINTF(__func__);
	/* do something */
	aplay_pthread(duplex_priv, duplex_priv->hpcm_name, duplex_priv->playback_mgr);
	FUNCTION_THREAD_STOP_LINE_PRINTF(__func__);
	duplex_priv->task_exit |= 0x1 << SND_PCM_STREAM_PLAYBACK;

	/* must be call at here */
	vTaskDelete(NULL);
}

int aduplex_capture(int argc, char ** argv)
{
	int c;
	int ret = 0;
	unsigned int bits = 16;
	TickType_t xTicksToWait = 0;
	int priority = configAPPLICATION_AUDIO_PRIORITY;
	uint32_t thread_size = 8 * 1024;	
	unsigned int direction = 0;
	unsigned int chunk_bytes = 0;

	duplex_priv[direction].pcm_name = "hw:snddaudio0";
	duplex_priv[direction].hpcm_name = "default";

	duplex_priv[direction].playback_mgr = audio_mgr_create();
	if (!duplex_priv[direction].playback_mgr) {
		awalsa_err("playback_mgr malloc failed.\n");
		return -1;
	}

	duplex_priv[direction].capture_mgr = audio_mgr_create();
	if (!duplex_priv[direction].capture_mgr) {
		awalsa_err("capture_mgr malloc failed.\n");
		return -1;
	}

	/* for init */
	duplex_priv[direction].capture_mgr->capture_duration = 5;

	optind = 0;
	while ((c = getopt(argc, argv, "D:H:r:f:c:p:b:t:h")) != -1) {
		switch (c) {
		case 'D':
			duplex_priv[direction].pcm_name = optarg;
			break;
		case 'H':
			duplex_priv[direction].hpcm_name = optarg;
			break;
		case 'r':
			duplex_priv[direction].playback_mgr->rate = atoi(optarg);
			duplex_priv[direction].capture_mgr->rate = atoi(optarg);
			break;
		case 'f':
			bits = atoi(optarg);
			break;
		case 'c':
			duplex_priv[direction].playback_mgr->channels = atoi(optarg);
			duplex_priv[direction].capture_mgr->channels = atoi(optarg);
			break;
		case 'p':
			duplex_priv[direction].playback_mgr->period_size = atoi(optarg);
			duplex_priv[direction].capture_mgr->period_size = atoi(optarg);
			break;
		case 'b':
			duplex_priv[direction].playback_mgr->buffer_size = atoi(optarg);
			duplex_priv[direction].capture_mgr->buffer_size = atoi(optarg);
			break;
		case 't':
			duplex_priv[direction].capture_mgr->capture_duration = atoi(optarg);
			break;
		case 'h':
		default:
			printf("%s", ADUPLEX_COMMOND_HELP);
			return -1;
		}
	}

	/* check params */
	if (duplex_priv[direction].capture_mgr->capture_duration > ADUPLEX_TIME_MAX)
		duplex_priv[direction].capture_mgr->capture_duration = ADUPLEX_TIME_MAX;
	if (duplex_priv[direction].capture_mgr->capture_duration <= 0)
		duplex_priv[direction].capture_mgr->capture_duration = ADUPLEX_TIME_MAX/2;
	duplex_priv[direction].playback_mgr->capture_duration =
			duplex_priv[direction].capture_mgr->capture_duration;
	xTicksToWait = pdMS_TO_TICKS((duplex_priv[direction].capture_mgr->capture_duration + 10) * 1000);

	switch (bits) {
	case 16:
		duplex_priv[direction].playback_mgr->format = SND_PCM_FORMAT_S16_LE;
		duplex_priv[direction].capture_mgr->format = SND_PCM_FORMAT_S16_LE;
		break;
	case 24:
		duplex_priv[direction].playback_mgr->format = SND_PCM_FORMAT_S24_LE;
		duplex_priv[direction].capture_mgr->format = SND_PCM_FORMAT_S24_LE;
		break;
	case 32:
		duplex_priv[direction].playback_mgr->format = SND_PCM_FORMAT_S32_LE;
		duplex_priv[direction].capture_mgr->format = SND_PCM_FORMAT_S32_LE;
		break;
	default:
		awalsa_err("%u bits not supprot\n", bits);
		return -1;
	}

	duplex_priv[direction].capture_mgr->chunk_size = duplex_priv[direction].capture_mgr->period_size;
	duplex_priv[direction].playback_mgr->chunk_size =
		duplex_priv[direction].playback_mgr->period_size;
	chunk_bytes = duplex_priv[direction].capture_mgr->chunk_size *
			duplex_priv[direction].capture_mgr->channels * bits / 8;

	ret = duplex_pcm_queue_init(playback_pcm_queue, chunk_bytes);
	if (ret < 0) {
		awalsa_err("pmc_queue_init error:%d\n", ret);
		return -1;
	}

	duplex_priv[direction].list_semaph = aduplex_mutex_init();
	if (duplex_priv[direction].list_semaph == NULL) {
		awalsa_err("create mutex failed.\n");
		return -1;
	}

	duplex_priv[direction].pcm_queue = xQueueCreate(ADUPLEX_PCM_QUEUE_SIZE, sizeof(unsigned int));
	if (duplex_priv[direction].pcm_queue == NULL) {
		awalsa_err("create pcm queue failed.\n");
		return -1;
	}

	duplex_priv[direction].wait_queue = xQueueCreate(ADUPLEX_WAIT_QUEUE_SIZE, sizeof(unsigned int));
	if (duplex_priv[direction].wait_queue == NULL) {
		awalsa_err("create wait queue failed.\n");
		return -1;
	}

	duplex_pcm_list_init(&duplex_priv[direction]);
	//record first.
	ret = xTaskCreate(arecord_centry, "arecord-cthread", thread_size,
			&duplex_priv[direction], priority, &duplex_priv[direction].record_task);
	if (ret != pdPASS) {
		awalsa_err("arecord-pthread create failed.\n");
		return -1;
	}

	ret = xTaskCreate(aplay_centry, "aplay-cthread", thread_size,
			&duplex_priv[direction], priority, &duplex_priv[direction].playback_task);
	if (ret != pdPASS) {
		awalsa_err("aplay-pthread create failed.\n");
		return -1;
	}

	return 0;
}

int aduplex_playback(int argc, char ** argv)
{
	int c;
	int ret = 0;
	unsigned int bits = 16;
	TickType_t xTicksToWait = 0;
	int priority = configAPPLICATION_AUDIO_PRIORITY;
	uint32_t thread_size = 4 * 1024;
	unsigned int direction = 1;
	unsigned int chunk_bytes = 0;

	duplex_priv[direction].pcm_name = "hw:snddaudio0";
	duplex_priv[direction].hpcm_name = "default";

	duplex_priv[direction].playback_mgr = audio_mgr_create();
	if (!duplex_priv[direction].playback_mgr) {
		awalsa_err("playback_mgr malloc failed.\n");
		return -1;
	}

	duplex_priv[direction].capture_mgr = audio_mgr_create();
	if (!duplex_priv[direction].capture_mgr) {
		awalsa_err("capture_mgr malloc failed.\n");
		return -1;
	}

	/* for init */
	duplex_priv[direction].capture_mgr->capture_duration = 5;

	optind = 0;
	while ((c = getopt(argc, argv, "D:H:r:f:c:p:b:t:")) != -1) {
		switch (c) {
		case 'D':
			duplex_priv[direction].pcm_name = optarg;
			break;
		case 'H':
			duplex_priv[direction].hpcm_name = optarg;
			break;
		case 'r':
			duplex_priv[direction].playback_mgr->rate = atoi(optarg);
			duplex_priv[direction].capture_mgr->rate = atoi(optarg);
			break;
		case 'f':
			bits = atoi(optarg);
			break;
		case 'c':
			duplex_priv[direction].playback_mgr->channels = atoi(optarg);
			duplex_priv[direction].capture_mgr->channels = atoi(optarg);
			break;
		case 'p':
			duplex_priv[direction].playback_mgr->period_size = atoi(optarg);
			duplex_priv[direction].capture_mgr->period_size = atoi(optarg);
			break;
		case 'b':
			duplex_priv[direction].playback_mgr->buffer_size = atoi(optarg);
			duplex_priv[direction].capture_mgr->buffer_size = atoi(optarg);
			break;
		case 't':
			duplex_priv[direction].capture_mgr->capture_duration = atoi(optarg);
			break;
		default:
			return -1;
		}
	}

	/* check params */
	if (duplex_priv[direction].capture_mgr->capture_duration > ADUPLEX_TIME_MAX)
		duplex_priv[direction].capture_mgr->capture_duration = ADUPLEX_TIME_MAX;
	if (duplex_priv[direction].capture_mgr->capture_duration <= 0)
		duplex_priv[direction].capture_mgr->capture_duration = ADUPLEX_TIME_MAX/2;
	duplex_priv[direction].playback_mgr->capture_duration =
			duplex_priv[direction].capture_mgr->capture_duration;
	xTicksToWait = pdMS_TO_TICKS((duplex_priv[direction].capture_mgr->capture_duration + 10) * 1000);

	switch (bits) {
	case 16:
		duplex_priv[direction].playback_mgr->format = SND_PCM_FORMAT_S16_LE;
		duplex_priv[direction].capture_mgr->format = SND_PCM_FORMAT_S16_LE;
		break;
	case 24:
		duplex_priv[direction].playback_mgr->format = SND_PCM_FORMAT_S24_LE;
		duplex_priv[direction].capture_mgr->format = SND_PCM_FORMAT_S24_LE;
		break;
	case 32:
		duplex_priv[direction].playback_mgr->format = SND_PCM_FORMAT_S32_LE;
		duplex_priv[direction].capture_mgr->format = SND_PCM_FORMAT_S32_LE;
		break;
	default:
		awalsa_err("%u bits not supprot\n", bits);
		return -1;
	}

	duplex_priv[direction].capture_mgr->chunk_size = duplex_priv[direction].capture_mgr->period_size;
	duplex_priv[direction].playback_mgr->chunk_size = duplex_priv[direction].playback_mgr->period_size;
	chunk_bytes = duplex_priv[direction].capture_mgr->chunk_size *
			duplex_priv[direction].capture_mgr->channels * bits / 8;

	ret = duplex_pcm_queue_init(playback_pcm_queue, chunk_bytes);
	if (ret < 0) {
		awalsa_err("pmc_queue_init error:%d\n", ret);
		return -1;
	}

	duplex_priv[direction].list_semaph = aduplex_mutex_init();
	if (duplex_priv[direction].list_semaph == NULL) {
		awalsa_err("create mutex failed.\n");
		return -1;
	}

	duplex_priv[direction].pcm_queue = xQueueCreate(ADUPLEX_PCM_QUEUE_SIZE, sizeof(unsigned int));
	if (duplex_priv[direction].pcm_queue == NULL) {
		awalsa_err("create pcm queue failed.\n");
		return -1;
	}

	duplex_priv[direction].wait_queue = xQueueCreate(ADUPLEX_WAIT_QUEUE_SIZE, sizeof(unsigned int));
	if (duplex_priv[direction].wait_queue == NULL) {
		awalsa_err("create wait queue failed.\n");
		return -1;
	}

	duplex_pcm_list_init(&duplex_priv[direction]);

	ret = xTaskCreate(aplay_pentry, "aplay-pthread", thread_size,
			&duplex_priv[direction], priority, &duplex_priv[direction].playback_task);
	if (ret != pdPASS) {
		awalsa_err("aplay-pthread create failed.\n");
		return -1;
	}

	ret = xTaskCreate(arecord_pentry, "arecord-pthread", thread_size,
			&duplex_priv[direction], priority, &duplex_priv[direction].record_task);
	if (ret != pdPASS) {
		awalsa_err("arecord-pthread create failed.\n");
		return -1;
	}

	return 0;
}

int aduplex(int argc, char ** argv)
{
	int c;
	int ret = 0;
	unsigned int bits = 16;
	TickType_t xTicksToWait = 0;
	int priority = configAPPLICATION_AUDIO_PRIORITY;
	uint32_t thread_size = 2 * 1024;
	unsigned int receive_val = 0;
	BaseType_t xStatus;
	unsigned int chunk_bytes = 0;
	unsigned int direction = 0;

	duplex_priv[direction].pcm_name = "hw:snddaudio0";
	duplex_priv[direction].hpcm_name = "default";

	duplex_priv[direction].playback_mgr = audio_mgr_create();
	if (!duplex_priv[direction].playback_mgr) {
		awalsa_err("playback_mgr malloc failed.\n");
		return -1;
	}

	duplex_priv[direction].capture_mgr = audio_mgr_create();
	if (!duplex_priv[direction].capture_mgr) {
		awalsa_err("capture_mgr malloc failed.\n");
		goto err_malloc_capture_mgr;
	}

	/* for init */
	duplex_priv[direction].capture_mgr->capture_duration = 5;

	optind = 0;
	while ((c = getopt(argc, argv, "D:H:r:f:c:p:b:t:")) != -1) {
		switch (c) {
		case 'D':
			duplex_priv[direction].pcm_name = optarg;
			break;
		case 'H':
			duplex_priv[direction].hpcm_name = optarg;
			break;
		case 'r':
			duplex_priv[direction].playback_mgr->rate = atoi(optarg);
			duplex_priv[direction].capture_mgr->rate = atoi(optarg);
			break;
		case 'f':
			bits = atoi(optarg);
			break;
		case 'c':
			duplex_priv[direction].playback_mgr->channels = atoi(optarg);
			duplex_priv[direction].capture_mgr->channels = atoi(optarg);
			break;
		case 'p':
			duplex_priv[direction].playback_mgr->period_size = atoi(optarg);
			duplex_priv[direction].capture_mgr->period_size = atoi(optarg);
			break;
		case 'b':
			duplex_priv[direction].playback_mgr->buffer_size = atoi(optarg);
			duplex_priv[direction].capture_mgr->buffer_size = atoi(optarg);
			break;
		case 't':
			duplex_priv[direction].capture_mgr->capture_duration = atoi(optarg);
			break;
		default:
			goto err_cmd;
		}
	}

	/* check params */
	if (duplex_priv[direction].capture_mgr->capture_duration > ADUPLEX_TIME_MAX)
		duplex_priv[direction].capture_mgr->capture_duration = ADUPLEX_TIME_MAX;
	if (duplex_priv[direction].capture_mgr->capture_duration <= 0)
		duplex_priv[direction].capture_mgr->capture_duration = ADUPLEX_TIME_MAX/2;
	duplex_priv[direction].playback_mgr->capture_duration =
		duplex_priv[direction].capture_mgr->capture_duration;

	xTicksToWait = (duplex_priv[direction].capture_mgr->capture_duration + 10) * configTICK_RATE_HZ;
	awalsa_err("capture_duration:%u, xTicksToWait:%u\n",
			duplex_priv[direction].capture_mgr->capture_duration, xTicksToWait);

	switch (bits) {
	case 16:
		duplex_priv[direction].playback_mgr->format = SND_PCM_FORMAT_S16_LE;
		duplex_priv[direction].capture_mgr->format = SND_PCM_FORMAT_S16_LE;
		break;
	case 24:
		duplex_priv[direction].playback_mgr->format = SND_PCM_FORMAT_S24_LE;
		duplex_priv[direction].capture_mgr->format = SND_PCM_FORMAT_S24_LE;
		break;
	case 32:
		duplex_priv[direction].playback_mgr->format = SND_PCM_FORMAT_S32_LE;
		duplex_priv[direction].capture_mgr->format = SND_PCM_FORMAT_S32_LE;
		break;
	default:
		awalsa_err("%u bits not supprot\n", bits);
		goto err_format;
	}

	duplex_priv[direction].capture_mgr->chunk_size = duplex_priv[direction].capture_mgr->period_size;
	duplex_priv[direction].playback_mgr->chunk_size = duplex_priv[direction].playback_mgr->period_size;
	chunk_bytes = duplex_priv[direction].capture_mgr->chunk_size *
			duplex_priv[direction].capture_mgr->channels * bits / 8;

	ret = duplex_pcm_queue_init(capture_pcm_queue, chunk_bytes);
	if (ret < 0) {
		awalsa_err("pmc_queue_init error:%d\n", ret);
		goto err_pcm_queue_init;
	}

	duplex_priv[direction].list_semaph = aduplex_mutex_init();
	if (duplex_priv[direction].list_semaph == NULL) {
		awalsa_err("create mutex failed.\n");
		ret =  -EFAULT;
		goto err_mutex_create;
	}

	duplex_priv[direction].pcm_queue = xQueueCreate(ADUPLEX_PCM_QUEUE_SIZE, sizeof(unsigned int));
	if (duplex_priv[direction].pcm_queue == NULL) {
		awalsa_err("create pcm queue failed.\n");
		ret =  -EFAULT;
		goto err_pcm_queue_create;
	}

	duplex_priv[direction].wait_queue = xQueueCreate(ADUPLEX_WAIT_QUEUE_SIZE, sizeof(unsigned int));
	if (duplex_priv[direction].wait_queue == NULL) {
		awalsa_err("create wait queue failed.\n");
		ret =  -EFAULT;
		goto err_wait_queue_create;
	}

	duplex_pcm_list_init(duplex_priv);

	//record first.
	ret = xTaskCreate(arecord_centry, "arecord-cthread", thread_size,
			&duplex_priv[direction], priority, &duplex_priv[direction].record_task);
	if (ret != pdPASS) {
		awalsa_err("arecord-thread create failed.\n");
		goto err_create_arecord;
	}

	ret = xTaskCreate(aplay_centry, "aplay-cthread", thread_size,
			&duplex_priv[direction], priority, &duplex_priv[direction].playback_task);
	if (ret != pdPASS) {
		awalsa_err("aplay-thread create failed.\n");
		goto err_create_aplay;
	}

	/* wait for capturing finished */
	for( ;; ) {
		if((ret = uxQueueMessagesWaiting(duplex_priv[direction].wait_queue)) != 0) {
			awalsa_print("%s QM waiting:%d\n", __func__, ret);
		}

		xStatus = xQueueReceive(duplex_priv[direction].wait_queue, &receive_val, xTicksToWait);
		if( xStatus == pdPASS ) {
			printf("------> Received = %d <------\n", receive_val);
			switch (receive_val) {
			case ADUPLEX_TASK_EXIT:
				goto exit_handle;
			default:
				break;
			}
		} else {
			printf("------> Received failed <------\n");
			break;
		}
	}

exit_handle:
	for (;;) {
		if ((audio_task_exit & (0x1 << SND_PCM_STREAM_PLAYBACK)) &&
			(audio_task_exit & (0x1 << SND_PCM_STREAM_CAPTURE))) {
			printf("audio_task_exit!!!\n");
			break;
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
	duplex_pcm_list_delete_all_pcm_queue(&duplex_priv[direction]);
	duplex_pcm_queue_free(capture_pcm_queue);

	vQueueDelete(duplex_priv[direction].wait_queue);
	vQueueDelete(duplex_priv[direction].pcm_queue);

	aduplex_mutex_delete(duplex_priv[direction].list_semaph);

//	vTaskDelete(pxPlayCreatedTask);
//	vTaskDelete(pxRecordCreatedTask);

	audio_mgr_release(duplex_priv[direction].capture_mgr);
	audio_mgr_release(duplex_priv[direction].playback_mgr);

	memset(&duplex_priv[direction], 0, sizeof(struct duplex_priv));
	audio_task_exit = 0;
	return 0;

err_create_aplay:
	vTaskDelete(duplex_priv[direction].record_task);
err_create_arecord:
	vQueueDelete(duplex_priv[direction].wait_queue);
	duplex_pcm_queue_free(capture_pcm_queue);
err_wait_queue_create:
	vQueueDelete(duplex_priv[direction].pcm_queue);
err_pcm_queue_create:
	aduplex_mutex_delete(duplex_priv[direction].list_semaph);
err_mutex_create:
err_pcm_queue_init:
err_format:
err_cmd:
	audio_mgr_release(duplex_priv[direction].capture_mgr);
err_malloc_capture_mgr:
	audio_mgr_release(duplex_priv[direction].playback_mgr);
	memset(&duplex_priv[direction], 0, sizeof(struct duplex_priv));
	return ret;
}
#ifdef CONFIG_COMPONENTS_FREERTOS_CLI
FINSH_FUNCTION_EXPORT_CMD(aduplex, aduplex, duplex test);
#endif
