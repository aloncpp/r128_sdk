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

#include <stdio.h>
#include <string.h>
#include "tls.h"

extern void mbedtls_client(void *arg);
extern void mbedtls_server(void *arg);

static const XR_OS_ThreadEntry_t tls_thread_entry[2] = {
	mbedtls_server,
	mbedtls_client,
};

XR_OS_Thread_t g_tls_thread;
volatile int mbedtls_string_mismatch;

void tls_thread_start(void *param)
{
	printf("<net> <tls> <test>\n");
	int client = (((mbedtls_test_param *)param)->flags & (MBEDTLS_SSL_FLAG_CLINET
	              | MBEDTLS_SSL_FLAG_WEBCLIENT)) == 0 ? 0 : 1;
	mbedtls_string_mismatch = -1;
	tls_thread_entry[client](param);

	char *str = mbedtls_string_mismatch == 0 ? "success" : "fail";
	printf("<net> <tls> <response : %s>\n", str);
	if (param)
		free(param);
	tls_thread_stop();
}

int tls_thread_stop(void)
{
	if (XR_OS_ThreadIsValid(&g_tls_thread)) {
		XR_OS_ThreadDelete(&g_tls_thread);
		return 0;
	} else
		return -1;
}

int tls_start(mbedtls_test_param *param)
{
	mbedtls_test_param *tls_arg = NULL;

	if (XR_OS_ThreadIsValid(&g_tls_thread)) {
		mbedtls_printf("tls task is running\n");
		return -1;
	}

	if (param) {
		tls_arg = malloc(sizeof(mbedtls_test_param));
		if (!tls_arg) {
			mbedtls_printf("tls arg malloc err\n");
			return -1;
		}
		memcpy(tls_arg, param, sizeof(mbedtls_test_param));
	}

	if (XR_OS_ThreadCreate(&g_tls_thread,
	                    "tls",
	                    tls_thread_start,
	                    (void *)tls_arg,
	                    XR_OS_THREAD_PRIO_APP,
	                    TLS_THREAD_STACK_SIZE) != XR_OS_OK) {
		mbedtls_printf("tls task create failed\n");
		if (tls_arg)
			free(tls_arg);
		return -1;
	}
	return 0;
}
