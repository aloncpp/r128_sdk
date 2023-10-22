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
#include "wlan_airkiss.h"

#define AK_TIME_OUT_MS 120000

static uint8_t ak_key_used = 0;
static char airkiss_key[17] = "1234567812345678";

#define DEVICE_TYPE "gh_a12b34cd567e"
#define DEVICE_ID "0080e129e8d1"

static XR_OS_Thread_t g_thread;
#define THREAD_STACK_SIZE       (4 * 1024)

extern struct netif *g_wlan_netif;

static void ak_task(void *arg)
{
	wlan_airkiss_result_t ak_result;

	memset(&ak_result, 0, sizeof(wlan_airkiss_result_t));

	CMD_DBG("%s getting ssid and psk...\n", __func__);

	if (wlan_airkiss_wait(AK_TIME_OUT_MS) == WLAN_AIRKISS_TIMEOUT) {
		CMD_DBG("%s get ssid and psk timeout\n", __func__);
		goto out;
	}
	CMD_DBG("%s get ssid and psk finished\n", __func__);

	if (wlan_airkiss_get_status() == AIRKISS_STATUS_COMPLETE) {
		if (!wlan_airkiss_connect_ack(g_wlan_netif, AK_TIME_OUT_MS, &ak_result)) {
			CMD_DBG("ssid:%s psk:%s random:%d\n", (char *)ak_result.ssid,
			        (char *)ak_result.passphrase, ak_result.random_num);
			/* use this to do airkiss discover */
			//wlan_airkiss_lan_discover_start(DEVICE_TYPE, DEVICE_ID, 1000);
			//XR_OS_MSleep(30000);
			//wlan_airkiss_lan_discover_stop();
		}
	}

out:
	wlan_airkiss_stop();
	sc_assistant_deinit(g_wlan_netif);
	XR_OS_ThreadDelete(&g_thread);
}

static int cmd_airkiss_start(void)
{
	int ret = 0;
	wlan_airkiss_status_t ak_status;
	sc_assistant_fun_t sca_fun;
	sc_assistant_time_config_t config;

	if (XR_OS_ThreadIsValid(&g_thread))
		return -1;

	xr_wlan_on(WLAN_MODE_MONITOR);

	sc_assistant_get_fun(&sca_fun);
	config.time_total = AK_TIME_OUT_MS;
	config.time_sw_ch_long = 100;
	config.time_sw_ch_short = 50;
	sc_assistant_init(g_wlan_netif, &sca_fun, &config);

	ak_status = wlan_airkiss_start(g_wlan_netif, ak_key_used ? airkiss_key : NULL);
	if (ak_status != WLAN_AIRKISS_SUCCESS) {
		CMD_DBG("airkiss start fiald!\n");
		ret = -1;
		goto out;
	}

	if (XR_OS_ThreadCreate(&g_thread,
	                    "cmd_ak",
	                    ak_task,
	                    NULL,
	                    XR_OS_THREAD_PRIO_APP,
	                    THREAD_STACK_SIZE) != XR_OS_OK) {
		CMD_ERR("create ak thread failed\n");
		ret = -1;
	}
out:
	return ret;
}

static int cmd_airkiss_stop(void)
{
	if (!XR_OS_ThreadIsValid(&g_thread))
		return -1;

	return wlan_airkiss_stop();
}

enum cmd_status cmd_airkiss_start_exec(char *cmd)
{
	int ret;

	if (XR_OS_ThreadIsValid(&g_thread)) {
		CMD_ERR("Airkiss is already start\n");
		ret = -1;
	} else {
		ret = cmd_airkiss_start();
	}

	return (ret == 0 ? CMD_STATUS_OK : CMD_STATUS_FAIL);
}

enum cmd_status cmd_airkiss_stop_exec(char *cmd)
{
	int ret;

	ret = cmd_airkiss_stop();
	ak_key_used = 0;

	return (ret == 0 ? CMD_STATUS_OK : CMD_STATUS_FAIL);
}

enum cmd_status cmd_airkiss_set_key_exec(char *cmd)
{
	if (cmd[0] != '\0') {
		if (cmd_strlen(cmd) == sizeof(airkiss_key) - 1) {
			cmd_memcpy(airkiss_key, cmd, sizeof(airkiss_key));
			ak_key_used = 1;
		} else {
			CMD_ERR("invalid argument '%s',len:%d\n", cmd, cmd_strlen(cmd));
			return CMD_STATUS_INVALID_ARG;
		}
	} else {
		ak_key_used = 1;
	}
	CMD_DBG("Airkiss set key: %s\n", airkiss_key);

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_airkiss_help_exec(char *cmd);

#if CMD_HELP_INFO_PRINTF
#define CMD_AIRKISS_HELP_INFO \
"[*] start\t\t: start airkiss\n" \
"[*] stop\t\t: stop airkiss\n" \
"[*] set_key\t\t: set airkiss key\n" \
"[*] help\t\t: print this message and quit\n"
#endif

static const struct cmd_data g_airkiss_cmds[] = {
    { "start",		cmd_airkiss_start_exec, CMD_DESC("start airkiss") },
    { "stop",		cmd_airkiss_stop_exec, CMD_DESC("stop airkiss") },
    { "set_key",	cmd_airkiss_set_key_exec, CMD_DESC("set airkiss key") },
    { "help",	    cmd_airkiss_help_exec, CMD_DESC(CMD_HELP_DESC) },
};

static enum cmd_status cmd_airkiss_help_exec(char *cmd)
{
#if CMD_HELP_INFO_PRINTF
	printf("%s", CMD_AIRKISS_HELP_INFO);
#else
	return cmd_help_exec(g_airkiss_cmds, cmd_nitems(g_airkiss_cmds), 8);
#endif
}

enum cmd_status cmd_airkiss_exec(char *cmd)
{
	if (g_wlan_netif == NULL) {
		CMD_ERR("g_wlan_netif is NULL, need to open wifi first\n");
		return CMD_STATUS_FAIL;
	}

	return cmd_exec(cmd, g_airkiss_cmds, cmd_nitems(g_airkiss_cmds));
}

