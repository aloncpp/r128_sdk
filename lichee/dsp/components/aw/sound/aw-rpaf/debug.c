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

#include <aw_rpaf/common.h>
#include <aw_rpaf/substream.h>
#include <aw_rpaf/component.h>
#include <aw_rpaf/debug.h>

extern struct arpaf_priv *arpaf_priv;

/* 用于debug的处理 */
int32_t snd_dsp_hal_debug_write(struct snd_soc_dsp_debug *soc_debug)
{
	switch (soc_debug->cmd_val) {
	case SND_SOC_DSP_DEBUG_SET_REG:
		snd_writel(*(soc_debug->buf), soc_debug->addr_start);
		break;
	default:
		break;
	}
	return 0;
}

int32_t snd_dsp_hal_debug_read(struct snd_soc_dsp_debug *soc_debug)
{
#ifdef CONFIG_COMPONENTS_AW_ALSA_RPAF_SUBSTREAM
	struct snd_soc_dsp_pcm_params *pcm_params = &soc_debug->pcm_params;
	struct snd_dsp_hal_substream *hal_substream =
			snd_dsp_hal_substream_get_from_list_by_card_device_stream(
				arpaf_priv, pcm_params->card, pcm_params->device,
				pcm_params->stream);
	struct snd_soc_dsp_substream *soc_substream = hal_substream->soc_substream;
#endif
	int32_t i = 0;

	switch (soc_debug->cmd_val) {
	case SND_SOC_DSP_DEBUG_GET_REG:
		for (i = 0; i < (soc_debug->addr_end - soc_debug->addr_start)/4; i++) {
			*(soc_debug->buf + i) = 0;
			*(soc_debug->buf + i) = i;//snd_readl(soc_debug->addr_start + (i << 2));
		}
		break;
	case SND_SOC_DSP_DEBUG_GET_HWPARAMS:
	case SND_SOC_DSP_DEBUG_GET_PCM_STATUS:
#ifdef CONFIG_COMPONENTS_AW_ALSA_RPAF_SUBSTREAM
		if (hal_substream)
			memcpy(pcm_params, &soc_substream->params,
				sizeof(struct snd_soc_dsp_pcm_params));
		else
#endif
			awrpaf_err("hal_substream is null.\n");
		break;
	default:
		break;
	}
	return 0;
}

int32_t snd_dsp_hal_debug_process(void *argv)
{
	struct snd_dsp_hal_queue_item *pAudioCmdItem = argv;
	struct snd_soc_dsp_debug *soc_debug = pAudioCmdItem->soc_debug;
	BaseType_t xStatus;

	switch (soc_debug->cmd_val) {
	case SND_SOC_DSP_DEBUG_GET_REG:
	case SND_SOC_DSP_DEBUG_GET_HWPARAMS:
	case SND_SOC_DSP_DEBUG_GET_PCM_STATUS:
		soc_debug->ret_val = snd_dsp_hal_debug_read(soc_debug);
		break;
	case SND_SOC_DSP_DEBUG_SET_REG:
		soc_debug->ret_val = snd_dsp_hal_debug_write(soc_debug);
		break;
	default:
		break;
	}

	xStatus = xQueueSendToBack(arpaf_priv->ServerReceQueue, pAudioCmdItem, pdMS_TO_TICKS(100));
	if (xStatus != pdPASS) {
		awrpaf_err("ServerReceQueue send faild.\n");
	}
	return 0;
}

