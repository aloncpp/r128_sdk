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

#include <aw_rpaf/component.h>
#include <aw_rpaf/common.h>
#include <aw_rpaf/substream.h>
#include <aw_rpaf/mixer.h>
#include <aw_rpaf/debug.h>
#include <hal_msgbox.h>
#ifdef CONFIG_COMPONENTS_ALGO_GENERATE
#include "../../../thirdparty/algo_common/algo_generate.h"
#endif
struct arpaf_priv *arpaf_priv;

#ifdef AWRPAF_MSGBOX_MODE
//extern struct messagebox *msgbox_dsp;
//extern struct messagebox *msgbox_cpu;
//static struct msg_channel *msgcpu_rxch;
//static struct msg_channel *msgcpu_txch;

struct msg_endpoint rpaf_medp;
#endif

/* 256 * 4 = 1024 bytes */
#define ulReceStackDepth 1024U
//static StackType_t puxReceStackBuffer[ulReceStackDepth];
//static StaticTask_t pxReceTaskBuffer;
/* 512 * 4 = 2048 bytes */
#define ulSendStackDepth 1024U
//static StackType_t puxSendStackBuffer[ulSendStackDepth];
//static StaticTask_t pxSendTaskBuffer;

#if 1
void arpaf_thread_start(const char *func_name)
{
	printf(AW_ALSA_LOG_COLOR_YELLOW
		"------> %s Start <------\n"
		AW_ALSA_LOG_COLOR_NONE, func_name);
}
void arpaf_thread_stop(const char *func_name)
{
	printf(AW_ALSA_LOG_COLOR_YELLOW
		"------> %s Stop <------\n"
		AW_ALSA_LOG_COLOR_NONE, func_name);
}
#else
void arpaf_thread_start(const char *func_name)
{
}
void arpaf_thread_stop(const char *func_name)
{
}
#endif

QueueHandle_t arpaf_mutex_init(void)
{
	return xSemaphoreCreateMutex();
}

int32_t arpaf_mutex_lock(QueueHandle_t semaphore)
{
	int32_t ret = 0;
	const TickType_t timeout = portMAX_DELAY;

	ret = xSemaphoreTake(semaphore, timeout);
	if (ret == pdFAIL) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	return 0;
}

int32_t arpaf_mutex_unlock(QueueHandle_t semaphore)
{
	int32_t ret = 0;

	ret = xSemaphoreGive(semaphore);
	if (ret == pdFAIL) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	return 0;
}

#if 0
static void arpaf_mutex_delete(QueueHandle_t semaphore)
{
	if (!semaphore)
		return;
	vSemaphoreDelete(semaphore);
}
#endif

int32_t arpaf_list_init(struct arpaf_priv *arpaf_priv,
		enum snd_dsp_list_type list_type)
{
	QueueHandle_t dsp_semaph = NULL;
	struct list_head *dsp_list = NULL;

	if (!arpaf_priv) {
		awrpaf_err("\n");
		return -EFAULT;
	}

	switch (list_type) {
	case LIST_TYPE_HAL_MIXER:
		dsp_semaph = arpaf_priv->hal_mixer_list_mutex;
		dsp_list = &arpaf_priv->list_hal_mixer;
		break;
	case LIST_TYPE_SOC_MIXER:
		dsp_semaph = arpaf_priv->soc_mixer_list_mutex;
		dsp_list = &arpaf_priv->list_soc_mixer;
		break;
	case LIST_TYPE_HAL_SUBSTREAM:
		dsp_semaph = arpaf_priv->hal_substream_list_mutex;
		dsp_list = &arpaf_priv->list_hal_substream;
		break;
	case LIST_TYPE_SOC_SUBSTREAM:
		dsp_semaph = arpaf_priv->soc_substream_list_mutex;
		dsp_list = &arpaf_priv->list_soc_substream;
		break;
	case LIST_TYPE_HAL_COMPONENT:
		dsp_semaph = arpaf_priv->hal_component_list_mutex;
		dsp_list = &arpaf_priv->list_hal_component;
		break;
	case LIST_TYPE_SOC_COMPONENT:
		dsp_semaph = arpaf_priv->soc_component_list_mutex;
		dsp_list = &arpaf_priv->list_soc_component;
		break;
	default:
		awrpaf_err("\n");
		return -EINVAL;
	}

	arpaf_mutex_lock(dsp_semaph);
	INIT_LIST_HEAD(dsp_list);
	arpaf_mutex_unlock(dsp_semaph);

	return 0;
}

#ifdef CONFIG_COMPONENTS_AW_ALSA_RPAF_DEBUG
#if 0
void arpaf_print_soc_mixer(struct snd_soc_dsp_mixer *soc_mixer)
{
	if (!soc_mixer) {
		awrpaf_err("\n");
		return;
	}

	awrpaf_info("Info:\n");
	printf("soc_mixer ptr:%p\n", soc_mixer);
	printf("soc_mixer->id:%lu\n", soc_mixer->id);
	printf("soc_mixer->used:%d\n", soc_mixer->used);
	printf("soc_mixer->cmd_val:%d\n", soc_mixer->cmd_val);
	printf("soc_mixer->params_val:%d\n", soc_mixer->params_val);
	printf("soc_mixer->card = %d\n", soc_mixer->card);
	printf("soc_mixer->device = %d\n", soc_mixer->device);
	printf("soc_mixer->driver = %s\n", soc_mixer->driver);
	printf("soc_mixer->ctl_name = %s\n", soc_mixer->ctl_name);
	printf("soc_mixer->value = %d\n", soc_mixer->value);
	printf("soc_mixer->ret_val = %d\n", soc_mixer->ret_val);
}

void arpaf_print_soc_debug(struct snd_soc_dsp_debug *soc_debug)
{
	uint32_t i = 0;

	if (!soc_debug) {
		awrpaf_err("null ptr!\n");
		return;
	}

	awrpaf_info("Info:\n");
	printf("soc_debug ptr:%p\n", soc_debug);
	printf("soc_debug->cmd_val:%d\n", soc_debug->cmd_val);
	printf("soc_debug->params_val:%d\n", soc_debug->params_val);

	printf("soc_debug->addr_start:%u\n", soc_debug->addr_start);
	printf("soc_debug->addr_start:%u\n", soc_debug->addr_end);
	for (i = 0; i < (soc_debug->addr_end - soc_debug->addr_start)/4; i++) {
		printf("soc_debug->buf[%d]:%u\n", i, soc_debug->buf[i]);
	}
	printf("soc_debug->mode:%u\n", soc_debug->addr_end);

	printf("pcm_params.card = %d\n", soc_debug->pcm_params.card);
	printf("pcm_params.device = %d\n", soc_debug->pcm_params.device);
	printf("pcm_params.driver = %s\n", soc_debug->pcm_params.driver);
	printf("pcm_params.stream = %d\n", soc_debug->pcm_params.stream);
	printf("pcm_params.format = %d\n", soc_debug->pcm_params.format);
	printf("pcm_params.rate = %u\n", soc_debug->pcm_params.rate);
	printf("pcm_params.channels = %u\n", soc_debug->pcm_params.channels);
	printf("pcm_params.resample_rate = %u\n", soc_debug->pcm_params.resample_rate);
	printf("pcm_params.period_size = %lu\n", soc_debug->pcm_params.period_size);
	printf("pcm_params.periods = %u\n", soc_debug->pcm_params.periods);
	printf("pcm_params.buffer_size = %lu\n", soc_debug->pcm_params.buffer_size);

	printf("pcm_params.data_type = %d\n", soc_debug->pcm_params.data_type);
	printf("pcm_params.codec_type = %d\n", soc_debug->pcm_params.codec_type);
	printf("pcm_params.dts_data = %p\n", soc_debug->pcm_params.dts_data);
	printf("pcm_params.status = %d\n", soc_debug->pcm_params.status);
	printf("pcm_params.hw_stream = %u\n", soc_debug->pcm_params.hw_stream);
	printf("pcm_params.data_mode = %u\n", soc_debug->pcm_params.data_mode);
	printf("pcm_params.stream_wake = %u\n", soc_debug->pcm_params.stream_wake);

	printf("soc_debug->ret_val = %d\n", soc_debug->ret_val);
}

void arpaf_print_soc_substream(struct snd_soc_dsp_substream *soc_substream)
{
	if (!soc_substream) {
		awrpaf_err("null ptr!\n");
		return;
	}

	awrpaf_info("Info:\n");
	printf("soc_substream ptr:%p\n", soc_substream);
	printf("soc_substream->id:%u\n", soc_substream->id);
	printf("soc_substream->used:%d\n", soc_substream->used);
	printf("soc_substream->cmd_val:%d\n", soc_substream->cmd_val);
	printf("soc_substream->params_val:%d\n", soc_substream->params_val);
	printf("soc_substream->buf_addr:%d\n", soc_substream->cmd_val);
	printf("soc_substream->buf_size:%d\n", soc_substream->params_val);

	printf("params.card = %d\n", soc_substream->params.card);
	printf("params.device = %d\n", soc_substream->params.device);
	printf("params.driver = %s\n", soc_substream->params.driver);
	printf("params.stream = %d\n", soc_substream->params.stream);
	printf("params.format = %d\n", soc_substream->params.format);
	printf("params.rate = %u\n", soc_substream->params.rate);
	printf("params.channels = %u\n", soc_substream->params.channels);
	printf("params.resample_rate = %u\n", soc_substream->params.resample_rate);
	printf("params.period_size = %lu\n", soc_substream->params.period_size);
	printf("params.periods = %u\n", soc_substream->params.periods);
	printf("params.buffer_size = %lu\n", soc_substream->params.buffer_size);

	printf("params.data_type = %d\n", soc_substream->params.data_type);
	printf("params.codec_type = %d\n", soc_substream->params.codec_type);
	printf("params.dts_data = %p\n", soc_substream->params.dts_data);
	printf("params.status = %d\n", soc_substream->params.status);
	printf("params.hw_stream = %u\n", soc_substream->params.hw_stream);
	printf("params.data_mode = %u\n", soc_substream->params.data_mode);
	printf("params.stream_wake = %u\n", soc_substream->params.stream_wake);

	printf("soc_substream->ret_val = %d\n", soc_substream->ret_val);
}

void arpaf_print_queue_item(struct snd_dsp_hal_queue_item *pvAudioCmdItem)
{
	int32_t i = 0;

	if (!pvAudioCmdItem)
		return;

	printf("<<<--- pxAudioMsgVal[%d] Info --->>>\n",
		pvAudioCmdItem->pxAudioMsgVal);	
#ifdef CONFIG_COMPONENTS_AW_ALSA_RPAF_SUBSTREAM
	if (pvAudioCmdItem->soc_substream) {
		arpaf_print_soc_substream(pvAudioCmdItem->soc_substream);
	}
	if (pvAudioCmdItem->hal_substream) {
		awrpaf_print("hal_substream ptr:%p\n", pvAudioCmdItem->hal_substream);
	}
	if (pvAudioCmdItem->soc_debug) {
		arpaf_print_soc_debug(pvAudioCmdItem->soc_debug);
	}
#endif

#ifdef CONFIG_COMPONENTS_AW_ALSA_RPAF_COMPONENT
	if (pvAudioCmdItem->soc_component) {
		awrpaf_print("soc_component ptr:%p\n", pvAudioCmdItem->soc_component);
	}
	if (pvAudioCmdItem->hal_component) {
		awrpaf_print("hal_component ptr:%p\n", pvAudioCmdItem->hal_component);
	}
#endif

#ifdef CONFIG_COMPONENTS_AW_ALSA_RPAF_MIXER
	if (pvAudioCmdItem->soc_mixer) {
		arpaf_print_soc_mixer(pvAudioCmdItem->soc_mixer);
	}
	if (pvAudioCmdItem->hal_mixer) {
		awrpaf_print("hal_mixer ptr:%p\n", pvAudioCmdItem->hal_mixer);
	}
#endif

	printf("<<<---");
	for (i = 0; i < 45; i++)
		printf("-");
	printf("--->>>\n");
}
#endif
#endif

#ifdef AWRPAF_MSGBOX_MODE
static volatile int32_t send_dsp_finish = 1;
static volatile int32_t send_cpu_finish = 1;

void arpaf_txch_to_cpu_callback(void __maybe_unused *p)
{
	send_cpu_finish = 1;
}

#if 0
static int32_t arpaf_txch_to_dsp_callback(unsigned long __maybe_unused v,
					void __maybe_unused *p)
{
	send_dsp_finish = 1;

	return 0;
}
#endif

/*
 * sample send function
 */
/* 将数据写入指定的通道 */
static int32_t arpaf_send_to_cpu(struct msg_endpoint *medp, uint8_t *bf, int32_t len)
{
	int32_t result;

//	while(!send_cpu_finish) {
//		vTaskDelay(pdMS_TO_TICKS(1));
//	}
	send_cpu_finish = 0;

	result = hal_msgbox_channel_send(medp, (void *)bf, len);
//	while(!send_cpu_finish) {
//		vTaskDelay(pdMS_TO_TICKS(1));
//	}
	return result;
}

#if 0
static int32_t arpaf_send_to_dsp(struct msg_channel *ch, uint8_t *d, int32_t len)
{
	int32_t result;

	while(!send_dsp_finish)
		vTaskDelay(pdMS_TO_TICKS(1));
	send_dsp_finish = 0;

	result = msgbox_channel_send_data(ch, d, len);
	while(!send_dsp_finish)
		vTaskDelay(pdMS_TO_TICKS(1));

	return result;
}
#endif

void arpaf_rxch_from_cpu_callback(uint32_t v, void __maybe_unused *p)
{
	static uint32_t data_count = 0;
//	static uint32_t recv_count = 0;
	static struct snd_dsp_hal_queue_item pvAudioCmdItem;
	BaseType_t ret;
	BaseType_t xHigherProTaskWoken = pdFALSE;

	if (data_count == 0) {
		pvAudioCmdItem.pxAudioMsgVal = v;
	} else if (data_count == 1) {
		pvAudioCmdItem.soc_substream = NULL;
		pvAudioCmdItem.soc_component = NULL;
		pvAudioCmdItem.soc_mixer = NULL;
		pvAudioCmdItem.soc_debug = NULL;
		switch (pvAudioCmdItem.pxAudioMsgVal) {
		case MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND:
			pvAudioCmdItem.soc_substream =
				(struct snd_soc_dsp_substream *)v;
			break;
		case MSGBOX_SOC_DSP_AUDIO_COMPONENT_COMMAND:
			pvAudioCmdItem.soc_component =
				(struct snd_soc_dsp_component *)v;
			break;
		case MSGBOX_SOC_DSP_AUDIO_MIXER_COMMAND:
			pvAudioCmdItem.soc_mixer =
				(struct snd_soc_dsp_mixer *)v;
			break;
		case MSGBOX_SOC_DSP_AUDIO_DEBUG_COMMAND:
			pvAudioCmdItem.soc_debug =
				(struct snd_soc_dsp_debug *)v;
			break;
		default:
			pvAudioCmdItem.ret_val = -EFAULT;
			break;
		}
	}
	snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_COMMON | 0x002);

	/* 一包数据发送接收完毕 */
	if (++data_count >= (sizeof(struct msg_audio_package) >> 2)) {
//		printfFromISR("recv_count:%d, pxAudioMsgVal:%u, sharePoint:0x%lx\n",
//				++recv_count, pvAudioCmdItem.pxAudioMsgVal, v);
		data_count = 0;
		/* 发送给音频服务 */
		ret = xQueueSendFromISR(arpaf_priv->ServerSendQueue, &pvAudioCmdItem, &xHigherProTaskWoken);
		if (ret == pdPASS) {
			portYIELD_FROM_ISR(xHigherProTaskWoken);
		}
		memset(&pvAudioCmdItem, 0, sizeof(struct snd_dsp_hal_queue_item));
	}
}

int32_t arpaf_msgbox_send(struct snd_dsp_hal_queue_item *pAudioCmdItem)
{
	struct msg_audio_package msg_audio;
	struct msg_endpoint *medp = &rpaf_medp;

	msg_audio.audioMsgVal = pAudioCmdItem->pxAudioMsgVal;
	switch (msg_audio.audioMsgVal) {
	case MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND:
		msg_audio.sharePointer = (uint32_t)(pAudioCmdItem->soc_substream);
		break;
	case MSGBOX_SOC_DSP_AUDIO_COMPONENT_COMMAND:
		msg_audio.sharePointer = (uint32_t)pAudioCmdItem->soc_component;
		break;
	case MSGBOX_SOC_DSP_AUDIO_MIXER_COMMAND:
		msg_audio.sharePointer = (uint32_t)pAudioCmdItem->soc_mixer;
		break;
	case MSGBOX_SOC_DSP_AUDIO_DEBUG_COMMAND:
		msg_audio.sharePointer = (uint32_t)pAudioCmdItem->soc_debug;
		break;
	default:
		break;
	}
	/* 发送给指定的CPU */
	arpaf_send_to_cpu(medp, (uint8_t *)&msg_audio,
			sizeof(struct msg_audio_package));
	return 0;
}
#endif

void pxAudioServerSendTask(void *arg)
{
	BaseType_t xStatus = 0;
	int32_t ret = 0;
	const TickType_t xTicksToWait = portMAX_DELAY;
	struct snd_dsp_hal_queue_item pvAudioCmdItem;
	struct arpaf_priv *arpaf_priv = arg;
	const char *func_name = __func__;
	static unsigned long long recv_count = 0;

	arpaf_thread_start(func_name);
	for (;;) {
#ifdef AWRPAF_MSGBOX_MODE
		if ((ret = uxQueueMessagesWaiting(arpaf_priv->ServerSendQueue)) != 0)
			awrpaf_debug("QM Waiting: %d!\n", ret);
		xStatus = xQueueReceive(arpaf_priv->ServerSendQueue,
					&pvAudioCmdItem, xTicksToWait);
#elif defined(AWRPAF_MSGBOX_SIMULATOR_MODE)
		if ((ret = uxQueueMessagesWaiting(arpaf_priv->MsgSendQueue)) != 0)
			awrpaf_err("QM Waiting: %d!\n", ret);
		xStatus = xQueueReceive(arpaf_priv->MsgSendQueue,
					&pvAudioCmdItem, xTicksToWait);
#endif
		if (xStatus != pdPASS) {
			awrpaf_err("Receive failed!\n");
			goto err_msg_send_queue_receive;
		}

		++recv_count;

		snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_COMMON | 0x003);
		switch (pvAudioCmdItem.pxAudioMsgVal) {
		case MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND:
			awrpaf_debug("recv_count:%llu, pxAudioMsgVal:%u, sharePoint:%p\n",
				recv_count, pvAudioCmdItem.pxAudioMsgVal,
				pvAudioCmdItem.soc_substream);
			if (!arpaf_priv->hal_substream_process)
				break;
			ret = arpaf_priv->hal_substream_process(&pvAudioCmdItem);
			if (ret < 0)
				goto substream_error;
			break;
		case MSGBOX_SOC_DSP_AUDIO_DEBUG_COMMAND:
			if (!arpaf_priv->hal_debug_process)
				break;
			ret = arpaf_priv->hal_debug_process(&pvAudioCmdItem);
			if (ret < 0)
				goto debug_error;
			break;
		case MSGBOX_SOC_DSP_AUDIO_COMPONENT_COMMAND:
			awrpaf_debug("recv_count:%llu, pxAudioMsgVal:%u, sharePoint:%p, write_addr:0x%x, read_addr:0x%x\n",
				recv_count, pvAudioCmdItem.pxAudioMsgVal,
				pvAudioCmdItem.soc_component,
				pvAudioCmdItem.soc_component->write_addr,
				pvAudioCmdItem.soc_component->read_addr);
			if (!arpaf_priv->hal_component_process)
				break;
			ret = arpaf_priv->hal_component_process(&pvAudioCmdItem);
			if (ret < 0)
				goto component_error;
			break;
		case MSGBOX_SOC_DSP_AUDIO_MIXER_COMMAND:
			if (!arpaf_priv->hal_mixer_process)
				break;
			ret = arpaf_priv->hal_mixer_process(&pvAudioCmdItem);
			if (ret < 0)
				goto mixer_error;
			break;
		default:
			awrpaf_print("pxAudioMsgVal:%u\n",
					pvAudioCmdItem.pxAudioMsgVal);
			break;
		}
		snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_COMMON | 0x004);
err_msg_send_queue_receive:
		continue;
mixer_error:
component_error:
debug_error:
substream_error:
		pvAudioCmdItem.ret_val = -EFAULT;
		/* 出现错误后：发送指令给MsgBox或者通告MsgBox的API发送指令 */
		#ifdef AWRPAF_MSGBOX_MODE
		arpaf_msgbox_send(&pvAudioCmdItem);
		#elif defined(AWRPAF_MSGBOX_SIMULATOR_MODE)
		xStatus = xQueueSendToBack(arpaf_priv->MsgReceQueue, &pvAudioCmdItem, 0);
		if (xStatus != pdPASS) {
			awrpaf_err("\n");
		}
		#endif
	}
	/* cannot reach */
	arpaf_thread_stop(func_name);
	vTaskDelete(NULL);
}

/* 用来接收各个子任务的信息，然后发送指令通知其它CPU */
void pxAudioServerReceTask(void *arg)
{
	BaseType_t xStatus;
	int32_t ret = 0;
	const TickType_t xTicksToWait = portMAX_DELAY;
	struct snd_dsp_hal_queue_item pvAudioCmdItem;
	struct arpaf_priv *arpaf_priv = arg;
	const char *func_name = __func__;

	arpaf_thread_start(func_name);
	for (;;) {
		if ((ret = uxQueueMessagesWaiting(arpaf_priv->ServerReceQueue)) != 0) {
			awrpaf_debug("QM Waiting: %d!\n", ret);
		}
		xStatus = xQueueReceive(arpaf_priv->ServerReceQueue,
					&pvAudioCmdItem, xTicksToWait);
		if (xStatus != pdPASS) {
			awrpaf_err("Receive failed!\n");
			goto err_mixer_queue_receive;
		}

		awrpaf_debug("\n");
		if (pvAudioCmdItem.ret_val < 0)
			awrpaf_err("<<<<<<--- Failed --->>>>>>\n");

		snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_COMMON | 0x005);
		/* 发送指令给MsgBox或者通告MsgBox的API发送指令 */
#ifdef AWRPAF_MSGBOX_MODE
		arpaf_msgbox_send(&pvAudioCmdItem);
#elif defined(AWRPAF_MSGBOX_SIMULATOR_MODE)
		xStatus = xQueueSendToBack(arpaf_priv->MsgReceQueue, &pvAudioCmdItem, pdMS_TO_TICKS(100));
		if (xStatus != pdPASS) {
			awrpaf_err("MsgReceQueue SendToBack failed!\n");
		}
#endif
err_mixer_queue_receive:
		snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_COMMON | 0x006);
		continue;
	}
	/* cannot reach */
	arpaf_thread_stop(func_name);;
	vTaskDelete(NULL);
}

int32_t snd_dsp_audio_remote_process_init(void)
{
	uint32_t uxItemSize = sizeof(struct snd_dsp_hal_queue_item);
	int32_t ret = 0;
	int32_t i = 0;
	struct msg_endpoint *medp = &rpaf_medp;

	arpaf_priv = rpaf_malloc(sizeof(struct arpaf_priv));
	if (arpaf_priv == NULL) {
		awrpaf_err("malloc arpaf_priv failed.\n");
		return -ENOMEM;
	}
	memset(arpaf_priv, 0, sizeof(struct arpaf_priv));

	snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_COMMON | 0x000);
#ifdef AWRPAF_MSGBOX_MODE
	/* 和ARM CPU系统通信接口 */
#if 0
	/* Old Version Interface */
	msgcpu_rxch = msgbox_alloc_channel(msgbox_cpu, 0,
					MSGBOX_CHANNEL_RECEIVE,
					arpaf_rxch_from_cpu_callback, NULL);
	msgcpu_txch = msgbox_alloc_channel(msgbox_cpu, 1,
					MSGBOX_CHANNEL_SEND,
					arpaf_txch_to_cpu_callback, NULL);
#endif

	medp->rec = arpaf_rxch_from_cpu_callback;
	medp->tx_done = arpaf_txch_to_cpu_callback;

#if defined(CONFIG_COMPONENTS_AW_ALSA_RPAF_REMOTE_AMP) && \
	defined(CONFIG_COMPONENTS_AW_ALSA_RPAF_READ_CH) && \
	defined(CONFIG_COMPONENTS_AW_ALSA_RPAF_WRITE_CH)
	ret = hal_msgbox_alloc_channel(medp,
				CONFIG_COMPONENTS_AW_ALSA_RPAF_REMOTE_AMP,
				CONFIG_COMPONENTS_AW_ALSA_RPAF_READ_CH,
				CONFIG_COMPONENTS_AW_ALSA_RPAF_WRITE_CH);
	awrpaf_debug("remote=%d, read=%d, write=%d\n",
			CONFIG_COMPONENTS_AW_ALSA_RPAF_REMOTE_AMP,
			CONFIG_COMPONENTS_AW_ALSA_RPAF_READ_CH,
			CONFIG_COMPONENTS_AW_ALSA_RPAF_WRITE_CH);
#else
	ret = hal_msgbox_alloc_channel(medp, 0, 1, 1);
#endif
	if (ret) {
		printf("[%s] failed !!! -> %d\n", __func__, __LINE__);
	}

#if 0
	/* 和另外一个DSP CPU系统通信接口(待定) */
	msgdsp_rxch = msgbox_alloc_channel(msgbox_dsp, 0,
					MSGBOX_CHANNEL_RECEIVE,
					arpaf_rxch_from_dsp_callback, NULL);
	msgdsp_txch = msgbox_alloc_channel(msgbox_dsp, 1,
					MSGBOX_CHANNEL_SEND,
					arpaf_txch_to_dsp_callback, NULL);
#endif
#endif
	/* 初始化互斥信号量和链表 */
	arpaf_priv->soc_mixer_list_mutex = arpaf_mutex_init();
	arpaf_list_init(arpaf_priv, LIST_TYPE_SOC_MIXER);

	arpaf_priv->hal_mixer_list_mutex = arpaf_mutex_init();
	arpaf_list_init(arpaf_priv, LIST_TYPE_HAL_MIXER);

	arpaf_priv->soc_substream_list_mutex = arpaf_mutex_init();
	arpaf_list_init(arpaf_priv, LIST_TYPE_SOC_SUBSTREAM);

	arpaf_priv->hal_substream_list_mutex = arpaf_mutex_init();
	arpaf_list_init(arpaf_priv, LIST_TYPE_HAL_SUBSTREAM);

	arpaf_priv->soc_component_list_mutex = arpaf_mutex_init();
	arpaf_list_init(arpaf_priv, LIST_TYPE_SOC_COMPONENT);

	arpaf_priv->hal_component_list_mutex = arpaf_mutex_init();
	arpaf_list_init(arpaf_priv, LIST_TYPE_HAL_COMPONENT);

	/* 创建audio信号收发队列 */
	arpaf_priv->MixerReceQueue = xQueueCreate(10, uxItemSize);
	for (i = 0; i < DSP_SOUND_CARDS; i++) {
		arpaf_priv->StreamCmdQueue[i][0] = xQueueCreate(4, uxItemSize);
		arpaf_priv->StreamCmdQueue[i][1] = xQueueCreate(4, uxItemSize);
	}

	arpaf_priv->ServerSendQueue = xQueueCreate(20, uxItemSize);
	arpaf_priv->ServerReceQueue = xQueueCreate(20, uxItemSize);

#if defined(AWRPAF_MSGBOX_SIMULATOR_MODE)
	arpaf_priv->MsgSendQueue = xQueueCreate(20, uxItemSize);
	arpaf_priv->MsgReceQueue = xQueueCreate(30, uxItemSize);
#endif

#ifdef CONFIG_COMPONENTS_AW_ALSA_RPAF_SUBSTREAM
	arpaf_priv->hal_substream_process = snd_dsp_hal_substream_process;
#else
	arpaf_priv->hal_substream_process = NULL;
#endif
#ifdef CONFIG_COMPONENTS_AW_ALSA_RPAF_DEBUG
	arpaf_priv->hal_debug_process = snd_dsp_hal_debug_process;
#else
	arpaf_priv->hal_debug_process = NULL;
#endif
#ifdef CONFIG_COMPONENTS_AW_ALSA_RPAF_COMPONENT
	arpaf_priv->hal_component_process = snd_dsp_hal_component_process;
#else
	arpaf_priv->hal_component_process = NULL;
#endif
#ifdef CONFIG_COMPONENTS_AW_ALSA_RPAF_MIXER
	arpaf_priv->hal_mixer_process = snd_dsp_hal_mixer_process;
#else
	arpaf_priv->hal_mixer_process = NULL;
#endif

#ifdef CONFIG_COMPONENTS_ALGO_GENERATE
	algo_generate_install();
#endif

	/* 创建音频服务 */
	xTaskCreate(pxAudioServerSendTask,
				"AudioSend", ulSendStackDepth,
				arpaf_priv,
				configAPPLICATION_AUDIO_PRIORITY,
				&arpaf_priv->ServerSendHandle);
	if (!arpaf_priv->ServerSendHandle)
		awrpaf_err("AudioSend Create failed.\n");

	xTaskCreate(pxAudioServerReceTask,
				"AudioRece", ulReceStackDepth,
				arpaf_priv,
				configAPPLICATION_AUDIO_PRIORITY,
				&arpaf_priv->ServerReceHandle);
	if (!arpaf_priv->ServerReceHandle)
		awrpaf_err("AudioRece Create failed.\n");
	snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_COMMON | 0x001);

	return ret;
}
