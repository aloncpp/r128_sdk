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
#define TAG	"AudioHWBT"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "audio_hw.h"
#include "AudioBase.h"

#include "local_audio_hw.h"

#include <bt_manager.h>
#include <ringbuffer.h>
#include <hal_event.h>


struct bt_attr {
	audio_hw_t *ahw;
	hal_ringbuffer_t rb;
	hal_event_t event;
};

static struct bt_attr *g_ba = NULL;
#define AS_BT_EV_DATA_GET (1 << 0)

static int bt_ahw_open(void *aa)
{
	_debug("");
	if (!btmg_a2dp_source_is_ready())
		return -1;

	btmg_a2dp_source_play_start();

	return 0;
}

static int data_put(struct bt_attr *ba, void *data, int len)
{
	int size = -1, ret = len;
	int residue = len;
	int timeout =  2000;
	int offset = 0;

	while (1) {
		/*_info("rb put data %d", residue);*/
		size = hal_ringbuffer_put(ba->rb, data + offset, residue);
		if (size == residue)
			break;
		residue -= size;
		offset += size;
		/*_info("wait... offset=%d, residue=%d", offset, residue);*/
		ret = hal_event_wait(ba->event, AS_BT_EV_DATA_GET,
			HAL_EVENT_OPTION_CLEAR | HAL_EVENT_OPTION_AND, timeout);
		/*_info("wait return %d", ret);*/
		if (!ret) {
			ret = -1;
			_info("wait data timeout:%d", timeout);
			break;
		}
	}

	return ret < 0 ? ret : len;
}

static int wait_bt_data_avail(struct rb_attr *rb, uint32_t frames)
{
	int timeout = 100;

	while (timeout > 0) {
		int interval = 5;
		if (rb->appl_ptr - rb->hw_ptr > frames)
			return 0;
		as_msleep(interval);
		timeout -= interval;
	}
	return -1;
}

static int bt_ahw_write(struct bt_attr *ba, struct rb_attr *rb)
{
	audio_hw_t *ahw = ba->ahw;
	as_pcm_config_t *pcm_config = audio_hw_pcm_config(ahw);

	snd_pcm_sframes_t size;
	snd_pcm_uframes_t frames_total = pcm_config->period_frames;
	snd_pcm_uframes_t frames_loop = 441;
	snd_pcm_uframes_t frames_count = 0;
	snd_pcm_uframes_t frames = 0, cont, offset;
	uint32_t frame_bytes = pcm_config->frame_bytes;
	void *data;


	while (1) {
		if (!btmg_a2dp_source_is_ready())
			return -1;
		if ((frames_total - frames_count) < frames_loop)
			frames = frames_total - frames_count;
		if (!frames)
			frames = frames_loop;
		cont = pcm_config->buffer_frames -
			rb->hw_ptr % pcm_config->buffer_frames;
		if (frames > cont)
			frames = cont;
		if (wait_bt_data_avail(rb, frames) < 0)
			return 0;
		offset = rb->hw_ptr % pcm_config->buffer_frames;
		data = rb->rb_buf + offset * frame_bytes;

		size = data_put(ba, data, frames * frame_bytes);
		/*_info("put data, %d return %d\n", frames * frame_bytes, size);*/
		if (size != frames * frame_bytes) {
			_err("rb put failed, return %d", size);
			as_msleep(10);
		}
		if (size < 0)
			continue;
		rb->hw_ptr += size / frame_bytes;
		if (rb->hw_ptr >= pcm_config->boundary)
			rb->hw_ptr -= pcm_config->boundary;
		/* TODO, fix boundary */
		/*_debug("update mixer_ptr:%u, ofs:%u", rb->hw_ptr, rb->hw_ptr % as_pcm->pcm_config.buffer_frames);*/
		/* fill silence data */
		memset(data, 0x0, size);
		frames_count += size / frame_bytes;
		frames -= size / frame_bytes;
		if (frames_total == frames_count)
			break;
	}
	return frames_count;
}

static int bt_ahw_close(void *aa)
{
	_debug("");
	btmg_a2dp_source_play_stop(true);
	return 0;
}

static void connection_state_report(const char *bd_addr, btmg_a2dp_source_connection_state_t state)
{
	_debug("state=%d", state);

	if (state == BTMG_A2DP_SOURCE_DISCONNECTED) {
		_info("disconnectd...");
	} else if (state == BTMG_A2DP_SOURCE_CONNECTING) {
		_info("connecting...");
	} else if (state == BTMG_A2DP_SOURCE_CONNECTED) {
		_info("connected...");
	} else if (state == BTMG_A2DP_SOURCE_DISCONNECTING) {
		_info("disconnecting...");
	}
	return;
}

static void audio_state_report(const char *bd_addr, btmg_a2dp_source_audio_state_t state)
{
	_info("");

}

static int32_t audio_data_report(uint8_t *data, int32_t len)
{
	int ret = 0;

	ret = hal_ringbuffer_get(g_ba->rb, data, len, -1);
	/*_debug("len=%d, ret=%d", len, ret);*/
	if (ret < 0) {
		_err("rb get failed, return %d", ret);
		return -1;
	}
	hal_event_set_bits(g_ba->event, AS_BT_EV_DATA_GET);
	return ret;
}

btmg_callback_t g_cb;
btmg_a2dp_source_callback_t g_a2dp_source_cb = {
	.conn_state_cb 	= connection_state_report,
	.audio_state_cb = audio_state_report,
	.audio_data_cb 	= audio_data_report,
};

static int bt_ahw_init(audio_hw_t *ahw)
{
	struct bt_attr *ba;
	as_pcm_config_t *pcm_config;
	uint32_t rb_size = 0;

	_debug("");
	ba = as_alloc(sizeof(struct bt_attr));
	if (!ba) {
		_err("no memory");
		goto err;
	}

	g_ba = ba;
	ba->ahw = ahw;
	audio_hw_set_private_data(ahw, ba);

	pcm_config = audio_hw_pcm_config(ahw);
	pcm_config->rate = 44100;
	pcm_config->channels = 2;
	pcm_config->format = SND_PCM_FORMAT_S16_LE;
	pcm_config->period_frames = 882;
	pcm_config->periods = 4;

	pcm_config->frame_bytes = format_to_bits(pcm_config->format) / 8 \
			* pcm_config->channels;
	pcm_config->buffer_frames = pcm_config->period_frames * pcm_config->periods;
	pcm_config->boundary = pcm_config->buffer_frames;
	while (pcm_config->boundary * 2 <= LONG_MAX - pcm_config->buffer_frames)
		pcm_config->boundary *= 2;

	rb_size = pcm_config->buffer_frames * pcm_config->frame_bytes;
	ba->rb = hal_ringbuffer_init(rb_size);
	_debug("ba rb size:%u", rb_size);
	if (!ba->rb) {
		_err("rb init failed");
		goto err;
	}

	ba->event = hal_event_create();

	memset(&g_cb, 0, sizeof(g_cb));
	g_cb.btmg_a2dp_source_cb = g_a2dp_source_cb;

	printf("[%s] line:%d \n", __func__, __LINE__);
	btmg_audiosystem_register_cb(&g_cb);

	return 0;
err:
	return -1;
}

static int bt_ahw_destroy(audio_hw_t *ahw)
{
	struct bt_attr *ba;

	ba = audio_hw_get_private_data(ahw);
	if (!ba)
		return 0;
	hal_event_delete(ba->event);
	as_free(ba);
	audio_hw_set_private_data(ahw, NULL);
	g_ba = NULL;

	return 0;
}

/*
 * amp ahw
 * instance:	AUDIO_HW_TYPE_BT
 * ops:		bt(a2dp source) api
 * */
static struct audio_hw_ops bt_ops = {
	.ahw_open = (ahw_func)bt_ahw_open,
	.ahw_read = NULL,
	.ahw_write = (ahw_func_xfer)bt_ahw_write,
	.ahw_close = (ahw_func)bt_ahw_close,
	.ahw_init = (ahw_func)bt_ahw_init,
	.ahw_destroy = (ahw_func)bt_ahw_destroy,
};

struct audio_hw_elem g_bt_ahw = {
	.name = "bt_src",
	.instance = AUDIO_HW_TYPE_BT,
	.card_name_pb = NULL,
	.card_name_cap = NULL,
	.ops = &bt_ops,
};
