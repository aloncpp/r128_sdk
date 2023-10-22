#ifndef _AW_RPAF_SUBSTREAM_H_
#define _AW_RPAF_SUBSTREAM_H_

#include <aw_rpaf/common.h>
#include <aw_rpaf/component.h>

struct sram_buffer {
	void *buf_addr;
	unsigned char used;
	struct list_head list;
};

/*
 * param[0] = MSGBOX_SOC_DSP_AUDIO_COMMAND->MSGBOX_SOC_DSP_*_COMMAND
 * param[1] = *snd_soc_dsp_substream
 * param[2] = SND_SOC_DSP_*_COMMAND
 * param[3] = *params/NULL
 */
struct snd_soc_dsp_substream {
	uint32_t id;
	unsigned char used;

	uint32_t cmd_val;
	uint32_t params_val;
	uint32_t audio_standby;

	struct snd_soc_dsp_pcm_params params;

	/* share data address */
	uint32_t input_addr;
	uint32_t output_addr;
	/* data_length < buf_size */
	uint32_t input_size;
	uint32_t output_size;

	/*API调用完毕之后需要判断该值 */
	int32_t ret_val;

	struct list_head list;
};

//共享内存分配：
//（1）用于音频对象的存储
//（2）用于音频共享数据

//策略：
//每次都会传输共用一个对象区域，附带共享音频buffer
struct snd_soc_dsp_queue_item {
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_soc_dsp_component *soc_comp;
	struct snd_soc_dsp_mixer *soc_mixer;
	struct snd_soc_dsp_debug *soc_debug;

	uint32_t msg_val;
	uint32_t cmd_val;
	uint32_t param_val;
};

/*
 * For DSP Audio Framework API
 */
struct snd_dsp_hal_substream_ops {
	/*
	 * ALSA PCM audio operations - all optional.
	 * Called by soc-core during audio PCM operations.
	 */
	/* 对接声卡的开关操作 */
	//int32_t (*startup)(struct snd_dsp_hal_substream *substream);
	int32_t (*startup)(void *substream);
	void (*shutdown)(void *substream);

	int32_t (*prepare)(void *substream);
	int32_t (*start)(void *substream);
	int32_t (*stop)(void *substream);
	int32_t (*drain)(void *substream);

	/* 将音频PCM格式传入进行设置 */
	int32_t (*hw_params)(void *substream);

	/* 用于数据的读操作, 数据最后才给到substream->soc_substream->buf_addr */
	snd_pcm_sframes_t (*readi)(void *substream);
	/* 用于数据的写操作, 数据最后才给到substream->soc_substream->buf_addr */
	snd_pcm_sframes_t (*writei)(void *substream);

	uint32_t (*status_params)(void *substream,
				//enum SND_SOC_DSP_PARAMS_COMMAND
				enum SND_SOC_DSP_PCM_COMMAND cmd,
				void *params);
};

struct snd_dsp_hal_substream_driver {
	//int32_t (*probe)(struct snd_dsp_hal_substream *substream);
	int32_t (*probe)(void *substream);
	int32_t (*remove)(void *substream);
	int32_t (*suspend)(void *substream);
	int32_t (*resume)(void *substream);
};

enum audio_standby_state {
	/* linux正常录音时 */
	AUDIO_STANDBY_NORMAL_STATE = 0x0,
	/* linux停止录音时 */
	AUDIO_STANDBY_STOP_STATE = 0x1,
	/* linux进入休眠后, dsp进入wfi前 */
	AUDIO_STANDBY_SUSPEND_STATE = 0x2,
	/* dsp进入WFI */
	AUDIO_STANDBY_DSP_WFI_STATE = 0x3,
	/* dsp退出WFI */
	AUDIO_STANDBY_DSP_WAKEUP_STATE = 0x4,
	/* linux退出休眠时 */
	AUDIO_STANDBY_RESUME_STATE = 0x5,
	/* linux关闭声卡时 */
	AUDIO_STANDBY_SHUTDOWN_STATE = 0x6,
};

struct snd_dsp_hal_substream {
	const char *name;
	uint32_t id;
	uint32_t handle_bit;
	EventGroupHandle_t xTaskCreateEvent;
	char taskName[configMAX_TASK_NAME_LEN];

	uint32_t need_dsp_suspend;
	uint32_t soc_suspended;
	uint32_t soc_resumed;

	snd_pcm_t *pcm_handle;

	xTaskHandle keyWordTask;
	uint32_t keyWordRun;
	bool stream_active;
	bool pcm_reading;
	enum audio_standby_state standby_state;
	void *sram_addr;
	struct sram_buffer sram_buf[SRAM_PCM_CACHE_QUEUE];
	char keyWordTaskName[configMAX_TASK_NAME_LEN];
	struct list_head list_sram_buf;
	QueueHandle_t sram_buf_list_mutex;
	SemaphoreHandle_t srambuf_tsleep;

	uint32_t prepared;

	/* 从共享内存共享过来并缓存 */
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_soc_dsp_native_component native_component; /* dsp使用 */

	/* 在list中的对象中hal_substream无法找到，则新建播放或者录音任务 */
	xTaskHandle taskHandle;
	SemaphoreHandle_t stream_mutex;
	/* 用于AudioTask和audioserver/msgboxserver通信，回调操作ops */
	xQueueHandle cmdQueue;
	struct snd_dsp_hal_queue_item pvAudioCmdItem;

	/* 可以统一实现或者根据声卡具体对应实现 */
	struct snd_dsp_hal_substream_driver driver_ops;
	struct snd_dsp_hal_substream_ops substream_ops;

	/*API调用完毕之后需要判断该值 */
	int32_t ret_val;

	void *private_data;

	struct list_head list;
};

int32_t snd_dsp_hal_substream_process(void *argv);

struct snd_dsp_hal_substream *snd_dsp_hal_substream_get_from_list_by_card_device_stream(
	struct arpaf_priv *arpaf_priv, uint32_t card, uint32_t device,
	uint32_t stream);

#endif

