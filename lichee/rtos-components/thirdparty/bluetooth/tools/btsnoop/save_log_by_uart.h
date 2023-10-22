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

#ifndef _SAVE_LOG_BY_UART_H_
#define _SAVE_LOG_BY_UART_H_

#include "save_log_interface.h"

int uart_save_log_start_up(int type);
int uart_save_log_shut_down(void);
int uart_save_log_write(const void *p_data, int len);
int get_uart_save_log_type(void);
const struct save_log_iface *uart_save_log_iface_create(save_log_type_t type, uint8_t uart, uint32_t baudrate);

#endif /* _SAVE_LOG_BY_UART_H_ */
