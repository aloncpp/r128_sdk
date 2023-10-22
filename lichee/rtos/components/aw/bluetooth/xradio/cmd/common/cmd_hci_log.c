/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <console.h>

#include "cmd_util.h"

#ifdef CONFIG_BT_DRIVERS_LOG_BTSNOOP
#include "btsnoop.h"
#endif

#ifdef CONFIG_BT_DRIVERS_LOG_HCIDUMP
#include "hcidump_xr.h"
#endif

#ifdef CONFIG_BT_DRIVERS_LOG_BTSNOOP
/*
 * command format: bt_snoop <on/off/flush> [file/uart/ellisys] [file path] [UART<n>] [buadrate]
 */
static void bt_snoop(int argc, char **argv)
{
	uint32_t cfg = BTSNOOP_UART_CFA;
	const struct save_log_iface *iface;
	uint8_t uart = 2; /* UART2, default */
	uint32_t br = 1500000; /* 1.5M, default */
	save_log_type_t type = SAVE_BT_LOG;

	if(argc < 2) {
		printf("para invalid\n");
		return ;
	}

	if (!strcmp(argv[1], "on")) {
		if (argc >= 3) {
			if (!strcmp(argv[2], "file")) {
				cfg = BTSNOOP_FILE_CFA;
				char *path = NULL;
				uint8_t sync = 0;
				if (argc >= 4) {
					if (!strncmp(argv[3], "--sync", 6)) {
						sync = 1; // it's really slow.
						if (argc == 5) {
							path = argv[4];
						}
					} else {
						path = argv[3];
					}
				}
				iface = file_save_log_iface_create(path, sync);
			} else if ((!strcmp(argv[2], "uart")) || (!strcmp(argv[2], "ellisys"))) {
				if (!strcmp(argv[2], "uart")) {
					cfg = BTSNOOP_UART_CFA;
					type = SAVE_BT_LOG;
				} else {
					cfg = BTSNOOP_ELLISYS;
					type = SAVE_ELLISYS;
				}

				if (argc >= 4 && (!strncmp(argv[3], "UART", 4))) {
					if (sscanf(argv[3] + 4, "%d", &uart) != 1)
						goto invalid;
				}
				if (argc >= 5) {
					if (sscanf(argv[4], "%d", &br) != 1)
						goto invalid;
				}

				iface = uart_save_log_iface_create(type, uart, br);
			} else {
				goto invalid;
			}
		} else {
			goto invalid;
		}

		if (iface == NULL)
			goto invalid;

		btsnoop_start_up(iface, cfg);
	} else if (!strcmp(argv[1], "off")) {
		btsnoop_shut_down();
	} else if (!strcmp(argv[1], "flush")) {
		file_save_log_flush();
	} else {
		goto invalid;
	}

	return;

invalid:
	printf("para invalid. bt_snoop <on/off/flush> <file/uart/ellisys> [file path] [UART<n>] [buadrate]\n");
	printf("example: bt_snoop on file data/btsnoop.cfa\n");
	printf("example: bt_snoop on uart UART1 15000000\n");
}

FINSH_FUNCTION_EXPORT_CMD(bt_snoop, bt_snoop, Console bt_snoop command);

#endif

#ifdef CONFIG_BT_DRIVERS_LOG_HCIDUMP
/*
 * command format: bt_hcidump <on, off>
 */
static void bt_hcidump(int argc, char **argv)
{
	if(argc < 2) {
		printf("para invalid\n");
		return ;
	}
	if (!strcmp(argv[1], "on")) {
		printf("bt_hcidump on\n");
		hcidump_start_up(PARSE);
	} else if (!strcmp(argv[1], "off")) {
		printf("bt_hcidump off\n");
		hcidump_shut_down();
	} else {
		printf("para invalid\n");
	}
}

FINSH_FUNCTION_EXPORT_CMD(bt_hcidump, bt_hcidump, Console hcidump command);

#endif

