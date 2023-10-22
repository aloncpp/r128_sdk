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
#define TAG	"AP-BitsConv"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "audio_plugin.h"
#include "AudioBase.h"




#define CHMAP_MAX_CH	(15)

struct bitsconv_data {
	struct as_pcm_config *src_config;
	struct as_pcm_config *dst_config;
	void *bitsconv_buf;
	uint32_t buf_size;
};

static int bitsconv_init(struct audio_plugin *ap)
{
	struct bitsconv_data *bc = ap->private_data;

	if (bc != NULL)
		return 0;

	_debug("");
	bc = as_alloc(sizeof(struct bitsconv_data));
	ap->private_data = bc;

	return 0;
}

static bool bitsconv_update_mode(struct audio_plugin *ap, struct as_pcm_config *src_config, struct as_pcm_config *dst_config)
{
	struct bitsconv_data *bc;

	if (dst_config->format == src_config->format) {
		_debug("");
		ap->mode = AP_MODE_BYPASS;
		return 0;
	}

	if (!ap->private_data)
		bitsconv_init(ap);
	bc = ap->private_data;
	if (bc->bitsconv_buf != NULL)
		return 0;

	bc->src_config = src_config;
	bc->dst_config = dst_config;

	_info("bits: %d -> %d", format_to_bits(src_config->format),
				format_to_bits(dst_config->format));
	/* 只有format做转换，channel不变 */
	bc->buf_size = src_config->period_frames * dst_config->frame_bytes * src_config->channels / dst_config->channels;
	bc->bitsconv_buf = as_alloc(bc->buf_size);
	if (!bc->bitsconv_buf)
		fatal("no memory");
	_debug("buf size=%d, addr:%p", bc->buf_size, bc->bitsconv_buf);

	ap->mode = AP_MODE_WORK;
	return 0;
}

static int bitsconv_process(struct audio_plugin *ap,
				void *in_data, uint32_t in_size,
				void **out_data, uint32_t *out_size)
{
	struct as_pcm_config *dst_config = NULL;
	struct as_pcm_config *src_config = NULL;
	struct bitsconv_data *bc = ap->private_data;
	int frames = in_size;
	int ch, channels;

	if (!bc) {
		_err("(%s) private_data not set", ap->ap_name);
		return -1;
	}

	dst_config = bc->dst_config;
	src_config = bc->src_config;

	channels = src_config->channels;
	/*
	 * support s16->s32, s32->s16 only
	 */
	if (src_config->format == SND_PCM_FORMAT_S16_LE &&
		dst_config->format == SND_PCM_FORMAT_S32_LE) {
		int16_t *src;
		int32_t *dst;

		/*
		 * xx12 -> 0012
		 * */
		for (ch = 0; ch < channels; ch++) {
			src = (int16_t *)in_data;
			dst = (int32_t *)bc->bitsconv_buf;
			while (frames-- > 0) {
				*(dst + ch) = (int32_t)((*(src + ch)) << 16);
				src += channels;
				dst += channels;
			}
		}
		/*printf("[%s] line:%d in_size=%d, bitsconv_buf=%p, dst=%p\n", __func__, __LINE__,*/
			/*in_size, bc->bitsconv_buf, dst);	*/
	} else if (src_config->format == SND_PCM_FORMAT_S32_LE &&
		dst_config->format == SND_PCM_FORMAT_S16_LE) {
		int32_t *src;
		int16_t *dst;

		/*
		 * 1234 -> 0034
		 * */
		for (ch = 0; ch < channels; ch++) {
			src = (int32_t *)in_data;
			dst = (int16_t *)bc->bitsconv_buf;
			while (frames-- > 0) {
				*(dst + ch) = (int16_t)((*(src + ch)) >> 16);
				src += channels;
				dst += channels;
			}
		}
	} else {
		fatal("unknown format");
	}

	*out_data = bc->bitsconv_buf;
	*out_size = in_size;

	return 0;
}

static int bitsconv_release(struct audio_plugin *ap)
{
	struct bitsconv_data *bc = ap->private_data;

	if (!bc)
		return 0;
	if (bc->bitsconv_buf) {
		as_free(bc->bitsconv_buf);
		bc->bitsconv_buf = NULL;
	}
	ap->private_data = NULL;
	as_free(bc);

	return 0;
}

/*
 * 运行bitsconv只会转换format,其他channels等信息仍维持src_pcm
 * 运行bitsconv插件之后，会导致frame_bytes不一致
 */
const struct audio_plugin bits_conv = {
	.ap_name =		"bitsconv",
	.ap_init =		bitsconv_init,
	.ap_process =		bitsconv_process,
	.ap_release =		bitsconv_release,
	.ap_update_mode =	bitsconv_update_mode,
	.mode =			AP_MODE_BYPASS,
};
