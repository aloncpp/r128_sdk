/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
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
#include <unistd.h>
#include <console.h>
#include <aw_common.h>
#include <sunxi_hal_usb.h>

#include <hal_thread.h>
#include <hal_event.h>
#include <hal_time.h>

#include <ringbuffer.h>

#include "u_audio.h"

typedef struct u_audio {
	hal_thread_t handle_source;
	hal_thread_t handle_sink;

	int iso_ep;

	hal_event_t ev_source;
	hal_event_t ev_sink;

	void *data_source;
	void *data_sink;

	uint32_t rb_size;
	hal_ringbuffer_t rb;

	uint32_t rate;
	uint32_t ch;
	uint32_t bits;

	uint32_t audio_buf_size;
	void *audio_buf;

	int8_t running;
} u_audio_t;
/* host -> device, PC playback, device capture */
/* device -> host, PC capture, device playback */

#define USB_ISO_IN_IDX		(1)
#define USB_ISO_OUT_IDX		(2)

#define U_AUDIO_START		(1 << 0)
#define U_AUDIO_STOP		(1 << 1)

static u_audio_t *g_audio[2] = {0};

static void usb_capture_source_task(void *arg);
static void usb_capture_sink_task(void *arg);
static void usb_playback_source_task(void *arg);
static void usb_playback_sink_task(void *arg);


__attribute__((weak)) int u_audio_sink_init(void **private_data, int rate, int ch, int bits)
{
	return 0;
}

__attribute__((weak)) int u_audio_sink_release(void *data)
{
	return 0;
}

__attribute__((weak)) int u_audio_sink_work(void *data, void *buf, int size)
{
	uacd_err("Error!!!");
	hal_msleep(1000);
	return 0;
}

__attribute__((weak)) int u_audio_source_init(void **private_data, int rate, int ch, int bits)
{
	return 0;
}

__attribute__((weak)) int u_audio_source_release(void *data)
{
	return 0;
}

__attribute__((weak)) int u_audio_source_work(void *data, void *buf, int size)
{
	uacd_err("Error!!!");
	hal_msleep(1000);
	return 0;
}

int u_audio_init(int stream, int rate, int ch, int bits)
{
	u_audio_t *u;

	uacd_debug("");
	if (stream > 1)
		return -1;

	u = calloc(1, sizeof(u_audio_t));
	if (!u) {
		uacd_err("no memory\n");
		return -1;
	}

	if (stream)
		u->iso_ep = USB_ISO_OUT_IDX;
	else
		u->iso_ep = USB_ISO_IN_IDX;

	u->ev_source = hal_event_create();
	if (!u->ev_source)
		goto err;
	u->ev_sink = hal_event_create();
	if (!u->ev_sink)
		goto err;

	/* 100 ms */
	u->rb_size = rate * ch * bits / 8 * 100 / 1000;
	u->rb = hal_ringbuffer_init(u->rb_size);
	if (!u->rb)
		goto err;

	/* interval, 1ms */
	u->audio_buf_size = rate * ch * bits / 8 * 1 / 1000;
	u->audio_buf = calloc(1, u->audio_buf_size);
	if (!u->audio_buf)
		goto err;

	u->rate = rate;
	u->ch = ch;
	u->bits = bits;

	uacd_info("stream=%d, rate=%d, ch=%d, bits=%d, audio_buf_size=%u",
			stream, rate, ch, bits, u->audio_buf_size);

	g_audio[stream] = u;

	if (stream) {
		g_audio[stream]->handle_source = hal_thread_create(usb_capture_source_task, g_audio[stream],
				"usb_cap_src", 2048, HAL_THREAD_PRIORITY_APP);
		g_audio[stream]->handle_sink = hal_thread_create(usb_capture_sink_task, g_audio[stream],
				"usb_cap_sink", 4096, HAL_THREAD_PRIORITY_APP);
	} else {
		g_audio[stream]->handle_source = hal_thread_create(usb_playback_source_task, g_audio[stream],
				"usb_pb_src", 2048, HAL_THREAD_PRIORITY_APP);
		g_audio[stream]->handle_sink = hal_thread_create(usb_playback_sink_task, g_audio[stream],
				"usb_pb_sink", 4096, HAL_THREAD_PRIORITY_APP);
	}


	return 0;
err:
	uacd_err("");
	if (u->ev_source)
		hal_event_delete(u->ev_source);
	if (u->ev_sink)
		hal_event_delete(u->ev_sink);
	if (u->rb)
		hal_ringbuffer_release(u->rb);
	if (u->audio_buf)
		free(u->audio_buf);
	if (u)
		free(u);
	return -1;
}

static void usb_capture_source(u_audio_t *u)
{
	int size = 0;
	int bytes = u->audio_buf_size;

	uacd_debug("");
	while (u->running) {
		uacd_verbose("ep:0x%x, size:%d\n", u->iso_ep, bytes);
		size = usb_gadget_function_read_timeout(u->iso_ep, u->audio_buf, bytes, 1000);
		if (size != bytes) {
			uacd_err("usb read error:%d", size);
			continue;
		}
		uacd_verbose("usb read size %d\n", size);
		size = hal_ringbuffer_wait_put(u->rb, u->audio_buf, bytes, 2000);
		if (size <= 0)
			uacd_err("hal_ringbuffer_wait_put return %d", size);
		/*size = hal_ringbuffer_force_put(u->rb, u->audio_buf, bytes);*/
	}
	uacd_info("running:%d", u->running);
	return;
}

static void usb_capture_source_task(void *arg)
{
	u_audio_t *u = (u_audio_t *)arg;
	uint32_t timeout = 10000;
	hal_event_bits_t bits;

	while (1) {
		uacd_verbose("");
		bits = 0;
		bits = hal_event_wait(u->ev_source, U_AUDIO_START,
				HAL_EVENT_OPTION_CLEAR | HAL_EVENT_OPTION_AND, timeout);
		uacd_verbose("bits:0x%x", bits);
		if (!bits)
			continue;
		if (bits & U_AUDIO_START) {
			uacd_verbose("");
			usb_capture_source(u);
		}
	}

	u->handle_source = NULL;
	hal_thread_stop(NULL);
}

static void usb_capture_sink(u_audio_t *u)
{
	int size = 0;
	uint32_t bytes = u->rate * u->ch * u->bits / 8 * 20 / 1000; /* 20ms */
	void *buf = NULL;
	int timeout = 1000;

	if (u_audio_sink_init(&u->data_sink, u->rate, u->ch, u->bits) != 0)
		return;

	uacd_verbose("bytes=%u", bytes);
	buf = malloc(bytes);
	if (!buf)
		return;

	uacd_debug("running:%d, bytes=%u", u->running, bytes);
	while (u->running) {
		memset(buf, 0, bytes);
		size = hal_ringbuffer_get(u->rb, buf, bytes, 1000);
		uacd_verbose("size=%d, bytes=%d", size, bytes);
		if (!size)
			continue;
		u_audio_sink_work(u->data_sink, buf, size);
	}
	uacd_info("running:%d", u->running);

	free(buf);
	u_audio_sink_release(u->data_sink);
	u->data_sink = NULL;

	return;
}

static void usb_capture_sink_task(void *arg)
{
	u_audio_t *u = (u_audio_t *)arg;
	uint32_t timeout = 10000;
	hal_event_bits_t bits;

	while (1) {
		uacd_verbose("");
		bits = 0;
		bits = hal_event_wait(u->ev_sink, U_AUDIO_START,
				HAL_EVENT_OPTION_CLEAR | HAL_EVENT_OPTION_AND, timeout);
		uacd_verbose("bits:0x%x", bits);
		if (!bits)
			continue;
		if (bits & U_AUDIO_START) {
			uacd_verbose("");
			usb_capture_sink(u);
		}
	}

	u->handle_source = NULL;
	hal_thread_stop(NULL);
}

int u_audio_start_capture(void *arg)
{
	u_audio_t *u = g_audio[1];

	uacd_info("");
	u->running = 1;
	hal_event_set_bits(u->ev_source, U_AUDIO_START);
	hal_event_set_bits(u->ev_sink, U_AUDIO_START);
	return 0;
}

int u_audio_stop_capture(void *arg)
{
	u_audio_t *u = g_audio[1];

	u->running = 0;
	uacd_info("");
	return 0;
}

/* USB Playback(MIC->capture->device->playback->USB->PC, means USB device playback)*/

static void usb_playback_source(u_audio_t *u)
{
	int size = 0;
	uint32_t bytes = u->rate * u->ch * u->bits / 8 * 20 / 1000; /* 20ms */
	void *buf = NULL;
	int timeout = 1000;

	if (u_audio_source_init(&u->data_source, u->rate, u->ch, u->bits) != 0)
		return;

	uacd_verbose("bytes=%u", bytes);
	buf = malloc(bytes);
	if (!buf)
		return;

	uacd_debug("running:%d, bytes=%u", u->running, bytes);
	while (u->running) {
		memset(buf, 0, bytes);
		size = u_audio_source_work(u->data_source, buf, bytes);
		if (size != bytes) {
			uacd_err("u_audio_source_work error, bytes=%u, size=%d", bytes, size);
			continue;
		}
		/*uacd_err("before size=%d, bytes=%d", size, bytes);*/
		size = hal_ringbuffer_wait_put(u->rb, buf, bytes, 2000);
		/*size = hal_ringbuffer_force_put(u->rb, buf, bytes);*/
		/*uacd_err("after size=%d, bytes=%d", size, bytes);*/
		if (size != bytes)
			uacd_err("hal_ringbuffer_wait_put error, bytes=%u, size=%d", bytes, size);
		/*if (size <= 0)*/
			/*uacd_err("hal_ringbuffer_wait_put return %d", size);*/
	}
	uacd_info("running:%d", u->running);

	free(buf);
	u_audio_source_release(u->data_source);
	u->data_source = NULL;

	return;
}

static void usb_playback_source_task(void *arg)
{
	u_audio_t *u = (u_audio_t *)arg;
	uint32_t timeout = 10000;
	hal_event_bits_t bits;

	while (1) {
		uacd_verbose("");
		bits = 0;
		bits = hal_event_wait(u->ev_source, U_AUDIO_START,
				HAL_EVENT_OPTION_CLEAR | HAL_EVENT_OPTION_AND, timeout);
		uacd_verbose("bits:0x%x", bits);
		if (!bits)
			continue;
		if (bits & U_AUDIO_START) {
			uacd_verbose("");
			usb_playback_source(u);
		}
	}

	u->handle_source = NULL;
	hal_thread_stop(NULL);
}

static void usb_playback_sink(u_audio_t *u)
{
	int size = 0;
	int bytes = u->audio_buf_size;

	uacd_debug("");
	while (u->running) {
		/*uacd_err("ep:0x%x, size:%d", u->iso_ep, bytes);*/
		size = hal_ringbuffer_get(u->rb, u->audio_buf, bytes, 1000);
		if (size != bytes) {
			uacd_err("rb get error:%d, bytes=%d", size, bytes);
			continue;
		}
		size = usb_gadget_function_write(u->iso_ep, u->audio_buf, bytes);
		if (size != bytes) {
			uacd_err("usb write error:%d", size);
			return;
		}
		/*uacd_err("usb write size %d", size);	*/
	}
	uacd_info("running:%d", u->running);
	return;
}

static void usb_playback_sink_task(void *arg)
{
	u_audio_t *u = (u_audio_t *)arg;
	uint32_t timeout = 10000;
	hal_event_bits_t bits;

	while (1) {
		uacd_verbose("");
		bits = 0;
		bits = hal_event_wait(u->ev_sink, U_AUDIO_START,
				HAL_EVENT_OPTION_CLEAR | HAL_EVENT_OPTION_AND, timeout);
		uacd_verbose("bits:0x%x", bits);
		if (!bits)
			continue;
		if (bits & U_AUDIO_START) {
			uacd_verbose("");
			usb_playback_sink(u);
		}
	}

	u->handle_sink = NULL;
	hal_thread_stop(NULL);
}


int u_audio_start_playback(void *arg)
{
	u_audio_t *u = g_audio[0];

	uacd_info("");
	u->running = 1;
	hal_event_set_bits(u->ev_source, U_AUDIO_START);
	hal_event_set_bits(u->ev_sink, U_AUDIO_START);
	return 0;
}

int u_audio_stop_playback(void *arg)
{
	u_audio_t *u = g_audio[0];

	u->running = 0;
	uacd_verbose("");
	uacd_info("");
}
