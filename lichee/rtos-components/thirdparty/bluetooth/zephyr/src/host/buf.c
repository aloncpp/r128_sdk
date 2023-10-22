/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/buf.h>
#include <bluetooth/l2cap.h>

#include "hci_core.h"
#include "conn_internal.h"
#include "iso_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_CORE)
#define LOG_MODULE_NAME bt_buf
#include "common/log.h"

NET_BUF_POOL_FIXED_DEFINE(hci_rx_pool, CONFIG_BT_RX_BUF_COUNT,
			  BT_BUF_RX_SIZE, NULL);

#if defined(CONFIG_BT_CONN)
#define NUM_COMLETE_EVENT_SIZE BT_BUF_SIZE(\
	sizeof(struct bt_hci_evt_hdr) +                                \
	sizeof(struct bt_hci_cp_host_num_completed_packets) +          \
	CONFIG_BT_MAX_CONN * sizeof(struct bt_hci_handle_count))
/* Dedicated pool for HCI_Number_of_Completed_Packets. This event is always
 * consumed synchronously by bt_recv_prio() so a single buffer is enough.
 * Having a dedicated pool for it ensures that exhaustion of the RX pool
 * cannot block the delivery of this priority event.
 */
NET_BUF_POOL_FIXED_DEFINE(num_complete_pool, 1, NUM_COMLETE_EVENT_SIZE, NULL);
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_DISCARDABLE_BUF_COUNT)
#define DISCARDABLE_EVENT_SIZE BT_BUF_SIZE(CONFIG_BT_DISCARDABLE_BUF_SIZE)
NET_BUF_POOL_FIXED_DEFINE(discardable_pool, CONFIG_BT_DISCARDABLE_BUF_COUNT,
			  DISCARDABLE_EVENT_SIZE, NULL);
#endif /* CONFIG_BT_DISCARDABLE_BUF_COUNT */

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
#define ACL_IN_SIZE BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_RX_MTU)
NET_BUF_POOL_DEFINE(acl_in_pool, CONFIG_BT_ACL_RX_COUNT, ACL_IN_SIZE,
		    sizeof(struct acl_data), bt_hci_host_num_completed_packets);
#endif /* CONFIG_BT_HCI_ACL_FLOW_CONTROL */

struct net_buf *bt_buf_get_rx(enum bt_buf_type type, k_timeout_t timeout)
{
	struct net_buf *buf;

	__ASSERT(type == BT_BUF_EVT || type == BT_BUF_ACL_IN ||
		 type == BT_BUF_ISO_IN, "Invalid buffer type requested");

	if (IS_ENABLED(CONFIG_BT_ISO) && type == BT_BUF_ISO_IN) {
		return bt_iso_get_rx(timeout);
	}

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	if (type == BT_BUF_EVT) {
		buf = net_buf_alloc(&hci_rx_pool, timeout);
	} else {
		buf = net_buf_alloc(&acl_in_pool, timeout);
	}
#else
	buf = net_buf_alloc(&hci_rx_pool, timeout);
#endif

	if (buf) {
		net_buf_reserve(buf, BT_BUF_RESERVE);
		bt_buf_set_type(buf, type);
	}

	return buf;
}

#ifndef CONFIG_BT_DUAL_HOST
struct net_buf *bt_buf_get_cmd_complete(k_timeout_t timeout)
#else
struct net_buf *bt_buf_get_cmd_complete(uint16_t *opcode, k_timeout_t timeout)
#endif
{
	struct net_buf *buf;
	unsigned int key;

#ifdef CONFIG_BT_DUAL_HOST
    if (opcode != NULL && bt_hci_get_sent_cmd_opcode() != *opcode) {
		//printf("drop cmd cmp sent 0x%x, cmp 0x%x\n", bt_hci_get_sent_cmd_opcode(), *opcode);
        return NULL;
    }
#endif

	key = irq_lock();
	buf = bt_dev.sent_cmd;
	bt_dev.sent_cmd = NULL;
	irq_unlock(key);

	BT_DBG("sent_cmd %p", buf);

	if (buf) {
		bt_buf_set_type(buf, BT_BUF_EVT);
		buf->len = 0U;
		net_buf_reserve(buf, BT_BUF_RESERVE);

		return buf;
	}

	return bt_buf_get_rx(BT_BUF_EVT, timeout);
}

struct net_buf *bt_buf_get_evt(uint8_t evt, bool discardable,
			       k_timeout_t timeout)
{
	switch (evt) {
#if defined(CONFIG_BT_CONN)
	case BT_HCI_EVT_NUM_COMPLETED_PACKETS:
		{
			struct net_buf *buf;

			buf = net_buf_alloc(&num_complete_pool, timeout);
			if (buf) {
				net_buf_reserve(buf, BT_BUF_RESERVE);
				bt_buf_set_type(buf, BT_BUF_EVT);
			}

			return buf;
		}
#endif /* CONFIG_BT_CONN */
	case BT_HCI_EVT_CMD_COMPLETE:
	case BT_HCI_EVT_CMD_STATUS:
#ifndef CONFIG_BT_DUAL_HOST
		return bt_buf_get_cmd_complete(timeout);
#else
		return bt_buf_get_cmd_complete(NULL, timeout);
#endif
	default:
#if defined(CONFIG_BT_DISCARDABLE_BUF_COUNT)
		if (discardable) {
			struct net_buf *buf;

			buf = net_buf_alloc(&discardable_pool, timeout);
			if (buf) {
				net_buf_reserve(buf, BT_BUF_RESERVE);
				bt_buf_set_type(buf, BT_BUF_EVT);
			}

			return buf;
		}
#endif /* CONFIG_BT_DISCARDABLE_BUF_COUNT */

		return bt_buf_get_rx(BT_BUF_EVT, timeout);
	}
}

#if defined(CONFIG_BT_DEINIT)
void bt_buf_reset(void)
{
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	NET_BUF_POOL_FIXED_DEINIT(&acl_in_pool, CONFIG_BT_ACL_RX_COUNT, ACL_IN_SIZE, report_completed_packet);
#endif

	NET_BUF_POOL_FIXED_DEINIT(&hci_rx_pool, CONFIG_BT_RX_BUF_COUNT,
		BT_BUF_RX_SIZE, NULL);

#if defined(CONFIG_BT_CONN)
	NET_BUF_POOL_FIXED_DEINIT(&num_complete_pool, 1, NUM_COMLETE_EVENT_SIZE, NULL);
#endif

#if defined(CONFIG_BT_DISCARDABLE_BUF_COUNT)
	NET_BUF_POOL_FIXED_DEINIT(&discardable_pool, CONFIG_BT_DISCARDABLE_BUF_COUNT,
		DISCARDABLE_EVENT_SIZE, NULL);
#endif
}
#endif
