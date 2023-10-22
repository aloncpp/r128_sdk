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

#if (defined(CONFIG_BLEHOST) && defined(CONFIG_DRIVERS_XRADIO))

#include <string.h>

#include <console.h>

#include "cmd_util.h"
#include "kernel/os/os.h"
#include "blink.h"
#include "ble/bluetooth/conn.h"
#include "ble/bluetooth/bluetooth.h"

#include "net_ctrl/net_init.h"
#include "smartlink/sc_assistant.h"

#define THREAD_STACK_SIZE       (2 * 1024)

static XR_OS_Thread_t g_thread;
static uint32_t timeout = 0;
static uint8_t blink_start_status = 0;

extern struct netif *g_wlan_netif;

static void blink_task(void *arg)
{
	blink_result_t result;

	CMD_DBG("%s getting ssid and psk...\n", __func__);

	if (blink_wait(timeout) != 0) {
		CMD_ERR("wait fail\n");
		goto out;
	}

	if (blink_get_result(&result) != BLINK_OK) {
		CMD_ERR("get result fail\n");
		goto out;
	}

	sc_assistant_open_sta();
	int ret = sc_assistant_connect_ap(result.ssid, result.ssid_len,
	                                  result.passphrase, 120000);
	if (ret < 0) {
		CMD_DBG("connect ap time out\n");
		blink_set_state(BLINK_STATE_FAIL);
	} else {
		CMD_DBG("connect ap success\n");
		blink_set_state(BLINK_STATE_SUCCESS);
	}

out:
	XR_OS_ThreadDelete(&g_thread);
}

enum cmd_status cmd_blink_start_exec(char *cmd)
{
	if (blink_start_status) {
		CMD_ERR("start against\n");
		return CMD_STATUS_FAIL;
	}
	blink_start_status = 1;

	xr_wlan_on(WLAN_MODE_MONITOR);

	sc_assistant_fun_t sca_fun;
	sc_assistant_time_config_t config;

	sc_assistant_get_fun(&sca_fun);
	config.time_total = 120000;
	config.time_sw_ch_long = 0;
	config.time_sw_ch_short = 0;
	sc_assistant_init(g_wlan_netif, &sca_fun, &config);

	XR_OS_MSleep(200);

	blink_param_t param;
	cmd_memset(&param, 0, sizeof(blink_param_t));
	blink_start(&param);

	return CMD_STATUS_OK;
}

enum cmd_status cmd_blink_wait_exec(char *cmd)
{
	if (!blink_start_status) {
		CMD_ERR("not started\n");
		return CMD_STATUS_FAIL;
	}

	if (XR_OS_ThreadIsValid(&g_thread))
		return CMD_STATUS_FAIL;

	int cnt = cmd_sscanf(cmd, "t=%u", &timeout);
	if (cnt != 1) {
		CMD_ERR("cmd_sscanf return: cnt = %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (XR_OS_ThreadCreate(&g_thread,
	                "cmd_blink",
	                blink_task,
	                &timeout,
	                XR_OS_THREAD_PRIO_APP,
	                THREAD_STACK_SIZE) != XR_OS_OK) {
		CMD_ERR("create vblink thread failed\n");
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

enum cmd_status cmd_blink_stop_exec(char *cmd)
{
	if (!blink_start_status) {
		CMD_ERR("not started\n");
		return CMD_STATUS_FAIL;
	}
	blink_start_status = 0;

	blink_stop();

	sc_assistant_deinit(g_wlan_netif);

	return CMD_STATUS_OK;
}

static const struct cmd_data g_blink_cmds[] = {
	{ "start",       cmd_blink_start_exec},
	{ "wait",        cmd_blink_wait_exec},
	{ "stop",        cmd_blink_stop_exec},
};

enum cmd_status cmd_blink_exec(char *cmd)
{
	return cmd_exec(cmd, g_blink_cmds, cmd_nitems(g_blink_cmds));
}

static void blink_exec(int argc, char *argv[])
{
	cmd2_main_exec(argc, argv, cmd_blink_exec);
}

FINSH_FUNCTION_EXPORT_CMD(blink_exec, blink, blink testcmd);

#endif /* defined(CONFIG_BLEHOST) && defined(CONFIG_DRIVERS_XRADIO) */
