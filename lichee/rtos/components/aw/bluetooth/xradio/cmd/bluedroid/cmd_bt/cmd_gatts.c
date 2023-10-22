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

/****************************************************************************
*
* This demo showcases BLE GATT server. It can send adv data, be connected by client.
* Run the gatt_client demo, the client demo will automatically connect to the gatt_server demo.
* Client demo will enable gatt_server's notify after connection. The two devices will then exchange
* data.
*
****************************************************************************/
#include "cmd_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xr_log.h"
#include "xr_bt.h"

#include "xr_gap_ble_api.h"
#include "xr_gatts_api.h"
#include "xr_bt_defs.h"
#include "xr_bt_main.h"
#include "xr_gatt_common_api.h"

#define GATTS_TAG "GATTS_DEMO"

#define WALKAROUND_GATTS_ADV_FAILED 1

///Declare the static function
static void gatts_profile_a_event_handler(xr_gatts_cb_event_t event, xr_gatt_if_t gatts_if, xr_ble_gatts_cb_param_t *param);
static void gatts_profile_b_event_handler(xr_gatts_cb_event_t event, xr_gatt_if_t gatts_if, xr_ble_gatts_cb_param_t *param);

#define GATTS_SERVICE_UUID_TEST_A   0x00FF
#define GATTS_CHAR_UUID_TEST_A      0xFF01
#define GATTS_DESCR_UUID_TEST_A     0x3333
#define GATTS_NUM_HANDLE_TEST_A     4

#define GATTS_SERVICE_UUID_TEST_B   0x00EE
#define GATTS_CHAR_UUID_TEST_B      0xEE01
#define GATTS_DESCR_UUID_TEST_B     0x2222
#define GATTS_NUM_HANDLE_TEST_B     4

#define TEST_DEVICE_NAME            "XR_GATTS_DEMO"
#define TEST_MANUFACTURER_DATA_LEN  17

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40

#define PREPARE_BUF_MAX_SIZE 1024

uint8_t gatts_device_name[25] = {0};

static uint8_t char1_str[] = {0x11,0x22,0x33};
static xr_gatt_char_prop_t a_property = 0;
static xr_gatt_char_prop_t b_property = 0;

static xr_attr_value_t gatts_demo_char1_val =
{
	.attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
	.attr_len     = sizeof(char1_str),
	.attr_value   = char1_str,
};

static uint8_t adv_config_done = 0;
#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)

#ifdef CONFIG_SET_RAW_ADV_DATA
static uint8_t raw_adv_data[] = {
		0x02, 0x01, 0x06,
		0x02, 0x0a, 0xeb, 0x03, 0x03, 0xab, 0xcd
};
static uint8_t raw_scan_rsp_data[] = {
		0x0f, 0x09, 0x45, 0x53, 0x50, 0x5f, 0x47, 0x41, 0x54, 0x54, 0x53, 0x5f, 0x44,
		0x45, 0x4d, 0x4f
};
#else

static uint8_t adv_service_uuid128[32] = {
	/* LSB <--------------------------------------------------------------------------------> MSB */
	//first uuid, 16bit, [12],[13] is the value
	0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
	//second uuid, 32bit, [12], [13], [14], [15] is the value
	0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

// The length of adv data must be less than 31 bytes
//static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
//adv data
static xr_ble_adv_data_t adv_data = {
	.set_scan_rsp = false,
	.include_name = true,
	.include_txpower = false,
	.min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
	.max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
	.appearance = 0x00,
	.manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
	.p_manufacturer_data =  NULL, //&test_manufacturer[0],
	.service_data_len = 0,
	.p_service_data = NULL,
	.service_uuid_len = sizeof(adv_service_uuid128),
	.p_service_uuid = adv_service_uuid128,
	.flag = (XR_BLE_ADV_FLAG_GEN_DISC | XR_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
// scan response data
static xr_ble_adv_data_t scan_rsp_data = {
	.set_scan_rsp = true,
	.include_name = true,
	.include_txpower = true,
	//.min_interval = 0x0006,
	//.max_interval = 0x0010,
	.appearance = 0x00,
	.manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
	.p_manufacturer_data =  NULL, //&test_manufacturer[0],
	.service_data_len = 0,
	.p_service_data = NULL,
	.service_uuid_len = sizeof(adv_service_uuid128),
	.p_service_uuid = adv_service_uuid128,
	.flag = (XR_BLE_ADV_FLAG_GEN_DISC | XR_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

#endif /* CONFIG_SET_RAW_ADV_DATA */

static xr_ble_adv_params_t adv_params = {
	.adv_int_min        = 0x20,
	.adv_int_max        = 0x40,
	.adv_type           = ADV_TYPE_IND,
	.own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
	//.peer_addr            =
	//.peer_addr_type       =
	.channel_map        = ADV_CHNL_ALL,
	.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

#define PROFILE_NUM 2
#define PROFILE_A_APP_ID 0
#define PROFILE_B_APP_ID 1

struct gatts_profile_inst {
	xr_gatts_cb_t gatts_cb;
	uint16_t gatts_if;
	uint16_t app_id;
	uint16_t conn_id;
	uint16_t service_handle;
	xr_gatt_srvc_id_t service_id;
	uint16_t char_handle;
	xr_bt_uuid_t char_uuid;
	xr_gatt_perm_t perm;
	xr_gatt_char_prop_t property;
	uint16_t descr_handle;
	xr_bt_uuid_t descr_uuid;
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by XR_GATTS_REG_EVT */
static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
	[PROFILE_A_APP_ID] = {
		.gatts_cb = gatts_profile_a_event_handler,
		.gatts_if = XR_GATT_IF_NONE,       /* Not get the gatt_if, so initial is XR_GATT_IF_NONE */
	},
	[PROFILE_B_APP_ID] = {
		.gatts_cb = gatts_profile_b_event_handler,                   /* This demo does not implement, similar as profile A */
		.gatts_if = XR_GATT_IF_NONE,       /* Not get the gatt_if, so initial is XR_GATT_IF_NONE */
	},
};

typedef struct {
	uint8_t                 *prepare_buf;
	int                     prepare_len;
} prepare_type_env_t;

static prepare_type_env_t a_prepare_write_env;
static prepare_type_env_t b_prepare_write_env;

void example_write_event_env(xr_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, xr_ble_gatts_cb_param_t *param);
void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, xr_ble_gatts_cb_param_t *param);

static void gap_event_handler(xr_gap_ble_cb_event_t event, xr_ble_gap_cb_param_t *param)
{
	switch (event) {
#ifdef CONFIG_SET_RAW_ADV_DATA
	case XR_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
		adv_config_done &= (~adv_config_flag);
		if (adv_config_done==0) {
			xr_ble_gap_start_advertising(&adv_params);
		}
		break;
	case XR_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
		adv_config_done &= (~scan_rsp_config_flag);
		if (adv_config_done==0) {
			xr_ble_gap_start_advertising(&adv_params);
		}
		break;
#else
	case XR_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
		adv_config_done &= (~adv_config_flag);
		if (adv_config_done == 0) {
			xr_ble_gap_start_advertising(&adv_params);
		}
		break;
	case XR_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
		adv_config_done &= (~scan_rsp_config_flag);
		if (adv_config_done == 0) {
			xr_ble_gap_start_advertising(&adv_params);
		}
		break;
#endif
	case XR_GAP_BLE_ADV_START_COMPLETE_EVT:
		//advertising start complete event to indicate advertising start successfully or failed
		if (param->adv_start_cmpl.status != XR_BT_STATUS_SUCCESS) {
			XR_LOGE(GATTS_TAG, "Advertising start failed\n");
		}
		break;
	case XR_GAP_BLE_ADV_STOP_COMPLETE_EVT:
		if (param->adv_stop_cmpl.status != XR_BT_STATUS_SUCCESS) {
			XR_LOGE(GATTS_TAG, "Advertising stop failed\n");
		} else {
			XR_LOGI(GATTS_TAG, "Stop adv successfully\n");
		}
		break;
	case XR_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
		 XR_LOGI(GATTS_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
				  param->update_conn_params.status,
				  param->update_conn_params.min_int,
				  param->update_conn_params.max_int,
				  param->update_conn_params.conn_int,
				  param->update_conn_params.latency,
				  param->update_conn_params.timeout);
		break;
	default:
		break;
	}
}

void example_write_event_env(xr_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, xr_ble_gatts_cb_param_t *param)
{
	xr_gatt_status_t status = XR_GATT_OK;
	if (param->write.need_rsp) {
		if (param->write.is_prep) {
			if (prepare_write_env->prepare_buf == NULL) {
				prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE*sizeof(uint8_t));
				prepare_write_env->prepare_len = 0;
				if (prepare_write_env->prepare_buf == NULL) {
					XR_LOGE(GATTS_TAG, "Gatt_server prep no mem\n");
					status = XR_GATT_NO_RESOURCES;
				}
			} else {
				if (param->write.offset > PREPARE_BUF_MAX_SIZE) {
					status = XR_GATT_INVALID_OFFSET;
				} else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
					status = XR_GATT_INVALID_ATTR_LEN;
				}
			}

			xr_gatt_rsp_t *gatt_rsp = (xr_gatt_rsp_t *)malloc(sizeof(xr_gatt_rsp_t));
			if (!gatt_rsp) {
				XR_LOGE(GATTS_TAG, "gatt_rsp malloc error\n");
				return;
			}
			gatt_rsp->attr_value.len = param->write.len;
			gatt_rsp->attr_value.handle = param->write.handle;
			gatt_rsp->attr_value.offset = param->write.offset;
			gatt_rsp->attr_value.auth_req = XR_GATT_AUTH_REQ_NONE;
			memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
			xr_err_t response_err = xr_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
			if (response_err != XR_OK) {
				XR_LOGE(GATTS_TAG, "Send response error\n");
			}
			free(gatt_rsp);
			if (status != XR_GATT_OK) {
				return;
			}
			memcpy(prepare_write_env->prepare_buf + param->write.offset,
				   param->write.value,
				   param->write.len);
			prepare_write_env->prepare_len += param->write.len;

		} else {
			xr_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
		}
	}
}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, xr_ble_gatts_cb_param_t *param)
{
	if (param->exec_write.exec_write_flag == XR_GATT_PREP_WRITE_EXEC) {
		xr_log_buffer_hex(GATTS_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
	} else {
		XR_LOGI(GATTS_TAG,"XR_GATT_PREP_WRITE_CANCEL");
	}
	if (prepare_write_env->prepare_buf) {
		free(prepare_write_env->prepare_buf);
		prepare_write_env->prepare_buf = NULL;
	}
	prepare_write_env->prepare_len = 0;
}

static void gatts_profile_a_event_handler(xr_gatts_cb_event_t event, xr_gatt_if_t gatts_if, xr_ble_gatts_cb_param_t *param)
{
	int err = -1;
	switch (event) {
	case XR_GATTS_REG_EVT:
		XR_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
		gl_profile_tab[PROFILE_A_APP_ID].service_id.is_primary = true;
		gl_profile_tab[PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
		gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.len = XR_UUID_LEN_16;
		gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;

		xr_err_t set_dev_name_ret = xr_ble_gap_set_device_name(gatts_device_name);
		if (set_dev_name_ret) {
			XR_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
		}
#ifdef CONFIG_SET_RAW_ADV_DATA
		xr_err_t raw_adv_ret = xr_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
		if (raw_adv_ret) {
			XR_LOGE(GATTS_TAG, "config raw adv data failed, error code = %x ", raw_adv_ret);
		}
		adv_config_done |= adv_config_flag;
		xr_err_t raw_scan_ret = xr_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
		if (raw_scan_ret) {
			XR_LOGE(GATTS_TAG, "config raw scan rsp data failed, error code = %x", raw_scan_ret);
		}
		adv_config_done |= scan_rsp_config_flag;
#else
		//config adv data
		xr_err_t ret = xr_ble_gap_config_adv_data(&adv_data);
		if (ret) {
			XR_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
		}
		adv_config_done |= adv_config_flag;
		//config scan response data
		ret = xr_ble_gap_config_adv_data(&scan_rsp_data);
		if (ret) {
			XR_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", ret);
		}
		adv_config_done |= scan_rsp_config_flag;

#endif
		xr_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_A_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_A);
		break;
	case XR_GATTS_READ_EVT: {
		XR_LOGI(GATTS_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
		xr_gatt_rsp_t rsp;
		memset(&rsp, 0, sizeof(xr_gatt_rsp_t));
		rsp.attr_value.handle = param->read.handle;
		rsp.attr_value.len = 4;
		rsp.attr_value.value[0] = 0xde;
		rsp.attr_value.value[1] = 0xed;
		rsp.attr_value.value[2] = 0xbe;
		rsp.attr_value.value[3] = 0xef;
		xr_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
									XR_GATT_OK, &rsp);
		break;
	}
	case XR_GATTS_WRITE_EVT: {
		XR_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
		if (!param->write.is_prep) {
			XR_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
			xr_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
			if (gl_profile_tab[PROFILE_A_APP_ID].descr_handle == param->write.handle && param->write.len == 2) {
				uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
				if (descr_value == 0x0001){
					if (a_property & XR_GATT_CHAR_PROP_BIT_NOTIFY) {
						XR_LOGI(GATTS_TAG, "notify enable");
						uint8_t notify_data[15];
						for (int i = 0; i < sizeof(notify_data); ++i) {
							notify_data[i] = i%0xff;
						}
						//the size of notify_data[] need less than MTU size
						xr_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
												sizeof(notify_data), notify_data, false);
					}
				} else if (descr_value == 0x0002) {
					if (a_property & XR_GATT_CHAR_PROP_BIT_INDICATE) {
						XR_LOGI(GATTS_TAG, "indicate enable");
						uint8_t indicate_data[15];
						for (int i = 0; i < sizeof(indicate_data); ++i) {
							indicate_data[i] = i%0xff;
						}
						//the size of indicate_data[] need less than MTU size
						xr_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
												sizeof(indicate_data), indicate_data, true);
					}
				} else if (descr_value == 0x0000) {
					XR_LOGI(GATTS_TAG, "notify/indicate disable ");
				} else {
					XR_LOGE(GATTS_TAG, "unknown descr value");
					xr_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
				}
			}
		}
		example_write_event_env(gatts_if, &a_prepare_write_env, param);
		break;
	}
	case XR_GATTS_EXEC_WRITE_EVT:
		XR_LOGI(GATTS_TAG,"XR_GATTS_EXEC_WRITE_EVT");
		xr_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, XR_GATT_OK, NULL);
		example_exec_write_event_env(&a_prepare_write_env, param);
		break;
	case XR_GATTS_MTU_EVT:
		XR_LOGI(GATTS_TAG, "XR_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
		break;
	case XR_GATTS_UNREG_EVT:
		break;
	case XR_GATTS_CREATE_EVT:
		XR_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
		gl_profile_tab[PROFILE_A_APP_ID].service_handle = param->create.service_handle;
		gl_profile_tab[PROFILE_A_APP_ID].char_uuid.len = XR_UUID_LEN_16;
		gl_profile_tab[PROFILE_A_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_A;

		xr_ble_gatts_start_service(gl_profile_tab[PROFILE_A_APP_ID].service_handle);
		a_property = XR_GATT_CHAR_PROP_BIT_READ | XR_GATT_CHAR_PROP_BIT_WRITE | XR_GATT_CHAR_PROP_BIT_NOTIFY;
		xr_err_t add_char_ret = xr_ble_gatts_add_char(gl_profile_tab[PROFILE_A_APP_ID].service_handle, &gl_profile_tab[PROFILE_A_APP_ID].char_uuid,
														XR_GATT_PERM_READ | XR_GATT_PERM_WRITE,
														a_property,
														&gatts_demo_char1_val, NULL);
		if (add_char_ret) {
			XR_LOGE(GATTS_TAG, "add char failed, error code =%x",add_char_ret);
		}
		break;
	case XR_GATTS_ADD_INCL_SRVC_EVT:
		break;
	case XR_GATTS_ADD_CHAR_EVT: {
		uint16_t length = 0;
		const uint8_t *prf_char;

		XR_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
				param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
		gl_profile_tab[PROFILE_A_APP_ID].char_handle = param->add_char.attr_handle;
		gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.len = XR_UUID_LEN_16;
		gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.uuid.uuid16 = XR_GATT_UUID_CHAR_CLIENT_CONFIG;
		xr_err_t get_attr_ret = xr_ble_gatts_get_attr_value(param->add_char.attr_handle,  &length, &prf_char);
		if (get_attr_ret == XR_FAIL) {
			XR_LOGE(GATTS_TAG, "ILLEGAL HANDLE");
		}

		XR_LOGI(GATTS_TAG, "the gatts demo char length = %x\n", length);
		for(int i = 0; i < length; i++) {
			XR_LOGI(GATTS_TAG, "prf_char[%x] =%x\n",i,prf_char[i]);
		}
		xr_err_t add_descr_ret = xr_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_A_APP_ID].service_handle, &gl_profile_tab[PROFILE_A_APP_ID].descr_uuid,
																XR_GATT_PERM_READ | XR_GATT_PERM_WRITE, NULL, NULL);
		if (add_descr_ret) {
			XR_LOGE(GATTS_TAG, "add char descr failed, error code =%x", add_descr_ret);
		}
		break;
	}
	case XR_GATTS_ADD_CHAR_DESCR_EVT:
		gl_profile_tab[PROFILE_A_APP_ID].descr_handle = param->add_char_descr.attr_handle;
		XR_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
				 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
		break;
	case XR_GATTS_DELETE_EVT:
		break;
	case XR_GATTS_START_EVT:
		XR_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
				 param->start.status, param->start.service_handle);
		break;
	case XR_GATTS_STOP_EVT:
		break;
	case XR_GATTS_CONNECT_EVT: {
		xr_ble_conn_update_params_t conn_params = {0};
		memcpy(conn_params.bda, param->connect.remote_bda, sizeof(xr_bd_addr_t));
		/* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
		conn_params.latency = 0;
		conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
		conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
		conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
		XR_LOGI(GATTS_TAG, "XR_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
				 param->connect.conn_id,
				 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
				 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
		gl_profile_tab[PROFILE_A_APP_ID].conn_id = param->connect.conn_id;
		//start sent the update connection parameters to the peer device.
		xr_ble_gap_update_conn_params(&conn_params);
		break;
	}
	case XR_GATTS_DISCONNECT_EVT:
		XR_LOGI(GATTS_TAG, "XR_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
#if WALKAROUND_GATTS_ADV_FAILED
		sleep(2);
#endif
		xr_ble_gap_start_advertising(&adv_params);
		break;
	case XR_GATTS_CONF_EVT:
		XR_LOGI(GATTS_TAG, "XR_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
		if (param->conf.status != XR_GATT_OK) {
			xr_log_buffer_hex(GATTS_TAG, param->conf.value, param->conf.len);
		}
		break;
	case XR_GATTS_OPEN_EVT:
	case XR_GATTS_CANCEL_OPEN_EVT:
	case XR_GATTS_CLOSE_EVT:
	case XR_GATTS_LISTEN_EVT:
	case XR_GATTS_CONGEST_EVT:
	default:
		break;
	}
}

static void gatts_profile_b_event_handler(xr_gatts_cb_event_t event, xr_gatt_if_t gatts_if, xr_ble_gatts_cb_param_t *param)
{
	switch (event) {
	case XR_GATTS_REG_EVT:
		XR_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
		gl_profile_tab[PROFILE_B_APP_ID].service_id.is_primary = true;
		gl_profile_tab[PROFILE_B_APP_ID].service_id.id.inst_id = 0x00;
		gl_profile_tab[PROFILE_B_APP_ID].service_id.id.uuid.len = XR_UUID_LEN_16;
		gl_profile_tab[PROFILE_B_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_B;

		xr_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_B_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_B);
		break;
	case XR_GATTS_READ_EVT: {
		XR_LOGI(GATTS_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
		xr_gatt_rsp_t rsp;
		memset(&rsp, 0, sizeof(xr_gatt_rsp_t));
		rsp.attr_value.handle = param->read.handle;
		rsp.attr_value.len = 4;
		rsp.attr_value.value[0] = 0xde;
		rsp.attr_value.value[1] = 0xed;
		rsp.attr_value.value[2] = 0xbe;
		rsp.attr_value.value[3] = 0xef;
		xr_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
									XR_GATT_OK, &rsp);
		break;
	}
	case XR_GATTS_WRITE_EVT: {
		XR_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d\n", param->write.conn_id, param->write.trans_id, param->write.handle);
		if (!param->write.is_prep) {
			XR_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
			xr_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
			if (gl_profile_tab[PROFILE_B_APP_ID].descr_handle == param->write.handle && param->write.len == 2) {
				uint16_t descr_value= param->write.value[1]<<8 | param->write.value[0];
				if (descr_value == 0x0001) {
					if (b_property & XR_GATT_CHAR_PROP_BIT_NOTIFY){
						XR_LOGI(GATTS_TAG, "notify enable");
						uint8_t notify_data[15];
						for (int i = 0; i < sizeof(notify_data); ++i) {
							notify_data[i] = i%0xff;
						}
						//the size of notify_data[] need less than MTU size
						xr_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_B_APP_ID].char_handle,
												sizeof(notify_data), notify_data, false);
					}
				} else if (descr_value == 0x0002) {
					if (b_property & XR_GATT_CHAR_PROP_BIT_INDICATE) {
						XR_LOGI(GATTS_TAG, "indicate enable");
						uint8_t indicate_data[15];
						for (int i = 0; i < sizeof(indicate_data); ++i) {
							indicate_data[i] = i%0xff;
						}
						//the size of indicate_data[] need less than MTU size
						xr_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_B_APP_ID].char_handle,
												sizeof(indicate_data), indicate_data, true);
					}
				} else if (descr_value == 0x0000) {
					XR_LOGI(GATTS_TAG, "notify/indicate disable ");
				} else {
					XR_LOGE(GATTS_TAG, "unknown value");
				}
			}
		}
		example_write_event_env(gatts_if, &b_prepare_write_env, param);
		break;
	}
	case XR_GATTS_EXEC_WRITE_EVT:
		XR_LOGI(GATTS_TAG,"XR_GATTS_EXEC_WRITE_EVT");
		xr_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, XR_GATT_OK, NULL);
		example_exec_write_event_env(&b_prepare_write_env, param);
		break;
	case XR_GATTS_MTU_EVT:
		XR_LOGI(GATTS_TAG, "XR_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
		break;
	case XR_GATTS_UNREG_EVT:
		break;
	case XR_GATTS_CREATE_EVT:
		XR_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
		gl_profile_tab[PROFILE_B_APP_ID].service_handle = param->create.service_handle;
		gl_profile_tab[PROFILE_B_APP_ID].char_uuid.len = XR_UUID_LEN_16;
		gl_profile_tab[PROFILE_B_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_B;

		xr_ble_gatts_start_service(gl_profile_tab[PROFILE_B_APP_ID].service_handle);
		b_property = XR_GATT_CHAR_PROP_BIT_READ | XR_GATT_CHAR_PROP_BIT_WRITE | XR_GATT_CHAR_PROP_BIT_NOTIFY;
		xr_err_t add_char_ret =xr_ble_gatts_add_char( gl_profile_tab[PROFILE_B_APP_ID].service_handle, &gl_profile_tab[PROFILE_B_APP_ID].char_uuid,
														XR_GATT_PERM_READ | XR_GATT_PERM_WRITE,
														b_property,
														NULL, NULL);
		if (add_char_ret) {
			XR_LOGE(GATTS_TAG, "add char failed, error code =%x",add_char_ret);
		}
		break;
	case XR_GATTS_ADD_INCL_SRVC_EVT:
		break;
	case XR_GATTS_ADD_CHAR_EVT:
		XR_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
				 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);

		gl_profile_tab[PROFILE_B_APP_ID].char_handle = param->add_char.attr_handle;
		gl_profile_tab[PROFILE_B_APP_ID].descr_uuid.len = XR_UUID_LEN_16;
		gl_profile_tab[PROFILE_B_APP_ID].descr_uuid.uuid.uuid16 = XR_GATT_UUID_CHAR_CLIENT_CONFIG;
		xr_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_B_APP_ID].service_handle, &gl_profile_tab[PROFILE_B_APP_ID].descr_uuid,
									 XR_GATT_PERM_READ | XR_GATT_PERM_WRITE,
									 NULL, NULL);
		break;
	case XR_GATTS_ADD_CHAR_DESCR_EVT:
		gl_profile_tab[PROFILE_B_APP_ID].descr_handle = param->add_char_descr.attr_handle;
		XR_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
				 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
		break;
	case XR_GATTS_DELETE_EVT:
		break;
	case XR_GATTS_START_EVT:
		XR_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
				 param->start.status, param->start.service_handle);
		break;
	case XR_GATTS_STOP_EVT:
		break;
	case XR_GATTS_CONNECT_EVT:
		XR_LOGI(GATTS_TAG, "CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
				 param->connect.conn_id,
				 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
				 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
		gl_profile_tab[PROFILE_B_APP_ID].conn_id = param->connect.conn_id;
		break;
	case XR_GATTS_CONF_EVT:
		XR_LOGI(GATTS_TAG, "XR_GATTS_CONF_EVT status %d attr_handle %d", param->conf.status, param->conf.handle);
		if (param->conf.status != XR_GATT_OK) {
			xr_log_buffer_hex(GATTS_TAG, param->conf.value, param->conf.len);
		}
	break;
	case XR_GATTS_DISCONNECT_EVT:
	case XR_GATTS_OPEN_EVT:
	case XR_GATTS_CANCEL_OPEN_EVT:
	case XR_GATTS_CLOSE_EVT:
	case XR_GATTS_LISTEN_EVT:
	case XR_GATTS_CONGEST_EVT:
	default:
		break;
	}
}

static void gatts_event_handler(xr_gatts_cb_event_t event, xr_gatt_if_t gatts_if, xr_ble_gatts_cb_param_t *param)
{
	/* If event is register event, store the gatts_if for each profile */
	if (event == XR_GATTS_REG_EVT) {
		if (param->reg.status == XR_GATT_OK) {
			gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
		} else {
			XR_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d\n",
					param->reg.app_id,
					param->reg.status);
			return;
		}
	}

	/* If the gatts_if equal to profile A, call profile A cb handler,
	 * so here call each profile's callback */
	do {
		int idx;
		for (idx = 0; idx < PROFILE_NUM; idx++) {
			if (gatts_if == XR_GATT_IF_NONE || /* XR_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
					gatts_if == gl_profile_tab[idx].gatts_if) {
				if (gl_profile_tab[idx].gatts_cb) {
					gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
				}
			}
		}
	} while (0);
}

enum cmd_status cmd_gatts_init_exec(char *cmd)
{
	int ret = XR_FAIL;

	int argc;
	char *argv[1];

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc != 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	strcpy(gatts_device_name, argv[0]);

	ret = xr_ble_gatts_register_callback(gatts_event_handler);
	if (ret) {
		XR_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
		return XR_FAIL;
	}
	ret = xr_ble_gap_register_callback(gap_event_handler);
	if (ret) {
		XR_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
		return XR_FAIL;
	}
	ret = xr_ble_gatts_app_register(PROFILE_A_APP_ID);
	if (ret) {
		XR_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
		return XR_FAIL;
	}
	ret = xr_ble_gatts_app_register(PROFILE_B_APP_ID);
	if (ret) {
		XR_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
		return XR_FAIL;
	}
	xr_err_t local_mtu_ret = xr_ble_gatt_set_local_mtu(500);
	if (local_mtu_ret) {
		XR_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
	}

	return XR_OK;
}

enum cmd_status cmd_gatts_adv_exec(char *cmd)
{
	//int ret = XR_FAIL;

	/*int argc;
	char *argv[1];

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc != 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	strcpy(gatts_device_name, argv[0]);
	*/
	xr_ble_gap_stop_advertising();
	usleep(100);
	xr_ble_gap_start_advertising(&adv_params);
	return XR_OK;
}

/*
    $gatts init <name>
    $gatts adv
*/
static const struct cmd_data g_gatts_cmds[] = {
	{ "init",               cmd_gatts_init_exec },
	{ "adv",                cmd_gatts_adv_exec  },
};

enum cmd_status cmd_gatts_exec(char *cmd)
{
	return cmd_exec(cmd, g_gatts_cmds, cmd_nitems(g_gatts_cmds));
}

