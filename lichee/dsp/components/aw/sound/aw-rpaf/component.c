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

extern struct arpaf_priv *arpaf_priv;

static struct snd_dsp_hal_component * arpaf_hal_component_list_malloc_add_tail(
			struct arpaf_priv *arpaf_priv,
			struct snd_soc_dsp_component *soc_component)
{
	struct snd_dsp_hal_component *hal_component = NULL;
	struct snd_soc_dsp_native_component *native_component = NULL;
	QueueHandle_t dsp_semaph = NULL;
	struct list_head *dsp_list = NULL;

	if (!soc_component) {
		awrpaf_err("\n");
		return NULL;
	}
	if (!arpaf_priv) {
		awrpaf_err("\n");
		return NULL;
	}
	dsp_semaph = arpaf_priv->hal_component_list_mutex;
	dsp_list = &arpaf_priv->list_hal_component;

	hal_component = rpaf_malloc(sizeof(struct snd_dsp_hal_component));
	if (!hal_component) {
		awrpaf_err("\n");
		return NULL;
	}
	memset(hal_component, 0, sizeof(struct snd_dsp_hal_component));

	native_component = &hal_component->native_component;
	memcpy(&native_component->soc_component, soc_component,
			sizeof(struct snd_soc_dsp_component));

	hal_component->xTaskCreateEvent = xEventGroupCreate();
	if (!hal_component->xTaskCreateEvent) {
		awrpaf_err("\n");
		rpaf_free(hal_component);
		return NULL;
	}
	arpaf_mutex_lock(dsp_semaph);
	list_add_tail(&hal_component->list, dsp_list);
	arpaf_mutex_unlock(dsp_semaph);

	return hal_component;
}

struct snd_dsp_hal_component *snd_dsp_hal_component_get_from_list_by_soc_component_id(
	struct arpaf_priv *arpaf_priv, struct snd_soc_dsp_component *soc_component)
{
	struct snd_dsp_hal_component *hal_component;
	struct snd_soc_dsp_native_component *native_component;
	QueueHandle_t dsp_semaph = NULL;
	struct list_head *dsp_list = NULL;

	if (!arpaf_priv) {
		awrpaf_err("\n");
		return NULL;
	}
	dsp_semaph = arpaf_priv->hal_component_list_mutex;
	dsp_list = &arpaf_priv->list_hal_component;

	arpaf_mutex_lock(dsp_semaph);
	list_for_each_entry(hal_component, dsp_list, list) {
		native_component = &hal_component->native_component;
		if (native_component->soc_component.id == soc_component->id) {
			arpaf_mutex_unlock(dsp_semaph);
			return hal_component;
		}
	}
	arpaf_mutex_unlock(dsp_semaph);

	return NULL;
}

struct snd_dsp_hal_substream *snd_dsp_hal_substream_get_from_list_by_soc_component(
	struct arpaf_priv *arpaf_priv, struct snd_soc_dsp_component *soc_component)
{
	struct snd_dsp_hal_substream *hal_substream = NULL;
	struct snd_soc_dsp_substream *soc_substream = NULL;
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
		soc_substream = hal_substream->soc_substream;
		if ((soc_substream->params.card == soc_component->params.card) &&
			(soc_substream->params.device == soc_component->params.device) &&
			(soc_substream->params.stream == soc_component->params.stream)) {
			arpaf_mutex_unlock(dsp_semaph);
			return hal_substream;
		}
	}
	arpaf_mutex_unlock(dsp_semaph);

	return NULL;
}

struct snd_dsp_hal_component_process_driver *snd_dsp_component_process[SND_DSP_COMPONENT_MAX + 1] = {0};

static int32_t snd_dsp_hal_component_find_config(uint32_t component_type,
				struct snd_dsp_component_algo_config **config)
{
	if ((component_type > SND_DSP_COMPONENT_MAX) ||
		(!snd_dsp_component_process[component_type])) {
		awrpaf_err("algo component type%d isn't installed.\n", component_type);
		return -1;
	}
	*config = &snd_dsp_component_process[component_type]->config;
	return 0;
}

void snd_dsp_hal_component_algo_hooks(const char *card_name, snd_pcm_stream_t stream,
				struct snd_soc_dsp_native_component *native_component)
{
	int32_t i;
	struct snd_dsp_component_algo_config *config;
	struct snd_soc_dsp_component *soc_component;

	soc_component = &native_component->soc_component;
	for (i = 0; i < ARRAY_SIZE(snd_dsp_component_process); i++) {
		if (!snd_dsp_component_process[i])
			continue;
		config = &snd_dsp_component_process[i]->config;
		if (!strcmp(config->card_name, card_name) &&
			config->stream == stream &&
			config->enable == 1) {
			awrpaf_info("find %s enabled algo, type:%d\n", card_name, i);
			soc_component->component_type |= (1 << i);
			soc_component->component_sort[config->sort_index] = i;
		}
	}
}

int32_t snd_dsp_hal_component_install(struct snd_dsp_hal_component_process_driver *algo_comp,
				enum SND_DSP_COMPONENT_TYPE type)
{
	if (snd_dsp_component_process[type] != NULL) {
		awrpaf_err("algo component type%d already install.\n", type);
		return -1;
	}
	awrpaf_debug("algo component type%d install.\n", type);
	snd_dsp_component_process[type] = algo_comp;
	return 0;
}

uint32_t snd_dsp_algorithmic_create(struct snd_soc_dsp_native_component *native_component)
{
	struct snd_soc_dsp_component *soc_component = NULL;
	uint32_t component_type = 0;
	int32_t index = 0;
	int32_t i = 0;

	if (!native_component) {
		awrpaf_debug("\n");
		return 0;
	}

	soc_component = &native_component->soc_component;
	component_type = soc_component->component_type;

	for (i = 0; i < RPAF_COMPONENT_MAX_NUM; i++) {
		/* 根据应用层设置算法处理顺序执行代码，以按照bit位的方式进行处理 */
		index = soc_component->component_sort[i];
		if ((index < 0) || (index >= RPAF_COMPONENT_MAX_NUM))
			break;
		awrpaf_debug("component_type:0x%x, index:%d\n", component_type, index);
		if ((component_type >> index) & 0x1) {
			if (snd_dsp_component_process[index]->create) {
				soc_component->status[index] =
					snd_dsp_component_process[index]->create(
						&(native_component->handle[index]),
						native_component);
			} else {
				soc_component->status[index] = -EFAULT;
			}
		}
	}

	return 0;
}

uint32_t snd_dsp_algorithmic_release(struct snd_soc_dsp_native_component *native_component)
{
	struct snd_soc_dsp_component *soc_component = NULL;
	uint32_t component_type = 0;
	uint32_t index = 0;
	int32_t i = 0;

	if (!native_component)
		return 0;

	soc_component = &native_component->soc_component;
	component_type = soc_component->component_type;

	for (i = 0; i < RPAF_COMPONENT_MAX_NUM; i++) {
		/* 根据应用层设置算法处理顺序执行代码，以查找表的方式进行处理 */
		index = soc_component->component_sort[i];
		if ((index < 0) || (index >= RPAF_COMPONENT_MAX_NUM))
			break;
		if ((component_type >> index) & 0x1) {
			if (snd_dsp_component_process[index]->release) {
				snd_dsp_component_process[index]->release(
					native_component->handle[index]);
				native_component->handle[index] = NULL;
			} else
				soc_component->status[index] = -EFAULT;
		}
	}

	return 0;
}

int32_t snd_dsp_algorithmic_process(struct snd_soc_dsp_native_component *native_component,
			void *input_buffer, uint32_t *input_size,
			void *output_buffer, uint32_t *output_size)
{
	struct snd_soc_dsp_component *soc_component = NULL;
	uint32_t component_type = 0;
	uint32_t index = 0;
	void *output_addr = output_buffer;
	uint32_t *output_length = output_size;
	int32_t i = 0;

	if (!native_component || !input_buffer || !output_buffer) {
		awrpaf_err("\n");
		return 0;
	}

	soc_component = &native_component->soc_component;
	component_type = soc_component->component_type;
	if (component_type == 0) {
		/* 拷贝到目的地址 */
		memcpy(output_buffer, input_buffer, *input_size);
		*output_size = *input_size;
		return 0;
	}

	for (i = 0; i < RPAF_COMPONENT_MAX_NUM; i++) {
		/* 根据应用层设置算法处理顺序执行代码，以查找表的方式进行处理 */
		index = soc_component->component_sort[i];
		if ((index < 0) || (index >= RPAF_COMPONENT_MAX_NUM))
			break;
		/* 找到对应的算法实现API */
		if ((component_type >> index) & 0x1) {
			if (snd_dsp_component_process[index]->process) {
				void *temp_buffer = NULL;
				uint32_t *temp_size = NULL;
				soc_component->status[index] =
					snd_dsp_component_process[index]->process(
						native_component->handle[index],
						input_buffer, input_size,
						output_buffer, output_size);
				/* 比较耗内存，dump内存全部放在dram中 */
				if ((!native_component->soc_suspended) &&
					(native_component->dump_start[index] &&
					(soc_component->dump_addr[index]))) {
					memcpy((void *)soc_component->dump_addr[index],
						output_buffer, *output_size);
					soc_component->dump_length[index] = *output_size;
				}
				/* 省去一次拷贝 */
				temp_buffer = input_buffer;
				temp_size = input_size;
				input_buffer = output_buffer;
				input_size = output_size;
				output_buffer = temp_buffer;
				output_size = temp_size;
			} else
				soc_component->status[index] = -EFAULT;
		}
	}
	/* copy to input addr because the last output is the input of the next */
	memcpy(output_addr, input_buffer, *input_size);
	*output_length = *input_size;
	return 0;
}

static int32_t snd_dsp_algorithmic_dump_start(
	struct snd_soc_dsp_native_component *native_component,
	struct snd_soc_dsp_component *soc_component)
{
	if (soc_component->transfer_type < RPAF_COMPONENT_MAX_NUM) {
		native_component->dump_start[soc_component->transfer_type] = 1;
	}

	return 0;
}

static int32_t snd_dsp_algorithmic_dump_stop(
	struct snd_soc_dsp_native_component *native_component,
	struct snd_soc_dsp_component *soc_component)
{
	if (soc_component->transfer_type < RPAF_COMPONENT_MAX_NUM) {
		native_component->dump_start[soc_component->transfer_type] = 0;
	}

	return 0;
}

int32_t snd_dsp_algorithmic_start(struct snd_soc_dsp_native_component *native_component)
{
	struct snd_soc_dsp_component *soc_component = &(native_component->soc_component);
	int32_t i = 0;
	int32_t index = 0;

	for (i = 0; i < RPAF_COMPONENT_MAX_NUM; i++) {
		index = soc_component->component_sort[i];
		if ((index < 0) || (index >= RPAF_COMPONENT_MAX_NUM))
			break;
		native_component->dump_start[index] = 1;
	}

	return 0;
}

int32_t snd_dsp_algorithmic_stop(struct snd_soc_dsp_native_component *native_component)
{
	struct snd_soc_dsp_component *soc_component = &(native_component->soc_component);
	int32_t i = 0;
	int32_t index = 0;

	for (i = 0; i < RPAF_COMPONENT_MAX_NUM; i++) {
		index = soc_component->component_sort[i];
		if ((index < 0) || (index >= RPAF_COMPONENT_MAX_NUM))
			break;
		native_component->dump_start[index] = 0;
	}

	return 0;
}

/*实现初始化，包括创建独立音频处理任务或者配置初始化substream的组件 */
int32_t snd_dsp_hal_component_create(struct snd_dsp_hal_component *component)
{
	/* 初始化各个算法组件 */
	snd_dsp_algorithmic_create(&component->native_component);
	return 0;
}

/* 去初始化，释放资源 */
int32_t snd_dsp_hal_component_remove(struct snd_dsp_hal_component *component)
{
	/* 删除组件流的各个算法处理资源 */
	snd_dsp_algorithmic_release(&component->native_component);
	return 0;
}

/* 告知Linux音频组件 */
int32_t snd_dsp_hal_component_suspend(struct snd_dsp_hal_component *component)
{
	struct snd_soc_dsp_native_component *native_component;

	awrpaf_debug("\n");
	native_component = &component->native_component;

	native_component->soc_suspended = 1;
	native_component->soc_resumed = 0;
	return 0;
}

int32_t snd_dsp_hal_component_resume(struct snd_dsp_hal_component *component)
{
	struct snd_soc_dsp_native_component *native_component;

	awrpaf_debug("\n");
	native_component = &component->native_component;

	native_component->soc_suspended = 0;
	native_component->soc_resumed = 1;
	return 0;
}

/* reserved. */
int32_t snd_dsp_hal_component_status(struct snd_dsp_hal_component *component)
{
	struct snd_soc_dsp_native_component *native_component = &component->native_component;
	struct snd_soc_dsp_component *soc_component = &(native_component->soc_component);
	int32_t i = 0;

	awrpaf_debug("\n");
	/* 不需要单独更新，在运算处理过程中已经实时更新到结构体中 */
	for (i = 0; i < SND_DSP_COMPONENT_MAX; i++) {
		if ((soc_component->component_type >> i) & 0x1) {
			awrpaf_info("algo_type[%d]: status:%d\n", i, soc_component->status[i]);
			awrpaf_info("card: %d\n", soc_component->params.card);
			awrpaf_info("device: %d\n", soc_component->params.device);
			awrpaf_info("driver: %s\n", soc_component->params.driver);
			awrpaf_info("stream: %d\n", soc_component->params.stream);
			awrpaf_info("format: %d\n", soc_component->params.format);
			awrpaf_info("rate: %u\n", soc_component->params.rate);
			awrpaf_info("channels: %u\n", soc_component->params.channels);
			awrpaf_info("resample_rate: %u\n", soc_component->params.resample_rate);
			awrpaf_info("period_size: %u\n", soc_component->params.period_size);
			awrpaf_info("periods: %u\n", soc_component->params.periods);
			awrpaf_info("buffer_size: %u\n", soc_component->params.buffer_size);
			awrpaf_info("data_type: %d\n", soc_component->params.data_type);
			awrpaf_info("codec_type: %d\n", soc_component->params.codec_type);
			awrpaf_info("dts_data: 0x%x\n", soc_component->params.dts_data);
			awrpaf_info("status: %d\n", soc_component->params.status);
			awrpaf_info("hw_stream: %u\n", soc_component->params.hw_stream);
			awrpaf_info("data_mode: %u\n", soc_component->params.data_mode);
			awrpaf_info("stream_wake: %u\n", soc_component->params.stream_wake);
		}
	}
	return 0;
}

int32_t snd_dsp_hal_component_sw_params(struct snd_dsp_hal_component *component)
{
	/* 设置算法组件 */
//	struct snd_soc_dsp_native_component *native_component = &component->native_component;
//	struct snd_soc_dsp_component *soc_component = &(native_component->soc_component);
//	struct snd_soc_dsp_pcm_params *pcm_params = &(soc_component->params);

	awrpaf_debug("\n");
	/* 后续如果各个小部分需要分配参数，可移此处处理配置 */

	return 0;
}

/*
 * 用于流组件的dump的启用和关闭，需配合substream或和component_write使用，
 * 以供dump数据可调用
 */
int32_t snd_dsp_hal_component_start(struct snd_dsp_hal_component *component)
{
	struct snd_soc_dsp_native_component *native_component = &component->native_component;
	struct snd_soc_dsp_component *soc_component = &(native_component->soc_component);
	int32_t i = 0;
	int32_t index = 0;

//	awrpaf_debug("\n");
	for (i = 0; i < RPAF_COMPONENT_MAX_NUM; i++) {
		index = soc_component->component_sort[i];
		if ((index < 0) || (index >= RPAF_COMPONENT_MAX_NUM))
			break;
		native_component->dump_start[index] = 1;
	}

	return 0;
}

int32_t snd_dsp_hal_component_stop(struct snd_dsp_hal_component *component)
{
	struct snd_soc_dsp_native_component *native_component = &component->native_component;
	struct snd_soc_dsp_component *soc_component = &(native_component->soc_component);
	int32_t i = 0;
	int32_t index = 0;

//	awrpaf_debug("\n");
	for (i = 0; i < RPAF_COMPONENT_MAX_NUM; i++) {
		index = soc_component->component_sort[i];
		if ((index < 0) || (index >= RPAF_COMPONENT_MAX_NUM))
			break;
		native_component->dump_start[index] = 0;
	}

	return 0;
}

/* 主要用于写入数据，保留 */
int32_t snd_dsp_hal_component_write(struct snd_dsp_hal_component *component)
{
	struct snd_soc_dsp_native_component *native_component = &component->native_component;
	struct snd_soc_dsp_component *soc_component = &native_component->soc_component;

//	awrpaf_debug("\n");
	snd_dsp_algorithmic_process(native_component,
			(void *)soc_component->write_addr, &(soc_component->write_length),
			(void *)soc_component->read_addr, &(soc_component->read_length));
	awrpaf_debug("write_buffer:0x%x, size;%u, output_buffer:0x%x, size:%u\n",
				soc_component->write_addr, soc_component->write_length,
				soc_component->read_addr, soc_component->read_length);

	return 0;
}

/* 这个接口可能废弃:主要用于dump 每个组件之后经过处理的数据，
 * 处理之后的数据已全部放在dump_addr的sdram地址里了 */
int32_t snd_dsp_hal_component_read(struct snd_dsp_hal_component *component)
{
#if 0
	struct snd_soc_dsp_native_component *native_component = &component->native_component;
	struct snd_soc_dsp_component *soc_component = &native_component->soc_component;
	struct snd_soc_dsp_sdram_buf *sdram_buf = NULL;

	/* sdram address */
	/* 获得队列首的数据 */
	sdram_buf = snd_dsp_list_sdram_buf_get_top_item(native_component->list_sdram_buf);
	if (sdram_buf) {
		snd_dsp_list_sdram_buf_remove_item(native_component->list_sdram_buf, sdram_buf);
		memcpy(soc_component->read_buf, sdram_buf->buf_addr, soc_component->buf_length);
	} else {
		native_component->ret_val = -EFAULT;
		return native_component->ret_val;
	}
#endif
//	awrpaf_debug("\n");
	return 0;
}

struct snd_dsp_hal_component_ops hal_component_ops = {
	/*实现初始化，包括创建独立音频处理任务或者配置初始化substream的组件 */
	.create = (int32_t (*)(void *))snd_dsp_hal_component_create,
	/* 去初始化，包括删除独立音频处理任务 */
	.remove = (int32_t (*)(void *))snd_dsp_hal_component_remove,
	/* 告知Linux音频组件 */
	.suspend = (int32_t (*)(void *))snd_dsp_hal_component_suspend,
	.resume = (int32_t (*)(void *))snd_dsp_hal_component_resume,

	.status = (int32_t (*)(void *))snd_dsp_hal_component_status,
	.sw_params = (int32_t (*)(void *))snd_dsp_hal_component_sw_params,

	/*
	 * 用于某个流的组件的启用和关闭，
	 * dump数据可调用，内部需要引用计数
	 */
	.start = (int32_t (*)(void *))snd_dsp_hal_component_start,
	.stop = (int32_t (*)(void *))snd_dsp_hal_component_stop,
	/* 主要用于播放经过编码的数据，保留 */
	.write = (int32_t (*)(void *))snd_dsp_hal_component_write,
	/* 主要用于dump 每个组件之后经过处理的数据 */
	.read = (int32_t (*)(void *))snd_dsp_hal_component_read,
};

void pxAudioCompTask(void *arg)
{
	struct snd_dsp_hal_component *hal_component = (struct snd_dsp_hal_component *)arg;
	struct snd_soc_dsp_native_component *native_component = &hal_component->native_component;
	portBASE_TYPE xStatus;
	struct snd_dsp_hal_queue_item pvAudioCmdItem;
	struct snd_soc_dsp_component *soc_component;

	hal_component->handle_bit = 0x1 << hal_component->id;

	xEventGroupSetBits(hal_component->xTaskCreateEvent, hal_component->handle_bit);

	for (;;) {
		xStatus = xQueueReceive(hal_component->cmdQueue, &pvAudioCmdItem,
					portMAX_DELAY);
		switch (pvAudioCmdItem.pxAudioMsgVal) {
		case MSGBOX_SOC_DSP_AUDIO_COMPONENT_COMMAND:
			break;
		default:
			awrpaf_err("pxAudioMsgVal:%u\n", pvAudioCmdItem.pxAudioMsgVal);
			goto err_compstream_msgval;
		}
		soc_component = pvAudioCmdItem.soc_component;
		awrpaf_debug("\n");

		switch (native_component->soc_component.cmd_val) {
		case SND_SOC_DSP_COMPONENT_CREATE:
			hal_component->handle_bit |= 0x1 << hal_component->id;
			hal_component->ops.create(hal_component);
			break;
		case SND_SOC_DSP_COMPONENT_SW_PARAMS:
			/* 增加其它函数的初始化等 */
			hal_component->ops.sw_params(hal_component);
			break;
		case SND_SOC_DSP_COMPONENT_SUSPEND:
			hal_component->ops.suspend(hal_component);
			break;
		case SND_SOC_DSP_COMPONENT_RESUME:
			hal_component->ops.resume(hal_component);
			break;
		case SND_SOC_DSP_COMPONENT_WRITE:
			/* 往组件流里写入数据 */
			hal_component->ops.write(hal_component);
			soc_component->write_length = native_component->soc_component.write_length;
			soc_component->read_length = native_component->soc_component.read_length;
			memcpy(soc_component->dump_length,
				native_component->soc_component.dump_length,
				sizeof(soc_component->dump_length));
			awrpaf_debug("write_buffer:0x%x, size;%u, output_buffer:0x%x, size:%u\n",
				soc_component->write_addr, soc_component->write_length,
				soc_component->read_addr, soc_component->read_length);
			break;
		case SND_SOC_DSP_COMPONENT_READ:
			/* 从某个流里的组件里读取数据 */
			hal_component->ops.read(hal_component);
			break;
		case SND_SOC_DSP_COMPONENT_START:
			hal_component->ops.start(hal_component);
			break;
		case SND_SOC_DSP_COMPONENT_STOP:
			hal_component->ops.stop(hal_component);
			break;
		case SND_SOC_DSP_COMPONENT_REMOVE:
			hal_component->ops.remove(hal_component);
			hal_component->handle_bit &= ~(0x1 << hal_component->id);
//			snd_dsp_hal_component_remove_item(hal_component);
			break;
		default:
			break;
		}
err_compstream_msgval:
		xStatus = xQueueSendToBack(arpaf_priv->ServerReceQueue, &pvAudioCmdItem, 0);
		if (xStatus != pdPASS)
			awrpaf_err("\n");
	}
}

int32_t snd_dsp_hal_component_independence_process(struct snd_dsp_hal_queue_item *pAudioCmdItem)
{
	uint32_t uxQueueLength = 10;
	uint32_t uxItemSize = sizeof(struct snd_dsp_hal_queue_item);
	struct snd_dsp_hal_component *hal_component = NULL;
	struct snd_soc_dsp_component *soc_component = pAudioCmdItem->soc_component;
	struct snd_soc_dsp_native_component *native_component = NULL;
	portBASE_TYPE xStatus;
	uint32_t i = 0;
	static uint32_t component_handle_bit = 0;

	/* 根据soc_component成员变量id比较 */
	hal_component = snd_dsp_hal_component_get_from_list_by_soc_component_id(
					arpaf_priv, soc_component);
	if (hal_component == NULL) {
		/* 创建一个新的对象 */
		hal_component = arpaf_hal_component_list_malloc_add_tail(
						arpaf_priv, soc_component);
		if (hal_component == NULL) {
			pAudioCmdItem->ret_val = -EFAULT;
			/* audio信号收发队列 */
			xStatus = xQueueSendToBack(arpaf_priv->ServerReceQueue,
					pAudioCmdItem, pdMS_TO_TICKS(20));
			if (xStatus != pdPASS)
				awrpaf_err("\n");
			return -EFAULT;
		}
		/* 注册回调 */
		hal_component->ops = hal_component_ops;
	}
	native_component = &hal_component->native_component;
	/* 参数更新 */
	memcpy(&native_component->soc_component, soc_component,
			sizeof(struct snd_soc_dsp_component));

	/* 给任务分发任务指令 */
	switch (soc_component->cmd_val) {
	case SND_SOC_DSP_COMPONENT_CREATE:
		/* 创建这个组件流任务 */
		if (hal_component->taskHandle == NULL) {
			for (i = 0; i < 32; i++) {
				if (!(component_handle_bit >> i) & 0x1)
					break;
			}
			hal_component->id = i;
			hal_component->cmdQueue = xQueueCreate(uxQueueLength, uxItemSize);
			memset(hal_component->taskName, 0, configMAX_TASK_NAME_LEN);
			snprintf(hal_component->taskName, configMAX_TASK_NAME_LEN - 1,
					"Comp%d", hal_component->id);
			xTaskCreate(pxAudioCompTask, hal_component->taskName,
					1024, hal_component,
					configAPPLICATION_AUDIO_PRIORITY,
					&hal_component->taskHandle);
			xEventGroupWaitBits(hal_component->xTaskCreateEvent,
					(0x1 << hal_component->id),
					pdTRUE, pdFALSE, portMAX_DELAY);
			component_handle_bit |= 0x1 << hal_component->id;
			awrpaf_info("CompTask:%s, Handle:%p, soc_component:%p\n",
				hal_component->taskName, hal_component->taskHandle, soc_component);
		}
	case SND_SOC_DSP_COMPONENT_SUSPEND:
	case SND_SOC_DSP_COMPONENT_RESUME:
	case SND_SOC_DSP_COMPONENT_STATUS:
	case SND_SOC_DSP_COMPONENT_SW_PARAMS:
	case SND_SOC_DSP_COMPONENT_WRITE:
	case SND_SOC_DSP_COMPONENT_READ:
	case SND_SOC_DSP_COMPONENT_START:
	case SND_SOC_DSP_COMPONENT_STOP:
	/* 删除组件任务流(暂时不删除) */
	case SND_SOC_DSP_COMPONENT_REMOVE:
		awrpaf_debug("\n");
		xStatus = xQueueSendToBack(hal_component->cmdQueue, pAudioCmdItem, 0);
		if (xStatus != pdPASS)
			awrpaf_err("\n");
		break;
	default:
		awrpaf_debug("\n");
		xStatus = xQueueSendToBack(arpaf_priv->ServerReceQueue, pAudioCmdItem, 0);
		if (xStatus != pdPASS)
			awrpaf_err("\n");
		break;
	}
	awrpaf_debug("\n");

	return 0;
}

/* 用于组件的处理 */
int32_t snd_dsp_hal_component_process(void *argv)
{
	struct snd_dsp_hal_queue_item *pAudioCmdItem = argv;
	struct snd_dsp_hal_substream *hal_substream = NULL;
	struct snd_soc_dsp_component *soc_component = pAudioCmdItem->soc_component;
	struct snd_soc_dsp_native_component *native_component = NULL;
	struct snd_dsp_component_algo_config *config = NULL;
	portBASE_TYPE xStatus;
	int32_t ret = 0;

	/* 独立的任务处理 */
	if (soc_component->comp_mode == SND_DSP_COMPONENT_MODE_INDEPENDENCE) {
		ret = snd_dsp_hal_component_independence_process(pAudioCmdItem);
		return ret;
	} else if (soc_component->comp_mode == SND_DSP_COMPONENT_MODE_ALGO) {
		switch (soc_component->cmd_val) {
		case SND_SOC_DSP_COMPONENT_ALGO_GET:
			ret = snd_dsp_hal_component_find_config(soc_component->component_type, &config);
			if (ret != 0) {
				soc_component->ret_val = ret;
				break;
			}
			strcpy(soc_component->params.driver, config->card_name);
			soc_component->params.stream = config->stream;
			soc_component->used = config->enable;
			soc_component->sort_index = config->sort_index;
			awrpaf_debug("component_type:%d, card:%s, stream:%d, sort_index:%d, enable:%d\n",
				soc_component->component_type,
				soc_component->params.driver,
				soc_component->params.stream,
				soc_component->sort_index,
				soc_component->used);
			soc_component->ret_val = 0;
			break;
		case SND_SOC_DSP_COMPONENT_ALGO_SET:
			ret = snd_dsp_hal_component_find_config(soc_component->component_type, &config);
			if (ret != 0) {
				soc_component->ret_val = ret;
				break;
			}
			/* 正常流程：ALGO_SET配置参数后，再打开声卡(会根据这些参数设置component_type) */
			strcpy(config->card_name, soc_component->params.driver);
			config->stream = soc_component->params.stream;
			config->sort_index = soc_component->sort_index;
			config->enable = soc_component->used;
			awrpaf_debug("component_type:%d, card:%s, stream:%d, sort_index:%d, enable:%d\n",
				soc_component->component_type,
				config->card_name, config->stream,
				config->sort_index, config->enable);
			soc_component->ret_val = 0;
			break;
		default:
			break;
		}
		xStatus = xQueueSendToBack(arpaf_priv->ServerReceQueue, pAudioCmdItem, 0);
		if (xStatus != pdPASS)
			awrpaf_err("\n");
	} else if (soc_component->comp_mode == SND_DSP_COMPONENT_MODE_STREAM) {
		struct snd_soc_dsp_substream *soc_substream;
		struct snd_soc_dsp_pcm_params *pcm_params;
		/*
		 * 与substream建立关联的操作:
		 * 需要先打开snd_pcm_open 然后是snd_component_open
		 * 根据pcm_params找到正在运行的音频流
		 */
		hal_substream = snd_dsp_hal_substream_get_from_list_by_soc_component(
					arpaf_priv, soc_component);
		if (hal_substream == NULL) {
			ret = -EFAULT;
			awrpaf_err("\n");
			pAudioCmdItem->soc_component->ret_val = 1;
			goto err_get_hal_substream;
		}
		soc_substream = hal_substream->soc_substream;
		pcm_params = &soc_substream->params;
		native_component = &hal_substream->native_component;

		/* 给任务分发任务指令 */
		switch (soc_component->cmd_val) {
		case SND_SOC_DSP_COMPONENT_SET_STREAM_PARAMS:
			/* 分配好了buffer地址，需要给到substream的成员对象里 */
			memcpy(&native_component->soc_component, soc_component,
				sizeof(struct snd_soc_dsp_component));
			/* 如果有算法绑定带该音频流，并且设置enable,则对应的设置音频流的component_type */
			snd_dsp_hal_component_algo_hooks(pcm_params->driver,
				pcm_params->stream, native_component);
			soc_component->ret_val = 0;
			break;
		case SND_SOC_DSP_COMPONENT_START:
			snd_dsp_algorithmic_dump_start(native_component, soc_component);
			soc_component->ret_val = 0;
			break;
		case SND_SOC_DSP_COMPONENT_STOP:
			snd_dsp_algorithmic_dump_stop(native_component, soc_component);
			soc_component->ret_val = 0;
			break;
		default:
			soc_component->ret_val = 0;
			break;
		}
err_get_hal_substream:
		xStatus = xQueueSendToBack(arpaf_priv->ServerReceQueue, pAudioCmdItem, 0);
		if (xStatus != pdPASS)
			awrpaf_err("\n");
	}

	return ret;
}
