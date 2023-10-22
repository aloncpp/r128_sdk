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

#ifndef _CMD_SHELL_H_
#define _CMD_SHELL_H_

#include "cmd_util.h"
#include "errno.h"
//#include "zephyr/types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct shell {
};


struct shell_static_args {
	uint8_t mandatory; /*!< Number of mandatory arguments. */
	uint8_t optional;  /*!< Number of optional arguments. */
};

/**
 * @brief Shell command handler prototype.
 *
 * @param shell Shell instance.
 * @param argc  Arguments count.
 * @param argv  Arguments.
 *
 * @retval 0 Successful command execution.
 * @retval 1 Help printed and command not executed.
 * @retval -EINVAL Argument validation failed.
 * @retval -ENOEXEC Command not executed.
 */
typedef int (*shell_cmd_handler)(const struct shell *shell,
				 int argc, char **argv);

struct shell_cmd_entry {
};

/*
 * @brief Shell static command descriptor.
 */
struct shell_static_entry {
	const char *syntax;			/*!< Command syntax strings. */
	const char *help;			/*!< Command help string. */
	const struct shell_cmd_entry *subcmd;	/*!< Pointer to subcommand. */
	shell_cmd_handler handler;		/*!< Command handler. */
	struct shell_static_args args;		/*!< Command arguments. */
};

/**
 * @brief Initializes a conditional shell command with arguments if expression
 *	  gives non-zero result at compile time.
 *
 * @see SHELL_CMD_ARG. Based on the expression, creates a valid entry or an
 * empty command which is ignored by the shell. It should be used instead of
 * @ref SHELL_COND_CMD_ARG if condition is not a single configuration flag,
 * e.g.:
 * SHELL_EXPR_CMD_ARG(IS_ENABLED(CONFIG_FOO) &&
 *		      IS_ENABLED(CONFIG_FOO_SETTING_1), ...)
 *
 * @param[in] _expr	 Expression.
 * @param[in] _syntax	 Command syntax (for example: history).
 * @param[in] _subcmd	 Pointer to a subcommands array.
 * @param[in] _help	 Pointer to a command help string.
 * @param[in] _handler	 Pointer to a function handler.
 * @param[in] _mand	 Number of mandatory arguments.
 * @param[in] _opt	 Number of optional arguments.
 */
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

/**
 * @brief Initializes a shell command with arguments.
 *
 * @note If a command will be called with wrong number of arguments shell will
 * print an error message and command handler will not be called.
 *
 * @param[in] syntax	 Command syntax (for example: history).
 * @param[in] subcmd	 Pointer to a subcommands array.
 * @param[in] help	 Pointer to a command help string.
 * @param[in] handler	 Pointer to a function handler.
 * @param[in] mand	 Number of mandatory arguments.
 * @param[in] opt	 Number of optional arguments.
 */
#define SHELL_CMD_ARG(syntax, subcmd, help, handler, mand, opt) \
	SHELL_EXPR_CMD_ARG(1, syntax, subcmd, help, handler, mand, opt)

/**
 * @brief Initializes a shell command.
 *
 * @param[in] _syntax	Command syntax (for example: history).
 * @param[in] _subcmd	Pointer to a subcommands array.
 * @param[in] _help	Pointer to a command help string.
 * @param[in] _handler	Pointer to a function handler.
 */
#define SHELL_CMD(_syntax, _subcmd, _help, _handler) \
	SHELL_CMD_ARG(_syntax, _subcmd, _help, _handler, 0, 0)

/**
 * @brief Define ending subcommands set.
 *
 */
#define SHELL_SUBCMD_SET_END {NULL}

/**
 * @brief Macro for creating a subcommand set. It must be used outside of any
 * function body.
 *
 * Example usage:
 * SHELL_STATIC_SUBCMD_SET_CREATE(
 *	foo,
 *	SHELL_CMD(abc, ...),
 *	SHELL_CMD(def, ...),
 *	SHELL_SUBCMD_SET_END
 * )
 *
 * @param[in] name	Name of the subcommand set.
 * @param[in] ...	List of commands created with @ref SHELL_CMD_ARG or
 *			or @ref SHELL_CMD
 */
#define SHELL_STATIC_SUBCMD_SET_CREATE(name, ...)			\
	static const struct shell_static_entry shell_##name[] = {	\
		__VA_ARGS__						\
	}


#define SHELL_CMD_ARG_REGISTER(a,b,c,d,e,f)

#define SHELL_CMD_REGISTER(syntax, subcmd, help, handler) \
	SHELL_CMD_ARG_REGISTER(syntax, subcmd, help, handler, 0, 0)

#define ARGC_MAX (12)

#define SHELL_CMD_HELP_PRINTED (1)

/* cmd format: <command-name> <arg>... */
static enum cmd_status cmd_exec_shell(char *cmd, const struct shell_static_entry *cdata, int count)
{
	int i;
//	char *args;
	const struct shell_static_entry *cdata1 = cdata;

//	args = cmd_strchr(cmd, ' ');
//	if (args) {
//		*args++ = '\0'; /* has arguments */
//	}

	int argc;
	char *argv[ARGC_MAX];
	argc = cmd_parse_argv(cmd, argv, ARGC_MAX);

	for (i = 0; i < count; ++i, ++cdata) {
		if (cmd_strcmp(cmd, cdata->syntax) == 0) {
			enum cmd_status ret = -1;
			if ((argc > (cdata->args.mandatory + cdata->args.optional))
				|| (argc < cdata->args.mandatory)) {
				CMD_ERR("invalid param number %d\n", argc);
				return CMD_STATUS_INVALID_ARG;
			}

			if (cdata->handler)
				ret = cdata->handler(NULL, argc, argv);
			else
				CMD_ERR("null handler for command %s\n", cdata1->syntax);

			if ((ret == SHELL_CMD_HELP_PRINTED) || (ret == -ENOEXEC)) {
				CMD_DBG("command arg: %s\n", cdata->help);
				return CMD_STATUS_INVALID_ARG;
			} else if (ret != 0)
				return CMD_STATUS_FAIL;
			else
				return CMD_STATUS_OK;

		}
	}

	CMD_ERR("unknown cmd '%s'\n", cmd);
	for (i = 0; i < count; ++i, ++cdata1) {
		CMD_DBG("%s %s\n", cdata1->syntax, cdata1->help ? cdata1->help : "");
	}

	return CMD_STATUS_UNKNOWN_CMD;
}

//extern void print_hex_dump_bytes(const void *addr, unsigned int len);
static inline void print_hex_dump_bytes(const void *addr, size_t len)
{
	unsigned int i;
	const unsigned char *add = addr;

	if ((unsigned int)(uintptr_t)add & 0x0f)
		printf("[%p]: ", add);
	for (i = 0; i < len; ++i) {
		if (((unsigned int)(uintptr_t)add & 0x0f) == 0x0) {
			printf("\n[%p]: ", add);
		}
		printf("%02x ", *add++);
	}
	printf("\n");
}

#define shell_error(shell, fmt, arg...) (void)shell; CMD_ERR(fmt"\n", ##arg)
#define shell_print(shell, fmt, arg...) (void)shell; CMD_DBG(fmt"\n", ##arg)
#define shell_hexdump(shell, data, len) (void)shell; print_hex_dump_bytes(data, len)
#define shell_help(shell) (void)shell;


#if 0
static int char2hex(char c, u8_t *x)
{
	if (c >= '0' && c <= '9') {
		*x = c - '0';
	} else if (c >= 'a' && c <= 'f') {
		*x = c - 'a' + 10;
	} else if (c >= 'A' && c <= 'F') {
		*x = c - 'A' + 10;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int hex2char(u8_t x, char *c)
{
	if (x <= 9) {
		*c = x + '0';
	} else  if (x >= 10 && x <= 15) {
		*c = x - 10 + 'a';
	} else {
		return -EINVAL;
	}

	return 0;
}

static size_t hex2bin(const char *hex, size_t hexlen, u8_t *buf, size_t buflen)
{
	u8_t dec;

	if (buflen < hexlen / 2 + hexlen % 2) {
		return 0;
	}

	/* if hexlen is uneven, insert leading zero nibble */
	if (hexlen % 2) {
		if (char2hex(hex[0], &dec) < 0) {
			return 0;
		}
		buf[0] = dec;
		hex++;
		buf++;
	}

	/* regular hex conversion */
	for (size_t i = 0; i < hexlen / 2; i++) {
		if (char2hex(hex[2 * i], &dec) < 0) {
			return 0;
		}
		buf[i] = dec << 4;

		if (char2hex(hex[2 * i + 1], &dec) < 0) {
			return 0;
		}
		buf[i] += dec;
	}

	return hexlen / 2 + hexlen % 2;
}

static size_t bin2hex(const u8_t *buf, size_t buflen, char *hex, size_t hexlen)
{
	if ((hexlen + 1) < buflen * 2) {
		return 0;
	}

	for (size_t i = 0; i < buflen; i++) {
		if (hex2char(buf[i] >> 4, &hex[2 * i]) < 0) {
			return 0;
		}
		if (hex2char(buf[i] & 0xf, &hex[2 * i + 1]) < 0) {
			return 0;
		}
	}

	hex[2 * buflen] = '\0';
	return 2 * buflen;
}

static int bt_addr_from_str(const char *str, bt_addr_t *addr)
{
	int i, j;
	u8_t tmp;

	if (strlen(str) != 17U) {
		return -EINVAL;
	}

	for (i = 5, j = 1; *str != '\0'; str++, j++) {
		if (!(j % 3) && (*str != ':')) {
			return -EINVAL;
		} else if (*str == ':') {
			i--;
			continue;
		}

		addr->val[i] = addr->val[i] << 4;

		if (char2hex(*str, &tmp) < 0) {
			return -EINVAL;
		}

		addr->val[i] |= tmp;
	}

	return 0;
}

static int bt_addr_le_from_str(const char *str, const char *type, bt_addr_le_t *addr)
{
	int err;

	err = bt_addr_from_str(str, &addr->a);
	if (err < 0) {
		return err;
	}

	if (!strcmp(type, "public") || !strcmp(type, "(public)")) {
		addr->type = BT_ADDR_LE_PUBLIC;
	} else if (!strcmp(type, "random") || !strcmp(type, "(random)")) {
		addr->type = BT_ADDR_LE_RANDOM;
	} else if (!strcmp(type, "public-id") || !strcmp(type, "(public-id)")) {
		addr->type = BT_ADDR_LE_PUBLIC_ID;
	} else if (!strcmp(type, "random-id") || !strcmp(type, "(random-id)")) {
		addr->type = BT_ADDR_LE_RANDOM_ID;
	} else {
		return -EINVAL;
	}

	return 0;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* _CMD_SHELL_H_ */

