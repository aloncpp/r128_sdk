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
#define TAG	"AP-softvol"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "audio_plugin.h"
#include "AudioBase.h"
#include <AudioSystem.h>

#include <math.h>

#include <hal_cfg.h>

#define VOL_SCALE_SHIFT         16
#define VOL_SCALE_MASK          ((1 << VOL_SCALE_SHIFT) - 1)

static LIST_HEAD(g_softvol_stream_list);

struct softvol_point {
	uint16_t	index;	/* volume index */
	int16_t		dB;     /* volume dB */
	/*uint32_t	value;  [> volume value <]*/
};

struct softvol_stream {
	struct list_head list;
	struct softvol_point *sv_point;
	uint32_t *dB_value;
	uint16_t zero_dB_vol;
	uint16_t min,max;
	uint16_t cur_vol[2];
	enum AUDIO_STREAM_TYPE stream_type;
};

struct softvol_data {
	struct softvol_stream *sv_link;
	struct as_pcm_config *pcm_config;
	uint32_t data_size;
	void *data;
};

/* volume setting */
static struct softvol_point g_softvol_point_default[] = {
	{ 1,	-4800 },
	{ 3,	-2400 },
	{ 6,	-1200 },
	{ 10,	0 },
};

static struct softvol_point g_softvol_point_music[] = {
	{ 1,	-4800 },
	{ 2,	-2400 },
	{ 5,	-1200 },
	{ 10,	0 },
};

static inline short multi_div_16(short a, unsigned int b)
{
	uint32_t gain = b >> VOL_SCALE_SHIFT;
	int fraction;

	fraction = (int)(a * (b & VOL_SCALE_MASK)) >> VOL_SCALE_SHIFT;
	if (gain) {
		int amp = a * gain + fraction;
		if (abs(amp) > 0x7fff)
			amp = (a < 0) ? (short) 0x8000 : (short)0x7fff;
		return (short)amp;
	}
	return (short)fraction;
}

/* (32bit x 16bit) >> 16 */
typedef union {
	int i;
	short s[2];
} val_t;

static inline int MULTI_DIV_32x16(int a, unsigned short b)
{
	val_t v, x, y;
	v.i = a;
	y.i = 0;
	x.i = (unsigned short)v.s[0];
	x.i *= b;
	y.s[0] = x.s[1];
	y.i += (int)v.s[1] * b;

	return y.i;
}

static inline int multi_div_32(int a, unsigned int b)
{
	uint32_t gain = b >> VOL_SCALE_SHIFT;
	int fraction;

	fraction = MULTI_DIV_32x16(a, b & VOL_SCALE_MASK);
	if (gain) {
		long long amp = (long long)a * gain + fraction;
		if (abs(amp) > (int)0x7fffffff)
			amp = (int)0x7fffffff;
		else if (amp < (int)0x80000000)
			amp = (int)0x80000000;
		return (int)amp;
	}
	return fraction;
}

/*#define SOFTVOL_DUMP*/
#ifdef SOFTVOL_DUMP
static void dump_sv_point(uint32_t *db_value, uint32_t num)
{
	int i;

	printf("stream value:\n");
	for (i = 0; i < num; i++) {
		printf("  0x%08x(%06f)", db_value[i],
			(double)db_value[i] / (1 << VOL_SCALE_SHIFT));
		if ((i + 1) % 6 == 0)
			printf("\n");
	}
	printf("\n");
}
#endif

static void softvol_add_stream(struct softvol_point *sv_point, uint16_t point_num,
				enum AUDIO_STREAM_TYPE type)
{
	struct softvol_stream *svs;
	int16_t i,j, index;
	double db;
	uint32_t step = 0;

	/*_info("type:%d init", type);*/
	svs = as_alloc(sizeof(struct softvol_stream));
	if (!svs)
		return;
	INIT_LIST_HEAD(&svs->list);
	svs->stream_type = type;
	svs->sv_point = sv_point;
	svs->min = 0;
	svs->max = svs->sv_point[point_num - 1].index;
	/* TODO: default volume index should be keep the last value */
	svs->cur_vol[0] = svs->cur_vol[1] = svs->max / 2;
	svs->dB_value = as_alloc((svs->max + 1 ) * sizeof(uint32_t));

	for (j = 0, i = 0; i < point_num; i++) {
		index = svs->sv_point[i].index;
		db = (double)svs->sv_point[i].dB / 100;
		/* TODO, sv_point need 0dB point */
		if (svs->sv_point[i].dB == 0)
			svs->zero_dB_vol = svs->sv_point[i].index;
		svs->dB_value[index] = (pow(10.0, db / 20.0) * (1 << VOL_SCALE_SHIFT));
		/*printf("index=%d, db=%f, value=0x%x\n", index, db, svs->dB_value[index]);*/
		step = svs->sv_point[i].index - j;
		if (!step || step == 1) {
			j = svs->sv_point[i].index;
			continue;
		}
		step = (svs->dB_value[index] - svs->dB_value[j]) / step;
		/*printf("step:0x%x, start:0x%x, end:0x%x\n", step,*/
			/*svs->dB_value[j], svs->dB_value[index]);*/
		for (j = j + 1; j < svs->sv_point[i].index; j++) {
			svs->dB_value[j] = svs->dB_value[j - 1] + step;
		}
	}
#if 0
#ifdef SOFTVOL_DUMP
	dump_sv_point(svs->dB_value, svs->max + 1);
#endif
#endif
	list_add_tail(&svs->list, &g_softvol_stream_list);
	return;
}

#ifdef CONFIG_COMPONENTS_AW_SYS_CONFIG_SCRIPT
static int softvol_stream_param(const char *type_name, struct softvol_point **svp, int *size)
{
#define SVP_STEP_MAX 	(20)
	struct softvol_point local_svp[SVP_STEP_MAX];
	struct softvol_point *svp_out = NULL;
	char name[32];
	char value_name[16];
	int32_t tmp_val;
	int i;

	snprintf(name, sizeof(name), "audio_stream_%s", type_name);
	for (i = 0; i < SVP_STEP_MAX; i++) {
		snprintf(value_name, sizeof(value_name), "index_%d", i);
		if (hal_cfg_get_keyvalue(name, value_name, (int32_t *)&tmp_val, 1) != 0)
			break;
		local_svp[i].index = tmp_val;
		snprintf(value_name, sizeof(value_name), "dB_%d", i);
		if (hal_cfg_get_keyvalue(name, value_name, (int32_t *)&tmp_val, 1) != 0)
			break;
		local_svp[i].dB = tmp_val;
	}
	if (i == 0)
		return -1;
	*size = i;
	svp_out = as_alloc(sizeof(struct softvol_point) * i);
	if (!svp_out)
		fatal("no memory");
	for (i = 0; i < *size; i++) {
		svp_out[i].index = local_svp[i].index;
		svp_out[i].dB = local_svp[i].dB;
	}
	*svp = svp_out;
	return 0;
}
#endif

void softvol_stream_init(void)
{
#ifdef CONFIG_COMPONENTS_AW_SYS_CONFIG_SCRIPT
	struct softvol_point *svp = NULL;
	int size = 0;

	if (!softvol_stream_param("system", &svp, &size)) {
		softvol_add_stream(svp, size, AUDIO_STREAM_SYSTEM);
	} else {
		softvol_add_stream(g_softvol_point_default,
			ARRAY_SIZE(g_softvol_point_default),
			AUDIO_STREAM_SYSTEM);
	}
	if (!softvol_stream_param("music", &svp, &size)) {
		softvol_add_stream(svp, size, AUDIO_STREAM_MUSIC);
	} else {
		softvol_add_stream(g_softvol_point_music,
			ARRAY_SIZE(g_softvol_point_music),
			AUDIO_STREAM_MUSIC);
	}
	if (!softvol_stream_param("notification", &svp, &size))
		softvol_add_stream(svp, size, AUDIO_STREAM_NOTIFICATION);
	if (!softvol_stream_param("bt_a2dp_sink", &svp, &size))
		softvol_add_stream(svp, size, AUDIO_STREAM_BT_A2DP_SINK);
	if (!softvol_stream_param("bt_a2dp_source", &svp, &size))
		softvol_add_stream(svp, size, AUDIO_STREAM_BT_A2DP_SOURCE);
#else
	/* as_alloc once */
	softvol_add_stream(g_softvol_point_default,
			ARRAY_SIZE(g_softvol_point_default),
			AUDIO_STREAM_SYSTEM);
	softvol_add_stream(g_softvol_point_music,
			ARRAY_SIZE(g_softvol_point_music),
			AUDIO_STREAM_MUSIC);
#endif
}

static int softvol_ap_init(struct audio_plugin *ap)
{
	struct softvol_data *sv;

	if (list_empty(&g_softvol_stream_list))
		softvol_stream_init();

	if (ap->private_data)
		return 0;

	sv = as_alloc(sizeof(struct softvol_data));
	if (!sv)
		return -1;
	sv->sv_link = NULL;

	ap->private_data = sv;

	return 0;
}

static bool softvol_ap_update_mode(struct audio_plugin *ap, struct as_pcm_config *src_config, struct as_pcm_config *dst_config)
{
	struct softvol_data *sv;
	/*struct softvol_stream *svs;*/

	if (!ap || !ap->private_data)
		return -1;

	sv = ap->private_data;
	sv->pcm_config = dst_config;

	if (sv->data != NULL)
		return 0;

#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_PLUGIN_RESAMPLE_ADJUST
	sv->data_size = RESAMPLE_ADJUST(sv->pcm_config->period_frames) * sv->pcm_config->frame_bytes;
#else
	sv->data_size = sv->pcm_config->period_frames * sv->pcm_config->frame_bytes;
#endif
	sv->data = as_alloc(sv->data_size);
	if (!sv->data)
		fatal("no memory");
	_debug("sv->data=%p, size=%u", sv->data, sv->data_size);

	return 0;
}

#define SOFTVOL_CONVERT_S16() \
do { \
	uint32_t ch, fr; \
	int16_t *in, *out; \
	for (ch = 0; ch < channels; ch++) { \
		for (fr = 0; fr < in_size; fr++) { \
			int index = fr * frame_bytes + ch * 2; \
			in = (int16_t *)(in_data + index); \
			out = (int16_t *)(sv->data + index); \
			*out = multi_div_16(*in, svs->dB_value[svs->cur_vol[ch]]); \
		} \
	} \
} while (0)

#define SOFTVOL_CONVERT_S32() \
do { \
	uint32_t ch, fr; \
	int32_t *in, *out; \
	for (ch = 0; ch < channels; ch++) { \
		for (fr = 0; fr < in_size; fr++) { \
			int index = fr * frame_bytes + ch * 4; \
			in = (int32_t *)(in_data + index); \
			out = (int32_t *)(sv->data + index); \
			*out = multi_div_32(*in, svs->dB_value[svs->cur_vol[ch]]); \
		} \
	} \
} while (0)

static int softvol_ap_process(struct audio_plugin *ap,
				void *in_data, uint32_t in_size,
				void **out_data, uint32_t *out_size)
{
	struct softvol_data *sv;
	struct softvol_stream *svs;
	int channels;
	snd_pcm_format_t format;
	uint32_t frame_bytes;

	if (!ap || !ap->private_data)
		return -1;
	sv = ap->private_data;
	svs = sv->sv_link;
	if (!sv->sv_link) {
		*out_data = (void *)sv->data;
		goto process_finish;
	}

	if (svs->cur_vol[0] == 0 && svs->cur_vol[1] == 0) {
		memset(sv->data, 0, sv->data_size);
		*out_data = (void *)sv->data;
		goto process_finish;
	} else if (svs->cur_vol[0] == svs->zero_dB_vol && svs->cur_vol[1] == svs->zero_dB_vol) {
		*out_data = in_data;
		goto process_finish;
	}

	/* format, channels */
	format = sv->pcm_config->format;
	channels = sv->pcm_config->channels;
	frame_bytes = sv->pcm_config->frame_bytes;

	if (format == SND_PCM_FORMAT_S16_LE) {
		SOFTVOL_CONVERT_S16();
		*out_data = (void *)sv->data;
	} else if (format == SND_PCM_FORMAT_S32_LE) {
		SOFTVOL_CONVERT_S32();
		*out_data = (void *)sv->data;
	} else {
		fatal("unknown format");
	}

process_finish:
	*out_size = in_size;
	return 0;
}

static int softvol_ap_release(struct audio_plugin *ap)
{
	struct softvol_data *sv;

	if (!ap || !ap->private_data)
		return -1;
	sv = ap->private_data;
	if (!sv)
		return -1;
	if (sv->data) {
		as_free(sv->data);
		sv->data = NULL;
	}
	as_free(sv);
	ap->private_data = NULL;
	return 0;
}

static int softvol_ap_setup(struct audio_plugin *ap, void *para, uint32_t size)
{
	struct softvol_stream *svs;
	enum AUDIO_STREAM_TYPE type;

	if (size != sizeof(enum AUDIO_STREAM_TYPE))
		return -1;
	type = *(enum AUDIO_STREAM_TYPE *)para;
	if (!ap->private_data)
		softvol_ap_init(ap);

	/* find and set sv_point */
	list_for_each_entry(svs, &g_softvol_stream_list, list) {
		if (svs->stream_type == type) {
			struct softvol_data *sv = ap->private_data;

			sv->sv_link = svs;
			ap->mode = AP_MODE_WORK;
			return 0;
		}
	}

	return -1;
}

/* volume adjust, mode:0-read, 1-write, 2-read range */
static int _softvol_control(struct softvol_stream *svs, uint32_t *vol_index, int16_t mode)
{


	switch (mode) {
	case 0:
		*vol_index = (uint32_t)(svs->cur_vol[0] | (svs->cur_vol[1] << 16));
		break;
	case 1:
		{
		uint32_t lvol, rvol;

		lvol = *vol_index & 0xffff;
		rvol = ((*vol_index) >> 16) & 0xffff;

		if (lvol < svs->min || lvol > svs->max)
			return -1;
		if (rvol < svs->min || rvol > svs->max)
			return -1;
		memcpy(svs->cur_vol, vol_index, sizeof(svs->cur_vol));
#if 0
		_info("cur_vol=%d,%d, value:%u,%u", svs->cur_vol[0], svs->cur_vol[1],
			svs->dB_value[svs->cur_vol[0]],
			svs->dB_value[svs->cur_vol[1]]);

		int16_t value = 0x7fff;
		_info("0x%x, -> 0x%x", value, multi_div_16(value, svs->dB_value[svs->cur_vol[0]]));
		dump_sv_point(svs->dB_value, svs->max + 1);
#endif
		}
		break;
	case 2:
		*vol_index = (uint32_t)(svs->min | (svs->max << 16));
		break;
	default:
		return -1;
	}

	return 0;
}

int softvol_control(struct audio_plugin *ap, uint32_t *vol_index, int16_t mode)
{
	struct softvol_data *sv;
	struct softvol_stream *svs;

	if (!ap || !ap->private_data)
		return -1;
	sv = ap->private_data;
	svs = sv->sv_link;

	if (!svs)
		return -1;
	return  _softvol_control(svs, vol_index, mode);
}

int softvol_control_with_streamtype(int stream_type, uint32_t *vol_index, int16_t mode)
{
	struct softvol_stream *svs = NULL;

	list_for_each_entry(svs, &g_softvol_stream_list, list) {
		if (svs->stream_type == stream_type) {
			return _softvol_control(svs, vol_index, mode);
		}
	}
	return -1;
}

struct audio_plugin softvol_ap = {
	.ap_name =		"softvol",
	.ap_init =		softvol_ap_init,
	.ap_process =		softvol_ap_process,
	.ap_release =		softvol_ap_release,
	.ap_update_mode =	softvol_ap_update_mode,
	.ap_setup =		softvol_ap_setup,
	.mode =			AP_MODE_BYPASS,
};
