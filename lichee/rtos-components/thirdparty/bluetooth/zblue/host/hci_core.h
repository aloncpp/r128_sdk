/* hci_core.h - Bluetooth HCI core access */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* LL connection parameters */
#define LE_CONN_LATENCY		0x0000
#define LE_CONN_TIMEOUT		0x002a

#if defined(CONFIG_BT_BREDR)
#define LMP_FEAT_PAGES_COUNT	3
#else
#define LMP_FEAT_PAGES_COUNT	1
#endif

/* SCO  settings */
#define BT_VOICE_CVSD_16BIT     0x0060

/* k_poll event tags */
enum {
	BT_EVENT_CMD_TX,
	BT_EVENT_CONN_TX_QUEUE,
};

/* bt_dev flags: the flags defined here represent BT controller state */
enum {
	BT_DEV_ENABLE,
	BT_DEV_READY,
	BT_DEV_PRESET_ID,
	BT_DEV_HAS_PUB_KEY,
	BT_DEV_PUB_KEY_BUSY,

	BT_DEV_SCANNING,
	BT_DEV_EXPLICIT_SCAN,
	BT_DEV_ACTIVE_SCAN,
	BT_DEV_SCAN_FILTER_DUP,
	BT_DEV_SCAN_WL,
	BT_DEV_SCAN_LIMITED,
	BT_DEV_INITIATING,

	BT_DEV_RPA_VALID,
	BT_DEV_RPA_TIMEOUT_SET,

	BT_DEV_ID_PENDING,
	BT_DEV_STORE_ID,

#if defined(CONFIG_BT_BREDR)
	BT_DEV_ISCAN,
	BT_DEV_PSCAN,
	BT_DEV_INQUIRY,
#endif /* CONFIG_BT_BREDR */

	/* Total number of flags - must be at the end of the enum */
	BT_DEV_NUM_FLAGS,
};

/* Flags which should not be cleared upon HCI_Reset */
#define BT_DEV_PERSISTENT_FLAGS (BIT(BT_DEV_ENABLE) | \
				 BIT(BT_DEV_PRESET_ID))

enum {
	/* Advertising set has been created in the host. */
	BT_ADV_CREATED,
	/* Advertising parameters has been set in the controller.
	 * This implies that the advertising set has been created in the
	 * controller.
	 */
	BT_ADV_PARAMS_SET,
	/* Advertising data has been set in the controller. */
	BT_ADV_DATA_SET,
	/* Advertising random address pending to be set in the controller. */
	BT_ADV_RANDOM_ADDR_PENDING,
	/* The private random address of the advertiser is valid for this cycle
	 * of the RPA timeout.
	 */
	BT_ADV_RPA_VALID,
	/* The advertiser set is limited by a timeout, or number of advertising
	 * events, or both.
	 */
	BT_ADV_LIMITED,
	/* Advertiser set is currently advertising in the controller. */
	BT_ADV_ENABLED,
	/* Advertiser should include name in advertising data */
	BT_ADV_INCLUDE_NAME,
	/* Advertiser set is connectable */
	BT_ADV_CONNECTABLE,
	/* Advertiser set is scannable */
	BT_ADV_SCANNABLE,
	/* Advertiser set has disabled the use of private addresses and is using
	 * the identity address instead.
	 */
	BT_ADV_USE_IDENTITY,
	/* Advertiser has been configured to keep advertising after a connection
	 * has been established as long as there are connections available.
	 */
	BT_ADV_PERSIST,
	/* Advertiser has been temporarily disabled. */
	BT_ADV_PAUSED,

	BT_ADV_NUM_FLAGS,
};

struct bt_le_ext_adv {
	/* ID Address used for advertising */
	uint8_t                    id;

	/* Advertising handle */
	uint16_t			handle;

	/* Current local Random Address */
	bt_addr_le_t		random_addr;

	/* Current target address */
	bt_addr_le_t            target_addr;

	ATOMIC_DEFINE(flags, BT_ADV_NUM_FLAGS);

#if defined(CONFIG_BT_EXT_ADV)
	const struct bt_le_ext_adv_cb *cb;

	/* TX Power in use by the controller */
	int8_t                    tx_power;
#endif /* defined(CONFIG_BT_EXT_ADV) */
};

struct bt_dev_le {
	/* LE features */
	uint8_t			features[8];
	/* LE states */
	uint64_t			states;

#if defined(CONFIG_BT_CONN)
	/* Controller buffer information */
	uint16_t			mtu;
	struct k_sem		pkts;
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_SMP)
	/* Size of the the controller resolving list */
	uint8_t                    rl_size;
	/* Number of entries in the resolving list. rl_entries > rl_size
	 * means that host-side resolving is used.
	 */
	uint8_t                    rl_entries;
#endif /* CONFIG_BT_SMP */
};

#if defined(CONFIG_BT_BREDR)
struct bt_dev_br {
	/* Max controller's acceptable ACL packet length */
	uint16_t         mtu;
	struct k_sem  pkts;
	uint16_t         esco_pkt_type;
};
#endif

/* The theoretical max for these is 8 and 64, but there's no point
 * in allocating the full memory if we only support a small subset.
 * These values must be updated whenever the host implementation is
 * extended beyond the current values.
 */
#define BT_DEV_VS_FEAT_MAX  1
#define BT_DEV_VS_CMDS_MAX  2

/* State tracking for the local Bluetooth controller */
struct bt_dev {
	/* Local Identity Address(es) */
	bt_addr_le_t            id_addr[CONFIG_BT_ID_MAX];
	uint8_t                    id_count;

	struct bt_conn_le_create_param create_param;

#if !defined(CONFIG_BT_EXT_ADV)
	/* Legacy advertiser */
	struct bt_le_ext_adv    adv;
#else
	/* Pointer to reserved advertising set */
	struct bt_le_ext_adv    *adv;
#endif
	/* Current local Random Address */
	bt_addr_le_t            random_addr;
	uint8_t                    adv_conn_id;

	/* Controller version & manufacturer information */
	uint8_t			hci_version;
	uint8_t			lmp_version;
	uint16_t			hci_revision;
	uint16_t			lmp_subversion;
	uint16_t			manufacturer;

	/* LMP features (pages 0, 1, 2) */
	uint8_t			features[LMP_FEAT_PAGES_COUNT][8];

	/* Supported commands */
	uint8_t			supported_commands[64];

#if defined(CONFIG_BT_HCI_VS_EXT)
	/* Vendor HCI support */
	uint8_t                    vs_features[BT_DEV_VS_FEAT_MAX];
	uint8_t                    vs_commands[BT_DEV_VS_CMDS_MAX];
#endif

	struct k_work           init;

	ATOMIC_DEFINE(flags, BT_DEV_NUM_FLAGS);

	/* LE controller specific features */
	struct bt_dev_le	le;

#if defined(CONFIG_BT_BREDR)
	/* BR/EDR controller specific features */
	struct bt_dev_br	br;
#endif

	/* Number of commands controller can accept */
	struct k_sem		ncmd_sem;

	/* Last sent HCI command */
	struct net_buf		*sent_cmd;

#if !defined(CONFIG_BT_RECV_IS_RX_THREAD)
	/* Queue for incoming HCI events & ACL data */
	struct k_fifo		rx_queue;
#endif

	/* Queue for outgoing HCI commands */
	struct k_fifo		cmd_tx_queue;

	/* Registered HCI driver */
	const struct bt_hci_driver *drv;

#if defined(CONFIG_BT_PRIVACY)
	/* Local Identity Resolving Key */
	uint8_t			irk[CONFIG_BT_ID_MAX][16];

	/* Work used for RPA rotation */
	struct k_delayed_work rpa_update;
#endif

	/* Local Name */
#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
	char			name[CONFIG_BT_DEVICE_NAME_MAX + 1];
#endif
};

extern struct bt_dev bt_dev;
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
extern const struct bt_conn_auth_cb *bt_auth;
#endif /* CONFIG_BT_SMP || CONFIG_BT_BREDR */


#if defined(CONFIG_BT_DEINIT)
int bt_hci_tx_thread_deinit(void);
int bt_hci_rx_thread_deinit(void);
void tx_deinit_poll_event_init(struct k_poll_event *events);
#endif

#if defined(CONFIG_BT_VAR_MEM_DYNC_ALLOC)
void bt_host_variable_mem_alloc(void);
void bt_host_variable_mem_free(void);
#endif

int bt_hci_disconnect(uint16_t handle, uint8_t reason);

bool bt_le_conn_params_valid(const struct bt_le_conn_param *param);
int bt_le_set_data_len(struct bt_conn *conn, uint16_t tx_octets, uint16_t tx_time);
int bt_le_set_phy(struct bt_conn *conn, uint8_t all_phys,
		  uint8_t pref_tx_phy, uint8_t pref_rx_phy, uint8_t phy_opts);

int bt_le_scan_update(bool fast_scan);

int bt_le_create_conn(const struct bt_conn *conn);
int bt_le_create_conn_cancel(void);

bool bt_addr_le_is_bonded(uint8_t id, const bt_addr_le_t *addr);
const bt_addr_le_t *bt_lookup_id_addr(uint8_t id, const bt_addr_le_t *addr);

int bt_send(struct net_buf *buf);

/* Don't require everyone to include keys.h */
struct bt_keys;
void bt_id_add(struct bt_keys *keys);
void bt_id_del(struct bt_keys *keys);

int bt_setup_random_id_addr(void);
void bt_setup_public_id_addr(void);

void bt_finalize_init(void);

int bt_le_adv_start_internal(const struct bt_le_adv_param *param,
			     const struct bt_data *ad, size_t ad_len,
			     const struct bt_data *sd, size_t sd_len,
			     const bt_addr_le_t *peer);

void bt_le_adv_resume(void);
bool bt_le_scan_random_addr_check(void);

int bt_hci_driver_register(const struct bt_hci_driver *drv);
