/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <hal_log.h>
#include <hal_cmd.h>
#include <hal_timer.h>
#include <hal_lpuart.h>
#include <hal_uart.h>

/* find a free uart_port or pc com as source */
#define UART_TEST UART_1
#define TEST_LEN 5
uint8_t cmp_data[TEST_LEN + 1] = "abcde";
static const uint32_t g_lpuart_baudrate_map[] =
{
	300,
	600,
	1200,
	2400,
	4800,
	9600
};

static void cmd_usage(void)
{
	printf("Usage:\n"
		"\t hal_lpuart <port> <baudrate> <cmplen>\n");
}

void uart_help_send(uart_baudrate_t baudrate, uint32_t cmplen)
{
	_uart_config_t uart_config;
	uart_config.baudrate = baudrate;
	uart_config.word_length = UART_WORD_LENGTH_8;
	uart_config.stop_bit = UART_STOP_BIT_2;
	uart_config.parity = UART_PARITY_NONE;
	hal_uart_init(UART_TEST);
	hal_uart_control(UART_TEST, 0, &uart_config);
	hal_uart_send(UART_TEST, cmp_data, cmplen);
}

void test_recv_data(lpuart_port_t port, uart_baudrate_t baudrate)
{
	printf("\nenter lpuart%d recv data test, baudrate:%d\n\n", port, g_lpuart_baudrate_map[baudrate]);
	hal_lpuart_enable_rx_data(port, NULL, NULL);
	/* use uart as source */
	uart_help_send(baudrate, 1);
	/* use pc com as source */
	printf("please enter a char in 5s, if you use hardware loopback, please ignore\n");
	hal_sleep(5);
	hal_lpuart_disable_rx_data(port);
}

static void compare_callback(void *arg)
{
	printf("data compare success!\n");
}

void test_cmp_data(lpuart_port_t port, uart_baudrate_t baudrate, int cmplen)
{
	printf("\nenter lpuart%d cmp data test, baudrate:%d, cmplen:%d\n\n", port, g_lpuart_baudrate_map[baudrate], cmplen);

	if (!hal_lpuart_is_init(port)) {
		printf("lpuart %d not inited\n", port);
		return;
	}

	hal_lpuart_rx_cmp(port, cmplen, cmp_data);
	hal_lpuart_enable_rx_cmp(port, compare_callback, NULL);
	/* use uart as source, stop bit of uart should be 2 */
	uart_help_send(baudrate, cmplen);
	/* use pc com as source */
	printf("please enter abcde in 5s, if you use hardware loopback, please ignore\n");
	hal_sleep(5);
	hal_lpuart_disable_rx_cmp(port);
}

void lpuart_reset_multiplex()
{
	lpuart_multiplex(LPUART_0, UART_0);
	lpuart_multiplex(LPUART_1, UART_1);
}

int cmd_test_lpuart(int argc, char **argv)
{
	_lpuart_config_t lpuart_config;
	int i;
	int j;

	if (argc != 4) {
		cmd_usage();
		return -1;
	}

	lpuart_port_t port;
	uint32_t baudrate;
	uint32_t cmplen;

	port = strtol(argv[1], NULL, 0);
	if (hal_lpuart_init(port) != SUNXI_HAL_OK) {
		printf("Fail to init lpuart\n");
		return -1;
	}

	if (port == 0) {
		lpuart_multiplex(LPUART_0, UART_TEST);
	} else if (port == 1) {
		lpuart_multiplex(LPUART_1, UART_TEST);
	}
	cmplen = strtol(argv[3], NULL, 0);

	lpuart_config.word_length = LPUART_WORD_LENGTH_8,
	lpuart_config.msb_bit     = LPUART_MSB_BIT_0,
	lpuart_config.parity      = LPUART_PARITY_NONE,
	baudrate = strtol(argv[2], NULL, 0);
	switch (baudrate) {
	case 300:
		lpuart_config.baudrate = LPUART_BAUDRATE_300;
		break;

	case 600:
		lpuart_config.baudrate = LPUART_BAUDRATE_600;
		break;

	case 1200:
		lpuart_config.baudrate = LPUART_BAUDRATE_1200;
		break;

	case 2400:
		lpuart_config.baudrate = LPUART_BAUDRATE_2400;
		break;

	case 4800:
		lpuart_config.baudrate = LPUART_BAUDRATE_4800;
		break;
	case 9600:
		lpuart_config.baudrate = LPUART_BAUDRATE_9600;
		break;
	default:
		hal_log_info("Using default baudrate: 9600\n");
		lpuart_config.baudrate = LPUART_BAUDRATE_9600;
		break;
	}

	if (cmplen >= 1 && cmplen <= 5) {
		hal_lpuart_control(port, (void *)&lpuart_config);
		test_recv_data(port, lpuart_config.baudrate);
		test_cmp_data(port, lpuart_config.baudrate, cmplen);
	} else {
		printf("test all conditons\n");
		for(i = 0; i <= LPUART_BAUDRATE_9600; i++) {
			lpuart_config.baudrate = i;
			hal_lpuart_control(port, (void *)&lpuart_config);
			test_recv_data(port, lpuart_config.baudrate);
			for (j=1; j <= 5;j++)
				test_cmp_data(port, lpuart_config.baudrate,j);
		}
	}

	lpuart_reset_multiplex();

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_lpuart, hal_lpuart, lpuart hal APIs tests)
