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
#ifndef __LOCAL_AUDIO_HW_H
#define __LOCAL_AUDIO_HW_H

#include <assert.h>
#include <aw_common.h>
#include <aw_list.h>
#include <errno.h>
#include <stdbool.h>

#include "AudioBase.h"

#include <aw-alsa-lib/pcm.h>
#include <aw-alsa-lib/common.h>



typedef int (*ahw_func)(void *pdata);
typedef int (*ahw_func_xfer)(void *pdata, struct rb_attr *rb);

struct audio_hw_ops {
	ahw_func	ahw_init;
	ahw_func	ahw_open;
	ahw_func_xfer	ahw_read;
	ahw_func_xfer	ahw_write;
	ahw_func	ahw_close;
	ahw_func	ahw_destroy;
};

enum AUDIO_HW_TYPE {
	AUDIO_HW_TYPE_UNKNOWN = 0,
	AUDIO_HW_TYPE_PCM,
	AUDIO_HW_TYPE_PCM_PB_ONLY,
	AUDIO_HW_TYPE_PCM_CAP_ONLY,
	AUDIO_HW_TYPE_PCM_MULTI,
	AUDIO_HW_TYPE_PCM_COMPOSITE,
	AUDIO_HW_TYPE_AMP = 20,
	AUDIO_HW_TYPE_BT = 30,
	AUDIO_HW_TYPE_USB = 40,
	AUDIO_HW_TYPE_EQ =  50,
	AUDIO_HW_TYPE_MAX = 128,
};

struct audio_hw_elem {
	char name[11];
	int8_t instance;
	const char *card_name_pb;
	const char *card_name_cap;
	struct audio_hw_ops *ops;
	struct list_head list;
};

#define AHW_ADD_PCM_DEFAULT(name) \
extern struct audio_hw_elem name; \
	a = as_alloc(sizeof(struct audio_hw_elem));if (!a) fatal("no memory"); \
	memcpy(a, &name, sizeof(struct audio_hw_elem)); \
	list_add_tail(&a->list, head);

#define AHW_DEFAULT_PB_RATE		(48000)
#define AHW_DEFAULT_PB_CHANNELS		(2)
#define AHW_DEFAULT_PB_FORMAT		(SND_PCM_FORMAT_S16_LE)
#define AHW_DEFAULT_PB_PERIOD_SIZE	(960)
#define AHW_DEFAULT_PB_PERIODS		(4)

#define AHW_DEFAULT_CAP_RATE		(16000)
#define AHW_DEFAULT_CAP_CHANNELS	(3)
#define AHW_DEFAULT_CAP_FORMAT		(SND_PCM_FORMAT_S16_LE)
#define AHW_DEFAULT_CAP_PERIOD_SIZE	(320)
#define AHW_DEFAULT_CAP_PERIODS		(4)

struct ahw_params {
	char card_name[32];
	uint16_t rate;
	uint16_t channels;
	uint16_t bits;
	uint16_t period_frames;
	uint16_t periods;
};

as_pcm_config_t *ahw_params_init(const char *prefix, struct ahw_params *params[], audio_hw_t *ahw);
int eq_hw_init(void);
int eq_hw_destroy(void);
int drc_hw_init(void);
int drc_hw_destroy(void);


#endif /* __LOCAK_AUDIO_HW_H */
