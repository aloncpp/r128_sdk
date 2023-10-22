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
#include "cmd_shttpd.h"
#include <console.h>

#define HTTPD_THREAD_STACK_SIZE           (4 * 1024)

static XR_OS_Thread_t g_httpd_thread;

extern int webserver_start(int argc, char *argv[]);
extern void webserver_stop();

#if CMD_DESCRIBE
#define httpd_help_info \
"[*] net httpd 1 : start httpd test.\n"\
"[*] net httpd 0 : stop httpd test."
#endif /* CMD_DESCRIBE */

static enum cmd_status cmd_httpd_help_exec(char *cmd)
{
#if CMD_DESCRIBE
	CMD_LOG(1, "%s\n", httpd_help_info);
#endif
	return CMD_STATUS_ACKED;
}

static void httpd_run(void *arg)
{
	webserver_start(0, NULL);
	XR_OS_ThreadDelete(&g_httpd_thread);
}

static int httpd_start()
{
	if (XR_OS_ThreadIsValid(&g_httpd_thread)) {
		CMD_ERR("HTTPD task is running\n");
		return -1;
	}

	if (XR_OS_ThreadCreate(&g_httpd_thread,
	                       "httpd",
	                       httpd_run,
	                       NULL,
	                       XR_OS_THREAD_PRIO_APP,
	                       HTTPD_THREAD_STACK_SIZE) != XR_OS_OK) {
		CMD_ERR("httpd task create failed\n");
		return -1;
	}

	return 0;
}

enum cmd_status cmd_httpd_exec(char *cmd)
{
	int argc;
	char *argv[3];

	if (cmd_strcmp(cmd, "help") == 0) {
		cmd_httpd_help_exec(cmd);
		return CMD_STATUS_ACKED;
	}

	argc = cmd_parse_argv(cmd, argv, 3);
	if (argc < 1) {
		CMD_ERR("invalid httpd cmd, argc %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}
	int enable = atoi(argv[0]);
	if (enable == 1)
		httpd_start();
	else
		webserver_stop();

	return CMD_STATUS_OK;
}

static void shttpd_exec(int argc, char *argv[])
{
	cmd2_main_exec(argc, argv, cmd_httpd_exec);
}

FINSH_FUNCTION_EXPORT_CMD(shttpd_exec, shttpd, shttpd testcmd);
