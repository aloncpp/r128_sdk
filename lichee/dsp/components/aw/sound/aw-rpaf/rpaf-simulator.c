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

#define ARPAF_ARGC 1U
#define ARPAF_SIMULATOR_COMMOND_HELP \
	"\r\narpaf_simulator <[-D card_num]> <-c channels> <-r rates> <-f bits>" \
	" <-p period_size> <-b buffer_size>" \
	" <-t seconds> <-H playback_card>\r\n" \
	" Expects 16 parameters, echos each in turn\n" \
	" eg: arpaf_simulator -D hw:snddaudio1 -c 8 -r 48000 -f 16 -p 512 -b 4096 -t 2 -H hw:snddaudio0\n"

extern struct arpaf_priv *arpaf_priv;

/* 256 * 4 = 1024 bytes */
#define ulReceStackDepth 512U
StackType_t puxReceStackBuffer[ulReceStackDepth];
StaticTask_t pxReceTaskBuffer;
/* 512 * 4 = 2048 bytes */
#define ulSendStackDepth 1024U
StackType_t puxSendStackBuffer[ulSendStackDepth];
StaticTask_t pxSendTaskBuffer;

//extern struct messagebox *msgbox_init_sx(enum msgbox_direction dir);
#if 0
int arpaf_rxch_callback_by_irqhandle(unsigned long index, void *data)
{
	printf("%s index:%lu\n", __func__, index);
	return 0;
}

int arpaf_txch_callback_by_irqhandle(unsigned long index, void *data)
{
	printf("%s index:%lu\n", __func__, index);
	return 0;
}
#endif

#ifdef CONFIG_COMPONENTS_FREERTOS_CLI
#if defined(AWRPAF_MSGBOX_SIMULATOR_MODE)
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
	printf("soc_substream->input_addr:%p\n", soc_substream->input_addr);
	printf("soc_substream->input_size:%d\n", soc_substream->input_size);
	printf("soc_substream->output_addr:%p\n", soc_substream->output_addr);
	printf("soc_substream->output_size:%d\n", soc_substream->output_size);

	printf("params.card = %d\n", soc_substream->params.card);
	printf("params.device = %d\n", soc_substream->params.device);
	printf("params.driver = %s\n", soc_substream->params.driver);
	printf("params.stream = %d\n", soc_substream->params.stream);
	printf("params.format = %d\n", soc_substream->params.format);
	printf("params.rate = %u\n", soc_substream->params.rate);
	printf("params.channels = %u\n", soc_substream->params.channels);
	printf("params.resample_rate = %u\n", soc_substream->params.resample_rate);
	printf("params.period_size = %u\n", soc_substream->params.period_size);
	printf("params.periods = %u\n", soc_substream->params.periods);
	printf("params.buffer_size = %u\n", soc_substream->params.buffer_size);

	printf("params.data_type = %d\n", soc_substream->params.data_type);
	printf("params.codec_type = %d\n", soc_substream->params.codec_type);
	printf("params.dts_data = 0%x\n", soc_substream->params.dts_data);
	printf("params.status = %d\n", soc_substream->params.status);
	printf("params.hw_stream = %u\n", soc_substream->params.hw_stream);
	printf("params.data_mode = %u\n", soc_substream->params.data_mode);
	printf("params.stream_wake = %u\n", soc_substream->params.stream_wake);

	printf("soc_substream->ret_val = %d\n", soc_substream->ret_val);
}

void pxAudioCmdItemFileSocSubstream(struct arpaf_priv *arpaf_priv)
{
	struct snd_soc_dsp_pcm_params *pcm_params;
	struct snd_dsp_hal_queue_item pvAudioCmdItem;
	unsigned int i = 0;
	static int j = 0;
	BaseType_t xStatus;
	unsigned int audio_cmd[] = {
		SND_SOC_DSP_PCM_PROBE,
		SND_SOC_DSP_PCM_SUSPEND,
		SND_SOC_DSP_PCM_RESUME,
		SND_SOC_DSP_PCM_REMOVE,

		/* the cmd of pcm stream interface & sync */
		SND_SOC_DSP_PCM_STARTUP,
		SND_SOC_DSP_PCM_HW_PARAMS,
		SND_SOC_DSP_PCM_PREPARE,
		SND_SOC_DSP_PCM_WRITEI,
		SND_SOC_DSP_PCM_READI,
		SND_SOC_DSP_PCM_START,
		SND_SOC_DSP_PCM_STOP,
		SND_SOC_DSP_PCM_SHUTDOWN,
	};
	struct snd_soc_dsp_substream soc_substream[12];

	j = ~j;

	for (i = 0; i < ARRAY_SIZE(audio_cmd); i++) {
		memset(&pvAudioCmdItem, 0, sizeof(struct snd_dsp_hal_queue_item));
		memset(&soc_substream[i], 0, sizeof(struct snd_soc_dsp_substream));

		soc_substream[i].input_size = 128;
		soc_substream[i].input_addr = 0;//malloc(soc_substream[i]->buf_size);
		soc_substream[i].output_size = 128;
		soc_substream[i].output_addr = 0;//malloc(soc_substream[i]->buf_size);
		soc_substream[i].cmd_val = audio_cmd[i];
		soc_substream[i].params_val = 0;

		/*
		 * 根据名字匹配:
		 * 0: audiocodec;
		 * 1: snddmic;
		 * 2: snddaudio0;
		 * 3: snddaudio1;
		 */
		pcm_params = &soc_substream[i].params;
		pcm_params->card = 0;
		pcm_params->device = 0;
		snprintf(pcm_params->driver, 28, "%s", "snddaudio0");
		/* 1:capture; 0:playback */
		pcm_params->stream = !!j?SND_STREAM_PLAYBACK:SND_STREAM_CAPTURE;
		pcm_params->format = SND_PCM_FORMAT_S16;
		pcm_params->rate = 48000;
		pcm_params->channels = 4;
		pcm_params->resample_rate = 16000;
		/* -- HW params -- */
		pcm_params->period_size = 1024; 	/* 中断周期 */
		pcm_params->periods = 8;          	/* 中断周期个数 */
		/* 在流中buffer务必一致大小, 代码中务必检查！ */
		/* 共享buf大小 = period_size * peiods */
		pcm_params->buffer_size = pcm_params->period_size * pcm_params->periods;
		/* data type */
		pcm_params->data_type = SND_DATA_TYPE_PCM;
		/* mp3 - aac */
		pcm_params->codec_type = SND_CODEC_TYPE_PCM;
		/* for dsp0 is 0, for dsp1 is 1 */
		pcm_params->hw_stream = 0;

		pvAudioCmdItem.soc_substream = &soc_substream[i];
		pvAudioCmdItem.pxAudioMsgVal = MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND;

		//arpaf_print_queue_item(&pvAudioCmdItem);
		/* 发送指令给MsgBox或者通告MsgBox的API发送指令 */
		xStatus = xQueueSendToBack(arpaf_priv->MsgSendQueue, &pvAudioCmdItem,
				pdMS_TO_TICKS(20));
		if (xStatus != pdPASS) {
			printf("[%s] xQueueSendToBack failed!\n", __func__);
			goto err_msg_send_queue_send;
		}

		vTaskDelay(pdMS_TO_TICKS(20));
err_msg_send_queue_send:
		continue;
	}
	for (i = 0; i < ARRAY_SIZE(audio_cmd); i++) {
		//rpaf_free(soc_substream[i]->buf_addr);;
		//rpaf_free(soc_substream[i]);;
	}
}

struct snd_soc_dsp_mixer dsp_soc_mixer[4];

void pxAudioCmdItemFileSocMixer(struct arpaf_priv *arpaf_priv)
{
	BaseType_t xStatus;
	int i = 0;
	struct snd_dsp_hal_queue_item pvAudioCmdItem;
	struct snd_soc_dsp_mixer *soc_mixer[4];
	unsigned int audio_cmd[] = {
		SND_SOC_DSP_MIXER_OPEN, 
		SND_SOC_DSP_MIXER_READ,
		SND_SOC_DSP_MIXER_WRITE,
		SND_SOC_DSP_MIXER_CLOSE,
	};

	for (i = 0; i < ARRAY_SIZE(audio_cmd); i++) {
		soc_mixer[i] = &dsp_soc_mixer[i];
		memset(&pvAudioCmdItem, 0, sizeof(struct snd_dsp_hal_queue_item));
		memset(soc_mixer[i], 0, sizeof(struct snd_soc_dsp_mixer));

		soc_mixer[i]->id = i;
		soc_mixer[i]->card = 0;
		soc_mixer[i]->device = 0;
		strncpy(soc_mixer[i]->driver, "snddaudio0", 32);
		strncpy(soc_mixer[i]->ctl_name, "sunxi daudio audio hub mode", 44);
		soc_mixer[i]->value = i;
		soc_mixer[i]->cmd_val = audio_cmd[i];
		soc_mixer[i]->params_val = 0;

		pvAudioCmdItem.pxAudioMsgVal = MSGBOX_SOC_DSP_AUDIO_MIXER_COMMAND;
		pvAudioCmdItem.soc_mixer = soc_mixer[i];

		/* 发送指令给MsgBox或者通告MsgBox的API发送指令 */
		xStatus = xQueueSendToBack(arpaf_priv->MsgSendQueue, &pvAudioCmdItem, 0);
		if (xStatus != pdPASS) {
			printf("[%s] xQueueSendToBack failed!\n", __func__);
			goto err_msg_send_queue_send;
		}
		vTaskDelay(pdMS_TO_TICKS(200));
err_msg_send_queue_send:
		continue;
	}
}

struct snd_soc_dsp_debug dsp_soc_debug[4];

void pxAudioCmdItemFileSocDebug(struct arpaf_priv *arpaf_priv)
{
	BaseType_t xStatus;
	int i = 0;
	struct snd_dsp_hal_queue_item pvAudioCmdItem;
	struct snd_soc_dsp_debug *soc_debug[4];
	unsigned int audio_cmd[] = {
		SND_SOC_DSP_DEBUG_SET_REG,
		SND_SOC_DSP_DEBUG_GET_REG, 
		SND_SOC_DSP_DEBUG_GET_HWPARAMS,
		SND_SOC_DSP_DEBUG_GET_PCM_STATUS,
	};
	/* 读还是写数值 */
	unsigned int buf0_val[] = {0x1, 0x2, 0x3, 0x4 ,0x5};
	unsigned int buf1_val[] = {0x6, 0x7, 0x8, 0x9 ,0xa};
	unsigned int buf2_val[] = {0x6, 0x7, 0x8, 0x9 ,0xa};
	unsigned int buf3_val[] = {0x6, 0x7, 0x8, 0x9 ,0xa};
	void *temp_buf[] = {buf0_val, buf1_val, buf2_val, buf3_val};

	for (i = 0; i < ARRAY_SIZE(audio_cmd); i++) {
		soc_debug[i] = &dsp_soc_debug[i];
		memset(&pvAudioCmdItem, 0, sizeof(struct snd_dsp_hal_queue_item));
		memset(soc_debug[i], 0, sizeof(struct snd_soc_dsp_debug));

		/*
		 * 根据名字匹配:
		 * 0: audiocodec;
		 * 1: snddmic;
		 * 2: snddaudio0;
		 * 3: snddaudio1;
		 */
		strncpy(soc_debug[i]->pcm_params.driver, "snddaudio0", 32);
		/* 共享内存地址，根据首末地址差分配空间大小 */
		soc_debug[i]->buf = temp_buf[i];
		/* 起始地址和结束地址 */
		soc_debug[i]->addr_start = 0x7033000;
		soc_debug[i]->addr_end = 0x7033010;
		soc_debug[i]->cmd_val = audio_cmd[i];
		soc_debug[i]->params_val = 0;

		soc_debug[i]->pcm_params.card = 0;
		soc_debug[i]->pcm_params.device = 0;

		/* 1:capture; 0:playback */
		soc_debug[i]->pcm_params.stream = (i % 2) ? SND_STREAM_PLAYBACK:SND_STREAM_CAPTURE;

		pvAudioCmdItem.pxAudioMsgVal = MSGBOX_SOC_DSP_AUDIO_DEBUG_COMMAND;
		pvAudioCmdItem.soc_debug = soc_debug[i];

		/* 发送指令给MsgBox或者通告MsgBox的API发送指令 */
		xStatus = xQueueSendToBack(arpaf_priv->MsgSendQueue, &pvAudioCmdItem, pdMS_TO_TICKS(100));
		if (xStatus != pdPASS) {
			printf("[%s] xQueueSendToBack failed!\n", __func__);
			goto err_msg_send_queue_send;
		}
		vTaskDelay(pdMS_TO_TICKS(200));
err_msg_send_queue_send:
		continue;
	}
}

static int arpaf_pcm_data_simulator_check(struct snd_soc_dsp_component *soc_component,
			unsigned int offset)
{
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_component->params;
	int i = 0;
	int j = 0;

	/* for every frame if it's pcm data */
	for (i = 0; i < soc_component->read_length/2/pcm_params->channels; i++) {
		/* for every channel data */
		for (j = 0; j < pcm_params->channels; j++) {
			switch (pcm_params->format) {
			default:
			case SND_PCM_FORMAT_S16_LE:
				if (*((unsigned short *)soc_component->read_addr + (i * pcm_params->channels) + j) !=
					i * pcm_params->channels + j + offset) {
					printf("%s format:%d, frames:%d check data:0x%x != 0x%x failed.\n",
						__func__, pcm_params->format, i, i * pcm_params->channels + j + offset,
						*((unsigned short *)soc_component->read_addr + (i * pcm_params->channels) + j));
					return -EFAULT;
				}
				break;
			}
		}
	}

	return 0;
}

static int arpaf_pcm_data_simulator_dump_check(struct snd_soc_dsp_component *soc_component,
			unsigned int offset)
{
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_component->params;
	int i = 0;
	int j = 0;

	/* for every frame if it's pcm data */
	for (i = 0; i < soc_component->dump_length[offset]/2/pcm_params->channels; i++) {
		/* for every channel data */
		for (j = 0; j < pcm_params->channels; j++) {
			switch (pcm_params->format) {
			default:
			case SND_PCM_FORMAT_S16_LE:
				if (*((unsigned short *)soc_component->dump_addr[offset] + (i * pcm_params->channels) + j) !=
					i * pcm_params->channels + j + offset) {
					printf("%s format:%d, type:%d, frames:%d, data:0x%x != 0x%x failed.\n",
						__func__, pcm_params->format, offset, i, i * pcm_params->channels + j + offset,
						*((unsigned short *)soc_component->dump_addr[offset] + (i * pcm_params->channels) + j));
					return -EFAULT;
				}
				break;
			}
		}
	}

	return 0;
}

void pxAudioCmdItemFileSocComponent(struct arpaf_priv *arpaf_priv)
{
	struct snd_soc_dsp_pcm_params *pcm_params;
	struct snd_dsp_hal_queue_item pvAudioCmdItem;
	unsigned int component_type = 0;
	unsigned int i = 0;
	static int j = 0;
	BaseType_t xStatus;
	unsigned int audio_cmd[] = {
		/* the cmd of interface & sync */
		SND_SOC_DSP_COMPONENT_CREATE,
		SND_SOC_DSP_COMPONENT_SUSPEND,
		SND_SOC_DSP_COMPONENT_RESUME,
		SND_SOC_DSP_COMPONENT_SW_PARAMS,
		SND_SOC_DSP_COMPONENT_STATUS,
		SND_SOC_DSP_COMPONENT_START,
		SND_SOC_DSP_COMPONENT_WRITE,
		SND_SOC_DSP_COMPONENT_READ,
		SND_SOC_DSP_COMPONENT_STOP,
		SND_SOC_DSP_COMPONENT_REMOVE,
	};

	int k = 0;
	int m = 0;
	struct snd_soc_dsp_component soc_component[ARRAY_SIZE(audio_cmd)];
	TickType_t xTickAverage = 0;
	TickType_t xTickAverage1 = 0;
	j = ~j;

	for (i = 0; i < ARRAY_SIZE(audio_cmd); i++) {
		memset(&pvAudioCmdItem, 0, sizeof(struct snd_dsp_hal_queue_item));
		memset(&soc_component[i], 0, sizeof(struct snd_soc_dsp_component));

		soc_component[i].cmd_val = audio_cmd[i];
		soc_component[i].params_val = 0;
		/* buf_length < buf_size */
		soc_component[i].write_length = 1024 * 2 * 4;
		soc_component[i].write_size = 1024 * 2 * 4;
		/* share data address */
		soc_component[i].write_addr = (unsigned int)rpaf_malloc(soc_component[i].write_size);
		soc_component[i].read_length = 1024 * 2 * 4;
		soc_component[i].read_size = 1024 * 2 * 4;
		soc_component[i].read_addr = (unsigned int)rpaf_malloc(soc_component[i].read_size);

		/* 代表的伴随音频流的组件还是独立操作用的组件 */
		soc_component[i].comp_mode = SND_DSP_COMPONENT_MODE_INDEPENDENCE;

		/* 代表的是该音频流有多少个组件（最多支持32个for 32bit machine）在用 */
		for (k = 0; k < 32; k++) {
			soc_component[i].component_sort[k] = -1;
			soc_component[i].dump_addr[k] = 0;
		}
		for (k = 0; k < 6; k++) {
			switch (k) {
			case 0:
				component_type = SND_DSP_COMPONENT_RESAMPLE;
				break;
			case 1:
				component_type = SND_DSP_COMPONENT_AGC;
				break;
			case 2:
				component_type = SND_DSP_COMPONENT_NS;
				break;
			case 3:
				component_type = SND_DSP_COMPONENT_AEC;
				break;
			case 4:
				component_type = SND_DSP_COMPONENT_EQ;
				break;
			case 5:
				component_type = SND_DSP_COMPONENT_DRC;
				break;
			default:
				continue;
			}
			soc_component[i].component_sort[k] = component_type;
			soc_component[i].component_type |= 0x1 << component_type;
			soc_component[i].dump_size = 1024 * 2 * 4;
			soc_component[i].dump_length[component_type] = 1024 * 2 * 4;
			soc_component[i].dump_addr[component_type] = (unsigned int)rpaf_malloc(soc_component[i].dump_size);
		}

		/*
		 * 根据名字匹配:
		 * 0: audiocodec;
		 * 1: snddmic;
		 * 2: snddaudio0;
		 * 3: snddaudio1;
		 */
		pcm_params = &soc_component[i].params;
		pcm_params->card = 0;
		pcm_params->device = 0;
		snprintf(pcm_params->driver, 28, "%s", "snddaudio0");


		/* 1:capture; 0:playback */
		pcm_params->stream = !!j?SND_STREAM_PLAYBACK:SND_STREAM_CAPTURE;
		pcm_params->format = SND_PCM_FORMAT_S16;
		pcm_params->rate = 48000;
		pcm_params->channels = 4;
		pcm_params->resample_rate = 16000;
		/* -- HW params -- */
		pcm_params->period_size = 1024; 	/* 中断周期 */
		pcm_params->periods = 8;          	/* 中断周期个数 */
		/* 在流中buffer务必一致大小, 代码中务必检查！ */
		/* 共享buf大小 = period_size * peiods */
		pcm_params->buffer_size = pcm_params->period_size * pcm_params->periods;
		/* data type */
		pcm_params->data_type = SND_DATA_TYPE_PCM;
		/* mp3 - aac */
		pcm_params->codec_type = SND_CODEC_TYPE_PCM;
		/* for dsp0 is 0, for dsp1 is 1 */
		pcm_params->hw_stream = 0;

		pvAudioCmdItem.soc_component = &soc_component[i];
		pvAudioCmdItem.pxAudioMsgVal = MSGBOX_SOC_DSP_AUDIO_COMPONENT_COMMAND;

		if (audio_cmd[i] == SND_SOC_DSP_COMPONENT_WRITE) {
			for (k = 0; k < 100; k++) {
				TickType_t xTicksSend = xTaskGetTickCount();
				xStatus = xQueueSendToBack(arpaf_priv->MsgSendQueue, &pvAudioCmdItem,
						pdMS_TO_TICKS(64));
				if (xStatus != pdPASS) {
					awrpaf_err("MsgSendQueue failed! k = %d\n", k);
					goto err_msg_send_queue_send;
				}
				xStatus = xQueueReceive(arpaf_priv->MsgReceQueue, &pvAudioCmdItem,
					portMAX_DELAY);
				if (xStatus != pdPASS) {
					awrpaf_err("MsgReceQueue failed! k = %d\n", k);
					goto err_msg_rece_queue_receive;
				}
				TickType_t xTicksRece = xTaskGetTickCount();

				xTickAverage += xTicksRece - xTicksSend;

				TickType_t xTicksSend1= xTaskGetTickCount();
				for (m = 0; m < 6; m++) {
					switch (m) {
					case 0:
						component_type = SND_DSP_COMPONENT_RESAMPLE;
						break;
					case 1:
						component_type = SND_DSP_COMPONENT_AGC;
						break;
					case 2:
						component_type = SND_DSP_COMPONENT_NS;
						break;
					case 3:
						component_type = SND_DSP_COMPONENT_AEC;
						break;
					case 4:
						component_type = SND_DSP_COMPONENT_EQ;
						break;
					case 5:
						component_type = SND_DSP_COMPONENT_DRC;
						break;
					default:
						continue;
					}
					if (arpaf_pcm_data_simulator_dump_check(pvAudioCmdItem.soc_component,
							component_type) < 0) {
						printf("simulator dump check Faile--->>>>>>i:%d, k:%d, m = %d\n", i, k, m);
					}
				}
				if (arpaf_pcm_data_simulator_check(pvAudioCmdItem.soc_component,
						component_type) < 0) {
					printf("simulator check Faile--->>>>>>i:%d, k = %d\n", i, k);
				}
				TickType_t xTicksRece1= xTaskGetTickCount();
				xTickAverage1 += xTicksRece1 - xTicksSend1;
			}
			printf("xTickAverage(counts:100):%d, %dms, xTickAverage1(counts):%d, %dms\n",
					xTickAverage, xTickAverage * 1000 / configTICK_RATE_HZ,
					xTickAverage1, xTickAverage1 * 1000 / configTICK_RATE_HZ);
		} else {
			xStatus = xQueueSendToBack(arpaf_priv->MsgSendQueue, &pvAudioCmdItem,
					pdMS_TO_TICKS(64));
			if (xStatus != pdPASS) {
				awrpaf_err("MsgSendQueue failed! k = %d\n", k);
				goto err_msg_send_queue_send;
			}
			xStatus = xQueueReceive(arpaf_priv->MsgReceQueue, &pvAudioCmdItem,
					portMAX_DELAY);
			if (xStatus != pdPASS) {
				awrpaf_err("MsgReceQueue failed! k = %d\n", k);
				goto err_msg_rece_queue_receive;
			}
		}

err_msg_send_queue_send:
err_msg_rece_queue_receive:
		continue;
	}
	for (i = 0; i < ARRAY_SIZE(audio_cmd); i++) {
		rpaf_free((void *)soc_component[i].read_addr);
		for (k = 0; k < 6; k++) {
			switch (k) {
			case 0:
				component_type = SND_DSP_COMPONENT_RESAMPLE;
				break;
			case 1:
				component_type = SND_DSP_COMPONENT_AGC;
				break;
			case 2:
				component_type = SND_DSP_COMPONENT_NS;
				break;
			case 3:
				component_type = SND_DSP_COMPONENT_AEC;
				break;
			case 4:
				component_type = SND_DSP_COMPONENT_EQ;
				break;
			case 5:
				component_type = SND_DSP_COMPONENT_DRC;
				break;
			default:
				continue;
			}
			rpaf_free((void *)soc_component[i].dump_addr[component_type]);
		}
		rpaf_free((void *)soc_component[i].write_addr);
	}
}

#define RPAF_TEST_API_CALL 2

/* for callback */
int pcm_module_set_playback_hw_params(struct snd_soc_dsp_pcm_params *pcm_params)
{
	pcm_params->stream = SND_STREAM_PLAYBACK;
	snd_dsp_pcm_module_set_fmt(pcm_params, SND_PCM_FORMAT_S16);
	snd_dsp_pcm_module_set_rate(pcm_params, 48000);
	snd_dsp_pcm_module_set_resample(pcm_params, 48000);
	snd_dsp_pcm_module_set_channel(pcm_params, 4);

	/* -- HW params -- */
	snd_dsp_pcm_module_set_period_size(pcm_params, 1024); 	/* 中断周期 */
	snd_dsp_pcm_module_set_periods(pcm_params, 8);          /* 中断周期个数 */
	/* 在流中buffer务必一致大小, 代码中务必检查！ */
	/* 共享buf大小 = period_size * peiods */
	snd_dsp_pcm_module_set_buffer_size(pcm_params);
	/* data type */
	pcm_params->data_type = SND_DATA_TYPE_PCM;
	/* mp3 - aac */
	pcm_params->codec_type = SND_CODEC_TYPE_PCM;
	/* for dsp0 is 0, for dsp1 is 1 */
	pcm_params->hw_stream = 0;

	return 0;
}

static struct snd_soc_dsp_component playback_soc_component;

void soc_component_set_playback_hw_params(struct arpaf_priv *arpaf_priv,
				struct snd_soc_dsp_component *soc_component)
{
	struct snd_soc_dsp_pcm_params *pcm_params;
	struct snd_dsp_hal_queue_item pvAudioCmdItem;
	unsigned int component_type = 0;
	BaseType_t xStatus;
	int k = 0;

	memset(&pvAudioCmdItem, 0, sizeof(struct snd_dsp_hal_queue_item));
	memset(soc_component, 0, sizeof(struct snd_soc_dsp_component));

	soc_component->cmd_val = SND_SOC_DSP_COMPONENT_SW_PARAMS;
	soc_component->params_val = 0;
	/* buf_length < buf_size */
	soc_component->write_length = 1024 * 2 * 4;
	soc_component->write_size = 1024 * 2 * 4;
	/* share data address */
	soc_component->write_addr = (unsigned int)rpaf_malloc(soc_component->write_size);

	soc_component->read_length = 1024 * 2 * 4;
	soc_component->read_size = 1024 * 2 * 4;
	soc_component->read_addr = (unsigned int)rpaf_malloc(soc_component->read_size);

	/* 代表的伴随音频流的组件还是独立操作用的组件 */
	soc_component->comp_mode = SND_DSP_COMPONENT_MODE_STREAM;

	/* 代表的是该音频流有多少个组件（最多支持32个for 32bit machine）在用 */
	for (k = 0; k < 32; k++) {
		soc_component->component_sort[k] = -1;
		soc_component->dump_addr[k] = 0;
	}
	for (k = 0; k < 3; k++) {
		switch (k) {
		case 0:
			component_type = SND_DSP_COMPONENT_RESAMPLE;
			break;
		case 1:
			component_type = SND_DSP_COMPONENT_EQ;
			break;
		case 2:
			component_type = SND_DSP_COMPONENT_DRC;
			break;
		default:
			continue;
		}
		soc_component->component_sort[k] = component_type;
		soc_component->component_type |= 0x1 << component_type;
		soc_component->dump_length[component_type] = 1024 * 2 * 4;
		soc_component->dump_size = 1024 * 2 * 4;
		soc_component->dump_addr[component_type] = (unsigned int)rpaf_malloc(soc_component->dump_size);
	}

	/*
	 * 根据名字匹配:
	 * 0: audiocodec;
	 * 1: snddmic;
	 * 2: snddaudio0;
	 * 3: snddaudio1;
	 */
	pcm_params = &soc_component->params;
	pcm_params->card = 0;
	pcm_params->device = 0;
	snprintf(pcm_params->driver, 28, "%s", "snddaudio0");


	pcm_module_set_playback_hw_params(pcm_params);

	pvAudioCmdItem.soc_component = soc_component;
	pvAudioCmdItem.pxAudioMsgVal = MSGBOX_SOC_DSP_AUDIO_COMPONENT_COMMAND;

	xStatus = xQueueSendToBack(arpaf_priv->MsgSendQueue, &pvAudioCmdItem,
					pdMS_TO_TICKS(64));
	if (xStatus != pdPASS) {
		awrpaf_err("MsgSendQueue failed! k = %d\n", k);
		goto err_msg_send_queue_send;
	}
	int card = pcm_params->card;
	int stream = pcm_params->stream;
	xStatus = xQueueReceive(arpaf_priv->StreamCmdQueue[card][stream],
			 &pvAudioCmdItem, portMAX_DELAY);
	if (xStatus != pdPASS) {
		awrpaf_err("\n");
	}

	soc_component->cmd_val = SND_SOC_DSP_COMPONENT_START;
	xStatus = xQueueSendToBack(arpaf_priv->MsgSendQueue, &pvAudioCmdItem,
					pdMS_TO_TICKS(64));
	if (xStatus != pdPASS) {
		awrpaf_err("MsgSendQueue failed! k = %d\n", k);
		goto err_msg_send_queue_send;
	}
	xStatus = xQueueReceive(arpaf_priv->StreamCmdQueue[card][stream],
			 &pvAudioCmdItem, portMAX_DELAY);
	if (xStatus != pdPASS) {
		awrpaf_err("\n");
	}

err_msg_send_queue_send:
	rpaf_free((void *)soc_component->read_addr);
	rpaf_free((void *)soc_component->write_addr);
	for (k = 0; k < 3; k++) {
		switch (k) {
		case 0:
			component_type = SND_DSP_COMPONENT_RESAMPLE;
			break;
		case 1:
			component_type = SND_DSP_COMPONENT_EQ;
			break;
		case 2:
			component_type = SND_DSP_COMPONENT_DRC;
			break;
		default:
			continue;
		}
		rpaf_free((void *)soc_component->dump_addr[component_type]);
	}
}

int pcm_module_set_capture_hw_params(struct snd_soc_dsp_pcm_params *pcm_params)
{
	pcm_params->stream = SND_STREAM_CAPTURE;
	snd_dsp_pcm_module_set_fmt(pcm_params, SND_PCM_FORMAT_S16);
	snd_dsp_pcm_module_set_rate(pcm_params, 48000);
	snd_dsp_pcm_module_set_resample(pcm_params, 48000);
	snd_dsp_pcm_module_set_channel(pcm_params, 4);

	/* -- HW params -- */
	snd_dsp_pcm_module_set_period_size(pcm_params, 1024); 	/* 中断周期 */
	snd_dsp_pcm_module_set_periods(pcm_params, 8);       	/* 中断周期个数 */
	/* 在流中buffer务必一致大小, 代码中务必检查！ */
	/* 共享buf大小 = period_size * peiods */
	snd_dsp_pcm_module_set_buffer_size(pcm_params);
	/* data type */
	pcm_params->data_type = SND_DATA_TYPE_PCM;
	/* mp3 - aac */
	pcm_params->codec_type = SND_CODEC_TYPE_PCM;
	/* for dsp0 is 0, for dsp1 is 1 */
	pcm_params->hw_stream = 0;

	return 0;
}

static struct snd_soc_dsp_component capture_soc_component;

void soc_component_set_capture_hw_params(struct arpaf_priv *arpaf_priv,
				struct snd_soc_dsp_component *soc_component)
{
	struct snd_soc_dsp_pcm_params *pcm_params;
	struct snd_dsp_hal_queue_item pvAudioCmdItem;
	unsigned int component_type = 0;
	BaseType_t xStatus;
	int k = 0;

	memset(&pvAudioCmdItem, 0, sizeof(struct snd_dsp_hal_queue_item));
	memset(soc_component, 0, sizeof(struct snd_soc_dsp_component));

	soc_component->cmd_val = SND_SOC_DSP_COMPONENT_SW_PARAMS;
	soc_component->params_val = 0;
	/* buf_length < buf_size */
	soc_component->write_length = 1024 * 2 * 4;
	soc_component->write_size = 1024 * 2 * 4;
	/* share data address */
	soc_component->write_addr = (unsigned int)rpaf_malloc(soc_component->write_size);

	soc_component->read_length = 1024 * 2 * 4;
	soc_component->read_size = 1024 * 2 * 4;
	soc_component->read_addr = (unsigned int)rpaf_malloc(soc_component->read_size);

	/* 代表的伴随音频流的组件还是独立操作用的组件 */
	soc_component->comp_mode = SND_DSP_COMPONENT_MODE_STREAM;

	/* 代表的是该音频流有多少个组件（最多支持32个for 32bit machine）在用 */
	for (k = 0; k < 32; k++) {
		soc_component->component_sort[k] = -1;
		soc_component->dump_addr[k] = 0;
	}
	for (k = 0; k < 3; k++) {
		switch (k) {
		case 0:
			component_type = SND_DSP_COMPONENT_AGC;
			break;
		case 1:
			component_type = SND_DSP_COMPONENT_NS;
			break;
		case 2:
			component_type = SND_DSP_COMPONENT_AEC;
			break;
		default:
			continue;
		}
		soc_component->component_sort[k] = component_type;
		soc_component->component_type |= 0x1 << component_type;
		soc_component->dump_length[component_type] = 1024 * 2 * 4;
		soc_component->dump_size = 1024 * 2 * 4;
		soc_component->dump_addr[component_type] = (unsigned int)rpaf_malloc(soc_component->dump_size);
	}

	/*
	 * 根据名字匹配:
	 * 0: audiocodec;
	 * 1: snddmic;
	 * 2: snddaudio0;
	 * 3: snddaudio1;
	 */
	pcm_params = &soc_component->params;
	pcm_params->card = 0;
	pcm_params->device = 0;
	snprintf(pcm_params->driver, 28, "%s", "snddaudio0");


	pcm_module_set_capture_hw_params(pcm_params);

	pvAudioCmdItem.soc_component = soc_component;
	pvAudioCmdItem.pxAudioMsgVal = MSGBOX_SOC_DSP_AUDIO_COMPONENT_COMMAND;

	xStatus = xQueueSendToBack(arpaf_priv->MsgSendQueue, &pvAudioCmdItem,
					pdMS_TO_TICKS(64));
	if (xStatus != pdPASS) {
		awrpaf_err("MsgSendQueue failed! k = %d\n", k);
		goto err_msg_send_queue_send;
	}
	int card = pcm_params->card;
	int stream = pcm_params->stream;
	xStatus = xQueueReceive(arpaf_priv->StreamCmdQueue[card][stream],
			 &pvAudioCmdItem, portMAX_DELAY);
	if (xStatus != pdPASS)
		awrpaf_err("\n");

	soc_component->cmd_val = SND_SOC_DSP_COMPONENT_START;
	xStatus = xQueueSendToBack(arpaf_priv->MsgSendQueue, &pvAudioCmdItem,
					pdMS_TO_TICKS(64));
	if (xStatus != pdPASS) {
		awrpaf_err("MsgSendQueue failed! k = %d\n", k);
		goto err_msg_send_queue_send;
	}
	xStatus = xQueueReceive(arpaf_priv->StreamCmdQueue[card][stream],
			 &pvAudioCmdItem, portMAX_DELAY);
	if (xStatus != pdPASS)
		awrpaf_err("\n");

err_msg_send_queue_send:
	rpaf_free((void *)soc_component->write_addr);
	rpaf_free((void *)soc_component->read_addr);
	for (k = 0; k < 3; k++) {
		switch (k) {
		case 0:
			component_type = SND_DSP_COMPONENT_AGC;
			break;
		case 1:
			component_type = SND_DSP_COMPONENT_NS;
			break;
		case 2:
			component_type = SND_DSP_COMPONENT_AEC;
			break;
		default:
			continue;
		}
		rpaf_free((void *)soc_component->dump_addr[component_type]);
	}
}

//cmd_val:
//	soc_substream->cmd_val = SND_SOC_DSP_PCM_PROBE;
//	soc_substream->cmd_val = SND_SOC_DSP_PCM_SUSPEND;
//	soc_substream->cmd_val = SND_SOC_DSP_PCM_RESUME;
//	soc_substream->cmd_val = SND_SOC_DSP_PCM_REMOVE;
//	soc_substream->cmd_val = SND_SOC_DSP_PCM_STARTUP;
//	soc_substream->cmd_val = SND_SOC_DSP_PCM_HW_PARAMS;
//	soc_substream->cmd_val = SND_SOC_DSP_PCM_START;
//	soc_substream->cmd_val = SND_SOC_DSP_PCM_PREPARE;
//	soc_substream->cmd_val = SND_SOC_DSP_PCM_STOP;
//	soc_substream->cmd_val = SND_SOC_DSP_PCM_SHUTDOWN;
typedef int (*func_set_params)(struct snd_soc_dsp_pcm_params *pcm_params);

int snd_dsp_pcm_module_switch_ops(struct arpaf_priv *arpaf_priv,
		struct snd_soc_dsp_substream *soc_substream, unsigned int cmd_val,
		func_set_params set_params)
{
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_substream->params;
	int card = pcm_params->card;
	int stream = pcm_params->stream;
	struct snd_dsp_hal_queue_item pvAudioCmdItem;
	BaseType_t xStatus;

	memset(&pvAudioCmdItem, 0, sizeof(struct snd_dsp_hal_queue_item));

	soc_substream->cmd_val = cmd_val;
	if ((cmd_val == SND_SOC_DSP_PCM_HW_PARAMS) &&
		(set_params != NULL)) {
		set_params(&soc_substream->params);
	}

	pvAudioCmdItem.soc_substream = soc_substream;
	pvAudioCmdItem.pxAudioMsgVal = MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND;

	xStatus = xQueueSendToBack(arpaf_priv->MsgSendQueue, &pvAudioCmdItem,
				pdMS_TO_TICKS(20));
	if (xStatus != pdPASS) {
		awrpaf_err("\n");
	}

//	awrpaf_debug("soc_substream->params.stream:%d, cmd_val:%d\n",
//		soc_substream->params.stream, soc_substream->cmd_val);

	memset(&pvAudioCmdItem, 0, sizeof(struct snd_dsp_hal_queue_item));
	xStatus = xQueueReceive(arpaf_priv->StreamCmdQueue[card][stream],
			 &pvAudioCmdItem, portMAX_DELAY);
	if (xStatus != pdPASS) {
		awrpaf_err("\n");
	}
	if (pvAudioCmdItem.soc_substream->ret_val < 0) {
		awrpaf_err("\n");
		return pvAudioCmdItem.soc_substream->ret_val;
	}

	return 0;
}

static int arpaf_pcm_data_fill(struct snd_soc_dsp_substream *soc_substream)
{
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_substream->params;
	int i = 0;
	int j = 0;

	memset(soc_substream->input_addr, 0, soc_substream->input_size);

	/* for every frame */
	for (i = 0; i < soc_substream->input_size/2/pcm_params->channels; i++) {
		/* for every channel data */
		for (j = 0; j < pcm_params->channels; j++) {
			switch (pcm_params->format) {
			default:
			case SND_PCM_FORMAT_S16_LE:
				*((unsigned short *)soc_substream->input_addr + (i * pcm_params->channels) + j) =
						i * pcm_params->channels + j;
				break;
			}
		}
	}
	return 0;
}

static int arpaf_pcm_data_check(struct snd_soc_dsp_substream *soc_substream, unsigned int offset)
{
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_substream->params;
	int i = 0;
	int j = 0;

	/* for every frame */
	for (i = 0; i < soc_substream->output_size/2/pcm_params->channels; i++) {
		/* for every channel data */
		for (j = 0; j < pcm_params->channels; j++) {
			switch (pcm_params->format) {
			default:
			case SND_PCM_FORMAT_S16_LE:
				if (*((unsigned short *)soc_substream->output_addr + (i * pcm_params->channels) + j) !=
					((i * pcm_params->channels + j + offset) & 0xFFFF)) {
					printf("%s format:%d, frames:%d check data:0x%x != 0x%x failed.\n",
						__func__, pcm_params->format, i, i * pcm_params->channels + j + offset,
						*((unsigned short *)soc_substream->output_addr + (i * pcm_params->channels) + j));
					return -EFAULT;
				}
				break;
			}
		}
	}
	return 0;
}

void pxAudioCaptureTask(void *arg)
{
	struct arpaf_priv *arpaf_priv = arg;
	const char *func_name = __func__;
	struct snd_soc_dsp_substream soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_substream.params;
	unsigned int component_type = 0;
	int i = 0;
	int m = 0;
	int k = 0;

	/*
	 * 根据名字匹配:
	 * 0: audiocodec;
	 * 1: snddmic;
	 * 2: snddaudio0;
	 * 3: snddaudio1;
	 */
	pcm_params->card = 0;
	pcm_params->device = 0;
//	snprintf(pcm_params->driver, 28, "%s", "audiocodec");
	snprintf(pcm_params->driver, 28, "%s", "snddaudio0");
	/* 1:capture; 0:playback */
	pcm_params->stream = SND_STREAM_CAPTURE;

	/* 测试audio standby read pcm data */
	soc_substream.audio_standby = 1;

	soc_substream.output_size = 1024 * 2 * 4;
	soc_substream.output_addr = rpaf_malloc(soc_substream.output_size);
	memset(soc_substream.output_addr, 0, soc_substream.output_size);

	soc_substream.input_size = 1024 * 2 * 4;
	soc_substream.input_addr = rpaf_malloc(soc_substream.input_size);
	memset(soc_substream.input_addr, 0, soc_substream.input_size);

	arpaf_thread_start(func_name);
	for (i = 0; ; i++) {
		printf("------------Capture Start[i:%d]-----------\n", i);
		snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
					SND_SOC_DSP_PCM_PROBE, NULL);
		//snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
		//			SND_SOC_DSP_PCM_SUSPEND, NULL);
		//snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
		//			SND_SOC_DSP_PCM_RESUME, NULL);

		snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
					SND_SOC_DSP_PCM_STARTUP, NULL);

		soc_component_set_capture_hw_params(arpaf_priv, &capture_soc_component);

		snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
					SND_SOC_DSP_PCM_HW_PARAMS,
					pcm_module_set_capture_hw_params);

		snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
					SND_SOC_DSP_PCM_PREPARE, NULL);
//		snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
//					SND_SOC_DSP_PCM_START, NULL);
		for (k = 0; ; k++) {
			snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
					SND_SOC_DSP_PCM_READI, NULL);
			for (m = 0; m < 3; m++) {
				switch (m) {
				case 0:
					component_type = SND_DSP_COMPONENT_AGC;
					break;
				case 1:
					component_type = SND_DSP_COMPONENT_NS;
					break;
				case 2:
					component_type = SND_DSP_COMPONENT_AEC;
					break;
				default:
					continue;
				}
				if (arpaf_pcm_data_simulator_dump_check(&capture_soc_component,
						component_type) < 0) {
					printf("simulator dump check Faile--->>>>>>i:%d, k:%d, m = %d\n", i, k, m);
				}
			}
			arpaf_pcm_data_check(&soc_substream, component_type);
		}
		snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
					SND_SOC_DSP_PCM_STOP, NULL);
		snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
					SND_SOC_DSP_PCM_SHUTDOWN, NULL);
		snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
					SND_SOC_DSP_PCM_REMOVE, NULL);
		printf("-------------Capture Stop[i:%d]--------------\n", i);
	}
	arpaf_thread_stop(func_name);
	vTaskDelete(NULL);
}

void pxAudioPlaybackTask(void *arg)
{
	struct arpaf_priv *arpaf_priv = arg;
	const char *func_name = __func__;
	struct snd_soc_dsp_substream soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_substream.params;
	unsigned int component_type = 0;
	int m = 0;
//	wav_file_t *wav_file = find_builtin_wav_file(NULL);
//	wav_header_t *wav_header = (wav_header_t *)wav_file->start;
	int i = 0;
//	int j = 0;
	/*
	 * 根据名字匹配:
	 * 0: audiocodec;
	 * 1: snddmic;
	 * 2: snddaudio0;
	 * 3: snddaudio1;
	 */

	pcm_params->card = 0;
	pcm_params->device = 0;
//	snprintf(pcm_params->driver, 28, "%s", "audiocodec");
	snprintf(pcm_params->driver, 28, "%s", "snddaudio0");

	/* 1:capture; 0:playback */
	pcm_params->stream = SND_STREAM_PLAYBACK;

	soc_substream.output_size = 1024 * 2 * 4;
	soc_substream.output_addr = rpaf_malloc(soc_substream.output_size);
	memset(soc_substream.output_addr, 0, soc_substream.output_size);

	soc_substream.input_size = 1024 * 2 * 4;
	soc_substream.input_addr = rpaf_malloc(soc_substream.input_size);
	memset(soc_substream.input_addr, 0, soc_substream.input_size);

//	int frame_count = wav_header->dataSize / (1024 * 2);

	arpaf_thread_start(func_name);

//	for (i = 0; ; i++) {
//		printf("------------Playback Start[i:%d]-----------\n", i);
		snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
					SND_SOC_DSP_PCM_PROBE, NULL);
		//snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
		//			SND_SOC_DSP_PCM_SUSPEND, NULL);
		//snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
		//			SND_SOC_DSP_PCM_RESUME, NULL);

		snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
					SND_SOC_DSP_PCM_STARTUP, NULL);

		soc_component_set_playback_hw_params(arpaf_priv, &playback_soc_component);

		snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
					SND_SOC_DSP_PCM_HW_PARAMS,
					pcm_module_set_playback_hw_params);

		snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
					SND_SOC_DSP_PCM_PREPARE, NULL);
		//snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
		//			SND_SOC_DSP_PCM_START, NULL);
		//
	for (i = 0; ; i++) {
//		printf("------------Playback Start[i:%d]-----------\n", i);
//		for (j = 0; j < frame_count; j++) {
//			memcpy(soc_substream.buf_addr,
//				((char *)wav_file->start) + sizeof(wav_header_t) +
//					j * soc_substream.buf_size,
//				soc_substream.buf_size);
			arpaf_pcm_data_fill(&soc_substream);
			snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
					SND_SOC_DSP_PCM_WRITEI, NULL);
			for (m = 0; m < 3; m++) {
				switch (m) {
				case 0:
					component_type = SND_DSP_COMPONENT_RESAMPLE;
					break;
				case 1:
					component_type = SND_DSP_COMPONENT_EQ;
					break;
				case 2:
					component_type = SND_DSP_COMPONENT_DRC;
					break;
				default:
					continue;
				}
				if (arpaf_pcm_data_simulator_dump_check(&playback_soc_component,
						component_type) < 0) {
					printf("simulator dump check Faile--->>>>>>i:%d, m = %d\n", i, m);
				}
			}
//		}
//		printf("-------------Playback Stop[i:%d]--------------\n", i);
	}
		snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
					SND_SOC_DSP_PCM_STOP, NULL);
		snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
					SND_SOC_DSP_PCM_SHUTDOWN, NULL);
		snd_dsp_pcm_module_switch_ops(arpaf_priv, &soc_substream,
					SND_SOC_DSP_PCM_REMOVE, NULL);
//		printf("-------------Playback Stop[i:%d]--------------\n", i);
//	}
	arpaf_thread_stop(func_name);
	vTaskDelete(NULL);
}


void pxAudioMsgReceTask(void *arg)
{
	BaseType_t xStatus;
	int ret = 0;
	const TickType_t xTicksToWait = pdMS_TO_TICKS(10 * 1000);
	struct snd_dsp_hal_queue_item pvAudioCmdItem;
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_soc_dsp_component *soc_component;
	struct snd_soc_dsp_pcm_params *pcm_params;
	struct arpaf_priv *arpaf_priv = arg;
	const char *func_name = __func__;

	arpaf_thread_start(func_name);
	for (;;) {
#if (RPAF_TEST_API_CALL == 1)
		vTaskDelay(pdMS_TO_TICKS(1000));
#else
		if ((ret = uxQueueMessagesWaiting(arpaf_priv->MsgReceQueue)) != 0) {
			awrpaf_err("QM Waiting: %d!\n", ret);
		}
		xStatus = xQueueReceive(arpaf_priv->MsgReceQueue, &pvAudioCmdItem, xTicksToWait);
		if (xStatus != pdPASS) {
			awrpaf_err("MsgReceQueue Rece failed!\n");
			goto err_msg_rece_queue_receive;
		}
//		awrpaf_info("++++++++++Start+++++++++\n");
#if (RPAF_TEST_API_CALL == 2)
		/* 将返回信息给到相应任务 */
		switch (pvAudioCmdItem.pxAudioMsgVal) {
		case MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND:
			soc_substream = pvAudioCmdItem.soc_substream;
			pcm_params = &soc_substream->params;
			break;
		case MSGBOX_SOC_DSP_AUDIO_COMPONENT_COMMAND:
			soc_component = pvAudioCmdItem.soc_component;
			pcm_params = &soc_component->params;
			break;
		}
		int card = pcm_params->card;
		int stream = pcm_params->stream;
		xStatus = xQueueSendToBack(arpaf_priv->StreamCmdQueue[card][stream],
					&pvAudioCmdItem, pdMS_TO_TICKS(20));
		if (xStatus != pdPASS)
			awrpaf_err("\n");

#elif (RPAF_TEST_API_CALL == 3)
		/* 调用msgbox指令发送给Linux */
#endif
//		awrpaf_info("++++++++++Stop+++++++++\n");
err_msg_rece_queue_receive:
#endif
		continue;
	}
	arpaf_thread_stop(func_name);
	vTaskDelete(NULL);
}

void pxAudioMsgSendTask(void *arg)
{
	struct arpaf_priv *arpaf_priv = arg;
	const char *func_name = __func__;

	arpaf_thread_start(func_name);
#if (RPAF_TEST_API_CALL == 1)
	int i = 0;
	/* test rpaf api call */
	for (;;) {
		printf("----------count:%d---------\n", i++);
		//pxAudioCmdItemFileSocMixer(arpaf_priv);
		//pxAudioCmdItemFileSocSubstream(arpaf_priv);
		//pxAudioCmdItemFileSocDebug(arpaf_priv);
		pxAudioCmdItemFileSocComponent(arpaf_priv);
		//vTaskDelay(pdMS_TO_TICKS(1000));
	}
#elif (RPAF_TEST_API_CALL == 2)
	/* test rpaf audio stream */
#if 1
	xTaskCreate(pxAudioPlaybackTask, "aplay", 1024 * 4,
				arpaf_priv,
				configAPPLICATION_AUDIO_PRIORITY,
				&arpaf_priv->PlaybackHandle);
#endif
#if 1
	xTaskCreate(pxAudioCaptureTask, "arecord", 1024 * 4,
				arpaf_priv,
				configAPPLICATION_AUDIO_PRIORITY,
				&arpaf_priv->CaptureHandle);
#endif
#endif
	arpaf_thread_stop(func_name);
	vTaskDelete(NULL);
}

/* 模拟信号收发 */
int arpaf_simulator(int argc, char ** argv)
{
	arpaf_priv->MsgReceHandle = xTaskCreateStatic(pxAudioMsgReceTask,
						"MsgRece", ulReceStackDepth,
						arpaf_priv,
						configAPPLICATION_AUDIO_PRIORITY,
						puxReceStackBuffer,
						&pxReceTaskBuffer);
	arpaf_priv->MsgSendHandle = xTaskCreateStatic(pxAudioMsgSendTask,
						"MsgSend", ulSendStackDepth,
						arpaf_priv,
						configAPPLICATION_AUDIO_PRIORITY,
						puxSendStackBuffer,
						&pxSendTaskBuffer);
	while (1)
		vTaskDelay(pdMS_TO_TICKS(2000));
	/* 不会抵达 */
	return 0;
}
#else
int arpaf_simulator(int argc, char ** argv)
{
	return 0;
}
#endif

static BaseType_t cmd_arpaf_simulator_handler(char *pcWriteBuffer, size_t xWriteBufferLen,
				const char *pcCommandString)
{
	const char *pcParameter;
	BaseType_t xParameterStringLength, xReturn = 0;
	static UBaseType_t uxParameterNumber = 0;
	static char *arpaf_argv[ARPAF_ARGC] = {NULL};
	char *command_string = NULL;
	int i = 0;

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	(void) pcCommandString;
	(void) xWriteBufferLen;
	configASSERT(pcWriteBuffer);

	if(uxParameterNumber < ARPAF_ARGC)
	{
		/* Obtain the parameter string. */
		pcParameter = FreeRTOS_CLIGetParameter(
					/* The command string itself. */
					pcCommandString,
					/* Return the next parameter. */
					uxParameterNumber,
					/* Store the parameter string length. */
					&xParameterStringLength);

		/* Sanity check something was returned. */
		configASSERT(pcParameter);
		command_string = malloc(128);
		if (command_string == NULL) {
			printf("malloc command_string failed.\n");
			uxParameterNumber = 0;
			for (i = 0; i < ARPAF_ARGC; i++) {
				if (arpaf_argv[i])
					free(arpaf_argv[i]);
				arpaf_argv[i] = NULL;
			}
			xReturn = pdFALSE;
			return xReturn;
		}
		memset(command_string, 0, 128);
		if (uxParameterNumber == 0) {
			strncpy(command_string, pcCommandString, 127);
			arpaf_argv[uxParameterNumber] = strtok(command_string, " ");
		} else {
			strncpy(command_string, pcParameter,
				(xParameterStringLength > 127)?
				127 : xParameterStringLength);
			arpaf_argv[uxParameterNumber] = command_string;
		}

		memset(pcWriteBuffer, 0x00, xWriteBufferLen);

		xReturn = pdPASS;

		if (++uxParameterNumber < ARPAF_ARGC) {
			free(command_string);
			return xReturn;
		}

		arpaf_simulator(ARPAF_ARGC, (char **)arpaf_argv);
		xReturn = pdFALSE;
		uxParameterNumber = 0;
		for (i = 0; i < ARPAF_ARGC; i++) {
			if (arpaf_argv[i])
				free(arpaf_argv[i]);
			arpaf_argv[i] = NULL;
		}
	}

	return xReturn;
}

static const CLI_Command_Definition_t cmd_arpaf_simulator = {
	"arpaf_simulator",
	ARPAF_SIMULATOR_COMMOND_HELP,
	cmd_arpaf_simulator_handler, ARPAF_ARGC - 1
};

void cmd_arpaf_simulator_register(void)
{
	FreeRTOS_CLIRegisterCommand(&cmd_arpaf_simulator);
}
#endif
