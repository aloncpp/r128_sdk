#ifndef _AW_RPAF_MIXER_H_
#define _AW_RPAF_MIXER_H_

#include <aw_rpaf/common.h>

/*
 * param[0] = MSGBOX_SOC_DSP_AUDIO_COMMAND->MSGBOX_SOC_DSP_AUDIO_MIXER_COMMAND
 * param[1] = *snd_soc_dsp_mixer
 * param[2] = SND_SOC_DSP_*_COMMAND
 * param[3] = *params/NULL
 */
struct snd_soc_dsp_mixer {
	uint32_t id;
	uint8_t used;

	uint32_t cmd_val;
	uint32_t params_val;

	/* eg:0 sndcodec; 1 snddmic; 2 snddaudio0; 3 snddaudio1 */
	int32_t card;
	int32_t device;
	/*
	 * 根据名字匹配:
	 * 0: maudiocodec; 1: msnddmic; 2: msnddaudio0; 3: msnddaudio1;
	 */
	char driver[32];

	/* ctl name length */
	char ctl_name[44];
	uint32_t value;

	/*API调用完毕之后需要判断该值 */
	int32_t ret_val;

	struct list_head list;
};

struct snd_dsp_hal_mixer_ops {
	//int32_t (*open)(struct snd_dsp_hal_mixer *mixer);
	int32_t (*open)(void *mixer);
	int32_t (*close)(void *mixer);
	int32_t (*read)(void *mixer);
	int32_t (*write)(void *mixer);
};

struct snd_dsp_hal_mixer {
	/* dsp声卡名字和编号 */
	const char *name;
	uint32_t id;

	/* 从共享内存共享过来 */
	struct snd_soc_dsp_mixer *soc_mixer;

	xTaskHandle *taskHandle;

	/* 用于AudioMixerTask和audioserver通信，回调操作ops */
//	xQueueHandle *ServerReceQueue;
//	xQueueHandle *ServerSendQueue;
//	struct snd_dsp_hal_queue_item CmdItem;

	/* 可以统一实现或者根据声卡具体对应实现 */
	struct snd_dsp_hal_mixer_ops *mixer_ops;

	/*API调用完毕之后需要判断该值 */
	int32_t ret_val;

	void *private_data;

	struct list_head list;
};

int32_t snd_dsp_hal_mixer_process(void *argv);

#endif

