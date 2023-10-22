/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#include <hal_uart.h>
#include <stdio.h>
#include "save_log_by_uart.h"

#define USERIAL_SAVE_LOG_UART_ID UART_3
#define USERIAL_SAVE_LOG_UART_BAUDRATE UART_BAUDRATE_1500000

static int uart_save_log_type = 0;

#define	_UART_WARN			1
#define	_UART_ERROR			1
#define	_UART_INFO			1

#define UART_LOG(flags, fmt, arg...)		\
        do {							\
            if (flags)					\
                printf(fmt, ##arg); 	\
        } while (0)

#define UART_WARN(fmt, arg...)	\
        UART_LOG(_UART_WARN, "[UART WRN] %s():%d "fmt, __func__, __LINE__, ##arg)

#define UART_ERROR(fmt, arg...)	\
        UART_LOG(_UART_ERROR, "[UART ERR] %s():%d "fmt, __func__, __LINE__, ##arg)

#define UART_INFO(fmt, arg...)	\
        UART_LOG(_UART_INFO, "[UART INF]  "fmt, ##arg)

// Internal functions
static int userial_save_log_init(void)
{
	int ret ;

	_uart_config_t uart_config;

	uart_config.word_length = UART_WORD_LENGTH_8;
	uart_config.stop_bit = UART_STOP_BIT_1;
	uart_config.parity = UART_PARITY_NONE;
	uart_config.baudrate = USERIAL_SAVE_LOG_UART_BAUDRATE;

	ret = hal_uart_init(USERIAL_SAVE_LOG_UART_ID);

	hal_uart_control(USERIAL_SAVE_LOG_UART_ID, 0, &uart_config);

	hal_uart_disable_flowcontrol(USERIAL_SAVE_LOG_UART_ID);

	return ret;

}

static int userial_save_log_deinit(int uart_id)
{
	return hal_uart_deinit(USERIAL_SAVE_LOG_UART_ID);
}


// Module lifecycle functions
int uart_save_log_start_up(int type)
{
    if (type != UART_SAVE_BT_LOG && type!= UART_SAVE_WLAN_LOG) {
        UART_ERROR("invalid type:%d !!!\n", type);
        return -1;
    }

    int ret = -1;
    ret = userial_save_log_init();
    if (ret != 0) {
        return ret;
    }

    uart_save_log_type = type;

#if SYNC_WITH_HCIDUMP
	//Send uart sync msg to indicate the begin of hcilog
	uint16_t sync_uart = 0xFFFF;
	uart_save_log_write((uint8_t *)&sync_uart, 2);
#endif

    return 0;
}

int uart_save_log_shut_down(void)
{
    uart_save_log_type = 0;

    return userial_save_log_deinit(USERIAL_SAVE_LOG_UART_ID);
}

// Interface function
int uart_save_log_write(const void *p_data, int len)
{
    if (uart_save_log_type != UART_SAVE_BT_LOG  &&  uart_save_log_type != UART_SAVE_WLAN_LOG) {
        UART_ERROR("uart_save_log module has not been initialized!!!\n");
        return -1;
    }

    int32_t total = 0;
    int ret;

    while (len > 0) {

		ret = hal_uart_send(USERIAL_SAVE_LOG_UART_ID, (uint8_t *)p_data + total, len);

        if (ret < 0) {
            UART_ERROR("error(%d) writing to serial port.\n", ret);
            return total;
        }
        total += ret;
        len -= ret;
    }

    return total;
}

int get_uart_save_log_type(void)
{
    return uart_save_log_type;
}
