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

#pragma once

#include <stdbool.h>
//#include "bt_types.h"
#include "../save_log_interface.h"
#include "../save_log_by_file.h"
#include "../save_log_by_uart.h"

enum btsnoop_cfg {
	BTSNOOP_CFG_WITHOUT_FILE_HEADER = (1 << 0),
	BTSNOOP_CFG_WITHOUT_PACKET_HEADER = (1 << 1),
};

#define BTSNOOP_ELLISYS (BTSNOOP_CFG_WITHOUT_FILE_HEADER | BTSNOOP_CFG_WITHOUT_PACKET_HEADER)
#define BTSNOOP_UART_CFA (BTSNOOP_CFG_WITHOUT_FILE_HEADER)
#define BTSNOOP_FILE_CFA (0)

int btsnoop_start_up(const struct save_log_iface *iface, uint32_t cfg);
int btsnoop_shut_down(void);

void btsnoop_capture(uint8_t type, const uint8_t *packet, bool is_received);
