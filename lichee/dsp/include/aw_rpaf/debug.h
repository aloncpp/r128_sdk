#ifndef _AW_RPAF_DEBUG_H
#define _AW_RPAF_DEBUG_H

#include <aw_rpaf/common.h>

/*
 * param[0] = MSGBOX_SOC_DSP_AUDIO_COMMAND->MSGBOX_SOC_DSP_DEBUG_COMMAND
 * param[1] = *snd_soc_dsp_substream
 * param[2] = SND_SOC_DSP_DEBUG_COMMAND
 * param[3] = *params
 */
struct snd_soc_dsp_debug {
	uint32_t cmd_val;
	uint32_t params_val;

	struct snd_soc_dsp_pcm_params pcm_params;
	/* 共享内存地址，根据首末地址差分配空间大小 */
	uint32_t *buf;
	/* 起始地址和结束地址 */
	uint32_t addr_start;
	uint32_t addr_end;
	/* 读还是写数值 */
	uint32_t mode;

	/*API调用完毕之后需要判断该值 */
	int32_t ret_val;
};

int32_t snd_dsp_hal_debug_write(struct snd_soc_dsp_debug *soc_debug);
int32_t snd_dsp_hal_debug_read(struct snd_soc_dsp_debug *soc_debug);
int32_t snd_dsp_hal_debug_process(void *argv);

#endif
