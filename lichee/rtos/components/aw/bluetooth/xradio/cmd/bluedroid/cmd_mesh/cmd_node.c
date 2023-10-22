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

#include "mesh_main.h"
#include "xr_ble_mesh_defs.h"
#include "xr_ble_mesh_common_api.h"
#include "xr_ble_mesh_networking_api.h"
#include "xr_ble_mesh_provisioning_api.h"
#include "xr_ble_mesh_config_model_api.h"
#include "xr_ble_mesh_generic_model_api.h"
#include "xr_ble_mesh_local_data_operation_api.h"

#include "ble_mesh_uuid.h"

#define TAG "EXAMPLE"

#define CID_ESP 0x02E5

//extern struct _led_state led_state[3];

static uint8_t dev_uuid[16] = { 0xdd, 0xdd };

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

XR_BLE_MESH_MODEL_PUB_DEFINE(onoff_pub_0, 2 + 3, ROLE_NODE);
static xr_ble_mesh_gen_onoff_srv_t onoff_server_0 = {
	.rsp_ctrl.get_auto_rsp = XR_BLE_MESH_SERVER_AUTO_RSP,
	.rsp_ctrl.set_auto_rsp = XR_BLE_MESH_SERVER_AUTO_RSP,
};

XR_BLE_MESH_MODEL_PUB_DEFINE(onoff_pub_1, 2 + 3, ROLE_NODE);
static xr_ble_mesh_gen_onoff_srv_t onoff_server_1 = {
	.rsp_ctrl.get_auto_rsp = XR_BLE_MESH_SERVER_RSP_BY_APP,
	.rsp_ctrl.set_auto_rsp = XR_BLE_MESH_SERVER_RSP_BY_APP,
};

XR_BLE_MESH_MODEL_PUB_DEFINE(onoff_pub_2, 2 + 3, ROLE_NODE);
static xr_ble_mesh_gen_onoff_srv_t onoff_server_2 = {
	.rsp_ctrl.get_auto_rsp = XR_BLE_MESH_SERVER_AUTO_RSP,
	.rsp_ctrl.set_auto_rsp = XR_BLE_MESH_SERVER_RSP_BY_APP,
};

static xr_ble_mesh_model_t root_models[] = {
	XR_BLE_MESH_MODEL_CFG_SRV(&config_server),
	XR_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoff_pub_0, &onoff_server_0),
};

static xr_ble_mesh_model_t extend_model_0[] = {
	XR_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoff_pub_1, &onoff_server_1),
};

static xr_ble_mesh_model_t extend_model_1[] = {
	XR_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoff_pub_2, &onoff_server_2),
};

static xr_ble_mesh_elem_t elements[] = {
	XR_BLE_MESH_ELEMENT(0, root_models, XR_BLE_MESH_MODEL_NONE),
	XR_BLE_MESH_ELEMENT(0, extend_model_0, XR_BLE_MESH_MODEL_NONE),
	XR_BLE_MESH_ELEMENT(0, extend_model_1, XR_BLE_MESH_MODEL_NONE),
};

static xr_ble_mesh_comp_t composition = {
	.cid = CID_ESP,
	.elements = elements,
	.element_count = ARRAY_SIZE(elements),
};

/* Disable OOB security for SILabs Android app */
static xr_ble_mesh_prov_t provision = {
	.uuid = dev_uuid,
#if 0
	.output_size = 4,
	.output_actions = XR_BLE_MESH_DISPLAY_NUMBER,
	.input_actions = XR_BLE_MESH_PUSH,
	.input_size = 4,
#else
	.output_size = 0,
	.output_actions = 0,
#endif
};

static int led_onoff = 0;

static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index)
{
	XR_LOGI(TAG, "net_idx: 0x%04x, addr: 0x%04x", net_idx, addr);
	XR_LOGI(TAG, "flags: 0x%02x, iv_index: 0x%08x", flags, iv_index);
	//board_led_operation(LED_G, LED_OFF);
	XR_LOGI(TAG,"led_g state is %d,prov_complete", led_onoff);
}

static void example_change_led_state(xr_ble_mesh_model_t *model,
									 xr_ble_mesh_msg_ctx_t *ctx, uint8_t onoff)
{
	uint16_t primary_addr = xr_ble_mesh_get_primary_element_address();
	uint8_t elem_count = xr_ble_mesh_get_element_count();
	struct _led_state *led = NULL;
	uint8_t i;

	if (XR_BLE_MESH_ADDR_IS_UNICAST(ctx->recv_dst)) {
		for (i = 0; i < elem_count; i++) {
			if (ctx->recv_dst == (primary_addr + i)) {
				//led = &led_state[i];
				//board_led_operation(led->pin, onoff);
				led_onoff = onoff;
			}
		}
	} else if (XR_BLE_MESH_ADDR_IS_GROUP(ctx->recv_dst)) {
		if (xr_ble_mesh_is_model_subscribed_to_group(model, ctx->recv_dst)) {
			//led = &led_state[model->element->element_addr - primary_addr];
			//board_led_operation(led->pin, onoff);
			led_onoff = onoff;
		}
	} else if (ctx->recv_dst == 0xFFFF) {
		//led = &led_state[model->element->element_addr - primary_addr];
		//board_led_operation(led->pin, onoff);
		led_onoff = onoff;
	}
}

static void example_handle_gen_onoff_msg(xr_ble_mesh_model_t *model,
										 xr_ble_mesh_msg_ctx_t *ctx,
										 xr_ble_mesh_server_recv_gen_onoff_set_t *set)
{
	xr_ble_mesh_gen_onoff_srv_t *srv = model->user_data;

	switch (ctx->recv_op) {
	case XR_BLE_MESH_MODEL_OP_GEN_ONOFF_GET:
		xr_ble_mesh_server_model_send_msg(model, ctx,
			XR_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, sizeof(srv->state.onoff), &srv->state.onoff);
		break;
	case XR_BLE_MESH_MODEL_OP_GEN_ONOFF_SET:
	case XR_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK:
		if (set->op_en == false) {
			srv->state.onoff = set->onoff;
		} else {
			/* TODO: Delay and state transition */
			srv->state.onoff = set->onoff;
		}
		if (ctx->recv_op == XR_BLE_MESH_MODEL_OP_GEN_ONOFF_SET) {
			xr_ble_mesh_server_model_send_msg(model, ctx,
				XR_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, sizeof(srv->state.onoff), &srv->state.onoff);
		}
		xr_ble_mesh_model_publish(model, XR_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS,
			sizeof(srv->state.onoff), &srv->state.onoff, ROLE_NODE);
		example_change_led_state(model, ctx, srv->state.onoff);
		break;
	default:
		break;
	}
}

static void example_ble_mesh_node_provisioning_cb(xr_ble_mesh_prov_cb_event_t event,
											 xr_ble_mesh_prov_cb_param_t *param)
{
	switch (event) {
	case XR_BLE_MESH_PROV_REGISTER_COMP_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_PROV_REGISTER_COMP_EVT, err_code %d", param->prov_register_comp.err_code);
		break;
	case XR_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT, err_code %d", param->node_prov_enable_comp.err_code);
		break;
	case XR_BLE_MESH_NODE_PROV_LINK_OPEN_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_NODE_PROV_LINK_OPEN_EVT, bearer %s",
			param->node_prov_link_open.bearer == XR_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
		break;
	case XR_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT, bearer %s",
			param->node_prov_link_close.bearer == XR_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
		break;
	case XR_BLE_MESH_NODE_PROV_COMPLETE_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_NODE_PROV_COMPLETE_EVT");
		prov_complete(param->node_prov_complete.net_idx, param->node_prov_complete.addr,
			param->node_prov_complete.flags, param->node_prov_complete.iv_index);
		break;
	case XR_BLE_MESH_NODE_PROV_RESET_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_NODE_PROV_RESET_EVT");
		break;
	case XR_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT, err_code %d", param->node_set_unprov_dev_name_comp.err_code);
		break;
	default:
		break;
	}
}

static void example_ble_mesh_node_generic_server_cb(xr_ble_mesh_generic_server_cb_event_t event,
											   xr_ble_mesh_generic_server_cb_param_t *param)
{
	xr_ble_mesh_gen_onoff_srv_t *srv;
	XR_LOGI(TAG, "event 0x%02x, opcode 0x%04x, src 0x%04x, dst 0x%04x",
		event, param->ctx.recv_op, param->ctx.addr, param->ctx.recv_dst);

	switch (event) {
	case XR_BLE_MESH_GENERIC_SERVER_STATE_CHANGE_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_GENERIC_SERVER_STATE_CHANGE_EVT");
		if (param->ctx.recv_op == XR_BLE_MESH_MODEL_OP_GEN_ONOFF_SET ||
			param->ctx.recv_op == XR_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK) {
			XR_LOGI(TAG, "onoff 0x%02x", param->value.state_change.onoff_set.onoff);
			example_change_led_state(param->model, &param->ctx, param->value.state_change.onoff_set.onoff);
		}
		break;
	case XR_BLE_MESH_GENERIC_SERVER_RECV_GET_MSG_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_GENERIC_SERVER_RECV_GET_MSG_EVT");
		if (param->ctx.recv_op == XR_BLE_MESH_MODEL_OP_GEN_ONOFF_GET) {
			srv = param->model->user_data;
			XR_LOGI(TAG, "onoff 0x%02x", srv->state.onoff);
			example_handle_gen_onoff_msg(param->model, &param->ctx, NULL);
		}
		break;
	case XR_BLE_MESH_GENERIC_SERVER_RECV_SET_MSG_EVT:
		XR_LOGI(TAG, "XR_BLE_MESH_GENERIC_SERVER_RECV_SET_MSG_EVT");
		if (param->ctx.recv_op == XR_BLE_MESH_MODEL_OP_GEN_ONOFF_SET ||
			param->ctx.recv_op == XR_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK) {
			XR_LOGI(TAG, "onoff 0x%02x, tid 0x%02x", param->value.set.onoff.onoff, param->value.set.onoff.tid);
			if (param->value.set.onoff.op_en) {
				XR_LOGI(TAG, "trans_time 0x%02x, delay 0x%02x",
					param->value.set.onoff.trans_time, param->value.set.onoff.delay);
			}
			example_handle_gen_onoff_msg(param->model, &param->ctx, &param->value.set.onoff);
		}
		break;
	default:
		XR_LOGE(TAG, "Unknown Generic Server event 0x%02x", event);
		break;
	}
}

static void example_ble_mesh_node_config_server_cb(xr_ble_mesh_cfg_server_cb_event_t event,
											  xr_ble_mesh_cfg_server_cb_param_t *param)
{
	if (event == XR_BLE_MESH_CFG_SERVER_STATE_CHANGE_EVT) {
		switch (param->ctx.recv_op) {
		case XR_BLE_MESH_MODEL_OP_APP_KEY_ADD:
			XR_LOGI(TAG, "XR_BLE_MESH_MODEL_OP_APP_KEY_ADD");
			XR_LOGI(TAG, "net_idx 0x%04x, app_idx 0x%04x",
				param->value.state_change.appkey_add.net_idx,
				param->value.state_change.appkey_add.app_idx);
			XR_LOG_BUFFER_HEX("AppKey", param->value.state_change.appkey_add.app_key, 16);
			break;
		case XR_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
			XR_LOGI(TAG, "XR_BLE_MESH_MODEL_OP_MODEL_APP_BIND");
			XR_LOGI(TAG, "elem_addr 0x%04x, app_idx 0x%04x, cid 0x%04x, mod_id 0x%04x",
				param->value.state_change.mod_app_bind.element_addr,
				param->value.state_change.mod_app_bind.app_idx,
				param->value.state_change.mod_app_bind.company_id,
				param->value.state_change.mod_app_bind.model_id);
			break;
		case XR_BLE_MESH_MODEL_OP_MODEL_SUB_ADD:
			XR_LOGI(TAG, "XR_BLE_MESH_MODEL_OP_MODEL_SUB_ADD");
			XR_LOGI(TAG, "elem_addr 0x%04x, sub_addr 0x%04x, cid 0x%04x, mod_id 0x%04x",
				param->value.state_change.mod_sub_add.element_addr,
				param->value.state_change.mod_sub_add.sub_addr,
				param->value.state_change.mod_sub_add.company_id,
				param->value.state_change.mod_sub_add.model_id);
			break;
		default:
			break;
		}
	}
}

static xr_err_t ble_mesh_node_init(void)
{
	xr_err_t err = XR_OK;

	xr_ble_mesh_register_prov_callback(example_ble_mesh_node_provisioning_cb);
	xr_ble_mesh_register_config_server_callback(example_ble_mesh_node_config_server_cb);
	xr_ble_mesh_register_generic_server_callback(example_ble_mesh_node_generic_server_cb);

	err = xr_ble_mesh_init(&provision, &composition);
	if (err != XR_OK) {
		XR_LOGE(TAG, "Failed to initialize mesh stack (err %d)", err);
		return err;
	}

	err = xr_ble_mesh_node_prov_enable(XR_BLE_MESH_PROV_ADV);
	if (err != XR_OK) {
		XR_LOGE(TAG, "Failed to enable mesh node (err %d)", err);
		return err;
	}

	XR_LOGI(TAG, "BLE Mesh Node initialized");
	led_onoff = 1;

	XR_LOGI(TAG,"led_g state is %d,prov_complete", led_onoff);

	return err;
}

enum cmd_status cmd_node_init_exec(char *cmd)
{
	xr_err_t err;
	char val = 1;
	ble_mesh_get_dev_uuid(dev_uuid);
	bt_mesh_node_set(val);

	/* Initialize the Bluetooth Mesh Subsystem */
	err = ble_mesh_node_init();
	if (err) {
		XR_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
	}
}

/*
    $prov init

*/
static const struct cmd_data g_node_cmds[] = {
	{ "init",                cmd_node_init_exec },
};

enum cmd_status cmd_node_exec(char *cmd)
{
	return cmd_exec(cmd, g_node_cmds, cmd_nitems(g_node_cmds));
}

