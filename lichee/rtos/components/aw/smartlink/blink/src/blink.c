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
#include "sys/defs.h"

#include "kernel/os/os.h"
#include "ble/bluetooth/bluetooth.h"
#include "ble/bluetooth/uuid.h"
#include "ble/bluetooth/gatt.h"
#include "ble/bluetooth/conn.h"

#include "blink.h"
#include "gatt_attr_manage.h"

#define BLINK_DBG_ON    0
#define BLINK_INF_ON    1
#define BLINK_WRN_ON    1
#define BLINK_ERR_ON    1

#define BLINK_LOG(flags, fmt, arg...)   \
	do {                                \
		if (flags)                      \
			printf(fmt, ##arg);         \
	} while (0)

#define BLINK_DBG(fmt, arg...) BLINK_LOG(BLINK_DBG_ON, "[BLINK DBG] "fmt, ##arg)
#define BLINK_INF(fmt, arg...) BLINK_LOG(BLINK_INF_ON, "[BLINK INF] "fmt, ##arg)
#define BLINK_WRN(fmt, arg...) BLINK_LOG(BLINK_WRN_ON, "[BLINK WRN] "fmt, ##arg)
#define BLINK_ERR(fmt, arg...)                                 \
	do {                                                       \
		BLINK_LOG(BLINK_ERR_ON, "[BLINK ERR] %s():%d, "fmt,    \
		          __func__, __LINE__, ##arg);                  \
	} while (0)

#define GATT_ATTR_CREATE(base, num)      \
	{                                    \
		base = gatt_attr_create(num);    \
		if (base == NULL)                \
			return -1;                   \
	}
#define GATT_ATTR_PRIMARY_SERVICE(base, _service) \
	{                                             \
		bt_gatt_attr_t param;                     \
		param.type = GATT_ATTR_SERVICE;           \
		param.attr.user_data = _service;          \
		base->add(base, &param);                  \
	}
#define GATT_ATTR_CHARACTERISTIC(base, _uuid, _props, _perm, _read, _write, _value) \
	{                                                                               \
		bt_gatt_attr_t param;                                                       \
		param.type = GATT_ATTR_CHARACTERISTIC;                                      \
		param.properties = _props;                                                  \
		param.attr.uuid = _uuid;                                                    \
		param.attr.perm = _perm;                                                    \
		param.attr.read = _read;                                                    \
		param.attr.write = _write;                                                  \
		param.attr.user_data = _value;                                              \
		base->add(base, &param);                                                    \
	}
#define GATT_ATTR_CCC(base, _changed, _perm)    \
	{                                           \
		bt_gatt_attr_t param;                   \
		param.type = GATT_ATTR_CCC;             \
		param.attr.perm = _perm;                \
		param.attr.user_data = _changed;        \
		base->add(base, &param);                \
	}
#define GATT_ATTR_GET(base, index)    base->get(base, index)
#define GATT_ATTR_DESTROY(base)       base->destroy(base)

#define BLINK_MSG_LEN_MAX             (108)
#define BLINK_MSG_SYNC                (0x5A6E3C6F)
#define BLINK_MSG_VERSION             (0x11)
#define BLINK_MSG_SERVICE_ID          (0x0B)

#define BLINK_MSG_VERSION_INDEX       (4)
#define BLINK_MSG_FMT_INDEX           (5)
#define BLINK_MSG_LEN_INDEX1          (6)
#define BLINK_MSG_LEN_INDEX2          (7)
#define BLINK_MSG_SSID_INDEX          (9)

#define BLINK_MSG_FMT_SUBC            (0)

#define BLINK_IND_LEN_MAX             (4)
#define BLINK_IND_SYNC                (0x0A)
#define BLINK_IND_STATUS_DEFAULT      (0x10)
#define BLINK_IND_STATUS_COMPLETE     (0x11)
#define BLINK_IND_STATUS_CONNECT      (0x12)

typedef enum {
	BLINK_STATUS_NORMAL = 0,
	BLINK_STATUS_READY,
	BLINK_STATUS_BUSY,
	BLINK_STATUS_TIMEOUT,
	BLINK_STATUS_COMPLETE,
} blink_status_t;

typedef struct blink_priv {
	uint8_t devFilter;
	blink_status_t status;
	XR_OS_Semaphore_t sem;
	uint8_t indicate;
	uint8_t indValue[BLINK_IND_LEN_MAX];
	uint8_t message[BLINK_MSG_LEN_MAX];
	struct bt_gatt_indicate_params indParams;
	set_state_callback cb;
	gatt_attr_base *attrBase;
	struct bt_gatt_service service;
} blink_priv_t;
static blink_priv_t *blink = NULL;

#define BLINK_SERVICE_UUID        (0xff01)
#define BLINK_CHAR_CONFIG_UUID    (0xff02)
#define BLINK_CHAR_IND_UUID       (0xff03)

static struct bt_uuid_16 blink_sevice_uuid = BT_UUID_INIT_16(BLINK_SERVICE_UUID);
static struct bt_uuid_16 blink_char_config_uuid = BT_UUID_INIT_16(BLINK_CHAR_CONFIG_UUID);
static struct bt_uuid_16 blink_char_ind_uuid = BT_UUID_INIT_16(BLINK_CHAR_IND_UUID);

#define SERVICE_UUID_16_1ST        (BLINK_SERVICE_UUID >> 8)
#define SERVICE_UUID_16_2ND        (BLINK_SERVICE_UUID & 0xFF)

#define BLINK_ADV_CONN_NAME_EX BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | \
                               BT_LE_ADV_OPT_USE_NAME,                     \
                               BT_GAP_ADV_FAST_INT_MIN_2,                  \
                               0X00B0, NULL)

static const struct bt_data blink_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, SERVICE_UUID_16_2ND, SERVICE_UUID_16_1ST),
};

static ssize_t blink_read_callback(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                   void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
	                         strlen(value));
}

static int blink_parse_message(uint8_t *value, uint8_t *buf, uint16_t len, uint16_t offset)
{
	blink_priv_t *priv = blink;
	uint8_t *message = buf;
	uint32_t sync = (uint32_t)message[0] << 24 | (uint32_t)message[1] << 16 |
	                (uint32_t)message[2] << 8 | message[3];
	uint8_t version = message[BLINK_MSG_VERSION_INDEX];

	static uint16_t remain = 0;

	if (priv->status != BLINK_STATUS_COMPLETE && sync == BLINK_MSG_SYNC) {
		if (version != BLINK_MSG_VERSION) {
			BLINK_ERR("blink version mismatch, need ver %02x, but %02x\n", BLINK_MSG_VERSION, version);
			return -1;
		}

		if (message[BLINK_MSG_FMT_INDEX] == BLINK_MSG_FMT_SUBC) {
			/* TO DO.. */
			return -1;
		}
		remain = (message[BLINK_MSG_LEN_INDEX1] << 8) | message[BLINK_MSG_LEN_INDEX2];
		memset(value, 0, BLINK_MSG_LEN_MAX);
	}

	memcpy(value + offset, buf, len);

	if (len <= remain) {
		remain -= len;
	} else {
		BLINK_WRN("not blink msg\n");
		return -1;
	}

	if (remain)
		return 0;

	XR_OS_ThreadSuspendScheduler();
	priv->status = BLINK_STATUS_COMPLETE;
	XR_OS_ThreadResumeScheduler();
	XR_OS_SemaphoreRelease(&priv->sem);

	return 0;
}

static ssize_t blink_write_callback(struct bt_conn *conn,
                                    const struct bt_gatt_attr *attr,
                                    const void *buf, uint16_t len, uint16_t offset,
                                    uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset + len > BLINK_MSG_LEN_MAX) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (flags & BT_GATT_WRITE_FLAG_PREPARE) {
		return 0;
	}

	blink_parse_message(value, (uint8_t *)buf, len, offset);

	return len;
}

static void blink_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BLINK_DBG("%s %d, %d\n", __func__, __LINE__, value);
	blink_priv_t *priv = blink;
	priv->indicate = (value > 0) ? 1 : 0;
}

static __inline int blink_adv_start(void)
{
	int err;
	err = bt_le_adv_start(BLINK_ADV_CONN_NAME_EX, blink_ad, ARRAY_SIZE(blink_ad), NULL, 0);
	if (err) {
		BLINK_ERR("Advertising failed to start (err %d)\n", err);
		return -1;
	}

	return 0;
}

static __inline int blink_adv_stop(void)
{
	return bt_le_adv_stop();
}

static int blink_gatt_service_init(blink_priv_t *priv)
{
	if (priv == NULL) {
		BLINK_WRN("invalid handle\n");
		return -1;
	}

	GATT_ATTR_CREATE(priv->attrBase, 6);
	GATT_ATTR_PRIMARY_SERVICE(priv->attrBase, &blink_sevice_uuid);
	GATT_ATTR_CHARACTERISTIC(priv->attrBase, &blink_char_config_uuid.uuid,
	                         BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_AUTH,
	                         BT_GATT_PERM_READ | BT_GATT_PERM_WRITE |BT_GATT_PERM_PREPARE_WRITE,
	                         blink_read_callback, blink_write_callback, &priv->message);
	GATT_ATTR_CHARACTERISTIC(priv->attrBase, &blink_char_ind_uuid.uuid,
	                         BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE | BT_GATT_CHRC_AUTH,
	                         BT_GATT_PERM_READ,
	                         blink_read_callback, NULL, &priv->indValue);
	GATT_ATTR_CCC(priv->attrBase, blink_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE);

	priv->service.attrs = GATT_ATTR_GET(priv->attrBase, 0);
	priv->service.attr_count = 6;

	return bt_gatt_service_register(&priv->service);
}

static int blink_gatt_service_deinit(void)
{
	int ret;
	blink_priv_t *priv = blink;
	if (priv == NULL) {
		BLINK_WRN("not init\n");
		return -1;
	}

	ret = bt_gatt_service_unregister(&priv->service);

	GATT_ATTR_DESTROY(priv->attrBase);

	return ret;
}

static void blink_indicate_cb(struct bt_conn *conn,
                              struct bt_gatt_indicate_params *params,
                              uint8_t err)
{
	blink_priv_t *priv = blink;

	BLINK_DBG("Indication %s\n", err != 0 ? "fail" : "success");

	blink_indicate_t ind;
	ind = (err != 0) ? BLINK_INDICATE_FAIL : BLINK_INDICATE_SUCCESS;
	if (priv->cb) {
		priv->cb(ind);
	}
}

blink_ret_t blink_start(blink_param_t *param)
{
	blink_priv_t *priv = blink;

	BLINK_DBG("%s %d\n", __func__, __LINE__);

	if (priv != NULL) {
		BLINK_WRN("blink has started\n");
		return BLINK_ERROR;
	}
	if (param == NULL) {
		BLINK_ERR("invalid param\n");
		return BLINK_INVALID;
	}

	priv = malloc(sizeof(blink_priv_t));
	if (priv == NULL) {
		BLINK_ERR("malloc fail\n");
		return BLINK_ERROR;
	}
	memset(priv, 0, sizeof(blink_priv_t));

	if (blink_gatt_service_init(priv) != 0) {
		BLINK_ERR("gatt service init fail\n");
		goto err;
	}

	if (blink_adv_start() != 0) {
		BLINK_ERR("adv start fail\n");
		goto err;
	}

	if (XR_OS_SemaphoreCreateBinary(&priv->sem) != XR_OS_OK) {
		BLINK_ERR("sem create fail\n");
		goto err;
	}

	priv->devFilter = param->dev_filter;
	priv->cb = param->cb;
	priv->status = BLINK_STATUS_READY;
	blink = priv;

	return BLINK_OK;

err:
	free(priv);
	return BLINK_ERROR;
}

blink_ret_t blink_wait(uint32_t timeout)
{
	blink_priv_t *priv = blink;

	if (!priv || priv->status != BLINK_STATUS_READY) {
		BLINK_ERR("blink not ready\n");
		return BLINK_ERROR;
	}

	if (timeout <= 0) {
		if (priv->status == BLINK_STATUS_COMPLETE)
			return BLINK_OK;
		return BLINK_ERROR;
	}

	if (XR_OS_SemaphoreWait(&priv->sem, timeout) != XR_OS_OK) {
		BLINK_WRN("sem wait fail\n");
		return BLINK_TIMEOUT;
	}
	if (priv->status != BLINK_STATUS_COMPLETE)
		return BLINK_ERROR;

	return BLINK_OK;
}

blink_ret_t blink_get_result(blink_result_t *result)
{
	blink_priv_t *priv = blink;
	if (!priv || !result) {
		BLINK_ERR("invalid state or param\n");
		return BLINK_ERROR;
	}
	if (priv->status != BLINK_STATUS_COMPLETE) {
		BLINK_WRN("blink not completed\n");
		return BLINK_ERROR;
	}

	result->ssid_len = priv->message[BLINK_MSG_SSID_INDEX];
	result->passphrase_len = priv->message[priv->message[BLINK_MSG_SSID_INDEX] + BLINK_MSG_SSID_INDEX + 1];

	if (result->ssid_len > WLAN_SSID_MAX_LEN ||
	    result->passphrase_len > WLAN_PASSPHRASE_MAX_LEN) {
		BLINK_WRN("invalid len\n");
		return BLINK_ERROR;
	}
	memcpy(result->ssid, priv->message + BLINK_MSG_SSID_INDEX + 1, result->ssid_len);
	memcpy(result->passphrase, priv->message + priv->message[BLINK_MSG_SSID_INDEX] + BLINK_MSG_SSID_INDEX + 2, result->passphrase_len);
	result->passphrase[result->passphrase_len] = '\0';

	return BLINK_OK;
}

blink_ret_t blink_set_state(blink_state_t state)
{
	blink_priv_t *priv = blink;
	if (priv == NULL) {
		BLINK_WRN("not init\n");
		return BLINK_ERROR;
	}

	if (priv->indicate > 0) {
		priv->indValue[0] = BLINK_IND_SYNC;
		priv->indValue[1] = BLINK_MSG_SERVICE_ID;
		priv->indValue[2] = (priv->status == BLINK_STATUS_COMPLETE) ?
		                     BLINK_IND_STATUS_COMPLETE : BLINK_IND_STATUS_DEFAULT;
		priv->indValue[3] = (state == BLINK_STATE_SUCCESS) ?
		                     BLINK_IND_STATUS_CONNECT : BLINK_IND_STATUS_DEFAULT;

		priv->indParams.attr = priv->attrBase->get(priv->attrBase, 3);
		priv->indParams.func = blink_indicate_cb;
		priv->indParams.data = &priv->indValue;
		priv->indParams.len = sizeof(priv->indValue);

		return (bt_gatt_indicate(NULL, &priv->indParams) == 0 ? BLINK_OK : BLINK_ERROR);
	}

	return BLINK_ERROR;
}

blink_ret_t blink_stop(void)
{
	blink_priv_t *priv = blink;

	if (priv == NULL) {
		BLINK_WRN("blink not started\n");
		return BLINK_ERROR;
	}
	if (priv->status == BLINK_STATUS_BUSY) {
		BLINK_WRN("blink busy\n");
		return BLINK_BUSY;
	}

	blink_adv_stop();

	blink_gatt_service_deinit();

	XR_OS_SemaphoreDelete(&priv->sem);

	free(priv);
	blink = NULL;
	return BLINK_OK;
}
