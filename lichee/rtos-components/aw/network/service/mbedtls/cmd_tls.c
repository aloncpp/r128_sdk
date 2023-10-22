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
#include "cmd_tls.h"
#include "tls.h"
#include <console.h>

struct config_exec_arg {
	char     cmd_str[5];
	int      (*handler)(mbedtls_test_param *param, void *var);
	int      config;
};

int count_one(unsigned int P)
{
	int n = 0;
	while (P != 0) {
		if ((P & 0x1) != 0) {
			n++;
		}
		P = P>>1;
	}
	return n;
}

static int set_port(mbedtls_test_param *param, void *arg)
{
	char *dest = arg;
	if (dest != NULL) {
		if (cmd_strlen(dest) > (sizeof(param->server_port) - 1)) {
			CMD_ERR("invalid param : port.\n");
			return -1;
		}
		cmd_memcpy(param->server_port, dest, cmd_strlen(dest));
		param->flags |= MBEDTLS_SSL_FLAG_SERVER_PORT;
		param->server_port[cmd_strlen(dest)] = '\0';
	}

	return 0;
}

static int set_servername(mbedtls_test_param *param, void *arg)
{
	char *dest = arg;
	if (dest != NULL) {
		if (cmd_strlen(dest) > (sizeof(param->server_name) - 1)) {
			CMD_ERR("invalid param : server_name.\n");
			return -1;
		}
		cmd_memcpy(param->server_name, dest, cmd_strlen(dest));
		param->flags |= MBEDTLS_SSL_FLAG_SERVER_NAME;
		param->server_name[cmd_strlen(dest)] = '\0';
	}

	return 0;
}

static int set_timer(mbedtls_test_param *param, void *arg)
{
	char *dest = arg;

	if (*dest) {
		param->continue_ms = cmd_atoi(dest);
		param->flags |= MBEDTLS_SSL_FLAG_CONTINUE;
	}

	return 0;
}

static int set_client(mbedtls_test_param *param, void *arg)
{
	param->flags |= MBEDTLS_SSL_FLAG_CLINET;

	return 0;
}

static int set_server(mbedtls_test_param *param, void *arg)
{
	param->flags |= MBEDTLS_SSL_FLAG_SERVER;

	return 0;
}
#if 0
static int set_webserver(mbedtls_test_param *param, void *arg)
{
	param->flags |= MBEDTLS_SSL_FLAG_WEBSERVER;

	return 0;
}

static int set_webclient(mbedtls_test_param *param, void *arg)
{
	param->flags |= MBEDTLS_SSL_FLAG_WEBCLIENT;

	return 0;
}
#endif

#if CMD_DESCRIBE
#define tls_help_info \
	"[*] -p <port> : set port, must be the same as the server port\n"\
	"[*] -t <time> : set test time(ms)\n"\
	"[*] -c : set to client mode\n"\
	"[*] -n : set server host name, or server ip\n"\
	"[*] -s : set to server mode"
#endif /* CMD_DESCRIBE */

static int print_help_info(void *arg)
{
#if CMD_DESCRIBE
	CMD_LOG(1, "%s\n", tls_help_info);
#endif
	return CMD_STATUS_ACKED;
}

static const struct config_exec_arg keywords[] = {

	{"-p",  set_port,       1},
	{"-n",  set_servername, 1},
	{"-t",  set_timer,      1},
	{"-c",  set_client,     0},
	{"-s",  set_server,     0},
	//{"-ws", set_webserver,  0},
	//{"-wc", set_webclient,  0},
};

enum cmd_status cmd_tls_exec(char *cmd)
{
	int argc, i = 0, j = 0;
	char *argv[12];
	unsigned int flags = 0;
	mbedtls_test_param param;

	memset(&param, 0, sizeof(mbedtls_test_param));
	argc = cmd_parse_argv(cmd, argv, 12);

	if (cmd_strcmp(cmd, "help") == 0 || cmd_strcmp(cmd, "-h") == 0) {
		print_help_info(cmd);
		return CMD_STATUS_ACKED;
	}

	for (j = 0; j < argc; ++j) {
		 for (i = 0; i < cmd_nitems(keywords); i++) {
			if (cmd_strcmp(argv[j], keywords[i].cmd_str) == 0) {
				if (keywords[i].config == 1)
					j++;
				keywords[i].handler(&param, argv[j]);
				break;
			}
		}
	}

	flags = MBEDTLS_SSL_FLAG_CLINET | MBEDTLS_SSL_FLAG_WEBCLIENT;

	if ((param.flags & flags) != 0) {
		if ((param.flags & MBEDTLS_SSL_FLAG_SERVER_NAME) == 0) {
			CMD_ERR("invalid tls cmd.\n");
			return CMD_STATUS_INVALID_ARG;
		}
	}
	flags = MBEDTLS_SSL_FLAG_CLINET | MBEDTLS_SSL_FLAG_SERVER |    \
	    MBEDTLS_SSL_FLAG_WEBSERVER | MBEDTLS_SSL_FLAG_WEBCLIENT;

	flags = param.flags & flags;
	if (count_one(flags) > 1 || count_one(flags) == 0) {
		CMD_ERR("invalid tls cmd.\n");
		return CMD_STATUS_INVALID_ARG;
	}

	if (tls_start(&param) == 0)
		return CMD_STATUS_OK;
	else
		return CMD_STATUS_FAIL;
}

static void tls_exec(int argc, char *argv[])
{
	cmd2_main_exec(argc, argv, cmd_tls_exec);
}

FINSH_FUNCTION_EXPORT_CMD(tls_exec, tls, tls testcmd);
