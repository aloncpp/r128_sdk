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

#ifndef __TINY_CMD_SHELL__
#define __TINY_CMD_SHELL__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct shell {
};

struct shell_static_args {
	int mandatory; /*!< Number of mandatory arguments. */
	int optional;  /*!< Number of optional arguments. */
};

// typedef int (*shell_cmd_handler)(int argc, char **argv);
typedef int (*shell_cmd_handler)(const struct shell *shell, int argc, char **argv);

struct shell_cmd_entry {
};

struct shell_static_entry {
	const char *syntax;
	const char *help;
	const struct shell_cmd_entry *subcmd;
	shell_cmd_handler handler;
	struct shell_static_args args;
};

#define SHELL_STATIC_SUBCMD_SET_CREATE(name, ...)			\
	static const struct shell_static_entry shell_##name[] = {	\
		__VA_ARGS__						\
	}

#define SHELL_EXPR_CMD_ARG(_expr, _syntax, _subcmd, _help, _handler, \
			   _mand, _opt) \
	{ \
		.syntax = (_expr) ? (const char *)STRINGIFY(_syntax) : "", \
		.help  = (_expr) ? (const char *)(_help ? _help : "") : NULL, \
		.subcmd = (const struct shell_cmd_entry *)((_expr) ? \
				_subcmd : NULL), \
		.handler = (shell_cmd_handler)((_expr) ? _handler : NULL), \
		.args = { .mandatory = _mand, .optional = _opt} \
	}

#define SHELL_CMD_ARG(syntax, subcmd, help, handler, mand, opt) \
	SHELL_EXPR_CMD_ARG(1, syntax, subcmd, help, handler, mand, opt)

#define SHELL_SUBCMD_SET_END {NULL}

#define SHELL_CMD_ARG_REGISTER(a,b,c,d,e,f)

#define SHELL_CMD_REGISTER(syntax, subcmd, help, handler) \
	SHELL_CMD_ARG_REGISTER(syntax, subcmd, help, handler, 0, 0)

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_DBG_ON  1
#define CMD_WRN_ON  1
#define CMD_ERR_ON  1

#define CMD_SYSLOG  printf

#define CMD_LOG(flags, fmt, arg...) \
    do {                            \
        if (flags)                  \
            CMD_SYSLOG(fmt, ##arg); \
    } while (0)

#define CMD_DBG(fmt, arg...) \
    CMD_LOG(CMD_DBG_ON, "[cmd] "fmt, ##arg)

#define CMD_WRN(fmt, arg...) \
    CMD_LOG(CMD_WRN_ON, "[cmd WRN] "fmt, ##arg)

#define CMD_ERR(fmt, arg...) \
    CMD_LOG(CMD_ERR_ON, "[cmd ERR] %s():%d, "fmt, __func__, __LINE__, ##arg)

#define shell_error(shell, fmt, arg...) (void)shell; CMD_ERR(fmt"\n", ##arg)
#define shell_print(shell, fmt, arg...) (void)shell; CMD_DBG(fmt"\n", ##arg)
#define shell_hexdump(shell, data, len) (void)shell; print_hex_dump_bytes(data, len)
#define shell_help(shell) (void)shell; printf("help\n")
#define cmd_strcmp strcmp

#ifndef EINVAL
#define EINVAL 22
#endif

static inline void print_hex_dump_bytes(const void *addr, size_t len)
{
	unsigned int i;
	const unsigned char *add = addr;

	if ((unsigned int)add & 0x0f)
		printf("[%p]: ", add);
	for (i = 0; i < len; ++i) {
		if (((unsigned int)add & 0x0f) == 0x0) {
			printf("\n[%p]: ", add);
		}
		printf("%02x ", *add++);
	}
	printf("\n");
}

#define SHELL_CMD_HELP_PRINTED 0
#define cmd_strncmp strncmp
#define cmd_sscanf sscanf

#ifdef __cplusplus
}
#endif

#endif //__TINY_CMD_SHELL__