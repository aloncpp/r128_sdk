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

#include <aw_rpaf/substream.h>
#include <aw_rpaf/component.h>
#include <aw_rpaf/common.h>
#include <aw_list.h>
#include <delay.h>
#include "standby.h"
#include <event_groups.h>
#include "../aw-alsa-utils/common.h"

#define AWRPAF_SUBSTREAM_AUDIO_STANDBY

static xTaskHandle gVoiceTask;

/* xSemaphoreCreateMutex */
extern struct arpaf_priv *arpaf_priv;

static void awpcm_xrun(snd_pcm_t *handle)
{
	int32_t ret;

	printf("%s occurred.\n", __func__);
	ret = snd_pcm_prepare(handle);
	if (ret < 0) {
		printf("prepare failed in xrun. return %d\n", ret);
	}
}

static int32_t awpcm_read(snd_pcm_t *handle, const char *data, snd_pcm_uframes_t frames_total,
		uint32_t frame_bytes)
{
	int32_t ret = 0;
	snd_pcm_sframes_t size;
	snd_pcm_uframes_t frames_loop = 512;
	snd_pcm_uframes_t frames_count = 0;
	snd_pcm_uframes_t frames = 0;
	uint32_t offset = 0;

	if ((handle == NULL) || (data == NULL) || (frames_total == 0))
		return -EFAULT;

	while (1) {
		if ((frames_total - frames_count) < frames_loop)
			frames = frames_total - frames_count;
		if (frames == 0)
			frames = frames_loop;
		size = snd_pcm_readi(handle, (void *)(data + offset), frames);
		if (size == -EAGAIN) {
			/* retry */
			vTaskDelay(pdMS_TO_TICKS(10));
			continue;
		} else if (size == -EPIPE) {
			awpcm_xrun(handle);
			continue;
		} else if (size == -ESTRPIPE) {
			continue;
		} else if (size < 0) {
			printf("snd_pcm_readi failed return %ld\n", size);
			ret = (int)size;
			goto err_func;
		}
		offset += (size * frame_bytes);
		frames_count += size;
		frames -= size;
		if (frames_total == frames_count)
			break;
	}
err_func:
	return frames_count > 0 ? frames_count : ret;
}

static int32_t awpcm_write(snd_pcm_t *handle, char *data, snd_pcm_uframes_t frames_total,
		uint32_t frame_bytes)
{
	snd_pcm_sframes_t size;
	snd_pcm_uframes_t frames_loop = 512;
	snd_pcm_uframes_t frames_count = 0;
	snd_pcm_uframes_t frames = 0;

	if ((handle == NULL) || (data == NULL) || (frames_total == 0))
		return -EFAULT;

	while (1) {
		if ((frames_total - frames_count) < frames_loop)
			frames = frames_total - frames_count;
		if (frames == 0)
			frames = frames_loop;
		/*usleep(500000);*/
		size = snd_pcm_writei(handle, data, frames);
		if (size != frames)
			printf("snd_pcm_writei return %ld\n", size);

		if (size == -EAGAIN) {
			vTaskDelay(pdMS_TO_TICKS(10));
			continue;
		} else if (size == -EPIPE) {
			awpcm_xrun(handle);
			continue;
		} else if (size == -ESTRPIPE) {
			continue;
		} else if (size < 0) {
			printf("snd_pcm_writei failed return %ld\n", size);
			return size;
		}
		data += (size * frame_bytes);
		frames_count += size;
		frames -= size;
		if (frames_total == frames_count)
			break;
	}

	return frames_count;
}

int32_t hal_substream_sram_buf_sched_timeout(SemaphoreHandle_t sched, uint32_t ticks)
{
	BaseType_t ret;

	if (!sched)
		return -EFAULT;
	ret = xSemaphoreTake(sched, ticks);
	if (ret == pdPASS)
		return 0;
	return -EFAULT;
}

void hal_substream_sram_buf_sched_wakeup(SemaphoreHandle_t sched)
{
	BaseType_t ret;

	if (!sched)
		return;
	ret = xSemaphoreGive(sched);
	if (ret == pdPASS)
		return;
}

SemaphoreHandle_t hal_substream_sram_buf_sched_init(void)
{
	return xSemaphoreCreateBinary();
}

void hal_substream_sram_buf_sched_delete(SemaphoreHandle_t sched)
{
	vSemaphoreDelete(sched);
}

static struct snd_dsp_hal_substream * arpaf_hal_substream_list_malloc_add_tail(
			struct arpaf_priv *arpaf_priv,
			struct snd_soc_dsp_substream *soc_substream)
{
	struct snd_dsp_hal_substream *hal_substream;
	QueueHandle_t dsp_semaph = NULL;
	struct list_head *dsp_list = NULL;

	if (!arpaf_priv) {
		awrpaf_err("\n");
		return NULL;
	}
	if (!soc_substream) {
		awrpaf_err("\n");
		return NULL;
	}
	dsp_semaph = arpaf_priv->hal_substream_list_mutex;
	dsp_list = &arpaf_priv->list_hal_substream;

	hal_substream = rpaf_malloc(sizeof(struct snd_dsp_hal_substream));
	if (!hal_substream) {
		awrpaf_err("\n");
		return NULL;
	}
	memset(hal_substream, 0, sizeof(struct snd_dsp_hal_substream));

	hal_substream->xTaskCreateEvent = xEventGroupCreate();
	if (!hal_substream->xTaskCreateEvent) {
		awrpaf_err("\n");
		rpaf_free(hal_substream);
		return NULL;
	}

	hal_substream->soc_substream = rpaf_malloc(sizeof(struct snd_soc_dsp_substream));
	if (!hal_substream->soc_substream) {
		awrpaf_err("\n");
		rpaf_free(hal_substream);
		return NULL;
	}
	memcpy(hal_substream->soc_substream, soc_substream,
			sizeof(struct snd_soc_dsp_substream));

	hal_substream->stream_mutex = arpaf_mutex_init();
	if (!hal_substream->stream_mutex) {
		awrpaf_err("\n");
		rpaf_free(hal_substream->soc_substream);
		rpaf_free(hal_substream);
		return NULL;
	}
	arpaf_mutex_lock(dsp_semaph);
	list_add_tail(&hal_substream->list, dsp_list);
	arpaf_mutex_unlock(dsp_semaph);

	return hal_substream;
}

#if 0
static int32_t arpaf_hal_substream_list_remove_free(struct arpaf_priv *arpaf_priv,
			    struct snd_dsp_hal_substream *hal_substream)
{
	struct snd_dsp_hal_substream *c = NULL;
	struct snd_dsp_hal_substream *tmp = NULL;
	QueueHandle_t dsp_semaph = NULL;
	struct list_head *dsp_list = NULL;

	if (!arpaf_priv) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	if (!hal_substream) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	dsp_semaph = arpaf_priv->hal_substream_list_mutex;
	dsp_list = &arpaf_priv->list_hal_substream;

	arpaf_mutex_lock(dsp_semaph);
	if (list_empty(dsp_list)) {
		awrpaf_err("\n");
		arpaf_mutex_unlock(dsp_semaph);
		return -EFAULT;
	}
	list_for_each_entry_safe(c, tmp, dsp_list, list) {
		if (c == hal_substream) {
			list_del(&c->list);
			rpaf_free(hal_substream->soc_substream);
			rpaf_free(hal_substream);
			arpaf_mutex_unlock(dsp_semaph);
			return 0;
		}
	}
	arpaf_mutex_unlock(dsp_semaph);
	return -EFAULT;
}

static int32_t arpaf_hal_substream_list_delete(struct arpaf_priv *arpaf_priv,
			    struct snd_dsp_hal_substream *hal_substream)
{
	struct snd_dsp_hal_substream *c = NULL;
	struct snd_dsp_hal_substream *tmp = NULL;
	QueueHandle_t dsp_semaph = NULL;
	struct list_head *dsp_list = NULL;

	if (!arpaf_priv) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	if (!hal_substream) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	dsp_semaph = arpaf_priv->hal_substream_list_mutex;
	dsp_list = &arpaf_priv->list_hal_substream;

	arpaf_mutex_lock(dsp_semaph);
	if (list_empty(dsp_list)) {
		awrpaf_err("\n");
		arpaf_mutex_unlock(dsp_semaph);
		return -EFAULT;
	}

	list_for_each_entry_safe(c, tmp, dsp_list, list) {
		list_del(&hal_substream->list);
	}

	list_del_init(dsp_list);
	arpaf_mutex_unlock(dsp_semaph);

	return 0;
}

struct snd_soc_dsp_substream *snd_soc_dsp_substream_get_from_list_by_card_device_stream(
	struct arpaf_priv *arpaf_priv, uint32_t card, uint32_t device,
	uint32_t stream)
{
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params;
	QueueHandle_t dsp_semaph = NULL;

	if (!arpaf_priv) {
		awrpaf_err("\n");
		return NULL;
	}
	dsp_semaph = arpaf_priv->soc_substream_list_mutex;

	arpaf_mutex_lock(dsp_semaph);
	list_for_each_entry(soc_substream, &arpaf_priv->list_soc_substream, list) {
		pcm_params = &soc_substream->params;
		if ((pcm_params->card == card) &&
			(pcm_params->device == device) &&
			(pcm_params->stream == stream)) {
			arpaf_mutex_unlock(dsp_semaph);
			return soc_substream;
		}
	}
	arpaf_mutex_unlock(dsp_semaph);

	return NULL;
}

struct snd_soc_dsp_substream *snd_soc_dsp_substream_get_from_list_by_name_stream(
				struct arpaf_priv *arpaf_priv, const char *name,
				uint32_t stream)
{
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params;
	QueueHandle_t dsp_semaph = NULL;

	if (!arpaf_priv) {
		awrpaf_err("\n");
		return NULL;
	}
	dsp_semaph = arpaf_priv->soc_substream_list_mutex;

	arpaf_mutex_lock(dsp_semaph);
	list_for_each_entry(soc_substream, &arpaf_priv->list_soc_substream, list) {
		pcm_params = &soc_substream->params;
		if (!strncmp(pcm_params->driver, name, 32) &&
			(pcm_params->stream == stream)) {
			arpaf_mutex_unlock(dsp_semaph);
			return soc_substream;
		}
	}
	arpaf_mutex_unlock(dsp_semaph);

	return NULL;
}

struct snd_dsp_hal_substream *snd_dsp_hal_substream_get_from_list_by_name(
		struct arpaf_priv *arpaf_priv, const char *name)
{
	struct snd_dsp_hal_substream *hal_substream;
	QueueHandle_t dsp_semaph = NULL;
	struct list_head *dsp_list = NULL;

	if (!arpaf_priv) {
		awrpaf_err("\n");
		return NULL;
	}
	if (!hal_substream) {
		awrpaf_err("\n");
		return NULL;
	}
	dsp_semaph = arpaf_priv->hal_substream_list_mutex;
	dsp_list = &arpaf_priv->list_hal_substream;

	arpaf_mutex_lock(dsp_semaph);
	list_for_each_entry(hal_substream, dsp_list, list) {
		if (!strcmp(hal_substream->name, name)) {
			arpaf_mutex_unlock(dsp_semaph);
			return hal_substream;
		}
	}
	arpaf_mutex_unlock(dsp_semaph);

	return NULL;
}
#endif

struct snd_dsp_hal_substream *snd_dsp_hal_substream_get_from_list_by_card_device_stream(
	struct arpaf_priv *arpaf_priv, uint32_t card, uint32_t device, uint32_t stream)
{
	struct snd_dsp_hal_substream *hal_substream;
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params;
	QueueHandle_t dsp_semaph = NULL;
	struct list_head *dsp_list = NULL;

	if (!arpaf_priv) {
		awrpaf_err("\n");
		return NULL;
	}
	dsp_semaph = arpaf_priv->hal_substream_list_mutex;
	dsp_list = &arpaf_priv->list_hal_substream;

	arpaf_mutex_lock(dsp_semaph);
	list_for_each_entry(hal_substream, dsp_list, list) {
		if (hal_substream->soc_substream) {
			soc_substream = hal_substream->soc_substream;
			pcm_params = &soc_substream->params;
			if ((pcm_params->card == card) &&
				(pcm_params->device == device) &&
				(pcm_params->stream == stream)) {
				arpaf_mutex_unlock(dsp_semaph);
				return hal_substream;
			}
		}
	}
	arpaf_mutex_unlock(dsp_semaph);

	return NULL;
}

#ifdef AWRPAF_SUBSTREAM_AUDIO_STANDBY
static int32_t snd_dsp_hal_substream_sram_buf_init(struct snd_dsp_hal_substream *hal_substream)
{
	uint32_t i = 0;
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_substream->params;
	uint32_t frame_bytes = 0;
	uint32_t bits = 0;
	uint32_t sram_number = (pcm_params->rate / pcm_params->period_size) + 1;

	if (sram_number >= SRAM_PCM_CACHE_QUEUE)
		sram_number = SRAM_PCM_CACHE_QUEUE;

	switch (pcm_params->format) {
	default:
	case SND_PCM_FORMAT_S16:
	case SND_PCM_FORMAT_U16:
		bits = 16;
		break;
	case SND_PCM_FORMAT_S24:
	case SND_PCM_FORMAT_U24:
	case SND_PCM_FORMAT_S32:
	case SND_PCM_FORMAT_U32:
		bits = 32;
		break;
	}
	frame_bytes = pcm_params->channels * bits >> 3;

	for (i = 0; i < sram_number; i++) {
		if (hal_substream->sram_buf[i].buf_addr)
			continue;
		memset(&hal_substream->sram_buf[i], 0, sizeof(struct sram_buffer));
		/* 预分配1s缓存时间, 根据实际优化空间 */
		hal_substream->sram_buf[i].buf_addr = rpaf_malloc(
				pcm_params->period_size * frame_bytes);
		if (hal_substream->sram_buf[i].buf_addr == NULL) {
			awrpaf_err("cache malloc failed.\n");
			goto err_malloc_sram;
		}
//		awrpaf_err("sram_buf[%d]:%p, buf_addr:%p\n", i,
//				&hal_substream->sram_buf[i],
//				hal_substream->sram_buf[i].buf_addr);
	}

	return 0;
err_malloc_sram:
	for (i = 0; i < sram_number; i++) {
		if (hal_substream->sram_buf[i].buf_addr) {
			rpaf_free(hal_substream->sram_buf[i].buf_addr);
			hal_substream->sram_buf[i].buf_addr = NULL;
		}
	}
	return -ENOMEM;
}

static int32_t sram_buf_queue_set_using(struct sram_buffer *sram_buf,
				uint32_t enable)
{
	if (!sram_buf) {
		awalsa_err("\n");
		return -EFAULT;
	}
	sram_buf->used = enable;

	return 0;
}

static int32_t sram_buf_queue_get_using(struct sram_buffer *sram_buf,
				uint32_t *enable)
{
	if (!sram_buf) {
		awalsa_err("\n");
		return -EFAULT;
	}
	*enable = sram_buf->used;

	return 0;
}

static int32_t wait_for_sram_buf_avail(struct snd_dsp_hal_substream *hal_substream)
{
	uint32_t tout = 0;

	for (;;) {
		/* 等待1s */
		tout = hal_substream_sram_buf_sched_timeout(hal_substream->srambuf_tsleep,
						pdMS_TO_TICKS(1 * 1000));
		if (tout < 0) {
			awrpaf_err("timeout!\n");
			return -EIO;
		} else
			break;
	}
	return 0;
}

static struct sram_buffer *sram_buf_get_idle_sram_buf(
		struct snd_dsp_hal_substream *hal_substream)
{
	int32_t i = 0;
	uint32_t enable = 0;
	QueueHandle_t dsp_semaph = NULL;
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params;
	uint32_t sram_number;

	if (!hal_substream) {
		awrpaf_err("\n");
		return NULL;
	}
	dsp_semaph = hal_substream->sram_buf_list_mutex;
	pcm_params = &soc_substream->params;
	sram_number = (pcm_params->rate / pcm_params->period_size) + 1;

	arpaf_mutex_lock(dsp_semaph);

	if (sram_number >= SRAM_PCM_CACHE_QUEUE)
		sram_number = SRAM_PCM_CACHE_QUEUE;

	for (i = 0; i < sram_number; i++) {
		sram_buf_queue_get_using(&(hal_substream->sram_buf[i]), &enable);
		if (enable == 0) {
//			awrpaf_debug("sram_buf:%p\n", &hal_substream->sram_buf[i]);
			arpaf_mutex_unlock(dsp_semaph);
			return &(hal_substream->sram_buf[i]);
		}
	}
	arpaf_mutex_unlock(dsp_semaph);

	return NULL;
}

int32_t snd_dsp_substream_list_sram_buf_get_head_buf(
		struct snd_dsp_hal_substream *hal_substream,
		struct sram_buffer **sram_buf)
{
	struct sram_buffer *c = NULL;
	QueueHandle_t dsp_semaph = NULL;
	struct list_head *dsp_list = NULL;
	int32_t ret = 0;

	if (!hal_substream) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	if (!sram_buf) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	dsp_semaph = hal_substream->sram_buf_list_mutex;
	dsp_list = &hal_substream->list_sram_buf;

	arpaf_mutex_lock(dsp_semaph);

	while (list_empty(dsp_list)) {
		arpaf_mutex_unlock(dsp_semaph);
		/* 等待有数据 */
//		awrpaf_debug("\n");
		ret = wait_for_sram_buf_avail(hal_substream);
		if (ret < 0) {
			awrpaf_err("\n");
			*sram_buf = NULL;
			return -EFAULT;
		}
		arpaf_mutex_lock(dsp_semaph);
	}
	c = list_first_entry(dsp_list, struct sram_buffer, list);
	if (!c) {
		awrpaf_err("\n");
		*sram_buf = NULL;
		arpaf_mutex_unlock(dsp_semaph);
		return -EFAULT;
	}
	*sram_buf = c;
//	awrpaf_debug("c:%p, sram_buf:%p\n", c, *sram_buf);
	arpaf_mutex_unlock(dsp_semaph);

	return 0;
}

int32_t snd_dsp_substream_list_sram_buf_add_item(struct snd_dsp_hal_substream *hal_substream,
					struct sram_buffer *sram_buf)
{
	QueueHandle_t dsp_semaph = NULL;

	if (!hal_substream) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	if (!sram_buf) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	dsp_semaph = hal_substream->sram_buf_list_mutex;

	arpaf_mutex_lock(dsp_semaph);
	sram_buf_queue_set_using(sram_buf, 1);
//	awrpaf_debug("sram_buf:%p\n", sram_buf);
	list_add_tail(&sram_buf->list, &hal_substream->list_sram_buf);
	arpaf_mutex_unlock(dsp_semaph);

	return 0;
}

static int32_t snd_dsp_substream_list_sram_buf_remove_item(
			struct snd_dsp_hal_substream *hal_substream,
			struct sram_buffer *sram_buf)
{
	struct sram_buffer *c = NULL;
	struct sram_buffer *tmp = NULL;
	QueueHandle_t dsp_semaph = NULL;

	if (!hal_substream) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	if (!sram_buf) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	dsp_semaph = hal_substream->sram_buf_list_mutex;

	arpaf_mutex_lock(dsp_semaph);
	if (list_empty(&hal_substream->list_sram_buf)) {
		arpaf_mutex_unlock(dsp_semaph);
		return -EFAULT;
	}
	list_for_each_entry_safe(c, tmp, &hal_substream->list_sram_buf, list) {
//		awrpaf_debug("c:%p, sram_buf:%p\n", c, sram_buf);
		if (c == sram_buf) {
			list_del(&c->list);
			sram_buf_queue_set_using(c, 0);
			arpaf_mutex_unlock(dsp_semaph);
			return 0;
		}
	}
	arpaf_mutex_unlock(dsp_semaph);
	return -EFAULT;
}

static int32_t snd_dsp_substream_list_sram_buf_remove_all(
			struct snd_dsp_hal_substream *hal_substream)
{
	struct sram_buffer *c = NULL;
	struct sram_buffer *tmp = NULL;
	QueueHandle_t dsp_semaph = NULL;

	if (!hal_substream) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	dsp_semaph = hal_substream->sram_buf_list_mutex;

	arpaf_mutex_lock(dsp_semaph);
	if (list_empty(&hal_substream->list_sram_buf)) {
		arpaf_mutex_unlock(dsp_semaph);
		return 0;
	}
	list_for_each_entry_safe(c, tmp, &hal_substream->list_sram_buf, list) {
		list_del(&c->list);
		sram_buf_queue_set_using(c, 0);
	}
	arpaf_mutex_unlock(dsp_semaph);
	return 0;
}
#endif

int32_t snd_dsp_hal_substream_startup(struct snd_dsp_hal_substream *hal_substream)
{
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_substream->params;
	QueueHandle_t dsp_semaph = hal_substream->stream_mutex;
	char card_name[32] = {0};

	arpaf_mutex_lock(dsp_semaph);

	awrpaf_debug("hal_substream->id:%d, stream:%s\n", hal_substream->id,
				pcm_params->stream?"Capture":"Playback");

	if (soc_substream->audio_standby) {
		if (hal_substream->srambuf_tsleep == NULL)
			hal_substream->srambuf_tsleep = hal_substream_sram_buf_sched_init();

		if (!hal_substream->sram_buf_list_mutex) {
			hal_substream->sram_buf_list_mutex = arpaf_mutex_init();
		}
#ifdef CONFIG_SND_PLATFORM_SUNXI_MAD
		amixer_sset_enum_ctl(pcm_params->driver, "bind mad function", "mad_bind");
#endif
		hal_substream->stream_active = 0;
	}

	snprintf(card_name, 31, "hw:%s", pcm_params->driver);
	soc_substream->ret_val = snd_pcm_open(&hal_substream->pcm_handle, card_name,
			(pcm_params->stream == SND_STREAM_PLAYBACK)?
				SND_PCM_STREAM_PLAYBACK:SND_PCM_STREAM_CAPTURE, 0);

	arpaf_mutex_unlock(dsp_semaph);

	return soc_substream->ret_val;
}

void snd_dsp_hal_substream_shutdown(struct snd_dsp_hal_substream *hal_substream)
{
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
	QueueHandle_t dsp_semaph = hal_substream->stream_mutex;
#ifdef CONFIG_SND_PLATFORM_SUNXI_MAD
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_substream->params;
#endif

	arpaf_mutex_lock(dsp_semaph);

	awrpaf_debug("hal_substream->id:%d, stream:%s\n", hal_substream->id,
				soc_substream->params.stream?"Capture":"Playback");

	if (soc_substream->audio_standby) {
		awrpaf_info("audio_standby mode!\n");
		hal_substream->keyWordRun = 0;
		hal_substream->standby_state = AUDIO_STANDBY_SHUTDOWN_STATE;

		arpaf_mutex_unlock(dsp_semaph);

		while (hal_substream->pcm_reading)
			vTaskDelay(pdMS_TO_TICKS(1));

		arpaf_mutex_lock(dsp_semaph);
	}

#ifdef CONFIG_COMPONENTS_AW_ALSA_RPAF_COMPONENT
	/* 停止各种各样算法处理 */
	snd_dsp_algorithmic_release(&hal_substream->native_component);
#endif
	/* 考虑录音场景下的设置 */
	if (hal_substream->pcm_handle) {
		snd_pcm_close(hal_substream->pcm_handle);
		hal_substream->pcm_handle = NULL;
	}

#ifdef CONFIG_SND_PLATFORM_SUNXI_MAD
	if (soc_substream->audio_standby)
		amixer_sset_enum_ctl(pcm_params->driver, "bind mad function", "mad_unbind");
#endif

//	vTaskDelete(hal_substream->keyWordTask);
//	hal_substream->keyWordTask = NULL;
	hal_substream->prepared = 0;
	soc_substream->ret_val = 0;

	arpaf_mutex_unlock(dsp_semaph);
}

/* 预备音频流，主要用于复位中间信息 */
int32_t snd_dsp_hal_substream_prepare(struct snd_dsp_hal_substream *hal_substream)
{
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
	QueueHandle_t dsp_semaph = hal_substream->stream_mutex;
#ifdef CONFIG_COMPONENTS_AW_ALSA_RPAF_COMPONENT
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_substream->params;
#endif

	arpaf_mutex_lock(dsp_semaph);

	awrpaf_debug("hal_substream->id:%d, stream:%s\n", hal_substream->id,
				pcm_params->stream?"Capture":"Playback");

	if ((pcm_params->stream == SND_STREAM_CAPTURE) &&
		(hal_substream->prepared)) {
		soc_substream->ret_val = 0;
	} else {
		soc_substream->ret_val = snd_pcm_prepare(hal_substream->pcm_handle);
		hal_substream->prepared = 1;
	}

	arpaf_mutex_unlock(dsp_semaph);

	return soc_substream->ret_val;
}

/* 启动音频流 */
int32_t snd_dsp_hal_substream_start(struct snd_dsp_hal_substream *hal_substream)
{
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
	QueueHandle_t dsp_semaph = hal_substream->stream_mutex;

	arpaf_mutex_lock(dsp_semaph);

	awrpaf_debug("hal_substream->id:%d, stream:%s\n", hal_substream->id,
				soc_substream->params.stream?"Capture":"Playback");

	soc_substream->ret_val = 0;

	arpaf_mutex_unlock(dsp_semaph);

	return soc_substream->ret_val;
}

static uint32_t vad_enable = 0;

void standby_enable_voice_wakeup(void)
{
#if 0
	BaseType_t ret = pdFALSE;
	if (gVoiceTask) {
		ret = xTaskNotify(gVoiceTask, STANDBY_NOTIFY_SUSPEND, eSetBits);
		if (ret == pdFALSE)
			printf("gVoiceTask notify failed.\n");
		else
			printf("gVoiceTask notify success.\n");
	}
#else
	vad_enable = 1;
#endif
}
/*
static uint32_t standby_waiting_linux_suspend(void)
{
	uint32_t timeout_count = 0;
	uint32_t notify = 0;
	static uint32_t standby_count = 0;

	for (;;) {
		printf("waiting suspend notify...\n");
		if (xTaskNotifyWait(0, UINT_MAX, &notify, portMAX_DELAY) == pdFALSE) {
			printf("notify waiting timeout[%u]!\n", timeout_count);
			continue;
		}

		// handle event of revice
		switch (notify) {
		case STANDBY_NOTIFY_SUSPEND:
			printf("------>>> notify is suspend[%u] <<<------\n", ++standby_count);
		case STANDBY_NOTIFY_RESUME:
			return notify;
		default:
			printf("notify is error %u!\n", notify);
			break;
		}
	}
}
*/
enum mad_keyword_task_notify_value {
	MAD_KEYWORD_NOTIFY_RUNNING = 0x1,
};

void mad_keyword_task_notify(struct snd_dsp_hal_substream *hal_substream, uint32_t value)
{
	BaseType_t ret = pdFALSE;

	if (hal_substream->keyWordTask) {
		ret = xTaskNotify(hal_substream->keyWordTask, value, eSetBits);
		if (ret == pdFALSE)
			awrpaf_info("notify failed.\n");
		else
			awrpaf_info("notify success.\n");
	}
}

uint32_t mad_keyword_waiting(void)
{
	uint32_t timeout_count = 0;
	uint32_t notify = 0;
	static uint32_t standby_count = 0;

	for (;;) {
		printf("waiting keywork notify...\n");
		if (xTaskNotifyWait(0, UINT_MAX, &notify, portMAX_DELAY) == pdFALSE) {
			awrpaf_err("notify waiting timeout[%u]!\n", timeout_count);
			continue;
		}

		// handle event of revice
		switch (notify) {
		case MAD_KEYWORD_NOTIFY_RUNNING:
			printf("------>>> notify is running[%u] <<<------\n", ++standby_count);
			return notify;
		default:
			awrpaf_err("notify is error %u!\n", notify);
			break;
		}
	}
}

#ifdef CONFIG_SND_PLATFORM_SUNXI_MAD
extern int32_t sunxi_mad_schd_timeout(TickType_t ticks);
extern int32_t amixer_sset_enum_ctl(const char *card_name, const char *ctl_name,
			const char *ctl_val);
#endif

int32_t audio_suspend_dsp_system(struct snd_dsp_hal_substream *hal_substream)
{
	int32_t ret = 0;
#ifdef CONFIG_SND_PLATFORM_SUNXI_MAD
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_substream->params;
#endif

	awrpaf_err("audio_standby start.\n");
#ifdef CONFIG_SND_PLATFORM_SUNXI_MAD
	/* enter standby */
	ret = amixer_sset_enum_ctl(pcm_params->driver, "mad standby control", "SUSPEND");
	if (ret < 0) {
		hal_substream->standby_state = AUDIO_STANDBY_SUSPEND_STATE;
		awrpaf_err("\n");
		return ret;
	}
	if (sunxi_mad_schd_timeout(portMAX_DELAY) == 0) {
		hal_substream->standby_state = AUDIO_STANDBY_DSP_WAKEUP_STATE;
	} else
		amixer_sset_enum_ctl(pcm_params->driver, "mad standby control", "RESUME");
#endif
	awrpaf_err("audio_standby stop.\n");
	return ret;
}

/* 停止录音流，除非进入休眠，否则算法仍然据需处理 */
void pxAudioKeyWordTask(void *arg)
{
	struct snd_dsp_hal_substream *hal_substream = (struct snd_dsp_hal_substream *)arg;
//	struct snd_soc_dsp_native_component *native_component = &hal_substream->native_component;
//	struct snd_soc_dsp_component *soc_component = &native_component->soc_component;
//	struct snd_soc_dsp_pcm_params *pcm_params = &soc_component->pcm_params;
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
	QueueHandle_t dsp_semaph = hal_substream->stream_mutex;
//	portBASE_TYPE xStatus;
	struct sram_buffer *sram_buf = NULL;

	for (;;) {
//		awrpaf_info("standby_state:%d\n", hal_substream->standby_state);
		arpaf_mutex_lock(dsp_semaph);
		if ((hal_substream->keyWordRun == 0) &&
			(hal_substream->standby_state == AUDIO_STANDBY_SHUTDOWN_STATE)) {
			arpaf_mutex_unlock(dsp_semaph);
			snd_dsp_substream_list_sram_buf_remove_all(hal_substream);
			switch (mad_keyword_waiting()) {
			case MAD_KEYWORD_NOTIFY_RUNNING:
				awrpaf_info("running.\n");
				break;
			default:
				awrpaf_err("\n");
				break;
			}
			arpaf_mutex_lock(dsp_semaph);
		}
		arpaf_mutex_unlock(dsp_semaph);

//		awrpaf_info("standby_state:%d\n", hal_substream->standby_state);
		/* 需要个缓存策略，比如达到1s数据，就覆盖最旧的数据 */
		sram_buf = sram_buf_get_idle_sram_buf(hal_substream);
		if (sram_buf == NULL) {
			awrpaf_err("state=%d\n", hal_substream->standby_state);
			/* 复位缓冲区 */
			//snd_dsp_substream_list_sram_buf_remove_all(hal_substream);
			/* 取出最旧的数据 */
			snd_dsp_substream_list_sram_buf_get_head_buf(hal_substream, &sram_buf);
			snd_dsp_substream_list_sram_buf_remove_item(hal_substream, sram_buf);
			continue;
		}
//		awrpaf_info("standby_state:%d\n", hal_substream->standby_state);

		arpaf_mutex_lock(dsp_semaph);
		if (hal_substream->standby_state == AUDIO_STANDBY_SHUTDOWN_STATE) {
			arpaf_mutex_unlock(dsp_semaph);
			continue;
		}

		/* 当shutdown操作时需要 */
		hal_substream->pcm_reading = 1;

		if (!hal_substream->prepared) {
			snd_pcm_prepare(hal_substream->pcm_handle);
			hal_substream->prepared = 1;
		}

		arpaf_mutex_unlock(dsp_semaph);

		/* 从声卡里读取数据 */
		awpcm_read(hal_substream->pcm_handle, hal_substream->sram_addr,
			snd_pcm_bytes_to_frames(hal_substream->pcm_handle,
						soc_substream->input_size),
			snd_pcm_frames_to_bytes(hal_substream->pcm_handle, 1));

#ifdef CONFIG_COMPONENTS_AW_ALSA_RPAF_COMPONENT
		/* 处理各种各样算法处理 */
		snd_dsp_algorithmic_process(&hal_substream->native_component,
				hal_substream->sram_addr, &soc_substream->input_size,
				sram_buf->buf_addr, &soc_substream->output_size);
#else
		memcpy(sram_buf->buf_addr, hal_substream->sram_addr, soc_substream->input_size);
		soc_substream->output_size = soc_substream->input_size;
#endif

		hal_substream->pcm_reading = 0;

//		awrpaf_info("standby_state:%d\n", hal_substream->standby_state);

		arpaf_mutex_lock(dsp_semaph);

		switch (hal_substream->standby_state) {
		/* dsp进WFI前的状态 */
		case AUDIO_STANDBY_STOP_STATE:
		case AUDIO_STANDBY_SUSPEND_STATE:
			/* 关键词识别算法 */
			/* 唤醒与否 */
			if (0) {
				//mytest
				printf("mytest: [%s] -> %d\n", __func__, __LINE__);
//				standby_wakeup(platform_standby_get());
				hal_substream->standby_state = AUDIO_STANDBY_RESUME_STATE;
				awrpaf_info("\n");
			} else {
				/* 临时处理 */
				//snd_dsp_substream_list_sram_buf_remove_all(hal_substream);

				/* 设置进WFI的标记 */
				if (vad_enable)
					hal_substream->need_dsp_suspend = 1;
				awrpaf_info("vad_enable=%d\n", vad_enable);
			}
			break;
		/* Linux唤醒前的状态 */
		case AUDIO_STANDBY_DSP_WAKEUP_STATE:
			/* 关键词识别算法 */

			/* 唤醒 */
			if (1) {
				//mytest
				printf("mytest: [%s] -> %d\n", __func__, __LINE__);
//				standby_wakeup(platform_standby_get());
				hal_substream->standby_state = AUDIO_STANDBY_RESUME_STATE;
				awrpaf_info("\n");
			} else {
				/* 设置进WFI的标记 */
				if (vad_enable)
					hal_substream->need_dsp_suspend = 1;
				awrpaf_info("\n");
			}
			break;
		case AUDIO_STANDBY_SHUTDOWN_STATE:
			awrpaf_info("\n");
			arpaf_mutex_unlock(dsp_semaph);
			continue;
		default:
			break;
		}

//		awrpaf_info("standby_state:%d\n", hal_substream->standby_state);

		/* 识别到关键词或者被唤醒了，添加数据让linux获取 */
		switch (hal_substream->standby_state) {
		case AUDIO_STANDBY_NORMAL_STATE:
		case AUDIO_STANDBY_RESUME_STATE:
			/*
			 * 为优化使用，可以常驻8个buf空位,
			 * 找到空闲内存并memcpy其中buffer的数据，
			 * 同时加载到该结构体链表中
			 */
			snd_dsp_substream_list_sram_buf_add_item(hal_substream, sram_buf);
			hal_substream_sram_buf_sched_wakeup(hal_substream->srambuf_tsleep);
			break;
		case AUDIO_STANDBY_SHUTDOWN_STATE:
			awrpaf_info("\n");
			arpaf_mutex_unlock(dsp_semaph);
			continue;
		default:
			break;
		}

//		awrpaf_info("standby_state:%d\n", hal_substream->standby_state);

		/* linux的音频已被挂起 */
		if (hal_substream->standby_state == AUDIO_STANDBY_SUSPEND_STATE) {
			/* 满足一定条件后，停止录音 */
			awrpaf_info("standby_state:%d\n", hal_substream->standby_state);
			if (hal_substream->need_dsp_suspend) {
				snd_pcm_drop(hal_substream->pcm_handle);
				hal_substream->prepared = 0;

				arpaf_mutex_unlock(dsp_semaph);

				/* 复位缓冲区buf,否则唤醒后录音获得旧数据 */
				snd_dsp_substream_list_sram_buf_remove_all(hal_substream);
				awrpaf_info("standby_state:%d\n", hal_substream->standby_state);
				/* dsp进入WFI，等待MAD唤醒中断 */
				audio_suspend_dsp_system(hal_substream);

				arpaf_mutex_lock(dsp_semaph);

				/* 退出休眠，将重新开始录音 */
				vad_enable = 0;
				hal_substream->need_dsp_suspend = 0;
				awrpaf_info("standby_state:%d\n", hal_substream->standby_state);
			}
		}

		arpaf_mutex_unlock(dsp_semaph);
	}
	/* unable reach */
	vTaskDelete(NULL);
}

int32_t snd_dsp_hal_substream_stop(struct snd_dsp_hal_substream *hal_substream)
{
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_substream->params;
	QueueHandle_t dsp_semaph = hal_substream->stream_mutex;

	arpaf_mutex_lock(dsp_semaph);

	awrpaf_debug("hal_substream->id:%d, stream:%s\n", hal_substream->id,
				pcm_params->stream?"Capture":"Playback");

	soc_substream->ret_val = 0;

	/* 针对录音流由独立任务运行, 不在此执行音频流操作。*/
	if (pcm_params->stream == SND_STREAM_PLAYBACK) {
		//snd_pcm_drop(hal_substream->pcm_handle);
		soc_substream->ret_val = snd_pcm_drop(hal_substream->pcm_handle);
		hal_substream->prepared = 0;
	} else if (pcm_params->stream == SND_STREAM_CAPTURE) {
		if (soc_substream->audio_standby) {
			/* 用于audio standby模式的暂不做处理 */
			awrpaf_info("\n");
			hal_substream->stream_active = 0;
			if (hal_substream->standby_state != AUDIO_STANDBY_SUSPEND_STATE)
				hal_substream->standby_state = AUDIO_STANDBY_STOP_STATE;
		} else {
			soc_substream->ret_val = snd_pcm_drop(hal_substream->pcm_handle);
			hal_substream->prepared = 0;
		}
	}

	arpaf_mutex_unlock(dsp_semaph);

	return soc_substream->ret_val;
}

int32_t snd_dsp_hal_substream_drain(struct snd_dsp_hal_substream *hal_substream)
{
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_substream->params;
	QueueHandle_t dsp_semaph = hal_substream->stream_mutex;

	arpaf_mutex_lock(dsp_semaph);

	awrpaf_debug("hal_substream->id:%d, stream:%s\n", hal_substream->id,
				pcm_params->stream?"Capture":"Playback");

	/* 针对录音流不在此执行音频流操作。*/
	if (pcm_params->stream == SND_STREAM_PLAYBACK) {
		soc_substream->ret_val = snd_pcm_drain(hal_substream->pcm_handle);
		/* for linux drain into sleep status */
		msleep(100);
		hal_substream->prepared = 0;

		arpaf_mutex_unlock(dsp_semaph);

		return soc_substream->ret_val;
	}

	arpaf_mutex_unlock(dsp_semaph);

	return 0;
}

int32_t set_pcm_params(snd_pcm_t *handle, snd_pcm_format_t format,
			uint32_t rate, uint32_t channels,
			snd_pcm_uframes_t period_size,
			snd_pcm_uframes_t buffer_size)
{
	int32_t ret = 0;
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *sw_params;

	/* HW params */
	snd_pcm_hw_params_alloca(&params);
	ret =  snd_pcm_hw_params_any(handle, params);
	if (ret < 0) {
		awrpaf_err("\n");
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
		awrpaf_err("\n");
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
		awrpaf_err("\n");
		return ret;
	}

	return ret;
}

/* 将音频PCM格式传入进行设置 */
int32_t snd_dsp_hal_substream_hw_params(struct snd_dsp_hal_substream *hal_substream)
{
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_substream->params;
	QueueHandle_t dsp_semaph = hal_substream->stream_mutex;
	struct snd_soc_dsp_native_component *native_component = &hal_substream->native_component;
	uint32_t frame_bytes = 0;
	uint32_t bits = 0;

	arpaf_mutex_lock(dsp_semaph);

	awrpaf_debug("hal_substream->id:%d, stream:%s\n", hal_substream->id,
				pcm_params->stream?"Capture":"Playback");
#if 1
	/*
	 * DSP Audio限制buffer_size大小,均小于4倍period_size
	 * 原因：
	 * 1.减少sram的消耗
	 * 2.减少Linux端的Xrun(因为在一段时间内,Linux端消耗period_size的速率并
	 *   不是准确的period_size/rate,而DSP端设置一个较小的buffer_size可以让
	 *   Linux端消耗period_size的速率相对准确)
	 */
	if (pcm_params->buffer_size/pcm_params->period_size >= 4)
	        pcm_params->buffer_size = pcm_params->period_size * 4;
#endif

	awrpaf_debug("======== hw_parmas ========\n");
	awrpaf_debug("format:%u\n", pcm_params->format);
	awrpaf_debug("rate:%u\n", pcm_params->rate);
	awrpaf_debug("channels:%u\n", pcm_params->channels);
	awrpaf_debug("period_size:%u\n", pcm_params->period_size);
	awrpaf_debug("buffer_size:%u\n", pcm_params->buffer_size);
	awrpaf_debug("===========================\n");

	switch (pcm_params->format) {
	default:
	case SND_PCM_FORMAT_S16:
	case SND_PCM_FORMAT_U16:
		bits = 16;
		break;
	case SND_PCM_FORMAT_S24:
	case SND_PCM_FORMAT_U24:
	case SND_PCM_FORMAT_S32:
	case SND_PCM_FORMAT_U32:
		bits = 32;
		break;
	}
	frame_bytes = pcm_params->channels * bits >> 3;

#ifdef CONFIG_COMPONENTS_AW_ALSA_RPAF_COMPONENT
	/* 预备各种各样算法处理 */
	snd_dsp_algorithmic_create(native_component);
#endif

	if ((pcm_params->stream == SND_STREAM_CAPTURE) &&
		(soc_substream->audio_standby)) {
		/* 用于休眠唤醒的录音参数不能重设(always running) */
		if (hal_substream->keyWordRun == 0) {
			if (!hal_substream->sram_buf[0].buf_addr) {
				/* 耗内存，只申请一次 */
				soc_substream->ret_val = snd_dsp_hal_substream_sram_buf_init(hal_substream);
				if (soc_substream->ret_val < 0) {
					awrpaf_err("\n");
					arpaf_mutex_unlock(dsp_semaph);
					return soc_substream->ret_val;
				}
				INIT_LIST_HEAD(&hal_substream->list_sram_buf);
			}
			if (!hal_substream->sram_addr) {
				hal_substream->sram_addr = rpaf_malloc(pcm_params->period_size * frame_bytes);
				if (hal_substream->sram_addr == NULL) {
					awrpaf_err("\n");
					soc_substream->ret_val = -ENOMEM;
					arpaf_mutex_unlock(dsp_semaph);
					return soc_substream->ret_val;
				}
			}
			soc_substream->ret_val = set_pcm_params(hal_substream->pcm_handle,
					pcm_params->format, pcm_params->rate,
					pcm_params->channels, pcm_params->period_size,
					pcm_params->buffer_size);

			arpaf_mutex_unlock(dsp_semaph);

			return soc_substream->ret_val;
		}
	} else {
		/* 正常播放和录音的参数可以重设 */
		soc_substream->ret_val = set_pcm_params(hal_substream->pcm_handle,
				pcm_params->format, pcm_params->rate,
				pcm_params->channels, pcm_params->period_size,
				pcm_params->buffer_size);
	}

	arpaf_mutex_unlock(dsp_semaph);

	return soc_substream->ret_val;
}

/* 用于数据的读操作, 数据最后才给到substream->soc_substream->buf_addr */
snd_pcm_sframes_t snd_dsp_hal_substream_readi(struct snd_dsp_hal_substream *hal_substream)
{
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_substream->params;
	struct sram_buffer *sram_buf = NULL;
	snd_pcm_uframes_t frame_bytes = snd_pcm_frames_to_bytes(hal_substream->pcm_handle, 1);
	QueueHandle_t dsp_semaph = hal_substream->stream_mutex;

	if (pcm_params->stream == SND_STREAM_PLAYBACK)
		return 0;

#ifdef AWRPAF_SUBSTREAM_AUDIO_STANDBY
	/* 创建这个录音流常驻任务, 用于休眠前的操作 */
	if (soc_substream->audio_standby) {

		arpaf_mutex_lock(dsp_semaph);

		/* 让关键录音程序退出等待 */
		if ((!hal_substream->keyWordRun) && hal_substream->keyWordTask)
			mad_keyword_task_notify(hal_substream, MAD_KEYWORD_NOTIFY_RUNNING);
		hal_substream->keyWordRun = 1;
		hal_substream->stream_active = 1;

		hal_substream->standby_state = AUDIO_STANDBY_NORMAL_STATE;

		arpaf_mutex_unlock(dsp_semaph);

		if (hal_substream->keyWordTask == NULL) {
			memset(hal_substream->keyWordTaskName, 0, configMAX_TASK_NAME_LEN);
			/* 创建audio信号收发队列 */
			snprintf(hal_substream->keyWordTaskName, configMAX_TASK_NAME_LEN - 1,
				"KWS-C%dD%d", pcm_params->card,
				pcm_params->device);

			xTaskCreate(pxAudioKeyWordTask, hal_substream->keyWordTaskName,
				1024 * 4, hal_substream,
				configAPPLICATION_AUDIO_PRIORITY, &hal_substream->keyWordTask);
			gVoiceTask = hal_substream->keyWordTask;
			awrpaf_info("KeyWordTask:%s, Handle:%p\n",
				hal_substream->keyWordTaskName, hal_substream->keyWordTask);
		}

		/*
		 * 获得队列首的数据，若没有数据,
		 * 则立即让出时间片, 并等待有效数据过来
		 */
		snd_dsp_substream_list_sram_buf_get_head_buf(hal_substream, &sram_buf);
		if (sram_buf) {
			/* 拷贝数据到共享内存 */
			memcpy((void *)soc_substream->output_addr, sram_buf->buf_addr,
								soc_substream->output_size);
			xthal_dcache_region_writeback_inv((void *)soc_substream->output_addr,
						soc_substream->output_size);
			snd_dsp_substream_list_sram_buf_remove_item(hal_substream,
						sram_buf);
			soc_substream->ret_val = 0;
		} else {
			/* 超时没有数据到来！此处基本不会执行到 */
			awrpaf_err("cannot get sram buffer!\n");
			soc_substream->ret_val = -EFAULT;
			return soc_substream->ret_val;
		}
	} else
#endif
	{
		arpaf_mutex_lock(dsp_semaph);

		if (!hal_substream->prepared) {
			snd_pcm_prepare(hal_substream->pcm_handle);
			hal_substream->prepared = 1;
		}
		awrpaf_debug("[read start] input_addr:0x%x, input_size:%u\n",
				soc_substream->input_addr, soc_substream->input_size);
		soc_substream->ret_val = awpcm_read(hal_substream->pcm_handle,
				(void *)soc_substream->input_addr,
				snd_pcm_bytes_to_frames(hal_substream->pcm_handle,
					soc_substream->input_size), frame_bytes);
#ifdef CONFIG_COMPONENTS_AW_ALSA_RPAF_COMPONENT
		/* 处理各种各样算法处理 */
		snd_dsp_algorithmic_process(&hal_substream->native_component,
					(void *)soc_substream->input_addr,
					&soc_substream->input_size,
					(void *)soc_substream->output_addr,
					&soc_substream->output_size);
#else
		memcpy((void *)soc_substream->output_addr,
				(void *)soc_substream->input_addr,
				soc_substream->input_size);
		soc_substream->output_size = soc_substream->input_size;
#endif
		xthal_dcache_region_writeback_inv((void *)soc_substream->output_addr,
						soc_substream->output_size);
		awrpaf_debug("[read stop] output_addr:0x%x, output_size:%u\n",
				soc_substream->output_addr, soc_substream->output_size);
		arpaf_mutex_unlock(dsp_semaph);
	}

	return soc_substream->ret_val;
}

/* 用于数据的写操作, 数据最先获取substream->soc_substream->buf_addr */
snd_pcm_sframes_t snd_dsp_hal_substream_writei(struct snd_dsp_hal_substream *hal_substream)
{
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_substream->params;
	QueueHandle_t dsp_semaph = hal_substream->stream_mutex;

	if (pcm_params->stream == SND_STREAM_CAPTURE)
		return 0;

	arpaf_mutex_lock(dsp_semaph);

	awrpaf_debug("[write start] input_addr:0x%x, input_size:%u\n",
				soc_substream->input_addr, soc_substream->input_size);
#ifdef CONFIG_COMPONENTS_AW_ALSA_RPAF_COMPONENT
	/* 处理各种各样算法处理 */
	snd_dsp_algorithmic_process(&hal_substream->native_component,
			(void *)soc_substream->input_addr, &soc_substream->input_size,
			(void *)soc_substream->output_addr, &soc_substream->output_size);
#endif
	if (!hal_substream->prepared) {
		snd_pcm_prepare(hal_substream->pcm_handle);
		hal_substream->prepared = 1;
	}

	soc_substream->ret_val = awpcm_write(hal_substream->pcm_handle,
			(void *)soc_substream->output_addr,
			snd_pcm_bytes_to_frames(hal_substream->pcm_handle,
					soc_substream->output_size),
			snd_pcm_frames_to_bytes(hal_substream->pcm_handle, 1));

	awrpaf_debug("[write stop] output_addr:0x%x, output_size:%u\n",
				soc_substream->output_addr, soc_substream->output_size);

	arpaf_mutex_unlock(dsp_semaph);

	return soc_substream->ret_val;
}

/* 用于检测声卡是否注册成功 */
int32_t snd_dsp_hal_substream_probe(struct snd_dsp_hal_substream *hal_substream)
{
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_substream->params;
	QueueHandle_t dsp_semaph = hal_substream->stream_mutex;

	arpaf_mutex_lock(dsp_semaph);

	soc_substream->ret_val = ksnd_card_index(pcm_params->driver);
	if (soc_substream->ret_val < 0) {
		awrpaf_err("no card:%s\n", pcm_params->driver);
		arpaf_mutex_unlock(dsp_semaph);
		return soc_substream->ret_val;
	}
	soc_substream->ret_val = 0;

	awrpaf_debug("\n");

	arpaf_mutex_unlock(dsp_semaph);

	return soc_substream->ret_val;
}

int32_t snd_dsp_hal_substream_remove(struct snd_dsp_hal_substream *hal_substream)
{
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
	QueueHandle_t dsp_semaph = hal_substream->stream_mutex;

	arpaf_mutex_lock(dsp_semaph);

	soc_substream->ret_val = 0;
	awrpaf_debug("\n");

	arpaf_mutex_unlock(dsp_semaph);

	return soc_substream->ret_val;
}

/* 用于记录linux音频驱动是否由于standby原因被挂起 */
int32_t snd_dsp_hal_substream_suspend(struct snd_dsp_hal_substream *hal_substream)
{
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
	QueueHandle_t dsp_semaph = hal_substream->stream_mutex;

	arpaf_mutex_lock(dsp_semaph);

	soc_substream->ret_val = 0;
	hal_substream->soc_suspended = 1;
	hal_substream->soc_resumed = 0;

	if (soc_substream->audio_standby) {
		awrpaf_info("\n");
		hal_substream->stream_active = 0;
	} else {
		if (hal_substream->pcm_handle != NULL)
			soc_substream->ret_val = snd_pcm_drop(hal_substream->pcm_handle);
		hal_substream->prepared = 0;
	}

	hal_substream->standby_state = AUDIO_STANDBY_SUSPEND_STATE;

	awrpaf_info("\n");

	arpaf_mutex_unlock(dsp_semaph);

	return soc_substream->ret_val;
}

/* 用于记录linux音频驱动是否被恢复，这个不能作为完全恢复的状态标记！*/
int32_t snd_dsp_hal_substream_resume(struct snd_dsp_hal_substream *hal_substream)
{
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
	QueueHandle_t dsp_semaph = hal_substream->stream_mutex;

	arpaf_mutex_lock(dsp_semaph);

	soc_substream->ret_val = 0;
	hal_substream->soc_suspended = 0;
	hal_substream->soc_resumed = 1;

	arpaf_mutex_unlock(dsp_semaph);

	if (hal_substream->pcm_handle != NULL && soc_substream->audio_standby == 0) {
		soc_substream->ret_val = snd_pcm_prepare(hal_substream->pcm_handle);
		awrpaf_info("\n");
	}

	return soc_substream->ret_val;
}

struct snd_dsp_hal_substream_driver hal_substream_driver = {
	.probe = (int32_t (*)(void *))snd_dsp_hal_substream_probe,
	.remove = (int32_t (*)(void *))snd_dsp_hal_substream_remove,
	.suspend = (int32_t (*)(void *))snd_dsp_hal_substream_suspend,
	.resume = (int32_t (*)(void *))snd_dsp_hal_substream_resume,
};

/* 算法API可在此回调中调用 */
struct snd_dsp_hal_substream_ops hal_substream_ops = {
	.startup = (int32_t (*)(void *))snd_dsp_hal_substream_startup,
	.hw_params = (int32_t (*)(void *))snd_dsp_hal_substream_hw_params,
	.prepare = (int32_t (*)(void *))snd_dsp_hal_substream_prepare,
	.start = (int32_t (*)(void *))snd_dsp_hal_substream_start,
	.readi = (snd_pcm_sframes_t (*)(void *))snd_dsp_hal_substream_readi,
	.writei = (snd_pcm_sframes_t (*)(void *))snd_dsp_hal_substream_writei,
	.stop = (int32_t (*)(void *))snd_dsp_hal_substream_stop,
	.drain = (int32_t (*)(void *))snd_dsp_hal_substream_drain,
	.shutdown = (void (*)(void *))snd_dsp_hal_substream_shutdown,
};

/* 任务不能频繁创建，否则上百次后可能是因为内存原因失败！*/
void pxAudioStreamTask(void *arg)
{
	struct snd_dsp_hal_substream *hal_substream = (struct snd_dsp_hal_substream *)arg;
	struct snd_dsp_hal_queue_item pvAudioCmdItem;
	int32_t ret = 0;
	portBASE_TYPE xStatus;

	/* the flag is for mark task create completed and running */
	hal_substream->handle_bit |= 0x1 << hal_substream->id;

	xEventGroupSetBits(hal_substream->xTaskCreateEvent, hal_substream->handle_bit);

	for (;;) {
		if ((ret = uxQueueMessagesWaiting(hal_substream->cmdQueue)) != 0)
			awrpaf_debug("QM Waiting:%d!\n", ret);

		xStatus = xQueueReceive(hal_substream->cmdQueue,
					&pvAudioCmdItem, portMAX_DELAY);
		if (pvAudioCmdItem.pxAudioMsgVal != MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND) {
			awrpaf_err("\n");
			goto err_audiostream_msgval;
		}

		snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x008);
		switch (pvAudioCmdItem.soc_substream->cmd_val) {
		case SND_SOC_DSP_PCM_STARTUP:
			snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x009);
			/* handle_bit:表示当前持有的idsubstream正在运行使用.*/
			hal_substream->handle_bit |= 0x1 << hal_substream->id;
			hal_substream->substream_ops.startup(hal_substream);
			break;
		case SND_SOC_DSP_PCM_HW_PARAMS:
			snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x00A);
			/* 增加其它函数的初始化等 */
			hal_substream->substream_ops.hw_params(hal_substream);
			break;
		case SND_SOC_DSP_PCM_PREPARE:
			snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x00B);
			hal_substream->substream_ops.prepare(hal_substream);
			break;
		case SND_SOC_DSP_PCM_WRITEI:
			snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x00C);
			/* 往声卡里写入数据 */
			hal_substream->substream_ops.writei(hal_substream);
			break;
		case SND_SOC_DSP_PCM_READI:
			snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x00D);
			/* 从声卡里读取数据 */
			hal_substream->substream_ops.readi(hal_substream);
			break;
		case SND_SOC_DSP_PCM_START:
			snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x00E);
			hal_substream->substream_ops.start(hal_substream);
			break;
		/* maybe had been flush buffer for waiting */
		case SND_SOC_DSP_PCM_DRAIN:
			snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x00F);
			hal_substream->substream_ops.drain(hal_substream);
			break;
		/* 停止音频流 */
		case SND_SOC_DSP_PCM_STOP:
			snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x010);
			hal_substream->substream_ops.stop(hal_substream);
			break;
		case SND_SOC_DSP_PCM_SHUTDOWN:
			snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x011);
			hal_substream->substream_ops.shutdown(hal_substream);
			hal_substream->handle_bit &= ~(0x1 << hal_substream->id);
			break;
		default:
			awrpaf_debug("\n");
			break;
		}

err_audiostream_msgval:
		pvAudioCmdItem.soc_substream->ret_val = hal_substream->soc_substream->ret_val;
		pvAudioCmdItem.soc_substream->used = 1;

		/* audio信号收发队列 */
		xStatus = xQueueSendToBack(arpaf_priv->ServerReceQueue,
				&pvAudioCmdItem, pdMS_TO_TICKS(20));
		if (xStatus != pdPASS)
			awrpaf_err("\n");
	}
}

int32_t snd_dsp_hal_substream_process(void *argv)
{
	struct snd_dsp_hal_queue_item *pAudioCmdItem = argv;
	uint32_t uxQueueLength = 20;
	uint32_t uxItemSize = sizeof(struct snd_dsp_hal_queue_item);
	struct snd_dsp_hal_substream *hal_substream = NULL;
	struct snd_soc_dsp_substream *soc_substream = pAudioCmdItem->soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params = NULL;
	static uint32_t stream_handle_bit = 0;
	uint32_t i = 0;
	BaseType_t xStatus;

	snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x000);
	/* 查找当前是否有正在运行的音频流，包括录音流和播放流 */
	hal_substream = snd_dsp_hal_substream_get_from_list_by_card_device_stream(arpaf_priv,
				soc_substream->params.card, soc_substream->params.device,
				soc_substream->params.stream);
	if (hal_substream == NULL) {
		hal_substream = arpaf_hal_substream_list_malloc_add_tail(arpaf_priv, soc_substream);
		if (hal_substream == NULL) {
			pAudioCmdItem->soc_substream->used = 1;
			pAudioCmdItem->ret_val = -EFAULT;
			/* audio信号收发队列 */
			xStatus = xQueueSendToBack(arpaf_priv->ServerReceQueue,
				pAudioCmdItem, pdMS_TO_TICKS(20));
			if (xStatus != pdPASS)
				awrpaf_err("\n");
			return -EFAULT;
		}
		/* 注册回调 */
		hal_substream->driver_ops = hal_substream_driver;
		hal_substream->substream_ops = hal_substream_ops;
	}

	/* 参数更新 */
	memcpy(hal_substream->soc_substream, soc_substream, sizeof(struct snd_soc_dsp_substream));
	pcm_params = &soc_substream->params;

#if 0
	static int32_t repeat_cmd = 0;

	if (pAudioCmdItem->soc_substream->cmd_val == SND_SOC_DSP_PCM_READI ||
		pAudioCmdItem->soc_substream->cmd_val == SND_SOC_DSP_PCM_WRITEI) {
		if (!repeat_cmd) {
			printf("[%s] line:%d cmd=%d\n", __func__, __LINE__, pAudioCmdItem->soc_substream->cmd_val);
			repeat_cmd = 1;
		}
	} else {
		repeat_cmd = 0;
		printf("[%s] line:%d cmd_val=%d\n", __func__, __LINE__, pAudioCmdItem->soc_substream->cmd_val);
	}
#endif

	snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x001);
	/* 给播放流或录音流任务分发任务指令 */
	switch (pAudioCmdItem->soc_substream->cmd_val) {
	case SND_SOC_DSP_PCM_PROBE:
		snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x002);
		hal_substream->driver_ops.probe(hal_substream);
		break;
	case SND_SOC_DSP_PCM_SUSPEND:
		snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x003);
		hal_substream->driver_ops.suspend(hal_substream);
		break;
	case SND_SOC_DSP_PCM_RESUME:
		snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x004);
		hal_substream->driver_ops.resume(hal_substream);
		break;
	case SND_SOC_DSP_PCM_REMOVE:
		snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x005);
		hal_substream->driver_ops.remove(hal_substream);
		break;
	}

	switch (pAudioCmdItem->soc_substream->cmd_val) {
	case SND_SOC_DSP_PCM_PROBE:
	case SND_SOC_DSP_PCM_SUSPEND:
	case SND_SOC_DSP_PCM_RESUME:
	case SND_SOC_DSP_PCM_REMOVE:
		awrpaf_debug("\n");
		soc_substream->ret_val = hal_substream->soc_substream->ret_val;
		soc_substream->used = 1;
		/* audio信号收发队列 */
		xStatus = xQueueSendToBack(arpaf_priv->ServerReceQueue,
				pAudioCmdItem, pdMS_TO_TICKS(20));
		if (xStatus != pdPASS)
			awrpaf_err("\n");
		break;
	default:
		break;
	}

	switch (pAudioCmdItem->soc_substream->cmd_val) {
	case SND_SOC_DSP_PCM_STARTUP:
		snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x006);
		/* 创建这个播放流或者录音流常驻任务 */
		if (hal_substream->taskHandle == NULL) {
			for (i = 0; i < 32; i++) {
				if (!(stream_handle_bit >> i) & 0x1)
					break;
			}
			hal_substream->id = i;
			hal_substream->cmdQueue = xQueueCreate(uxQueueLength, uxItemSize);
			memset(hal_substream->taskName, 0, configMAX_TASK_NAME_LEN);
			snprintf(hal_substream->taskName, configMAX_TASK_NAME_LEN - 1,
				"pcmC%dD%d%s", pcm_params->card,
				pcm_params->device, pcm_params->stream?"c":"p");
			xTaskCreate(pxAudioStreamTask, hal_substream->taskName, 1024, hal_substream,
				configAPPLICATION_AUDIO_PRIORITY, &hal_substream->taskHandle);
			xEventGroupWaitBits(hal_substream->xTaskCreateEvent,
					(0x1 << hal_substream->id),
					pdTRUE, pdFALSE, portMAX_DELAY);
			stream_handle_bit |= 0x1 << hal_substream->id;
			awrpaf_info("StreamTask:%s, Handle:%p\n",
				hal_substream->taskName, hal_substream->taskHandle);
		}
	case SND_SOC_DSP_PCM_HW_PARAMS:
	case SND_SOC_DSP_PCM_PREPARE:
	case SND_SOC_DSP_PCM_WRITEI:
	case SND_SOC_DSP_PCM_READI:
	case SND_SOC_DSP_PCM_START:
	case SND_SOC_DSP_PCM_STOP:
	case SND_SOC_DSP_PCM_DRAIN:
	/* 关闭声卡 */
	case SND_SOC_DSP_PCM_SHUTDOWN:
		xStatus = xQueueSendToBack(hal_substream->cmdQueue, pAudioCmdItem, pdMS_TO_TICKS(20));
		if (xStatus != pdPASS) {
			awrpaf_err("\n");
			return -EBUSY;
		}
		break;
	default:
		awrpaf_debug("\n");
		break;
	}
	snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_PCM | 0x007);

	return 0;
}
