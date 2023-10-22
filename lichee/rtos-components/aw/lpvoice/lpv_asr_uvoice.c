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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <AudioSystem.h>

#include <hal_time.h>
#include <hal_sem.h>
#include <hal_thread.h>

#include <uvoice_asr_api.h>

typedef struct lpv_asr {

	void *asr_streaming_handle;
	void *asr_recognizer_handle;

	uint32_t rate;
	uint32_t channels;
	uint32_t bits;
} lpv_asr_t;

void *lpv_asr_init(uint32_t rate, uint32_t ch, uint32_t bits)
{
	uv_audio_code uret;
	lpv_asr_t *la = NULL;

	la = malloc(sizeof(lpv_asr_t));
	if (!la)
		return NULL;
	memset(la, 0, sizeof(lpv_asr_t));

	la->rate = rate;
	la->channels = ch;
	la->bits = bits;

	uret = uvoice_sdk_streaming_init(&la->asr_streaming_handle);
	if (uret != UV_SDK_STREAMING_OK)
		goto err;

	uret = uvoice_sdk_recognizer_init(&la->asr_recognizer_handle);
	if (uret != UV_SDK_RECOGNIZER_OK)
		goto err;

	uvoice_sdk_streaming_enable_vad(la->asr_streaming_handle);

	return la;
err:
	return NULL;
}

int lpv_asr_process(void *handle, void *data, uint32_t size, void *out_data,
		int *word_id, int *vad_flag, float *confidence)
{
	lpv_asr_t *la = (lpv_asr_t *)handle;
	uvoice_stream audio;
	uint32_t frame_bytes = 0;
	uv_audio_code uret;

	if (!handle)
		return -1;

	frame_bytes = la->channels * (la->bits / 8);
	/* 3channels: 2MIC + 1AEC */
	memset(&audio, 0, sizeof(audio));
	audio.audioin[0] = (int16_t *)(data);
	audio.audioin[1] = (int16_t *)(data + size / la->channels);
	audio.audioref[0] = (int16_t *)(data + size * 2 / la->channels);

	uret = uvoice_sdk_streaming_process(la->asr_streaming_handle, &audio,
		(int16_t *)out_data, vad_flag, size / frame_bytes);
	if (uret != UV_SDK_RECOGNIZER_OK) {
		printf("uvoice streaming process error\n");
		return -1;
	}

	uret = uvoice_sdk_recognizer_process(la->asr_recognizer_handle, out_data,
		word_id, confidence);
	if (uret != UV_SDK_RECOGNIZER_OK) {
		printf("recognizer process error\n");
		return -1;
	}

	return 0;
}

int lpv_asr_destroy(void *handle)
{
	lpv_asr_t *la = (lpv_asr_t *)handle;

	if (!handle)
		return -1;
	if (la->asr_recognizer_handle)
		uvoice_sdk_recognizer_release(la->asr_recognizer_handle);

	if (la->asr_streaming_handle) {
		uvoice_sdk_streaming_disable_vad(la->asr_streaming_handle);
		uvoice_sdk_streaming_release(la->asr_streaming_handle);
	}
	return 0;
}
