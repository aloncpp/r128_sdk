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

#ifdef CONFIG_DRIVER_R128
#define USERIAL_SAVE_LOG_UART_ID UART_2
#else
#define USERIAL_SAVE_LOG_UART_ID UART_3
#endif
#define USERIAL_SAVE_LOG_UART_BAUDRATE UART_BAUDRATE_1500000

static int uart_save_log_type = -1;
static int32_t uart_port = USERIAL_SAVE_LOG_UART_ID;
static uint32_t uart_baudrate = USERIAL_SAVE_LOG_UART_BAUDRATE;

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

static void userial_save_log_config(uint8_t uart, uint32_t baudrate)
{
    switch (uart) {
    case 0:
        uart_port = UART_0;
        break;
    case 1:
        uart_port = UART_1;
        break;
    case 2:
        uart_port = UART_2;
        break;
    case 3:
        uart_port = UART_3;
        break;
    case 4:
        uart_port = UART_4;
        break;
    default:
        uart_port = USERIAL_SAVE_LOG_UART_ID;
        break;
    }

    switch (baudrate) {
    case 115200:
        uart_baudrate = UART_BAUDRATE_115200;
        break;
    case 921600:
        uart_baudrate = UART_BAUDRATE_921600;
        break;
    case 1000000:
        uart_baudrate = UART_BAUDRATE_1000000;
        break;
    case 1500000:
        uart_baudrate = UART_BAUDRATE_1500000;
        break;
    case 3000000:
        uart_baudrate = UART_BAUDRATE_3000000;
        break;
    default:
        uart_baudrate = USERIAL_SAVE_LOG_UART_BAUDRATE;
        break;
    }
}

// Internal functions
static int userial_save_log_init(void)
{
    int ret ;

    _uart_config_t uart_config;

    uart_config.word_length = UART_WORD_LENGTH_8;
    uart_config.stop_bit = UART_STOP_BIT_1;
    uart_config.parity = UART_PARITY_NONE;
    uart_config.baudrate = uart_baudrate;

    ret = hal_uart_init(uart_port);

    hal_uart_control(uart_port, 0, &uart_config);

    hal_uart_disable_flowcontrol(uart_port);

    return ret;

}

static int userial_save_log_deinit(int uart_id)
{
    uart_port = USERIAL_SAVE_LOG_UART_ID;
    uart_baudrate = USERIAL_SAVE_LOG_UART_BAUDRATE;

    return hal_uart_deinit(uart_id);
}

// Module lifecycle functions
int uart_save_log_start_up(int type)
{
    if (type > SAVE_WLAN_LOG) {
        UART_ERROR("invalid type:%d !!!\n", type);
        return -1;
    }

    int ret = -1;
    ret = userial_save_log_init();
    if (ret != 0) {
        return ret;
    }

    uart_save_log_type = type;

    if (type == SAVE_BT_LOG) {
        //Send uart sync msg to indicate the begin of hcilog
        uint16_t sync_uart = 0xFFFF;
        uart_save_log_write((uint8_t *)&sync_uart, 2);
    }

    return 0;
}

int uart_save_log_shut_down(void)
{
    uart_save_log_type = -1;

    return userial_save_log_deinit(uart_port);
}

// Interface function
int uart_save_log_write(const void *p_data, int len)
{
    if (uart_save_log_type < 0) {
        UART_ERROR("uart_save_log module has not been initialized!!!\n");
        return -1;
    }

    int32_t total = 0;
    int ret;

    while (len > 0) {
        ret = hal_uart_send(uart_port, (uint8_t *)p_data + total, len);

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

static const struct save_log_iface save_log_if =
{
    .write = uart_save_log_write,
    .shut_down = uart_save_log_shut_down,
};

const struct save_log_iface *uart_save_log_iface_create(save_log_type_t type, uint8_t uart, uint32_t baudrate)
{
    userial_save_log_config(uart, baudrate);

    if (uart_save_log_start_up(type) == 0)
        return &save_log_if;
    return NULL;
}

