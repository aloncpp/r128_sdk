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

#include <console.h>
#include <stdlib.h>
#include <string.h>
#include "iperf.h"
#if defined(CONFIG_ARCH_SUN20IW2) && !defined(CONFIG_ETF)
#include "net_ctrl/net_ctrl.h"
#else
#include "tcpip_adapter.h"
#endif

#define CMD_DESCRIBE 1

#if CMD_DESCRIBE
static const char *iperf_help_info =
"[*] -p : The server port for the server to listen on and the client to connect to.\n"
"[*] -f : A letter specifying the format to print bandwidth numbers in. Supported 'm' = Mbits/sec 'K' = KBytes/sec.\n"
"[*] -i : Sets the interval time in seconds between periodic bandwidth. Zero means one second(default).\n"
"[*] -c : Run iPerf in client mode, connecting to an iPerf server running on host.\n"
"[*] -s : Run iPerf in server mode. (This will only allow one iperf connection at a time).\n"
"[*] -u : Use UDP rather than TCP. See also the -b option.\n"
"[*] -t : The time in seconds to transmit for. Zero means forever(default).\n"
#if IPERF_OPT_BANDWIDTH
"[*] -b : Set target bandwidth to n bits/sec (default 1 Mbit/sec for UDP, unlimited for TCP).\n"
#endif
#if IPERF_OPT_TOS
"[*] -S : The type-of-service for outgoing packets.\n"
#endif
#if IPERF_OPT_NUM
"[*] -n : The number of bytes to transmit.\n"
#endif
"[*] -Q : Quit the iperf thread according to the iperf handle. Quit handl 1: -Q 1, quit all threads: -Q a\n"
"[*] -L : Show the iperf thread list.";
#endif /* CMD_DESCRIBE */

static void cmd_iperf_help_exec(char *cmd)
{
#if CMD_DESCRIBE
	printf("%s\n", iperf_help_info);
#endif
	return ;
}

int cmd_iperf(int argc, char ** argv)
{
	int  handle, ret;
	struct netif *nif = NULL;

	if (argc == 1) {
#if CMD_DESCRIBE
		printf("%s\n", iperf_help_info);
#endif
		return 0;
	}

	if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "-h") == 0) {
		cmd_iperf_help_exec(argv[1]);
		return 0;
	}

	handle = iperf_parse_argv(argc, argv);

	if (handle < 0) {
		if (handle == -2) {
			return 0;
		} else {
			printf("command exec failed,handle=%d\n", handle);
			return -1;
		}
	}

#if defined(CONFIG_ARCH_SUN20IW2) && !defined(CONFIG_ETF)
	nif = g_wlan_netif;
#else
	nif = get_netif(MODE_STA);
	if (nif == NULL)
		nif = get_netif(MODE_AP);
#endif

	if (nif == NULL || !(netif_is_up(nif) && !ip_addr_isany(&((nif)->ip_addr)))) {
		printf("net is down, iperf start failed\n");
		iperf_handle_free(handle);
		return -1;
	}

	if (handle >= 0 && handle < IPERF_ARG_HANDLE_MAX) {
		ret = iperf_handle_start(nif, handle);
		if (ret == -1)
			iperf_handle_free(handle);
	}

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_iperf, iperf, network iperf test);
