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

#ifndef _BLINK_H_
#define _BLINK_H_

#include "net/wlan/wlan_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	BLINK_INDICATE_SUCCESS = 0,
	BLINK_INDICATE_FAIL,
} blink_indicate_t;

typedef void (*set_state_callback)(blink_indicate_t state);

typedef struct blink_param {
	uint8_t dev_filter;
	set_state_callback cb;
} blink_param_t;

typedef struct blink_result {
	uint8_t ssid[WLAN_SSID_MAX_LEN];
	uint8_t ssid_len;
	uint8_t passphrase[WLAN_PASSPHRASE_MAX_LEN + 1]; /* ASCII string ending with '\0' */
	uint8_t passphrase_len;
} blink_result_t;

typedef enum {
	BLINK_STATE_SUCCESS = 0,
	BLINK_STATE_FAIL,
} blink_state_t;

typedef enum {
	BLINK_OK      = 0,     /* success */
	BLINK_ERROR   = -1,    /* general error */
	BLINK_BUSY    = -2,    /* device or resource busy */
	BLINK_TIMEOUT = -3,    /* wait timeout */
	BLINK_INVALID = -4,    /* invalid argument */
} blink_ret_t;

/**
 * @brief Start to enter the blink(BLE link) scenario.
 * @param param Blink initialization parameters.
 * @return BLINK_OK in case of success or else value in case of error.
 */
blink_ret_t blink_start(blink_param_t *param);

/**
 * @brief Wait to receive the message by LE.
 * @param timeout Timeout for receiving, unit as MS.
 * @return BLINK_OK in case of success or else value in case of error.
 */
blink_ret_t blink_wait(uint32_t timeout);

/**
 * @brief Get the raw data after receiving message.
 * @param type Type for raw data.
 * @param result Pointer to the data buffer.
 * @return BLINK_OK in case of success or else value in case of error.
 */
blink_ret_t blink_get_result(blink_result_t *result);

/**
 * @brief Indicate the network configuration status for the client.
 * @param state Status for the network configuration.
 * @return BLINK_OK in case of success or else value in case of error.
 */
blink_ret_t blink_set_state(blink_state_t state);

/**
 * @brief Exit the blink scenario.
 * @param none.
 * @return BLINK_OK in case of success or else value in case of error.
 */
blink_ret_t blink_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* _BLINK_H_ */

