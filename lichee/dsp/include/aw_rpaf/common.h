#ifndef _AW_RPAF_COMMON_H_
#define _AW_RPAF_COMMON_H_

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
#include <event_groups.h>
#include <portmacro.h>

#include <aw-alsa-lib/pcm.h>
#include <aw-alsa-lib/control.h>

#include <aw_common.h>
#include <aw_list.h>
#include <awlog.h>
#include <FreeRTOS-Plus-CLI/FreeRTOS_CLI.h>

#include <rtc_dbg.h>

#define STANDBY_NOTIFY_SUSPEND (0x1 << 0)
#define STANDBY_NOTIFY_RESUME (0x1 << 1)

#define __maybe_unused                  __attribute__((__unused__))

#if 1
#define snd_dsp_save_flag(flag) 	save_state_flag(flag)
#else
#define snd_dsp_save_flag(flag)
#endif


#define SRAM_PCM_CACHE_QUEUE 16U
#define DSP_SOUND_CARDS 4U
#define AWRPAF_MSGBOX_SIMULATOR_MODE
#define AWRPAF_MSGBOX_MODE

#define snd_readl(addr)		    (*((volatile unsigned long	*)(addr)))
#define snd_writel(v, addr) (*((volatile unsigned long	*)(addr)) = (unsigned long)(v))

/* debug option */
#define AW_RPAF_LOG_COLOR_NONE		"\e[0m"
#define AW_RPAF_LOG_COLOR_RED		"\e[31m"
#define AW_RPAF_LOG_COLOR_GREEN		"\e[32m"
#define AW_RPAF_LOG_COLOR_YELLOW	"\e[33m"
#define AW_RPAF_LOG_COLOR_BLUE		"\e[34m"

static const char * const awrpaf_print_pre = "AWRPAF_PRINT";
static const char * const awrpaf_info_pre = "AWRPAF_INFO";
static const char * const awrpaf_debug_pre = "AWRPAF_DEBUG";
static const char * const awrpaf_err_pre = "AWRPAF_ERR";

//#define AW_RPAF_PRINT
#ifdef AW_RPAF_PRINT
#define awrpaf_print(fmt, args...) \
	printf(AW_RPAF_LOG_COLOR_NONE "[%s][%s:%u]" fmt \
		AW_RPAF_LOG_COLOR_NONE, awrpaf_print_pre, __func__, __LINE__, ##args)
#else
#define awrpaf_print(fmt, args...)
#endif

//#define AW_RPAF_DEBUG
#ifdef AW_RPAF_DEBUG
#define awrpaf_debug(fmt, args...) \
	printf(AW_RPAF_LOG_COLOR_GREEN "[%s][%s:%u]" fmt \
		AW_RPAF_LOG_COLOR_NONE, awrpaf_debug_pre, __func__, __LINE__, ##args)
#else
#define awrpaf_debug(fmt, args...)
#endif

#define AW_RPAF_INFO
#ifdef AW_RPAF_INFO
#define awrpaf_info(fmt, args...) \
	printf(AW_RPAF_LOG_COLOR_BLUE "[%s][%s:%u]" fmt \
		AW_RPAF_LOG_COLOR_NONE, awrpaf_info_pre, __func__, __LINE__, ##args)
#else
#define awrpaf_info(fmt, args...)
#endif

#define AW_RPAF_ERR
#ifdef AW_RPAF_ERR
#define awrpaf_err(fmt, args...) \
	printf(AW_RPAF_LOG_COLOR_RED "[%s][%s:%u]" fmt \
		AW_RPAF_LOG_COLOR_NONE, awrpaf_err_pre, __func__, __LINE__, ##args)
#else
#define awrpaf_err(fmt, args...)
#endif

#if 0
#define rpaf_malloc(size) pvPortMalloc(size)
#define rpaf_free(ptr) vPortFree(ptr)
#else
#define rpaf_malloc(size) malloc(size)
#define rpaf_free(ptr) free(ptr)
#endif

struct msg_audio_package {
	uint32_t audioMsgVal;
	uint32_t sharePointer;
};

enum snd_stream_direction {
	SND_STREAM_PLAYBACK = 0,
	SND_STREAM_CAPTURE,
};

enum snd_codec_type {
	SND_CODEC_TYPE_PCM,
	SND_CODEC_TYPE_MP3,
	SND_CODEC_TYPE_AAC,
	SND_CODEC_TYPE_OTHER,
};

enum snd_data_type {
	SND_DATA_TYPE_PCM,
	SND_DATA_TYPE_RAW,
	SND_DATA_TYPE_OTHER,
};

enum MSGBOX_SOC_DSP_AUDIO_COMMAND {
	MSGBOX_SOC_DSP_AUDIO_NULL_COMMAND = 0,
	/* PCM stream的同步操作接口 */
	MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND,
	/* 该定义各式各样并且种类过多， 暂不做全功能支持，后者后期迭代增加 */
	MSGBOX_SOC_DSP_AUDIO_MIXER_COMMAND,

	/* for misc driver */
	/* 该定义用于调试，如：外挂codec寄存器的读写 */
	MSGBOX_SOC_DSP_AUDIO_DEBUG_COMMAND,
	/* 该定义用于组件数据读写 */
	MSGBOX_SOC_DSP_AUDIO_COMPONENT_COMMAND,
};

/*
 * param[0] = MSGBOX_SOC_DSP_AUDIO_COMMAND->MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND
 * param[1] = *snd_soc_dsp_substream
 * param[2] = SND_SOC_DSP_PCM_COMMAND
 * param[3] = *params
 */
enum SND_SOC_DSP_PCM_COMMAND {
	/*
	 * for cpudai driver and machine driver
	 * need to get status after execing at dsp & sync
	 */
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
	SND_SOC_DSP_PCM_DRAIN,
	SND_SOC_DSP_PCM_SHUTDOWN,
};

/*
 * param[0] = MSGBOX_DSP_AUDIO_COMMAND->MSGBOX_DSP_AUDIO_CTL_COMMAND
 * param[1] = *snd_dsp_hal_mixer
 * param[2] = SND_SOC_DSP_MIXER_COMMAND
 * param[3] = NULL
 */
enum SND_SOC_DSP_MIXER_COMMAND {
	/* the cmd of interface & sync */
	SND_SOC_DSP_MIXER_OPEN = 0,
	SND_SOC_DSP_MIXER_WRITE,
	SND_SOC_DSP_MIXER_READ,
	SND_SOC_DSP_MIXER_CLOSE,
};

/*
 * param[0] = MSGBOX_SOC_DSP_AUDIO_COMMAND->MSGBOX_SOC_DSP_DEBUG_COMMAND
 * param[1] = *snd_soc_dsp_substream
 * param[2] = SND_SOC_DSP_DEBUG_COMMAND
 * param[3] = *params
 */
enum SND_SOC_DSP_DEBUG_COMMAND {
	/* for getting params */
	SND_SOC_DSP_DEBUG_GET_REG,
	/* for setting params */
	SND_SOC_DSP_DEBUG_SET_REG,
	/* the cmd of pcm stream status interface & sync */
	SND_SOC_DSP_DEBUG_GET_HWPARAMS,
	SND_SOC_DSP_DEBUG_GET_PCM_STATUS,
};

/*
 * param[0] = MSGBOX_DSP_AUDIO_COMMAND->MSGBOX_DSP_AUDIO_COMPONENT_COMMAND
 * param[1] = *snd_dsp_hal_component
 * param[2] = SND_SOC_DSP_COMPONENT_COMMAND
 * param[3] = NULL
 */
enum SND_SOC_DSP_COMPONENT_COMMAND {
	/* the cmd of interface & sync */
	SND_SOC_DSP_COMPONENT_CREATE,
	SND_SOC_DSP_COMPONENT_REMOVE,
	SND_SOC_DSP_COMPONENT_SUSPEND,
	SND_SOC_DSP_COMPONENT_RESUME,
	SND_SOC_DSP_COMPONENT_STATUS,
	SND_SOC_DSP_COMPONENT_SW_PARAMS,
	SND_SOC_DSP_COMPONENT_START,
	SND_SOC_DSP_COMPONENT_STOP,
	SND_SOC_DSP_COMPONENT_WRITE,
	SND_SOC_DSP_COMPONENT_READ,

	/* stream component */
	SND_SOC_DSP_COMPONENT_SET_STREAM_PARAMS,

	/* algo control */
	SND_SOC_DSP_COMPONENT_ALGO_GET,
	SND_SOC_DSP_COMPONENT_ALGO_SET,
};

/* 用于dsp任务之间信息单元传递 */
struct snd_dsp_hal_queue_item {
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_soc_dsp_component *soc_component;
	struct snd_soc_dsp_mixer *soc_mixer;
	struct snd_soc_dsp_debug *soc_debug;

	struct snd_dsp_hal_substream *hal_substream;
	struct snd_dsp_hal_component *hal_component;
	struct snd_dsp_hal_mixer *hal_mixer;

	uint32_t pxAudioMsgVal;
	uint8_t used;
	struct list_head list;

	int32_t ret_val;
};

/* 会缓存在dram中 */
struct snd_soc_dsp_sdram_buf {
	void *read_buf;
	uint32_t read_size;
	struct list_head list;
};

struct snd_dsp_component_algo_config {
	char card_name[32];		/* 声卡名 */
	snd_pcm_stream_t stream;	/* 音频流 */
	uint32_t enable;		/* 是否使能 */
	uint32_t sort_index;	/* component使能序号 */
};

struct snd_dsp_hal_component_process_driver {
	/* 分配自身算法需要的结构空间变量和配置参数 */
	//int32_t (*create)(void **handle, struct snd_soc_dsp_native_component *native_component);
	int32_t (*create)(void **handle, void *native_component);
	/* 实现该算法处理buffer，并回填buffer, 算法设计不同时更改输入和输出buf */
	int32_t (*process)(void *handle, void *input_buffer, uint32_t * const input_size,
				void *output_buffer, uint32_t * const output_size);
	/* 算法资源释放 */
	int32_t (*release)(void *handle);

	/* 算法组件通用配置 */
	struct snd_dsp_component_algo_config config;
};

struct snd_soc_dsp_pcm_params {
	/* eg:0 audiocodec; 1 snddmic; 2 snddaudio0; 3 snddaudio1 */
	int32_t card;
	int32_t device;
	/*
	 * 根据名字匹配:
	 * 0: hw:audiocodec;
	 * 1: hw:snddmic;
	 * 2: hw:snddaudio0;
	 * 3: hw:snddaudio1;
	 */
	char driver[32];
	/* 1:capture; 0:playback */
	enum snd_stream_direction stream;

	/* -- HW params -- */
	snd_pcm_format_t format;		/* SNDRV_PCM_FORMAT_* */
	uint32_t rate;				/* rate in Hz */
	uint32_t channels;
	uint32_t resample_rate;
	/* only for hw substream */
	uint32_t period_size; /* 中断周期 */
	uint32_t periods;          /* 中断周期个数 */
	/* 在流中buffer务必一致大小, 代码中务必检查！ */
	uint32_t buffer_size;	/* 共享buf大小 */
	uint32_t pcm_frames;

	/* data type */
	enum snd_data_type data_type;
	/* mp3 - aac */
	enum snd_codec_type codec_type;
	/* dsp pcm status */
	int32_t status;

	/* 从设备树种获取的私有数据 */
	uint32_t dts_data;

	/* for dsp0 is 1, for dsp1 is 0 */
	uint32_t hw_stream;

	/* dsp data transmission mode */
	uint32_t data_mode;
	/* soc stream wake/sleep */
	uint32_t stream_wake;

	/* 独立算法组件用到的参数:buffer大小 */
	unsigned int input_size;
	unsigned int output_size;
	unsigned int dump_size;
	/* 保存算法用到的参数，具体由算法定义，预留32字节 */
	uint32_t algo_params[8];
};

enum snd_dsp_handle_type {
	HANDLE_TYPE_NULL= 0,
	HANDLE_TYPE_SOC_MIXER,
	HANDLE_TYPE_HAL_MIXER,
	HANDLE_TYPE_HAL_SUBSTREAM,
	HANDLE_TYPE_HAL_COMPONENT,
};

enum snd_dsp_list_type {
	LIST_TYPE_NULL= 0,
	LIST_TYPE_HAL_MIXER,
	LIST_TYPE_SOC_MIXER,
	LIST_TYPE_HAL_SUBSTREAM,
	LIST_TYPE_SOC_SUBSTREAM,
	LIST_TYPE_HAL_COMPONENT,
	LIST_TYPE_SOC_COMPONENT,
};

struct arpaf_priv {
	QueueHandle_t MixerReceQueue;
	/*
	 * 0:audiocodec;
	 * 1:snddaudio0;
	 * 2:snddmic
	 */
	QueueHandle_t StreamCmdQueue[DSP_SOUND_CARDS][2];

	QueueHandle_t ServerSendQueue;
	QueueHandle_t ServerReceQueue;

#if defined(AWRPAF_MSGBOX_SIMULATOR_MODE)
	QueueHandle_t MsgSendQueue;
	QueueHandle_t MsgReceQueue;
	TaskHandle_t MsgReceHandle;
	TaskHandle_t MsgSendHandle;
	TaskHandle_t CaptureHandle;
	TaskHandle_t PlaybackHandle;
#endif

	uint32_t handle_bit;
	TaskHandle_t MixerTaskHandle;

	TaskHandle_t ServerSendHandle;
	TaskHandle_t ServerReceHandle;

	struct list_head list_soc_mixer;
	SemaphoreHandle_t soc_mixer_list_mutex;

	struct list_head list_hal_mixer;
	SemaphoreHandle_t hal_mixer_list_mutex;
	int32_t (*hal_mixer_process)(void *argv);

	struct list_head list_soc_substream;
	SemaphoreHandle_t soc_substream_list_mutex;

	struct list_head list_hal_substream;
	SemaphoreHandle_t hal_substream_list_mutex;
	int32_t (*hal_substream_process)(void *argv);

	int32_t (*hal_debug_process)(void *argv);

	struct list_head list_soc_component;
	SemaphoreHandle_t soc_component_list_mutex;

	struct list_head list_hal_component;
	SemaphoreHandle_t hal_component_list_mutex;
	int32_t (*hal_component_process)(void *argv);
};

int32_t snd_dsp_audio_remote_process_init(void);

void arpaf_thread_start(const char *func_name);
void arpaf_thread_stop(const char *func_name);

QueueHandle_t arpaf_mutex_init(void);
int32_t arpaf_mutex_lock(QueueHandle_t semaphore);
int32_t arpaf_mutex_unlock(QueueHandle_t semaphore);

int32_t arpaf_hal_queue_item_list_free(struct snd_dsp_hal_queue_item *hal_queue_item);
int32_t arpaf_hal_queue_item_list_free_remove_used(struct arpaf_priv *arpaf_priv);
struct snd_dsp_hal_queue_item *arpaf_hal_queue_item_list_malloc_add_tail(
				struct arpaf_priv *arpaf_priv,
				struct snd_dsp_hal_queue_item *hal_queue_item);
void arpaf_print_soc_mixer(struct snd_soc_dsp_mixer *soc_mixer);
void arpaf_print_queue_item(struct snd_dsp_hal_queue_item *pvAudioCmdItem);

static inline void snd_dsp_pcm_module_set_fmt(
		struct snd_soc_dsp_pcm_params *pcm_params, uint32_t value)
{
	pcm_params->format = value;
}

static inline void snd_dsp_pcm_module_set_rate(
		struct snd_soc_dsp_pcm_params *pcm_params, uint32_t value)
{
	pcm_params->rate = value;
}

static inline void snd_dsp_pcm_module_set_resample(
		struct snd_soc_dsp_pcm_params *pcm_params, uint32_t value)
{
	pcm_params->resample_rate = value;
}

static inline void snd_dsp_pcm_module_set_channel(
		struct snd_soc_dsp_pcm_params *pcm_params, uint32_t value)
{
	pcm_params->channels = value;
}

static inline void snd_dsp_pcm_module_set_periods(
		struct snd_soc_dsp_pcm_params *pcm_params, uint32_t value)
{
	pcm_params->periods = value;
}

static inline void snd_dsp_pcm_module_set_period_size(
		struct snd_soc_dsp_pcm_params *pcm_params, uint32_t value)
{
	pcm_params->period_size = value;
}

static inline void snd_dsp_pcm_module_set_buffer_size(
		struct snd_soc_dsp_pcm_params *pcm_params)
{
	pcm_params->buffer_size = pcm_params->period_size * pcm_params->periods;
}

#endif
