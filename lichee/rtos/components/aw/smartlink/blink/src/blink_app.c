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

#include <stdio.h>
#include <stdlib.h>

#include "kernel/os/os.h"
#include "blink.h"
#include "smartlink/sc_assistant.h"
#include "blink_app.h"

#define WLAN_BLINK_DBG_ON    0
#define WLAN_BLINK_INF_ON    1
#define WLAN_BLINK_WRN_ON    1
#define WLAN_BLINK_ERR_ON    1

#define WLAN_BLINK_LOG(flags, fmt, arg...)  \
	do {                                    \
		if (flags)                          \
			printf(fmt, ##arg);             \
	} while (0)

#define WB_DBG(fmt, arg...) WLAN_BLINK_LOG(WLAN_BLINK_DBG_ON, "[WB DBG] "fmt, ##arg)
#define WB_INF(fmt, arg...) WLAN_BLINK_LOG(WLAN_BLINK_INF_ON, "[WB INF] "fmt, ##arg)
#define WB_WRN(fmt, arg...) WLAN_BLINK_LOG(WLAN_BLINK_WRN_ON, "[WB WRN] "fmt, ##arg)
#define WB_ERR(fmt, arg...)                                           \
	do {                                                              \
		WLAN_BLINK_LOG(WLAN_BLINK_ERR_ON, "[WB ERR] %s():%d, "fmt,    \
		               __func__, __LINE__, ##arg);                    \
	} while (0)

typedef struct wlan_blink_priv {
	wlan_blink_status_t status;
	struct netif *nif;
} wlan_blink_priv_t;

static wlan_blink_priv_t *wlan_blink = NULL;

wlan_blink_ret_t wlan_blink_start(wlan_blink_param_t *param)
{
	wlan_blink_priv_t *priv = wlan_blink;

	WB_DBG("%s %d\n", __func__, __LINE__);

	if (priv != NULL) {
		WB_WRN("wlan blink has started\n");
		return WLAN_BLINK_INVALID;
	}
	if (param == NULL || param->nif == NULL) {
		WB_ERR("invalid param\n");
		return WLAN_BLINK_INVALID;
	}

	priv = malloc(sizeof(wlan_blink_priv_t));
	if (priv == NULL) {
		WB_ERR("malloc fail\n");
		return WLAN_BLINK_FAIL;
	}
	memset(priv, 0, sizeof(wlan_blink_priv_t));

	priv->nif = param->nif;

	blink_param_t blink_param;
	memset(&blink_param, 0, sizeof(blink_param_t));
	if (blink_start(&blink_param) != BLINK_OK) {
		WB_ERR("start fail\n");
		goto err;
	}
	priv->status = WB_STATUS_NORMAL;
	wlan_blink = priv;

	return WLAN_BLINK_SUCCESS;

err:
	free(priv);
	return WLAN_BLINK_FAIL;
}

wlan_blink_ret_t wlan_blink_wait_once(void)
{
	wlan_blink_priv_t *priv = wlan_blink;
	if (priv == NULL)
		return WLAN_BLINK_FAIL;

	if (blink_wait(0) != BLINK_OK)
		return WLAN_BLINK_FAIL;

	sc_assistant_newstatus(SCA_STATUS_COMPLETE, NULL, NULL);
	priv->status = WB_STATUS_COMPLETE;

	return WLAN_BLINK_SUCCESS;
}

#if 0
wlan_blink_ret_t wlan_blink_get_result(wlan_blink_result_t *result)
{
	wlan_blink_priv_t *priv = wlan_blink;
	if (priv == NULL || result == NULL)
		return WLAN_BLINK_FAIL;

	if (priv->status != WB_STATUS_COMPLETE)
		return WLAN_BLINK_FAIL;

	uint32_t passphrase_len;
	blink_get_result(BLINK_MSG_SSID, result->ssid, (uint32_t *)&result->ssid_len);
	if (result->ssid_len > WLAN_SSID_MAX_LEN) {
		WB_ERR("invalid ssid\n");
		return WLAN_BLINK_INVALID;
	}
	blink_get_result(BLINK_MSG_PASSWORD, result->passphrase, &passphrase_len);
	if (passphrase_len > WLAN_PASSPHRASE_MAX_LEN) {
		WB_ERR("invalid password\n");
		return WLAN_BLINK_INVALID;
	}

	return WLAN_BLINK_SUCCESS;
}
#endif

wlan_blink_ret_t wlan_blink_connect(uint32_t timeout_ms)
{
	int ret = WLAN_BLINK_FAIL;
	blink_result_t result;
	wlan_blink_priv_t *priv = wlan_blink;
	if (priv == NULL || priv->status == WB_STATUS_BUSY)
		return WLAN_BLINK_FAIL;

	ret = blink_get_result(&result);
	if (ret != BLINK_OK) {
		WB_ERR("get result fail\n");
		return ret;
	}

	priv->status = WB_STATUS_BUSY;
	priv->nif = sc_assistant_open_sta();

	/* wait timeout_ms not too long for get ssid success */
	if (sc_assistant_connect_ap(result.ssid, result.ssid_len, result.passphrase, timeout_ms) < 0) {
		WB_DBG("connect ap time out\n");
		blink_set_state(BLINK_STATE_FAIL);
	} else {
		ret = WLAN_BLINK_SUCCESS;
		WB_DBG("connect ap success\n");
		blink_set_state(BLINK_STATE_SUCCESS);
	}
	priv->status = WB_STATUS_NORMAL;

	return ret;
}

wlan_blink_status_t wlan_blink_get_status(void)
{
	wlan_blink_priv_t *priv = wlan_blink;

	if (!priv)
		return WB_STATUS_NOTREADY;

	return priv->status;
}

wlan_blink_ret_t wlan_blink_stop(void)
{
	wlan_blink_priv_t *priv = wlan_blink;
	if (priv == NULL) {
		WB_WRN("not start\n");
		return WLAN_BLINK_FAIL;
	}

	if (priv->status == WB_STATUS_BUSY) {
		WB_WRN("state busy!\n");
		return WLAN_BLINK_FAIL;
	}

	sc_assistant_stop_connect_ap();

	blink_stop();

	free(priv);
	wlan_blink = NULL;

	return WLAN_BLINK_SUCCESS;
}
