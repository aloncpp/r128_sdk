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

#ifdef CONFIG_BLEHOST
#include <stdint.h>
#include "ble/drivers/bluetooth/hci_driver.h"
#include "zephyr.h"
#include <stdio.h>

#include "kernel/os/os.h"
#include "net/buf.h"

#include "bt_lib.h"

#define H4_NONE 0x00
#define H4_CMD  0x01
#define H4_ACL  0x02
#define H4_SCO  0x03
#define H4_EVT  0x04

static const bt_lib_interface_t *bt_lib_if = NULL;
static int bt_zephyr_id;

static struct {
	uint8_t type;
	struct net_buf *buf;
	struct k_fifo   fifo;
} tx = {
	.fifo = Z_FIFO_INITIALIZER(tx.fifo),
};

static int virtual_hci_h2c(struct net_buf *buf)
{
	unsigned char status = 0;
	unsigned char h2c_type = 0xFF;
	//struct net_buf *buf = (struct net_buf *)b;

	if ((bt_lib_if != NULL) && !bt_lib_if->hci_ctrl_ops->status())
		return -ENODEV;

	uint8_t event_type = bt_buf_get_type(buf);


	net_buf_put(&tx.fifo, buf);
	if (!tx.buf) {
		tx.buf = net_buf_get(&tx.fifo, K_NO_WAIT);
		if (!tx.buf) {
			printf("TX no pending buffer!");
			return 1;
		}
	}

	HOSTMINI_LOG("[H2C] %d ", tx.buf->len);

	switch (event_type) {
	case BT_BUF_CMD:
		h2c_type = H4_CMD; // cmd
		HOSTMINI_LOG("Opcode %02x%02x", tx.buf->data[1], tx.buf->data[0]);
		break;
	case BT_BUF_ACL_OUT:
		h2c_type = H4_ACL; // acl out
		break;
	default:
		//ASSERT(0);
		break;
	}

	HOSTMINI_LOG("\n");

#if CONFIG_BT_DEBUG_LOG_WITH_HCI_PRINT
	int i;
	printf("[h2c] data : ");
	for( i = 0; i < tx.buf->len; i++ )
		printf("0x%02x ", *(tx.buf->data+i));
	printf("\n");
#endif

	if ((h2c_type == H4_CMD) || (h2c_type == H4_ACL)) {
		status = bt_lib_if->hci_ops->write(bt_zephyr_id, h2c_type, tx.buf->data, tx.buf->len);
		if (status != 0) {
			printf("h2c err %d %d!\n", event_type, status);
		}

		net_buf_unref(tx.buf);
		tx.buf = net_buf_get(&tx.fifo, K_NO_WAIT);
	} else {
		printf("h2c err h2c_type %d!\n", h2c_type);
	}

	return 0;
}

static bool rx_event_discardable(const uint8_t *evt_data)
{
	uint8_t evt_type = evt_data[0];

	switch (evt_type) {
	case BT_HCI_EVT_LE_META_EVENT: {
		uint8_t subevt_type = evt_data[sizeof(struct bt_hci_evt_hdr)];

		switch (subevt_type) {
		case BT_HCI_EVT_LE_ADVERTISING_REPORT:
			return true;
		default:
			return false;
		}
	}
	default:
		return false;
	}
}

static struct net_buf *get_rx(uint8_t type, const uint8_t *buf)
{
	bool discardable = false;

	if (type == H4_EVT && (buf[0] == BT_HCI_EVT_CMD_COMPLETE ||
				 buf[0] == BT_HCI_EVT_CMD_STATUS)) {
		uint16_t opcode;
		if (buf[0] == BT_HCI_EVT_CMD_COMPLETE)
			opcode = buf[4] << 8 | buf[3];
		else
			opcode = buf[5] << 8 | buf[4];
#ifndef CONFIG_BT_DUAL_HOST
		struct net_buf *cmd_cpl = bt_buf_get_cmd_complete(K_FOREVER);
#else
		struct net_buf *cmd_cpl = bt_buf_get_cmd_complete(&opcode, K_FOREVER);
#endif
		if (cmd_cpl != NULL)
			net_buf_reset(cmd_cpl);
		return cmd_cpl;
	}

	if (type == H4_ACL) {
		return bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	} else {
		discardable = rx_event_discardable(buf);
		return bt_buf_get_evt(BT_BUF_EVT, discardable, discardable ? K_NO_WAIT : K_FOREVER);
	}
}

static int zephyr_data_ind(const uint8_t *data, uint16_t len)
{
	uint32_t buf_tailroom;
	struct net_buf *tmp_buf = NULL;
	len--;
	const uint8_t *buff = data + 1;

	uint8_t hci_type = data[0];

	if ((hci_type != H4_ACL) && (hci_type != H4_EVT)
#if (defined(CONFIG_BT_DUAL_HOST) && defined(CONFIG_BT_DIST_MODE_ALL))
		&& (hci_type != H4_SCO)
#endif
	) {
		printf("hci c2h hci type errr: %d\n", hci_type);
		return 1;
	}

	tmp_buf = get_rx(hci_type, buff);
	if (tmp_buf == NULL) {
		return 2;
	}

	buf_tailroom = net_buf_tailroom(tmp_buf);
	if (buf_tailroom < len) {
		printf("[Discard] not enough space in buffer, need %d, but %d!\n",
		          len, buf_tailroom);
		net_buf_unref(tmp_buf);
		return 3;
	}

	net_buf_add_mem(tmp_buf, buff, len);
#if CONFIG_BT_DEBUG_LOG_WITH_HCI_PRINT
	int i;
	printf("[c2h](%d, %d) : ", tmp_buf->len, len);
	for( i = 0; i < tmp_buf->len; i++ )
		printf("0x%02x ", *(tmp_buf->data+i));
	printf("\n");
#endif

	bt_recv(tmp_buf);
	return 0;
}

static bt_hc_callbacks_t zephyr_hc_callbacks = {
	.data_ind = zephyr_data_ind,
};

static int virtual_hci_open(void)
{
	bt_lib_if = bt_lib_get_interface();
	if (bt_lib_if && bt_lib_if->hci_ops && bt_lib_if->hci_ops->open) {
		bt_zephyr_id = bt_lib_if->hci_ops->open(BT_FEATURES_BLE, &zephyr_hc_callbacks);
		if (bt_zephyr_id >= 0)
			return 0;
	}
	return -1;
}

#if defined(CONFIG_BT_DEINIT)
static int virtual_hci_close(void)
{
	if (bt_lib_if && bt_lib_if->hci_ops && bt_lib_if->hci_ops->close) {
		bt_lib_if->hci_ops->close(bt_zephyr_id);
		bt_zephyr_id = -1;
		bt_lib_if = NULL;
	}
	return 0;
}
#endif

static const struct bt_hci_driver drv = {
	.name = "",
	.bus = BT_HCI_DRIVER_BUS_VIRTUAL,
	.open = virtual_hci_open,
#if defined(CONFIG_BT_DEINIT)
	.close = virtual_hci_close,
#endif
	.send = virtual_hci_h2c,
};

const struct bt_hci_driver * bt_ctrl_get_zephyr_interface(void)
{
	return &drv;
}

#endif /* CONFIG_BLEHOST */
