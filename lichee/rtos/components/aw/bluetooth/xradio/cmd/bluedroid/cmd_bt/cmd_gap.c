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
#include "bt_target.h"
#include "xr_gap_bt_api.h"
#include "xr_bt_device.h"

#include "bt_utils.h"
#include "xr_bt_defs.h"
#include "ctype.h"

#define MAX_SCAN_RESULT_SIZE 5      /* equal to BTM_INQ_DB_SIZE, which is defined in bt_target.h */

#define AUTH_IDLE 0
#define AUTH_PIN_CODE 1
#define AUTH_ENTER_PASSKEY 2
#define AUTH_PASSKEY_CONFIRM 3
#define AUTH_PAIRING_CONFIRM 4

#define XR_BT_SCAN_INQ 0
#define XR_BT_SCAN_PAGE 1

#define DEFAULT_DISC_WINDOW 0x12
#define DEFAULT_DISC_INTERVAL 0x0800
#define DEFAULT_CONN_WINDOW 0x12
#define DEFAULT_CONN_INTERVAL 0x0800

typedef struct {
	char bd_addr_str[18];
	char dev_name[40];
} device_t;

typedef struct {
	xr_bd_addr_t bd_addr;
	uint32_t passkey;
	bool digit;
	uint8_t flag;//0:none 1:pin code 2:enter passkey 3:passkey_confirm 4: pairing confirm
} gap_req_t;

static gap_req_t gap_req = { {0,0,0,0,0,0}, 0, 0, 0 };

static uint8_t sniff_control;
static const uint8_t *power_mode[] = {
	"Active",
	"Hold",
	"Sniff",
	"Park",
};

#define MODE2STATE(mode) (mode - sizeof(power_mode))
static const uint8_t *power_state[] = {
	"SSR",
	"PENDING",
	"ERROR"
};

typedef struct {
	device_t scan_result[MAX_SCAN_RESULT_SIZE];
	int next_device_pos;
} scan_result_tb_t;

/* scan result table */
static scan_result_tb_t *scan_result_tb_ptr = NULL;

/* discovery state */
static int disc_state = XR_BT_GAP_DISCOVERY_STOPPED;

static xr_bt_connection_mode_t conn_mode = XR_BT_CONNECTABLE;
static xr_bt_discovery_mode_t dis_mode = XR_BT_GENERAL_DISCOVERABLE;

static const char *conn_mode_str[] = {
    "NON_CONNECTABLE",
    "CONNECTABLE",
};

static const char *dis_mode_str[] = {
    "NON_DISCOVERABLE",
    "LIMITED_DISCOVERABLE",
    "GENERAL_DISCOVERABLE",
};

static uint8_t scan_result_print = 1;

static enum cmd_status cmd_gap_help_exec(char *cmd);

static void cmd_bt_gap_cb(xr_bt_gap_cb_event_t event, xr_bt_gap_cb_param_t *param)
{
	switch (event) {
#if CONFIG_BT_SSP_ENABLE || CONFIG_BT_SSP_ENABLED
	case XR_BT_GAP_DISC_STATE_CHANGED_EVT:{
		if (param->disc_st_chg.state == XR_BT_GAP_DISCOVERY_STARTED) {
			disc_state = XR_BT_GAP_DISCOVERY_STARTED;
			CMD_DBG("Discovery started ...\n");
		} else if (param->disc_st_chg.state == XR_BT_GAP_DISCOVERY_STOPPED) {
			if (disc_state == XR_BT_GAP_DISCOVERY_STARTED) {
				disc_state = XR_BT_GAP_DISCOVERY_STOPPED;
				CMD_DBG("Discovery stopped !!!\n");
			}
		}
		break;
	}
	case XR_BT_GAP_DISC_RES_EVT:{
		char bd_addr_str[18] = {0};
		char *dev_name = NULL;
		int device_pos = 0;

		/* get bd_addr */
		bda2str(param->disc_res.bda, bd_addr_str);

		/* get device_name */
		if (param->disc_res.prop ) {
			xr_bt_gap_dev_prop_t *prop = param->disc_res.prop;
			if (prop->type == XR_BT_GAP_DEV_PROP_BDNAME) {
				dev_name = (char *)prop->val;
			} else {
				dev_name = "NULL";
			}
		}
		if (scan_result_print)
			CMD_DBG("name:[%s]\t\taddress:[%s]\n", dev_name, bd_addr_str);

		for (int i = 0; i < MAX_SCAN_RESULT_SIZE; i++) {
			device_t *item = &(scan_result_tb_ptr->scan_result[i]);
			if (!cmd_strcmp(item->bd_addr_str, bd_addr_str)) {  /* device already exists, update dev_name if necessary */
				if(cmd_strcmp(dev_name, "NULL")) {
					cmd_memcpy(item->dev_name, dev_name, cmd_strlen(dev_name)+1 > 40 ? 40 : cmd_strlen(dev_name)+1);
					item->dev_name[39] = '\0';
				}
				return;
			}
		}

		/* add new device info */
		device_pos = scan_result_tb_ptr->next_device_pos;
		cmd_memcpy(scan_result_tb_ptr->scan_result[device_pos].bd_addr_str, bd_addr_str, sizeof(bd_addr_str));
		cmd_memcpy(scan_result_tb_ptr->scan_result[device_pos].dev_name, dev_name, cmd_strlen(dev_name)+1 > 40 ? 40 : cmd_strlen(dev_name)+1);
		scan_result_tb_ptr->scan_result[device_pos].dev_name[39] = '\0';

		scan_result_tb_ptr->next_device_pos++;
		if (scan_result_tb_ptr->next_device_pos == MAX_SCAN_RESULT_SIZE) {
			scan_result_tb_ptr->next_device_pos = 0;
		}

		break;
	}
	case XR_BT_GAP_RMT_SRVCS_EVT:{
		int uuid_num = param->rmt_srvcs.num_uuids;
		xr_bt_uuid_t *uuid_list;

		if (uuid_num == 0) {
			CMD_DBG("no uuid found for device[%02x:%02x:%02x:%02x:%02x:%02x]\n ",
			        param->rmt_srvcs.bda[0], param->rmt_srvcs.bda[1], param->rmt_srvcs.bda[2],
			        param->rmt_srvcs.bda[3], param->rmt_srvcs.bda[4], param->rmt_srvcs.bda[5]);
		} else {
			uuid_list = param->rmt_srvcs.uuid_list;

			CMD_DBG("uuid list for device[%02x:%02x:%02x:%02x:%02x:%02x]\n",
			        param->rmt_srvcs.bda[0], param->rmt_srvcs.bda[1], param->rmt_srvcs.bda[2],
			        param->rmt_srvcs.bda[3], param->rmt_srvcs.bda[4], param->rmt_srvcs.bda[5]);

			for (int i = 0; i < uuid_num; i++, uuid_list++) {
				switch (uuid_list->len) {
				case 2:
					CMD_DBG("uuid16: 0x%04x\n", uuid_list->uuid.uuid16);
					break;
				case 4:
					CMD_DBG("uuid32: 0x%08x\n", uuid_list->uuid.uuid32);
					break;
				case 16:
					/* TO-DO: translate uuid128 and print it*/
					break;
				}
			}
		}
		break;
	}
	case XR_BT_GAP_AUTH_CMPL_EVT:{
		if (param->auth_cmpl.stat == XR_BT_STATUS_SUCCESS) {
			CMD_DBG("authentication success: \n");
			if (strlen((const char *)param->auth_cmpl.device_name) != 0) {
				CMD_DBG("device_name[%s] \n", param->auth_cmpl.device_name);
			}

			CMD_DBG("bd_addr[%02x:%02x:%02x:%02x:%02x:%02x]\n",
			        param->auth_cmpl.bda[0], param->auth_cmpl.bda[1], param->auth_cmpl.bda[2],
			        param->auth_cmpl.bda[3], param->auth_cmpl.bda[4], param->auth_cmpl.bda[5]);
		} else {
			CMD_DBG("authentication failed, status:%d\n", param->auth_cmpl.stat);
		}
		break;
	}
	case XR_BT_GAP_CFM_REQ_EVT:
		CMD_DBG("XR_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d\n", param->cfm_req.num_val);
		memcpy(gap_req.bd_addr, param->key_req.bda, sizeof(xr_bd_addr_t));
		gap_req.flag = AUTH_PAIRING_CONFIRM;
		break;
	case XR_BT_GAP_KEY_NOTIF_EVT:
		CMD_DBG("XR_BT_GAP_KEY_NOTIF_EVT passkey:%u\n", param->key_notif.passkey);
		memcpy(gap_req.bd_addr, param->key_req.bda, sizeof(xr_bd_addr_t));
		gap_req.passkey = param->key_notif.passkey;
		gap_req.flag = AUTH_PASSKEY_CONFIRM;
		break;
	case XR_BT_GAP_KEY_REQ_EVT:
		CMD_DBG("XR_BT_GAP_KEY_REQ_EVT Please enter passkey!\n");
		memcpy(gap_req.bd_addr, param->key_req.bda, sizeof(xr_bd_addr_t));
		gap_req.flag = AUTH_ENTER_PASSKEY;
		break;
#endif ///CONFIG_BT_SSP_ENABLE || CONFIG_BT_SSP_ENABLED
	case XR_BT_GAP_READ_LOCAL_NAME_EVT:
		CMD_DBG("XR_BT_GAP_READ_LOCAL_NAME_EVT!\n");
		CMD_DBG("local device name is %s\n", param->read_local_name.name);
		break;
	case XR_BT_GAP_READ_REMOTE_NAME_EVT:
		CMD_DBG("XR_BT_GAP_READ_REMOTE_NAME_EVT!\n");
		CMD_DBG("remote device name is %s\n", param->read_rmt_name.rmt_name);
		break;
	case XR_BT_GAP_PIN_REQ_EVT:
		CMD_DBG("XR_BT_GAP_PIN_REQ_EVT param->pin_req.min_16_digit : %d\n", param->pin_req.min_16_digit);
		memcpy(gap_req.bd_addr, param->pin_req.bda, sizeof(xr_bd_addr_t));
		gap_req.digit = param->pin_req.min_16_digit;
		gap_req.flag = AUTH_PIN_CODE;
		break;
	case XR_BT_GAP_POWER_MODE_EVT:
		if (param->power_mode.mode < sizeof(power_mode))
			CMD_DBG("Current Power Mode: %s\n", power_mode[param->power_mode.mode]);
		else {
			uint8_t state = MODE2STATE(param->power_mode.mode);
			if (state < sizeof(power_state))
				CMD_DBG("Current Power Mode is Unknow with state %s\n", power_state[state]);
			else
				CMD_DBG("Current Power Mode is Unknow with mode id %d\n", param->power_mode.mode);
		}
		break;
	case XR_BT_GAP_GET_ROLE_COMPLETE_EVT: {
		char bd_addr_str[18] = {0};
		if (param->get_role_cmpl.status == XR_BT_STATUS_SUCCESS) {
			/* get bd_addr */
			bda2str(param->get_role_cmpl.bda, bd_addr_str);
			CMD_DBG("get role success, current role is %s with remote device [%s]\n",
			        ((param->get_role_cmpl.role == 0) ? "master" : "slave"), bd_addr_str);
		} else {
			CMD_DBG("get role fail, status: %d\n", param->get_role_cmpl.status);
		}
		break;
	}
	case XR_BT_GAP_SET_ROLE_COMPLETE_EVT: {
		char bd_addr_str[18] = {0};
		if (param->set_role_cmpl.status == XR_BT_STATUS_SUCCESS) {
			/* get bd_addr */
			bda2str(param->set_role_cmpl.bda, bd_addr_str);
			CMD_DBG("role switch success, current role is %s with remote device [%s]\n",
			        ((param->set_role_cmpl.new_role == 0) ? "master" : "slave"), bd_addr_str);
		} else {
			CMD_DBG("role switch fail, status: %d\n", param->set_role_cmpl.status);
		}
		break;
	}
	case XR_BT_GAP_REMOVE_BOND_DEV_COMPLETE_EVT:
		CMD_DBG("remove device[%02x:%02x:%02x:%02x:%02x:%02x] %s\n",
		         param->remove_bond_dev_cmpl.bda[0], param->remove_bond_dev_cmpl.bda[1], param->remove_bond_dev_cmpl.bda[2],
		         param->remove_bond_dev_cmpl.bda[3], param->remove_bond_dev_cmpl.bda[4], param->remove_bond_dev_cmpl.bda[5],
		         (param->remove_bond_dev_cmpl.status == XR_BT_STATUS_SUCCESS) ? "success" : "fail");
		break;
	case XR_BT_GAP_SET_AFH_CHANNELS_EVT:
		if (param->set_afh_channels.stat == XR_BT_STATUS_SUCCESS)
			CMD_DBG("set AFH complete\n");
		break;
	default:
		CMD_DBG("[gap_bt] event: %d\n", event);
		break;
	}
	return;
}

//todo
/* $gap set_scan_mode <connect_mode> <discoverable_mode> [pair_mode] [conn_paired_only_mode]*/
enum cmd_status cmd_gap_set_scan_mode_exec(char *cmd)
{
	int argc;
	char *argv[4];
	xr_err_t ret;

	int con_mode = -1;
	int dis_mode = -1;
	int pair_mode = XR_BT_PAIRABLE;
	int pair_only_mode = XR_BT_ALL_PAIRABLE;

	argc = cmd_parse_argv(cmd, argv, cmd_nitems(argv));
	if (argc < 2 || argc > 4) {
		CMD_DBG("bt gap set-scan-mode connect_mode discoverable_mode pair_mode conn_paired_only_mode\n");
		CMD_SYSLOG("connect_mode: connectable, non_connectable\n");
		CMD_SYSLOG("discoverable_mode: general_dis, limited_dis, non_dis\n");
		CMD_SYSLOG("pair_mode: pair, non_pair\n");
		CMD_SYSLOG("conn_paired_only_mode: pair_all, pair_connect_only\n");
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	for (int i = 0; i < argc; i++) {
		const char *arg = argv[i];
		if (!strcmp(arg, "connectable")) {
			con_mode = XR_BT_CONNECTABLE;
		} else if (!strcmp(arg, "non_connectable")) {
			con_mode = XR_BT_NON_CONNECTABLE;
		} else if (!strcmp(arg, "general_dis")) {
			dis_mode = XR_BT_GENERAL_DISCOVERABLE;
		} else if (!strcmp(arg, "limited_dis")) {
			dis_mode = XR_BT_LIMITED_DISCOVERABLE;
		} else if (!strcmp(arg, "non_dis")) {
			dis_mode = XR_BT_LIMITED_DISCOVERABLE;
		} else if (!strcmp(arg, "pair")) {
			pair_mode = XR_BT_PAIRABLE;
		} else if (!strcmp(arg, "non_pair")) {
			pair_mode = XR_BT_NON_PAIRABLE;
		} else if (!strcmp(arg, "pair_all")) {
			pair_only_mode = XR_BT_ALL_PAIRABLE;
		} else if (!strcmp(arg, "pair_connect_only")) {
			pair_only_mode = XR_BT_ONLY_CON_PAIRABLE;
		} else {
			CMD_ERR("invalid param %s\n", argv[i]);
		}
	}

	if ((ret = xr_bt_gap_set_scan_mode(con_mode, dis_mode, pair_mode, pair_only_mode)) != XR_OK )
	{
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $gap scan <on, off> [scan_time=0x<hex>] [scan_num=0x<hex>] [general, limited] [print, no-print] */
enum cmd_status cmd_gap_scan_exec(char *cmd)
{
	int argc;
	char *argv[8];
	int scan_num = 0;
	int scan_time = 0x30;   //0x30*1.28s = 61.44s
	xr_err_t ret;
	int scan_en = -1;
	int scan_mode = XR_BT_INQ_MODE_GENERAL_INQUIRY;

	argc = cmd_parse_argv(cmd, argv, cmd_nitems(argv));
	if (argc < 1 || argc > 5) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	for (int i = 0; i < argc; i++) {
		const char *arg = argv[i];

		if (!strcmp(arg, "on")) {
			scan_en = 1;
		} else if (!strcmp(arg, "off")) {
			scan_en = 0;
			break;
		} else if (!strcmp(arg, "general")) {
			scan_mode = XR_BT_INQ_MODE_GENERAL_INQUIRY;
		} else if (!strcmp(arg, "limited")) {
			scan_mode = XR_BT_INQ_MODE_LIMITED_INQUIRY;
		} else if (!strcmp(arg, "print")) {
			scan_result_print = 1;
		} else if (!strcmp(arg, "no-print")) {
			scan_result_print = 0;
		} else if (!strncmp(arg, "scan_time=", 10)) {
			if (cmd_sscanf(arg + 10, "%x", &scan_time) != 1) {
				CMD_ERR("invalid param %s\n", arg + 10);
				return CMD_STATUS_INVALID_ARG;
			}
			if (scan_time < 0 || scan_time > 0x30) {
				CMD_ERR("invalid param %d\n", scan_time);
				return CMD_STATUS_INVALID_ARG;
			}
		} else if (!strncmp(arg, "scan_num=", 9)) {
			if (cmd_sscanf(arg + 9, "%x", &scan_num) != 1) {
				CMD_ERR("invalid param %d\n", scan_num);
				return CMD_STATUS_INVALID_ARG;
			}
			//the scan response number should be 0 ~ 255, if the value is 0,
			// unlimited number of response, others 1~0xFF, maxium number of
			// response from the inquiry before the inquiry is halted.
			if (scan_num < 0 || scan_num > 0xFF) {
				CMD_ERR("invalid param, please set scan_num between 0 to 255 !\n");
				return CMD_STATUS_INVALID_ARG;
			}
		}
	}

	if (scan_en < 0) {
		CMD_ERR("invalid param, please set scan on or off!\n");
		return CMD_STATUS_FAIL;
	} else if (!scan_en) {
		if ((ret = xr_bt_gap_cancel_discovery()) != XR_OK) {
			CMD_ERR("cancel discovery failed: %d\n", ret);
			return CMD_STATUS_FAIL;
		}
		return CMD_STATUS_OK;
	} else if (disc_state != XR_BT_GAP_DISCOVERY_STOPPED) {
		CMD_WRN("discovery started already\n");
		return CMD_STATUS_OK;
	}

	/* clear old scan result */
	cmd_memset(scan_result_tb_ptr, 0, sizeof(scan_result_tb_t));

	/* note: the max device num rsp is 5, which is defined in bt_target.h */
	if ((ret = xr_bt_gap_start_discovery(scan_mode, scan_time, scan_num)) != XR_OK) {
		CMD_ERR("start_discovery return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/* $gap scan_result */
enum cmd_status cmd_gap_scan_result_exec(char *cmd)
{
	scan_result_tb_t zero;
	cmd_memset(&zero, 0, sizeof(scan_result_tb_t));
	if (!scan_result_tb_ptr || !cmd_memcmp(scan_result_tb_ptr, &zero, sizeof(scan_result_tb_t))) {
		CMD_ERR("No scan result, please execute 'gap scan cmd' first!!!\n");
		return CMD_STATUS_FAIL;
	}

	int bd_addr_empty[18] = {0};
	for (int i = 0; i < MAX_SCAN_RESULT_SIZE; i++) {
		device_t *item = &(scan_result_tb_ptr->scan_result[i]);
		if (cmd_memcmp(bd_addr_empty, item->bd_addr_str, 18)) {
			CMD_DBG("device_%02d:  name[%s],   bd_addr[%s]\n", i+1, item->dev_name, item->bd_addr_str);
		}
	}

	return CMD_STATUS_OK;
}

/* $gap iscan <mode: on/limited/off> [win=0x<hex>] [int=0x<hex>] */
enum cmd_status cmd_gap_iscan_exec(char *cmd)
{
	int argc;
	char *argv[3];
	xr_err_t ret;
	uint32_t window = DEFAULT_DISC_WINDOW, interval = DEFAULT_DISC_INTERVAL;

	argc = cmd_parse_argv(cmd, argv, 3);
	if (argc < 1 || argc > 3) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	for (int i = 0; i < argc; i++) {
		const char *arg = argv[i];

		if (!strcmp(arg, "on")) {
			dis_mode = XR_BT_GENERAL_DISCOVERABLE;
		} else if (!strcmp(arg, "limited")) {
			dis_mode = XR_BT_LIMITED_DISCOVERABLE;
		} else if (!strcmp(arg, "off")) {
			dis_mode = XR_BT_NON_DISCOVERABLE;
		} else if (!cmd_strncmp(arg, "win=0x", 4)) {
			if (cmd_sscanf(arg + 4, "%x", &window) != 1) {
				CMD_ERR("invalid param %s\n", arg + 4);
				return CMD_STATUS_INVALID_ARG;
			}
		} else if (!cmd_strncmp(arg, "int=0x", 4)) {
			if (cmd_sscanf(arg + 4, "%x", &interval) != 1) {
				CMD_ERR("invalid param %s\n", arg + 4);
				return CMD_STATUS_INVALID_ARG;
			}
		} else {
			CMD_ERR("invalid cmd %s\n", arg);
			return CMD_STATUS_UNKNOWN_CMD;
		}
	}

	if (dis_mode != XR_BT_NON_DISCOVERABLE) {
		if (window > interval) {
			CMD_ERR("windows:%x should be less than or equal to interval:%x\n", window, interval);
			return CMD_STATUS_INVALID_ARG;
		}
		xr_bt_gap_set_ipscan_param(window, interval, XR_BT_SCAN_INQ);
	}

	CMD_DBG("pscan state %s, iscan state %s\n", conn_mode_str[conn_mode], dis_mode_str[dis_mode]);
	if (( ret = xr_bt_gap_set_scan_mode(conn_mode, dis_mode, XR_BT_PAIRABLE, XR_BT_ALL_PAIRABLE) ) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $gap pscan <mode: on/off> [win=0x<hex>] [int=0x<hex>] */
enum cmd_status cmd_gap_pscan_exec(char *cmd)
{
	int argc;
	char *argv[3];
	xr_err_t ret;
	uint32_t window = DEFAULT_CONN_WINDOW, interval = DEFAULT_CONN_INTERVAL;
	bool mode_flag = false;

	argc = cmd_parse_argv(cmd, argv, 3);
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	for (int i = 0; i < argc; i++) {
		const char *arg = argv[i];

		if (!strcmp(arg, "on")) {
			conn_mode = XR_BT_CONNECTABLE;
			mode_flag = true;
		} else if (!strcmp(arg, "off")) {
			conn_mode = XR_BT_NON_CONNECTABLE;
			mode_flag = true;
		} else if (!cmd_strncmp(arg, "win=0x", 4)) {
			if (cmd_sscanf(arg + 4, "%x", &window) != 1) {
				CMD_ERR("invalid param %s\n", arg + 4);
				return CMD_STATUS_INVALID_ARG;
			}
		} else if (!cmd_strncmp(arg, "int=0x", 4)) {
			if (cmd_sscanf(arg + 4, "%x", &interval) != 1) {
				CMD_ERR("invalid param %s\n", arg + 4);
				return CMD_STATUS_INVALID_ARG;
			}
		} else {
			CMD_ERR("invalid cmd %s\n", arg);
			return CMD_STATUS_UNKNOWN_CMD;
		}
	}

	if (!mode_flag) {
		CMD_ERR("pscan invalid param\n");
		return CMD_STATUS_INVALID_ARG;
	}

	if (conn_mode == XR_BT_CONNECTABLE) {
		if (window > interval) {
			CMD_ERR("windows:%x should be less than or equal to interval:%x\n", window, interval);
			return CMD_STATUS_INVALID_ARG;
		}
		xr_bt_gap_set_ipscan_param(window, interval, XR_BT_SCAN_PAGE);
	}

	CMD_DBG("pscan state %s, iscan state %s\n", conn_mode_str[conn_mode], dis_mode_str[dis_mode]);
	if (( ret = xr_bt_gap_set_scan_mode(conn_mode, dis_mode, XR_BT_PAIRABLE, XR_BT_ALL_PAIRABLE) ) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $gap clear <bd_addr> */
enum cmd_status cmd_gap_clear(char *cmd)
{
	int argc;
	char *argv[1];
	int device_num = -1;

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	if (!strcmp(argv[0], "all")) {
		device_num = xr_bt_gap_get_bond_device_num();
		if (device_num <= 0) {
			CMD_DBG("no device is bonded!\n");
			return CMD_STATUS_OK;
		}

		xr_err_t ret;
		xr_bd_addr_t *dev_list_addr;
		xr_bd_name_t *dev_list_name;
		char bda_str[18];

		dev_list_addr = cmd_malloc(device_num * sizeof(xr_bd_addr_t));
		if (dev_list_addr == NULL) {
			CMD_ERR("malloc fail\n");
			return CMD_STATUS_FAIL;
		}

		dev_list_name = cmd_malloc(device_num * sizeof(xr_bd_name_t));
		if (dev_list_name == NULL) {
			CMD_ERR("malloc fail\n");
			cmd_free(dev_list_addr);
			return CMD_STATUS_FAIL;
		}

		ret = xr_bt_gap_get_bond_device_list(&device_num, dev_list_addr, dev_list_name);
		if (ret != XR_OK) {
			CMD_ERR("return failed: %d\n", ret);
			cmd_free(dev_list_name);
			cmd_free(dev_list_addr);
			return CMD_STATUS_FAIL;
		}
		CMD_DBG("begin to clear %d devices\n", device_num);

		for (int i = 0; i < device_num; i++) {
			ret = xr_bt_gap_remove_bond_device(dev_list_addr[i]);
			if (ret != XR_OK) {
				CMD_ERR("return failed: %d\n", ret);
				cmd_free(dev_list_name);
				cmd_free(dev_list_addr);
				return CMD_STATUS_FAIL;
			}
		}
		cmd_free(dev_list_name);
		cmd_free(dev_list_addr);

		return CMD_STATUS_OK;
	} else {
		xr_err_t ret;
		xr_bd_addr_t remote_bda = { 0 };
		char bda_str[18];

		str2bda(argv[0], remote_bda);
		bda2str(remote_bda, bda_str);
		CMD_DBG("remove_bond_device: [%s]\n", bda_str);

		ret = xr_bt_gap_remove_bond_device(remote_bda);
		if (ret != XR_OK) {
			CMD_ERR("return failed: %d\n", ret);
			return CMD_STATUS_FAIL;
		}
		return CMD_STATUS_OK;
	}
}

/* $gap bonds */
enum cmd_status cmd_gap_get_bond_device_list(char *cmd)
{
	int device_num = -1;
	xr_err_t ret;
	xr_bd_addr_t *dev_list_addr;
	xr_bd_name_t *dev_list_name;

	char bda_str[18];

	device_num = xr_bt_gap_get_bond_device_num();
	if (device_num <= 0) {
		CMD_DBG("no device is bonded!\n");
		return CMD_STATUS_OK;
	}

	dev_list_addr = cmd_malloc(device_num * sizeof(xr_bd_addr_t));
	if (dev_list_addr == NULL) {
		CMD_ERR("malloc fail\n");
		return CMD_STATUS_FAIL;
	}

	dev_list_name = cmd_malloc(device_num * sizeof(xr_bd_name_t));
	if (dev_list_name == NULL) {
		CMD_ERR("malloc fail\n");
		cmd_free(dev_list_addr);
		return CMD_STATUS_FAIL;
	}

	ret = xr_bt_gap_get_bond_device_list(&device_num, dev_list_addr, dev_list_name);
	if (ret != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		cmd_free(dev_list_name);
		cmd_free(dev_list_addr);
		return CMD_STATUS_FAIL;
	}

	CMD_DBG("get_bond_device_list:\n");
	for (int i = 0; i < device_num; i++) {
		bda2str(dev_list_addr[i], bda_str);
		CMD_DBG("device_%d: [%s]\n", i, bda_str);
	}
	CMD_DBG("Total device num is %d\n", device_num);

	cmd_free(dev_list_name);
	cmd_free(dev_list_addr);

	return CMD_STATUS_OK;
}

/* $gap get_remote_services <bd_addr> */
enum cmd_status cmd_gap_get_remote_services(char *cmd)
{
	int argc;
	char *argv[1];

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	xr_err_t ret;
	xr_bd_addr_t remote_bda = { 0 };
	char bda_str[18];

	str2bda(argv[0], remote_bda);
	bda2str(remote_bda, bda_str);
	CMD_DBG("get_remote_services for [%s]\n", bda_str);

	if ((ret = xr_bt_gap_get_remote_services(remote_bda)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $gap name [name] */
enum cmd_status cmd_gap_name(char *cmd)
{
	int argc;
	char *argv[1];
	xr_err_t ret;

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc < 1) {
		ret = xr_bt_dev_get_device_name();
		return CMD_STATUS_OK;
	}

	xr_bt_dev_set_device_name(cmd);
	return CMD_STATUS_OK;
}

/* $gap mac */
enum cmd_status cmd_gap_mac(char *cmd)
{
	xr_err_t ret;
	uint8_t mac[6] = {0};

	const uint8_t *addr = xr_bt_dev_get_address();
	if (addr != NULL) {
		memcpy(mac, addr, 6);
	} else {
		CMD_ERR("bt not ready!\n");
		return CMD_STATUS_FAIL;
	}

	bda2str_dump("mac is", mac);
	return CMD_STATUS_OK;
}

/* $gap init */
enum cmd_status cmd_gap_init(char *cmd)
{
	xr_err_t ret;

	char dev_name[32];
	// cmd_sprintf(dev_name, "RTOS_BT_TEST_"XR_BD_ADDR_STR, XR_BD_ADDR_HEX(sysinfo_get()->mac_addr));
	cmd_sprintf(dev_name, "RTOS_BT_TEST");//todo
	CMD_DBG("!!!!!!!! bt device name: %s !!!!!!!!\n", dev_name);
	xr_bt_dev_set_device_name(dev_name);

	if ((ret = xr_bt_gap_register_callback(cmd_bt_gap_cb)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	/* set discoverable and connectable mode, wait to be connected */
	if ((ret = xr_bt_gap_set_scan_mode(conn_mode, dis_mode, XR_BT_PAIRABLE, XR_BT_ALL_PAIRABLE)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	scan_result_tb_ptr = (scan_result_tb_t *)cmd_malloc(sizeof(scan_result_tb_t));
	if (scan_result_tb_ptr == NULL) {
		CMD_ERR("scan_result_tb malloc failed!!!\n");
		return CMD_STATUS_FAIL;
	}
	cmd_memset(scan_result_tb_ptr, 0, sizeof(scan_result_tb_t));
	// xr_bt_pin_type_t pin_type = XR_BT_PIN_TYPE_FIXED;
	// xr_bt_pin_code_t pin_code;
	// pin_code[0] = '1';
	// pin_code[1] = '2';
	// pin_code[2] = '3';
	// pin_code[3] = '4';
	// xr_bt_gap_set_pin(pin_type, 4, pin_code);

	return CMD_STATUS_OK;
}

/* $gap deinit */
enum cmd_status cmd_gap_deinit(char *cmd)
{
	if (scan_result_tb_ptr != NULL) {
		cmd_free(scan_result_tb_ptr);
		scan_result_tb_ptr = NULL;
	}
	return CMD_STATUS_OK;
}

/* $gap fixed_pin_code <pin_code(0000~9999)>*/
enum cmd_status cmd_gap_fixed_pin_code(char *cmd)
{

	xr_bt_pin_type_t pin_type = XR_BT_PIN_TYPE_FIXED;
	xr_bt_pin_code_t pin_code;

	int argc;
	char *argv[1];
	int i = 0;

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	for (i = 0; i < 4; i++) {
		if (argv[0][i] < '0' || argv[0][i] > '9') {
			CMD_ERR("invalid fixed pin code\n");
			return CMD_STATUS_INVALID_ARG;
		}
		pin_code[i] = argv[0][i] - '0';
	}

	xr_bt_gap_set_pin(pin_type, 4, pin_code);

	return CMD_STATUS_OK;
}

/* $gap pin_code <pin_code(0000~9999)>*/
enum cmd_status cmd_gap_pin_code(char *cmd)
{
	if (gap_req.flag != AUTH_PIN_CODE) {
		CMD_ERR("There is no need to enter pin code now\n");
		return CMD_STATUS_FAIL;
	}
	xr_bt_pin_type_t pin_type = XR_BT_PIN_TYPE_VARIABLE;
	xr_bt_pin_code_t pin_code;
	int i = 0;

	if (gap_req.digit) {
		CMD_LOG(1, "need to enter 16 digital pin code\n");
	} else {
		CMD_LOG(1, "need to enter 4 digital pin code\n");
	}
	int argc;
	char *argv[1];

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}
	//gap_req.digit is 1,pin is 16 digit. gap_req.digitis 0,pin is 4.
	for (i = 4 + 12 * gap_req.digit - 1; i >= 0; i--) {

		if (argv[0][i] < '0' || argv[0][i] > '9') {
			CMD_ERR("invalid pin code\n");
			return CMD_STATUS_INVALID_ARG;
		}

		pin_code[i] = argv[0][i];
	}
	xr_bt_gap_pin_reply(gap_req.bd_addr, true, 4 + 12 * gap_req.digit, pin_code);
	gap_req.flag = AUTH_IDLE;
	return CMD_STATUS_OK;
}

#if (BT_SSP_INCLUDED == TRUE)
/* $gap auth_passkey <passkey(000000~999999)>*/
enum cmd_status cmd_gap_auth_passkey(char *cmd)
{
	if (gap_req.flag != AUTH_ENTER_PASSKEY) {
		CMD_ERR("There is no need to enter passkey now\n");
		return CMD_STATUS_FAIL;
	}
	uint32_t passkey = 0;
	int i = 0;

	int argc;
	char *argv[1];

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}
	passkey = atoi(argv[0]);
	if (passkey > 999999) {
		CMD_ERR("Passkey should be between 0-999999");
		return CMD_STATUS_INVALID_ARG;
	}
	xr_bt_gap_ssp_passkey_reply(gap_req.bd_addr, true, passkey);
	gap_req.flag = AUTH_IDLE;
	return CMD_STATUS_OK;
}

/* $gap auth_passkey_confirm*/
enum cmd_status cmd_gap_auth_passkey_confirm(char *cmd)
{

	if (gap_req.flag != AUTH_PASSKEY_CONFIRM) {
		CMD_ERR("There is no need to enter this command now\n");
		return CMD_STATUS_FAIL;
	}

	xr_bt_gap_ssp_passkey_reply(gap_req.bd_addr, true, gap_req.passkey);
	gap_req.flag = AUTH_IDLE;
	return CMD_STATUS_OK;
}

/* $gap pairing_confirm*/
enum cmd_status cmd_gap_pairing_confirm(char *cmd)
{
	if (gap_req.flag != AUTH_PAIRING_CONFIRM) {
		CMD_ERR("There is no need to enter this command now\n");
		return CMD_STATUS_FAIL;
	}
	xr_bt_gap_ssp_confirm_reply(gap_req.bd_addr, true);

	gap_req.flag = AUTH_IDLE;
	return CMD_STATUS_OK;
}

/* $gap auth*/
enum cmd_status cmd_gap_auth(char *cmd)
{
	int argc;
	int ret;
	char *argv[1];
	xr_bt_io_cap_t iocap;
	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}
	if (!strcmp(argv[0], "input")) {
		iocap = XR_BT_IO_CAP_IN;
	} else if (!strcmp(argv[0], "display")) {
		iocap = XR_BT_IO_CAP_OUT;
	} else if (!strcmp(argv[0], "yesno")) {
		iocap = XR_BT_IO_CAP_IO;
	} else if (!strcmp(argv[0], "none")) {
		iocap = XR_BT_IO_CAP_NONE;
	} else {
		CMD_ERR("invalid param %s\n", argv[0]);
		return CMD_STATUS_INVALID_ARG;
	}

	xr_bt_sp_param_t param_type = XR_BT_SP_IOCAP_MODE;
	if (( ret = xr_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t))) != XR_OK) {
		CMD_ERR("xr_bt_gap_set_security_param failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}
#endif

/* $gap ssp <mode:on/off>*/
#if CONFIG_CHIP_328
void set_ssp_flag(bool flag);
#endif

enum cmd_status cmd_gap_ssp(char *cmd)
{
	int argc;
	char *argv[1];
	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

#if CONFIG_CHIP_328
	if (!strcmp(argv[0], "on")) {
		set_ssp_flag(false);
	} else if (!strcmp(argv[0], "off")) {
		set_ssp_flag(true);
	} else {
		CMD_ERR("invalid param %s\n", argv[0]);
		return CMD_STATUS_INVALID_ARG;
	}
#else
	CMD_ERR("Command is not support\n");
#endif

	return CMD_STATUS_OK;
}

/* $gap sec <mode: BTA_SEC> */
#if CONFIG_CHIP_328
void set_sec_flag(int flag);
#endif
enum cmd_status cmd_gap_sec(char *cmd)
{
	int argc;
	char *argv[1];
	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}
	CMD_DBG("0:BTA_SEC_NONE\n");
	CMD_DBG("1:BTA_SEC_AUTHORIZE\n");
	CMD_DBG("2:BTA_SEC_AUTHENTICATE\n");
	CMD_DBG("3:BTA_SEC_ENCRYPT\n");
	CMD_DBG("4:BTA_SEC_MODE4_LEVEL4\n");
	CMD_DBG("5:BTA_SEC_MITM\n");
	CMD_DBG("select %d\n", argv[0][0] - '0');
#if CONFIG_CHIP_328
	set_sec_flag(argv[0][0] - '0');
#else
	CMD_ERR("Command is not support\n");
#endif

	return CMD_STATUS_OK;
}

#if CONFIG_CHIP_328
void set_io_flag(int flag);
#endif
enum cmd_status cmd_gap_io(char *cmd)
{
	int argc;
	char *argv[1];
	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}
	CMD_DBG("0:NO MITM;NO BONDING;Numeric comparison\n");
	CMD_DBG("1:MITM;NO BONDING;Use io\n");
	CMD_DBG("2:NO MITM;Delicated BONDING;Numeric comparison\n");
	CMD_DBG("3:MITM;Delicated BONDING;Use io\n");
	CMD_DBG("4:NO MITM;General BONDING;Numeric comparison\n");
	CMD_DBG("5:NO MITM;General BONDING;Use io\n");
	CMD_DBG("select %d\n", argv[0][0] - '0');
#if CONFIG_CHIP_328
	set_io_flag(argv[0] - '0');
#else
	CMD_ERR("Command is not support\n");
#endif

	return CMD_STATUS_OK;
}

/* $gap sniff <on/off> <bd_addr> [min=0x<hex>] [max=0x<hex>] [attempt=0x<hex>] [to=0x<hex>]*/
enum cmd_status cmd_gap_sniff(char *cmd)
{
	int argc;
	char *argv[8];
	xr_err_t ret;
	xr_bt_sniff_mode_t sniff_mode = XR_BT_SNIFF_MODE_OFF;
	uint32_t max_period = 0x0320;   //0x0320*625us=500ms
	uint32_t min_period = 0x0190;   //0x0190*625us=250ms
	uint32_t attempt = 0x04;       //0x04*625us=2.5ms
	uint32_t timeout = 0x01;      //1*625us=0.625ms

	argc = cmd_parse_argv(cmd, argv, cmd_nitems(argv));
	if (argc < 2) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	for (size_t argn = 2; argn < argc; argn++) {
		const char *arg = argv[argn];

		if (!cmd_strncmp(arg, "min=0x", 6)) {
			uint32_t num;
			int cnt = cmd_sscanf(arg + 6, "%x", &num);
			if (cnt != 1) {
				CMD_ERR("invalid param %d\n", num);
				return CMD_STATUS_INVALID_ARG;
			}
			min_period = num;
		} else if (!cmd_strncmp(arg, "max=0x", 6)) {
			uint32_t num;
			int cnt = cmd_sscanf(arg + 6, "%x", &num);
			if (cnt != 1) {
				CMD_ERR("invalid param %d\n", num);
				return CMD_STATUS_INVALID_ARG;
			}
			max_period = num;
		} else if (!cmd_strncmp(arg, "attempt=0x", 10)) {
			uint32_t num;
			int cnt = cmd_sscanf(arg + 10, "%x", &num);
			if (cnt != 1) {
				CMD_ERR("invalid param %d\n", num);
				return CMD_STATUS_INVALID_ARG;
			}
			attempt = num;
		} else if (!cmd_strncmp(arg, "to=0x", 5)) {
			uint32_t num;
			int cnt = cmd_sscanf(arg + 5, "%x", &num);
			if (cnt != 1) {
				CMD_ERR("invalid param %d\n", num);
				return CMD_STATUS_INVALID_ARG;
			}
			timeout = num;
		}
	}

	if (!strcmp(argv[0], "on")) {
		sniff_mode = XR_BT_SNIFF_MODE_ON;
	} else if (!strcmp(argv[0], "off")) {
		sniff_mode = XR_BT_SNIFF_MODE_OFF;
	} else {
		return CMD_STATUS_FAIL;
	}

	if ((min_period < 0x2) || (min_period > 0xFFFE) || (max_period < 0x2) || (max_period > 0xFFFE)
		|| (min_period > max_period) || (attempt == 0x0) || (attempt > 0x7FFF)
		|| (timeout > 0x7FFF)) {
		CMD_ERR("invalid param\n");
		return CMD_STATUS_INVALID_ARG;
	}

	xr_bd_addr_t remote_bda = { 0 };
	char bda_str[18];

	str2bda(argv[1], remote_bda);
	bda2str(remote_bda, bda_str);
	CMD_DBG("set sniff for [%s]\n", bda_str);

	if ((ret = xr_bt_gap_set_sniff_mode(remote_bda, sniff_mode, min_period, max_period,
		attempt, timeout)) != XR_OK) {
		CMD_ERR("xr_bt_gap_set_sniff_mode failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/* $gap power_mode <bd_addr> */
enum cmd_status cmd_gap_power_mode(char *cmd)
{
	int argc;
	char *argv[8];
	xr_err_t ret;
	xr_bd_addr_t remote_bda = { 0 };
	char bda_str[18];

	argc = cmd_parse_argv(cmd, argv, cmd_nitems(argv));
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	str2bda(argv[0], remote_bda);
	bda2str(remote_bda, bda_str);
	CMD_DBG("get power mode for [%s]\n", bda_str);

	if ((ret = xr_bt_gap_power_mode(remote_bda)) != XR_OK) {
		CMD_ERR("xr_bt_gap_power_mode failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $gap role_switch <addr> <role>*/
enum cmd_status cmd_gap_role_switch(char *cmd)
{
	int argc;
	char *argv[8];
	xr_err_t ret;
	xr_bt_role_t role;

	argc = cmd_parse_argv(cmd, argv, cmd_nitems(argv));
	if (argc < 2) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	if (!strcmp(argv[1], "master")) {
		role = XR_BT_ROLE_MASTER;
	} else if (!strcmp(argv[1], "slave")) {
		role = XR_BT_ROLE_SLAVE;
	} else {
		return CMD_STATUS_FAIL;
	}

	xr_bd_addr_t remote_bda = { 0 };
	char bda_str[18];

	str2bda(argv[0], remote_bda);
	bda2str(remote_bda, bda_str);
	CMD_DBG("role switch for [%s]\n", bda_str);

	if (( ret = xr_bt_gap_set_role(remote_bda, role)) != XR_OK) {
		CMD_ERR("xr_bt_gap_set_role failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/* $gap get_role <addr>*/
enum cmd_status cmd_gap_get_role(char *cmd)
{
	int argc;
	char *argv[8];
	xr_err_t ret;
	xr_bd_addr_t remote_bda = { 0 };
	char bda_str[18];

	argc = cmd_parse_argv(cmd, argv, cmd_nitems(argv));
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	str2bda(argv[0], remote_bda);
	bda2str(remote_bda, bda_str);
	CMD_DBG("get role for [%s]\n", bda_str);

	if ((ret = xr_bt_gap_role(remote_bda)) != XR_OK) {
		CMD_ERR("xr_bt_gap_role failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

int hex_to_bin(char ch)
{
	if ((ch >= '0') && (ch <= '9'))
		return ch - '0';
	ch = tolower(ch);
	if ((ch >= 'a') && (ch <= 'f'))
		return ch - 'a' + 10;
	return -1;
}

int hex2bin(uint8_t *dst, const char *src, size_t count)
{
	while (count--) {
		int hi = hex_to_bin(*src++);
		int lo = hex_to_bin(*src++);

		if ((hi < 0) || (lo < 0))
			return -1;

		*dst++ = (hi << 4) | lo;
	}
	return 0;
}

static void sys_mem_swap(void *buf, size_t length)
{
	size_t i;

	for (i = 0; i < (length/2); i++) {
		uint8_t tmp = ((uint8_t *)buf)[i];

		((uint8_t *)buf)[i] = ((uint8_t *)buf)[length - 1 - i];
		((uint8_t *)buf)[length - 1 - i] = tmp;
	}
}

/* $gap afh <afh-map: XXXXXXXXXX> (36-0) */
enum cmd_status cmd_gap_set_afh(char *cmd)
{
	int argc;
	char *argv[8];
	xr_err_t ret;
	xr_bt_gap_afh_channels channels;

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	if (hex2bin(argv[0], channels, strlen(argv[0])) == 0) {
		CMD_ERR("invalid afh map\n");
		return CMD_STATUS_INVALID_ARG;
	}

	sys_mem_swap(channels, XR_BT_GAP_AFH_CHANNELS_LEN);

	if (( ret = xr_bt_gap_set_afh_channels(channels)) != XR_OK) {
		CMD_ERR("xr_bt_gap_set_afh_channels failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/* $gap auto_sniff <on/off> */
enum cmd_status cmd_gap_set_auto_sniff_mode(char *cmd)
{
	int argc;
	char *argv[1];
	xr_err_t ret;
	uint8_t mode;

	argc = cmd_parse_argv(cmd, (char **)&argv, 1);
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	if (!strcmp(argv[0], "on")) {
		mode = 1;
	} else if (!strcmp(argv[0], "off")) {
		mode = 0;
	} else {
		CMD_ERR("invalid param %s\n", argv[0]);
		return CMD_STATUS_FAIL;
	}

	if ((ret = xr_bt_gap_set_auto_sniff_mode(mode)) != XR_OK) {
		CMD_ERR("xr_bt_gap_set_auto_sniff_mode failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/*
    $gap set_scan_mode <connect_mode> <discoverable_mode> <pair_mode> <conn_paired_only_mode>
    $gap scan <on, off> [scan_time=0x<hex>] [scan_num=0x<hex>] [general, limited] [print, no-print]
    $gap scan_result
    $gap scan_cancel
    $gap iscan <mode: on/limited/off> [win=0x<hex>] [int=0x<hex>]
    $gap pscan <mode: on/off> [win=0x<hex>] [int=0x<hex>]
    $gap clear <bd_addr>
    $gap bonds
    $gap get_remote_services <bd_addr>
    $gap set_name <name>
    $gap name [name]
    $gap get_mac
    $gap init
    $gap deinit
    $gap fixed_pin_code <pin_code>
    $gap pin_code <pin_code>
    $gap auth_passkey <passkey>
    $gap auth_passkey_confirm
    $gap pairing_confirm
    $gap auth [mode: input/display/yesno/none]
    $gap ssp <mode:on/off>
    $gap sec
    $gap io
    $gap sniff <on/off> <bd_addr> [min=0x<hex>] [max=0x<hex>] [attempt=0x<hex>] [to=0x<hex>]
    $gap power_mode <bd_addr>
    $gap role_switch <addr> <role>
    $gap get_role <addr>
    $gap afh <afh-map: XXXXXXXXXX> (36-0)
    $gap auto_sniff <on/off>
*/
static const struct cmd_data g_gap_cmds[] = {
	{ "set_scan_mode",          cmd_gap_set_scan_mode_exec,   CMD_DESC("<connect_mode:connectable/non_connectable> <discoverable_mode:general_dis/limited_dis/non_dis> [pair_mode:pair/ssnon_pair] [conn_paired_only_mode:pair_all/ pair_connect_only]") },
	{ "scan",                   cmd_gap_scan_exec,            CMD_DESC("<ctrl:on/off>[scan_time=0x<hex>] [scan_num=0x<hex>] [mode:general/limited] [print-mode:print/no-print]")},
	{ "scan_result",            cmd_gap_scan_result_exec,     CMD_DESC("No parameters") },
	{ "iscan",                  cmd_gap_iscan_exec,           CMD_DESC("<mode: on/limited/off> [win=0x<hex>] [int=0x<hex>]") },
	{ "pscan",                  cmd_gap_pscan_exec,           CMD_DESC("<mode: on/off> [win=0x<hex>] [int=0x<hex>]") },
	{ "clear",                  cmd_gap_clear,                CMD_DESC("<bd_addr/all>") },
	{ "bonds",                  cmd_gap_get_bond_device_list, CMD_DESC("No parameters") },
	{ "get_remote_services",    cmd_gap_get_remote_services,  CMD_DESC("<bd_addr>") },
	{ "name",                   cmd_gap_name,                 CMD_DESC("<name>") },
	{ "mac",                    cmd_gap_mac,                  CMD_DESC("No parameters") },
	{ "init",                   cmd_gap_init,                 CMD_DESC("No parameters") },
	{ "deinit",                 cmd_gap_deinit,               CMD_DESC("No parameters") },
	{ "fixed_pin_code",         cmd_gap_fixed_pin_code,       CMD_DESC("<pin_code>") },
	{ "pin_code",               cmd_gap_pin_code,             CMD_DESC("<pin_code>") },
	{ "auth_passkey",           cmd_gap_auth_passkey,         CMD_DESC("<passkey>") },
	{ "auth_passkey_confirm",   cmd_gap_auth_passkey_confirm, CMD_DESC("No parameters") },
	{ "pairing_confirm",        cmd_gap_pairing_confirm,      CMD_DESC("No parameters") },
	{ "auth",                   cmd_gap_auth,                 CMD_DESC("<input/display/yesno/none>") },
	{ "ssp",                    cmd_gap_ssp,                  CMD_DESC("<on/off>")},
	{ "sec",                    cmd_gap_sec,                  CMD_DESC("<mode>(0:NONE, 1:AUTHORIZE, 2:AUTHENTICATE, 3:ENCRYPT, 4:SEC_MODE4_LEVEL4, 5:MIMT)") },
	{ "io",                     cmd_gap_io },
	{ "sniff",                  cmd_gap_sniff,                CMD_DESC("<on/off> <bd_addr> [min=0x<hex>] [max=0x<hex>] [attempt=0x<hex>] [to=0x<hex>]") },
	{ "power_mode",             cmd_gap_power_mode,           CMD_DESC("<bd_addr>") },
	{ "role_switch",            cmd_gap_role_switch,          CMD_DESC("<bd_addr> <master/slave>") },
	{ "get_role",               cmd_gap_get_role,             CMD_DESC("<bd_addr>") },
	{ "afh",                    cmd_gap_set_afh,              CMD_DESC("<afh-map:0x<hex>>") },
	{ "auto_sniff",             cmd_gap_set_auto_sniff_mode,  CMD_DESC("<on/off>") },
	{ "help",                   cmd_gap_help_exec,            CMD_DESC(CMD_HELP_DESC) },
};

static enum cmd_status cmd_gap_help_exec(char *cmd)
{
	return cmd_help_exec(g_gap_cmds, cmd_nitems(g_gap_cmds), 20);
}

enum cmd_status cmd_gap_exec(char *cmd)
{
	return cmd_exec(cmd, g_gap_cmds, cmd_nitems(g_gap_cmds));
}
