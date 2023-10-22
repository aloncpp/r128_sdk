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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "xr_log.h"
#include "xr_bt_main.h"
#include "xr_bt_device.h"
#include "xr_gap_bt_api.h"
#include "xr_hf_ag_api.h"
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "freertos/queue.h"
//#include "freertos/semphr.h"
#include "time.h"
#include "sys/time.h"
//#include "sdkconfig.h"
#include "bt_app_core.h"
//#include "bt_app_hf.h"
#include "cmd_util.h"
#include "bt_utils.h"
// #include "btc_hf_ag.h"
#include <math.h>
static xr_bd_addr_t hf_peer_addr;
/*
static void bda2str_dump(char *prefex, xr_bd_addr_t bda)
{
	if (bda == NULL) {
		return;
	}

	uint8_t *p = bda;
	CMD_DBG("%s %02x:%02x:%02x:%02x:%02x:%02x\n", prefex,
			p[0], p[1], p[2], p[3], p[4], p[5]);
}*/

static const char *c_hf_evt_str[] = {
	"CONNECTION_STATE_EVT",              /*!< SERVICE LEVEL CONNECTION STATE CONTROL */
	"AUDIO_STATE_EVT",                   /*!< AUDIO CONNECTION STATE CONTROL */
	"VR_STATE_CHANGE_EVT",               /*!< VOICE RECOGNITION CHANGE */
	"VOLUME_CONTROL_EVT",                /*!< AUDIO VOLUME CONTROL */
	"UNKNOW_AT_CMD",                     /*!< UNKNOW AT COMMAND RECIEVED */
	"IND_UPDATE",                        /*!< INDICATION UPDATE */
	"CIND_RESPONSE_EVT",                 /*!< CALL & DEVICE INDICATION */
	"COPS_RESPONSE_EVT",                 /*!< CURRENT OPERATOR EVENT */
	"CLCC_RESPONSE_EVT",                 /*!< LIST OF CURRENT CALL EVENT */
	"CNUM_RESPONSE_EVT",                 /*!< SUBSCRIBER INFORTMATION OF CALL EVENT */
	"DTMF_RESPONSE_EVT",                 /*!< DTMF TRANSFER EVT */
	"NREC_RESPONSE_EVT",                 /*!< NREC RESPONSE EVT */
	"ANSWER_INCOMING_EVT",               /*!< ANSWER INCOMING EVT */
	"REJECT_INCOMING_EVT",               /*!< AREJECT INCOMING EVT */
	"DIAL_EVT",                          /*!< DIAL INCOMING EVT */
	"WBS_EVT",                           /*!< CURRENT CODEC EVT */
	"BCS_EVT",                           /*!< CODEC NEGO EVT */
};

//xr_hf_connection_state_t
static const char *c_connection_state_str[] = {
	"DISCONNECTED",
	"CONNECTING",
	"CONNECTED",
	"SLC_CONNECTED",
	"DISCONNECTING",
};

// xr_hf_audio_state_t
static const char *c_audio_state_str[] = {
	"disconnected",
	"connecting",
	"connected",
	"connected_msbc",
};

// xr_hf_vr_state_t
static const char *c_vr_state_str[] = {
	"Disabled",
	"Enabled",
};

// xr_hf_nrec_t
static const char *c_nrec_status_str[] = {
	"NREC DISABLE",
	"NREC ABLE",
};

// xr_hf_control_target_t
static const char *c_volume_control_target_str[] = {
	"SPEAKER",
	"MICROPHONE",
};

// xr_hf_subscriber_service_type_t
static const char *c_operator_name_str[] = {
	"中国移动",
	"中国联通",
	"中国电信",
};

// xr_hf_subscriber_service_type_t
static const char *c_subscriber_service_type_str[] = {
	"UNKNOWN",
	"VOICE",
	"FAX",
};

// xr_hf_nego_codec_status_t
static const char *c_codec_mode_str[] = {
	"CVSD Only",
	"Use CVSD",
	"Use MSBC",
};

#if CONFIG_BT_HFP_AUDIO_DATA_PATH_HCI
// Produce a sine audio
static uint32_t bt_app_hf_outgoing_cb(uint8_t *p_buf, uint32_t sz)
{
	const float pi = 3.1415926;
	const float freq = 2000; // 2000hz
	const float sample_rate_8k = 8000; // 8k sample rate
	const float volume = 30000.0;
	static int j = 0;
	for (int i = 0; 2 * i + 1 < sz; i++, j++) {
		int16_t tmp = (int16_t)(sin((2 * pi * freq) / sample_rate_8k * j) * volume);
		p_buf[i * 2]     = tmp;
		p_buf[i * 2 + 1] = tmp >> 8;
    }
	return sz;
}

static void bt_app_hf_incoming_cb(const uint8_t *buf, uint32_t sz)
{
	xr_hf_outgoing_data_ready();
}
#endif /* #if CONFIG_BT_HFP_AUDIO_DATA_PATH_HCI */

void bt_app_hf_cb(xr_hf_cb_event_t event, xr_hf_cb_param_t *param)
{
	if (event <= XR_HF_BCS_RESPONSE_EVT) {
		CMD_LOG(1, "APP HFP event: %s\n", c_hf_evt_str[event]);
	} else {
		CMD_ERR("APP HFP invalid event %d\n", event);
	}

	switch (event) {
	case XR_HF_CONNECTION_STATE_EVT: {
		CMD_LOG(1, "HF connection state %s, peer feats 0x%x, chld_feats 0x%x\n",
		    c_connection_state_str[param->conn_stat.state],
		    param->conn_stat.peer_feat,
		    param->conn_stat.chld_feat);
		memcpy(hf_peer_addr, param->conn_stat.remote_bda, XR_BD_ADDR_LEN);
		break;
	}

	case XR_HF_AUDIO_STATE_EVT: {
		CMD_LOG(1, "HF Audio State %s\n", c_audio_state_str[param->audio_stat.state]);
#if CONFIG_BT_HFP_AUDIO_DATA_PATH_HCI
		if (param->audio_stat.state == XR_HF_AUDIO_STATE_CONNECTED ||
		    param->audio_stat.state == XR_HF_AUDIO_STATE_CONNECTED_MSBC) {
			xr_bt_hf_register_data_callback(bt_app_hf_incoming_cb, bt_app_hf_outgoing_cb);
		} else if (param->audio_stat.state == XR_HF_AUDIO_STATE_DISCONNECTED) {
			CMD_LOG(1, "HF Audio Connection Disconnected.\n");
		}
#endif /* #if CONFIG_BT_HFP_AUDIO_DATA_PATH_HCI */
		break;
	}

	case XR_HF_BVRA_RESPONSE_EVT: {
		CMD_LOG(1, "HF Voice Recognition is %s\n", c_vr_state_str[param->vra_rep.value]);
		break;
	}

	case XR_HF_VOLUME_CONTROL_EVT: {
		CMD_LOG(1, "HF Volume Target: %s, Volume %d\n", c_volume_control_target_str[param->volume_control.type], param->volume_control.volume);
		break;
	}

	case XR_HF_UNAT_RESPONSE_EVT: {
		CMD_LOG(1, "HF UNKOW AT CMD: %s\n", param->unat_rep.unat);
		xr_hf_unat_response(hf_peer_addr, NULL);
		break;
	}

	case XR_HF_IND_UPDATE_EVT: {
		CMD_LOG(1, "HF UPDATE INDCATOR!\n");
		xr_hf_call_status_t call_state = 1;
		xr_hf_call_setup_status_t call_setup_state = 2;
		xr_hf_network_state_t ntk_state = 1;
		int signal = 2;
		xr_bt_hf_indchange_notification(hf_peer_addr, call_state, call_setup_state, ntk_state, signal);
		break;
	}

	case XR_HF_CIND_RESPONSE_EVT: {
		CMD_LOG(1, "HF CIND Start.\n");
		xr_hf_call_status_t call_status = 0;
		xr_hf_call_setup_status_t call_setup_status = 0;
		xr_hf_network_state_t ntk_state = 1;
		int signal = 4;
		xr_hf_roaming_status_t roam = 0;
		int batt_lev = 3;
		xr_hf_call_held_status_t call_held_status = 0;
		xr_bt_hf_cind_response(hf_peer_addr, call_status, call_setup_status, ntk_state, signal, roam, batt_lev, call_held_status);
		break;
	}

	case XR_HF_COPS_RESPONSE_EVT: {
		const int svc_type = 1;
		xr_bt_hf_cops_response(hf_peer_addr, (char *)c_operator_name_str[svc_type]);
		break;
	}

	case XR_HF_CLCC_RESPONSE_EVT: {
		int index = 1;
		//mandatory
		xr_hf_current_call_direction_t dir = 1;
		xr_hf_current_call_status_t current_call_status = 0;
		xr_hf_current_call_mode_t mode = 0;
		xr_hf_current_call_mpty_type_t mpty = 0;
		//option
		char *number = {"123456"};
		xr_hf_call_addr_type_t type = XR_HF_CALL_ADDR_TYPE_UNKNOWN;

		CMD_LOG(1, "HF Calling Line Identification.\n");
		xr_bt_hf_clcc_response(hf_peer_addr, index, dir, current_call_status, mode, mpty, number, type);
		break;
	}

	case XR_HF_CNUM_RESPONSE_EVT: {
		char *number = {"123456"};
		xr_hf_subscriber_service_type_t type = 1;
		CMD_LOG(1, "HF Current Number is %s ,Type is %s.\n", number, c_subscriber_service_type_str[type]);
		xr_bt_hf_cnum_response(hf_peer_addr, number,type);
		break;
	}

	case XR_HF_VTS_RESPONSE_EVT: {
		CMD_LOG(1, "HF DTMF code is: %s.\n", param->vts_rep.code);
		break;
	}

	case XR_HF_NREC_RESPONSE_EVT: {
		CMD_LOG(1, "HF NREC status is: %s.\n", c_nrec_status_str[param->nrec.state]);
		break;
	}

	case XR_HF_ATA_RESPONSE_EVT: {
		CMD_LOG(1, "HF Asnwer Incoming Call.\n");
		char *number = {"123456"};
		xr_bt_hf_answer_call(hf_peer_addr, 1, 0, 1, 0, number, 0);
		break;
	}

	case XR_HF_CHUP_RESPONSE_EVT: {
		CMD_LOG(1, "HF Reject Incoming Call.");
		char *number = {"123456"};
		xr_bt_hf_reject_call(hf_peer_addr, 0, 0, 0, 0, number, 0);
		break;
	}

	case XR_HF_DIAL_EVT: {
		if (param->out_call.num_or_loc) {
			//dia_num_or_mem
			CMD_LOG(1, "HF Dial \"%s\".\n", param->out_call.num_or_loc);
			xr_bt_hf_out_call(hf_peer_addr, 1, 0, 1, 0, param->out_call.num_or_loc, 0);
		} else {
			//dia_last
			CMD_LOG(1, "HF Dial last number.\n");
		}
		break;
	}
#if (BTM_WBS_INCLUDED == TRUE)
	case XR_HF_WBS_RESPONSE_EVT: {
		CMD_LOG(1, "HF Current codec: %s\n",c_codec_mode_str[param->wbs_rep.codec]);
		break;
	}
#endif
	case XR_HF_BCS_RESPONSE_EVT: {
		CMD_LOG(1, "HF Consequence of codec negotiation: %s\n",c_codec_mode_str[param->bcs_rep.mode]);
		break;
	}

	default:
		CMD_LOG(1, "Unsupported HF_AG EVT: %d.\n", event);
		break;
	}
}

static enum cmd_status cmd_hfp_ag_init_exec(char *cmd)
{
	xr_bd_addr_t remote_bda = { 0 };

	int argc;
	char *argv[1];
	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc != 0) {
		str2bda(argv[0], remote_bda);
	}

	//xr_bt_hf_register_data_callback(bt_incoming_data, bt_outgoing_data);

	xr_bt_hf_register_callback(bt_app_hf_cb);

	if (xr_bt_hf_init(remote_bda) != XR_OK) {
		CMD_ERR("HF init fail!\n");
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_hfp_ag_deinit_exec(char *cmd)
{
	xr_bd_addr_t remote_bda = { 0 };

	int argc;
	char *argv[1];
	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc != 0) {
		str2bda(argv[0], remote_bda);
	}

	if ((xr_bt_hf_deinit(remote_bda) != XR_OK)) {
		CMD_ERR("HF deinit fail!\n");
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/* $hfp_ag connect 31:AF:00:CC:BB:AA */
static enum cmd_status cmd_hfp_ag_connect_exec(char *cmd)
{
	xr_err_t ret;
	xr_bd_addr_t remote_bda = { 0 };
	int argc;
	char *argv[1];

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc != 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	str2bda(argv[0], remote_bda);
	bda2str_dump("hfp ag connect", remote_bda);

	if ((ret = xr_bt_hf_connect(remote_bda)) != XR_OK) {
		CMD_ERR("xr_bt_hf_connect failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $hfp_ag disconnect <addr> */
static enum cmd_status cmd_hfp_ag_disconnect_exec(char *cmd)
{
	xr_err_t ret;
	xr_bd_addr_t remote_bda = { 0 };
	int argc;
	char *argv[1];

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc != 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	str2bda(argv[0], remote_bda);
	bda2str_dump("hfp ag disconnect", remote_bda);

	if ((ret = xr_bt_hf_disconnect(remote_bda)) != XR_OK) {
		CMD_ERR("xr_hf_client_disconnect failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $hfp audio <connect/disconnect/play> [31:AF:00:CC:BB:AA] */
static enum cmd_status cmd_hfp_audio_exec(char *cmd)
{
	xr_err_t ret;
	xr_bd_addr_t remote_bda = {0};
	int argc;
	char *argv[2];

	argc = cmd_parse_argv(cmd, argv, 2);
	if (argc < 1 || argc > 2) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}
	str2bda(argv[1], remote_bda);

	if (!cmd_strcmp(argv[0], "connect")) {
		bda2str_dump("hfp ag audio connect", remote_bda);
		if ((ret = xr_bt_hf_connect_audio(remote_bda)) != XR_OK) {
			CMD_ERR("return failed: %d\n", ret);
			return CMD_STATUS_FAIL;
		}
	} else if (!cmd_strcmp(argv[0], "disconnect")) {
		bda2str_dump("hfp ag audio disconnect", remote_bda);
		if ((ret = xr_bt_hf_disconnect_audio(remote_bda)) != XR_OK) {
			CMD_ERR("return failed: %d\n", ret);
			return CMD_STATUS_FAIL;
		}
	} else if (!cmd_strcmp(argv[0], "play")) {
		CMD_LOG(1, "hfp ag audio play\n");
		xr_hf_outgoing_data_ready();
	} else {
		CMD_ERR("invalid param %s\n", argv[0]);
		return CMD_STATUS_INVALID_ARG;
	}

	return CMD_STATUS_OK;
}

/*
	$hfp_ag init
	$hfp_ag deinit
	$hfp_ag connect <addr>
		$hfp_ag connect 31:AF:00:CC:BB:AA
	$hfp_ag disconnect <addr>
	$hfp_ag audio <conn/disconn> <addr>
		$hfp_ag audio conn 31:AF:00:CC:BB:AA
*/
static const struct cmd_data g_hfp_ag_cmds[] = {
	{ "init",        cmd_hfp_ag_init_exec },
	{ "deinit",      cmd_hfp_ag_deinit_exec },
	{ "connect",     cmd_hfp_ag_connect_exec },
	{ "disconnect",  cmd_hfp_ag_disconnect_exec },
	{ "audio",       cmd_hfp_audio_exec },
};

enum cmd_status cmd_hfp_ag_exec(char *cmd)
{
	return cmd_exec(cmd, g_hfp_ag_cmds, cmd_nitems(g_hfp_ag_cmds));
}


