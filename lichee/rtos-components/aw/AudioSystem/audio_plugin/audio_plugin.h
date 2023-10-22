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
#ifndef __AUDIO_PLUGIN_H
#define __AUDIO_PLUGIN_H

#include <assert.h>
#include <aw_common.h>
#include <aw_list.h>
#include <errno.h>
#include <stdbool.h>

#include "AudioBase.h"

#include <aw-alsa-lib/pcm.h>
#include <aw-alsa-lib/common.h>

enum {
	AP_MODE_BYPASS = 0,
	AP_MODE_WORK,
};


/*
 * ap_init,
 * ap_process,
 * ap_release,
 * ap_update_mode,
 * ap_setup,
 *
 */
struct audio_plugin {
	const char *ap_name;
	struct list_head list;

	int (*ap_init)(struct audio_plugin *ap);
	int (*ap_process)(struct audio_plugin *ap, void *in_data, uint32_t in_size, void **out_data, uint32_t *out_size);
	int (*ap_release)(struct audio_plugin *ap);
	bool (*ap_update_mode)(struct audio_plugin *ap, struct as_pcm_config *src_config, struct as_pcm_config *dst_config);
	int (*ap_setup)(struct audio_plugin *ap, void *para, uint32_t size);

	void *private_data;

	uint8_t mode;
	uint8_t stream;
};

#define RESAMPLE_ADJUST(val)	(val * 5 / 4)

void ap_add_default_plugin(struct list_head *head, uint8_t stream);
int do_audio_plugin(struct list_head *head, void *data, uint32_t size, void **data_out, uint32_t *data_out_size);
bool need_audio_plugin(struct list_head *head);

#endif /* __AUDIO_PLUGIN_H */
