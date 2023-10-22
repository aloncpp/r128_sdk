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

typedef enum {
    UART_SAVE_BT_LOG = 1,
    UART_SAVE_WLAN_LOG = 2,
} uart_save_log_type_t;

/*
    if choose 1, user need use Hcidump Tool to save hci log;
    if choose 0, user can combine Hci Log and Air Log for debugging (by ellisys).
*/
#define SYNC_WITH_HCIDUMP		1

int uart_save_log_start_up(int type);
int uart_save_log_shut_down(void);

int uart_save_log_write(const void *p_data, int len);

int get_uart_save_log_type(void);

#endif /* _SAVE_LOG_BY_UART_H_ */
