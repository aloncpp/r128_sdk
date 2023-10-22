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

#include <aw_rpaf/mixer.h>
#include <aw_rpaf/common.h>

extern struct arpaf_priv *arpaf_priv;

static struct snd_dsp_hal_mixer *arpaf_hal_mixer_list_malloc_add_tail(struct arpaf_priv *arpaf_priv,
			    struct snd_soc_dsp_mixer *soc_mixer,
				struct snd_dsp_hal_mixer_ops *hal_mixer_ops)
{
	QueueHandle_t dsp_semaph = NULL;
	struct list_head *dsp_list = NULL;
	struct snd_dsp_hal_mixer *hal_mixer = NULL;

	if (!soc_mixer) {
		awrpaf_err("\n");
		return NULL;
	}
	if (!arpaf_priv) {
		awrpaf_err("\n");
		return NULL;
	}
	dsp_semaph = arpaf_priv->hal_mixer_list_mutex;
	dsp_list = &arpaf_priv->list_hal_mixer;

	hal_mixer = rpaf_malloc(sizeof(struct snd_dsp_hal_mixer));
	if (!hal_mixer) {
		awrpaf_err("\n");
		return NULL;
	}
	memset(hal_mixer, 0, sizeof(struct snd_dsp_hal_mixer));

	hal_mixer->taskHandle = &arpaf_priv->MixerTaskHandle;
	hal_mixer->mixer_ops = hal_mixer_ops;
	hal_mixer->soc_mixer = rpaf_malloc(sizeof(struct snd_soc_dsp_mixer));
	if (!hal_mixer->soc_mixer) {
		awrpaf_err("\n");
		rpaf_free(hal_mixer);
		return NULL;
	}
	memcpy(hal_mixer->soc_mixer, soc_mixer, sizeof(struct snd_soc_dsp_mixer));

	arpaf_mutex_lock(dsp_semaph);
	list_add_tail(&hal_mixer->list, dsp_list);
	arpaf_mutex_unlock(dsp_semaph);

	return hal_mixer;
}

#if 0
static int arpaf_hal_mixer_list_get_head(struct arpaf_priv *arpaf_priv,
			    struct snd_dsp_hal_mixer **hal_mixer)
{
	struct snd_dsp_hal_mixer *c = NULL;
	QueueHandle_t dsp_semaph = NULL;
	struct list_head *dsp_list = NULL;

	if (!arpaf_priv) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	if (!hal_mixer) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	dsp_semaph = arpaf_priv->hal_mixer_list_mutex;
	dsp_list = &arpaf_priv->list_hal_mixer;

	arpaf_mutex_lock(dsp_semaph);
	if (list_empty(dsp_list)) {
		awrpaf_err("\n");
		*hal_mixer = NULL;
		arpaf_mutex_unlock(dsp_semaph);
		return -EFAULT;
	}

	c = list_first_entry(dsp_list, struct snd_dsp_hal_mixer, list);
	if (!c) {
		awrpaf_err("\n");
		*hal_mixer = NULL;
		arpaf_mutex_unlock(dsp_semaph);
		return -EFAULT;
	}
	*hal_mixer = c;
	arpaf_mutex_unlock(dsp_semaph);

	return 0;
}
#endif

static int arpaf_hal_mixer_list_remove_free(struct arpaf_priv *arpaf_priv,
			    struct snd_dsp_hal_mixer *hal_mixer)
{
	struct snd_dsp_hal_mixer *c = NULL;
	struct snd_dsp_hal_mixer *tmp = NULL;
	QueueHandle_t dsp_semaph = NULL;
	struct list_head *dsp_list = NULL;

	if (!arpaf_priv) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	if (!hal_mixer) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	dsp_semaph = arpaf_priv->hal_mixer_list_mutex;
	dsp_list = &arpaf_priv->list_hal_mixer;

	arpaf_mutex_lock(dsp_semaph);
	if (list_empty(dsp_list)) {
		awrpaf_err("\n");
		arpaf_mutex_unlock(dsp_semaph);
		return -EFAULT;
	}
	list_for_each_entry_safe(c, tmp, dsp_list, list) {
		if (c == hal_mixer) {
			list_del(&c->list);
			rpaf_free(c->soc_mixer);
			rpaf_free(c);
			arpaf_mutex_unlock(dsp_semaph);
			return 0;
		}
	}
	arpaf_mutex_unlock(dsp_semaph);
	return -EFAULT;
}

#if 0
static int arpaf_hal_mixer_list_delete(struct arpaf_priv *arpaf_priv)
{
	struct snd_dsp_hal_mixer *c = NULL;
	struct snd_dsp_hal_mixer *tmp = NULL;
	QueueHandle_t dsp_semaph = NULL;
	struct list_head *dsp_list = NULL;

	if (!arpaf_priv) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	if (!hal_mixer) {
		awrpaf_err("\n");
		return -EFAULT;
	}
	dsp_semaph = arpaf_priv->hal_mixer_list_mutex;
	dsp_list = &arpaf_priv->list_hal_mixer;

	arpaf_mutex_lock(dsp_semaph);
	if (list_empty(dsp_list)) {
		awrpaf_err("\n");
		arpaf_mutex_unlock(dsp_semaph);
		return -EFAULT;
	}

	list_for_each_entry_safe(c, tmp, dsp_list, list) {
		list_del(&c->list);
		rpaf_free(c->soc_mixer);
		rpaf_free(c);
	}

	list_del(&hal_mixer->list);
	list_del_init(dsp_list);
	arpaf_mutex_unlock(dsp_semaph);

	return 0;
}
#endif

struct snd_dsp_hal_mixer *snd_dsp_hal_mixer_get_from_list_by_name(struct arpaf_priv *arpaf_priv,
							const char *name)
{
	struct snd_dsp_hal_mixer *hal_mixer;
	QueueHandle_t dsp_semaph = NULL;
	struct list_head *dsp_list = NULL;

	if (!arpaf_priv) {
		awrpaf_err("arpaf_priv is null.\n");
		return NULL;
	}
//	if (!hal_mixer) {
//		awrpaf_err("hal_mixer is null.\n");
//		return NULL;
//	}
	dsp_semaph = arpaf_priv->hal_mixer_list_mutex;
	dsp_list = &arpaf_priv->list_hal_mixer;

	arpaf_mutex_lock(dsp_semaph);
	list_for_each_entry(hal_mixer, dsp_list, list) {
		if (!strcmp(hal_mixer->name, name)) {
			arpaf_mutex_unlock(dsp_semaph);
			return hal_mixer;
		}
	}
	arpaf_mutex_unlock(dsp_semaph);
	return NULL;
}

struct snd_dsp_hal_mixer *snd_dsp_hal_mixer_get_from_list_by_card_device(
	struct arpaf_priv *arpaf_priv, unsigned int card, unsigned int device)
{
	struct snd_dsp_hal_mixer *hal_mixer;
	QueueHandle_t dsp_semaph = NULL;
	struct list_head *dsp_list = NULL;

	if (!arpaf_priv) {
		awrpaf_err("arpaf_priv is null.\n");
		return NULL;
	}
	dsp_semaph = arpaf_priv->hal_mixer_list_mutex;
	dsp_list = &arpaf_priv->list_hal_mixer;

	arpaf_mutex_lock(dsp_semaph);
	list_for_each_entry(hal_mixer, dsp_list, list) {
		if (hal_mixer->soc_mixer &&
			(hal_mixer->soc_mixer->card == card) &&
			(hal_mixer->soc_mixer->device == device)) {
			arpaf_mutex_unlock(dsp_semaph);
			return hal_mixer;
		}
	}
	arpaf_mutex_unlock(dsp_semaph);

	return NULL;
}

int snd_dsp_hal_mixer_open(struct snd_dsp_hal_mixer *mixer)
{
	return 0;
}

int snd_dsp_hal_mixer_close(struct snd_dsp_hal_mixer *mixer)
{
	return 0;
}

int snd_dsp_hal_mixer_read(struct snd_dsp_hal_mixer *mixer)
{
	struct snd_soc_dsp_mixer *soc_mixer = mixer->soc_mixer;
	snd_ctl_info_t info;

	soc_mixer->ret_val = snd_ctl_get(soc_mixer->driver, soc_mixer->ctl_name, &info);
	if (soc_mixer->ret_val == 0)
		soc_mixer->value = info.value;
	awrpaf_debug("ctl_name:%s, value:%d\n", soc_mixer->ctl_name, soc_mixer->value);
	return soc_mixer->ret_val;
}

int snd_dsp_hal_mixer_write(struct snd_dsp_hal_mixer *mixer)
{
	struct snd_soc_dsp_mixer *soc_mixer = mixer->soc_mixer;
	snd_ctl_info_t info;

	info.value = soc_mixer->value;
	awrpaf_debug("ctl_name:%s, value:%d\n", soc_mixer->ctl_name, soc_mixer->value);
	soc_mixer->ret_val = snd_ctl_set(soc_mixer->driver, soc_mixer->ctl_name, info.value);
	return soc_mixer->ret_val;
}

struct snd_dsp_hal_mixer_ops hal_mixer_ops = {
	.open = (int (*)(void *mixer))snd_dsp_hal_mixer_open,
	.close = (int (*)(void *mixer))snd_dsp_hal_mixer_close,
	.read = (int (*)(void *mixer))snd_dsp_hal_mixer_read,
	.write = (int (*)(void *mixer))snd_dsp_hal_mixer_write,
};

static void pxAudioMixerTask(void *arg)
{
	portBASE_TYPE xStatus;
	/*const portTickType xTicksToWait = 10 * 1000 / portTICK_RATE_MS;*/
	const portTickType xTicksToWait = portMAX_DELAY;
	struct snd_dsp_hal_queue_item pvAudioCmdItem;
	struct snd_dsp_hal_mixer *hal_mixer = NULL;
	struct snd_soc_dsp_mixer *soc_mixer = NULL;
	struct snd_dsp_hal_mixer_ops *mixer_ops = NULL;
	int ret = 0;

	for (;;) {
		/* 从AudioServerSend接收 */
		arpaf_priv->handle_bit |= 0x1 << HANDLE_TYPE_SOC_MIXER;
		if ((ret = uxQueueMessagesWaiting(arpaf_priv->MixerReceQueue)) != 0) {
			awrpaf_print("QM Waiting: %d!\n", ret);
		}
		xStatus = xQueueReceive(arpaf_priv->MixerReceQueue, &pvAudioCmdItem, xTicksToWait);
		if (xStatus != pdPASS) {
			awrpaf_err("Receive failed!\n");
			goto err_mixer_queue_receive;
		}
		snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_MIXER | 0x001);
		switch (pvAudioCmdItem.pxAudioMsgVal) {
		case MSGBOX_SOC_DSP_AUDIO_MIXER_COMMAND:
			soc_mixer = pvAudioCmdItem.soc_mixer;
			/* 找到对应的声卡的mixer即可 */
			hal_mixer = snd_dsp_hal_mixer_get_from_list_by_card_device(arpaf_priv,
							soc_mixer->card, soc_mixer->device);
			if (!hal_mixer) {
				awrpaf_print("cannot find from list.\n");
				hal_mixer = arpaf_hal_mixer_list_malloc_add_tail(arpaf_priv,
							soc_mixer, &hal_mixer_ops);
				if (hal_mixer == NULL) {
					awrpaf_err("\n");
					goto err_add_queue;
				}
			} else {
				/* 更新内容 */
				memcpy(hal_mixer->soc_mixer, soc_mixer,
					sizeof(struct snd_soc_dsp_mixer));

			}
			mixer_ops = hal_mixer->mixer_ops;
			break;
		default:
			awrpaf_err("pxAudioMsgVal:%u\n", pvAudioCmdItem.pxAudioMsgVal);
			goto err_mixer_msgval;
		}

		/* for amixer to setting audio routes */
		switch (hal_mixer->soc_mixer->cmd_val) {
		case SND_SOC_DSP_MIXER_OPEN:
			snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_MIXER | 0x002);
			ret = mixer_ops->open(hal_mixer);
			break;
		case SND_SOC_DSP_MIXER_READ:
			snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_MIXER | 0x003);
			ret = mixer_ops->read(hal_mixer);
			break;
		case SND_SOC_DSP_MIXER_WRITE:
			snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_MIXER | 0x004);
			ret = mixer_ops->write(hal_mixer);
			break;
		case SND_SOC_DSP_MIXER_CLOSE:
			snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_MIXER | 0x005);
			ret = mixer_ops->close(hal_mixer);
			arpaf_hal_mixer_list_remove_free(arpaf_priv, hal_mixer);
			break;
		default:
			awrpaf_err("pxAudioCmdVal:%u is error!\n",
					hal_mixer->soc_mixer->cmd_val);
			goto err_mixer_cmdval;
		}
		memcpy(pvAudioCmdItem.soc_mixer, hal_mixer->soc_mixer, sizeof(struct snd_soc_dsp_mixer));
		pvAudioCmdItem.soc_mixer->used = 1;
		pvAudioCmdItem.soc_mixer->ret_val = ret;
		pvAudioCmdItem.ret_val = 0;
		xStatus = xQueueSendToBack(arpaf_priv->ServerReceQueue, &pvAudioCmdItem, pdMS_TO_TICKS(100));
		if (xStatus != pdPASS) {
			awrpaf_err("\n");
		}
		snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_MIXER | 0x006);
		continue;
err_add_queue:
err_mixer_queue_receive:
err_mixer_msgval:
err_mixer_cmdval:
		pvAudioCmdItem.ret_val = -EFAULT;
		/* 发送给AudioServerRece */
		xStatus = xQueueSendToBack(arpaf_priv->ServerReceQueue, &pvAudioCmdItem, 0);
		if (xStatus != pdPASS) {
			awrpaf_err("\n");
		}
		/* 立即让出剩余时间片 */
//		taskYIELD();
	}

//	vTaskDelete(NULL);
}

int snd_dsp_hal_mixer_process(void *argv)
{
	struct snd_dsp_hal_queue_item *pAudioCmdItem = argv;
	BaseType_t xStatus;
	int ret = 0;

	snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_MIXER | 0x000);
	/* 查找当前是否有正在运行的mixer任务 */
	if (arpaf_priv->MixerTaskHandle == NULL) {
		xTaskCreate(pxAudioMixerTask, "AudioMixer", 512, arpaf_priv,
			configAPPLICATION_NORMAL_PRIORITY, &arpaf_priv->MixerTaskHandle);
		/* need waiting AudioMixer Task running */
		while (!((arpaf_priv->handle_bit >> HANDLE_TYPE_SOC_MIXER) & 0x1))
			vTaskDelay(pdMS_TO_TICKS(20));
	}

	switch (pAudioCmdItem->soc_mixer->cmd_val) {
	case SND_SOC_DSP_MIXER_OPEN:
	/* 读写声卡的mixer数值 */
	case SND_SOC_DSP_MIXER_WRITE:
	case SND_SOC_DSP_MIXER_READ:
	/* 关闭声卡 */
	case SND_SOC_DSP_MIXER_CLOSE:
		xStatus = xQueueSendToBack(arpaf_priv->MixerReceQueue, pAudioCmdItem, 0);
		if (xStatus != pdPASS) {
			ret  = -EBUSY;
			awrpaf_err("\n");
		} 
	break;
	}
	snd_dsp_save_flag(REC_AUDIO | REC_AUDIO_MIXER | 0x007);
	return ret;
}
