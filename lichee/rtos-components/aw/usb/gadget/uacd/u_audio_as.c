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

#include <ringbuffer.h>

#include "u_audio.h"

#include <AudioSystem.h>

#define U_AUDIO_SINK_AS_NAME	"default"

int u_audio_sink_init(void **private_data, int rate, int ch, int bits)
{
	int ret;
	tAudioTrack *at;

	uacd_verbose("");
	at = AudioTrackCreate(U_AUDIO_SINK_AS_NAME);
	if (!at)
		return -1;
	ret = AudioTrackSetup(at, rate, ch, bits);
	if (ret != 0)
		goto err;

	*private_data = (void *)at;
	uacd_verbose("");

	return 0;
err:
	if (at)
		AudioTrackDestroy(at);
	return -1;
}

int u_audio_sink_release(void *data)
{
	tAudioTrack *at = (tAudioTrack *)data;

	if (at)
		AudioTrackDestroy(at);
	return 0;
}

int u_audio_sink_work(void *data, void *buf, int size)
{
	int ret;
	tAudioTrack *at = (tAudioTrack *)data;

	ret = AudioTrackWrite(at, buf, size);
	uacd_verbose("size=%d, ret=%d", size, ret);
	return ret;
}

int u_audio_source_init(void **private_data, int rate, int ch, int bits)
{
	int ret;
	tAudioRecord *ar;

	uacd_verbose("");
	ar = AudioRecordCreate(U_AUDIO_SINK_AS_NAME);
	if (!ar)
		return -1;
	ret = AudioRecordSetup(ar, rate, ch, bits);
	if (ret != 0)
		goto err;

	*private_data = (void *)ar;
	uacd_verbose("");

	return 0;
err:
	if (ar)
		AudioRecordDestroy(ar);
	return -1;
}

int u_audio_source_release(void *data)
{
	tAudioRecord *ar = (tAudioRecord *)data;

	if (ar)
		AudioRecordDestroy(ar);
	return 0;
}

int u_audio_source_work(void *data, void *buf, int size)
{
	int ret;
	tAudioRecord *ar = (tAudioRecord *)data;

	ret = AudioRecordRead(ar, buf, size);
	uacd_verbose("size=%d, ret=%d", size, ret);
	return ret;
}
