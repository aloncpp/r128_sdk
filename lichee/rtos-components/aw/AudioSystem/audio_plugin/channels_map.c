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
#define TAG	"AP-chmap"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "audio_plugin.h"
#include "AudioBase.h"

#define CHMAP_MAX_CH	(15)

struct chmap_data {
	void *chmap_buf;
	uint32_t chmap_buf_size;
	int (*do_chmap)(void *in_data, uint32_t in_size,
			void **out_data, uint32_t *out_size);

	struct as_pcm_config *src_config;
	struct as_pcm_config *dst_config;

	int8_t chsel[CHMAP_MAX_CH];
	int8_t already_set;
};

static int chmap_ap_init(struct audio_plugin *ap)
{
	struct chmap_data *cm = ap->private_data;

	if (cm != NULL)
		return 0;

	_debug("");
	cm = as_alloc(sizeof(struct chmap_data));
	ap->private_data = cm;

	return 0;
}

/*
 * channel map rule:
 * -1: NULL, 0 data
 * 0~n: ch0~n, total n+1 channels
 * n+1: average
 *
 * */
static int chmap_ap_setup(struct audio_plugin *ap, void *para, uint32_t size)
{
	struct chmap_data *cm;

	if (!ap->private_data)
		chmap_ap_init(ap);
	cm = ap->private_data;

	if (size > CHMAP_MAX_CH)
		return -1;
	memcpy(cm->chsel, para, size);

	cm->already_set = 1;
	return 0;
}

static bool chmap_ap_update_mode(struct audio_plugin *ap, struct as_pcm_config *src_config, struct as_pcm_config *dst_config)
{
	struct chmap_data *cm;
	int8_t i;
	uint8_t stream = 0;

	if (!ap->private_data && dst_config->channels == src_config->channels) {
		_debug("");
		ap->mode = AP_MODE_BYPASS;
		return 0;
	}

	if (!ap->private_data)
		chmap_ap_init(ap);
	cm = ap->private_data;
	if (cm->chmap_buf != NULL)
		return 0;

	cm->src_config = src_config;
	cm->dst_config = dst_config;
	stream = ap->stream;

	_debug("");
	if (!cm->already_set) {
		if (dst_config->channels == 1) {
			if (stream == 0) {
				/* n --> 1, (n > 1): average */
				cm->chsel[0] = src_config->channels;
			} else {
				cm->chsel[0] = 0;
			}
		} else if (src_config->channels == 1) {
			/* n --> m, (m > 1, n == 1): dup */
			for (i = 0; i < dst_config->channels; i++)
				cm->chsel[i] = 0;
		} else if (src_config->channels > dst_config->channels) {
			/* n --> m, (n > m >= 2): copy */
			for (i = 0; i < dst_config->channels; i++)
				cm->chsel[i] = i;
		} else if (src_config->channels < dst_config->channels) {
			/* n --> m, (m > n >= 2): copy and discard */
			for (i = 0; i < dst_config->channels; i++) {
				if (i < src_config->channels)
					cm->chsel[i] = i;
				else
					cm->chsel[i] = -1;
			}
		} else {
			_err("unexpected case");
		}
		cm->already_set = 1;
	}
	_debug("");
#if 1
	for (i = 0; i < dst_config->channels; i++) {
		if (cm->chsel[i] < 0) {
			_info("NULL -> ch%d", i);
		} else if (cm->chsel[i] == src_config->channels) {
			_info("ALL  -> ch%d(average)", i);
		} else {
			_info("ch%d -> ch%d", cm->chsel[i], i);
		}
		if (cm->chsel[i] > src_config->channels) {
			_err("chsel[%d] is %d, but src channels is %d",
				i, cm->chsel[i], src_config->channels);
			ap->mode = AP_MODE_BYPASS;
			return 0;
		}
	}
#endif

#if 1
	/*
	 * chmap buffer
	 *
	 * playback, src->dst(audio_hw)
	 *     store dst->frame_bytes * src->period_frames
	 * capture, dst<-src(audio_hw)
	 *     store src->frame_bytes * src->period_frames
	 *
	 */
	if (stream == 0) {
		cm->chmap_buf_size = src_config->period_frames * dst_config->frame_bytes;
	} else {
		cm->chmap_buf_size = src_config->period_frames * src_config->frame_bytes;
	}
	cm->chmap_buf = as_alloc(cm->chmap_buf_size);
	_debug("malloc chmap buf:%p, size:%d", cm->chmap_buf, cm->chmap_buf_size);
#else
	if (slave_pcm->channels > at_pcm->channels) {
		/* need tmp buffer to store output chmap data */
		cm->chmap_buf_size = slave_pcm->period_frames * slave_pcm->frame_bytes;
		cm->chmap_buf = as_alloc(cm->chmap_buf_size);
		_info("malloc chmap buf:%p, size:%d", cm->chmap_buf, cm->chmap_buf_size);
	}
#endif

	ap->mode = AP_MODE_WORK;
	return 0;
}

static int do_channel_map_16(struct chmap_data *cm, uint8_t src_ch, uint8_t dst_ch, void *in_data, uint32_t in_size)
{
	int16_t *out = NULL, *in = NULL;
	int i,j;

	in = (int16_t *)in_data;
	out = (int16_t *)cm->chmap_buf;
	/*_info("dst ch:%d, in_size:%d", dst_ch, in_size);*/
	for (i = 0; i < dst_ch; i++) {
		/*_info("set ch:%d", i);*/
		for (j = 0; j < in_size; j++) {
			int iOut = j * dst_ch + i;
			int iIn = j * src_ch + cm->chsel[i];
			/*_info("set out index:%d, in index:%d", iOut, iIn);*/
			if (cm->chsel[i] < 0)
				out[iOut] = (int16_t)0;
			else if (cm->chsel[i] == src_ch) {
				int z;
				int32_t tmp = 0;
				for (z = 0; z < src_ch; z++) {
					iIn = j * src_ch + z;
					tmp += in[iIn] / src_ch;
				}
				if (tmp > 0x7fff)
					out[iOut] = 0x7fff;
				else if (tmp < -0x8000)
					out[iOut] = -0x8000;
				else
					out[iOut] = tmp;
			} else
				out[iOut] = in[iIn];
		}
	}
	return 0;
}

static int do_channel_map_32(struct chmap_data *cm, uint8_t src_ch, uint8_t dst_ch, void *in_data, uint32_t in_size)
{
	int32_t *out = NULL, *in = NULL;
	int i,j;

	in = (int32_t *)in_data;
	out = (int32_t *)cm->chmap_buf;
	/*_info("dst ch:%d, in_size:%d", dst_ch, in_size);*/
	for (i = 0; i < dst_ch; i++) {
		/*_info("set ch:%d", i);*/
		for (j = 0; j < in_size; j++) {
			int iOut = j * dst_ch + i;
			int iIn = j * src_ch + cm->chsel[i];
			/*_info("set out index:%d, in index:%d", iOut, iIn);*/
			if (cm->chsel[i] < 0)
				out[iOut] = (int32_t)0;
			else if (cm->chsel[i] == src_ch) {
				int z;
				int32_t tmp = 0;
				for (z = 0; z < src_ch; z++) {
					iIn = j * src_ch + z;
					tmp += in[iIn] / src_ch;
				}
				if (tmp > (int)0x7fffffff)
					out[iOut] = (int)0x7fffffff;
				else if (tmp < (int)0x80000000)
					out[iOut] = (int)0x80000000;
				else
					out[iOut] = tmp;
			} else
				out[iOut] = in[iIn];
		}
	}
	return 0;
}

static int chmap_ap_process(struct audio_plugin *ap,
				void *in_data, uint32_t in_size,
				void **out_data, uint32_t *out_size)
{
	struct as_pcm_config *dst_config = NULL;
	struct as_pcm_config *src_config = NULL;
	struct chmap_data *cm = ap->private_data;

	if (!cm) {
		_err("(%s) private_data not set", ap->ap_name);
		return -1;
	}

	dst_config = cm->dst_config;
	src_config = cm->src_config;

	if (dst_config->format == SND_PCM_FORMAT_S16_LE) {
		do_channel_map_16(cm, src_config->channels, dst_config->channels, in_data, in_size);
	} else if (dst_config->format == SND_PCM_FORMAT_S32_LE) {
		do_channel_map_32(cm, src_config->channels, dst_config->channels, in_data, in_size);
	} else {
		fatal("unknown format");
	}

	*out_data = cm->chmap_buf;
	*out_size = in_size;

	return 0;
}

static int chmap_ap_release(struct audio_plugin *ap)
{
	struct chmap_data *cm = ap->private_data;

	if (!cm)
		return 0;
	if (cm->chmap_buf != NULL) {
		as_free(cm->chmap_buf);
		cm->chmap_buf = NULL;
	}
	as_free(cm);

	return 0;
}

/*
 * 运行channel map插件,保证format一致，且以slave_pcm的为准
 * 注意不要使用src_pcm的frame_bytes, 如果之前执行过bitsconv，那么src_pcm的frame_bytes会不一致
 */
const struct audio_plugin chmap_ap = {
	.ap_name =		"chmap",
	.ap_init =		chmap_ap_init,
	.ap_process =		chmap_ap_process,
	.ap_release =		chmap_ap_release,
	.ap_update_mode =	chmap_ap_update_mode,
	.ap_setup =		chmap_ap_setup,
	.mode =			AP_MODE_BYPASS,
};
