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
#include <stdarg.h>

/* cmd format: <command-name> <arg>... */
enum cmd_status cmd_exec(char *cmd, const struct cmd_data *cdata, int count)
{
	int i;
	char *args;
	const struct cmd_data *temp_data;
	temp_data = cdata;

	args = cmd_strchr(cmd, ' ');
	if (args) {
		*args++ = '\0'; /* has arguments */
	}

	for (i = 0; i < count; ++i, ++cdata) {
		if (cmd_strcmp(cmd, cdata->name) == 0) {
			return cdata->exec(args ? args : "");
		}
	}

	CMD_ERR("unknown cmd '%s'\n", cmd);
	cmd_help_exec(temp_data, count, 20);
	return CMD_STATUS_UNKNOWN_CMD;
}

/* cmd2 format: <command-name>[ <arg>...] */
enum cmd_status cmd2_exec(char *cmd, const struct cmd2_data *cdata, int count)
{
	int i;

	for (i = 0; i < count; ++i, ++cdata) {
		if (cmd_strncmp(cmd, cdata->name, cdata->name_len) == 0) {
			return cdata->exec(cmd + cdata->name_len);
		}
	}

	CMD_ERR("unknown cmd '%s'\n", cmd);
	return CMD_STATUS_UNKNOWN_CMD;
}

enum cmd_status cmd_help_exec(const struct cmd_data *cdata, int count, int align)
{
	for (int i = 0; i < count; ++i, ++cdata) {
#if CMD_DESCRIBE
		if (cdata->desc)
			CMD_LOG(1, "[*] %-*.*s : %s\n", align, cmd_strlen(cdata->name), cdata->name, cdata->desc);
		else
#endif
		CMD_LOG(1, "[*] %s\n", cdata->name);
	}

	CMD_LOG(1, "\nFor detail please use xxx help\n");
	return CMD_STATUS_ACKED;
}

enum cmd_status cmd2_help_exec(const struct cmd2_data *cdata, int count, int align)
{
	for (int i = 0; i < count; ++i, ++cdata) {
#if CMD_DESCRIBE
		if (cdata->desc)
			CMD_LOG(1, "[*] %-*.*s : %s\n", align, cmd_strlen(cdata->name), cdata->name, cdata->desc);
		else
#endif
		CMD_LOG(1, "[*] %s\n", cdata->name);
	}

	CMD_LOG(1, "\nFor detail please use xxx help\n");
	return CMD_STATUS_ACKED;
}

enum cmd_status cmd_main_exec(char *cmd, const struct cmd_data *cdata, int count)
{
	enum cmd_status status = CMD_STATUS_OK;

	if (cmd[0] != '\0') {
#if (!CONSOLE_ECHO_EN)
		if (cmd_strcmp(cmd, "efpg"))
			CMD_LOG(CMD_DBG_ON, "$ %s\n", cmd);
#endif

		status = cmd_exec(cmd, cdata, count);
		if (status != CMD_STATUS_ACKED) {
			cmd_write_respond(status, cmd_get_status_desc(status));
			if (status == CMD_STATUS_INVALID_ARG ||
			    status == CMD_STATUS_UNKNOWN_CMD) {
				CMD_LOG(1, "Use 'help' to list available subcommands\n");
			}
		}
	}
#if (!CONSOLE_ECHO_EN)
	else { /* empty command */
		CMD_LOG(1, "$\n");
	}
#endif
#if CONSOLE_ECHO_EN
	console_write((uint8_t *)"$ ", 2);
#endif
	return status;
}

enum cmd_status cmd2_main_exec(int argc, char *argv[], enum cmd_status (*func)(char *))
{
	char *ptr;
	enum cmd_status status = CMD_STATUS_OK;
	ptr = cmd_conv_from_argv(argc, argv, 1);
	CMD_DBG("cmd: %s\n", ptr);
	if (ptr[0] != '\0') {
#if (!CONSOLE_ECHO_EN)
		if (cmd_strcmp(ptr, "efpg")) {
			CMD_LOG(CMD_DBG_ON, "$ %s\n", ptr);
		}
#endif
	status = func(ptr);
	if (status != CMD_STATUS_ACKED) {
			cmd_write_respond(status, cmd_get_status_desc(status));
			if (status == CMD_STATUS_INVALID_ARG ||
			    status == CMD_STATUS_UNKNOWN_CMD) {
				CMD_LOG(1, "Use 'help' to list available subcommands\n");
			}
		}
	}
#if (!CONSOLE_ECHO_EN)
	else { /* empty command */
		CMD_LOG(1, "$\n");
	}
#endif
#if CONSOLE_ECHO_EN
	console_write((uint8_t *)"$ ", 2);
#endif
	return status;
}

#define MSH_CMD_BUF_SIZE	512
char msh_cmd_buf[MSH_CMD_BUF_SIZE];
/**
 * @brief       This function is transformation all argument vectors to a command string.
 *              This interface function can only be called once during CMD execution.
 *
 * @param[in]   index:   Command line parameter number copied to character array.
 *
 * @return
 *              msh_cmd_buf: when argc less than index, return empty array.
 */
char* cmd_conv_from_argv(int argc, char *argv[], int index)
{
	int len;
	int i = index;
	int left = MSH_CMD_BUF_SIZE;
	memset(msh_cmd_buf, 0, MSH_CMD_BUF_SIZE);
	char *ptr = msh_cmd_buf;
	for (i; i < argc && left >= 2; ++i) {
		len = cmd_strlcpy(ptr, argv[i], left);
		ptr += len;
		left -= len;
		if (i < argc - 1 && left >= 2) {
			*ptr++ = ' ';
			*ptr = '\0';
			left -= 1;
		}
	}
	ptr = msh_cmd_buf;
	return ptr;
}

/* parse all argument vectors from a command string, return argument count */
int cmd_parse_argv(char *cmd, char *argv[], int size)
{
	int last, argc;
	char *start, *end;

	argc = 0;
	start = cmd;

	while (argc < size && *start != '\0') {
		while (*start == ' ' || *start == '\t')
			start++;
		if (*start == '\0')
			break;
		end = start;
		while (*end != ' ' && *end != '\t' && *end != '\0')
			end++;
		last = *end == '\0';
		*end = '\0';
		argv[argc++] = start;
		if (last)
			break;
		start = end + 1;
	}

	if (argc > 0 && argc < size) {
		argv[argc] = NULL; /* ANSI-C requirement */
	}

	return argc;
}

static const char *cmd_status_success_desc[CMD_STATUS_SUCCESS_MAX + 1 -
                                           CMD_STATUS_SUCCESS_MIN] = {
	"OK",
};

static const char *cmd_status_error_desc[CMD_STATUS_ERROR_MAX + 1 -
                                         CMD_STATUS_ERROR_MIN] = {
	"Unknown command",
	"Invalid argument",
	"Fail",
};

static const char *cmd_empty_desc = "";

const char *cmd_get_status_desc(enum cmd_status status)
{
	if (status >= CMD_STATUS_SUCCESS_MIN &&
	    status <= CMD_STATUS_SUCCESS_MAX) {
		return cmd_status_success_desc[status - CMD_STATUS_SUCCESS_MIN];
	} else if (status >= CMD_STATUS_ERROR_MIN &&
	           status <= CMD_STATUS_ERROR_MAX) {
		return cmd_status_error_desc[status - CMD_STATUS_ERROR_MIN];
	}

	return cmd_empty_desc;
}

int cmd_write(enum cmd_code_type type, int code, const char *fmt, ...)
{
	static char str_buf[400];

	va_list ap;
	int len, left;
	char *ptr;

	ptr = str_buf;
	left = sizeof(str_buf) - 1; /* reserve 1 byte for '\n' */
	len = cmd_snprintf(ptr, left, "<%s> %d ",
	                   type == CMD_CODE_TYEP_STATUS ? "ACK" : "EVT", code);

	ptr += len;
	left -= len;
	va_start(ap, fmt);
	len = vsnprintf(ptr, left, fmt, ap);
	va_end(ap);

	ptr += len;
	left -= len;
	left += 1;
	len = cmd_snprintf(ptr, left, "\n");

	left -= len;
	len = sizeof(str_buf) - left;

#ifdef CONFIG_OS_RTTHREAD
	return puts(str_buf);
#else
#ifdef CONFIG_CHIP_XRADIO
	return console_write((uint8_t *)str_buf, len);
#else
	return printf("%s", str_buf);
#endif
#endif
}

#ifdef CONFIG_CHIP_XRADIO
int32_t cmd_raw_mode_read(uint8_t *buf, int32_t size, uint32_t msec)
{
	UART_ID uart_id = console_get_uart_id();

	if (uart_id < UART_NUM) {
		return HAL_UART_Receive_Poll(uart_id, buf, size, msec);
	} else {
		return -1;
	}
}

int32_t cmd_raw_mode_write(uint8_t *buf, int32_t size)
{
	UART_ID uart_id = console_get_uart_id();

	if (uart_id < UART_NUM) {
		return HAL_UART_Transmit_Poll(uart_id, buf, size);
	} else {
		return -1;
	}
}

void cmd_print_uint8_array(uint8_t *buf, int32_t size)
{
	for (int i = 0; i < size; i++) {
		if (i % 8 == 0)
			printf("  ");
		if (i % 32 == 0)
			printf("\n");
		printf("%02x ", buf[i]);
	}
	printf("\n");
}

void cmd_print_uint32_array(uint32_t *buf, int32_t size)
{
	for (int i = 0; i < size; i++) {
		if (i % 4 == 0)
			printf("\n0x%08x: ", (uint32_t)&buf[i]);
		printf("%08x  ", buf[i]);
	}
	printf("\n");
}
#endif
