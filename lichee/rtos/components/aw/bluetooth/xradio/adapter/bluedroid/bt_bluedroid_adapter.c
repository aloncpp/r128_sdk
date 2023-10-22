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

#include "vendor.h"
#include "bt_lib.h"

extern int hci_hal_h4_recv(const uint8_t *data, uint16_t len);

static const bt_lib_interface_t *bt_lib_if = NULL;
static int bt_bluedroid_id;

static int bluedroid_data_ind(const uint8_t *data, uint16_t len)
{
	return hci_hal_h4_recv(data, len);
}

static bt_hc_callbacks_t bluedroid_hc_callbacks = {
	.data_ind = bluedroid_data_ind,
};

static bool vendor_open(const uint8_t *local_bdaddr, const hci_t *hci_interface)
{
	bt_lib_if = bt_lib_get_interface();
	if (bt_lib_if && bt_lib_if->hci_ops && bt_lib_if->hci_ops->open) {
		bt_bluedroid_id = bt_lib_if->hci_ops->open(BT_FEATURES_BR, &bluedroid_hc_callbacks);
		if (bt_bluedroid_id >= 0)
			return true;
	}
	return false;
}

static void vendor_close(void)
{
	bt_lib_if = bt_lib_get_interface();
	bt_lib_if->hci_ops->close(bt_bluedroid_id);
}

static int vendor_send_command(vendor_opcode_t opcode, void *param)
{
	switch (opcode)
	{
		case VENDOR_CHIP_POWER_CONTROL:
			break;
		case VENDOR_OPEN_USERIAL:
			break;
		case VENDOR_CLOSE_USERIAL:
			break;
		case VENDOR_GET_LPM_IDLE_TIMEOUT:
			break;
		case VENDOR_SET_LPM_WAKE_STATE:
			break;
		case VENDOR_SET_AUDIO_STATE:
			break;
		default :
			break;

	}
	return 0;
}

static int vendor_transmit(uint8_t *buf, int32_t size)
{
	bt_lib_if->hci_ops->write(bt_bluedroid_id, *buf, buf + 1, size - 1);
}

static int vendor_receive(uint8_t *buf, int32_t size, uint32_t msec)
{
	return 0;
}

static int vendor_send_async_command(vendor_async_opcode_t opcode, void *param)
{
	switch (opcode)
	{
		case VENDOR_CONFIGURE_FIRMWARE:
			break;
		case VENDOR_CONFIGURE_SCO:
			break;
		case VENDOR_SET_LPM_MODE:
			break;
		case VENDOR_DO_EPILOG:
			break;
		default :
			break;
	}
	return 0;
}

static void vendor_set_callback(vendor_async_opcode_t opcode, vendor_cb callback)
{
	if (opcode >= VENDOR_LAST_OP)
		return;
}

static const vendor_t xradio_vendor = {
	.open = vendor_open,
	.close = vendor_close,
	.send_command = vendor_send_command,
	.send_async_command = vendor_send_async_command,
	.transmit = vendor_transmit,
	.receive = vendor_receive,
	.set_callback = vendor_set_callback,
};

const struct vendor_t *bt_ctrl_get_bluedroid_interface(void)
{
	return &xradio_vendor;
}

