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
#ifndef __AUDIOTRACK_H
#define __AUDIOTRACK_H

#include <stdint.h>

typedef struct AudioTrack tAudioTrack;

enum AUDIO_STREAM_TYPE {
	AUDIO_STREAM_UNKNOWN = 0,
	AUDIO_STREAM_SYSTEM,
	AUDIO_STREAM_MUSIC,
	AUDIO_STREAM_NOTIFICATION,
	AUDIO_STREAM_BT_A2DP_SINK = 20,
	AUDIO_STREAM_BT_A2DP_SOURCE,
	AUDIO_STREAM_MAX = 128,
};

tAudioTrack *AudioTrackCreate(const char *name);
tAudioTrack *AudioTrackCreateWithStream(const char *name, uint16_t type);
int AudioTrackSetup(tAudioTrack *at, uint32_t rate, uint8_t channels, uint8_t bits);
/*
 * size: bytes
 * return value: bytes
 */
int AudioTrackWrite(tAudioTrack *at, void *data, uint32_t size);
int AudioTrackStart(tAudioTrack *at);
int AudioTrackStop(tAudioTrack *at);
int AudioTrackDelay(tAudioTrack *at);
int AudioTrackDestroy(tAudioTrack *at);

int AudioTrackSetupComponent(tAudioTrack *at, const char *name,
			void *data, unsigned int size);
int AudioTrackVolumeGet(tAudioTrack *at, uint32_t *index);
int AudioTrackVolumeSet(tAudioTrack *at, uint32_t index);
int AudioTrackVolumeGetRange(tAudioTrack *at, uint16_t *min, uint16_t *max);

int AudioTrackChannelMap(tAudioTrack *at, uint8_t *map, uint32_t size);
int AudioTrackResampleCtrl(tAudioTrack *at, int onoff);

#endif /* __AUDIOTRACK_H */
