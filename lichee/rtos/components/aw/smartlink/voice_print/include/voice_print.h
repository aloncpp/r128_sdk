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

#ifndef _VOICE_PRINT_H_
#define _VOICE_PRINT_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "kernel/os/os.h"
#include "smartlink/sc_assistant.h"
//#include "audio/pcm/audio_pcm.h"
//#include "audio/manager/audio_manager.h"
#include "net/wlan/wlan_defs.h"
#include "adt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* @brief voice print policy
 *  - policy 1: simple policy, the success rate is not very good
 *  - policy 2: improve success rate
 *
 * @note
 *  - all policies are incompatible
 *  - encoder(server) MUST use the same policy
 */
#define VOICE_PRINT_POLICY    2

typedef enum {
	WLAN_VOICEPRINT_SUCCESS  = 0,   /* success */
	WLAN_VOICEPRINT_FAIL     = -1,  /* general error */
	WLAN_VOICEPRINT_TIMEOUT  = -2,  /* wait timeout */
	WLAN_VOICEPRINT_INVALID  = -3,  /* invalid argument */
	WLAN_VOICEPRINT_OVERFLOW = -4,  /* buffer overflow */
} voiceprint_ret_t;

/*
 * decoder_fedpcm() return value define
 */
#define RET_DEC_ERROR    -1
#define RET_DEC_NORMAL   0
#define RET_DEC_NOTREADY 1
#define RET_DEC_END      2

typedef enum {
	VP_STATUS_NORMAL = 0,
	VP_STATUS_NOTREADY,
	VP_STATUS_DEC_ERROR,
	VP_STATUS_COMPLETE,
} voiceprint_status_t;

typedef struct voiceprint_param {
	uint8_t audio_card;
	struct netif *nif;
} voiceprint_param_t;

typedef struct wlan_voiceprint_result {
	uint8_t ssid[WLAN_SSID_MAX_LEN];
	uint8_t ssid_len;
	uint8_t passphrase[WLAN_PASSPHRASE_MAX_LEN + 1]; /* ASCII string ending with '\0' */
	uint8_t random_num;
} wlan_voiceprint_result_t;

int voiceprint_start(voiceprint_param_t *param);
int voiceprint_stop(uint32_t wait);
voiceprint_status_t voiceprint_get_status(void);
voiceprint_status_t voiceprint_wait_once(void);
voiceprint_ret_t voiceprint_wait(uint32_t timeout_ms);

voiceprint_ret_t wlan_voiceprint_get_raw_result(char *result, int *len);
voiceprint_ret_t wlan_voiceprint_connect_ack(struct netif *nif, uint32_t timeout_ms,
                                             wlan_voiceprint_result_t *result);

/*
 * Obsoleted APIs, for compatibility only
 */
static __inline int voice_print_start(struct netif *nif, const char *key)
{
	voiceprint_param_t param;
	param.audio_card = AUDIO_SND_CARD_DEFAULT;
	param.nif = nif;
	return voiceprint_start(&param);
}

#define voice_print_stop(wait)  voiceprint_stop(wait)
#define voice_print_wait_once() voiceprint_wait_once()
#define voice_print_wait(tmo)   voiceprint_wait(tmo)

#ifdef __cplusplus
}
#endif

#endif /* _VOICE_PRINT_H_ */
