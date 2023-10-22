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

#include <stdbool.h>

#include "cmd_util.h"
#include "cmd_wlan.h"
#include "cmd_ifconfig.h"
#ifdef CONFIG_XRADIO
#include "cmd_iperf.h"
#endif
#include "cmd_lmac.h"
#include "cmd_wlancmd.h"
#include <console.h>
#ifdef CONFIG_ETF
#include "cmd_etf.h"
#endif
#include "net_init.h"

/*
 * net commands
 */
static const struct cmd_data g_net_cmds[] = {
#ifndef CONFIG_ETF
	{ "mode",		cmd_wlan_mode_exec },
#ifdef CONFIG_WLAN_AP
	{ "ap", 		cmd_wlan_ap_exec },
#endif
#ifdef CONFIG_WLAN_STA
	{ "sta",		cmd_wlan_sta_exec },
#endif
#ifdef CONFIG_WLAN_MONITOR
	{ "monitor",	cmd_wlan_mon_exec },
#endif
	{ "ifconfig",	cmd_ifconfig_exec },
	{ "wlan",		cmd_wlan_exec },
#endif
};

static enum cmd_status cmd_net_exec(char *cmd)
{
	return cmd_exec(cmd, g_net_cmds, cmd_nitems(g_net_cmds));
}

static void xradio_net_exec(int argc, char *argv[])
{
	if ((wlan_get_init_status() == WLAN_STATUS_NO_INIT)) {
		net_core_init();
		wlan_set_init_status(WLAN_STATUS_INITED);
	}

	if (argc <= 1) {
		CMD_WRN("please input parameter.\n");
		return;
	}
	cmd2_main_exec(argc, argv, cmd_net_exec);
}

FINSH_FUNCTION_EXPORT_CMD(xradio_net_exec, net, xradio testcmd);
