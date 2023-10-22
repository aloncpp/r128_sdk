/**
 *******************************************************************************
 * Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
 *******************************************************************************
 * @file     comuart.h
 * @brief    Declaration of low-level UART functions, macros, etc.
 * @details  none
 * @author   Alex Lu
 * @date     2016-10-17
 * @version  v0.1
 * *****************************************************************************
 */

#ifndef __COMUART_H__
#define __COMUART_H__

#include <stdint.h>
#include <aw_types.h>
#include "hci_tp.h"

#define COM_RX_BUFFER_SIZE          0x800   /* RX buffer size */
#define COM_RX_BUFFER_SIZE_MASK     0x7FF   /* Buffer size mask */
#define COM_RX_ENABLE_COUNT         0x300   /* Enable RX */
#define COM_RX_DISABLE_COUNT        0x400   /* Disable RX */

typedef void (*TCallBack)(void);

typedef struct t_tcomdata
{
    uint8_t  *buffer;
    uint32_t length;
} T_COMDATA;

typedef struct t_comuart
{
    T_COMDATA                 tx_data;
    uint32_t                  baudrate;
    bool                      rx_disabled;
    bool                      tx_disabled;

    uint32_t                  rx_data_len;
    uint32_t                  rx_offset;
    uint16_t                  rx_read_index;
    uint16_t                  rx_write_index;
    uint8_t                   rx_buffer[COM_RX_BUFFER_SIZE];

    TCallBack                 data_recv;
    TCallBack                 start_transmit;
    TCallBack                 transmit_completed;
} T_COMUART;

#if 0
extern T_COMUART hci_comuart;
extern void comuart_init(void);
extern void comuart_start_transmit(void);
extern void comuart_set_baudrate(uint32_t baudrate);
extern void comuart_rx_disable(void);
extern void comuart_rx_enable(void);

void set_rx_ind_callback(P_HCI_TP_RX_IND ind);
#else
#include "bt_uart.h"
#endif
#endif /* __COMUART_H__ */
