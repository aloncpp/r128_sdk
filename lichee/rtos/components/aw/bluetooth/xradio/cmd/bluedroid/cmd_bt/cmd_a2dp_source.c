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
#include "string.h"
#include "xr_a2dp_api.h"
#include "xr_gap_bt_api.h"
#include "bt_utils.h"
#include "bt_app_audio.h"

/* If open music file failed and TEST_DATA is 1,src will send fixed data to sink.
 * In this case, we can hear the sound of sink.
 */
static int s_a2d_state = APP_AV_STATE_IDLE;

static enum cmd_status cmd_a2dp_source_help_exec(char *cmd);

static void bt_app_av_state_idle(uint16_t event, void *param)
{
	xr_a2d_cb_param_t *a2d = NULL;
	bt_av_media_state state;
	switch (event) {
	case XR_A2D_CONNECTION_STATE_EVT: {
		a2d = (xr_a2d_cb_param_t *)(param);
		if (a2d->conn_stat.state == XR_A2D_CONNECTION_STATE_CONNECTED) {
			CMD_DBG("a2dp connected.\n");
			s_a2d_state = APP_AV_STATE_CONNECTED;
			state = APP_AV_MEDIA_STATE_IDLE;
			bt_app_av_media_state_change(state);
			xr_a2d_media_ctrl(XR_A2D_MEDIA_CTRL_CHECK_SRC_RDY);
		} else if (a2d->conn_stat.state == XR_A2D_CONNECTION_STATE_DISCONNECTED) {
			s_a2d_state = APP_AV_STATE_IDLE;
		}
		break;
	}
	case XR_A2D_AUDIO_STATE_EVT:
	case XR_A2D_AUDIO_CFG_EVT:
	case XR_A2D_MEDIA_CTRL_ACK_EVT:
		break;
	case XR_A2D_PROF_STATE_EVT:
		a2d = (xr_a2d_cb_param_t *)(param);
		CMD_DBG("a2dp_src %s success\n", (a2d->a2d_prof_stat.init_state == XR_A2D_INIT_SUCCESS) ? "init" : "deinit");
		break;
	default:
		CMD_DBG("%s unhandled evt %d\n", __func__, event);
		break;
	}
}

static void bt_app_av_media_proc(uint16_t event, void *param)
{
	xr_a2d_cb_param_t *a2d = NULL;
	bt_av_media_state media_state;
	a2d = (xr_a2d_cb_param_t *)(param);
	CMD_DBG("media_state:%d ctl_cmd:%d, status:%d\n",
	        bt_app_get_media_state(), a2d->media_ctrl_stat.cmd, a2d->media_ctrl_stat.status);
	switch (bt_app_get_media_state()) {
	case APP_AV_MEDIA_STATE_IDLE: {
		if (a2d->media_ctrl_stat.cmd == XR_A2D_MEDIA_CTRL_CHECK_SRC_RDY &&
				a2d->media_ctrl_stat.status == XR_A2D_MEDIA_CTRL_ACK_SUCCESS) {
			CMD_DBG("a2dp media check src ready.\n");
			media_state = APP_AV_MEDIA_STATE_STARTING;
			bt_app_av_media_state_change(media_state);
		}

		if (a2d->media_ctrl_stat.cmd == XR_A2D_MEDIA_CTRL_START &&
				a2d->media_ctrl_stat.status == XR_A2D_MEDIA_CTRL_ACK_SUCCESS) {
			CMD_DBG("a2dp media ready, restart successfully\n");
			media_state = APP_AV_MEDIA_STATE_STARTED;
			bt_app_av_media_state_change(media_state);
		}
		break;
	}
	case APP_AV_MEDIA_STATE_SUSPEND:
	case APP_AV_MEDIA_STATE_STARTING: {
		if (a2d->media_ctrl_stat.cmd == XR_A2D_MEDIA_CTRL_START) {
			if (a2d->media_ctrl_stat.status == XR_A2D_MEDIA_CTRL_ACK_SUCCESS) {
				CMD_DBG("a2dp media start successfully.\n");
				media_state = APP_AV_MEDIA_STATE_STARTED;
				bt_app_av_media_state_change(media_state);
			} else {
				CMD_DBG("a2dp media start failed!\n");
			}
		} else if (a2d->media_ctrl_stat.cmd == XR_A2D_MEDIA_CTRL_STOP) {
			if (a2d->media_ctrl_stat.status == XR_A2D_MEDIA_CTRL_ACK_SUCCESS) {
				CMD_DBG("a2dp media stop successfully.\n");
				media_state = APP_AV_MEDIA_STATE_STOPPING;
				bt_app_av_media_state_change(media_state);
			} else {
				CMD_DBG("a2dp media stop failed!\n");
			}
		}
		break;
	}
	case APP_AV_MEDIA_STATE_STARTED: {
		if (a2d->media_ctrl_stat.cmd == XR_A2D_MEDIA_CTRL_SUSPEND) {
			if (a2d->media_ctrl_stat.status == XR_A2D_MEDIA_CTRL_ACK_SUCCESS) {
				CMD_DBG("a2dp media suspended successfully.\n");
				media_state = APP_AV_MEDIA_STATE_SUSPEND;
				bt_app_av_media_state_change(media_state);
			} else {
				CMD_DBG("a2dp media suspend failed!\n");
			}
		} else if (a2d->media_ctrl_stat.cmd == XR_A2D_MEDIA_CTRL_STOP) {
			if (a2d->media_ctrl_stat.status == XR_A2D_MEDIA_CTRL_ACK_SUCCESS) {
				CMD_DBG("a2dp media stop successfully!\n");
				media_state = APP_AV_MEDIA_STATE_STOPPING;
				bt_app_av_media_state_change(media_state);
			} else {
				CMD_DBG("a2dp media stop failed!\n");
			}
		}
		break;
	}
	default:
		CMD_DBG("%s invalid state %d\n", __func__, bt_app_get_media_state());
		break;
	}
}

static void bt_app_av_state_connected(uint16_t event, void *param)
{
	xr_a2d_cb_param_t *a2d = NULL;
	bt_av_media_state media_state;
	switch (event) {
	case XR_A2D_CONNECTION_STATE_EVT: {
		a2d = (xr_a2d_cb_param_t *)(param);
		if (a2d->conn_stat.state == XR_A2D_CONNECTION_STATE_DISCONNECTED) {
			CMD_DBG("a2dp disconnected\n");
			s_a2d_state = APP_AV_STATE_IDLE;
			media_state = APP_AV_MEDIA_STATE_STOPPED;
			bt_app_av_media_state_change(media_state);
		}
		break;
	}
	case XR_A2D_AUDIO_STATE_EVT: {
		a2d = (xr_a2d_cb_param_t *)(param);
		if (XR_A2D_AUDIO_STATE_STARTED == a2d->audio_stat.state) {
			CMD_DBG("a2dp audio is started\n");
		}
		break;
	}
	case XR_A2D_AUDIO_CFG_EVT:
		break;
	case XR_A2D_MEDIA_CTRL_ACK_EVT: {
		bt_app_av_media_proc(event, param);
		break;
	}
	default:
		CMD_ERR("%s unhandled evt %d\n", __func__, event);
		break;
	}
}

static void bt_app_av_sm_hdlr(uint16_t event, void *param)
{
	switch (s_a2d_state) {
	case APP_AV_STATE_IDLE:
		bt_app_av_state_idle(event, param);
		CMD_DBG("a2dp source idle\n");
		break;
	case APP_AV_STATE_CONNECTED:
		CMD_DBG("a2dp source connected\n");
		bt_app_av_state_connected(event, param);
		break;
	default:
		CMD_DBG("%s invalid state %d\n", __func__, s_a2d_state);
		break;
	}
}

static int32_t bt_app_a2d_data_cb(uint8_t *data, int32_t len)
{
	return bt_app_data_cb(data, len);
}

static void bt_app_a2d_cb(xr_a2d_cb_event_t event, xr_a2d_cb_param_t *param)
{
	bt_app_av_sm_hdlr(event, param);
}

/* $a2dp_src init */
enum cmd_status cmd_source_init_exec(char *cmd)
{
	xr_err_t ret;

	if ((ret = xr_a2d_register_callback(&bt_app_a2d_cb)) != XR_OK) {
		CMD_ERR("a2d_source_reg_cb return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	if ((ret = xr_a2d_source_register_data_callback(bt_app_a2d_data_cb)) != XR_OK) {
		CMD_ERR("a2d_source_reg_data_cb return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	if ((ret = xr_a2d_source_init()) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $a2dp_src deinit */
enum cmd_status cmd_source_deinit_exec(char *cmd)
{
	xr_err_t ret;
	if ((ret = xr_a2d_source_deinit()) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $a2dp_src connect <bd_addr> */
enum cmd_status cmd_source_connect_exec(char *cmd)
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
	bda2str_dump("source_connect %s.\n", remote_bda);

	if ((ret = xr_a2d_source_connect(remote_bda)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/* $a2dp_src disconnect [bd_addr]*/
enum cmd_status cmd_source_disconnect_exec(char *cmd)
{
	xr_err_t ret;
	xr_bd_addr_t remote_bda = { 0 };
	int argc;
	char *argv[1];

	argc = cmd_parse_argv(cmd, argv, 1);

	if (argc) {
		str2bda(argv[0], remote_bda);
		bda2str_dump("source_disconnect %s.\n", remote_bda);
	}

	if ((ret = xr_a2d_source_disconnect(remote_bda)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $a2dp_src play [name]*/
enum cmd_status cmd_source_play_exec(char *cmd)
{
	xr_err_t ret;
	int argc;
	char *argv[1];

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc) {
		bt_app_play_music(argv[0]);
	} else {
		bt_app_play_music(NULL);
	}

	if ((ret = xr_a2d_media_ctrl(XR_A2D_MEDIA_CTRL_START)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $a2dp_src suspend */
enum cmd_status cmd_source_suspend_exec(char *cmd)
{
	xr_err_t ret;

	if ((ret = xr_a2d_media_ctrl(XR_A2D_MEDIA_CTRL_SUSPEND)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $a2dp_src stop */
enum cmd_status cmd_source_stop_exec(char *cmd)
{
	xr_err_t ret;

	if ((ret = xr_a2d_media_ctrl(XR_A2D_MEDIA_CTRL_STOP)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/*
    $a2dp_src init
    $a2dp_src deinit
    $a2dp_src connect <bd_addr>
    $a2dp_src disconnect [bd_addr]
    $a2dp_src play
    $a2dp_src suspend
    $a2dp_src stop
*/

static const struct cmd_data g_a2dp_source_cmds[] = {
	{ "init",            cmd_source_init_exec,        CMD_DESC("No parameters") },
	{ "deinit",          cmd_source_deinit_exec,      CMD_DESC("No parameters") },
	{ "connect",         cmd_source_connect_exec,     CMD_DESC("<bd_addr>") },
	{ "disconnect",      cmd_source_disconnect_exec,  CMD_DESC("[bd_addr]") },
	{ "play",            cmd_source_play_exec,        CMD_DESC("[path]") },
	{ "suspend",         cmd_source_suspend_exec,     CMD_DESC("No parameters") },
	{ "stop",            cmd_source_stop_exec,        CMD_DESC("No parameters") },
	{ "help",            cmd_a2dp_source_help_exec,   CMD_DESC(CMD_HELP_DESC) },
};

static enum cmd_status cmd_a2dp_source_help_exec(char *cmd)
{
	return cmd_help_exec(g_a2dp_source_cmds, cmd_nitems(g_a2dp_source_cmds), 10);
}

enum cmd_status cmd_a2dp_source_exec(char *cmd)
{
	return cmd_exec(cmd, g_a2dp_source_cmds, cmd_nitems(g_a2dp_source_cmds));
}
