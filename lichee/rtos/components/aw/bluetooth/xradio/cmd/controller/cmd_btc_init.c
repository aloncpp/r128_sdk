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
#include "cmd_util.h"

extern int xrbtc_cmd_exec(uint8_t * pCmd);

#define BTC_CMD_BUF_SIZE    128

static char btc_cmd_buf[BTC_CMD_BUF_SIZE];

static void btc_exec(int argc, char *argv[])
{
	int i, len;
	int left = BTC_CMD_BUF_SIZE;
	char *ptr = btc_cmd_buf;

	*ptr = '\0';
	for (i = 1; i < argc && left >= 2; ++i) {
		len = cmd_strlcpy(ptr, argv[i], left);
		ptr += len;
		left -= len;
		if (i < argc - 1 && left >= 2) {
			*ptr++ = ' ';
			*ptr = '\0';
			left -= 1;
		}
	}
	*ptr = '\0';

	xrbtc_cmd_exec(btc_cmd_buf);
}

FINSH_FUNCTION_EXPORT_CMD(btc_exec, xbtc, bluetooth controller command);


