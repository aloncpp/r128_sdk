/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _BT_APP_AUDIO_H_
#define _BT_APP_AUDIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sys_ctrl/sys_ctrl.h"

typedef enum {
	BT_APP_AUDIO_EVENTS_A2DP_START,
	BT_APP_AUDIO_EVENTS_A2DP_STOP,
	BT_APP_AUDIO_EVENTS_HFP_START,
	BT_APP_AUDIO_EVENTS_HFP_STOP,
} bt_app_audio_events;

typedef enum {
	BT_APP_AUDIO_TYPE_ERROR,
	BT_APP_AUDIO_TYPE_A2DP,
	BT_APP_AUDIO_TYPE_HFP,
} bt_audio_type_t;

/* A2DP global state */
typedef enum {
	APP_AV_STATE_IDLE,
	APP_AV_STATE_CONNECTED,
} bt_av_state;

typedef enum {
	APP_AV_MEDIA_FORWARD,
	APP_AV_MEDIA_BACKWARD,
	APP_AV_MEDIA_REOPEN,
} bt_av_media_ctrl;

/* sub states of APP_AV_STATE_CONNECTED */
typedef enum {
	APP_AV_MEDIA_STATE_IDLE,
	APP_AV_MEDIA_STATE_STARTING,
	APP_AV_MEDIA_STATE_STARTED,
	APP_AV_MEDIA_STATE_SUSPEND,
	APP_AV_MEDIA_STATE_STOPPING,
	APP_AV_MEDIA_STATE_STOPPED,
} bt_av_media_state;
int bt_app_audio_init(void);

int bt_app_audio_deinit(void);

void bt_app_audio_config(uint32_t samplerate, uint32_t channels, bt_audio_type_t flag);//flag 0 a2dp 1 hfp

void bt_app_audio_ctrl(event_msg *msg);
bt_av_media_state bt_app_get_media_state(void);
int32_t bt_app_data_cb(uint8_t *data, int32_t len);
void bt_app_av_media_state_change(bt_av_media_state media_state);
// int bt_app_audio_write(const uint8_t *data, uint32_t len);

int bt_app_audio_write_unblock(const uint8_t *data, uint32_t len, bt_audio_type_t type);
int bt_app_audio_read_unblock(uint8_t *data, uint32_t len);
void bt_app_av_media_play_state_change(bt_av_media_ctrl event);
int8_t bt_app_set_system_audio_vol(uint32_t volume);
uint8_t bt_app_play_music(const char *name);
#ifdef __cplusplus
}
#endif

#endif /* _BT_APP_AUDIO_H_ */