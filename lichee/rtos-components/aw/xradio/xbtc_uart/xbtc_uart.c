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
#include "bt_lib.h"
#include "xbtc_uart.h"
#include "kernel/os/os.h"

static const bt_lib_interface_t *bt_uart_lib_if;
static int bt_uart_lib_id = -1;
static int32_t bt_uart_id = -1;

static XR_OS_Thread_t g_xbtc_uart_thread;
#define BT_UART_THREAD_PROI           (3)
#define BT_UART_THREAD_SIZE           (4 * 1024)

#define XBTC_UART_TRACE_LEVEL_ERROR     (0)
#define XBTC_UART_TRACE_LEVEL_WARNING   (1)
#define XBTC_UART_TRACE_LEVEL_DEBUG     (2)

#define XBTC_UART_DBG_LEVEL             XBTC_UART_TRACE_LEVEL_DEBUG
#define XBTC_UART_TRACE_ERROR(fmt, args...)       {if (XBTC_UART_DBG_LEVEL >= XBTC_UART_TRACE_LEVEL_ERROR)   printf(fmt, ##args);}
#define XBTC_UART_TRACE_WARNING(fmt, args...)     {if (XBTC_UART_DBG_LEVEL >= XBTC_UART_TRACE_LEVEL_WARNING) printf(fmt, ##args);}
#define XBTC_UART_TRACE_DEBUG(fmt, args...)       {if (XBTC_UART_DBG_LEVEL >= XBTC_UART_TRACE_LEVEL_DEBUG)   printf(fmt, ##args);}

#define DATA_INDEX                    (2)

#define TYPE_LEN                      (1)
#define DATA_LEN                      (1)
#define OPCODE_OR_HANDLE_LEN          (2)

#define HCI_TYPE_MAX                  (4)
/*opcode len*/
#define RECIVE_LEN                (OPCODE_OR_HANDLE_LEN + DATA_LEN)

#define DBG_XBTC_UART                 (1)
static int xbtc_uart_recv_cb(const uint8_t *data, uint16_t len)
{
	if (hal_uart_send(bt_uart_id, data, len) != len) {
		return XBTC_UART_FAIL;
	}

#if DBG_XBTC_UART
	XBTC_UART_TRACE_DEBUG("send:");
	for (int i = 0; i < len; i++) {
		XBTC_UART_TRACE_DEBUG("%x ", data[i]);
	}
	XBTC_UART_TRACE_DEBUG("\n");
#endif
	return XBTC_UART_SUCCESS;
}

static int xbtc_uart_register_lib(void)
{
	char mac[6] = {0};
	bt_hc_callbacks_t bt_hc_callbacks;
	bt_uart_lib_if = bt_lib_get_interface();
	for (int i = 0; i < 6; ++i) {
		mac[i] = (uint8_t)XR_OS_GetTicks();
	}
	XBTC_UART_TRACE_DEBUG("mca is %02x %02x %02x %02x %02x %02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	bt_uart_lib_if->init();
	bt_hc_callbacks.data_ind = &xbtc_uart_recv_cb;
	bt_uart_lib_id = bt_uart_lib_if->hci_ops->open(BT_FEATURES_DUAL, (void *)&bt_hc_callbacks);
	bt_uart_lib_if->hci_ctrl_ops->set_mac(mac);

	return (bt_uart_lib_id == 0) ? XBTC_UART_SUCCESS : XBTC_UART_FAIL;
}

static void xbtc_uart_unregister_lib(void)
{
	bt_uart_lib_if->hci_ops->close(bt_uart_lib_id);

	bt_uart_lib_if->deinit();
}

static void xbtc_uart_thread(void *param)
{
	uint8_t type = 0xff;
	uint8_t *data = NULL;
	uint8_t buf[RECIVE_LEN] = {0};

	while (1) {
		while (type > HCI_TYPE_MAX) {
			hal_uart_receive(bt_uart_id, &type, 1);
		}
		hal_uart_receive(bt_uart_id, buf, RECIVE_LEN);

		data = (uint8_t *)malloc((buf[DATA_INDEX] + RECIVE_LEN) * sizeof(uint8_t));
		if (data == NULL) {
			XBTC_UART_TRACE_ERROR("malloc failed\n");
			XR_OS_ThreadDelete(NULL);
			return;
		}

		memcpy(data, buf, RECIVE_LEN);

		hal_uart_receive(bt_uart_id, data + RECIVE_LEN, buf[DATA_INDEX]);

#if DBG_XBTC_UART
		XBTC_UART_TRACE_DEBUG("rec:");
		for (int i = 0; i < RECIVE_LEN + buf[DATA_INDEX]; i++) {
			XBTC_UART_TRACE_DEBUG("%x ", data[i]);
		}
		XBTC_UART_TRACE_DEBUG("\n");
#endif

		bt_uart_lib_if->hci_ops->write(bt_uart_lib_id, type, data, RECIVE_LEN + buf[DATA_INDEX]);

		free(data);
		data = NULL;
		type = 0xff;
	}
}

int xbtc_uart_init(int32_t id, xbtc_uart_config *config)
{
	int ret = XBTC_UART_SUCCESS;

	if (bt_uart_id != -1) {
		XBTC_UART_TRACE_ERROR("uart have been init\n");
		return XBTC_UART_FAIL;
	}

	ret = hal_uart_init(id);
	if (ret != XBTC_UART_SUCCESS) {
		XBTC_UART_TRACE_ERROR("uart init failed, reason is %d\n", ret);
		goto failed1;
	}

	hal_uart_control(id, 0, config);

	if (config->flow_control == FALSE)
		hal_uart_disable_flowcontrol(id);
	else
		hal_uart_set_hardware_flowcontrol(id);

	ret = xbtc_uart_register_lib();
	if (ret != XBTC_UART_SUCCESS) {
		XBTC_UART_TRACE_ERROR("lib register failed, reason is %d\n", ret);
		goto failed2;
	}

	memset(&g_xbtc_uart_thread, 0, sizeof(g_xbtc_uart_thread));
	ret = XR_OS_ThreadCreate(&g_xbtc_uart_thread, "xbtc_uart", xbtc_uart_thread, NULL, BT_UART_THREAD_PROI, BT_UART_THREAD_SIZE);

	if (ret != XR_OS_OK) {
		XBTC_UART_TRACE_ERROR("thread create failed, reason is %d\n", ret);
		goto failed2;
	}

	bt_uart_id = id;

	return XBTC_UART_SUCCESS;

failed2:
	xbtc_uart_unregister_lib();

failed1:
	hal_uart_deinit(id);

	return XBTC_UART_FAIL;
}

void xbtc_uart_deinit(void)
{
	if (bt_uart_id == -1) {
		XBTC_UART_TRACE_ERROR("bt uart have not been init\n");
		return;
	}

	XR_OS_ThreadDelete(&g_xbtc_uart_thread);

	xbtc_uart_unregister_lib();

	hal_uart_deinit(bt_uart_id);

	bt_uart_id = -1;
}

int xbtc_uart_test(const uint8_t *data, uint16_t len)
{
	if (bt_uart_id == -1) {
		XBTC_UART_TRACE_ERROR("bt uart have not been init\n");
		return XBTC_UART_FAIL;
	}

	if (hal_uart_send(bt_uart_id, data, len) != len) {
		return XBTC_UART_FAIL;
	}
#if DBG_XBTC_UART
		XBTC_UART_TRACE_DEBUG("test send:");
		for (int i = 0; i < len; i++) {
			XBTC_UART_TRACE_DEBUG("%x ", data[i]);
		}
		XBTC_UART_TRACE_DEBUG("\n");
#endif
	return XBTC_UART_SUCCESS;

}
