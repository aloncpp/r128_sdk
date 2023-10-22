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
#define TAG	"AHWConf"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "audio_hw.h"
#include "AudioBase.h"
#include "local_audio_hw.h"

#include "AudioSystem.h"

#include <hal_cfg.h>
#include <script.h>

as_pcm_config_t *ahw_params_init(const char *prefix, struct ahw_params *params[], audio_hw_t *ahw)
{
	int32_t tmp_val;
	char prefix_name[32];
	char card_name[32];
	char value_name[32];
	as_pcm_config_t *pcm_config = NULL;
	struct audio_hw_elem *elem = NULL;
	int stream;

	stream = audio_hw_stream(ahw);
	elem = audio_hw_elem_item(ahw);

	if (params[stream])
		goto alread_init;

	pcm_config = audio_hw_pcm_config(ahw);

#ifdef CONFIG_ARCH_RISCV_C906
#define AHW_SUFFIX	"_rv"
#elif defined(CONFIG_ARCH_ARM_CORTEX_M33)
#define AHW_SUFFIX	"_m33"
#elif defined(CONFIG_ARCH_DSP)
#define AHW_SUFFIX	"_dsp"
#else
#define AHW_SUFFIX	""
#endif
	/* init once */
	params[stream] = as_alloc(sizeof(struct ahw_params));
	if (!params[stream]) {
		_err("no memory");
		return NULL;
	}

	if (!stream) {
		params[stream]->rate = AHW_DEFAULT_PB_RATE;
		params[stream]->channels = AHW_DEFAULT_PB_CHANNELS;
		params[stream]->bits = 16;
		params[stream]->period_frames = AHW_DEFAULT_PB_PERIOD_SIZE;
		params[stream]->periods = AHW_DEFAULT_PB_PERIODS;
		snprintf(prefix_name, sizeof(prefix_name), "%s_pb", prefix);
	} else {
		params[stream]->rate = AHW_DEFAULT_CAP_RATE;
		params[stream]->channels = AHW_DEFAULT_CAP_CHANNELS;
		params[stream]->bits = 16;
		params[stream]->period_frames = AHW_DEFAULT_CAP_PERIOD_SIZE;
		params[stream]->periods = AHW_DEFAULT_CAP_PERIODS;
		snprintf(prefix_name, sizeof(prefix_name), "%s_cap", prefix);
	}

	snprintf(value_name, sizeof(value_name), "%s_rate", prefix_name);
	if (!hal_cfg_get_keyvalue("audio_hw"AHW_SUFFIX, value_name, (int32_t *)&tmp_val, 1))
		params[stream]->rate = tmp_val;
	snprintf(value_name, sizeof(value_name), "%s_channels", prefix_name);
	if (!hal_cfg_get_keyvalue("audio_hw"AHW_SUFFIX, value_name, (int32_t *)&tmp_val, 1))
		params[stream]->channels = tmp_val;
	snprintf(value_name, sizeof(value_name), "%s_bits", prefix_name);
	if (!hal_cfg_get_keyvalue("audio_hw"AHW_SUFFIX, value_name, (int32_t *)&tmp_val, 1))
		params[stream]->bits = tmp_val;
	snprintf(value_name, sizeof(value_name), "%s_period_size", prefix_name);
	if (!hal_cfg_get_keyvalue("audio_hw"AHW_SUFFIX, value_name, (int32_t *)&tmp_val, 1))
		params[stream]->period_frames = tmp_val;
	snprintf(value_name, sizeof(value_name), "%s_periods", prefix_name);
	if (!hal_cfg_get_keyvalue("audio_hw"AHW_SUFFIX, value_name, (int32_t *)&tmp_val, 1))
		params[stream]->periods = tmp_val;
	snprintf(value_name, sizeof(value_name), "%s_card", prefix_name);
	if (!hal_cfg_get_keyvalue("audio_hw"AHW_SUFFIX, value_name, (int32_t *)card_name, sizeof(card_name) / sizeof(int)))
		strncpy(params[stream]->card_name, card_name, sizeof(params[stream]->card_name));

	_debug("%s params:", "audio_hw"AHW_SUFFIX);
	_debug("rate:%d", params[stream]->rate);
	_debug("channels:%d", params[stream]->channels);
	_debug("bits:%d", params[stream]->bits);
	_debug("period_size:%d", params[stream]->period_frames);
	_debug("periods:%d", params[stream]->periods);
	_debug("card_name:%s", params[stream]->card_name);

alread_init:
	pcm_config->rate = params[stream]->rate;
	pcm_config->channels = params[stream]->channels;
	switch (params[stream]->bits) {
	case 16:
		pcm_config->format = SND_PCM_FORMAT_S16_LE;
		break;
	case 32:
		pcm_config->format = SND_PCM_FORMAT_S32_LE;
		break;
	}
	pcm_config->period_frames = params[stream]->period_frames;
	pcm_config->periods = params[stream]->periods;

	if (!strlen(params[stream]->card_name))
		return pcm_config;
	if (!stream)
		elem->card_name_pb = params[stream]->card_name;
	else
		elem->card_name_cap = params[stream]->card_name;

	return pcm_config;
}
