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
#include <stdlib.h>
#include <string.h>
#include <hal_gpio.h>
#include <hal_dma.h>
#include <hal_clk.h>
#include <hal_reset.h>
#include <hal_time.h>
#ifdef CONFIG_DRIVER_SYSCONFIG
#include <script.h>
#include <hal_cfg.h>
#endif
#ifdef CONFIG_COMPONENTS_PM
#include <pm_devops.h>
#endif
#include <sound/snd_core.h>
#include <sound/snd_pcm.h>
#include <sound/snd_dma.h>
#include <sound/dma_wrap.h>
#include <sound/snd_io.h>
#include <sound/pcm_common.h>
#include <sound/common/snd_sunxi_common.h>
#include <sound/common/snd_sunxi_alg_cfg.h>

#include "sun20iw2-codec.h"

struct snd_codec sunxi_audiocodec_dac;

/*
 * playback         : period_bytes=960*(16*2/8)=3840, buffer_bytes=3840*4=15360
 * capture(loopback): period_bytes=960*(16*2/8)=3840, buffer_bytes=3840*4=15360
 */
static struct snd_pcm_hardware sun20iw2_hardware[2] = {
	{	/* SNDRV_PCM_STREAM_PLAYBACK */
		.info			= SNDRV_PCM_INFO_INTERLEAVED
					| SNDRV_PCM_INFO_BLOCK_TRANSFER
					| SNDRV_PCM_INFO_MMAP
					| SNDRV_PCM_INFO_MMAP_VALID
					| SNDRV_PCM_INFO_PAUSE
					| SNDRV_PCM_INFO_RESUME,
		.buffer_bytes_max	= 15360,
		.period_bytes_min	= 256,
		.period_bytes_max	= 7680,
		.periods_min		= 2,
		.periods_max		= 8,
	},
	{	/* SNDRV_PCM_STREAM_CAPTURE */
		.info			= SNDRV_PCM_INFO_INTERLEAVED
					| SNDRV_PCM_INFO_BLOCK_TRANSFER
					| SNDRV_PCM_INFO_MMAP
					| SNDRV_PCM_INFO_MMAP_VALID
					| SNDRV_PCM_INFO_PAUSE
					| SNDRV_PCM_INFO_RESUME,
		.buffer_bytes_max	= 15360,
		.period_bytes_min	= 256,
		.period_bytes_max	= 7680,
		.periods_min		= 2,
		.periods_max		= 8,
	},
};

#ifdef CONFIG_COMPONENTS_PM
static struct audio_reg_label sunxi_reg_labels[] = {
	/* power reg */
	REG_LABEL(AC_POWER_CTRL),

	/* dac ana reg */
	REG_LABEL(AC_DAC_ANA_CTRL),

	/* dac dig reg */
	REG_LABEL(AC_DAC_DIG_CTRL),
	REG_LABEL(AC_DAC_DIG_VOL),
	REG_LABEL(AC_DAC_DPH_GAIN),
	REG_LABEL(AC_DAC_TXFIFO_CTRL),
	REG_LABEL(AC_DAC_TXFIFO_STA),
	/* REG_LABEL(SUNXI_DAC_TXDATA), */
	REG_LABEL(AC_DAC_TXCNT),
	REG_LABEL(AC_DAC_LBFIFO_CTRL),
	REG_LABEL(AC_DAC_LBFIFO_STA),
	REG_LABEL(AC_DAC_LBFIFO),
	REG_LABEL(AC_DAC_LBCNT),
	REG_LABEL(AC_DAC_DEBUG),
	REG_LABEL_END,
};
#endif

static struct sunxi_codec_param default_param = {
	.dacl_vol	= 129,
	.dacr_vol	= 129,
	.lineout_vol	= 7,
	.lineoutl_en	= true,
	.lineoutr_en	= true,
};

static struct sunxi_pa_config default_pa_cfg = {
	.gpio 		= GPIOB(3),
	.drv_level	= GPIO_DRIVING_LEVEL1,
	.mul_sel	= GPIO_MUXSEL_OUT,
	.data		= GPIO_DATA_HIGH,
	.pa_msleep_time	= 160,
};

struct sample_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};

/* dac_clk_div = audio_pll_clk / (fs*128)
 * 0 -> div1
 * 1 -> div2
 * 2 -> div3
 * 3 -> div4
 * 4 -> div6
 * 5 -> div8
 * 6 -> div12
 * 7 -> div16
 * 8 -> div24
 */
static const struct sample_rate sample_rate_conv[] = {
	{384000, 0},	/* audio_pll_clk -> 49.152M */
	{352800, 0},	/* audio_pll_clk -> 45.1584M */
	{192000, 0},	/* audio_pll_clk -> 24.576M */
	{176400, 0},	/* audio_pll_clk -> 22.5792M */
	{96000, 1},
	{88200, 1},
	{64000, 2},
	{48000, 3},
	{44100, 3},
	{32000, 4},
	{24000, 5},
	{22050, 5},
	{16000, 6},
	{12000, 7},
	{11025, 7},
	{8000, 8},
};

static const char *const codec_switch_onoff[] = {"off", "on"};

static int snd_sunxi_clk_init(struct sunxi_codec_clk *clk);
static void snd_sunxi_clk_exit(struct sunxi_codec_clk *clk);
static int snd_sunxi_clk_enable(struct sunxi_codec_clk *clk);
static void snd_sunxi_clk_disable(struct sunxi_codec_clk *clk);
static int snd_sunxi_clk_set_rate(struct sunxi_codec_clk *clk, int stream,
				  unsigned int freq_in, unsigned int freq_out);

static int sunxi_codec_set_sysclk(struct snd_dai *dai, int clk_id, unsigned int freq, int dir)
{
	int ret;
	struct snd_codec *codec = dai->component;
	struct sunxi_codec_info *sunxi_codec = codec->private_data;

	snd_print("\n");

	ret = snd_sunxi_clk_set_rate(&sunxi_codec->clk, dir, freq, freq);
	if (ret < 0) {
		snd_err("snd_sunxi_clk_set_rate failed\n");
		return -1;
	}

	return 0;
}

static int sunxi_codec_startup(struct snd_pcm_substream *substream, struct snd_dai *dai)
{
	snd_print("\n");

	return 0;
}

static void sunxi_codec_shutdown(struct snd_pcm_substream *substream, struct snd_dai *dai)
{
	snd_print("\n");

	return;
}

static int sunxi_codec_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_dai *dai)
{
	struct snd_codec *codec = dai->component;
	int i;
	unsigned int simple_rates;
	unsigned int simple_channels;

	snd_print("\n");

	/* simple bit */
	switch (params_format(params)) {
	case	SND_PCM_FORMAT_S16_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_codec_update_bits(codec, AC_DAC_TXFIFO_CTRL,
				1 << DAC_FIFO_MODE, 1 << DAC_FIFO_MODE);
			snd_codec_update_bits(codec, AC_DAC_TXFIFO_CTRL,
				1 << DAC_SAMPLE_BITS, 0 << DAC_SAMPLE_BITS);
		} else {
			snd_codec_update_bits(codec, AC_DAC_LBFIFO_CTRL,
				1 << DAC_LB_FIFO_MODE, 1 << DAC_LB_FIFO_MODE);
			snd_codec_update_bits(codec, AC_DAC_LBFIFO_CTRL,
				1 << DAC_LB_SAMPLE_BITS, 0 << DAC_LB_SAMPLE_BITS);
		}
		break;
	case	SND_PCM_FORMAT_S24_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_codec_update_bits(codec, AC_DAC_TXFIFO_CTRL,
				1 << DAC_FIFO_MODE, 0 << DAC_FIFO_MODE);
			snd_codec_update_bits(codec, AC_DAC_TXFIFO_CTRL,
				1 << DAC_SAMPLE_BITS, 1 << DAC_SAMPLE_BITS);
		} else {
			snd_codec_update_bits(codec, AC_DAC_LBFIFO_CTRL,
				1 << DAC_LB_FIFO_MODE, 0 << DAC_LB_FIFO_MODE);
			snd_codec_update_bits(codec, AC_DAC_LBFIFO_CTRL,
				1 << DAC_LB_SAMPLE_BITS, 1 << DAC_LB_SAMPLE_BITS);
		}
		break;
	default:
		snd_err("cannot support bits:%u.\n", params_format(params));
		return -EINVAL;
	}

	/* simple rate */
	simple_rates = params_rate(params);
	for (i = 0; i < ARRAY_SIZE(sample_rate_conv); i++) {
		if (sample_rate_conv[i].samplerate == simple_rates)
			break;
	}
	if (sample_rate_conv[i].samplerate != simple_rates) {
		snd_err("cannot support play rates:%u.\n", simple_rates);
		return -EINVAL;
	}
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		/* for digital clk */
		snd_codec_update_bits(codec, AC_DAC_DIG_CTRL, 0xf << DAC_CLK,
			sample_rate_conv[i].rate_bit << DAC_CLK);
		/* for analog clk */
		switch (simple_rates) {
		case 384000:
		case 352800:
		case 192000:
		case 176400:
			snd_codec_update_bits(codec, AC_DAC_DIG_CTRL, 3 << DAC_OSR, 2 << DAC_OSR);
			break;
		case 96000:
		case 88200:
			snd_codec_update_bits(codec, AC_DAC_DIG_CTRL, 3 << DAC_OSR, 1 << DAC_OSR);
			break;
		default:
			snd_codec_update_bits(codec, AC_DAC_DIG_CTRL, 3 << DAC_OSR, 0 << DAC_OSR);
			break;
		}
	} else {
		/* null */
	}

	/* simple channel */
	simple_channels = params_channels(params);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (simple_channels) {
		case 1:
			/* enable mono mode, txr mix should from TXL */
			snd_codec_update_bits(codec, AC_DAC_TXFIFO_CTRL,
					1 << DAC_MONO_EN, 1 << DAC_MONO_EN);
			snd_codec_update_bits(codec, AC_DAC_DIG_CTRL,
					0x3 << TXR_MIX_CTRL, 0x0 << TXR_MIX_CTRL);
			break;
		case 2:
			/* disable mono mode, txr mix from TXR */
			snd_codec_update_bits(codec, AC_DAC_TXFIFO_CTRL,
					1 << DAC_MONO_EN, 0 << DAC_MONO_EN);
			snd_codec_update_bits(codec, AC_DAC_DIG_CTRL,
					0x3 << TXR_MIX_CTRL, 0x1 << TXR_MIX_CTRL);
			break;
		default:
			snd_err("cannot support the channels:%u.\n", simple_channels);
			return -EINVAL;
		}
	} else {
		switch (simple_channels) {
		case 1:
			snd_codec_update_bits(codec, AC_DAC_LBFIFO_CTRL,
					1 << DACL_LB_EN, 1 << DACL_LB_EN);
			snd_codec_update_bits(codec, AC_DAC_LBFIFO_CTRL,
					1 << DACR_LB_EN, 0 << DACR_LB_EN);
			break;
		case 2:
			snd_codec_update_bits(codec, AC_DAC_LBFIFO_CTRL,
					1 << DACL_LB_EN, 1 << DACL_LB_EN);
			snd_codec_update_bits(codec, AC_DAC_LBFIFO_CTRL,
					1 << DACR_LB_EN, 1 << DACR_LB_EN);
			break;
		default:
			snd_err("cannot support the channels:%u.\n", simple_channels);
			return -EINVAL;
		}
	}

	return 0;
}

static int sunxi_codec_prepare(struct snd_pcm_substream *substream, struct snd_dai *dai)
{
	struct snd_codec *codec = dai->component;

	snd_print("\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_codec_update_bits(codec, AC_DAC_TXFIFO_CTRL,
			1 << DAC_FIFO_FLUSH, 1 << DAC_FIFO_FLUSH);
		snd_codec_write(codec, AC_DAC_TXFIFO_STA,
			(1 << DAC_TXE_INT | 1 << DAC_TXU_INT | 1 << DAC_TXO_INT));
		snd_codec_write(codec, AC_DAC_TXCNT, 0);
	} else {
		snd_codec_update_bits(codec, AC_DAC_LBFIFO_CTRL,
			1 << DAC_LB_FIFO_FLUSH, 1 << DAC_LB_FIFO_FLUSH);
		snd_codec_write(codec, AC_DAC_LBFIFO_STA, 1 << DAC_LBA_INT | 1 << DAC_LBO_INT);
		snd_codec_write(codec, AC_DAC_LBCNT, 0);
	}

	return 0;
}

static int sunxi_codec_playback_event(struct snd_codec *codec, bool enable)
{
	struct sunxi_codec_info *sunxi_codec = codec->private_data;
	struct sunxi_codec_param *param = &sunxi_codec->param;

	if (enable) {

	/* digital DAC global enable */
	snd_codec_update_bits(codec, AC_DAC_DIG_CTRL, 0x1 << DAC_GEN, 0x1 << DAC_GEN);
	if (param->lineoutl_en)
		snd_codec_update_bits(codec, AC_DAC_DIG_CTRL, 0x1<<DACL_DIG_EN, 0x1<<DACL_DIG_EN);
	if (param->lineoutr_en)
		snd_codec_update_bits(codec, AC_DAC_DIG_CTRL, 0x1<<DACR_DIG_EN, 0x1<<DACR_DIG_EN);

	/* analog DAC enable */
	snd_codec_update_bits(codec, AC_DHP_ANA_CTRL,
			      1 << DHP_PGA_CHOPPER_EN, 1 << DHP_PGA_CHOPPER_EN);

	} else {

	if (param->lineoutl_en == false && param->lineoutr_en == false) {
		/* analog DAC enable */
		snd_codec_update_bits(codec, AC_DHP_ANA_CTRL,
				      1 << DHP_PGA_CHOPPER_EN, 0 << DHP_PGA_CHOPPER_EN);

		/* digital DAC global enable */
		snd_codec_update_bits(codec, AC_DAC_DIG_CTRL, 0x1 << DAC_GEN, 0x0 << DAC_GEN);
	}

	if (param->lineoutl_en)
		snd_codec_update_bits(codec, AC_DAC_DIG_CTRL, 0x1<<DACL_DIG_EN, 0x0<<DACL_DIG_EN);
	if (param->lineoutr_en)
		snd_codec_update_bits(codec, AC_DAC_DIG_CTRL, 0x1<<DACR_DIG_EN, 0x0<<DACR_DIG_EN);

	}

	return 0;
}

static int sunxi_codec_lienoutl_event(struct snd_codec *codec, bool enable)
{
	if (enable) {
		snd_codec_update_bits(codec, AC_DHP_ANA_CTRL, 0x1 << DHPL_EN, 0x1 << DHPL_EN);
		snd_codec_update_bits(codec, AC_DAC_ANA_CTRL, 0x1 << DACL_EN, 0x1 << DACL_EN);
	} else {
		snd_codec_update_bits(codec, AC_DHP_ANA_CTRL, 0x1 << DHPL_EN, 0x0 << DHPL_EN);
		snd_codec_update_bits(codec, AC_DAC_ANA_CTRL, 0x1 << DACL_EN, 0x0 << DACL_EN);
	}

	return 0;
}

static int sunxi_codec_lienoutr_event(struct snd_codec *codec, bool enable)
{
	if (enable) {
		snd_codec_update_bits(codec, AC_DHP_ANA_CTRL, 0x1 << DHPR_EN, 0x1 << DHPR_EN);
		snd_codec_update_bits(codec, AC_DAC_ANA_CTRL, 0x1 << DACR_EN, 0x1 << DACR_EN);
	} else {
		snd_codec_update_bits(codec, AC_DHP_ANA_CTRL, 0x1 << DHPR_EN, 0x0 << DHPR_EN);
		snd_codec_update_bits(codec, AC_DAC_ANA_CTRL, 0x1 << DACR_EN, 0x0 << DACR_EN);
	}

	return 0;
}

static int sunxi_codec_dapm_control(struct snd_pcm_substream *substream, struct snd_dai *dai, int onoff)
{
	struct snd_codec *codec = dai->component;
	struct sunxi_codec_info *sunxi_codec = codec->private_data;
	struct sunxi_codec_param *param = &sunxi_codec->param;
	struct sunxi_pa_config *pa_cfg = &sunxi_codec->pa_cfg;
	int ret;

	snd_print("\n");

	if (substream->dapm_state == onoff)
		return 0;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		/*
		 * Playback:
		 * Playback --> DAC1 -->  LINEOUT1 --> External Speaker
		 * Playback --> DAC2 -->  LINEOUT2 --> External Speaker
		 */
		param->playback_dapm = onoff;
		if (onoff) {
			if (param->lineoutl_en)
				sunxi_codec_lienoutl_event(codec, true);
			if (param->lineoutr_en)
				sunxi_codec_lienoutr_event(codec, true);
			sunxi_codec_playback_event(codec, true);

			ret = snd_sunxi_pa_enable(pa_cfg);
			if (ret) {
				snd_err("pa pin enable failed!\n");
			}
		} else {
			snd_sunxi_pa_disable(pa_cfg);

			sunxi_codec_playback_event(codec, false);
			if (param->lineoutl_en)
				sunxi_codec_lienoutl_event(codec, false);
			if (param->lineoutr_en)
				sunxi_codec_lienoutr_event(codec, false);
		}
	} else {
		/* null */
	}
	substream->dapm_state = onoff;
	return 0;
}

static int sunxi_codec_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_dai *dai)
{
	struct snd_codec *codec = dai->component;

	snd_print("\n");

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_codec_update_bits(codec, AC_DAC_TXFIFO_CTRL,
				1 << DAC_DRQ_EN, 1 << DAC_DRQ_EN);
		}
		else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			snd_codec_update_bits(codec, AC_DAC_LBFIFO_CTRL,
				1 << DAC_LB_DRQ_EN, 1 << DAC_LB_DRQ_EN);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_codec_update_bits(codec, AC_DAC_TXFIFO_CTRL,
				1 << DAC_DRQ_EN, 0 << DAC_DRQ_EN);
		}
		else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			snd_codec_update_bits(codec, AC_DAC_LBFIFO_CTRL,
				1 << DAC_LB_DRQ_EN, 0 << DAC_LB_DRQ_EN);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static struct snd_dai_ops sun20iw2_codec_dai_ops = {
	.set_sysclk	= sunxi_codec_set_sysclk,
	.startup	= sunxi_codec_startup,
	.shutdown	= sunxi_codec_shutdown,
	.hw_params	= sunxi_codec_hw_params,
	.prepare	= sunxi_codec_prepare,
	.trigger	= sunxi_codec_trigger,
	.dapm_control   = sunxi_codec_dapm_control,
};

static struct snd_dai sun20iw2_codec_dai[] = {
	{
		.name	= "sun20iw2codec-dac",
		.playback = {
			.stream_name	= "Playback",
			.channels_min	= 1,
			.channels_max	= 2,
			.rates		= SNDRV_PCM_RATE_8000_192000
					| SNDRV_PCM_RATE_KNOT,
			.formats	= SNDRV_PCM_FMTBIT_S16_LE
					| SNDRV_PCM_FMTBIT_S24_LE,
			.rate_min	= 8000,
			.rate_max	= 192000,
		},
		.capture = {
			.stream_name	= "Capture",
			.channels_min	= 1,
			.channels_max	= 2,
			.rates		= SNDRV_PCM_RATE_8000_192000
					| SNDRV_PCM_RATE_KNOT,
			.formats	= SNDRV_PCM_FMTBIT_S16_LE
					| SNDRV_PCM_FMTBIT_S24_LE,
			.rate_min	= 8000,
			.rate_max	= 192000,
		},
		.ops = &sun20iw2_codec_dai_ops,
	},
};

static int sunxi_set_lineout_ch(struct snd_kcontrol *kcontrol, unsigned long value)
{
	snd_print("\n");

	if (kcontrol->type != SND_CTL_ELEM_TYPE_ENUMERATED) {
		snd_err("invalid kcontrol type = %d.\n", kcontrol->type);
		return -EINVAL;
	}

	if (value >= kcontrol->items) {
		snd_err("invalid kcontrol items = %ld.\n", value);
		return -EINVAL;
	}

	if (kcontrol->private_data_type == SND_MODULE_CODEC) {
		struct snd_codec *codec = kcontrol->private_data;
		struct sunxi_codec_info *sunxi_codec = codec->private_data;
		struct sunxi_codec_param *codec_param = &sunxi_codec->param;

		if (kcontrol->mask == SND_CTL_ENUM_LINEOUTL_MASK) {
			codec_param->lineoutl_en = value;
			if (codec_param->playback_dapm)
				sunxi_codec_lienoutl_event(codec, value);
		} else if (kcontrol->mask == SND_CTL_ENUM_LINEOUTR_MASK) {
			codec_param->lineoutr_en = value;
			if (codec_param->playback_dapm)
				sunxi_codec_lienoutr_event(codec, value);
		}
	} else {
		snd_err("invalid kcontrol data type = %d.\n", kcontrol->private_data_type);
	}
	snd_info("mask:0x%x, items:%d, value:0x%x\n", kcontrol->mask, kcontrol->items, value);

	return 0;
}

static int sunxi_get_lineout_ch(struct snd_kcontrol *kcontrol, struct snd_ctl_info *info)
{
	unsigned int val = 0;

	snd_print("\n");

	if (kcontrol->type != SND_CTL_ELEM_TYPE_ENUMERATED) {
		snd_err("invalid kcontrol type = %d.\n", kcontrol->type);
		return -EINVAL;
	}

	if (kcontrol->private_data_type == SND_MODULE_CODEC) {
		struct snd_codec *codec = kcontrol->private_data;
		struct sunxi_codec_info *sunxi_codec = codec->private_data;
		struct sunxi_codec_param *codec_param = &sunxi_codec->param;

		if (kcontrol->mask == SND_CTL_ENUM_LINEOUTL_MASK)
			val = codec_param->lineoutl_en << 0;
		else if (kcontrol->mask == SND_CTL_ENUM_LINEOUTR_MASK)
			val = codec_param->lineoutr_en << 1;
	} else {
		snd_err("%s invalid kcontrol data type = %d.\n", __func__,
			kcontrol->private_data_type);
	}

	snd_kcontrol_to_snd_ctl_info(kcontrol, info, val);

	return 0;
}

static int sunxi_set_lineout_volume(struct snd_kcontrol *kcontrol, unsigned long value)
{
	struct snd_codec *codec = kcontrol->private_data;

	snd_print("\n");

	if (kcontrol->type != SND_CTL_ELEM_TYPE_INTEGER) {
		snd_err("invalid kcontrol type = %d.\n", kcontrol->type);
		return -EINVAL;
	}

	if (value > kcontrol->max) {
		snd_err("invalid kcontrol value = %ld.\n", value);
		return -EINVAL;
	}

	if (kcontrol->private_data_type == SND_MODULE_CODEC) {
		value = kcontrol->max - value;
		snd_codec_update_bits(codec, AC_DAC_DPH_GAIN,
				      0x7 << DHP_OUTPUT_GAIN, value << DHP_OUTPUT_GAIN);
	} else {
		snd_err("invalid kcontrol data type = %d.\n", kcontrol->private_data_type);
	}
	snd_info("mask:0x%x, items:%d, value:0x%x\n", kcontrol->mask, kcontrol->items, value);

	return 0;
}

static int sunxi_get_lineout_volume(struct snd_kcontrol *kcontrol, struct snd_ctl_info *info)
{
	unsigned int val = 0;
	unsigned int reg_val;
	struct snd_codec *codec = kcontrol->private_data;

	snd_print("\n");

	if (kcontrol->type != SND_CTL_ELEM_TYPE_INTEGER) {
		snd_err("invalid kcontrol type = %d.\n", kcontrol->type);
		return -EINVAL;
	}

	if (kcontrol->private_data_type == SND_MODULE_CODEC) {
		reg_val = snd_codec_read(codec, AC_DAC_DPH_GAIN);
		val = kcontrol->max - ((reg_val >> DHP_OUTPUT_GAIN) & kcontrol->max);
	} else {
		snd_err("%s invalid kcontrol data type = %d.\n", __func__,
			kcontrol->private_data_type);
	}

	snd_kcontrol_to_snd_ctl_info(kcontrol, info, val);

	return 0;
}

static struct snd_kcontrol sunxi_codec_controls[] = {
	/* -64dB to 63dB, 0.5dB/step, if regvol==0 will mute */
	SND_CTL_KCONTROL("DACL dig volume", AC_DAC_DIG_VOL, DACL_DIG_VOL, 0xff),
	SND_CTL_KCONTROL("DACR dig volume", AC_DAC_DIG_VOL, DACR_DIG_VOL, 0xff),
	SND_CTL_KCONTROL_EXT("LINEOUT volume", 0x7, 0x0,
			     sunxi_get_lineout_volume, sunxi_set_lineout_volume),
	/* line out switch */
	SND_CTL_ENUM_EXT("LINEOUTL switch",
			 ARRAY_SIZE(codec_switch_onoff), codec_switch_onoff,
			 SND_CTL_ENUM_LINEOUTL_MASK,
			 sunxi_get_lineout_ch, sunxi_set_lineout_ch),
	SND_CTL_ENUM_EXT("LINEOUTR switch",
			 ARRAY_SIZE(codec_switch_onoff), codec_switch_onoff,
			 SND_CTL_ENUM_LINEOUTR_MASK,
			 sunxi_get_lineout_ch, sunxi_set_lineout_ch),
};

static void sunxi_codec_init(struct snd_codec *codec)
{
	struct sunxi_codec_info *sunxi_codec = codec->private_data;
	struct sunxi_codec_param *param = &sunxi_codec->param;

	snd_print("\n");

	/* *** ANA PATH *** */
	snd_codec_write(codec, AC_POWER_CTRL, 0x801c382a);	/* ldo calibration */
	snd_codec_write(codec, AC_DHP_ANA_CTRL, 0x86c);		/* DHP params setting */

	/* *** DAC PATH *** */
	/* DAC data from TX mixer */
	snd_codec_update_bits(codec, AC_DAC_DIG_CTRL, 0x3<<DAC_PTN_SEL, 0x0<<DAC_PTN_SEL);

	/* volume set */
	snd_codec_update_bits(codec, AC_DAC_DIG_VOL,
			      0xff << DACL_DIG_VOL, param->dacl_vol << DACL_DIG_VOL);
	snd_codec_update_bits(codec, AC_DAC_DIG_VOL,
			      0xff << DACR_DIG_VOL, param->dacr_vol << DACR_DIG_VOL);
	snd_codec_update_bits(codec, AC_DAC_DPH_GAIN,
			      0x7 << DHP_OUTPUT_GAIN,
			      (0x7 - param->lineout_vol) << DHP_OUTPUT_GAIN);
}

static int snd_sunxi_clk_enable(struct sunxi_codec_clk *clk)
{
	int ret;

	snd_print("\n");

	/* rst & bus */
	ret = hal_reset_control_deassert(clk->clk_rst_dac);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("codec clk_rst_dac clk_deassert failed.\n");
		goto err_deassert_clk_rst_dac;
	}
	ret = hal_clock_enable(clk->clk_bus_dac);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("codec clk_bus_dac enable failed.\n");
		goto err_enable_clk_bus_dac;
	}

#ifndef CONFIG_SND_CODEC_AUDIOCODEC_DAC_WITHOUT_ADC_CLK
	ret = hal_reset_control_deassert(clk->clk_rst_adc);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("codec clk_rst_adc clk_deassert failed.\n");
		goto err_deassert_clk_rst_adc;
	}
	ret = hal_clock_enable(clk->clk_bus_adc);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("codec clk_bus_adc enable failed.\n");
		goto err_enable_clk_bus_adc;
	}
#endif

	/* pll */
	ret = hal_clock_enable(clk->clk_pll_audio);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("codec clk_pll_audio enable failed.\n");
		goto err_enable_clk_pll_audio;
	}

	ret = hal_clock_enable(clk->clk_ck1_aud_div);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("codec clk_ck1_aud_div enable failed.\n");
		goto err_enable_clk_ck1_aud_div;
	}

	ret = hal_clock_enable(clk->clk_dac);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("codec clk_dac enable failed.\n");
		goto err_enable_clk_dac;
	}

#ifndef CONFIG_SND_CODEC_AUDIOCODEC_DAC_WITHOUT_ADC_CLK
	ret = hal_clock_enable(clk->clk_adc_gate);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("codec clk_adc_gate enable failed.\n");
		goto err_enable_clk_adc_gate;
	}
#endif

	return HAL_CLK_STATUS_OK;

#ifndef CONFIG_SND_CODEC_AUDIOCODEC_DAC_WITHOUT_ADC_CLK
err_enable_clk_adc_gate:
	hal_clock_disable(clk->clk_ck1_aud_div);
#endif
err_enable_clk_dac:
	hal_clock_disable(clk->clk_ck1_aud_div);
err_enable_clk_ck1_aud_div:
	hal_clock_disable(clk->clk_pll_audio);
err_enable_clk_pll_audio:
#ifndef CONFIG_SND_CODEC_AUDIOCODEC_DAC_WITHOUT_ADC_CLK
	hal_clock_disable(clk->clk_bus_adc);
err_enable_clk_bus_adc:
	hal_reset_control_assert(clk->clk_rst_adc);
err_deassert_clk_rst_adc:
#endif
	hal_clock_disable(clk->clk_bus_dac);
err_enable_clk_bus_dac:
	hal_reset_control_assert(clk->clk_rst_dac);
err_deassert_clk_rst_dac:
	return HAL_CLK_STATUS_ERROR;
}

static void snd_sunxi_clk_disable(struct sunxi_codec_clk *clk)
{
	snd_print("\n");

#ifndef CONFIG_SND_CODEC_AUDIOCODEC_DAC_WITHOUT_ADC_CLK
	hal_clock_disable(clk->clk_adc_gate);
	hal_clock_disable(clk->clk_bus_adc);
	hal_reset_control_assert(clk->clk_rst_adc);
#endif
	hal_clock_disable(clk->clk_dac);
	hal_clock_disable(clk->clk_bus_dac);
	hal_reset_control_assert(clk->clk_rst_dac);
	hal_clock_disable(clk->clk_ck1_aud_div);
	hal_clock_disable(clk->clk_pll_audio);

	return;
}

static int snd_sunxi_clk_init(struct sunxi_codec_clk *clk)
{
	int ret;

	snd_print("\n");

	/* rst & bus */
	clk->clk_rst_dac = hal_reset_control_get(HAL_SUNXI_AON_RESET, RST_CODEC_DAC);
	if (!clk->clk_rst_dac) {
		snd_err("codec clk_rst_dac hal_reset_control_get failed\n");
		goto err_get_clk_rst_dac;
	}
	clk->clk_bus_dac = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_BUS_CODEC_DAC);
	if (!clk->clk_bus_dac) {
		snd_err("codec clk_bus_dac hal_clock_get failed\n");
		goto err_get_clk_bus_dac;
	}

#ifndef CONFIG_SND_CODEC_AUDIOCODEC_DAC_WITHOUT_ADC_CLK
	/* enable adc clk to enable analog domain */
	clk->clk_rst_adc = hal_reset_control_get(HAL_SUNXI_AON_RESET, RST_CODEC_ADC);
	if (!clk->clk_rst_adc) {
		snd_err("codec clk_rst_adc hal_reset_control_get failed\n");
		goto err_get_clk_rst_adc;
	}
	clk->clk_bus_adc = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_BUS_CODEC_ADC);
	if (!clk->clk_bus_adc) {
		snd_err("codec clk_bus_adc hal_clock_get failed\n");
		goto err_get_clk_bus_adc;
	}
#endif
	/* pll clk -> 24.576M */
	clk->clk_pll_audio = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_PLL_AUDIO);
	if (!clk->clk_pll_audio) {
		snd_err("codec clk_pll_audio hal_clock_get failed\n");
		goto err_get_clk_pll_audio;
	}
	clk->clk_pll_audio1x = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_PLL_AUDIO1X);
	if (!clk->clk_pll_audio1x) {
		snd_err("codec clk_pll_audio1x hal_clock_get failed\n");
		goto err_get_clk_pll_audio1x;
	}
	clk->clk_pll_audio2x = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_PLL_AUDIO2X);
	if (!clk->clk_pll_audio2x) {
		snd_err("codec clk_pll_audio2x hal_clock_get failed\n");
		goto err_get_clk_pll_audio2x;
	}
	clk->clk_hosc = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_HOSC);
	if (!clk->clk_hosc) {
		snd_err("codec clk_hosc hal_clock_get failed\n");
		goto err_get_clk_hosc;
	}

	clk->clk_audpll_hosc_sel = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_AUDPLL_HOSC_SEL);
	if (!clk->clk_audpll_hosc_sel) {
		snd_err("codec clk_audpll_hosc_sel hal_clock_get failed\n");
		goto err_get_clk_audpll_hosc_sel;
	}

	/* pll clk -> 22.5792M */
	clk->clk_ck1_aud_div = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_CK1_AUD_DIV);
	if (!clk->clk_ck1_aud_div) {
		snd_err("codec clk_ck1_aud_div hal_clock_get failed\n");
		goto err_get_clk_ck1_aud_div;
	}

	/* pll clk -> 8.192M */
	clk->clk_aud_rco_div = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_AUD_RCO_DIV);
	if (!clk->clk_aud_rco_div) {
		snd_err("codec clk_aud_rco_div hal_clock_get failed\n");
		goto err_get_clk_aud_rco_div;
	}

	/* module dac clk */
	clk->clk_dac = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_CODEC_DAC);
	if (!clk->clk_dac) {
		snd_err("codec clk_dac hal_clock_get failed\n");
		goto err_get_clk_dac;
	}

#ifndef CONFIG_SND_CODEC_AUDIOCODEC_DAC_WITHOUT_ADC_CLK
	/* module adc clk */
	clk->clk_adc_div = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_CODEC_ADC_DIV);
	if (!clk->clk_adc_div) {
		snd_err("codec clk_adc_div hal_clock_get failed\n");
		goto err_get_clk_adc_div;
	}
	clk->clk_adc_sel1 = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_CODEC_ADC_SEL1);
	if (!clk->clk_adc_sel1) {
		snd_err("codec clk_adc_sel1 hal_clock_get failed\n");
		goto err_get_clk_adc_sel1;
	}
	clk->clk_adc_gate = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_CODEC_ADC_GATE);
	if (!clk->clk_adc_gate) {
		snd_err("codec clk_adc_gate hal_clock_get failed\n");
		goto err_get_clk_adc_gate;
	}
#endif
	/* pll tree: (24.576MHz)
	 * CLK_PLL_AUDIO --> CLK_PLL_AUDIO1X --> CLK_AUDPLL_HOSC_SEL --> CLK_CODEC_DAC
	 *
	 * pll tree: (22.5792MHz)
	 * CLK_DPLL1 --> CLK_CK1_AUD --> CLK_CK1_AUD_DIV --> CLK_CODEC_DAC
	 */
	ret = hal_clk_set_parent(clk->clk_audpll_hosc_sel, clk->clk_pll_audio1x);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("codec clk_pll_audio1x -> clk_audpll_hosc_sel clk_set_parent failed.\n");
		goto err_set_parent_clk;
	}

#ifndef CONFIG_SND_CODEC_AUDIOCODEC_DAC_WITHOUT_ADC_CLK
	ret = hal_clk_set_parent(clk->clk_adc_div, clk->clk_audpll_hosc_sel);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("codec clk_audpll_hosc_sel -> clk_adc_div clk_set_parent failed.\n");
		goto err_set_parent_clk;
	}

	ret = hal_clk_set_parent(clk->clk_adc_sel1, clk->clk_adc_div);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("codec clk_adc_div -> clk_adc_sel1 clk_set_parent failed.\n");
		goto err_set_parent_clk;
	}
#endif
	ret = hal_clk_set_parent(clk->clk_dac, clk->clk_audpll_hosc_sel);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("codec clk_audpll_hosc_sel -> clk_dac clk_set_parent failed.\n");
		goto err_set_parent_clk;
	}

	/* note: Enable and then set the freq to avoid clock lock errors */
	ret = snd_sunxi_clk_enable(clk);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("codec snd_sunxi_clk_enable failed.\n");
		goto err_clk_enable;
	}

	/* pll div limit */
	ret = hal_clk_set_rate(clk->clk_pll_audio, 98333333);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("set clk_pll_audio rate failed\n");
		goto err_set_rate_clk;
	}
	ret = hal_clk_set_rate(clk->clk_ck1_aud_div, 22588236);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("set clk_ck1_aud_div rate failed\n");
		goto err_set_rate_clk;
	}

	return HAL_CLK_STATUS_OK;

err_clk_enable:
err_set_rate_clk:
err_set_parent_clk:
#ifndef CONFIG_SND_CODEC_AUDIOCODEC_DAC_WITHOUT_ADC_CLK
	hal_clock_put(clk->clk_adc_gate);
err_get_clk_adc_gate:
	hal_clock_put(clk->clk_adc_sel1);
err_get_clk_adc_sel1:
	hal_clock_put(clk->clk_adc_div);
err_get_clk_adc_div:
#endif
	hal_clock_put(clk->clk_dac);
err_get_clk_dac:
	hal_clock_put(clk->clk_aud_rco_div);
err_get_clk_aud_rco_div:
	hal_clock_put(clk->clk_ck1_aud_div);
err_get_clk_ck1_aud_div:
	hal_clock_put(clk->clk_audpll_hosc_sel);
err_get_clk_audpll_hosc_sel:
	hal_clock_put(clk->clk_hosc);
err_get_clk_hosc:
	hal_clock_put(clk->clk_pll_audio2x);
err_get_clk_pll_audio2x:
	hal_clock_put(clk->clk_pll_audio1x);
err_get_clk_pll_audio1x:
	hal_clock_put(clk->clk_pll_audio);
err_get_clk_pll_audio:
#ifndef CONFIG_SND_CODEC_AUDIOCODEC_DAC_WITHOUT_ADC_CLK
	hal_clock_put(clk->clk_bus_adc);
err_get_clk_bus_adc:
	hal_reset_control_put(clk->clk_rst_adc);
err_get_clk_rst_adc:
#endif
	hal_clock_put(clk->clk_bus_dac);
err_get_clk_bus_dac:
	hal_reset_control_put(clk->clk_rst_dac);
err_get_clk_rst_dac:
	return HAL_CLK_STATUS_ERROR;
}

static void snd_sunxi_clk_exit(struct sunxi_codec_clk *clk)
{
	snd_print("\n");

	snd_sunxi_clk_disable(clk);
#ifndef CONFIG_SND_CODEC_AUDIOCODEC_DAC_WITHOUT_ADC_CLK
	hal_clock_put(clk->clk_adc_gate);
	hal_clock_put(clk->clk_adc_sel1);
	hal_clock_put(clk->clk_adc_div);
#endif
	hal_clock_put(clk->clk_dac);
	hal_clock_put(clk->clk_aud_rco_div);
	hal_clock_put(clk->clk_ck1_aud_div);
	hal_clock_put(clk->clk_audpll_hosc_sel);
	hal_clock_put(clk->clk_hosc);
	hal_clock_put(clk->clk_pll_audio2x);
	hal_clock_put(clk->clk_pll_audio1x);
#ifndef CONFIG_SND_CODEC_AUDIOCODEC_DAC_WITHOUT_ADC_CLK
	hal_clock_put(clk->clk_bus_adc);
	hal_reset_control_put(clk->clk_rst_adc);
#endif
	hal_clock_put(clk->clk_bus_dac);
	hal_reset_control_put(clk->clk_rst_dac);

	return;
}

static int snd_sunxi_clk_set_rate(struct sunxi_codec_clk *clk, int stream,
				  unsigned int freq_in, unsigned int freq_out)
{
	int ret;
	unsigned int freq;
	hal_clk_t clk_codec;

	snd_print("freq_out -> %u\n", freq_out);

	(void)freq_in;

	clk_codec = clk->clk_dac;

	if (freq_out == 24576000) {
		freq = 24583333;
		ret = hal_clk_set_parent(clk_codec, clk->clk_audpll_hosc_sel);
		if (ret != HAL_CLK_STATUS_OK) {
			snd_err("codec clk_audpll_hosc_sel -> clk_codec clk_set_parent failed.\n");
			return HAL_CLK_STATUS_ERROR;
		}
	} else {
		freq = 22588236;
		ret = hal_clk_set_parent(clk_codec, clk->clk_ck1_aud_div);
		if (ret != HAL_CLK_STATUS_OK) {
			snd_err("codec clk_ck1_aud_div -> clk_codec clk_set_parent failed.\n");
			return HAL_CLK_STATUS_ERROR;
		}
	}

	/* note: CLK_CODEC_DAC no need set rate */

	return HAL_CLK_STATUS_OK;
}

static void snd_sunxi_params_init(struct sunxi_codec_param *params)
{
#ifdef CONFIG_DRIVER_SYSCONFIG
	int ret;
	int32_t val;

	ret = hal_cfg_get_keyvalue(CODEC, "dacl_vol", &val, 1);
	if (ret) {
		snd_err("%s: dacl_vol miss.\n", CODEC);
		params->dacl_vol = default_param.dacl_vol;
	} else
		params->dacl_vol = val;

	ret = hal_cfg_get_keyvalue(CODEC, "dacr_vol", &val, 1);
	if (ret) {
		snd_err("%s: dacr_vol miss.\n", CODEC);
		params->dacr_vol = default_param.dacr_vol;
	} else
		params->dacr_vol = val;

	ret = hal_cfg_get_keyvalue(CODEC, "lineout_vol", &val, 1);
	if (ret) {
		snd_err("%s: lineout_vol miss.\n", CODEC);
		params->lineout_vol = default_param.lineout_vol;
	} else
		params->lineout_vol = val;

	ret = hal_cfg_get_keyvalue(CODEC, "lineoutl_en", &val, 1);
	if (ret) {
		snd_err("%s: lineoutl_en miss.\n", CODEC);
		params->lineoutl_en = default_param.lineoutl_en;
	} else
		params->lineoutl_en = val;

	ret = hal_cfg_get_keyvalue(CODEC, "lineoutr_en", &val, 1);
	if (ret) {
		snd_err("%s: lineoutr_en miss.\n", CODEC);
		params->lineoutr_en = default_param.lineoutr_en;
	} else
		params->lineoutr_en = val;

	ret = hal_cfg_get_keyvalue(CODEC, "playback_cma", &val, 1);
	if (ret)
		snd_print("%s: playback_cma miss.\n", CODEC);
	else
		sun20iw2_hardware[0].buffer_bytes_max = val;
#else
	*params = default_param;
#endif
}

/* suspend and resume */
#ifdef CONFIG_COMPONENTS_PM
static unsigned int snd_read_func(void *data, unsigned int reg)
{
	struct snd_codec *codec;

	if (!data) {
		snd_err("data is invailed\n");
		return 0;
	}

	codec = data;
	return snd_codec_read(codec, reg);
}

static void snd_write_func(void *data, unsigned int reg, unsigned int val)
{
	struct snd_codec *codec;

	if (!data) {
		snd_err("data is invailed\n");
		return;
	}

	codec = data;
	snd_codec_write(codec, reg, val);
}

static int sunxi_codec_prepared(struct pm_device *dev, suspend_mode_t mode) {
	struct snd_codec *codec;

	if (!dev->data) {
		snd_err("data is invailed\n");
		return 0;
	}
	codec = dev->data;

	snd_sunxi_save_reg(sunxi_reg_labels, (void *)codec, snd_read_func);

	return 0;
}

static int sunxi_codec_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	struct snd_codec *codec = dev->data;
	struct sunxi_codec_info *sunxi_codec = codec->private_data;

	snd_print("\n");

	snd_sunxi_pa_disable(&sunxi_codec->pa_cfg);
#ifdef CONFIG_SND_CODEC_AUDIOCODEC_DAC_KEEP_AUDIO_PLL_ON
	/* set clk enable_count++ to ensure audio pll always on */
	hal_clock_enable(sunxi_codec->clk.clk_pll_audio);
#endif
	snd_sunxi_clk_disable(&sunxi_codec->clk);

	return 0;
}

static int sunxi_codec_resume(struct pm_device *dev, suspend_mode_t mode)
{
	struct snd_codec *codec = dev->data;
	struct sunxi_codec_info *sunxi_codec = codec->private_data;

	snd_print("\n");

	snd_sunxi_pa_disable(&sunxi_codec->pa_cfg);
	snd_sunxi_clk_enable(&sunxi_codec->clk);
	sunxi_codec_init(codec);
	snd_sunxi_echo_reg(sunxi_reg_labels, (void *)codec, snd_write_func);
#ifdef CONFIG_SND_CODEC_AUDIOCODEC_DAC_KEEP_AUDIO_PLL_ON
	/* restore clk enable_count */
	hal_clock_disable(sunxi_codec->clk.clk_pll_audio);
#endif

	return 0;
}

struct pm_devops pm_audiocodec_dac_ops = {
	.prepared = sunxi_codec_prepared,
	.suspend = sunxi_codec_suspend,
	.resume = sunxi_codec_resume,
};

struct pm_device pm_audiocodec_dac = {
	.name = "audiocodecdac",
	.ops = &pm_audiocodec_dac_ops,
};
#endif

/* audiocodec probe */
static int sun20iw2_codec_probe(struct snd_codec *codec)
{
	struct sunxi_codec_info *sunxi_codec = NULL;
	hal_clk_status_t ret;
	struct alg_cfg_reg_domain domain_meq = {
		.reg_base = (void *)SUNXI_CODEC_BASE_ADDR,
		.reg_min = MAIN_EQ_REG_BEGAIN,
		.reg_max = MAIN_EQ_REG_END,
	};
	struct alg_cfg_reg_domain domain_peq = {
		.reg_base = (void *)SUNXI_CODEC_BASE_ADDR,
		.reg_min = POST_EQ_REG_BEGAIN,
		.reg_max = POST_EQ_REG_END,
	};
	struct alg_cfg_reg_domain domain_1bdrc = {
		.reg_base = (void *)SUNXI_CODEC_BASE_ADDR,
		.reg_min = DRC_1B_REG_BEGAIN,
		.reg_max = DRC_1B_REG_END,
	};
	struct alg_cfg_reg_domain domain_3bdrc = {
		.reg_base = (void *)SUNXI_CODEC_BASE_ADDR,
		.reg_min = DRC_3B_REG_BEGAIN,
		.reg_max = DRC_3B_REG_END,
	};

	snd_print("\n");


	if (!codec->codec_dai)
		return -1;

	sunxi_codec = snd_malloc(sizeof(struct sunxi_codec_info));
	if (!sunxi_codec) {
		snd_err("no memory\n");
		return -ENOMEM;
	}

	codec->private_data = (void *)sunxi_codec;

	snd_sunxi_params_init(&sunxi_codec->param);
	snd_sunxi_pa_init(&sunxi_codec->pa_cfg, &default_pa_cfg, CODEC);
	snd_sunxi_pa_disable(&sunxi_codec->pa_cfg);

	codec->codec_base_addr = (void *)SUNXI_CODEC_BASE_ADDR;
	codec->codec_dai->component = codec;

	ret = snd_sunxi_clk_init(&sunxi_codec->clk);
	if (ret != 0) {
		snd_err("snd_sunxi_clk_init failed\n");
		goto err_codec_set_clock;
	}

	sunxi_codec_init(codec);

	/* audio hardware algorithm config reg domain set */
	ret = 0;
	ret += sunxi_alg_cfg_domain_set(&domain_meq, SUNXI_ALG_CFG_DOMAIN_MEQ);
	ret += sunxi_alg_cfg_domain_set(&domain_peq, SUNXI_ALG_CFG_DOMAIN_PEQ);
	ret += sunxi_alg_cfg_domain_set(&domain_1bdrc, SUNXI_ALG_CFG_DOMAIN_1BDRC);
	ret += sunxi_alg_cfg_domain_set(&domain_3bdrc, SUNXI_ALG_CFG_DOMAIN_3BDRC);
	if (ret) {
		printf("alg_cfg domain set failed\n");
		goto err;
	}

#ifdef CONFIG_COMPONENTS_PM
	pm_audiocodec_dac.data = (void *)codec;
	ret = pm_devops_register(&pm_audiocodec_dac);
	if (ret) {
		snd_err("pm_devops_register failed\n");
	}
#endif

	return 0;

err:
err_codec_set_clock:
	snd_sunxi_clk_exit(&sunxi_codec->clk);

	return -1;
}

static int sun20iw2_codec_remove(struct snd_codec *codec)
{
	struct sunxi_codec_info *sunxi_codec = codec->private_data;

	snd_print("\n");

	snd_sunxi_clk_exit(&sunxi_codec->clk);

	snd_free(sunxi_codec);
	codec->private_data = NULL;

	return 0;
}



struct snd_codec sunxi_audiocodec_dac = {
	.name		= "audiocodecdac",
	.codec_dai	= sun20iw2_codec_dai,
	.codec_dai_num  = ARRAY_SIZE(sun20iw2_codec_dai),
	.private_data	= NULL,
	.probe		= sun20iw2_codec_probe,
	.remove		= sun20iw2_codec_remove,
	.controls       = sunxi_codec_controls,
	.num_controls   = ARRAY_SIZE(sunxi_codec_controls),
	.hw 		= sun20iw2_hardware,
};
