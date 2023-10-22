/**
 ******************************************************************************
 Copyright (c) 2015, Realtek Semiconductor Corporation. All rights reserved.
 ******************************************************************************
 * @file       trace_uart.h
 * @brief      This file contains type definitions and function declarations for trace
 */

#ifndef _TRACE_UART_H
#define _TRACE_UART_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
typedef void (*P_CALL_BACK)(void);

typedef struct t_trace_data
{
    char *buffer;
    uint32_t length;
} T_TRACE_DATA;

typedef struct t_trace_uart
{
    T_TRACE_DATA          tx_data;
    P_CALL_BACK           p_transmit_completed;
	  uint8_t               tx_busy;
} T_TRACE_UART;

extern T_TRACE_UART trace_uart;

typedef bool (*UART_TX_CB)(void);

bool trace_uart_init(void);

bool trace_uart_deinit(void);

bool trace_uart_tx(uint8_t *p_buf, uint16_t len, UART_TX_CB tx_cb);

extern void dma_transmit_enable(void);

#ifdef __cplusplus
}
#endif

#endif

