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

#ifndef _BLINK_APP_H_
#define _BLINK_APP_H_


#ifdef __cplusplus
extern "C" {
#endif

typedef struct wlan_blink_param {
	struct netif *nif;
} wlan_blink_param_t;

typedef struct wlan_blink_result {
	uint8_t ssid[WLAN_SSID_MAX_LEN];
	uint8_t ssid_len;
	uint8_t passphrase[WLAN_PASSPHRASE_MAX_LEN + 1]; /* ASCII string ending with '\0' */
	uint8_t random_num;
} wlan_blink_result_t;

typedef enum {
	WLAN_BLINK_SUCCESS = 0,   /* success */
	WLAN_BLINK_FAIL    = -1,  /* general error */
	WLAN_BLINK_TIMEOUT = -2,  /* wait timeout */
	WLAN_BLINK_INVALID = -3,  /* invalid argument */
} wlan_blink_ret_t;

typedef enum {
	WB_STATUS_NOTREADY = 0,
	WB_STATUS_NORMAL,
	WB_STATUS_COMPLETE,
	WB_STATUS_BUSY,
} wlan_blink_status_t;

/**
 * @brief Start to enter the network configuration scenario.
 * @param param Wlan and blink initialization parameters.
 * @return WLAN_BLINK_SUCCESS in case of success or else value in case of error.
 */
wlan_blink_ret_t wlan_blink_start(wlan_blink_param_t *param);

/**
 * @brief Wait to receive the message.
 * @param none.
 * @return WLAN_BLINK_SUCCESS in case of success or else value in case of error.
 */
wlan_blink_ret_t wlan_blink_wait_once(void);

/**
 * @brief To connnect the network.
 * @param timeout_ms Timeout for connection, unit as MS.
 * @return WLAN_BLINK_SUCCESS in case of success or else value in case of error.
 */
wlan_blink_ret_t wlan_blink_connect(uint32_t timeout_ms);

/**
 * @brief To get the status of wlan blink.
 * @param none.
 * @return WLAN_BLINK_SUCCESS in case of success or else value in case of error.
 */
wlan_blink_status_t wlan_blink_get_status(void);

/**
 * @brief Exit the network configuration scenario.
 * @param none.
 * @return WLAN_BLINK_SUCCESS in case of success or else value in case of error.
 */
wlan_blink_ret_t wlan_blink_stop(void);


#ifdef __cplusplus
}
#endif

#endif /* _BLINK_APP_H_ */

