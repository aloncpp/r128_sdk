/**
 * @file hci_ecc.c
 * HCI ECC emulation
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ble/sys/atomic.h>
//#include <debug/stack.h>
#include <ble/sys/byteorder.h>
#include <tinycrypt/constants.h>
#include <tinycrypt/utils.h>
#include <tinycrypt/ecc.h>
#include <tinycrypt/ecc_dh.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_driver.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_CORE)
#define LOG_MODULE_NAME bt_hci_ecc
#include "common/log.h"

#include "hci_ecc.h"
#ifdef CONFIG_BT_HCI_RAW
#include <bluetooth/hci_raw.h>
#include "hci_raw_internal.h"
#else
#include "hci_core.h"
#endif

#ifdef CONFIG_COMPONENTS_BT_PM
#include "bt_pm.h"
#endif

static struct k_thread ecc_thread_data;
static K_KERNEL_STACK_DEFINE(ecc_thread_stack, CONFIG_BT_HCI_ECC_STACK_SIZE);

/* based on Core Specification 4.2 Vol 3. Part H 2.3.5.6.1 */
static const uint8_t debug_private_key_be[32] = {
	0x3f, 0x49, 0xf6, 0xd4, 0xa3, 0xc5, 0x5f, 0x38,
	0x74, 0xc9, 0xb3, 0xe3, 0xd2, 0x10, 0x3f, 0x50,
	0x4a, 0xff, 0x60, 0x7b, 0xeb, 0x40, 0xb7, 0x99,
	0x58, 0x99, 0xb8, 0xa6, 0xcd, 0x3c, 0x1a, 0xbd,
};

enum {
	PENDING_PUB_KEY,
	PENDING_DHKEY,

	USE_DEBUG_KEY,

#if defined(CONFIG_BT_DEINIT)
	PENDING_ECC_DEINIT,
#endif

	/* Total number of flags - must be at the end of the enum */
	NUM_FLAGS,
};

static ATOMIC_DEFINE(flags, NUM_FLAGS);

#if 1 //Wewisetek Modified
struct k_sem		cmd_sem;
#else //Origin
static K_SEM_DEFINE(cmd_sem, 0, 1);
#endif

static struct {
	uint8_t private_key_be[32];

	union {
		uint8_t public_key_be[64];
		uint8_t dhkey_be[32];
	};
} ecc;

static void send_cmd_status(uint16_t opcode, uint8_t status)
{
	struct bt_hci_evt_cmd_status *evt;
	struct bt_hci_evt_hdr *hdr;
	struct net_buf *buf;

	BT_DBG("opcode %x status %x", opcode, status);

	buf = bt_buf_get_evt(BT_HCI_EVT_CMD_STATUS, false, K_FOREVER);
	bt_buf_set_type(buf, BT_BUF_EVT);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = BT_HCI_EVT_CMD_STATUS;
	hdr->len = sizeof(*evt);

	evt = net_buf_add(buf, sizeof(*evt));
	evt->ncmd = 1U;
	evt->opcode = sys_cpu_to_le16(opcode);
	evt->status = status;

	if (IS_ENABLED(CONFIG_BT_RECV_IS_RX_THREAD)) {
		bt_recv_prio(buf);
	} else {
		bt_recv(buf);
	}
}

static uint8_t generate_keys(void)
{
	do {
		int rc;

		rc = uECC_make_key(ecc.public_key_be, ecc.private_key_be,
				   &curve_secp256r1);
		if (rc == TC_CRYPTO_FAIL) {
			BT_ERR("Failed to create ECC public/private pair");
			return BT_HCI_ERR_UNSPECIFIED;
		}

	/* make sure generated key isn't debug key */
	} while (memcmp(ecc.private_key_be, debug_private_key_be, 32) == 0);

	return 0;
}

static void emulate_le_p256_public_key_cmd(void)
{
	struct bt_hci_evt_le_p256_public_key_complete *evt;
	struct bt_hci_evt_le_meta_event *meta;
	struct bt_hci_evt_hdr *hdr;
	struct net_buf *buf;
	uint8_t status;

	BT_DBG("");

	status = generate_keys();

	buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = BT_HCI_EVT_LE_META_EVENT;
	hdr->len = sizeof(*meta) + sizeof(*evt);

	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = BT_HCI_EVT_LE_P256_PUBLIC_KEY_COMPLETE;

	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = status;

	if (status) {
		(void)memset(evt->key, 0, sizeof(evt->key));
	} else {
		/* Convert X and Y coordinates from big-endian (provided
		 * by crypto API) to little endian HCI.
		 */
		sys_memcpy_swap(evt->key, ecc.public_key_be, 32);
		sys_memcpy_swap(&evt->key[32], &ecc.public_key_be[32], 32);
	}

	atomic_clear_bit(flags, PENDING_PUB_KEY);

	bt_recv(buf);
}

static void emulate_le_generate_dhkey(void)
{
	struct bt_hci_evt_le_generate_dhkey_complete *evt;
	struct bt_hci_evt_le_meta_event *meta;
	struct bt_hci_evt_hdr *hdr;
	struct net_buf *buf;
	int ret;

	ret = uECC_valid_public_key(ecc.public_key_be, &curve_secp256r1);
	if (ret < 0) {
		BT_ERR("public key is not valid (ret %d)", ret);
		ret = TC_CRYPTO_FAIL;
	} else {
		bool use_debug = atomic_test_bit(flags, USE_DEBUG_KEY);

		ret = uECC_shared_secret(ecc.public_key_be,
					 use_debug ? debug_private_key_be :
						     ecc.private_key_be,
					 ecc.dhkey_be, &curve_secp256r1);
	}

	buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = BT_HCI_EVT_LE_META_EVENT;
	hdr->len = sizeof(*meta) + sizeof(*evt);

	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = BT_HCI_EVT_LE_GENERATE_DHKEY_COMPLETE;

	evt = net_buf_add(buf, sizeof(*evt));

	if (ret == TC_CRYPTO_FAIL) {
		evt->status = BT_HCI_ERR_UNSPECIFIED;
		(void)memset(evt->dhkey, 0xff, sizeof(evt->dhkey));
	} else {
		evt->status = 0U;
		/* Convert from big-endian (provided by crypto API) to
		 * little-endian HCI.
		 */
		sys_memcpy_swap(evt->dhkey, ecc.dhkey_be, sizeof(ecc.dhkey_be));
	}

	atomic_clear_bit(flags, PENDING_DHKEY);

	bt_recv(buf);
}

static void ecc_thread(void *p1, void *p2, void *p3)
{
	while (true) {
		k_sem_take(&cmd_sem, K_FOREVER);

#ifdef CONFIG_COMPONENTS_BT_PM
		bt_pm_lock();
#endif

		if (atomic_test_bit(flags, PENDING_PUB_KEY)) {
			emulate_le_p256_public_key_cmd();
		} else if (atomic_test_bit(flags, PENDING_DHKEY)) {
			emulate_le_generate_dhkey();
#if defined(CONFIG_BT_DEINIT)
		} else if (atomic_test_bit(flags, PENDING_ECC_DEINIT)) {
#ifdef CONFIG_COMPONENTS_BT_PM
			bt_pm_unlock();
#endif
			break;
#endif
		} else {
			__ASSERT(0, "Unhandled ECC command");
		}

#ifdef CONFIG_COMPONENTS_BT_PM
		bt_pm_unlock();
#endif
	}

#if defined(CONFIG_BT_DEINIT)
	atomic_clear_bit(flags, PENDING_ECC_DEINIT);
#endif
}

static void clear_ecc_events(struct net_buf *buf)
{
	struct bt_hci_cp_le_set_event_mask *cmd;

	cmd = (void *)(buf->data + sizeof(struct bt_hci_cmd_hdr));

	/*
	 * don't enable controller ECC events as those will be generated from
	 * emulation code
	 */
	cmd->events[0] &= ~0x80; /* LE Read Local P-256 PKey Compl */
	cmd->events[1] &= ~0x01; /* LE Generate DHKey Compl Event */
}

static uint8_t le_gen_dhkey(uint8_t *key, uint8_t key_type)
{
	if (atomic_test_bit(flags, PENDING_PUB_KEY)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (key_type > BT_HCI_LE_KEY_TYPE_DEBUG) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	if (atomic_test_and_set_bit(flags, PENDING_DHKEY)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Convert X and Y coordinates from little-endian HCI to
	 * big-endian (expected by the crypto API).
	 */
	sys_memcpy_swap(ecc.public_key_be, key, 32);
	sys_memcpy_swap(&ecc.public_key_be[32], &key[32], 32);

	atomic_set_bit_to(flags, USE_DEBUG_KEY,
			  key_type == BT_HCI_LE_KEY_TYPE_DEBUG);

	k_sem_give(&cmd_sem);

	return BT_HCI_ERR_SUCCESS;
}

static void le_gen_dhkey_v1(struct net_buf *buf)
{
	struct bt_hci_cp_le_generate_dhkey *cmd;
	uint8_t status;

	cmd = (void *)buf->data;
	status = le_gen_dhkey(cmd->key, BT_HCI_LE_KEY_TYPE_GENERATED);

	net_buf_unref(buf);
	send_cmd_status(BT_HCI_OP_LE_GENERATE_DHKEY, status);
}

static void le_gen_dhkey_v2(struct net_buf *buf)
{
	struct bt_hci_cp_le_generate_dhkey_v2 *cmd;
	uint8_t status;

	cmd = (void *)buf->data;
	status = le_gen_dhkey(cmd->key, cmd->key_type);

	net_buf_unref(buf);
	send_cmd_status(BT_HCI_OP_LE_GENERATE_DHKEY_V2, status);
}

static void le_p256_pub_key(struct net_buf *buf)
{
	uint8_t status;

	net_buf_unref(buf);

	if (atomic_test_bit(flags, PENDING_DHKEY)) {
		status = BT_HCI_ERR_CMD_DISALLOWED;
	} else if (atomic_test_and_set_bit(flags, PENDING_PUB_KEY)) {
		status = BT_HCI_ERR_CMD_DISALLOWED;
	} else {
		k_sem_give(&cmd_sem);
		status = BT_HCI_ERR_SUCCESS;
	}

	send_cmd_status(BT_HCI_OP_LE_P256_PUBLIC_KEY, status);
}

int bt_hci_ecc_send(struct net_buf *buf)
{
	if (bt_buf_get_type(buf) == BT_BUF_CMD) {
		struct bt_hci_cmd_hdr *chdr = (void *)buf->data;

		switch (sys_le16_to_cpu(chdr->opcode)) {
		case BT_HCI_OP_LE_P256_PUBLIC_KEY:
			net_buf_pull(buf, sizeof(*chdr));
			le_p256_pub_key(buf);
			return 0;
		case BT_HCI_OP_LE_GENERATE_DHKEY:
			net_buf_pull(buf, sizeof(*chdr));
			le_gen_dhkey_v1(buf);
			return 0;
		case BT_HCI_OP_LE_GENERATE_DHKEY_V2:
			net_buf_pull(buf, sizeof(*chdr));
			le_gen_dhkey_v2(buf);
			return 0;
		case BT_HCI_OP_LE_SET_EVENT_MASK:
			clear_ecc_events(buf);
			break;
		default:
			break;
		}
	}

	return bt_dev.drv->send(buf);
}

void bt_hci_ecc_supported_commands(uint8_t *supported_commands)
{
	/* LE Read Local P-256 Public Key */
	supported_commands[34] |= BIT(1);
	/* LE Generate DH Key v1 */
	supported_commands[34] |= BIT(2);
	/* LE Generate DH Key v2 */
	supported_commands[41] |= BIT(2);
}

int default_CSPRNG(uint8_t *dst, unsigned int len)
{
	return !bt_rand(dst, len);
}

void bt_hci_ecc_init(void)
{
#if 1 //Wewisetek Modified
	k_sem_init(&cmd_sem, 0, 1);
#endif
	k_thread_create(&ecc_thread_data, ecc_thread_stack,
			K_KERNEL_STACK_SIZEOF(ecc_thread_stack), ecc_thread,
			NULL, NULL, NULL, K_PRIO_PREEMPT(2), 0, K_NO_WAIT);
}

#if defined(CONFIG_BT_DEINIT)
int bt_hci_ecc_deinit(void)
{
	int ret;

	if (atomic_test_bit(flags, PENDING_ECC_DEINIT)) {
		ret = -EALREADY;
		return ret;
	}

	while (atomic_test_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY) ||
		(bt_dh_key_cb_get() != NULL)) {
		k_sleep(2);
	}

	atomic_set_bit(flags, PENDING_ECC_DEINIT);

	k_sem_give(&cmd_sem);

	k_thread_join(&ecc_thread_data, K_FOREVER);

	memset(&ecc, 0, sizeof(ecc));

	atomic_clear(flags);

	k_sem_delete(&cmd_sem);

	return 0;
}
#endif
