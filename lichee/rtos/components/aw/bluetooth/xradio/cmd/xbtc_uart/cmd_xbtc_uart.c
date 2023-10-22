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
#include "string.h"
#include "xbtc_uart.h"

#define XBTC_UART_CMD_BUF_SIZE    (128)

#define WORD_LENGTH_INDEX         (5)
static uart_word_length_t word_length_table[4] = {UART_WORD_LENGTH_5, UART_WORD_LENGTH_6, UART_WORD_LENGTH_7, UART_WORD_LENGTH_8};

#define STOP_BIT_INDEX            (1)
static uart_stop_bit_t stop_bit_table[2] = {UART_STOP_BIT_1, UART_STOP_BIT_2};

typedef struct {
	uart_baudrate_t baudrate_index;
	uint32_t baudrate;
} baudrate_cfg_t;

static const baudrate_cfg_t g_uart_baudrate_map[] =
{
	{UART_BAUDRATE_300, 300},
	{UART_BAUDRATE_600, 600},
	{UART_BAUDRATE_1200, 1200},
	{UART_BAUDRATE_2400, 2400},
	{UART_BAUDRATE_4800, 4800},
	{UART_BAUDRATE_9600, 9600},
	{UART_BAUDRATE_19200, 19200},
	{UART_BAUDRATE_38400, 38400},
	{UART_BAUDRATE_57600, 57600},
	{UART_BAUDRATE_115200, 115200},
	{UART_BAUDRATE_230400, 230400},
	{UART_BAUDRATE_576000, 576000},
	{UART_BAUDRATE_921600, 921600},
	{UART_BAUDRATE_1000000, 1000000},
	{UART_BAUDRATE_1500000, 1500000},
	{UART_BAUDRATE_3000000, 3000000},
	{UART_BAUDRATE_4000000, 4000000},
};

static int get_baudrate(xbtc_uart_config *config, int32_t baudrate)
{
	int index = 0;

	for (index = 0; index < sizeof(g_uart_baudrate_map) / sizeof(uint32_t); index++) {
		if (g_uart_baudrate_map[index].baudrate == baudrate) {
			config->config.baudrate = g_uart_baudrate_map[index].baudrate_index;
			return XBTC_UART_SUCCESS;
		}
	}

	return XBTC_UART_FAIL;
}

static int get_parity(xbtc_uart_config *config, char *parity)
{
	if (!strcmp(parity, "none")) {
		config->config.parity = UART_PARITY_NONE;
	} else if (!strcmp(parity, "odd")) {
		config->config.parity = UART_PARITY_ODD;
	} else if (!strcmp(parity, "even")) {
		config->config.parity = UART_PARITY_EVEN;
	} else {
		return XBTC_UART_FAIL;
	}
	return XBTC_UART_SUCCESS;
}

static void init_config(xbtc_uart_config *config)
{
	config->config.baudrate = UART_BAUDRATE_115200;
	config->config.word_length = UART_WORD_LENGTH_8;
	config->config.stop_bit = UART_STOP_BIT_1;
	config->config.parity = UART_PARITY_NONE;
	config->flow_control = TRUE;
}

/*xbtc_uart <on/off> <id> [baudrate] [word length] [stopbit] [parity] [flow control]*/
static void xbtc_uart_cmd_exec(char *pcmd)
{
	int ret;
	int32_t uart_id = 0;
	char state[5] = { 0 };
	int32_t baudrate = 0;
	int32_t word_length = 0;
	int32_t stop_bit = 0;
	char parity[5] = { 0 };
	int32_t flow = 0;
	xbtc_uart_config config;

	cmd_sscanf(pcmd, "%s id=%d b=%d w=%d s=%d p=%s f=%d",
	           state, &uart_id, &baudrate, &word_length, &stop_bit, parity, &flow);

	if (!strcmp(state, "on")) {

		if (uart_id < 0 || uart_id > 9) {
			CMD_ERR("invalid uart id number %c\n", uart_id);
			return;
		}

		init_config(&config);

		if (get_baudrate(&config, baudrate) == XBTC_UART_FAIL) {
			CMD_ERR("invalid baudrate %d\n", baudrate);
			return;
		}

		if (word_length > 8 || word_length < 5) {
			CMD_ERR("invalid word length %d\n", word_length);
			return;
		} else {
			config.config.word_length = word_length_table[word_length - WORD_LENGTH_INDEX];
		}

		if (stop_bit > 2 || stop_bit < 1) {
			CMD_ERR("invalid stop bit %d\n", stop_bit);
			return;
		} else {
			config.config.stop_bit = stop_bit_table[stop_bit - STOP_BIT_INDEX];
		}

		if (get_parity(&config, parity)  == XBTC_UART_FAIL) {
			CMD_ERR("invalid parity %s\n", parity);
			return;
		}

		if (flow > 1) {
			CMD_ERR("invalid flow control %d\n", flow);
			return;
		} else {
			config.flow_control = (flow == 1) ? TRUE : FALSE;
		}

		CMD_DBG("now config is %d %d %d %d %d\n", config.config.baudrate, config.config.word_length, config.config.stop_bit, config.config.parity, config.flow_control);

		if (xbtc_uart_init(uart_id, &config) == XBTC_UART_FAIL) {
			CMD_ERR("uart init failed\n");
		}
	} else if (!strcmp(state, "off")) {
		xbtc_uart_deinit();
	} else if (!strcmp(state, "help")) {
		CMD_DBG("xbtc_uart <on/off> <id> [baudrate] [word length:5~8] [stopbit:1~2] [parity:none/odd/even] [flow control:0/1]\n");
		CMD_DBG("example:xbtc_uart on id=2 b=115200 w=8 s=1 p=none f=1");
		CMD_DBG("default:baudrate: 115200 word_length: 8 stopbit: 1 parity: none flow_control: on\n");
	} else if (!strcmp(state, "test")) {
		xbtc_uart_test(pcmd, strlen(pcmd));
	} else {
		CMD_ERR("invalid uart state %s\n", state);
	}
}

static void xbtc_uart_exec(int argc, char *argv[])
{
	char xbtc_uart_cmd_buf[XBTC_UART_CMD_BUF_SIZE];
	int i, len;
	int left = XBTC_UART_CMD_BUF_SIZE;
	char *ptr = xbtc_uart_cmd_buf;

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

	xbtc_uart_cmd_exec(xbtc_uart_cmd_buf);
}
FINSH_FUNCTION_EXPORT_CMD(xbtc_uart_exec, xbtc_uart, bluetooth uart command);
