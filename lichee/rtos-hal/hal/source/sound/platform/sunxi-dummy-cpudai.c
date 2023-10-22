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
#include <stdio.h>
#include <string.h>
#include <hal_dma.h>
#include <sound/snd_core.h>
#include <sound/snd_pcm.h>

#include "sunxi-pcm.h"
#include "sunxi-dummy-cpudai.h"

#ifdef CONFIG_SND_PLATFORM_SUNXI_MAD
#include "sunxi-mad.h"
#endif

struct sunxi_cpudai_info {
	struct sunxi_dma_params playback_dma_param;
	struct sunxi_dma_params capture_dma_param;
};

static int sunxi_cpudai_platform_probe(struct snd_platform *platform)
{
	struct sunxi_cpudai_info *info = NULL;

	snd_print("\n");
	if (!platform->cpu_dai)
		return -1;

	info = snd_malloc(sizeof(struct sunxi_cpudai_info));
	if (!info) {
		snd_err("no memory\n");
		return -ENOMEM;
	}
	platform->private_data = (void *)info;
	/* dma para */
	info->playback_dma_param.dma_addr = (dma_addr_t)(SUNXI_CODEC_BASE_ADDR + SUNXI_DAC_TXDATA);
	info->playback_dma_param.dma_drq_type_num = DRQDST_AUDIO_CODEC;
	info->playback_dma_param.dst_maxburst = 4;
	info->playback_dma_param.src_maxburst = 4;

	info->capture_dma_param.dma_addr = (dma_addr_t)(SUNXI_CODEC_BASE_ADDR + SUNXI_ADC_RXDATA);
	info->capture_dma_param.dma_drq_type_num = DRQSRC_AUDIO_CODEC;
	info->capture_dma_param.dst_maxburst = 4;
	info->capture_dma_param.src_maxburst = 4;

	platform->cpu_dai->component = platform;

	return 0;
}

#if defined(CONFIG_SND_CODEC_AUDIOCODEC_DAC)
static int sunxi_cpudai_platform_probe_dac(struct snd_platform *platform)
{
	struct sunxi_cpudai_info *info = NULL;

	snd_print("\n");
	if (!platform->cpu_dai)
		return -1;

	info = snd_malloc(sizeof(struct sunxi_cpudai_info));
	if (!info) {
		snd_err("no memory\n");
		return -ENOMEM;
	}
	platform->private_data = (void *)info;
	/* dma para */
	info->playback_dma_param.dma_addr = (dma_addr_t)(SUNXI_CODEC_BASE_ADDR + SUNXI_DAC_TXDATA);
	info->playback_dma_param.dma_drq_type_num = DRQDST_AUDIO_CODEC;
	info->playback_dma_param.dst_maxburst = 4;
	info->playback_dma_param.src_maxburst = 4;

	info->capture_dma_param.dma_addr = (dma_addr_t)(SUNXI_CODEC_BASE_ADDR + AC_DAC_LBFIFO);
	info->capture_dma_param.dma_drq_type_num = DRQSRC_CODEC_DAC_RX;
	info->capture_dma_param.dst_maxburst = 4;
	info->capture_dma_param.src_maxburst = 4;

	platform->cpu_dai->component = platform;

	return 0;
}
#else
static int sunxi_cpudai_platform_probe_dac(struct snd_platform *platform)
{
	(void)platform;
	snd_err("api is disable\n");
	return 0;
}
#endif

#if defined(CONFIG_SND_CODEC_AUDIOCODEC_ADC)
static int sunxi_cpudai_platform_probe_adc(struct snd_platform *platform)
{
	struct sunxi_cpudai_info *info = NULL;

	snd_print("\n");
	if (!platform->cpu_dai)
		return -1;

	info = snd_malloc(sizeof(struct sunxi_cpudai_info));
	if (!info) {
		snd_err("no memory\n");
		return -ENOMEM;
	}
	platform->private_data = (void *)info;
	/* dma para */
	info->capture_dma_param.dma_addr = (dma_addr_t)(SUNXI_CODEC_BASE_ADDR + SUNXI_ADC_RXDATA);
	info->capture_dma_param.dma_drq_type_num = DRQSRC_AUDIO_CODEC;
	info->capture_dma_param.dst_maxburst = 4;
	info->capture_dma_param.src_maxburst = 4;

	platform->cpu_dai->component = platform;

	return 0;
}
#else
static int sunxi_cpudai_platform_probe_adc(struct snd_platform *platform)
{
	(void)platform;
	snd_err("api is disable\n");
	return 0;
}
#endif

static int sunxi_cpudai_platform_remove(struct snd_platform *platform)
{
	struct sunxi_cpudai_info *info = NULL;

	snd_print("\n");
	info = platform->private_data;
	snd_free(info);
	platform->private_data = NULL;
	return 0;
}

#ifdef CONFIG_SND_PLATFORM_SUNXI_MAD
static int sunxi_cpudai_mad_startup(struct snd_pcm_substream *substream, struct snd_dai *dai)
{
	struct snd_platform *platform = snd_soc_dai_get_component(dai);
	struct sunxi_cpudai_info *info = platform->private_data;
	int ret = 0;
	bool mad_bind = false;

	(void)substream;

	snd_print("mad\n");

	ret = sunxi_mad_bind_get(MAD_PATH_CODECADC, &mad_bind);
	if (ret) {
		mad_bind = false;
		snd_err("get mad_bind failed, path: %d\n", MAD_PATH_CODECADC);
	}

	if (mad_bind) {
		snd_print("mad bind\n");
		sunxi_sram_dma_config(&info->capture_dma_param);
		return 0;
	}

	snd_print("mad unbind\n");

	info->capture_dma_param.dma_addr = (dma_addr_t)(SUNXI_CODEC_BASE_ADDR + SUNXI_ADC_RXDATA);
	info->capture_dma_param.dma_drq_type_num = DRQSRC_AUDIO_CODEC;

	return 0;
}
#endif

static int sunxi_cpudai_startup(struct snd_pcm_substream *substream, struct snd_dai *dai)
{
	struct snd_platform *platform = snd_soc_dai_get_component(dai);
	struct sunxi_cpudai_info *info = platform->private_data;

	snd_print("\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dai->playback_dma_data = &info->playback_dma_param;
	} else {
#ifdef CONFIG_SND_PLATFORM_SUNXI_MAD
		snd_print("mad\n");
		sunxi_cpudai_mad_startup(substream, dai);
#endif
		dai->capture_dma_data = &info->capture_dma_param;
	}

	return 0;
}

#ifdef CONFIG_SND_PLATFORM_SUNXI_MAD
static void sunxi_cpudai_mad_shutdown(struct snd_pcm_substream *substream, struct snd_dai *dai)
{
	struct snd_platform *platform = snd_soc_dai_get_component(dai);
	struct sunxi_cpudai_info *info = platform->private_data;

	(void)substream;

	snd_print("mad\n");

	/*if not use mad again*/
	info->capture_dma_param.dma_addr = (dma_addr_t)(SUNXI_CODEC_BASE_ADDR + SUNXI_ADC_RXDATA);
	info->capture_dma_param.dma_drq_type_num = DRQSRC_AUDIO_CODEC;
}
#endif

static void sunxi_cpudai_shutdown(struct snd_pcm_substream *substream, struct snd_dai *dai)
{
#ifdef CONFIG_SND_PLATFORM_SUNXI_MAD
	snd_print("mad\n");

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		sunxi_cpudai_mad_shutdown(substream, dai);
#endif
}

#if 0
static int sunxi_cpudai_probe(struct snd_dai *dai)
{
	dai->playback_dma_data = 0;
	dai->capture_dma_data = 0;
}
#endif

static struct snd_dai_ops sunxi_cpudai_dai_ops = {
	.startup = sunxi_cpudai_startup,
	.shutdown = sunxi_cpudai_shutdown,
};

static struct snd_dai sunxi_cpudai = {
	.id		= 1,
	.name		= "audiocodec-cpudai",
	/*.probe	= sunxi_cpudai_probe,*/
	.playback	= {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
		.rate_min	= 8000,
		.rate_max	= 192000,
	},
	.capture	= {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 4,
		.rates		= SNDRV_PCM_RATE_8000_48000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
		.rate_min	= 8000,
		.rate_max	= 48000,
	},
	.ops		= &sunxi_cpudai_dai_ops,
};

#if 0
struct snd_platform gAudioCodecPlatform = {
	.name		= "audiocodec-cpudai",
	.ops		= &sunxi_pcm_ops,
	.pcm_new	= sunxi_pcm_new,
	.pcm_free	= sunxi_pcm_free,
	.cpu_dai	= &sunxi_cpudai,
	.private_data	= NULL,
};
#endif

int snd_platform_cpudai_register(struct snd_platform *platform, int arg)
{
	int ret = 0;

	platform->name = strdup("audiocodec-platform");
	platform->ops = &sunxi_pcm_ops;
	platform->pcm_new = sunxi_pcm_new;
	platform->pcm_free = sunxi_pcm_free;
	platform->cpu_dai = snd_malloc(sizeof(struct snd_dai));
	if (!platform->cpu_dai) {
		snd_err("cpu_dai malloc failed.\n");
		ret = -ENOMEM;
		goto err_cpu_dai_malloc;
	}
	memcpy(platform->cpu_dai, &sunxi_cpudai, sizeof(struct snd_dai));

	switch (arg) {
		case SND_PLATFORM_TYPE_CPUDAI:
			platform->probe = sunxi_cpudai_platform_probe;
			break;
		case SND_PLATFORM_TYPE_CPUDAI_DAC:
			platform->probe = sunxi_cpudai_platform_probe_dac;
			break;
		case SND_PLATFORM_TYPE_CPUDAI_ADC:
			platform->probe = sunxi_cpudai_platform_probe_adc;
			break;
		default:
			goto err_platform_type;
	}
	platform->remove = sunxi_cpudai_platform_remove;

	return 0;

err_platform_type:
	snd_free(platform->cpu_dai);
err_cpu_dai_malloc:
	snd_free(platform->name);
	return ret;
}
