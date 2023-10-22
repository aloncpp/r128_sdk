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

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "xr_bt.h"
#include "xr_gap_ble_api.h"
#include "xr_gattc_api.h"
#include "xr_gatt_defs.h"
#include "xr_bt_main.h"
#include "xr_gatt_common_api.h"
#include "xr_log.h"

#define GATTC_TAG "GATTC_DEMO"

#define REMOTE_SERVICE_UUID        0x00FF
#define REMOTE_NOTIFY_CHAR_UUID    0xFF01
#define PROFILE_NUM      5
#define PROFILE_A_APP_ID 0
#define PROFILE_B_APP_ID 1
#define PROFILE_C_APP_ID 2
#define PROFILE_D_APP_ID 3
#define PROFILE_E_APP_ID 4

#define INVALID_HANDLE   0

//void print_hex_dump_bytes(const void *addr, unsigned int len);

static char con_cnt    = 0;
static bool get_server = false;
static xr_gattc_char_elem_t *char_elem_result   = NULL;
static xr_gattc_descr_elem_t *descr_elem_result = NULL;

/* eclare static functions */
static void xr_gap_cb(xr_gap_ble_cb_event_t event, xr_ble_gap_cb_param_t *param);
static void xr_gattc_cb(xr_gattc_cb_event_t event, xr_gatt_if_t gattc_if, xr_ble_gattc_cb_param_t *param);
static void gattc_profile_a_event_handler(xr_gattc_cb_event_t event, xr_gatt_if_t gattc_if, xr_ble_gattc_cb_param_t *param);
static void gattc_profile_b_event_handler(xr_gattc_cb_event_t event, xr_gatt_if_t gattc_if, xr_ble_gattc_cb_param_t *param);
static void gattc_profile_c_event_handler(xr_gattc_cb_event_t event, xr_gatt_if_t gattc_if, xr_ble_gattc_cb_param_t *param);
static void gattc_profile_d_event_handler(xr_gattc_cb_event_t event, xr_gatt_if_t gattc_if, xr_ble_gattc_cb_param_t *param);
static void gattc_profile_e_event_handler(xr_gattc_cb_event_t event, xr_gatt_if_t gattc_if, xr_ble_gattc_cb_param_t *param);

static xr_bt_uuid_t remote_filter_service_uuid = {
	.len = XR_UUID_LEN_16,
	.uuid = {.uuid16 = REMOTE_SERVICE_UUID,},
};

static xr_bt_uuid_t remote_filter_char_uuid = {
	.len = XR_UUID_LEN_16,
	.uuid = {.uuid16 = REMOTE_NOTIFY_CHAR_UUID,},
};

static xr_bt_uuid_t notify_descr_uuid = {
	.len = XR_UUID_LEN_16,
	.uuid = {.uuid16 = XR_GATT_UUID_CHAR_CLIENT_CONFIG,},
};

static bool conn_device_a   = false;
static bool conn_device_b   = false;
static bool conn_device_c   = false;
static bool conn_device_d   = false;
static bool conn_device_e   = false;

static bool get_service_a   = false;
static bool get_service_b   = false;
static bool get_service_c   = false;
static bool get_service_d   = false;
static bool get_service_e   = false;

static bool Isconnecting    = false;
static bool stop_scan_done  = false;

static char remote_device_name[5][20] = { 0 };
static bool scan_flag = false;


static xr_gattc_char_elem_t  *char_elem_result_a   = NULL;
static xr_gattc_descr_elem_t *descr_elem_result_a  = NULL;
static xr_gattc_char_elem_t  *char_elem_result_b   = NULL;
static xr_gattc_descr_elem_t *descr_elem_result_b  = NULL;
static xr_gattc_char_elem_t  *char_elem_result_c   = NULL;
static xr_gattc_descr_elem_t *descr_elem_result_c  = NULL;
static xr_gattc_char_elem_t  *char_elem_result_d   = NULL;
static xr_gattc_descr_elem_t *descr_elem_result_d  = NULL;
static xr_gattc_char_elem_t  *char_elem_result_e   = NULL;
static xr_gattc_descr_elem_t *descr_elem_result_e  = NULL;

static xr_ble_scan_params_t ble_scan_params = {
	.scan_type              = BLE_SCAN_TYPE_ACTIVE,
	.own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
	.scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
	.scan_interval          = 0x50,
	.scan_window            = 0x30,
	.scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

struct gattc_profile_inst {
	xr_gattc_cb_t gattc_cb;
	uint16_t gattc_if;
	uint16_t app_id;
	uint16_t conn_id;
	uint16_t service_start_handle;
	uint16_t service_end_handle;
	uint16_t char_handle;
	xr_bd_addr_t remote_bda;
};

/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by XR_GATTS_REG_EVT */
static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
	[PROFILE_A_APP_ID] = {
		.gattc_cb = gattc_profile_a_event_handler,
		.gattc_if = XR_GATT_IF_NONE,       /* Not get the gatt_if, so initial is XR_GATT_IF_NONE */
	},
	[PROFILE_B_APP_ID] = {
		.gattc_cb = gattc_profile_b_event_handler,
		.gattc_if = XR_GATT_IF_NONE,       /* Not get the gatt_if, so initial is XR_GATT_IF_NONE */
	},
	[PROFILE_C_APP_ID] = {
		.gattc_cb = gattc_profile_c_event_handler,
		.gattc_if = XR_GATT_IF_NONE,       /* Not get the gatt_if, so initial is XR_GATT_IF_NONE */
	},
	[PROFILE_D_APP_ID] = {
		.gattc_cb = gattc_profile_d_event_handler,
		.gattc_if = XR_GATT_IF_NONE,       /* Not get the gatt_if, so initial is XR_GATT_IF_NONE */
	},
	[PROFILE_E_APP_ID] = {
		.gattc_cb = gattc_profile_e_event_handler,
		.gattc_if = XR_GATT_IF_NONE,       /* Not get the gatt_if, so initial is XR_GATT_IF_NONE */
	},
};

static void start_scan(void)
{
	stop_scan_done = false;
	Isconnecting = false;
	uint32_t duration = 30;
	xr_ble_gap_start_scanning(duration);
}


static void gattc_profile_a_event_handler(xr_gattc_cb_event_t event, xr_gatt_if_t gattc_if, xr_ble_gattc_cb_param_t *param)
{

	xr_ble_gattc_cb_param_t *p_data = (xr_ble_gattc_cb_param_t *)param;

	switch (event) {
	case XR_GATTC_REG_EVT:
		XR_LOGI(GATTC_TAG, "REG_EVT");
		xr_err_t scan_ret = xr_ble_gap_set_scan_params(&ble_scan_params);
		if (scan_ret){
			XR_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
		}
		break;
	/* one device connect successfully, all profiles callback function will get the XR_GATTC_CONNECT_EVT,
	 so must compare the mac address to check which device is connected, so it is a good choice to use XR_GATTC_OPEN_EVT. */
	case XR_GATTC_CONNECT_EVT:
		break;
	case XR_GATTC_OPEN_EVT:
		if (p_data->open.status != XR_GATT_OK) {
			//open failed, ignore the first device, connect the second device
			XR_LOGE(GATTC_TAG, "connect device failed, status %d", p_data->open.status);
			conn_device_a = false;
			con_cnt--;
			//start_scan();
			break;
		}
		memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->open.remote_bda, 6);
		gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->open.conn_id;
		XR_LOGI(GATTC_TAG, "XR_GATTC_OPEN_EVT conn_id %d, if %d, status %d, mtu %d", p_data->open.conn_id, gattc_if, p_data->open.status, p_data->open.mtu);
		XR_LOGI(GATTC_TAG, "REMOTE BDA:");
		xr_log_buffer_hex(GATTC_TAG, p_data->open.remote_bda, sizeof(xr_bd_addr_t));
		xr_err_t mtu_ret = xr_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
		if (mtu_ret){
			XR_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
		}
		break;
	case XR_GATTC_CFG_MTU_EVT:
		if (param->cfg_mtu.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG,"Config mtu failed");
		}
		XR_LOGI(GATTC_TAG, "Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
		xr_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
		break;
	case XR_GATTC_SEARCH_RES_EVT: {
		XR_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
		XR_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
		if (p_data->search_res.srvc_id.uuid.len == XR_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID) {
			XR_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
			get_service_a = true;
			gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
			gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
		}
		break;
	}
	case XR_GATTC_SEARCH_CMPL_EVT:
		if (p_data->search_cmpl.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
			break;
		}
		if (get_service_a) {
			uint16_t count = 0;
			xr_gatt_status_t status = xr_ble_gattc_get_attr_count(gattc_if,
			                                                      p_data->search_cmpl.conn_id,
			                                                      XR_GATT_DB_CHARACTERISTIC,
			                                                      gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
			                                                      gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
			                                                      INVALID_HANDLE,
			                                                      &count);
			if (status != XR_GATT_OK){
				XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_attr_count error");
			}
			if (count > 0) {
				char_elem_result_a = (xr_gattc_char_elem_t *)malloc(sizeof(xr_gattc_char_elem_t) * count);
				if (!char_elem_result_a){
					XR_LOGE(GATTC_TAG, "gattc no mem");
				} else {
					status = xr_ble_gattc_get_char_by_uuid(gattc_if,
					                                       p_data->search_cmpl.conn_id,
					                                       gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
					                                       gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
					                                       remote_filter_char_uuid,
					                                       char_elem_result_a,
					                                       &count);
					if (status != XR_GATT_OK){
						XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_char_by_uuid error");
					}

					/*  Every service have only one char in our 'XR_GATTS_DEMO' demo, so we used first 'char_elem_result' */
					if (count > 0 && (char_elem_result_a[0].properties & XR_GATT_CHAR_PROP_BIT_NOTIFY)){
						gl_profile_tab[PROFILE_A_APP_ID].char_handle = char_elem_result_a[0].char_handle;
						xr_ble_gattc_register_for_notify (gattc_if, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, char_elem_result_a[0].char_handle);
					}
				}
				/* free char_elem_result */
				free(char_elem_result_a);
			}else {
				XR_LOGE(GATTC_TAG, "no char found");
			}
		}
		break;
	case XR_GATTC_REG_FOR_NOTIFY_EVT: {
		if (p_data->reg_for_notify.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "reg notify failed, error status =%x", p_data->reg_for_notify.status);
			break;
		}
		uint16_t count = 0;
		uint16_t notify_en = 1;
		xr_gatt_status_t ret_status = xr_ble_gattc_get_attr_count(gattc_if,
		                                                          gl_profile_tab[PROFILE_A_APP_ID].conn_id,
		                                                          XR_GATT_DB_DESCRIPTOR,
		                                                          gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
		                                                          gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
		                                                          gl_profile_tab[PROFILE_A_APP_ID].char_handle,
		                                                          &count);
		if (ret_status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_attr_count error");
		}
		if (count > 0) {
			descr_elem_result_a = (xr_gattc_descr_elem_t *)malloc(sizeof(xr_gattc_descr_elem_t) *count);
			if (!descr_elem_result_a) {
				XR_LOGE(GATTC_TAG, "malloc error, gattc no mem");
			} else {
				ret_status = xr_ble_gattc_get_descr_by_char_handle(gattc_if,
				                                                   gl_profile_tab[PROFILE_A_APP_ID].conn_id,
				                                                   p_data->reg_for_notify.handle,
				                                                   notify_descr_uuid,
				                                                   descr_elem_result_a,
				                                                   &count);
				if (ret_status != XR_GATT_OK){
					XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_descr_by_char_handle error");
				}

				/* Every char has only one descriptor in our 'XR_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
				if (count > 0 && descr_elem_result_a[0].uuid.len == XR_UUID_LEN_16 && descr_elem_result_a[0].uuid.uuid.uuid16 == XR_GATT_UUID_CHAR_CLIENT_CONFIG) {
					ret_status = xr_ble_gattc_write_char_descr(gattc_if,
					                                           gl_profile_tab[PROFILE_A_APP_ID].conn_id,
					                                           descr_elem_result_a[0].handle,
					                                           sizeof(notify_en),
					                                           (uint8_t *)&notify_en,
					                                           XR_GATT_WRITE_TYPE_RSP,
					                                           XR_GATT_AUTH_REQ_NONE);
				}

				if (ret_status != XR_GATT_OK) {
					XR_LOGE(GATTC_TAG, "xr_ble_gattc_write_char_descr error");
				}

				/* free descr_elem_result */
				free(descr_elem_result_a);
			}
		} else {
			XR_LOGE(GATTC_TAG, "decsr not found");
		}
		break;
	}
	case XR_GATTC_NOTIFY_EVT:
		XR_LOGI(GATTC_TAG, "XR_GATTC_NOTIFY_EVT, Receive notify value:");
		xr_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
		break;
	case XR_GATTC_WRITE_DESCR_EVT:
		if (p_data->write.status != XR_GATT_OK){
			XR_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
			break;
		}
		XR_LOGI(GATTC_TAG, "write descr success");
		uint8_t write_char_data[35];
		for (int i = 0; i < sizeof(write_char_data); ++i) {
			write_char_data[i] = i % 256;
		}
		xr_ble_gattc_write_char(gattc_if,
		                        gl_profile_tab[PROFILE_A_APP_ID].conn_id,
		                        gl_profile_tab[PROFILE_A_APP_ID].char_handle,
		                        sizeof(write_char_data),
		                        write_char_data,
		                        XR_GATT_WRITE_TYPE_RSP,
		                        XR_GATT_AUTH_REQ_NONE);
		break;
	case XR_GATTC_WRITE_CHAR_EVT:
		if (p_data->write.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "write char failed, error status = %x", p_data->write.status);
		} else {
			XR_LOGI(GATTC_TAG, "write char success");
		}
		start_scan();
		break;
	case XR_GATTC_SRVC_CHG_EVT: {
		xr_bd_addr_t bda;
		memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(xr_bd_addr_t));
		XR_LOGI(GATTC_TAG, "XR_GATTC_SRVC_CHG_EVT, bd_addr:%08x%04x",(bda[0] << 24) + (bda[1] << 16) + (bda[2] << 8) + bda[3],
		       (bda[4] << 8) + bda[5]);
		break;
	}
	case XR_GATTC_DISCONNECT_EVT:
		//Start scanning again
		start_scan();
		if (memcmp(p_data->disconnect.remote_bda, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, 6) == 0) {
			XR_LOGI(GATTC_TAG, "device a disconnect");
			conn_device_a = false;
			get_service_a = false;
			con_cnt--;
		}
		break;
	default:
		break;
	}
}

static void gattc_profile_b_event_handler(xr_gattc_cb_event_t event, xr_gatt_if_t gattc_if, xr_ble_gattc_cb_param_t *param)
{
	xr_ble_gattc_cb_param_t *p_data = (xr_ble_gattc_cb_param_t *)param;

	switch (event) {
	case XR_GATTC_REG_EVT:
		XR_LOGI(GATTC_TAG, "REG_EVT");
		break;
	case XR_GATTC_CONNECT_EVT:
		break;
	case XR_GATTC_OPEN_EVT:
		if (p_data->open.status != XR_GATT_OK) {
			//open failed, ignore the second device, connect the third device
			XR_LOGE(GATTC_TAG, "connect device failed, status %d", p_data->open.status);
			conn_device_b = false;
			con_cnt--;
			//start_scan();
			break;
		}
		memcpy(gl_profile_tab[PROFILE_B_APP_ID].remote_bda, p_data->open.remote_bda, 6);
		gl_profile_tab[PROFILE_B_APP_ID].conn_id = p_data->open.conn_id;
		XR_LOGI(GATTC_TAG, "XR_GATTC_OPEN_EVT conn_id %d, if %d, status %d, mtu %d", p_data->open.conn_id, gattc_if, p_data->open.status, p_data->open.mtu);
		XR_LOGI(GATTC_TAG, "REMOTE BDA:");
		xr_log_buffer_hex(GATTC_TAG, p_data->open.remote_bda, sizeof(xr_bd_addr_t));
		xr_err_t mtu_ret = xr_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
		if (mtu_ret) {
			XR_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
		}
		break;
	case XR_GATTC_CFG_MTU_EVT:
		if (param->cfg_mtu.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG,"Config mtu failed");
		}
		XR_LOGI(GATTC_TAG, "Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
		xr_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
		break;
	case XR_GATTC_SEARCH_RES_EVT: {
		XR_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
		XR_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
		if (p_data->search_res.srvc_id.uuid.len == XR_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID) {
			XR_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
			get_service_b = true;
			gl_profile_tab[PROFILE_B_APP_ID].service_start_handle = p_data->search_res.start_handle;
			gl_profile_tab[PROFILE_B_APP_ID].service_end_handle = p_data->search_res.end_handle;
		}
		break;
	}
	case XR_GATTC_SEARCH_CMPL_EVT:
		if (p_data->search_cmpl.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
			break;
		}
		if (get_service_b) {
			uint16_t count = 0;
			xr_gatt_status_t status = xr_ble_gattc_get_attr_count(gattc_if,
			                                                      p_data->search_cmpl.conn_id,
			                                                      XR_GATT_DB_CHARACTERISTIC,
			                                                      gl_profile_tab[PROFILE_B_APP_ID].service_start_handle,
			                                                      gl_profile_tab[PROFILE_B_APP_ID].service_end_handle,
			                                                      INVALID_HANDLE,
			                                                      &count);
			if (status != XR_GATT_OK) {
				XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_attr_count error");
			}

			if (count > 0) {
				char_elem_result_b = (xr_gattc_char_elem_t *)malloc(sizeof(xr_gattc_char_elem_t) * count);
				if (!char_elem_result_b) {
					XR_LOGE(GATTC_TAG, "gattc no mem");
				} else {
					status = xr_ble_gattc_get_char_by_uuid(gattc_if,
					                                       p_data->search_cmpl.conn_id,
					                                       gl_profile_tab[PROFILE_B_APP_ID].service_start_handle,
					                                       gl_profile_tab[PROFILE_B_APP_ID].service_end_handle,
					                                       remote_filter_char_uuid,
					                                       char_elem_result_b,
					                                       &count);
					if (status != XR_GATT_OK) {
						XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_char_by_uuid error");
					}

					/*  Every service have only one char in our 'XR_GATTS_DEMO' demo, so we used first 'char_elem_result' */
					if (count > 0 && (char_elem_result_b[0].properties & XR_GATT_CHAR_PROP_BIT_NOTIFY)) {
						gl_profile_tab[PROFILE_B_APP_ID].char_handle = char_elem_result_b[0].char_handle;
						xr_ble_gattc_register_for_notify (gattc_if, gl_profile_tab[PROFILE_B_APP_ID].remote_bda, char_elem_result_b[0].char_handle);
					}
				}
				/* free char_elem_result */
				free(char_elem_result_b);
			}else{
				XR_LOGE(GATTC_TAG, "no char found");
			}
		}
		break;
	case XR_GATTC_REG_FOR_NOTIFY_EVT: {

		if (p_data->reg_for_notify.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "reg notify failed, error status =%x", p_data->reg_for_notify.status);
			break;
		}
		uint16_t count = 0;
		uint16_t notify_en = 1;
		xr_gatt_status_t ret_status = xr_ble_gattc_get_attr_count(gattc_if,
		                                                          gl_profile_tab[PROFILE_B_APP_ID].conn_id,
		                                                          XR_GATT_DB_DESCRIPTOR,
		                                                          gl_profile_tab[PROFILE_B_APP_ID].service_start_handle,
		                                                          gl_profile_tab[PROFILE_B_APP_ID].service_end_handle,
		                                                          gl_profile_tab[PROFILE_B_APP_ID].char_handle,
		                                                          &count);
		if (ret_status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_attr_count error");
		}
		if (count > 0) {
			descr_elem_result_b = (xr_gattc_descr_elem_t *)malloc(sizeof(xr_gattc_descr_elem_t) * count);
			if (!descr_elem_result_b) {
				XR_LOGE(GATTC_TAG, "malloc error, gattc no mem");
			} else {
				ret_status = xr_ble_gattc_get_descr_by_char_handle(gattc_if,
				                                                   gl_profile_tab[PROFILE_B_APP_ID].conn_id,
				                                                   p_data->reg_for_notify.handle,
				                                                   notify_descr_uuid,
				                                                   descr_elem_result_b,
				                                                   &count);
				if (ret_status != XR_GATT_OK) {
					XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_descr_by_char_handle error");
				}

				/* Every char has only one descriptor in our 'XR_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
				if (count > 0 && descr_elem_result_b[0].uuid.len == XR_UUID_LEN_16 && descr_elem_result_b[0].uuid.uuid.uuid16 == XR_GATT_UUID_CHAR_CLIENT_CONFIG) {
					ret_status = xr_ble_gattc_write_char_descr(gattc_if,
					                                           gl_profile_tab[PROFILE_B_APP_ID].conn_id,
					                                           descr_elem_result_b[0].handle,
					                                           sizeof(notify_en),
					                                           (uint8_t *)&notify_en,
					                                           XR_GATT_WRITE_TYPE_RSP,
					                                           XR_GATT_AUTH_REQ_NONE);
				}

				if (ret_status != XR_GATT_OK) {
					XR_LOGE(GATTC_TAG, "xr_ble_gattc_write_char_descr error");
				}

				/* free descr_elem_result */
				free(descr_elem_result_b);
			}
		} else{
			XR_LOGE(GATTC_TAG, "decsr not found");
		}
		break;
	}
	case XR_GATTC_NOTIFY_EVT:
		XR_LOGI(GATTC_TAG, "XR_GATTC_NOTIFY_EVT, Receive notify value:");
		xr_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
		break;
	case XR_GATTC_WRITE_DESCR_EVT:
		if (p_data->write.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
			break;
		}
		XR_LOGI(GATTC_TAG, "write descr success");
		uint8_t write_char_data[35];
		for (int i = 0; i < sizeof(write_char_data); ++i) {
			write_char_data[i] = i % 256;
		}
		xr_ble_gattc_write_char(gattc_if,
		                        gl_profile_tab[PROFILE_B_APP_ID].conn_id,
		                        gl_profile_tab[PROFILE_B_APP_ID].char_handle,
		                        sizeof(write_char_data),
		                        write_char_data,
		                        XR_GATT_WRITE_TYPE_RSP,
		                        XR_GATT_AUTH_REQ_NONE);
		break;
	case XR_GATTC_WRITE_CHAR_EVT:
		if (p_data->write.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "Write char failed, error status = %x", p_data->write.status);
		} else {
			XR_LOGI(GATTC_TAG, "Write char success");
		}
		start_scan();
		break;
	case XR_GATTC_SRVC_CHG_EVT: {
		xr_bd_addr_t bda;
		memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(xr_bd_addr_t));
		XR_LOGI(GATTC_TAG, "XR_GATTC_SRVC_CHG_EVT, bd_addr:%08x%04x",(bda[0] << 24) + (bda[1] << 16) + (bda[2] << 8) + bda[3],
		       (bda[4] << 8) + bda[5]);
		break;
	}
	case XR_GATTC_DISCONNECT_EVT:
		if (memcmp(p_data->disconnect.remote_bda, gl_profile_tab[PROFILE_B_APP_ID].remote_bda, 6) == 0) {
			XR_LOGI(GATTC_TAG, "device b disconnect");
			conn_device_b = false;
			get_service_b = false;
			con_cnt--;
		}
		break;
	default:
		break;
	}
}

static void gattc_profile_c_event_handler(xr_gattc_cb_event_t event, xr_gatt_if_t gattc_if, xr_ble_gattc_cb_param_t *param)
{
	xr_ble_gattc_cb_param_t *p_data = (xr_ble_gattc_cb_param_t *)param;

	switch (event) {
	case XR_GATTC_REG_EVT:
		XR_LOGI(GATTC_TAG, "REG_EVT");
		break;
	case XR_GATTC_CONNECT_EVT:
		break;
	case XR_GATTC_OPEN_EVT:
		if (p_data->open.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "connect device failed, status %d", p_data->open.status);
			conn_device_c = false;
			con_cnt--;
			//start_scan();
			break;
		}
		memcpy(gl_profile_tab[PROFILE_C_APP_ID].remote_bda, p_data->open.remote_bda, 6);
		gl_profile_tab[PROFILE_C_APP_ID].conn_id = p_data->open.conn_id;
		XR_LOGI(GATTC_TAG, "XR_GATTC_OPEN_EVT conn_id %d, if %d, status %d, mtu %d", p_data->open.conn_id, gattc_if, p_data->open.status, p_data->open.mtu);
		XR_LOGI(GATTC_TAG, "REMOTE BDA:");
		xr_log_buffer_hex(GATTC_TAG, p_data->open.remote_bda, sizeof(xr_bd_addr_t));
		xr_err_t mtu_ret = xr_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
		if (mtu_ret) {
			XR_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
		}
		break;
	case XR_GATTC_CFG_MTU_EVT:
		if (param->cfg_mtu.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG,"Config mtu failed");
		}
		XR_LOGI(GATTC_TAG, "Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
		xr_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
		break;
	case XR_GATTC_SEARCH_RES_EVT: {
		XR_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
		XR_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
		if (p_data->search_res.srvc_id.uuid.len == XR_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID) {
			XR_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
			get_service_c = true;
			gl_profile_tab[PROFILE_C_APP_ID].service_start_handle = p_data->search_res.start_handle;
			gl_profile_tab[PROFILE_C_APP_ID].service_end_handle = p_data->search_res.end_handle;
		}
		break;
	}
	case XR_GATTC_SEARCH_CMPL_EVT:
		if (p_data->search_cmpl.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
			break;
		}
		if (get_service_c) {
			uint16_t count = 0;
			xr_gatt_status_t status = xr_ble_gattc_get_attr_count( gattc_if,
																	 p_data->search_cmpl.conn_id,
																	 XR_GATT_DB_CHARACTERISTIC,
																	 gl_profile_tab[PROFILE_C_APP_ID].service_start_handle,
																	 gl_profile_tab[PROFILE_C_APP_ID].service_end_handle,
																	 INVALID_HANDLE,
																	 &count);
			if (status != XR_GATT_OK) {
				XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_attr_count error");
			}

			if (count > 0){
				char_elem_result_c = (xr_gattc_char_elem_t *)malloc(sizeof(xr_gattc_char_elem_t) * count);
				if (!char_elem_result_c) {
					XR_LOGE(GATTC_TAG, "gattc no mem");
				} else {
					status = xr_ble_gattc_get_char_by_uuid( gattc_if,
															 p_data->search_cmpl.conn_id,
															 gl_profile_tab[PROFILE_C_APP_ID].service_start_handle,
															 gl_profile_tab[PROFILE_C_APP_ID].service_end_handle,
															 remote_filter_char_uuid,
															 char_elem_result_c,
															 &count);
					if (status != XR_GATT_OK){
						XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_char_by_uuid error");
					}

					/*  Every service have only one char in our 'XR_GATTS_DEMO' demo, so we used first 'char_elem_result' */
					if (count > 0 && (char_elem_result_c[0].properties & XR_GATT_CHAR_PROP_BIT_NOTIFY)){
						gl_profile_tab[PROFILE_C_APP_ID].char_handle = char_elem_result_c[0].char_handle;
						xr_ble_gattc_register_for_notify (gattc_if, gl_profile_tab[PROFILE_C_APP_ID].remote_bda, char_elem_result_c[0].char_handle);
					}
				}
				/* free char_elem_result */
				free(char_elem_result_c);
			}else{
				XR_LOGE(GATTC_TAG, "no char found");
			}
		}
		break;
	case XR_GATTC_REG_FOR_NOTIFY_EVT: {
		if (p_data->reg_for_notify.status != XR_GATT_OK){
			XR_LOGE(GATTC_TAG, "reg notify failed, error status =%x", p_data->reg_for_notify.status);
			break;
		}
		uint16_t count = 0;
		uint16_t notify_en = 1;
		xr_gatt_status_t ret_status = xr_ble_gattc_get_attr_count( gattc_if,
																	 gl_profile_tab[PROFILE_C_APP_ID].conn_id,
																	 XR_GATT_DB_DESCRIPTOR,
																	 gl_profile_tab[PROFILE_C_APP_ID].service_start_handle,
																	 gl_profile_tab[PROFILE_C_APP_ID].service_end_handle,
																	 gl_profile_tab[PROFILE_C_APP_ID].char_handle,
																	 &count);
		if (ret_status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_attr_count error");
		}
		if (count > 0) {
			descr_elem_result_c = (xr_gattc_descr_elem_t *)malloc(sizeof(xr_gattc_descr_elem_t) * count);
			if (!descr_elem_result_c) {
				XR_LOGE(GATTC_TAG, "malloc error, gattc no mem");
			} else {
				ret_status = xr_ble_gattc_get_descr_by_char_handle( gattc_if,
																	 gl_profile_tab[PROFILE_C_APP_ID].conn_id,
																	 p_data->reg_for_notify.handle,
																	 notify_descr_uuid,
																	 descr_elem_result_c,
																	 &count);
				if (ret_status != XR_GATT_OK) {
					XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_descr_by_char_handle error");
				}

				/* Every char has only one descriptor in our 'XR_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
				if (count > 0 && descr_elem_result_c[0].uuid.len == XR_UUID_LEN_16 && descr_elem_result_c[0].uuid.uuid.uuid16 == XR_GATT_UUID_CHAR_CLIENT_CONFIG) {
					ret_status = xr_ble_gattc_write_char_descr( gattc_if,
																 gl_profile_tab[PROFILE_C_APP_ID].conn_id,
																 descr_elem_result_c[0].handle,
																 sizeof(notify_en),
																 (uint8_t *)&notify_en,
																 XR_GATT_WRITE_TYPE_RSP,
																 XR_GATT_AUTH_REQ_NONE);
				}

				if (ret_status != XR_GATT_OK) {
					XR_LOGE(GATTC_TAG, "xr_ble_gattc_write_char_descr error");
				}

				/* free descr_elem_result */
				free(descr_elem_result_c);
			}
		} else {
			XR_LOGE(GATTC_TAG, "decsr not found");
		}
		break;
	}
	case XR_GATTC_NOTIFY_EVT:
		XR_LOGI(GATTC_TAG, "XR_GATTC_NOTIFY_EVT, Receive notify value:");
		xr_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
		break;
	case XR_GATTC_WRITE_DESCR_EVT:
		if (p_data->write.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
			break;
		}
		XR_LOGI(GATTC_TAG, "write descr success");
		uint8_t write_char_data[35];
		for (int i = 0; i < sizeof(write_char_data); ++i) {
			write_char_data[i] = i % 256;
		}
		xr_ble_gattc_write_char( gattc_if,
								  gl_profile_tab[PROFILE_C_APP_ID].conn_id,
								  gl_profile_tab[PROFILE_C_APP_ID].char_handle,
								  sizeof(write_char_data),
								  write_char_data,
								  XR_GATT_WRITE_TYPE_RSP,
								  XR_GATT_AUTH_REQ_NONE);
		break;
	case XR_GATTC_WRITE_CHAR_EVT:
		if (p_data->write.status != XR_GATT_OK){
			XR_LOGE(GATTC_TAG, "Write char failed, error status = %x", p_data->write.status);
			break;
		}
		XR_LOGI(GATTC_TAG, "Write char success");
		start_scan();
		break;
	case XR_GATTC_SRVC_CHG_EVT: {
		xr_bd_addr_t bda;
		memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(xr_bd_addr_t));
		XR_LOGI(GATTC_TAG, "XR_GATTC_SRVC_CHG_EVT, bd_addr:%08x%04x",(bda[0] << 24) + (bda[1] << 16) + (bda[2] << 8) + bda[3],
				 (bda[4] << 8) + bda[5]);
		break;
	}
	case XR_GATTC_DISCONNECT_EVT:
		if (memcmp(p_data->disconnect.remote_bda, gl_profile_tab[PROFILE_C_APP_ID].remote_bda, 6) == 0){
			XR_LOGI(GATTC_TAG, "device c disconnect");
			conn_device_c = false;
			get_service_c = false;
			con_cnt--;
		}
		break;
	default:
		break;
	}
}

static void gattc_profile_d_event_handler(xr_gattc_cb_event_t event, xr_gatt_if_t gattc_if, xr_ble_gattc_cb_param_t *param)
{
	xr_ble_gattc_cb_param_t *p_data = (xr_ble_gattc_cb_param_t *)param;

	switch (event) {
	case XR_GATTC_REG_EVT:
		XR_LOGI(GATTC_TAG, "REG_EVT");
		break;
	case XR_GATTC_CONNECT_EVT:
		break;
	case XR_GATTC_OPEN_EVT:
		if (p_data->open.status != XR_GATT_OK) {
			//open failed, ignore the second device, connect the third device
			XR_LOGE(GATTC_TAG, "connect device failed, status %d", p_data->open.status);
			conn_device_d = false;
			con_cnt--;
			//start_scan();
			break;
		}
		memcpy(gl_profile_tab[PROFILE_D_APP_ID].remote_bda, p_data->open.remote_bda, 6);
		gl_profile_tab[PROFILE_D_APP_ID].conn_id = p_data->open.conn_id;
		XR_LOGI(GATTC_TAG, "XR_GATTC_OPEN_EVT conn_id %d, if %d, status %d, mtu %d", p_data->open.conn_id, gattc_if, p_data->open.status, p_data->open.mtu);
		XR_LOGI(GATTC_TAG, "REMOTE BDA:");
		xr_log_buffer_hex(GATTC_TAG, p_data->open.remote_bda, sizeof(xr_bd_addr_t));
		xr_err_t mtu_ret = xr_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
		if (mtu_ret) {
			XR_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
		}
		break;
	case XR_GATTC_CFG_MTU_EVT:
		if (param->cfg_mtu.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG,"Config mtu failed");
		}
		XR_LOGI(GATTC_TAG, "Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
		xr_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
		break;
	case XR_GATTC_SEARCH_RES_EVT: {
		XR_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
		XR_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
		if (p_data->search_res.srvc_id.uuid.len == XR_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID) {
			XR_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
			get_service_d = true;
			gl_profile_tab[PROFILE_D_APP_ID].service_start_handle = p_data->search_res.start_handle;
			gl_profile_tab[PROFILE_D_APP_ID].service_end_handle = p_data->search_res.end_handle;
		}
		break;
	}
	case XR_GATTC_SEARCH_CMPL_EVT:
		if (p_data->search_cmpl.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
			break;
		}
		if (get_service_d) {
			uint16_t count = 0;
			xr_gatt_status_t status = xr_ble_gattc_get_attr_count( gattc_if,
																	 p_data->search_cmpl.conn_id,
																	 XR_GATT_DB_CHARACTERISTIC,
																	 gl_profile_tab[PROFILE_D_APP_ID].service_start_handle,
																	 gl_profile_tab[PROFILE_D_APP_ID].service_end_handle,
																	 INVALID_HANDLE,
																	 &count);
			if (status != XR_GATT_OK) {
				XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_attr_count error");
			}

			if (count > 0) {
				char_elem_result_d = (xr_gattc_char_elem_t *)malloc(sizeof(xr_gattc_char_elem_t) * count);
				if (!char_elem_result_d) {
					XR_LOGE(GATTC_TAG, "gattc no mem");
				} else {
					status = xr_ble_gattc_get_char_by_uuid( gattc_if,
															 p_data->search_cmpl.conn_id,
															 gl_profile_tab[PROFILE_D_APP_ID].service_start_handle,
															 gl_profile_tab[PROFILE_D_APP_ID].service_end_handle,
															 remote_filter_char_uuid,
															 char_elem_result_d,
															 &count);
					if (status != XR_GATT_OK) {
						XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_char_by_uuid error");
					}

					/*  Every service have only one char in our 'XR_GATTS_DEMO' demo, so we used first 'char_elem_result' */
					if (count > 0 && (char_elem_result_d[0].properties & XR_GATT_CHAR_PROP_BIT_NOTIFY)) {
						gl_profile_tab[PROFILE_D_APP_ID].char_handle = char_elem_result_d[0].char_handle;
						xr_ble_gattc_register_for_notify (gattc_if, gl_profile_tab[PROFILE_D_APP_ID].remote_bda, char_elem_result_d[0].char_handle);
					}
				}
				/* free char_elem_result */
				free(char_elem_result_d);
			}else{
				XR_LOGE(GATTC_TAG, "no char found");
			}
		}
		break;
	case XR_GATTC_REG_FOR_NOTIFY_EVT: {
		if (p_data->reg_for_notify.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "reg notify failed, error status =%x", p_data->reg_for_notify.status);
			break;
		}
		uint16_t count = 0;
		uint16_t notify_en = 1;
		xr_gatt_status_t ret_status = xr_ble_gattc_get_attr_count( gattc_if,
																	 gl_profile_tab[PROFILE_D_APP_ID].conn_id,
																	 XR_GATT_DB_DESCRIPTOR,
																	 gl_profile_tab[PROFILE_D_APP_ID].service_start_handle,
																	 gl_profile_tab[PROFILE_D_APP_ID].service_end_handle,
																	 gl_profile_tab[PROFILE_D_APP_ID].char_handle,
																	 &count);
		if (ret_status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_attr_count error");
		}
		if (count > 0) {
			descr_elem_result_d = (xr_gattc_descr_elem_t *)malloc(sizeof(xr_gattc_descr_elem_t) * count);
			if (!descr_elem_result_d) {
				XR_LOGE(GATTC_TAG, "malloc error, gattc no mem");
			} else {
				ret_status = xr_ble_gattc_get_descr_by_char_handle( gattc_if,
																	 gl_profile_tab[PROFILE_D_APP_ID].conn_id,
																	 p_data->reg_for_notify.handle,
																	 notify_descr_uuid,
																	 descr_elem_result_d,
																	 &count);
				if (ret_status != XR_GATT_OK) {
					XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_descr_by_char_handle error");
				}

				/* Every char has only one descriptor in our 'XR_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
				if (count > 0 && descr_elem_result_d[0].uuid.len == XR_UUID_LEN_16 && descr_elem_result_d[0].uuid.uuid.uuid16 == XR_GATT_UUID_CHAR_CLIENT_CONFIG){
					ret_status = xr_ble_gattc_write_char_descr( gattc_if,
																 gl_profile_tab[PROFILE_D_APP_ID].conn_id,
																 descr_elem_result_d[0].handle,
																 sizeof(notify_en),
																 (uint8_t *)&notify_en,
																 XR_GATT_WRITE_TYPE_RSP,
																 XR_GATT_AUTH_REQ_NONE);
				}

				if (ret_status != XR_GATT_OK) {
					XR_LOGE(GATTC_TAG, "xr_ble_gattc_write_char_descr error");
				}

				/* free descr_elem_result */
				free(descr_elem_result_d);
			}
		} else {
			XR_LOGE(GATTC_TAG, "decsr not found");
		}
		break;
	}
	case XR_GATTC_NOTIFY_EVT:
		XR_LOGI(GATTC_TAG, "XR_GATTC_NOTIFY_EVT, Receive notify value:");
		xr_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
		break;
	case XR_GATTC_WRITE_DESCR_EVT:
		if (p_data->write.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
			break;
		}
		XR_LOGI(GATTC_TAG, "write descr success");
		uint8_t write_char_data[35];
		for (int i = 0; i < sizeof(write_char_data); ++i) {
			write_char_data[i] = i % 256;
		}
		xr_ble_gattc_write_char( gattc_if,
								  gl_profile_tab[PROFILE_D_APP_ID].conn_id,
								  gl_profile_tab[PROFILE_D_APP_ID].char_handle,
								  sizeof(write_char_data),
								  write_char_data,
								  XR_GATT_WRITE_TYPE_RSP,
								  XR_GATT_AUTH_REQ_NONE);
		break;
	case XR_GATTC_WRITE_CHAR_EVT:
		if (p_data->write.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "Write char failed, error status = %x", p_data->write.status);
		}else{
			XR_LOGI(GATTC_TAG, "Write char success");
		}
		start_scan();
		break;
	case XR_GATTC_SRVC_CHG_EVT: {
		xr_bd_addr_t bda;
		memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(xr_bd_addr_t));
		XR_LOGI(GATTC_TAG, "XR_GATTC_SRVC_CHG_EVT, bd_addr:%08x%04x",(bda[0] << 24) + (bda[1] << 16) + (bda[2] << 8) + bda[3],
				 (bda[4] << 8) + bda[5]);
		break;
	}
	case XR_GATTC_DISCONNECT_EVT:
		if (memcmp(p_data->disconnect.remote_bda, gl_profile_tab[PROFILE_D_APP_ID].remote_bda, 6) == 0) {
			XR_LOGI(GATTC_TAG, "device d disconnect");
			conn_device_d = false;
			get_service_d = false;
			con_cnt--;
		}
		break;
	default:
		break;
	}
}

static void gattc_profile_e_event_handler(xr_gattc_cb_event_t event, xr_gatt_if_t gattc_if, xr_ble_gattc_cb_param_t *param)
{
	xr_ble_gattc_cb_param_t *p_data = (xr_ble_gattc_cb_param_t *)param;

	switch (event) {
	case XR_GATTC_REG_EVT:
		XR_LOGI(GATTC_TAG, "REG_EVT");
		break;
	case XR_GATTC_CONNECT_EVT:
		break;
	case XR_GATTC_OPEN_EVT:
		if (p_data->open.status != XR_GATT_OK) {
			//open failed, ignore the second device, connect the third device
			XR_LOGE(GATTC_TAG, "connect device failed, status %d", p_data->open.status);
			conn_device_e = false;
			con_cnt--;
			//start_scan();
			break;
		}
		memcpy(gl_profile_tab[PROFILE_E_APP_ID].remote_bda, p_data->open.remote_bda, 6);
		gl_profile_tab[PROFILE_E_APP_ID].conn_id = p_data->open.conn_id;
		XR_LOGI(GATTC_TAG, "XR_GATTC_OPEN_EVT conn_id %d, if %d, status %d, mtu %d", p_data->open.conn_id, gattc_if, p_data->open.status, p_data->open.mtu);
		XR_LOGI(GATTC_TAG, "REMOTE BDA:");
		xr_log_buffer_hex(GATTC_TAG, p_data->open.remote_bda, sizeof(xr_bd_addr_t));
		xr_err_t mtu_ret = xr_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
		if (mtu_ret) {
			XR_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
		}
		break;
	case XR_GATTC_CFG_MTU_EVT:
		if (param->cfg_mtu.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG,"Config mtu failed");
		}
		XR_LOGI(GATTC_TAG, "Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
		xr_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
		break;
	case XR_GATTC_SEARCH_RES_EVT: {
		XR_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
		XR_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
		if (p_data->search_res.srvc_id.uuid.len == XR_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID) {
			XR_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
			get_service_e = true;
			gl_profile_tab[PROFILE_E_APP_ID].service_start_handle = p_data->search_res.start_handle;
			gl_profile_tab[PROFILE_E_APP_ID].service_end_handle = p_data->search_res.end_handle;
		}
		break;
	}
	case XR_GATTC_SEARCH_CMPL_EVT:
		if (p_data->search_cmpl.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
			break;
		}
		if (get_service_e) {
			uint16_t count = 0;
			xr_gatt_status_t status = xr_ble_gattc_get_attr_count( gattc_if,
																	 p_data->search_cmpl.conn_id,
																	 XR_GATT_DB_CHARACTERISTIC,
																	 gl_profile_tab[PROFILE_E_APP_ID].service_start_handle,
																	 gl_profile_tab[PROFILE_E_APP_ID].service_end_handle,
																	 INVALID_HANDLE,
																	 &count);
			if (status != XR_GATT_OK) {
				XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_attr_count error");
			}

			if (count > 0) {
				char_elem_result_e = (xr_gattc_char_elem_t *)malloc(sizeof(xr_gattc_char_elem_t) * count);
				if (!char_elem_result_e) {
					XR_LOGE(GATTC_TAG, "gattc no mem");
				} else {
					status = xr_ble_gattc_get_char_by_uuid( gattc_if,
															 p_data->search_cmpl.conn_id,
															 gl_profile_tab[PROFILE_E_APP_ID].service_start_handle,
															 gl_profile_tab[PROFILE_E_APP_ID].service_end_handle,
															 remote_filter_char_uuid,
															 char_elem_result_e,
															 &count);
					if (status != XR_GATT_OK) {
						XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_char_by_uuid error");
					}

					/*  Every service have only one char in our 'XR_GATTS_DEMO' demo, so we used first 'char_elem_result' */
					if (count > 0 && (char_elem_result_e[0].properties & XR_GATT_CHAR_PROP_BIT_NOTIFY)) {
						gl_profile_tab[PROFILE_E_APP_ID].char_handle = char_elem_result_e[0].char_handle;
						xr_ble_gattc_register_for_notify (gattc_if, gl_profile_tab[PROFILE_E_APP_ID].remote_bda, char_elem_result_e[0].char_handle);
					}
				}
				/* free char_elem_result */
				free(char_elem_result_e);
			}else{
				XR_LOGE(GATTC_TAG, "no char found");
			}
		}
		break;
	case XR_GATTC_REG_FOR_NOTIFY_EVT: {

		if (p_data->reg_for_notify.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "reg notify failed, error status =%x", p_data->reg_for_notify.status);
			break;
		}
		uint16_t count = 0;
		uint16_t notify_en = 1;
		xr_gatt_status_t ret_status = xr_ble_gattc_get_attr_count( gattc_if,
																	 gl_profile_tab[PROFILE_E_APP_ID].conn_id,
																	 XR_GATT_DB_DESCRIPTOR,
																	 gl_profile_tab[PROFILE_E_APP_ID].service_start_handle,
																	 gl_profile_tab[PROFILE_E_APP_ID].service_end_handle,
																	 gl_profile_tab[PROFILE_E_APP_ID].char_handle,
																	 &count);
		if (ret_status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_attr_count error");
		}
		if (count > 0) {
			descr_elem_result_e = (xr_gattc_descr_elem_t *)malloc(sizeof(xr_gattc_descr_elem_t) * count);
			if (!descr_elem_result_e) {
				XR_LOGE(GATTC_TAG, "malloc error, gattc no mem");
			}else{
				ret_status = xr_ble_gattc_get_descr_by_char_handle( gattc_if,
																	 gl_profile_tab[PROFILE_E_APP_ID].conn_id,
																	 p_data->reg_for_notify.handle,
																	 notify_descr_uuid,
																	 descr_elem_result_e,
																	 &count);
				if (ret_status != XR_GATT_OK) {
					XR_LOGE(GATTC_TAG, "xr_ble_gattc_get_descr_by_char_handle error");
				}

				/* Every char has only one descriptor in our 'XR_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
				if (count > 0 && descr_elem_result_e[0].uuid.len == XR_UUID_LEN_16 && descr_elem_result_e[0].uuid.uuid.uuid16 == XR_GATT_UUID_CHAR_CLIENT_CONFIG) {
					ret_status = xr_ble_gattc_write_char_descr( gattc_if,
																 gl_profile_tab[PROFILE_E_APP_ID].conn_id,
																 descr_elem_result_e[0].handle,
																 sizeof(notify_en),
																 (uint8_t *)&notify_en,
																 XR_GATT_WRITE_TYPE_RSP,
																 XR_GATT_AUTH_REQ_NONE);
				}

				if (ret_status != XR_GATT_OK) {
					XR_LOGE(GATTC_TAG, "xr_ble_gattc_write_char_descr error");
				}

				/* free descr_elem_result */
				free(descr_elem_result_e);
			}
		}
		else{
			XR_LOGE(GATTC_TAG, "decsr not found");
		}
		break;
	}
	case XR_GATTC_NOTIFY_EVT:
		XR_LOGI(GATTC_TAG, "XR_GATTC_NOTIFY_EVT, Receive notify value:");
		xr_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
		break;
	case XR_GATTC_WRITE_DESCR_EVT:
		if (p_data->write.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
			break;
		}
		XR_LOGI(GATTC_TAG, "write descr success");
		uint8_t write_char_data[35];
		for (int i = 0; i < sizeof(write_char_data); ++i) {
			write_char_data[i] = i % 256;
		}
		xr_ble_gattc_write_char( gattc_if,
								  gl_profile_tab[PROFILE_E_APP_ID].conn_id,
								  gl_profile_tab[PROFILE_E_APP_ID].char_handle,
								  sizeof(write_char_data),
								  write_char_data,
								  XR_GATT_WRITE_TYPE_RSP,
								  XR_GATT_AUTH_REQ_NONE);
		break;
	case XR_GATTC_WRITE_CHAR_EVT:
		if (p_data->write.status != XR_GATT_OK) {
			XR_LOGE(GATTC_TAG, "Write char failed, error status = %x", p_data->write.status);
		} else {
			XR_LOGI(GATTC_TAG, "Write char success");
		}
		start_scan();
		break;
	case XR_GATTC_SRVC_CHG_EVT: {
		xr_bd_addr_t bda;
		memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(xr_bd_addr_t));
		XR_LOGI(GATTC_TAG, "XR_GATTC_SRVC_CHG_EVT, bd_addr:%08x%04x",(bda[0] << 24) + (bda[1] << 16) + (bda[2] << 8) + bda[3],
				 (bda[4] << 8) + bda[5]);
		break;
	}
	case XR_GATTC_DISCONNECT_EVT:
		if (memcmp(p_data->disconnect.remote_bda, gl_profile_tab[PROFILE_E_APP_ID].remote_bda, 6) == 0) {
			XR_LOGI(GATTC_TAG, "device e disconnect");
			conn_device_e = false;
			get_service_e = false;
			con_cnt--;
		}
		break;
	default:
		break;
	}
}

static void xr_gap_cb(xr_gap_ble_cb_event_t event, xr_ble_gap_cb_param_t *param)
{
	uint8_t *adv_name = NULL;
	uint8_t adv_name_len = 0;
	switch (event) {
	case XR_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
		 XR_LOGI(GATTC_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
				  param->update_conn_params.status,
				  param->update_conn_params.min_int,
				  param->update_conn_params.max_int,
				  param->update_conn_params.conn_int,
				  param->update_conn_params.latency,
				  param->update_conn_params.timeout);
		break;
	case XR_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
		//the unit of the duration is second
		uint32_t duration = 30;
		//xr_ble_gap_start_scanning(duration);
		break;
	}
	case XR_GAP_BLE_SCAN_START_COMPLETE_EVT:
		//scan start complete event to indicate scan start successfully or failed
		if (param->scan_start_cmpl.status == XR_BT_STATUS_SUCCESS) {
			XR_LOGI(GATTC_TAG, "Scan start success");
		} else {
			XR_LOGE(GATTC_TAG, "Scan start failed");
		}
		break;
	case XR_GAP_BLE_SCAN_RESULT_EVT: {
		xr_ble_gap_cb_param_t *scan_result = (xr_ble_gap_cb_param_t *)param;
		switch (scan_result->scan_rst.search_evt) {
		case XR_GAP_SEARCH_INQ_RES_EVT:
			xr_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
			XR_LOGI(GATTC_TAG, "Searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
			adv_name = xr_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
												XR_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
			XR_LOGI(GATTC_TAG, "Searched Device Name Len %d", adv_name_len);
			xr_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);
			XR_LOGI(GATTC_TAG, "\n");
			if (Isconnecting || scan_flag) {
				break;
			}
			if (conn_device_a && conn_device_b && conn_device_c && conn_device_d && conn_device_e && !stop_scan_done) {
				stop_scan_done = true;
				xr_ble_gap_stop_scanning();
				XR_LOGI(GATTC_TAG, "all devices are connected");
				break;
			}
			if (adv_name != NULL) {
				XR_LOGI(GATTC_TAG, "Searched device %s", remote_device_name[con_cnt]);

				if ((strlen(remote_device_name[0]) == adv_name_len && strncmp((char *)adv_name, remote_device_name[0], adv_name_len) == 0) && (conn_device_a == false)) {
					conn_device_a = true;
					XR_LOGI(GATTC_TAG, "ASearched device %s", remote_device_name[0]);
					xr_ble_gap_stop_scanning();
					xr_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
					Isconnecting = true;
					con_cnt++;
					break;
				}
				else if ((strlen(remote_device_name[1]) == adv_name_len && strncmp((char *)adv_name, remote_device_name[1], adv_name_len) == 0) && (conn_device_b == false)) {
					conn_device_b = true;
					XR_LOGI(GATTC_TAG, "BSearched device %s", remote_device_name[1]);
					xr_ble_gap_stop_scanning();
					xr_ble_gattc_open(gl_profile_tab[PROFILE_B_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
					Isconnecting = true;
					con_cnt++;
					break;
				}
				else if ((strlen(remote_device_name[2]) == adv_name_len && strncmp((char *)adv_name, remote_device_name[2], adv_name_len) == 0)  && (conn_device_c == false)) {
					conn_device_c = true;
					XR_LOGI(GATTC_TAG, "CSearched device %s", remote_device_name[2]);
					xr_ble_gap_stop_scanning();
					xr_ble_gattc_open(gl_profile_tab[PROFILE_C_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
					Isconnecting = true;
					con_cnt++;
					break;
				}
				else if ((strlen(remote_device_name[3]) == adv_name_len && strncmp((char *)adv_name, remote_device_name[3], adv_name_len) == 0)  && (conn_device_d == false)) {
					conn_device_d = true;
					XR_LOGI(GATTC_TAG, "DSearched device %s", remote_device_name[3]);
					xr_ble_gap_stop_scanning();
					xr_ble_gattc_open(gl_profile_tab[PROFILE_D_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
					Isconnecting = true;
					con_cnt++;
					break;
				}
				else if ((strlen(remote_device_name[4]) == adv_name_len && strncmp((char *)adv_name, remote_device_name[4], adv_name_len) == 0)  && (conn_device_e == false)) {
					conn_device_e = true;
					XR_LOGI(GATTC_TAG, "ESearched device %s", remote_device_name[4]);
					xr_ble_gap_stop_scanning();
					xr_ble_gattc_open(gl_profile_tab[PROFILE_E_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
					Isconnecting = true;
					con_cnt++;
					break;
				}

			}
			break;
		case XR_GAP_SEARCH_INQ_CMPL_EVT:
			break;
		default:
			break;
		}
		break;
	}

	case XR_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
		if (param->scan_stop_cmpl.status != XR_BT_STATUS_SUCCESS) {
			XR_LOGE(GATTC_TAG, "Scan stop failed");
			break;
		}
		XR_LOGI(GATTC_TAG, "Stop scan successfully");

		break;

	case XR_GAP_BLE_ADV_STOP_COMPLETE_EVT:
		if (param->adv_stop_cmpl.status != XR_BT_STATUS_SUCCESS) {
			XR_LOGE(GATTC_TAG, "Adv stop failed");
			break;
		}
		XR_LOGI(GATTC_TAG, "Stop adv successfully");
		break;

	default:
		break;
	}
}

static void xr_gattc_cb(xr_gattc_cb_event_t event, xr_gatt_if_t gattc_if, xr_ble_gattc_cb_param_t *param)
{
	XR_LOGI(GATTC_TAG, "EVT %d, gattc if %d, app_id %d", event, gattc_if, param->reg.app_id);

	/* If event is register event, store the gattc_if for each profile */
	if (event == XR_GATTC_REG_EVT) {
		if (param->reg.status == XR_GATT_OK) {
			gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
		} else {
			XR_LOGI(GATTC_TAG, "Reg app failed, app_id %04x, status %d",
					param->reg.app_id,
					param->reg.status);
			return;
		}
	}

	/* If the gattc_if equal to profile A, call profile A cb handler,
	 * so here call each profile's callback */
	do {
		int idx;
		for (idx = 0; idx < PROFILE_NUM; idx++) {
			if (gattc_if == XR_GATT_IF_NONE || /* XR_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
					gattc_if == gl_profile_tab[idx].gattc_if) {
				if (gl_profile_tab[idx].gattc_cb) {
					gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
				}
			}
		}
	} while (0);
}

enum cmd_status cmd_gattc_init_exec(char *cmd)
{
	xr_err_t ret;
	ret = xr_ble_gap_register_callback(xr_gap_cb);
	if (ret) {
		XR_LOGE(GATTC_TAG, "%s gap register failed, error code = %x\n", __func__, ret);
		return XR_FAIL;
	}

	ret = xr_ble_gattc_register_callback(xr_gattc_cb);
	if(ret) {
		XR_LOGE(GATTC_TAG, "%s gattc register failed, error code = %x\n", __func__, ret);
		return XR_FAIL;
	}

	ret = xr_ble_gattc_app_register(PROFILE_A_APP_ID);
	if (ret) {
		XR_LOGE(GATTC_TAG, "%s gattc appA register failed, error code = %x\n", __func__, ret);
		return XR_FAIL;
	}

	ret = xr_ble_gattc_app_register(PROFILE_B_APP_ID);
	if (ret) {
		XR_LOGE(GATTC_TAG, "%s gattc appB register failed, error code = %x\n", __func__, ret);
		return XR_FAIL;
	}

	ret = xr_ble_gattc_app_register(PROFILE_C_APP_ID);
	if (ret) {
		XR_LOGE(GATTC_TAG, "%s gattc appC register failed, error code = %x\n", __func__, ret);
		return XR_FAIL;
	}

	ret = xr_ble_gattc_app_register(PROFILE_D_APP_ID);
	if (ret) {
		XR_LOGE(GATTC_TAG, "%s gattc appC register failed, error code = %x\n", __func__, ret);
		return XR_FAIL;
	}

	ret = xr_ble_gattc_app_register(PROFILE_E_APP_ID);
	if (ret) {
		XR_LOGE(GATTC_TAG, "%s gattc appC register failed, error code = %x\n", __func__, ret);
		return XR_FAIL;
	}

	xr_err_t local_mtu_ret = xr_ble_gatt_set_local_mtu(200);
	if (local_mtu_ret) {
		XR_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
	}

}

enum cmd_status cmd_gattc_scan_exec(char *cmd)
{
	xr_err_t ret = XR_FAIL;
	int argc;
	char *argv[1];

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc != 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}
	if (strcmp(argv[0], "on") == 0) {
		ret = xr_ble_gap_set_scan_params(&ble_scan_params);
		if (ret){
		    XR_LOGE(GATTC_TAG, "set scan params error, error code = %x", ret);
		}
		xr_ble_gap_start_scanning(30);
		scan_flag = true;
	}
	else if (strcmp(argv[0], "off") == 0) {
		xr_ble_gap_stop_scanning();
		scan_flag = false;
	}
	else{
		XR_LOGE(GATTC_TAG, "wrong params,please enter on or off");
	}
}

enum cmd_status cmd_gattc_con_exec(char *cmd)
{
	xr_err_t ret = XR_FAIL;
	int argc;
	char *argv[1];

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc != 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	if (con_cnt >= PROFILE_NUM){
		XR_LOGE(GATTC_TAG, "Gatt client connection full\n", __func__);
		return CMD_STATUS_INVALID_ARG;
	}

	strcpy(remote_device_name[con_cnt], argv[0]);
	start_scan();
}

/*
    $gattc init
    $gattc connect name
    $gattc scan on/off
*/
static const struct cmd_data g_gattc_cmds[] = {
	{ "init",				    cmd_gattc_init_exec},
	{ "connect",                cmd_gattc_con_exec },
	{ "scan",                   cmd_gattc_scan_exec},
};

enum cmd_status cmd_gattc_exec(char *cmd)
{
	return cmd_exec(cmd, g_gattc_cmds, cmd_nitems(g_gattc_cmds));
}

