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
#define TAG	"AudioPlugin"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "audio_plugin.h"
#include "AudioBase.h"


extern const struct audio_plugin bits_conv;
extern const struct audio_plugin chmap_ap;
extern const struct audio_plugin resample_ap;
extern const struct audio_plugin softvol_ap;

/*
 * be careful sequence
 * 1.format, 后续plugin都采用dst_config的采样精度参数
 * 2.channels, 后续channels都采用dst_config的通道数
 * 3.rate, 后续rate都采样dst_config的通道数
 *
 */
void ap_add_default_plugin(struct list_head *head, uint8_t stream)
{
	struct audio_plugin *ap;

	/* bits convert */
	ap = as_alloc(sizeof(struct audio_plugin));
	if (!ap)
		fatal("no memory");
	memcpy(ap, &bits_conv, sizeof(struct audio_plugin));
	list_add_tail(&ap->list, head);
	ap->stream = stream;

	/* channel mapping */
	ap = as_alloc(sizeof(struct audio_plugin));
	if (!ap)
		fatal("no memory");
	memcpy(ap, &chmap_ap, sizeof(struct audio_plugin));
	list_add_tail(&ap->list, head);
	ap->stream = stream;

	/* rate resample */
	ap = as_alloc(sizeof(struct audio_plugin));
	if (!ap)
		fatal("no memory");
	memcpy(ap, &resample_ap, sizeof(struct audio_plugin));
	list_add_tail(&ap->list, head);
	ap->stream = stream;

	/* soft volume adjust */
	ap = as_alloc(sizeof(struct audio_plugin));
	if (!ap)
		fatal("no memory");
	memcpy(ap, &softvol_ap, sizeof(struct audio_plugin));
	list_add_tail(&ap->list, head);
	ap->stream = stream;
}

int do_audio_plugin(struct list_head *head, void *data, uint32_t size, void **data_out, uint32_t *data_out_size)
{
	struct audio_plugin *ap;
	void *in = NULL, *out = NULL;
	uint32_t in_size = 0, out_size = 0;
	int ret;

	in = data;
	in_size = size;
	list_for_each_entry(ap, head, list) {
		/*printf("[%s] line:%d ap name:%s\n", __func__, __LINE__, ap->ap_name);*/
		if (ap->mode == AP_MODE_BYPASS)
			continue;
		/*printf("[%s] line:%d in:%p, in_size=%d\n", __func__, __LINE__, in, in_size);*/
		ret = ap->ap_process(ap, in, in_size,
				&out, &out_size);
		/*printf("[%s] line:%d out:%p, out_size=%d\n", __func__, __LINE__, out, out_size);*/
		if (!ret) {
			in = out;
			in_size = out_size;
			out = NULL;
			out_size = 0;
		}
	}
	*data_out = in;
	*data_out_size = in_size;

	return 0;
}

bool need_audio_plugin(struct list_head *head)
{
	struct audio_plugin *ap;

	list_for_each_entry(ap, head, list) {
		if (ap->mode == AP_MODE_BYPASS)
			continue;
		return true;
	}
	return false;
}
