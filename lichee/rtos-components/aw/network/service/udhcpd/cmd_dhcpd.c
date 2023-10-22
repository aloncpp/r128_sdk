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
#include "cmd_dhcpd.h"
#include "usr_dhcpd.h"
#include "lwip/inet.h"
#include "console.h"

#define CMD_DHCPD_ADDR_START "192.168.51.100"
#define CMD_DHCPD_ADDR_END   "192.168.51.150"
#define CMD_DHCPD_LEASE_TIME (60 * 60 * 12)
#define CMD_DHCPD_MAX_LEASE  5

static struct dhcp_server_info dhcpd_info;

static enum cmd_status dhcpd_start_exec(char *cmd)
{
	if (dhcpd_info.addr_start == 0)
		dhcpd_info.addr_start = inet_addr(CMD_DHCPD_ADDR_START);
	if (dhcpd_info.addr_end == 0)
		dhcpd_info.addr_end = inet_addr(CMD_DHCPD_ADDR_END);
	if (dhcpd_info.lease_time == 0)
		dhcpd_info.lease_time = CMD_DHCPD_LEASE_TIME;
	if (dhcpd_info.max_leases == 0)
		dhcpd_info.max_leases = CMD_DHCPD_MAX_LEASE;

	dhcp_server_start(&dhcpd_info);
	return CMD_STATUS_OK;
}

static enum cmd_status dhcpd_stop_exec(char *cmd)
{
	dhcp_server_stop();
	return CMD_STATUS_OK;
}

static enum cmd_status dhcpd_set_ippool_exec(char *cmd)
{
	int argc;
	char *argv[2];
	ip_addr_t ip_addr_start;
	ip_addr_t ip_addr_end;

	argc = cmd_parse_argv(cmd, argv, cmd_nitems(argv));
	if (argc != 2) {
		CMD_ERR("invalid dhcp cmd, argc %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}
	if (inet_aton(argv[0], &ip_addr_start) == 0 ||
		inet_aton(argv[1], &ip_addr_end) == 0) {
		CMD_ERR("invalid dhcp cmd <%s %s>\n", argv[0], argv[1]);
		return CMD_STATUS_INVALID_ARG;
	}

#ifdef CONFIG_LWIP_V1
	dhcpd_info.addr_start = ip4_addr_get_u32(&ip_addr_start);
	dhcpd_info.addr_end   = ip4_addr_get_u32(&ip_addr_end);
#elif LWIP_IPV4 /* now only for IPv4 */
	dhcpd_info.addr_start = ip4_addr_get_u32(ip_2_ip4(&ip_addr_start));
	dhcpd_info.addr_end   = ip4_addr_get_u32(ip_2_ip4(&ip_addr_end));
#else
	#error "IPv4 not support!"
#endif

	return CMD_STATUS_OK;
}

static enum cmd_status dhcpd_set_max_leases_exec(char *cmd)
{
	int argc;
	char *argv[1];
	int leases;

	argc = cmd_parse_argv(cmd, argv, cmd_nitems(argv));
	if (argc != 1) {
		CMD_ERR("invalid dhcp cmd, argc %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	leases = cmd_atoi(argv[0]);
	if (leases < 0 || leases > 254) {
		CMD_ERR("invalid dhcp cmd, leases=%s", argv[0]);
		return CMD_STATUS_INVALID_ARG;
	}

	dhcpd_info.max_leases = leases;
	return CMD_STATUS_OK;
}

static enum cmd_status dhcpd_set_lease_time_exec(char *cmd)
{
	int argc;
	char *argv[1];
	int lease_time;

	argc = cmd_parse_argv(cmd, argv, cmd_nitems(argv));
	if (argc != 1) {
		CMD_ERR("invalid dhcp cmd, argc %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}
	lease_time = cmd_atoi(argv[0]);
	if (lease_time < 60) {
		CMD_ERR("leasetime must greater than 60\n");
		return CMD_STATUS_INVALID_ARG;
	}

	dhcpd_info.lease_time = lease_time;
	return CMD_STATUS_OK;
}

#if CMD_DESCRIBE
#define dhcpd_start_help_info "start the dhcpd server."
#define dhcpd_stop_help_info  "stop the dhcpd server."
#define dhcpd_ippool_help_info "set the dhcpd server ippool, ippool <ip-addr-start> <ip-addr-end>."
#define dhcpd_set_max_leases_help_info "set the set max leases of dhcpd server, leases <lease-num>."
#define dhcpd_set_lease_time_help_info "set the set leases time of dhcpd server, leasetime <sec>."
#endif

/*
 * dhcp commands
 */
static enum cmd_status cmd_dhcpd_help_exec(char *cmd);

static const struct cmd_data g_dhcpd_cmds[] = {
	{ "start",      dhcpd_start_exec,          CMD_DESC(dhcpd_start_help_info) },
	{ "stop",       dhcpd_stop_exec,           CMD_DESC(dhcpd_stop_help_info) },
	{ "ippool",     dhcpd_set_ippool_exec,     CMD_DESC(dhcpd_ippool_help_info) },
	{ "leases",     dhcpd_set_max_leases_exec, CMD_DESC(dhcpd_set_max_leases_help_info) },
	{ "leasetime",  dhcpd_set_lease_time_exec, CMD_DESC(dhcpd_set_lease_time_help_info) },
	{ "help",       cmd_dhcpd_help_exec,       CMD_DESC(CMD_HELP_DESC) },
};

static enum cmd_status cmd_dhcpd_help_exec(char *cmd)
{
	return cmd_help_exec(g_dhcpd_cmds, cmd_nitems(g_dhcpd_cmds), 8);
}

enum cmd_status cmd_dhcpd_exec(char *cmd)
{
	return cmd_exec(cmd, g_dhcpd_cmds, cmd_nitems(g_dhcpd_cmds));
}
static void dhcpd_exec(int argc, char *argv[])
{
	cmd2_main_exec(argc, argv, cmd_dhcpd_exec);
}

FINSH_FUNCTION_EXPORT_CMD(dhcpd_exec, dhcpd, dhcpd testcmd);
