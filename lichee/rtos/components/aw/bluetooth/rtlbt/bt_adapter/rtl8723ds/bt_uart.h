#ifndef __BT_UART_H__
#define __BT_UART_H__

#include <stdint.h>
#include <aw_types.h>

typedef bool (*HCI_UART_CB)(void);

void comuart_init(void);
void comuart_start_transmit(void);
void comuart_set_baudrate(uint32_t baudrate);
void comuart_rx_disable(void);
void comuart_rx_enable(void);
void set_rx_ind_callback(HCI_UART_CB ind);

#endif
