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

#include "sys_ctrl/sys_ctrl.h"
#include "net_ctrl/net_ctrl.h"
#include "net_ctrl/net_init.h"
#include "soft_ap_config.h"
#include "kernel/os/os.h"
#include "cmd_softap.h"

static char *softap_ssid = "XRADIO_SOFT_AP_CONFIG_TEST";
static soft_ap_config_result soft_ap_result;
static SOFT_AP_CONFIG_STA soft_ap_state;

static void soft_ap_config_callback(soft_ap_config_result *result,
                                         SOFT_AP_CONFIG_STA state)
{
	/* copy the result and state */
	memcpy(&soft_ap_result, result, sizeof(soft_ap_result));
	soft_ap_state = state;

	printf("ssid:%s psk:%s state:%d\n", result->ssid, result->psk, state);
}

int soft_ap_config_main(void)
{
	int soft_ap_has_start = 0;

	/* set to ap mode */
	xr_wlan_on(WLAN_MODE_HOSTAP);

	wlan_ap_disable();
	wlan_ap_set((unsigned char *)softap_ssid, strlen(softap_ssid), NULL);
	wlan_ap_enable();

	/* set soft_ap_config callback */
	soft_ap_config_set_cb(soft_ap_config_callback);
	struct netif *nif = g_wlan_netif;
	while (1) {
		if (NETIF_IS_AVAILABLE(nif) && !soft_ap_has_start) {
			/* if the network is up, start the soft_ap_config */
			soft_ap_config_start();
			soft_ap_has_start = 1;
		}

		XR_OS_MSleep(100);
	}

	return 0;
}

enum cmd_status cmd_softap_start_exec(char *cmd)
{
	int ret;

	ret = soft_ap_config_main();

	return (ret == 0 ? CMD_STATUS_OK : CMD_STATUS_FAIL);
}

static const struct cmd_data g_softap_cmds[] = {
    { "start",		cmd_softap_start_exec, CMD_DESC("start softap") }
};

static enum cmd_status cmd_softap_help_exec(char *cmd)
{
	return cmd_help_exec(g_softap_cmds, cmd_nitems(g_softap_cmds), 8);
}

enum cmd_status cmd_softap_exec(char *cmd)
{
	if (g_wlan_netif == NULL) {
		CMD_ERR("g_wlan_netif is NULL, need to open wifi first\n");
		return CMD_STATUS_FAIL;
	}

	return cmd_exec(cmd, g_softap_cmds, cmd_nitems(g_softap_cmds));
}

