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
#include "HTTPCUsr_api.h"
#include "mbedtls/mbedtls.h"
#include <console.h>

#if defined(CONFIG_ARCH_SUN20IW2) && defined(CONFIG_ARCH_RISCV_RV64)
#define HTTPC_THREAD_STACK_SIZE        (10 * 1024)
#else
#define HTTPC_THREAD_STACK_SIZE        (4 * 1024)
#endif

#define CMD_HTTPC_LOG printf

struct cmd_httpc_common {
	XR_OS_Thread_t thread;
	char arg_buf[1024];
	security_client user_param;
};

static struct cmd_httpc_common cmd_httpc;

extern const char   mbedtls_test_cas_pem[];
extern const size_t mbedtls_test_cas_pem_len;

void *get_certs(void)
{
	memset(&cmd_httpc.user_param, 0, sizeof(cmd_httpc.user_param));
	cmd_httpc.user_param.pCa = (char *)mbedtls_test_cas_pem;
	cmd_httpc.user_param.nCa = mbedtls_test_cas_pem_len;
	return &cmd_httpc.user_param;
}

static unsigned char data_checksum;
static int cmd_data_process_init(void)
{
	data_checksum = 0;
	return 0;
}

static int cmd_data_process(void *buffer, int length)
{
	unsigned char *cal = (unsigned char *)buffer;
	while (length != 0) {
		data_checksum += cal[--length];
	}
	return 0;
}

static int cmd_data_process_deinit(void)
{
	CMD_HTTPC_LOG("[httpc cmd test]:checksum = %#x\n", data_checksum);
	data_checksum = 0;
	return 0;
}

static int httpc_get(char *url)
{
	int ret = -1;
	int recvSize = 0;
	char *buf = NULL;
	unsigned int toReadLength = 4096;
	HTTPParameters *clientParams = NULL;

	clientParams = malloc(sizeof(HTTPParameters));
	if (clientParams == NULL) {
		goto exit0;
	}
	memset(clientParams, 0, sizeof(HTTPParameters));

	buf = malloc(toReadLength);
	if (buf == NULL) {
		goto exit0;
	}
	memset(buf, 0, toReadLength);

	strcpy(clientParams->Uri, url);
	cmd_data_process_init();
	do {
		ret = HTTPC_get(clientParams, buf, 4096, (INT32 *)&recvSize);
		if ((ret != HTTP_CLIENT_SUCCESS) && (ret != HTTP_CLIENT_EOS)) {
			CMD_ERR("Transfer err...ret:%d\n", ret);
			ret = -1;
			break;
		}
		cmd_data_process(buf, recvSize);
		if (ret == HTTP_CLIENT_EOS) {
			CMD_DBG("The end..\n");
			ret = 0;
			break;
		}
	} while (1);
	cmd_data_process_deinit();

exit0:
	CMD_HTTPC_LOG("buf:%s\n", buf);
	free(buf);
	free(clientParams);
	return ret;
}

static int httpc_get_fresh(char *url, char *user_name, char *password,
                           int use_ssl)
{
	int ret = -1;
	char *buf = NULL;
	unsigned int toReadLength = 4096;
	unsigned int Received = 0;
	HTTP_CLIENT httpClient;
	HTTPParameters *clientParams = NULL;

	clientParams = malloc(sizeof(HTTPParameters));
	if (clientParams == NULL) {
		goto exit0;
	}
	memset(clientParams, 0, sizeof(HTTPParameters));

	buf = malloc(toReadLength);
	if (buf == NULL) {
		goto exit0;
	}
	memset(buf, 0, toReadLength);

	memset(&httpClient, 0, sizeof(httpClient));
	clientParams->HttpVerb = VerbGet;
	strcpy(clientParams->Uri, url);
	if ((user_name) && (password)) {
		cmd_strlcpy(clientParams->UserName, user_name,
		            sizeof(clientParams->UserName));
		cmd_strlcpy(clientParams->Password, password,
		            sizeof(clientParams->Password));
		clientParams->AuthType = AuthSchemaDigest;
	}

	if (use_ssl) {
		HTTPC_Register_user_certs(get_certs);
		//HTTPC_set_ssl_verify_mode(MBEDTLS_SSL_VERIFY_OPTIONAL);
	}

	ret = HTTPC_open(clientParams);
	if (ret != 0) {
		CMD_ERR("http open err..\n");
		goto exit0;
	}

	ret = HTTPC_request(clientParams, NULL);
	if (ret != 0) {
		CMD_ERR("http request err..\n");
		goto exit1;
	}

	ret = HTTPC_get_request_info(clientParams, &httpClient);
	if (ret != 0) {
		CMD_ERR("http get request info err..\n");
		goto exit1;
	}

	cmd_data_process_init();
	if (httpClient.TotalResponseBodyLength != 0) {
		do {
			ret = HTTPC_read(clientParams, buf, toReadLength,
			                 (void *)&Received);
			if ((ret != HTTP_CLIENT_SUCCESS) && (ret != HTTP_CLIENT_EOS)) {
				CMD_ERR("Transfer err...ret:%d\n", ret);
				ret = -1;
				break;
			}
			cmd_data_process(buf, Received);
			if (ret == HTTP_CLIENT_EOS) {
				CMD_DBG("The end..\n");
				ret = 0;
				break;
			}
		} while (1);
	}
	cmd_data_process_deinit();

exit1:
	HTTPC_close(clientParams);
exit0:
	free(buf);
	free(clientParams);
	return ret;
}

static int httpc_post(char *url, char *credentials, int use_ssl)
{
	int   ret = 0;
	char *buf = NULL;
	unsigned int toReadLength = 4096;
	unsigned int Received = 0;
	HTTP_CLIENT  httpClient;
	HTTPParameters *clientParams = NULL;

	clientParams = malloc(sizeof(HTTPParameters));
	if (clientParams == NULL) {
		goto exit0;
	}
	memset(clientParams, 0, sizeof(HTTPParameters));

	buf = malloc(toReadLength);
	if (buf == NULL) {
		goto exit0;
	}
	memset(buf, 0, toReadLength);

	memset(&httpClient, 0, sizeof(httpClient));
	clientParams->HttpVerb = VerbPost;
	clientParams->pData = buf;
	strcpy(clientParams->Uri, url);
	clientParams->pLength = strlen(credentials);
	memcpy(buf, credentials, strlen(credentials));

request:
	ret = HTTPC_open(clientParams);
	if (ret != 0) {
		CMD_ERR("http open err..\n");
		goto exit0;
	}

	ret = HTTPC_request(clientParams, NULL);
	if (ret != 0) {
		CMD_ERR("http request err..\n");
		goto exit1;
	}

	ret = HTTPC_get_request_info(clientParams, &httpClient);
	if (ret != 0) {
		CMD_ERR("http get request info err..\n");
		goto exit1;
	}

	if (httpClient.HTTPStatusCode != HTTP_STATUS_OK) {
		if ((httpClient.HTTPStatusCode == HTTP_STATUS_OBJECT_MOVED) ||
				(httpClient.HTTPStatusCode == HTTP_STATUS_OBJECT_MOVED_PERMANENTLY)) {
			CMD_DBG("Redirect url..\n");
			HTTPC_close(clientParams);
			memset(clientParams, 0, sizeof(*clientParams));
			clientParams->HttpVerb = VerbGet;
			if (httpClient.RedirectUrl->nLength < sizeof(clientParams->Uri)) {
				strncpy(clientParams->Uri, httpClient.RedirectUrl->pParam,
						httpClient.RedirectUrl->nLength);
			} else {
				goto exit0;
			}
			CMD_DBG("go to request.\n");
			goto request;

		} else {
			ret = -1;
			CMD_DBG("get result not correct, status: %d\n", httpClient.HTTPStatusCode);
			goto exit1;
		}
	}
	cmd_data_process_init();
	if (httpClient.TotalResponseBodyLength != 0
		|| (httpClient.HttpFlags & HTTP_CLIENT_FLAG_CHUNKED)) {
		do {
			ret = HTTPC_read(clientParams, buf, toReadLength,
			                 (void *)&Received);
			if ((ret != HTTP_CLIENT_SUCCESS) && (ret != HTTP_CLIENT_EOS)) {
				CMD_ERR("Transfer err...ret:%d\n", ret);
				ret = -1;
				break;
			}
			cmd_data_process(buf, Received);
			if (ret == HTTP_CLIENT_EOS) {
				CMD_DBG("The end..\n");
				ret = 0;
				break;
			}
		} while (1);
	}
	cmd_data_process_deinit();

exit1:
	HTTPC_close(clientParams);
exit0:
	free(buf);
	free(clientParams);
	return ret;
}

static int httpc_muti_get(char *url0, char *url1)
{
	int ret = -1;
	char *buf = NULL;
	char *pUrl0 = url0;
	char *pUrl1 = url1;
	unsigned int toReadLength = 4096;
	unsigned int Received = 0;
	HTTP_CLIENT httpClient;
	HTTPParameters *clientParams = NULL;

	clientParams = malloc(sizeof(HTTPParameters));
	if (clientParams == NULL) {
		goto exit0;
	}
	memset(clientParams, 0, sizeof(HTTPParameters));

	buf = malloc(toReadLength);
	if (buf == NULL) {
		goto exit0;
	}
	memset(buf, 0, toReadLength);

	memset(&httpClient, 0, sizeof(httpClient));
	clientParams->HttpVerb = VerbGet;
	clientParams->Flags |= HTTP_CLIENT_FLAG_KEEP_ALIVE;
	cmd_strlcpy(clientParams->Uri, pUrl0, sizeof(clientParams->Uri));

	ret = HTTPC_open(clientParams);
	if (ret != 0) {
		CMD_ERR("http open err..\n");
		goto exit0;
	}
	pUrl0 = NULL;
	goto direct_request;
set_url1:
	HTTPC_reset_session(clientParams);
	cmd_strlcpy(clientParams->Uri, pUrl1, sizeof(clientParams->Uri));
	CMD_DBG("http test get url1..\n");
	pUrl1 = NULL;
direct_request:
	ret = HTTPC_request(clientParams, NULL);
	if (ret != 0) {
		CMD_ERR("http request err..\n");
		goto exit1;
	}

	ret = HTTPC_get_request_info(clientParams, &httpClient);
	if (ret != 0) {
		CMD_ERR("http get request info err..\n");
		goto exit1;
	}

	cmd_data_process_init();
	if (httpClient.TotalResponseBodyLength != 0) {
		do {
			ret = HTTPC_read(clientParams, buf, toReadLength,
			                 (void *)&Received);
			if ((ret != HTTP_CLIENT_SUCCESS) && (ret != HTTP_CLIENT_EOS)) {
				CMD_ERR("Transfer err...ret:%d\n", ret);
				ret = -1;
				break;
			}
			cmd_data_process(buf, Received);
			if (ret == HTTP_CLIENT_EOS) {
				CMD_DBG("The end..\n");
				ret = 0;
				break;
			}
		} while (1);
	}
	cmd_data_process_deinit();

	if ((ret == 0) && (pUrl1 != NULL)) {
		goto set_url1;
	}

exit1:
	HTTPC_close(clientParams);
exit0:
	free(buf);
	free(clientParams);
	return ret;
}

static enum cmd_status cmd_httpc_get(char *cmd)
{
	int ret;

	ret = httpc_get(cmd);
	if (ret != 0) {
		return CMD_STATUS_FAIL;
	} else {
		return CMD_STATUS_OK;
	}
}

static enum cmd_status cmd_httpc_post(char *cmd)
{
	int ret;
	int argc = 0;
	char *argv[2];

	argc = cmd_parse_argv(cmd, argv, 2);
	if (argc < 2) {
		CMD_ERR("invalid httpc cmd(post), argc %d\n", argc);
		return CMD_STATUS_FAIL;
	}

	ret = httpc_post(argv[0], argv[1], 0);
	if (ret != 0) {
		return CMD_STATUS_FAIL;
	} else {
		return CMD_STATUS_OK;
	}
}

static enum cmd_status cmd_httpc_get_fresh(char *cmd)
{
	int ret;

	ret = httpc_get_fresh(cmd, NULL, NULL, 0);
	if (ret != 0) {
		return CMD_STATUS_FAIL;
	} else {
		return CMD_STATUS_OK;
	}
}

static enum cmd_status cmd_httpc_head(char *cmd)
{
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_httpc_auth_get(char *cmd)
{
	int ret;
	int argc = 0;
	char *argv[3];

	argc = cmd_parse_argv(cmd, argv, 3);
	if (argc < 3) {
		CMD_ERR("invalid httpc cmd(auth-get), argc %d\n", argc);
		return CMD_STATUS_FAIL;
	}

	ret = httpc_get_fresh(argv[0], argv[1], argv[2], 0);
	if (ret != 0) {
		return CMD_STATUS_FAIL;
	} else {
		return CMD_STATUS_OK;
	}
}

static enum cmd_status cmd_httpc_ssl_get(char *cmd)
{
	int ret;

	ret = httpc_get_fresh(cmd, NULL, NULL, 1);
	if (ret != 0) {
		return CMD_STATUS_FAIL;
	} else {
		return CMD_STATUS_OK;
	}
}

static enum cmd_status cmd_httpc_ssl_post(char *cmd)
{
	int ret;
	int argc = 0;
	char *argv[2];

	argc = cmd_parse_argv(cmd, argv, 2);
	if (argc < 2) {
		CMD_ERR("invalid httpc cmd(post), argc %d\n", argc);
		return CMD_STATUS_FAIL;
	}

	ret = httpc_post(argv[0], argv[1], 1);
	if (ret != 0) {
		return CMD_STATUS_FAIL;
	} else {
		return CMD_STATUS_OK;
	}
}

static enum cmd_status cmd_httpc_multi_get(char *cmd)
{
	int ret;
	int argc = 0;
	char *argv[2];

	argc = cmd_parse_argv(cmd, argv, 2);
	if (argc < 2) {
		CMD_ERR("invalid httpc cmd(multi-get), argc %d\n", argc);
		return CMD_STATUS_FAIL;
	}

	ret = httpc_muti_get(argv[0], argv[1]);
	if (ret != 0) {
		return CMD_STATUS_FAIL;
	} else {
		return CMD_STATUS_OK;
	}
}

static enum cmd_status cmd_httpc_multi_post(char *cmd)
{
	return CMD_STATUS_OK;
}

static const struct cmd_data g_httpc_cmds[] = {
	{ "get",        cmd_httpc_get },
	{ "post",       cmd_httpc_post },
	{ "-get",       cmd_httpc_get_fresh },
	{ "head",       cmd_httpc_head },
	{ "auth-get",   cmd_httpc_auth_get },
	{ "ssl-get",    cmd_httpc_ssl_get },
	{ "ssl-post",   cmd_httpc_ssl_post },
	{ "multi-get",  cmd_httpc_multi_get },
	{ "multi-post", cmd_httpc_multi_post },
};

void httpc_cmd_task(void *arg)
{
	enum cmd_status status;
	char *cmd = (char *)arg;

	CMD_LOG(1, "<net> <httpc> <request> <cmd : %s>\n", cmd);
	status = cmd_exec(cmd, g_httpc_cmds, cmd_nitems(g_httpc_cmds));
	if (status != CMD_STATUS_OK)
		CMD_LOG(1, "<net> <httpc> <response : fail> <%s>\n", cmd);
	else {
		CMD_LOG(1, "<net> <httpc> <response : success> <%s>\n", cmd);
	}
	XR_OS_ThreadDelete(&cmd_httpc.thread);
}

#if CMD_DESCRIBE
#define httpc_help_info \
"[*] net httpc <cmd> <url> file=<file_test> <user> <password>\n"\
"[*] cmd: {get | -get | post | auth-get | ssl-get | ssl-post}\n"\
"[*]    get: get method\n"\
"[*]    -get: same as get method, but use new interfaces\n"\
"[*]    post: post method\n"\
"[*]    auth-get: get method with login, at this time <user> and <password> are valid, otherwise, this parameter need not be written\n"\
"[*]    ssl-get: get method with ssl\n"\
"[*]    ssl-post: post method with ssl\n"\
"[*] url: server url\n"\
"[*] file_test: only used in post method and represents the file sent to the server\n"\
"[*] user: username, used only in auth-get method\n"\
"[*] password: password, used only in auth-get method"
#endif /* CMD_DESCRIBE */

static enum cmd_status cmd_httpc_help_exec(char *cmd)
{
#if CMD_DESCRIBE
	CMD_LOG(1, "%s\n", httpc_help_info);
#endif
	return CMD_STATUS_ACKED;
}

enum cmd_status cmd_httpc_exec(char *cmd)
{
	if (cmd_strcmp(cmd, "help") == 0) {
		cmd_write_respond(CMD_STATUS_OK, "OK");
		return cmd_httpc_help_exec(cmd);
	}

	if (XR_OS_ThreadIsValid(&cmd_httpc.thread)) {
		CMD_ERR("httpc task is running\n");
		return CMD_STATUS_FAIL;
	}

	memset(cmd_httpc.arg_buf, 0, sizeof(cmd_httpc.arg_buf));
	cmd_strlcpy(cmd_httpc.arg_buf, cmd, sizeof(cmd_httpc.arg_buf));

	if (XR_OS_ThreadCreate(&cmd_httpc.thread,
	                    "httpc",
	                    httpc_cmd_task,
	                    (void *)cmd_httpc.arg_buf,
	                    XR_OS_THREAD_PRIO_APP,
	                    HTTPC_THREAD_STACK_SIZE) != XR_OS_OK) {
		CMD_ERR("httpc task create failed\n");
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

static void httpc_exec(int argc, char *argv[])
{
	cmd2_main_exec(argc, argv, cmd_httpc_exec);
}

FINSH_FUNCTION_EXPORT_CMD(httpc_exec, httpc, httpc testcmd);
