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
#define TAG	"AP-Resample"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "audio_plugin.h"
#include "AudioBase.h"

#include "speex_resampler.h"

struct resample_data {
	struct as_pcm_config *src_config;
	struct as_pcm_config *dst_config;

	void *src_buf;
	uint32_t src_buf_size;

	SpeexResamplerState *st;
	int quality;
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_PLUGIN_RESAMPLE_ADJUST
	int adjust_onoff;
#endif
};


#define SRC_DEFAULT_TYPE	(SRC_SPEEX)
static int resample_ap_init(struct audio_plugin *ap)
{
	struct resample_data *rd;

	if (ap->private_data != NULL)
		return 0;

	rd = as_alloc(sizeof(struct resample_data));
	if (!rd)
		fatal("no memory");

	ap->private_data = rd;

	rd->quality = 3;

	return 0;

}

#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_PLUGIN_RESAMPLE_ADJUST
static int resample_ap_setup(struct audio_plugin *ap, void *para, uint32_t size)
{
	struct resample_data *rd = NULL;
	int *onoff;

	if (size != sizeof(int))
		return -1;

	if (!ap->private_data)
		resample_ap_init(ap);
	rd = ap->private_data;

	onoff = (int *)para;
	rd->adjust_onoff = *onoff;

	return 0;
}

static int check_resample_adjust_onoff(struct audio_plugin *ap)
{
	struct resample_data *rd;

	rd = ap->private_data;
	if (rd && rd->adjust_onoff)
		return rd->adjust_onoff;
	return -1;
}

/*
 * dir: 0:playback; 1:capture
 * playback: in_rate --> out_rate, out_rate should adjust(larger)
 * capture: in_rate --> out_rate, out_rate should adjust(larger)
 */
static void get_input_output_rate(struct resample_data *rd, uint32_t *in, uint32_t *out, int dir)
{
	uint32_t out_rate = 48000;
	uint32_t in_rate = 48000;
	uint32_t den = 0;
	uint32_t den_irate = in_rate;
	uint32_t den_orate = out_rate;

	/* output rate */
	switch (rd->dst_config->rate) {
		case 96000:
		case 48000:
		case 16000:
		case 8000:
			den_orate = rd->dst_config->rate;
			break;
		case 88200:
			den_orate = 96000;
			break;
		case 44100:
			den_orate = 48000;
			break;
		case 22050:
			den_orate = 24000;
			break;
		default:
			printf("unknown rate:%u\n", rd->dst_config->rate);
			break;
	}
	switch (rd->dst_config->rate) {
		case 96000:
		case 48000:
		case 16000:
		case 8000:
			/*out_rate =  24583333;*/
			/* 3ms init time, 260frames=5.4ms/h lack */
			if (dir)
				out_rate =  191942;
			else
				out_rate =  192057;
			break;
		case 88200:
		case 44100:
		case 22050:
			if (dir)
				out_rate =  176179;
			else
				out_rate =  176470;
			break;
		default:
			break;
	}

	/* input rate */
	switch (rd->src_config->rate) {
		case 96000:
		case 48000:
		case 16000:
		case 8000:
			/*in_rate =  24576000;*/
			/* 3ms init time, 260frames=5.4ms/h lack */
			in_rate =  192000;
			den_irate = rd->src_config->rate;
			/*in_rate =  48000;*/
			break;
		case 88200:
			den_irate = 96000;
			/*in_rate =  22579200;*/
			in_rate =  176400;
			/*in_rate =  44100;*/
			break;
		case 44100:
			den_irate = 48000;
			in_rate =  176400;
			break;
		case 22050:
			den_irate = 24000;
			in_rate =  176400;
			break;
		default:
			printf("unknown rate:%u\n", rd->src_config->rate);
			break;
	}
	/*
	 *  dst_rate:out_rate   src_rate:in_rate
	 *  96000:192057        96000:192000  192000/1
	 *  96000:192057        48000:96000   192000/2
	 *  96000:192057        16000:32000   192000/6
	 *  96000:192057        8000:16000    192000/12
	 *
	 *  48000:192057        96000:384000  192000*2
	 *  48000:192057        88200:352800  176400*1
	 *  48000:192057        48000:192000  192000/1
	 *  48000:192057        44100:176400  176400/1
	 *  48000:192057        22050:88200   176400/2
	 *  48000:192057        16000:64000   192000/3
	 *  48000:192057        8000:32000    192000/6
	 *
	 *  44100:176470        96000:384000  192000*2
	 *  44100:176470        88200:352800  176400*1
	 *  44100:176470        48000:192000  192000/1
	 *  44100:176470        44100:176400  176400/1
	 *  44100:176470        22050:88200   176400/2
	 *  44100:176470        16000:64000   192000/3
	 *  44100:176470        8000:32000    192000/6
	 *
	 *  16000:192057        96000:1152000 192000*6
	 *  16000:192057        48000:576000  192000*3
	 *  16000:192057        16000:192000  192000/1
	 *  16000:192057        8000:96000    192000/2
	 *
	 *  8000:192057         96000:2304000 192000*12
	 *  8000:192057         48000:1152000 192000*6
	 *  8000:192057         16000:384000  192000*2
	 *  8000:192057         8000:192000   192000/1
	 */
	if (den_orate >= den_irate) {
		den = den_orate / den_irate;
		if (den > 0)
			in_rate = in_rate / den;
	} else {
		den = den_irate / den_orate;
		if (den > 0)
			in_rate = in_rate * den;
	}

	*in = in_rate;
	*out = out_rate;
	return;
}
#endif

static bool resample_ap_update_mode(struct audio_plugin *ap, struct as_pcm_config *src_config, struct as_pcm_config *dst_config)
{
	struct resample_data *rd;
	int ret;
	uint32_t in_rate;
	uint32_t out_rate;
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_PLUGIN_RESAMPLE_ADJUST
	int adjust_onoff = 0;

	adjust_onoff = check_resample_adjust_onoff(ap);
	if (adjust_onoff > 0)
		goto ignore_rate_check;
#endif
	_debug("");
	if (dst_config->rate == src_config->rate) {
		ap->mode = AP_MODE_BYPASS;
		return 0;
	}
	if (dst_config->format == SND_PCM_FORMAT_S32_LE ||
		src_config->rate == SND_PCM_FORMAT_S32_LE) {
		fatal("not support s32_le rate convert");
	}

	if (!ap->private_data)
		resample_ap_init(ap);
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_PLUGIN_RESAMPLE_ADJUST
ignore_rate_check:
#endif
	rd = ap->private_data;

	rd->src_config = src_config;
	rd->dst_config = dst_config;

	if (rd->st != NULL)
		return 0;
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_PLUGIN_RESAMPLE_ADJUST
	if (adjust_onoff) {
		get_input_output_rate(rd, &in_rate ,&out_rate, adjust_onoff & (1 << 1));
	} else {
		in_rate = rd->src_config->rate;
		out_rate = rd->dst_config->rate;
	}
#else
	in_rate = rd->src_config->rate;
	out_rate = rd->dst_config->rate;
#endif

	rd->st = speex_resampler_init_frac(rd->dst_config->channels,
			in_rate, out_rate, in_rate, out_rate,
			rd->quality, &ret);
	if (!rd->st || ret != 0) {
		_err("st:%p, ret:%d", rd->st, ret);
		return -1;
	}

	speex_resampler_set_rate_frac(rd->st, in_rate, out_rate, in_rate, out_rate);

#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_PLUGIN_RESAMPLE_ADJUST
	if (adjust_onoff)
		rd->src_buf_size = RESAMPLE_ADJUST(rd->dst_config->period_frames) \
					* rd->dst_config->frame_bytes;
	else
#endif
		rd->src_buf_size = rd->dst_config->period_frames * rd->dst_config->frame_bytes;
	rd->src_buf = as_alloc(rd->src_buf_size);
	if (!rd->src_buf)
		fatal("no memory");

	ap->mode = AP_MODE_WORK;

	_debug("src buf:%p, size:%u", rd->src_buf, rd->src_buf_size);
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_PLUGIN_RESAMPLE_ADJUST
	if (adjust_onoff)
		_debug("resample adjust: %u -> %u", in_rate, out_rate);
	else
#endif
		_info("create resample: %u -> %u", in_rate, out_rate);

	return 0;
}

static int resample_ap_process(struct audio_plugin *ap,
				void *in_data, uint32_t in_size,
				void **out_data, uint32_t *out_size)
{
	struct resample_data *rd = ap->private_data;
	uint32_t src_frames = in_size;
	int16_t *src = (int16_t *)in_data;
	uint32_t dst_frames;
	int16_t *dst = (int16_t *)rd->src_buf;
	int ret;
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_PLUGIN_RESAMPLE_ADJUST
	int adjust_onoff = 0;

	adjust_onoff = check_resample_adjust_onoff(ap);
	if (adjust_onoff > 0)
		dst_frames = RESAMPLE_ADJUST(rd->dst_config->period_frames);
	else
#endif
		dst_frames = rd->dst_config->period_frames;

	/*_debug("src_frames:%u, dst_frames:%u", src_frames, dst_frames);*/
	if (!dst)
		fatal("src buf is NULL");
	ret = speex_resampler_process_interleaved_int(rd->st, src, &src_frames,
				dst, &dst_frames);
	if (ret != 0 || src_frames != in_size) {
		_err("src_frames:%u != in_size:%u, ret=%d", src_frames, in_size, ret);
	}

	/*_debug("src:%p, src_frames:%u, dst:%p, dst_frames:%u",*/
			/*src, src_frames, dst, dst_frames);*/

	*out_data = dst;
	*out_size = dst_frames;

	return 0;
}

static int resample_ap_release(struct audio_plugin *ap)
{
	struct resample_data *rd = ap->private_data;

	if (!rd)
		return 0;

	if (rd->st != NULL) {
		speex_resampler_destroy(rd->st);
		rd->st = NULL;
	}
	if (rd->src_buf) {
		as_free(rd->src_buf);
		rd->src_buf = NULL;
	}
	as_free(rd);

	return 0;
}

/*
 * 运行resample插件,保证format,channels一致，且以slave_pcm的为准
 * 注意不要使用src_pcm的frame_bytes, 如果之前执行过bitsconv或者chmap，那么src_pcm的frame_bytes会不一致
 */
const struct audio_plugin resample_ap = {
	.ap_name =		"resample",
	.ap_init =		resample_ap_init,
	.ap_process =		resample_ap_process,
	.ap_release =		resample_ap_release,
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_PLUGIN_RESAMPLE_ADJUST
	.ap_setup =             resample_ap_setup,
#endif
	.ap_update_mode =	resample_ap_update_mode,
	.mode =			AP_MODE_BYPASS,
};
