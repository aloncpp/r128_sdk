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
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "xr_log.h"
#include "xr_bt.h"
#include "xr_bt_main.h"
#include "xr_gap_bt_api.h"
#include "xr_bt_device.h"
#include "xr_spp_api.h"

#include "time.h"
#include "sys/time.h"

#include "cmd_util.h"
#include "bt_utils.h"

#define xr_err_to_name(ret)    (ret)

#define SPP_SERVER_NAME "SPP_SERVER"
#define DEVICE_NAME "XR_SPP_MODULE"
#define SPP_SHOW_DATA 0      /*Choose show mode: show data or speed*/
#define SPP_SHOW_SPEED 1
#define SPP_MIN_SCN 1
#define SPP_MAX_SCN 30
#define PRINT_TIME_DIFF 3
#define MAX_DATA_LEN 512

static const xr_spp_mode_t xr_spp_mode = XR_SPP_MODE_CB;

static struct timeval time_new, time_old;
static long data_num = 0;
static uint32_t connect_handle;

static const xr_spp_sec_t sec_mask = XR_SPP_SEC_AUTHENTICATE;
//static const xr_spp_role_t role_slave = XR_SPP_ROLE_SLAVE;
static int spp_show_mode = SPP_SHOW_DATA;
static xr_spp_status_t discover_status;
static uint8_t discovery_scn[XR_SPP_MAX_SCN];

static enum cmd_status cmd_spp_help_exec(char *cmd);

static void print_speed(void)
{
	float time_old_s = time_old.tv_sec + time_old.tv_usec / 1000000.0;
	float time_new_s = time_new.tv_sec + time_new.tv_usec / 1000000.0;
	float time_interval = time_new_s - time_old_s;
	float speed = data_num * 8 / time_interval / 1000.0;
	CMD_DBG("speed(%fs ~ %fs): %f kbit/s\n" , time_old_s, time_new_s, speed);
	data_num = 0;
	time_old.tv_sec = time_new.tv_sec;
	time_old.tv_usec = time_new.tv_usec;
}

static void xr_spp_cb(xr_spp_cb_event_t event, xr_spp_cb_param_t *param)
{
	switch (event) {
	case XR_SPP_INIT_EVT:
		CMD_DBG("spp initialized\n");
		break;
	case XR_SPP_DISCOVERY_COMP_EVT:
		CMD_DBG("spp discovery status=%d scn_num=%d\n", param->disc_comp.status, param->disc_comp.scn_num);
		if (param->disc_comp.status != XR_OK) {
			CMD_ERR("spp discovery failed, status %d\n", param->disc_comp.status);
			break;
		}
		discover_status = param->disc_comp.status;
		discovery_scn[0] = param->disc_comp.scn[0];
		for (int i = 0; i < param->disc_comp.scn_num; i++) {
			CMD_DBG("disc_comp.scn[%d] = %d\n", i, param->disc_comp.scn[i]);
		}
		break;
	case XR_SPP_OPEN_EVT:
		CMD_DBG("spp client connection open\n");
		connect_handle = param->open.handle;
		CMD_DBG("client connect handle = %d\n", connect_handle);
		break;
	case XR_SPP_CLOSE_EVT:
		CMD_DBG("spp client connection close\n");
		break;
	case XR_SPP_START_EVT:
		CMD_DBG("spp server start\n");
		break;
	case XR_SPP_CL_INIT_EVT:
		CMD_DBG("spp client initiated connection\n");
		break;
	case XR_SPP_DATA_IND_EVT:
		if (spp_show_mode == SPP_SHOW_DATA) {
			CMD_DBG("spp connection received data len=%d handle=%d\n",
			        param->data_ind.len, param->data_ind.handle);
		xr_log_buffer_char("", param->data_ind.data, param->data_ind.len);
		} else {
			gettimeofday(&time_new, NULL);
			data_num += param->data_ind.len;
			if (time_new.tv_sec - time_old.tv_sec >= PRINT_TIME_DIFF) {
				print_speed();
		}
	}
		break;
	case XR_SPP_CONG_EVT:
		CMD_DBG("spp connection congestion\n");
		break;
	case XR_SPP_WRITE_EVT:
		CMD_DBG("spp completes writing data\n");
		break;
	case XR_SPP_SRV_OPEN_EVT:
		CMD_DBG("spp server connection open\n");
		connect_handle = param->srv_open.handle;
		CMD_DBG("service connect handle = %d\n", connect_handle);
		gettimeofday(&time_old, NULL);
		break;
	case XR_SPP_SRV_STOP_EVT:
		CMD_DBG("spp server connection stop\n");
		break;
	case XR_SPP_UNINIT_EVT:
		CMD_DBG("spp deinitialized\n");
		break;
	default:
		CMD_DBG("spp unhandle event %d\n", event);
		break;
	}
}

/* bt spp init*/
static enum cmd_status cmd_spp_init_exec(char *cmd)
{
	xr_err_t ret;
	if ((ret = xr_spp_register_callback(xr_spp_cb)) != XR_OK) {
		CMD_ERR("spp register failed: %d\n", xr_err_to_name(ret));
		return CMD_STATUS_FAIL;
	}

	if ((ret = xr_spp_init(xr_spp_mode)) != XR_OK) {
		CMD_ERR("spp init failed: %d\n", xr_err_to_name(ret));
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/* bt spp deinit*/
static enum cmd_status cmd_spp_deinit_exec(char *cmd)
{
	xr_err_t ret;
	if ((ret = xr_spp_deinit()) != XR_OK) {
		CMD_ERR("spp deinit failed: %d\n", xr_err_to_name(ret));
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/* bt spp start_srv [scn] */
static enum cmd_status cmd_spp_start_srv_exec(char *cmd)
{
	xr_err_t ret;
	int argc;
	int scn = 0;
	char *argv[1];
	argc = cmd_parse_argv(cmd, argv, 1);

	if (argc > 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	if (argc == 1) {
		const char *arg = argv[0];
		if (cmd_sscanf(arg, "%d", &scn) != 1) {
			CMD_ERR("invalid param %s\n", arg);
			return CMD_STATUS_INVALID_ARG;
		}
		if (scn < SPP_MIN_SCN || scn > SPP_MAX_SCN) {
			CMD_ERR("The value range of SCN is 1 to 30\n");
			return CMD_STATUS_INVALID_ARG;
		}
	}

	if ((ret = xr_spp_start_srv(sec_mask, XR_SPP_ROLE_MASTER, scn, SPP_SERVER_NAME)) != XR_OK) {
		CMD_ERR("spp start srv failed: %d\n", xr_err_to_name(ret));
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/* bt spp stop_srv [scn] */
static enum cmd_status cmd_spp_stop_srv_exec(char *cmd)
{
	xr_err_t ret;
	int argc;
	int scn;
	char *argv[1];
	argc = cmd_parse_argv(cmd, argv, 1);

	if (argc > 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	if (argc == 1) {
		const char *arg = argv[0];
		if (cmd_sscanf(arg, "%d", &scn) != 1) {
			CMD_ERR("invalid param %s\n", arg);
			return CMD_STATUS_INVALID_ARG;
		}
		if ((ret = xr_spp_stop_srv_scn(scn)) != XR_OK) {
			CMD_ERR("spp stop srv failed: %d\n",  xr_err_to_name(ret));
			return CMD_STATUS_FAIL;
		}
	} else {
		if ((ret = xr_spp_stop_srv()) != XR_OK) {
			CMD_ERR("spp stop all srv failed: %d\n", xr_err_to_name(ret));
			return CMD_STATUS_FAIL;
		}
	}

	return CMD_STATUS_OK;
}

/* bt spp wirte <data> [handle] */
static enum cmd_status cmd_spp_write_exec(char *cmd)
{
	xr_err_t ret;
	int argc;
	char *argv[2];
	size_t len;
	int handle = connect_handle;

	argc = cmd_parse_argv(cmd, argv, 2);
	if (argc > 2 || argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	/* The length limit is to prevent data from occupying too much memory */
	len = MIN(strlen(argv[0]), MAX_DATA_LEN);
	if (strlen(argv[0]) > MAX_DATA_LEN) {
		CMD_DBG("The maximum length of data sent is %d\n", MAX_DATA_LEN);
	}

	if (argc == 2) {
		const char *arg = argv[1];
		if(cmd_sscanf(arg, "%d", &handle) != 1) {
			CMD_ERR("invalid param %s\n", arg);
			return CMD_STATUS_INVALID_ARG;
		}
	}

	if ((ret = xr_spp_write(handle, len, (uint8_t*)argv[0])) != XR_OK) {
		CMD_ERR("spp write data failed: %d\n", xr_err_to_name(ret));
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/* bt spp disconnect [handle] */
static enum cmd_status cmd_spp_disconnect_exec(char *cmd)
{
	xr_err_t ret;
	int argc;
	char *argv[1];
	int handle = connect_handle;

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc > 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	if (argc == 1) {
		const char *arg = argv[0];
		if (cmd_sscanf(arg, "%d", &handle) != 1) {
			CMD_ERR("invalid param %s\n", arg);
			return CMD_STATUS_INVALID_ARG;
		}
	}

	if ((ret = xr_spp_disconnect(handle)) != XR_OK) {
		CMD_ERR("spp disconnect failed: %d\n", xr_err_to_name(ret));
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/* bt spp discovery <mac> */
static enum cmd_status cmd_spp_discovery_exec(char *cmd)
{
	xr_err_t ret;
	xr_bd_addr_t remote_bda;
	int argc;
	char *argv[1];

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc != 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	str2bda(argv[0], remote_bda);
	bda2str_dump("spp discovery", remote_bda);

	if ((ret = xr_spp_start_discovery(remote_bda)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/* bt spp connect <mac> [scn] */
static enum cmd_status cmd_spp_connect_exec(char *cmd)
{
	int scn = discovery_scn[0];
	int argc;
	char *argv[2];
	xr_err_t ret;
	xr_bd_addr_t remote_bda;

	argc = cmd_parse_argv(cmd, argv, 2);
	if (argc < 1 || argc > 2) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	str2bda(argv[0], remote_bda);
	bda2str_dump("spp connect", remote_bda);

	if (argc == 2) {
		const char *arg = argv[1];
		if (cmd_sscanf(arg, "%d", &scn) != 1) {
			CMD_ERR("invalid param %s\n", arg);
			return CMD_STATUS_INVALID_ARG;
		}
	}

	if ((ret = xr_spp_connect(sec_mask, XR_SPP_ROLE_MASTER, scn, remote_bda)) != XR_OK) {
		CMD_ERR("spp connect failed: %d\n", xr_err_to_name(ret));
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;

}

/* bt spp show <data/speed> */
static enum cmd_status cmd_spp_show_mode_exec(char *cmd)
{
	int argc;
	char *argv[1];

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc != 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	if (!cmd_strcmp(argv[0], "speed")) {
		spp_show_mode = SPP_SHOW_SPEED;
	} else if (!cmd_strcmp(argv[0], "data")) {
		spp_show_mode = SPP_SHOW_DATA;
	} else {
		CMD_ERR("invalid param %s\n", argv[0]);
		return CMD_STATUS_INVALID_ARG;
	}

	return CMD_STATUS_OK;

}

static const struct cmd_data spp_cmds[] = {
	{ "init",                cmd_spp_init_exec,       CMD_DESC("No parameters") },
	{ "deinit",              cmd_spp_deinit_exec,     CMD_DESC("No parameters")},
	{ "write",               cmd_spp_write_exec,      CMD_DESC("<data> [handle]") },
	{ "disconnect",          cmd_spp_disconnect_exec, CMD_DESC("[handle]") },
	{ "start_srv",           cmd_spp_start_srv_exec,  CMD_DESC("[scn]") },
	{ "stop_srv",            cmd_spp_stop_srv_exec,   CMD_DESC("[scn]") },
	{ "discovery",           cmd_spp_discovery_exec,  CMD_DESC("<mac>") },
	{ "connect",             cmd_spp_connect_exec,    CMD_DESC("<mac> [scn]") },
	{ "show",                cmd_spp_show_mode_exec,  CMD_DESC("<data/speed>") },
	{ "help",                cmd_spp_help_exec,       CMD_DESC(CMD_HELP_DESC) }
};

static enum cmd_status cmd_spp_help_exec(char *cmd)
{
	return cmd_help_exec(spp_cmds, cmd_nitems(spp_cmds), 10);
}

enum cmd_status cmd_spp_exec(char *cmd)
{
	return cmd_exec(cmd, spp_cmds, cmd_nitems(spp_cmds));
}

