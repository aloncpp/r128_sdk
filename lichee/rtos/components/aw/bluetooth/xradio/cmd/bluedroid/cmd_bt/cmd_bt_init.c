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

#include "cmd_util.h"
#include "xr_bt_main.h"
#include "bt_ctrl.h"
#include "xr_bt_defs.h"
#include "bt_utils.h"
#include "bt_app_audio.h"
#include "bt_app_core.h"

/* $bt init */
enum cmd_status cmd_bt_init_exec(char *cmd)
{
	int argc;
	char *argv[1];
	int err = XR_FAIL;
	xr_bd_addr_t mac = { 0 };

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc > 0)
		str2bda(argv[0], mac);

	CMD_DBG("bt_ctrl enable\n");

	/* enable BT Controller */
	if (bt_ctrl_enable() != 0) {
		CMD_ERR("bt_ctrl_enable failed\n");
		return CMD_STATUS_FAIL;
	}

	/* register Bluedroid to lib */
	bt_bluedroid_adapter_register();
	XR_OS_MSleep(100);


	/* initialize Bluedroid */
	if ((err = xr_bluedroid_init()) != XR_OK) {
		CMD_ERR("%s initialize bluedroid failed: %d\n", __func__, err);
		goto bl_init_err;
	}

	/* notice: should register Bluedroid to lib firstly, before setting mac */
	bt_ctrl_set_mac(((argc >= 1) ? mac : NULL));

	/* enable Bluedroid */
	if ((err = xr_bluedroid_enable()) != XR_OK) {
		CMD_ERR("%s enable bluedroid failed: %d\n", __func__, err);
		goto bl_enable_err;
	}


	/* app audio config */
	bt_app_audio_init();

	err = sys_ctrl_create();
	if (err) {
		CMD_ERR("sys create failed.\n");
		goto sys_create_err;
	}

	bt_app_task_start_up();

	CMD_DBG("BT run ready!\n");

	return CMD_STATUS_OK;

sys_create_err:
	bt_app_audio_deinit();
	xr_bluedroid_disable();

bl_enable_err:
	xr_bluedroid_deinit();

bl_init_err:
	bt_ctrl_disable();

	return CMD_STATUS_FAIL;
}

/* $bt deinit */
enum cmd_status cmd_bt_deinit_exec(char *cmd)
{
	int err = XR_FAIL;

	bt_app_task_shut_down();

	bt_app_audio_deinit();

	if ((err = xr_bluedroid_disable()) != XR_OK) {
		CMD_ERR("%s disable bluedroid failed: %d\n", __func__, err);
		return CMD_STATUS_FAIL;
	}

	if ((err = xr_bluedroid_deinit()) != XR_OK) {
		CMD_ERR("%s deinitialize bluedroid failed: %d\n", __func__, err);
		return CMD_STATUS_FAIL;
	}

	if (bt_ctrl_disable() != 0) {
		return CMD_STATUS_FAIL;
	}
	bt_bluedroid_adapter_unregister();

	CMD_DBG("BT run exit!\n");

	return CMD_STATUS_OK;
}

