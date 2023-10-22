/**
 ******************************************************************************
 Copyright (c) 2015, Realtek Semiconductor Corporation. All rights reserved.
 ******************************************************************************
 * @file       trace_uart.c
 * @brief      This file contains the platform dependent part of the trace
 *            implementation.
 */

#include "drivers/hal_uart.h"
#include "FreeRTOS.h"
#include "trace_uart.h"

#include "console.h"
enum { __FILE_NUM__ = 0 };

static int uart3_tx_cmplt_flag = 0;
static void bt_uart3_callback(uart_callback_event_t event, void *user_data)
{
	switch(event) {
	case UART_EVENT_TX_COMPLETE:
		uart3_tx_cmplt_flag = 1;
		break;
	case UART_EVENT_RX_COMPLETE:
		break;
	default:
		break;
	}
}


bool trace_uart_init(void)
{
	portBASE_TYPE ret;
	uart_config_t tmp_config = {
		.baudrate = UART_BAUDRATE_1500000,
		.word_length = UART_WORD_LENGTH_8,
		.stop_bit = UART_STOP_BIT_1,
		.parity = UART_PARITY_NONE
    };

	printf("uart 3 init start.\n");

	hal_uart_init(UART_3, &tmp_config);
	hal_uart_register_callback(UART_3, bt_uart3_callback, NULL);
}

bool trace_uart_tx(uint8_t *pstr,uint16_t len, UART_TX_CB tx_cb)
{
	const TickType_t xVeryShortDelay = 2UL;
	hal_uart_send(UART_3, pstr, len);

	while (uart3_tx_cmplt_flag == 0)
		vTaskDelay( xVeryShortDelay );

	uart3_tx_cmplt_flag = 0;

	if(tx_cb!=NULL)
	{
		tx_cb();
	}
    return true;
}
bool trace_uart_deinit(void)
{
  	//deinit
     return true;
}

static void vUart3TestTx(void)
{
	char p_buf[8]={0xc0,0x00,0x2f,0x00,0xd0,0x01,0x7e,0xc0};
	while(1) {
		trace_uart_tx(p_buf,8, NULL);
		usleep(1000000);
	}
	vTaskDelete(NULL);
}


void cmd_uart3_test(void)
{
	portBASE_TYPE ret;

	trace_uart_init();

	ret = xTaskCreate(vUart3TestTx,
				(signed portCHAR *)"uart3-tx",
				4096,
				NULL,
				configTIMER_TASK_PRIORITY - 1,
				NULL);

	if (ret != pdPASS) {
		printf("Error creating uart1 rx task, status was %d\r\n", ret);
		return;
	}

	
}

FINSH_FUNCTION_EXPORT_CMD(cmd_uart3_test, u3, Console uart 3 test Command);
