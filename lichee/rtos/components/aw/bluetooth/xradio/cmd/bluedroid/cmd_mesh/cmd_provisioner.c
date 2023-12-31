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

#include "cmd_util.h"

#include <stdio.h>
#include <string.h>

#include "xr_log.h"
#include "nvs_flash.h"

#include "xr_ble_mesh_defs.h"
#include "xr_ble_mesh_common_api.h"
#include "xr_ble_mesh_provisioning_api.h"
#include "xr_ble_mesh_networking_api.h"
#include "xr_ble_mesh_config_model_api.h"
#include "xr_ble_mesh_generic_model_api.h"

#include "ble_mesh_uuid.h"

#define TAG "EXAMPLE"

#define LED_OFF             0x0
#define LED_ON              0x1

#define CID_ESP             0x02E5

#define PROV_OWN_ADDR       0x0001

#define MSG_SEND_TTL        3
#define MSG_SEND_REL        false
#define MSG_TIMEOUT         0
#define MSG_ROLE            ROLE_PROVISIONER

#define COMP_DATA_PAGE_0    0x00

#define APP_KEY_IDX         0x0000
#define APP_KEY_OCTET       0x12

static uint8_t dev_uuid[16];

typedef struct {
	uint8_t  uuid[16];
	uint16_t unicast;
	uint8_t  elem_num;
	uint8_t  onoff;
} xr_ble_mesh_node_info_t;

#define BLE_MESH_MAX_PROV_NODES 6
static xr_ble_mesh_node_info_t nodes[BLE_MESH_MAX_PROV_NODES] = {
	[0 ... (BLE_MESH_MAX_PROV_NODES - 1)] = {
		.unicast = XR_BLE_MESH_ADDR_UNASSIGNED,
		.elem_num = 0,
		.onoff = LED_OFF,
	}
};

static struct xr_ble_mesh_key {
	uint16_t net_idx;
	uint16_t app_idx;
	uint8_t  app_key[16];
} prov_key;

static xr_ble_mesh_client_t config_client;
static xr_ble_mesh_client_t onoff_client;

static xr_ble_mesh_cfg_srv_t config_server = {
	.relay = XR_BLE_MESH_RELAY_DISABLED,
	.beacon = XR_BLE_MESH_BEACON_ENABLED,
#if defined(CONFIG_BLE_MESH_FRIEND)
	.friend_state = XR_BLE_MESH_FRIEND_ENABLED,
#else
	.friend_state = XR_BLE_MESH_FRIEND_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BLE_MESH_GATT_PROXY_SERVER)
	.gatt_proxy = XR_BLE_MESH_GATT_PROXY_ENABLED,
#else
	.gatt_proxy = XR_BLE_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
	.default_ttl = 7,
	/* 3 transmissions with 20ms interval */
	.net_transmit = XR_BLE_MESH_TRANSMIT(2, 20),
	.relay_retransmit = XR_BLE_MESH_TRANSMIT(2, 20),
};

static xr_ble_mesh_model_t root_models[] = {
	XR_BLE_MESH_MODEL_CFG_SRV(&config_server),
	XR_BLE_MESH_MODEL_CFG_CLI(&config_client),
	XR_BLE_MESH_MODEL_GEN_ONOFF_CLI(NULL, &onoff_client),
};

static xr_ble_mesh_elem_t elements[] = {
	XR_BLE_MESH_ELEMENT(0, root_models, XR_BLE_MESH_MODEL_NONE),
};

static xr_ble_mesh_comp_t composition = {
	.cid = CID_ESP,
	.elements = elements,
	.element_count = ARRAY_SIZE(elements),
};

static xr_ble_mesh_prov_t provision = {
	.prov_uuid           = dev_uuid,
	.prov_unicast_addr   = PROV_OWN_ADDR,
	.prov_start_address  = 0x0005,
	.prov_attention      = 0x00,
	.prov_algorithm      = 0x00,
	.prov_pub_key_oob    = 0x00,
	.prov_static_oob_val = NULL,
	.prov_static_oob_len = 0x00,
	.flags               = 0x00,
	.iv_index            = 0x00,
};

static xr_err_t example_ble_mesh_store_node_info(const uint8_t uuid[16], uint16_t unicast,
												  uint8_t elem_num, uint8_t onoff_state)
{
	int i;

	if (!uuid || !XR_BLE_MESH_ADDR_IS_UNICAST(unicast)) {
		return XR_ERR_INVALID_ARG;
	}

	/* Judge if the device has been provisioned before */
	for (i = 0; i < ARRAY_SIZE(nodes); i++) {
		if (!memcmp(nodes[i].uuid, uuid, 16)) {
			XR_LOGW(TAG, "%s: reprovisioned device 0x%04x", __func__, unicast);
			nodes[i].unicast = unicast;
			nodes[i].elem_num = elem_num;
			nodes[i].onoff = onoff_state;
			return XR_OK;
		}
	}

	for (i = 0; i < ARRAY_SIZE(nodes); i++) {
		if (nodes[i].unicast == XR_BLE_MESH_ADDR_UNASSIGNED) {
			memcpy(nodes[i].uuid, uuid, 16);
			nodes[i].unicast = unicast;
			nodes[i].elem_num = elem_num;
			nodes[i].onoff = onoff_state;
			return XR_OK;
		}
	}

	return XR_FAIL;
}

static xr_ble_mesh_node_info_t *example_ble_mesh_get_node_info(uint16_t unicast)
{
	int i;

	if (!XR_BLE_MESH_ADDR_IS_UNICAST(unicast)) {
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(nodes); i++) {
		if (nodes[i].unicast <= unicast &&
				nodes[i].unicast + nodes[i].elem_num > unicast) {
			return &nodes[i];
		}
	}

	return NULL;
}

static xr_err_t example_ble_mesh_set_msg_common(xr_ble_mesh_client_common_param_t *common,
												 xr_ble_mesh_node_info_t *node,
												 xr_ble_mesh_model_t *model, uint32_t opcode)
{
	if (!common || !node || !model) {
		return XR_ERR_INVALID_ARG;
	}

	common->opcode = opcode;
	common->model = model;
	common->ctx.net_idx = prov_key.net_idx;
	common->ctx.app_idx = prov_key.app_idx;
	common->ctx.addr = node->unicast;
	common->ctx.send_ttl = MSG_SEND_TTL;
	common->ctx.send_rel = MSG_SEND_REL;
	common->msg_timeout = MSG_TIMEOUT;
	common->msg_role = MSG_ROLE;

	return XR_OK;
}

static xr_err_t prov_complete(int node_idx, const xr_ble_mesh_octet16_t uuid,
							   uint16_t unicast, uint8_t elem_num, uint16_t net_idx)
{
	xr_ble_mesh_client_common_param_t common = {0};
	xr_ble_mesh_cfg_client_get_state_t get_state = {0};
	xr_ble_mesh_node_info_t *node = NULL;
	char name[11] = {0};
	int err;

	XR_LOGI(TAG, "node index: 0x%x, unicast address: 0x%02x, element num: %d, netkey index: 0x%02x",
			 node_idx, unicast, elem_num, net_idx);
	XR_LOGI(TAG, "device uuid: %s", bt_hex(uuid, 16));

	sprintf(name, "%s%d", "NODE-", node_idx);
	err = xr_ble_mesh_provisioner_set_node_name(node_idx, name);
	if (err) {
		XR_LOGE(TAG, "%s: Set node name failed", __func__);
		return XR_FAIL;
	}

	err = example_ble_mesh_store_node_info(uuid, unicast, elem_num, LED_OFF);
	if (err) {
		XR_LOGE(TAG, "%s: Store node info failed", __func__);
		return XR_FAIL;
	}

	node = example_ble_mesh_get_node_info(unicast);
	if (!node) {
		XR_LOGE(TAG, "%s: Get node info failed", __func__);
		return XR_FAIL;
	}

	example_ble_mesh_set_msg_common(&common, node, config_client.model, XR_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
	get_state.comp_data_get.page = COMP_DATA_PAGE_0;
	err = xr_ble_mesh_config_client_get_state(&common, &get_state);
	if (err) {
		XR_LOGE(TAG, "%s: Send config comp data get failed", __func__);
		return XR_FAIL;
	}

	return XR_OK;
}

static void prov_link_open(xr_ble_mesh_prov_bearer_t bearer)
{
	XR_LOGI(TAG, "%s link open", bearer == XR_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
}

static void prov_link_close(xr_ble_mesh_prov_bearer_t bearer, uint8_t reason)
{
	XR_LOGI(TAG, "%s link close, reason 0x%02x",
			 bearer == XR_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT", reason);
}

static void recv_unprov_adv_pkt(uint8_t dev_uuid[16], uint8_t addr[BD_ADDR_LEN],
								xr_ble_mesh_addr_type_t addr_type, uint16_t oob_info,
								uint8_t adv_type, xr_ble_mesh_prov_bearer_t bearer)
{
	xr_ble_mesh_unprov_dev_add_t add_dev = {0};
	int err;

	/* Due to the API xr_ble_mesh_provisioner_set_dev_uuid_match, Provisioner will only
	 * use this callback to report the devices, whose device UUID starts with 0xdd & 0xdd,
	 * to the application layer.
	 */

	XR_LOGI(TAG, "address: %s, address type: %d, adv type: %d", bt_hex(addr, BD_ADDR_LEN), addr_type, adv_type);
	XR_LOGI(TAG, "device uuid: %s", bt_hex(dev_uuid, 16));
	XR_LOGI(TAG, "oob info: %d, bearer: %s", oob_info, (bearer & XR_BLE_MESH_PROV_ADV) ? "PB-ADV" : "PB-GATT");

	memcpy(add_dev.addr, addr, BD_ADDR_LEN);
	add_dev.addr_type = (uint8_t)addr_type;
	memcpy(add_dev.uuid, dev_uuid, 16);
	add_dev.oob_info = oob_info;
	add_dev.bearer = (uint8_t)bearer;
	/* Note: If unprovisioned device adv packets have not been received, we should not add
			 device with ADD_DEV_START_PROV_NOW_FLAG set. */
	err = xr_ble_mesh_provisioner_add_unprov_dev(&add_dev,
			ADD_DEV_RM_AFTER_PROV_FLAG | ADD_DEV_START_PROV_NOW_FLAG | ADD_DEV_FLUSHABLE_DEV_FLAG);
	if (err) {
		XR_LOGE(TAG, "%s: Add unprovisioned device into queue failed", __func__);
	}

	return;
}

static void example_ble_mesh_prov_provisioning_cb(xr_ble_mesh_prov_cb_event_t event,
											 xr_ble_mesh_prov_cb_param_t *param)
{
	switch (event) {
	case XR_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT, err_code %d", param->provisioner_prov_enable_comp.err_code);
		break;
	case XR_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT, err_code %d", param->provisioner_prov_disable_comp.err_code);
		break;
	case XR_BLE_MESH_PROVISIONER_RECV_UNPROV_ADV_PKT_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_PROVISIONER_RECV_UNPROV_ADV_PKT_EVT");
		recv_unprov_adv_pkt(param->provisioner_recv_unprov_adv_pkt.dev_uuid, param->provisioner_recv_unprov_adv_pkt.addr,
							param->provisioner_recv_unprov_adv_pkt.addr_type, param->provisioner_recv_unprov_adv_pkt.oob_info,
							param->provisioner_recv_unprov_adv_pkt.adv_type, param->provisioner_recv_unprov_adv_pkt.bearer);
		break;
	case XR_BLE_MESH_PROVISIONER_PROV_LINK_OPEN_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_PROVISIONER_CLOSE_EVT");
		prov_link_open(param->provisioner_prov_link_open.bearer);
		break;
	case XR_BLE_MESH_PROVISIONER_PROV_LINK_CLOSE_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_PROVISIONER_OPEN_EVT");
		prov_link_close(param->provisioner_prov_link_close.bearer, param->provisioner_prov_link_close.reason);
		break;
	case XR_BLE_MESH_PROVISIONER_PROV_COMPLETE_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_PROVISIONER_PROV_COMPLETE_EVT");
		prov_complete(param->provisioner_prov_complete.node_idx, param->provisioner_prov_complete.device_uuid,
					  param->provisioner_prov_complete.unicast_addr, param->provisioner_prov_complete.element_num,
					  param->provisioner_prov_complete.netkey_idx);
		break;
	case XR_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT, err_code %d", param->provisioner_add_unprov_dev_comp.err_code);
		break;
	case XR_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT, err_code %d", param->provisioner_set_dev_uuid_match_comp.err_code);
		break;
	case XR_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT: {
		XR_LOGI(TAG, "XR_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT, err_code %d", param->provisioner_set_node_name_comp.err_code);
		if (param->provisioner_set_node_name_comp.err_code == XR_OK) {
			const char *name = NULL;
			name = xr_ble_mesh_provisioner_get_node_name(param->provisioner_set_node_name_comp.node_index);
			if (!name) {
				XR_LOGE(TAG, "Get node name failed");
				return;
			}
			XR_LOGI(TAG, "Node %d name is: %s", param->provisioner_set_node_name_comp.node_index, name);
		}
		break;
	}
	case XR_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT: {
		XR_LOGI(TAG, "XR_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT, err_code %d", param->provisioner_add_app_key_comp.err_code);
		if (param->provisioner_add_app_key_comp.err_code == XR_OK) {
			xr_err_t err = 0;
			prov_key.app_idx = param->provisioner_add_app_key_comp.app_idx;
			err = xr_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
					XR_BLE_MESH_MODEL_ID_GEN_ONOFF_CLI, XR_BLE_MESH_CID_NVAL);
			if (err != XR_OK) {
				XR_LOGE(TAG, "Provisioner bind local model appkey failed");
				return;
			}
		}
		break;
	}
	case XR_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT, err_code %d", param->provisioner_bind_app_key_to_model_comp.err_code);
		break;
	default:
		break;
	}

	return;
}

static void example_ble_mesh_prov_config_client_cb(xr_ble_mesh_cfg_client_cb_event_t event,
											  xr_ble_mesh_cfg_client_cb_param_t *param)
{
	xr_ble_mesh_client_common_param_t common = {0};
	xr_ble_mesh_node_info_t *node = NULL;
	uint32_t opcode;
	uint16_t addr;
	int err;

	opcode = param->params->opcode;
	addr = param->params->ctx.addr;

	XR_LOGI(TAG, "%s, error_code = 0x%02x, event = 0x%02x, addr: 0x%04x, opcode: 0x%04x",
			 __func__, param->error_code, event, param->params->ctx.addr, opcode);

	if (param->error_code) {
		XR_LOGE(TAG, "Send config client message failed, opcode 0x%04x", opcode);
		return;
	}

	node = example_ble_mesh_get_node_info(addr);
	if (!node) {
		XR_LOGE(TAG, "%s: Get node info failed", __func__);
		return;
	}

	switch (event) {
	case XR_BLE_MESH_CFG_CLIENT_GET_STATE_EVT:
		switch (opcode) {
		case XR_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET: {
			XR_LOGI(TAG, "composition data %s", bt_hex(param->status_cb.comp_data_status.composition_data->data,
					 param->status_cb.comp_data_status.composition_data->len));
			xr_ble_mesh_cfg_client_set_state_t set_state = {0};
			example_ble_mesh_set_msg_common(&common, node, config_client.model, XR_BLE_MESH_MODEL_OP_APP_KEY_ADD);
			set_state.app_key_add.net_idx = prov_key.net_idx;
			set_state.app_key_add.app_idx = prov_key.app_idx;
			memcpy(set_state.app_key_add.app_key, prov_key.app_key, 16);
			err = xr_ble_mesh_config_client_set_state(&common, &set_state);
			if (err) {
				XR_LOGE(TAG, "%s: Config AppKey Add failed", __func__);
				return;
			}
			break;
		}
		default:
			break;
		}
		break;
	case XR_BLE_MESH_CFG_CLIENT_SET_STATE_EVT:
		switch (opcode) {
		case XR_BLE_MESH_MODEL_OP_APP_KEY_ADD: {
			xr_ble_mesh_cfg_client_set_state_t set_state = {0};
			example_ble_mesh_set_msg_common(&common, node, config_client.model, XR_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
			set_state.model_app_bind.element_addr = node->unicast;
			set_state.model_app_bind.model_app_idx = prov_key.app_idx;
			set_state.model_app_bind.model_id = XR_BLE_MESH_MODEL_ID_GEN_ONOFF_SRV;
			set_state.model_app_bind.company_id = XR_BLE_MESH_CID_NVAL;
			err = xr_ble_mesh_config_client_set_state(&common, &set_state);
			if (err) {
				XR_LOGE(TAG, "%s: Config Model App Bind failed", __func__);
				return;
			}
			break;
		}
		case XR_BLE_MESH_MODEL_OP_MODEL_APP_BIND: {
			xr_ble_mesh_generic_client_get_state_t get_state = {0};
			example_ble_mesh_set_msg_common(&common, node, onoff_client.model, XR_BLE_MESH_MODEL_OP_GEN_ONOFF_GET);
			err = xr_ble_mesh_generic_client_get_state(&common, &get_state);
			if (err) {
				XR_LOGE(TAG, "%s: Generic OnOff Get failed", __func__);
				return;
			}
			break;
		}
		default:
			break;
		}
		break;
	case XR_BLE_MESH_CFG_CLIENT_PUBLISH_EVT:
		switch (opcode) {
		case XR_BLE_MESH_MODEL_OP_COMPOSITION_DATA_STATUS:
			XR_LOG_BUFFER_HEX("composition data %s", param->status_cb.comp_data_status.composition_data->data,
							   param->status_cb.comp_data_status.composition_data->len);
			break;
		case XR_BLE_MESH_MODEL_OP_APP_KEY_STATUS:
			break;
		default:
			break;
		}
		break;
	case XR_BLE_MESH_CFG_CLIENT_TIMEOUT_EVT:
		switch (opcode) {
		case XR_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET: {
			xr_ble_mesh_cfg_client_get_state_t get_state = {0};
			example_ble_mesh_set_msg_common(&common, node, config_client.model, XR_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
			get_state.comp_data_get.page = COMP_DATA_PAGE_0;
			err = xr_ble_mesh_config_client_get_state(&common, &get_state);
			if (err) {
				XR_LOGE(TAG, "%s: Config Composition Data Get failed", __func__);
				return;
			}
			break;
		}
		case XR_BLE_MESH_MODEL_OP_APP_KEY_ADD: {
			xr_ble_mesh_cfg_client_set_state_t set_state = {0};
			example_ble_mesh_set_msg_common(&common, node, config_client.model, XR_BLE_MESH_MODEL_OP_APP_KEY_ADD);
			set_state.app_key_add.net_idx = prov_key.net_idx;
			set_state.app_key_add.app_idx = prov_key.app_idx;
			memcpy(set_state.app_key_add.app_key, prov_key.app_key, 16);
			err = xr_ble_mesh_config_client_set_state(&common, &set_state);
			if (err) {
				XR_LOGE(TAG, "%s: Config AppKey Add failed", __func__);
				return;
			}
			break;
		}
		case XR_BLE_MESH_MODEL_OP_MODEL_APP_BIND: {
			xr_ble_mesh_cfg_client_set_state_t set_state = {0};
			example_ble_mesh_set_msg_common(&common, node, config_client.model, XR_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
			set_state.model_app_bind.element_addr = node->unicast;
			set_state.model_app_bind.model_app_idx = prov_key.app_idx;
			set_state.model_app_bind.model_id = XR_BLE_MESH_MODEL_ID_GEN_ONOFF_SRV;
			set_state.model_app_bind.company_id = XR_BLE_MESH_CID_NVAL;
			err = xr_ble_mesh_config_client_set_state(&common, &set_state);
			if (err) {
				XR_LOGE(TAG, "%s: Config Model App Bind failed", __func__);
				return;
			}
			break;
		}
		default:
			break;
		}
		break;
	default:
		XR_LOGE(TAG, "Not a config client status message event");
		break;
	}
}

static void example_ble_mesh_prov_generic_client_cb(xr_ble_mesh_generic_client_cb_event_t event,
											   xr_ble_mesh_generic_client_cb_param_t *param)
{
	xr_ble_mesh_client_common_param_t common = {0};
	xr_ble_mesh_node_info_t *node = NULL;
	uint32_t opcode;
	uint16_t addr;
	int err;

	opcode = param->params->opcode;
	addr = param->params->ctx.addr;

	XR_LOGI(TAG, "%s, error_code = 0x%02x, event = 0x%02x, addr: 0x%04x, opcode: 0x%04x",
			 __func__, param->error_code, event, param->params->ctx.addr, opcode);

	if (param->error_code) {
		XR_LOGE(TAG, "Send generic client message failed, opcode 0x%04x", opcode);
		return;
	}

	node = example_ble_mesh_get_node_info(addr);
	if (!node) {
		XR_LOGE(TAG, "%s: Get node info failed", __func__);
		return;
	}

	switch (event) {
	case XR_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT:
		switch (opcode) {
		case XR_BLE_MESH_MODEL_OP_GEN_ONOFF_GET: {
			xr_ble_mesh_generic_client_set_state_t set_state = {0};
			node->onoff = param->status_cb.onoff_status.present_onoff;
			XR_LOGI(TAG, "XR_BLE_MESH_MODEL_OP_GEN_ONOFF_GET onoff: 0x%02x", node->onoff);
			/* After Generic OnOff Status for Generic OnOff Get is received, Generic OnOff Set will be sent */
			example_ble_mesh_set_msg_common(&common, node, onoff_client.model, XR_BLE_MESH_MODEL_OP_GEN_ONOFF_SET);
			set_state.onoff_set.op_en = false;
			set_state.onoff_set.onoff = !node->onoff;
			set_state.onoff_set.tid = 0;
			err = xr_ble_mesh_generic_client_set_state(&common, &set_state);
			if (err) {
				XR_LOGE(TAG, "%s: Generic OnOff Set failed", __func__);
				return;
			}
			break;
		}
		default:
			break;
		}
		break;
	case XR_BLE_MESH_GENERIC_CLIENT_SET_STATE_EVT:
		switch (opcode) {
		case XR_BLE_MESH_MODEL_OP_GEN_ONOFF_SET:
			node->onoff = param->status_cb.onoff_status.present_onoff;
			XR_LOGI(TAG, "XR_BLE_MESH_MODEL_OP_GEN_ONOFF_SET onoff: 0x%02x", node->onoff);
			break;
		default:
			break;
		}
		break;
	case XR_BLE_MESH_GENERIC_CLIENT_PUBLISH_EVT:
		break;
	case XR_BLE_MESH_GENERIC_CLIENT_TIMEOUT_EVT:
		/* If failed to receive the responses, these messages will be resend */
		switch (opcode) {
		case XR_BLE_MESH_MODEL_OP_GEN_ONOFF_GET: {
			xr_ble_mesh_generic_client_get_state_t get_state = {0};
			example_ble_mesh_set_msg_common(&common, node, onoff_client.model, XR_BLE_MESH_MODEL_OP_GEN_ONOFF_GET);
			err = xr_ble_mesh_generic_client_get_state(&common, &get_state);
			if (err) {
				XR_LOGE(TAG, "%s: Generic OnOff Get failed", __func__);
				return;
			}
			break;
		}
		case XR_BLE_MESH_MODEL_OP_GEN_ONOFF_SET: {
			xr_ble_mesh_generic_client_set_state_t set_state = {0};
			node->onoff = param->status_cb.onoff_status.present_onoff;
			XR_LOGI(TAG, "XR_BLE_MESH_MODEL_OP_GEN_ONOFF_GET onoff: 0x%02x", node->onoff);
			example_ble_mesh_set_msg_common(&common, node, onoff_client.model, XR_BLE_MESH_MODEL_OP_GEN_ONOFF_SET);
			set_state.onoff_set.op_en = false;
			set_state.onoff_set.onoff = !node->onoff;
			set_state.onoff_set.tid = 0;
			err = xr_ble_mesh_generic_client_set_state(&common, &set_state);
			if (err) {
				XR_LOGE(TAG, "%s: Generic OnOff Set failed", __func__);
				return;
			}
			break;
		}
		default:
			break;
		}
		break;
	default:
		XR_LOGE(TAG, "Not a generic client status message event");
		break;
	}
}

static xr_err_t ble_mesh_prov_init(void)
{
	uint8_t match[2] = {0xdd, 0xdd};
	xr_err_t err = XR_OK;

	prov_key.net_idx = XR_BLE_MESH_KEY_PRIMARY;
	prov_key.app_idx = APP_KEY_IDX;
	memset(prov_key.app_key, APP_KEY_OCTET, sizeof(prov_key.app_key));

	xr_ble_mesh_register_prov_callback(example_ble_mesh_prov_provisioning_cb);
	xr_ble_mesh_register_config_client_callback(example_ble_mesh_prov_config_client_cb);
	xr_ble_mesh_register_generic_client_callback(example_ble_mesh_prov_generic_client_cb);

	err = xr_ble_mesh_init(&provision, &composition);
	if (err != XR_OK) {
		XR_LOGE(TAG, "Failed to initialize mesh stack (err %d)", err);
		return err;
	}

	err = xr_ble_mesh_provisioner_set_dev_uuid_match(match, sizeof(match), 0x0, false);
	if (err != XR_OK) {
		XR_LOGE(TAG, "Failed to set matching device uuid (err %d)", err);
		return err;
	}

	err = xr_ble_mesh_provisioner_prov_enable(XR_BLE_MESH_PROV_ADV);
	if (err != XR_OK) {
		XR_LOGE(TAG, "Failed to enable mesh provisioner (err %d)", err);
		return err;
	}

	err = xr_ble_mesh_provisioner_add_local_app_key(prov_key.app_key, prov_key.net_idx, prov_key.app_idx);
	if (err != XR_OK) {
		XR_LOGE(TAG, "Failed to add local AppKey (err %d)", err);
		return err;
	}

	XR_LOGI(TAG, "BLE Mesh Provisioner initialized");

	return err;
}

enum cmd_status cmd_prov_init_exec(char *cmd)
{
	xr_err_t err;

	XR_LOGI(TAG, "Initializing...");
	ble_mesh_get_dev_uuid(dev_uuid);

	/* Initialize the Bluetooth Mesh Subsystem */
	err = ble_mesh_prov_init();
	if (err) {
		XR_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
	}
}

/*
    $prov init

*/
static const struct cmd_data g_prov_cmds[] = {
	{ "init",                cmd_prov_init_exec },
};

enum cmd_status cmd_prov_exec(char *cmd)
{
	return cmd_exec(cmd, g_prov_cmds, cmd_nitems(g_prov_cmds));
}

