/**
 * Copyright (c) 2017, Realsil Semiconductor Corporation. All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include "hci_tp.h"

#include "os_mem.h"
#include "os_pool.h"

#include "hci_process.h"
#include "bt_types.h"

#include "bsp.h"
#include "comuart.h"

#include "hci_uart.h"
#include "bt_board.h"
#include "hci_board.h"
#include "os_sync.h"

#include "trace_app.h"


typedef struct
{
    P_HCI_TP_OPEN_CB    open_cb;
    uint8_t            *tx_buf;
} T_HCI_UART;

T_HCI_UART hci_rtk;
T_HCI_UART *p_hci_rtk = &hci_rtk;

uint8_t g_hci_step = 0;
extern HCI_PROCESS_TABLE hci_process_table[];
#if 0
extern uint8_t hci_total_step;
extern bool hci_board_complete(void);
//================================internal===========================
bool hci_rtk_tx_cb(void)
{
    if (p_hci_rtk->tx_buf != NULL)
    {
        os_mem_free(p_hci_rtk->tx_buf);
        p_hci_rtk->tx_buf = NULL;
    }
    return true;
}
#endif
//=================================external==========================
bool hci_adapter_send(uint8_t *p_buf, uint16_t len)
{
	#if 0 
    p_hci_rtk->tx_buf  = p_buf;
    return hci_tp_send(p_buf, len, hci_rtk_tx_cb);
	#endif
	
}


//====================hci_tp.h================

void hci_tp_close(void)
{
	#if 0 
    HCI_PRINT_INFO0("hci_tp_close");
    bt_power_off();
    hci_uart_deinit();
	#endif
    return;
}

bool hci_tp_send(uint8_t *p_buf, uint16_t len, P_HCI_TP_TX_CB tx_cb)
{

    return hci_uart_tx(p_buf, len, tx_cb);
}

void hci_tp_open(P_HCI_TP_OPEN_CB open_cb, P_HCI_TP_RX_IND rx_ind)
{
	APP_PRINT_INFO0("Reset Bluetooth");
	printf("%s,%d\n",__func__,__LINE__);
	bt_rf_probe();
	bt_reset_clear();
	os_delay(500);
	bt_reset_set();
	os_delay(1000);
    //hci_comuart.data_recv  = rx_ind;
	set_rx_ind_callback(rx_ind);
    comuart_init();
	os_if_printf("start firmware download");
}
#if 0
int hci_tp_h5_recv(uint8_t **data, int *num)
{
	  uint32_t rx_data_len;
    uint32_t flags;
    int ret;
    int consumed = 0;

	#if 0
    if (hciapi_info.data_ind_blocked)
    {
			  *num =0;
        return 0;
    }
#endif
    flags = os_lock();
    rx_data_len = hci_comuart.rx_data_len;
    os_unlock(flags);
    
		if(rx_data_len== 0)
		{
			 //printf("thereis no data in comuart\r\n");
			 return 0;
		}
    /* hci_frame_received() will be called if there is one complete h5 packet */
    if (hci_comuart.rx_read_index + rx_data_len <= COM_RX_BUFFER_SIZE)
    {
			*data = &hci_comuart.rx_buffer[hci_comuart.rx_read_index];
			*num = rx_data_len;
    }
    else
    {
			 *data = &hci_comuart.rx_buffer[hci_comuart.rx_read_index];
			 *num = COM_RX_BUFFER_SIZE - hci_comuart.rx_read_index;

    }
		//printf("\r\n====rx_len: %x, rx_read_index: %x, consumed: %x, *num : %x\r\n", rx_data_len, hci_comuart.rx_read_index,consumed,*num);
    consumed = *num;
    hci_comuart.rx_read_index += consumed;
		if(hci_comuart.rx_read_index == COM_RX_BUFFER_SIZE)  
		{
			  hci_comuart.rx_read_index = 0;
		}
		else if (hci_comuart.rx_read_index > COM_RX_BUFFER_SIZE_MASK)
		{
			  hci_comuart.rx_read_index &= COM_RX_BUFFER_SIZE_MASK;
		}
    flags = os_lock();
    hci_comuart.rx_data_len -= consumed;
    if (hci_comuart.rx_disabled == true) /* flow control */
    {
        if (hci_comuart.rx_data_len < COM_RX_ENABLE_COUNT)
        {
            comuart_rx_enable();
        }
    }
    os_unlock(flags);
		return rx_data_len - consumed;
}
#endif
