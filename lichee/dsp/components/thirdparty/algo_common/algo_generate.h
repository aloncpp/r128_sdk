#ifndef _AW_ALGO_GENERATE_H_
#define _AW_ALGO_GENERATE_H_

#include <aw_rpaf/substream.h>
#include <aw_rpaf/component.h>
#include <aw_rpaf/common.h>

int algo_generate_install(void);

#ifdef AW_ARPAF_COMPONENT_SIMULATOR

int arpaf_pcm_data_simulator_fill(struct snd_soc_dsp_native_component *native_component,
			void *buffer, unsigned int size, unsigned int offset);
int arpaf_pcm_data_simulator_check(struct snd_soc_dsp_native_component *native_component,
			void *buffer, unsigned int size, unsigned int offset);
#endif

#endif

