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

#include "kernel/os/os_mutex.h"

#include "bt_app_audio.h"
#include "bt_app_core.h"
#include "xr_gap_bt_api.h"
#include "xr_a2dp_api.h"

#include "cmd_util.h"
#include "xr_a2dp_api.h"
#include "xr_gap_bt_api.h"
#include "bt_utils.h"

static xr_a2d_audio_state_t s_audio_state = XR_A2D_AUDIO_STATE_STOPPED;
static const char *s_a2d_conn_state_str[] = {"Disconnected", "Connecting", "Connected", "Disconnecting"};
static const char *s_a2d_audio_state_str[] = {"Suspended", "Stopped", "Started"};
static enum cmd_status cmd_a2dp_sink_help_exec(char *cmd);

static void pcm_config(uint8_t *codec_info_element)
{
	int sample_rate = 16000;
	int channels = 2;
	char oct0 = codec_info_element[0];
	if (oct0 & (0x01 << 6)) {
		sample_rate = 32000;
	} else if (oct0 & (0x01 << 5)) {
		sample_rate = 44100;
	} else if (oct0 & (0x01 << 4)) {
		sample_rate = 48000;
	}

	if ((oct0 & 0x0F) == 0x08) {
		channels = 1;
	}

	bt_app_audio_config(sample_rate, channels, BT_APP_AUDIO_TYPE_A2DP);

	CMD_DBG("pcm_config: channels:%d, sampling:%d\n", channels, sample_rate);
}

static void bt_a2dp_handle_event(uint16_t event, void *p_param)
{
	CMD_DBG("%s evt %d\n", __func__, event);
	xr_a2d_cb_param_t *a2d = NULL;
	switch (event) {
	case XR_A2D_CONNECTION_STATE_EVT: {
		a2d = (xr_a2d_cb_param_t *)(p_param);
		uint8_t *bda = a2d->conn_stat.remote_bda;
		CMD_DBG("A2DP connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]\n",
		    s_a2d_conn_state_str[a2d->conn_stat.state], bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
		if (a2d->conn_stat.state == XR_A2D_CONNECTION_STATE_DISCONNECTED) {
			xr_bt_gap_set_scan_mode(XR_BT_CONNECTABLE, XR_BT_GENERAL_DISCOVERABLE, XR_BT_PAIRABLE, XR_BT_ALL_PAIRABLE);
		} else if (a2d->conn_stat.state == XR_A2D_CONNECTION_STATE_CONNECTED){
			xr_bt_gap_set_scan_mode(XR_BT_CONNECTABLE, XR_BT_GENERAL_DISCOVERABLE, XR_BT_PAIRABLE, XR_BT_ALL_PAIRABLE);
		}
		break;
	}
	case XR_A2D_AUDIO_STATE_EVT: {
		a2d = (xr_a2d_cb_param_t *)(p_param);
		CMD_DBG("A2DP audio state: %s state %d\n", s_a2d_audio_state_str[a2d->audio_stat.state], a2d->audio_stat.state);
		s_audio_state = a2d->audio_stat.state;
		if (XR_A2D_AUDIO_STATE_STARTED == a2d->audio_stat.state) {
			sys_handler_send(bt_app_audio_ctrl, BT_APP_AUDIO_EVENTS_A2DP_START, 1);
		} else {
			sys_handler_send(bt_app_audio_ctrl, BT_APP_AUDIO_EVENTS_A2DP_STOP, 1);
		}
		break;
	}
	case XR_A2D_AUDIO_CFG_EVT: {
		a2d = (xr_a2d_cb_param_t *)(p_param);
		CMD_DBG("A2DP audio stream configuration, codec type %d\n", a2d->audio_cfg.mcc.type);
		if (a2d->audio_cfg.mcc.type == XR_A2D_MCT_SBC) {
			pcm_config(a2d->audio_cfg.mcc.cie.sbc);
		}
		break;
	}
	case XR_A2D_PROF_STATE_EVT:
		a2d = (xr_a2d_cb_param_t *)(p_param);
		CMD_DBG("a2dp_snk %s success\n", (a2d->a2d_prof_stat.init_state == XR_A2D_INIT_SUCCESS) ? "init" : "deinit");
		break;
	default:
		CMD_ERR("%s unhandled evt %d\n", __func__, event);
		break;
	}
}

/* callback for A2DP sink */
static void bt_app_a2dp_cb_handle(xr_a2d_cb_event_t event, xr_a2d_cb_param_t *param)
{
	bt_app_work_dispatch(bt_a2dp_handle_event, event, param, sizeof(xr_a2d_cb_param_t), NULL);
}

static void bt_app_a2d_data_cb_handle(const uint8_t *data, uint32_t len)
{
	bt_app_audio_write_unblock(data, len, BT_APP_AUDIO_TYPE_A2DP);
	// bt_app_audio_write((uint8_t *)data, len);
}

/* initialize A2DP sink */
enum cmd_status cmd_sink_init_exec(char *cmd)
{
	xr_a2d_register_callback(&bt_app_a2dp_cb_handle);
	xr_a2d_sink_register_data_callback(bt_app_a2d_data_cb_handle);
	xr_a2d_sink_init();
	return CMD_STATUS_OK;
}

/* deinitialize A2DP sink */
enum cmd_status cmd_sink_deinit_exec(char *cmd)
{
	xr_a2d_sink_deinit();
	return CMD_STATUS_OK;
}

enum cmd_status cmd_sink_con_exec(char *cmd)
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

	str2bda(argv[0], remote_bda);
	bda2str_dump("sink_connect.\n", remote_bda);

	if ((ret = xr_a2d_sink_connect(remote_bda)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/* $a2dp_snk disconnect [bd_addr]*/
enum cmd_status cmd_sink_discon_exec(char *cmd)
{
	xr_err_t ret;
	xr_bd_addr_t remote_bda = { 0 };
	int argc;
	char *argv[1];

	argc = cmd_parse_argv(cmd, argv, 1);

	if (argc) {
		str2bda(argv[0], remote_bda);
		bda2str_dump("sink_disconnect %s.\n", remote_bda);
	}

	if ((ret = xr_a2d_sink_disconnect(remote_bda)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/*
    $a2dp_snk init
    $a2dp_snk connect <bdaddr>
    $a2dp_snk deinit
 */
static const struct cmd_data g_a2dp_sink_cmds[] = {
	{ "init",             cmd_sink_init_exec,      CMD_DESC("No parameters") },
	{ "connect",          cmd_sink_con_exec,       CMD_DESC("<bd_addr>") },
	{ "disconnect",       cmd_sink_discon_exec,    CMD_DESC("<bd_addr>") },
	{ "deinit",           cmd_sink_deinit_exec,    CMD_DESC("No parameters") },
	{ "help",             cmd_a2dp_sink_help_exec, CMD_DESC(CMD_HELP_DESC) },
};

static enum cmd_status cmd_a2dp_sink_help_exec(char *cmd)
{
	return cmd_help_exec(g_a2dp_sink_cmds, cmd_nitems(g_a2dp_sink_cmds), 8);
}

enum cmd_status cmd_a2dp_sink_exec(char *cmd)
{
	return cmd_exec(cmd, g_a2dp_sink_cmds, cmd_nitems(g_a2dp_sink_cmds));
}
