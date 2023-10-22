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
#define TAG	"AudioHW"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "audio_hw.h"
#include "AudioBase.h"
#include "local_audio_hw.h"

#include "AudioSystem.h"



static LIST_HEAD(g_ahw_elem_list);


struct audio_hw {
	uint8_t stream;
	as_pcm_config_t pcm_config;

	struct audio_hw_elem *ahw_elem;
	void *private_data;
};

static struct ahw_alias_config  g_ahw_name_alias[] = {
        {
                "default", "playback", 0,
        },
        {
                "default", "capture", 1,
        },
};

__attribute__((weak)) void *get_ahw_name_alias(int *num)
{
	*num = ARRAY_SIZE(g_ahw_name_alias);

	return g_ahw_name_alias;
}

/*extern struct audio_hw_elem g_pcm_ahw;*/
void add_default_ahw(void)
{
	struct audio_hw_elem *a = NULL;
	struct list_head *head = &g_ahw_elem_list;

	_debug("");
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_PCM
	/*AHW_ADD_PCM_DEFAULT(g_pcm_ahw);*/
	AHW_ADD_PCM_DEFAULT(g_pcm_pb_ahw);
	AHW_ADD_PCM_DEFAULT(g_pcm_cap_ahw);
#endif
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_MULTI_PCM
	AHW_ADD_PCM_DEFAULT(g_pcm_multi_ahw);
#endif
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_AMP
	AHW_ADD_PCM_DEFAULT(g_amp_ahw);
	AHW_ADD_PCM_DEFAULT(g_amp_pb_ahw);
	AHW_ADD_PCM_DEFAULT(g_amp_cap_ahw);
#endif
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_BT
	AHW_ADD_PCM_DEFAULT(g_bt_ahw);
#endif
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_AW_EQ
	AHW_ADD_PCM_DEFAULT(g_eq_ahw);
#endif
	AS_MAYBE_UNUSED(a);
	AS_MAYBE_UNUSED(head);
	return;
}

#if 1
#include <console.h>
static int cmd_ahw_list(int argc, char *argv[])
{
	struct audio_hw_elem *elem;
	struct ahw_alias_config *palias = NULL;
	int num = 0;
	int i;

	printf("audio hw list:\n");
	printf("%-10s|%-12s|%-10s|%-10s\n", "instance", "name", "read", "write");
	list_for_each_entry(elem, &g_ahw_elem_list, list) {
		printf("%-10d|%-12s|%p|%p\n", elem->instance, elem->name, elem->ops->ahw_read, elem->ops->ahw_write);
	}
	palias = get_ahw_name_alias(&num);
	if (!palias)
		return 0;
	printf("-----------------------\n");
	printf("audio hw alias:\n");
	for (i = 0; i < num; i++) {
		printf("%c: %s --> %s\n", palias[i].stream ? 'c' : 'p',
				palias[i].alias_name, palias[i].ahw_name);
	}
	printf("\n");

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_ahw_list, ahw_list, audio system hw list);
#endif

static struct audio_hw_elem *find_alias_name(const char *name, int8_t stream)
{
	struct ahw_alias_config *palias = NULL;
	int num = 0;
	int i;

	palias = get_ahw_name_alias(&num);
	if (!palias)
		return NULL;
	for (i = 0; i < num; i++) {
		struct audio_hw_elem *elem = NULL;

		_debug("ahw alias name:%s vs %s stream:%d", palias[i].ahw_name, name, stream);
		if (palias[i].stream != stream)
			continue;
		if (strncmp(palias[i].alias_name, name, strlen(name) + 1))
			continue;
		list_for_each_entry(elem, &g_ahw_elem_list, list) {
			_debug("ahw elem name:%s vs %s", elem->name, palias[i].ahw_name);
			if (!strncmp(elem->name, palias[i].ahw_name, strlen(elem->name) + 1))
				return elem;
		}
	}

	return NULL;
}

static inline struct audio_hw_elem *find_audio_hw_elem(const char *name, int8_t stream)
{
	struct audio_hw_elem *elem = NULL;

	list_for_each_entry(elem, &g_ahw_elem_list, list) {
		_debug("ahw elem name:%s vs %s stream:%d", elem->name, name, stream);
		if (!strncmp(elem->name, name, strlen(elem->name) + 1))
			return elem;
	}

	elem = find_alias_name(name, stream);
	if (elem)
		return elem;

	return NULL;
}

audio_hw_t *audio_hw_init(const char *name, int8_t stream)
{
	int ret;
	audio_hw_t *ahw;

	_debug("stream:%d", stream);
	if (list_empty(&g_ahw_elem_list)) {
		add_default_ahw();
	}

	/* as_alloc once */
	ahw = as_alloc(sizeof(audio_hw_t));
	if (!ahw) {
		_err("no memory");
		goto err;
	}

	ahw->stream = stream;

	_debug("");
	ahw->ahw_elem = find_audio_hw_elem(name, stream);
	if (!ahw->ahw_elem) {
		_err("ahw elem:%s not found", name);
		goto err;
	}

	_debug("");
	if (!ahw->ahw_elem->ops || !ahw->ahw_elem->ops->ahw_init)
		goto err;
	_debug("");
	ret = ahw->ahw_elem->ops->ahw_init(ahw);
	if (ret != 0)
		_err("ahw_init failed");

	if (ahw->pcm_config.periods > 8) {
		_info("periods=%d is large", ahw->pcm_config.periods);
	}
	if (ahw->pcm_config.period_frames > 2048) {
		_info("period_frames=%d is large", ahw->pcm_config.period_frames);
	}
	_debug("rate:%u, channels:%u, format:0x%x, period size:%u, periods:%u, frame_bytes:%u, boundary:%lu",
			ahw->pcm_config.rate, ahw->pcm_config.channels,
			ahw->pcm_config.format, ahw->pcm_config.period_frames,
			ahw->pcm_config.periods, ahw->pcm_config.frame_bytes,
			ahw->pcm_config.boundary);


	return ahw;
err:
	if (ahw != NULL)
		as_free(ahw);
	return NULL;
}

int audio_hw_destroy(audio_hw_t *ahw)
{
	if (!ahw)
		return 0;
	if (ahw->ahw_elem && ahw->ahw_elem->ops && ahw->ahw_elem->ops->ahw_destroy)
		ahw->ahw_elem->ops->ahw_destroy(ahw);
	as_free(ahw);
	return 0;
}

int audio_hw_open(audio_hw_t *ahw)
{
	struct audio_hw_elem *elem = ahw->ahw_elem;

	return elem->ops->ahw_open(ahw->private_data);
}
int audio_hw_close(audio_hw_t *ahw)
{
	struct audio_hw_elem *elem = ahw->ahw_elem;

	return elem->ops->ahw_close(ahw->private_data);
}
int audio_hw_write(audio_hw_t *ahw, struct rb_attr *rb)
{
	struct audio_hw_elem *elem = ahw->ahw_elem;

	return elem->ops->ahw_write(ahw->private_data, rb);
}

int audio_hw_read(audio_hw_t *ahw, struct rb_attr *rb)
{
	struct audio_hw_elem *elem = ahw->ahw_elem;

	return elem->ops->ahw_read(ahw->private_data, rb);
}

int audio_hw_instance(audio_hw_t *ahw)
{
	struct audio_hw_elem *elem = ahw->ahw_elem;

	return elem->instance;
}

const char *audio_hw_name(audio_hw_t *ahw)
{
	struct audio_hw_elem *elem = ahw->ahw_elem;

	return elem->name;
}

int audio_hw_name_to_instance(const char *name, int8_t stream)
{
	struct audio_hw_elem *elem;

	elem = find_audio_hw_elem(name, stream);
	if (!elem)
		return -1;
	return elem->instance;
}

as_pcm_config_t *audio_hw_pcm_config(audio_hw_t *ahw)
{
	return &ahw->pcm_config;
}

uint8_t audio_hw_stream(audio_hw_t *ahw)
{
	return ahw->stream;
}

struct audio_hw_elem *audio_hw_elem_item(audio_hw_t *ahw)
{
	return ahw->ahw_elem;
}

int audio_hw_time_to_frames(audio_hw_t *ahw, uint32_t ms)
{
	as_pcm_config_t *pcm_config = &ahw->pcm_config;

	if (!ms)
		return 0;
	return pcm_config->rate * ms / 1000;

}

void audio_hw_set_private_data(audio_hw_t *ahw, void *pdata)
{
	ahw->private_data = pdata;
}

void *audio_hw_get_private_data(audio_hw_t *ahw)
{
	return ahw->private_data;
}
