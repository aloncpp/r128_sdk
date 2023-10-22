#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tinatest.h>
#include <hal_timer.h>
#include <hal/hal_uart.h>

#define UART_PORT	CONFIG_UART_PORT
#define UART_BAUDRATE   CONFIG_UART_BAUDRATE

//tt uarttester
int tt_uarttest(int argc, char **argv)
{
	char tbuf[6] = {"hello"};
	uint8_t rbuf[10] = {0};
	uart_port_t port;
	uint32_t baudrate;
	_uart_config_t uart_config;
	int i;

	printf("==========Begin to test uart===========\n");
	printf("Note that:short circuit tx and rx!\n");
	port = strtol(UART_PORT, NULL, 0);
	baudrate = strtol(UART_BAUDRATE, NULL, 0);

	printf("test args is:\n");
	printf("uart port:uart%d\n", port);
	printf("uart baudrate:%d\n", baudrate);

	memset(rbuf, 0, 10 * sizeof(uint8_t));

	switch (baudrate) {
	case 4800:
		uart_config.baudrate = UART_BAUDRATE_4800;
		break;
	case 9600:
		uart_config.baudrate = UART_BAUDRATE_9600;
		break;
	case 115200:
		uart_config.baudrate = UART_BAUDRATE_115200;
		break;
	default:
		uart_config.baudrate = UART_BAUDRATE_115200;
		break;
	}

	uart_config.word_length = UART_WORD_LENGTH_8;
	uart_config.stop_bit = UART_STOP_BIT_1;
	uart_config.parity = UART_PARITY_NONE;

	hal_uart_init(port);
	hal_uart_control(port, 0, &uart_config);
	hal_uart_disable_flowcontrol(port);
	hal_uart_set_loopback(port, 1);

	/* send */
	hal_uart_send(port, tbuf, 5);

	printf("Sending:");
	for (i = 0; i < 5; i++)
		printf("%c", tbuf[i]);
	printf("\n");

	/* receive */
	hal_uart_receive_no_block(port, rbuf, 5, MS_TO_OSTICK(1000));

	printf("Receiving:");
	for (i = 0; i < 5; i++)
		printf("%c", rbuf[i]);
	printf("\n");

	/* verify data */
	for (i = 0; i < 5; i++) {
		if (tbuf[i] != rbuf[i])
			break;
	}
	if (i == 5)
		printf("==========Test uart%d success!========\n", port);
	else
		printf("!!Test uart%d failed!\n", port);
	hal_msleep(1000);
	hal_uart_deinit(port);

	return 0;
}
testcase_init(tt_uarttest, uarttester, uarttester for tinatest);
