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

#include <string.h>
#include <stdbool.h>

#include "net/wlan/wlan.h"
#include "net/wlan/wlan_defs.h"
#include "lwip/netif.h"

#include "cmd_util.h"
#include "net_ctrl.h"
#include "net_init.h"

#include "sc_assistant.h"
#include "wlan_smart_config.h"

#define SC_TIME_OUT 120000

static uint8_t sc_key_used = 0;
static char sc_key[17] = "1234567812345678";

static XR_OS_Thread_t g_thread;
#define THREAD_STACK_SIZE       (4 * 1024)

extern struct netif *g_wlan_netif;

static void sc_task(void *arg)
{
	wlan_smart_config_result_t sc_result;
	wlan_smart_config_status_t ret;

	memset(&sc_result, 0, sizeof(wlan_smart_config_result_t));

	CMD_DBG("%s getting ssid and psk...\n", __func__);

	ret = wlan_smart_config_wait(SC_TIME_OUT);
	if ((ret == WLAN_SMART_CONFIG_TIMEOUT) || (ret == WLAN_SMART_CONFIG_STOP)) {
		CMD_DBG("%s get ssid and psk: %s\n", __func__,
		        (ret == WLAN_SMART_CONFIG_TIMEOUT) ? "TIMEOUT" : "STOP");
		goto out;
	}

	CMD_DBG("%s get ssid and psk finished, wait status:%d\n", __func__, ret);

	if (wlan_smart_config_get_status() == SC_STATUS_COMPLETE) {
		ret = wlan_smart_config_connect_ack(g_wlan_netif, SC_TIME_OUT, &sc_result);
		CMD_DBG("conn_ack status:%d, ssid:%s psk:%s random:%d\n", ret, (char *)sc_result.ssid,
		        (char *)sc_result.passphrase, sc_result.random_num);
	}

out:
	wlan_smart_config_stop();
	sc_assistant_deinit(g_wlan_netif);
	XR_OS_ThreadDelete(&g_thread);
}

static __inline int sc_task_quit(void)
{
	wlan_smart_config_quit_wait();
	wlan_smart_config_quit_connect_ack();
	return 0;
}

static int cmd_sc_start(void)
{
	wlan_smart_config_status_t sc_status;
	sc_assistant_fun_t sca_fun;
	sc_assistant_time_config_t config;

	if (XR_OS_ThreadIsValid(&g_thread))
		return -1;

	xr_wlan_on(WLAN_MODE_MONITOR);

	sc_assistant_get_fun(&sca_fun);
	config.time_total = SC_TIME_OUT;
	config.time_sw_ch_long = 100;
	config.time_sw_ch_short = 50;
	sc_assistant_init(g_wlan_netif, &sca_fun, &config);

	sc_status = wlan_smart_config_start(g_wlan_netif, sc_key_used ? sc_key : NULL);
	if (sc_status != WLAN_SMART_CONFIG_SUCCESS) {
		CMD_DBG("smartconfig start fiald!\n");
		goto out;
	}

	if (XR_OS_ThreadCreate(&g_thread,
	                    "cmd_sc",
	                    sc_task,
	                    NULL,
	                    XR_OS_THREAD_PRIO_APP,
	                    THREAD_STACK_SIZE) != XR_OS_OK) {
		CMD_ERR("create sc thread failed\n");
		goto out;
	}
	return 0;
out:
	return -1;
}

static int cmd_sc_stop(void)
{
	if (!XR_OS_ThreadIsValid(&g_thread))
		return -1;

	return sc_task_quit();
}

enum cmd_status cmd_smart_config_start_exec(char *cmd)
{
	int ret;

	if (XR_OS_ThreadIsValid(&g_thread)) {
		CMD_ERR("Smartconfig already start\n");
		ret = -1;
	} else {
		ret = cmd_sc_start();
	}
	return (ret == 0 ? CMD_STATUS_OK : CMD_STATUS_FAIL);
}

enum cmd_status cmd_smart_config_stop_exec(char *cmd)
{
	int ret;

	ret = cmd_sc_stop();
	sc_key_used = 0;
	return (ret == 0 ? CMD_STATUS_OK : CMD_STATUS_FAIL);
}

enum cmd_status cmd_smart_config_set_key_exec(char *cmd)
{
	if (cmd[0] != '\0') {
		if (cmd_strlen(cmd) == sizeof(sc_key) - 1) {
			cmd_memcpy(sc_key, cmd, sizeof(sc_key));
			sc_key_used = 1;
		} else {
			CMD_ERR("invalid argument '%s',len:%d\n", cmd, cmd_strlen(cmd));
			return CMD_STATUS_INVALID_ARG;
		}
	} else {
		sc_key_used = 1;
	}
	CMD_DBG("Smartconfig set key: %s\n", sc_key);

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_smart_config_help_exec(char *cmd);

static const struct cmd_data g_smart_config_cmds[] = {
    { "start",		cmd_smart_config_start_exec, CMD_DESC("start smart config") },
    { "stop",		cmd_smart_config_stop_exec, CMD_DESC("stop smart config") },
    { "set_key",	cmd_smart_config_set_key_exec, CMD_DESC("set smart config key") },
    { "help",	    cmd_smart_config_help_exec, CMD_DESC(CMD_HELP_DESC) },
};

static enum cmd_status cmd_smart_config_help_exec(char *cmd)
{
	return cmd_help_exec(g_smart_config_cmds, cmd_nitems(g_smart_config_cmds), 8);
}

enum cmd_status cmd_smart_config_exec(char *cmd)
{
	if (g_wlan_netif == NULL) {
		CMD_ERR("g_wlan_netif is NULL, need to open wifi first\n");
		return CMD_STATUS_FAIL;
	}

	return cmd_exec(cmd, g_smart_config_cmds, cmd_nitems(g_smart_config_cmds));
}
