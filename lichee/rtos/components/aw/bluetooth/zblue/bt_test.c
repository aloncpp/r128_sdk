#include "console.h"
#include <bluetooth/bluetooth.h>
#include <bluetooth/gap.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <btsnoop.h>
#include <settings/settings.h>

#define DEVICE_NAME		CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)

#define NAME_LEN 30

#define CMD_GATT_VALUE_PRINT      1
#define CMD_GATT_WRITE_DYNAMIC    1
#define CMD_GATT_INDICATION       1
struct bt_conn *default_conn;

/* Connection context for BR/EDR legacy pairing in sec mode 3 */
static struct bt_conn *pairing_conn;

static struct bt_le_oob oob_local;
static struct bt_le_oob oob_remote;

static const struct bt_data ad_discov[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static int bt_le_adv_test(void)
{
	int err;
	struct bt_le_adv_param param = {};
	const struct bt_data *ad;
	size_t ad_len;

	param.id = 0;
	param.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
	param.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;

	param.options = (BT_LE_ADV_OPT_CONNECTABLE |
				 BT_LE_ADV_OPT_USE_NAME);

	ad = ad_discov;
	ad_len = ARRAY_SIZE(ad_discov);

	err = bt_le_adv_start(&param, ad, ad_len, NULL, 0);

	if (err < 0) {
		printf("Failed to start advertising (err %d)\n",err);
		return err;
	} else {
		printf("Advertising started\n");
	}

	return 0;
}
/* Custom Service Variables */
static struct bt_uuid_128 vnd_uuid = BT_UUID_INIT_128(
	0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
static struct bt_uuid_128 vnd_auth_uuid = BT_UUID_INIT_128(
	0xf2, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
static const struct bt_uuid_128 vnd_long_uuid1 = BT_UUID_INIT_128(
	0xf3, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
static const struct bt_uuid_128 vnd_long_uuid2 = BT_UUID_INIT_128(
	0xde, 0xad, 0xfa, 0xce, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static uint8_t vnd_value[] = { 'V', 'e', 'n', 'd', 'o', 'r' };

static struct bt_uuid_128 vnd1_uuid = BT_UUID_INIT_128(
	0xf4, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static const struct bt_uuid_128 vnd1_echo_uuid = BT_UUID_INIT_128(
	0xf5, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static uint8_t echo_enabled;
static void vnd1_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
#if !CMD_GATT_INDICATION
	echo_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
#else
    if (value) {
        if (value & BT_GATT_CCC_NOTIFY) {
            echo_enabled |= 0x1;
        }
        if (value & BT_GATT_CCC_INDICATE) {
            echo_enabled |= 0x2;
        }
    } else {
        echo_enabled = 0;
    }
#endif /* !CMD_GATT_INDICATION */
}

#if CMD_GATT_INDICATION
struct bt_gatt_indicate_params vnd1_indicate_params;

static void vnd1_indicate_cb(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, u8_t err)
{
    printk("Vnd1 Indication\n");
}
#endif

static ssize_t write_vnd1(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  const void *buf, uint16_t len, uint16_t offset,
			  uint8_t flags)
{
#if CMD_GATT_INDICATION
    if (echo_enabled & 0x2) {
        printf("Indicate Echo attr len %u\n", len);
        memset(&vnd1_indicate_params, 0, sizeof(vnd1_indicate_params));
        vnd1_indicate_params.attr = attr;
        vnd1_indicate_params.func = vnd1_indicate_cb;
        vnd1_indicate_params.data = buf;
        vnd1_indicate_params.len = len;
        bt_gatt_indicate(conn, &vnd1_indicate_params);
    }
	else
#endif
	if (echo_enabled) {
		printf("Echo attr len %u\n", len);
		bt_gatt_notify(conn, attr, buf, len);
	}

	return len;
}



static ssize_t read_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

static ssize_t write_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset + len > sizeof(vnd_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

#define MAX_DATA 30
static uint8_t vnd_long_value1[MAX_DATA] = { 'V', 'e', 'n', 'd', 'o', 'r' };
static uint8_t vnd_long_value2[MAX_DATA] = { 'S', 't', 'r', 'i', 'n', 'g' };

static ssize_t read_long_vnd(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	uint8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(vnd_long_value1));
}

static ssize_t write_long_vnd(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, const void *buf,
			      uint16_t len, uint16_t offset, uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (flags & BT_GATT_WRITE_FLAG_PREPARE) {
		return 0;
	}

	if (offset + len > sizeof(vnd_long_value1)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	/* Copy to buffer */
	memcpy(value + offset, buf, len);

	return len;
}


static struct bt_gatt_attr vnd_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&vnd_uuid),

	BT_GATT_CHARACTERISTIC(&vnd_auth_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_vnd, write_vnd, vnd_value),

	BT_GATT_CHARACTERISTIC(&vnd_long_uuid1.uuid, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_EXT_PROP,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE |
			       BT_GATT_PERM_PREPARE_WRITE,
			       read_long_vnd, write_long_vnd,
			       &vnd_long_value1),

	BT_GATT_CHARACTERISTIC(&vnd_long_uuid2.uuid, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_EXT_PROP,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE |
			       BT_GATT_PERM_PREPARE_WRITE,
			       read_long_vnd, write_long_vnd,
			       &vnd_long_value2),
};

static struct bt_gatt_service vnd_svc = BT_GATT_SERVICE(vnd_attrs);

static struct bt_gatt_attr vnd1_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&vnd1_uuid),

	BT_GATT_CHARACTERISTIC(&vnd1_echo_uuid.uuid,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP |
#if CMD_GATT_INDICATION
			       BT_GATT_CHRC_INDICATE |
#endif
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_WRITE, NULL, write_vnd1, NULL),
	BT_GATT_CCC(vnd1_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};

static struct bt_gatt_service vnd1_svc = BT_GATT_SERVICE(vnd1_attrs);



static int bt_gatt_register_svc_test(void)
{
	bt_gatt_service_register(&vnd_svc);
	bt_gatt_service_register(&vnd1_svc);
	return 0;
}
static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		memcpy(name, data->data, MIN(data->data_len, NAME_LEN - 1));
		return false;
	default:
		return true;
	}
}

static const char *phy2str(uint8_t phy)
{
	switch (phy) {
	case 0: return "No packets";
	case BT_GAP_LE_PHY_1M: return "LE 1M";
	case BT_GAP_LE_PHY_2M: return "LE 2M";
	case BT_GAP_LE_PHY_CODED: return "LE Coded";
	default: return "Unknown";
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN];

	(void)memset(name, 0, sizeof(name));

	bt_data_parse(buf, data_cb, name);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	printf("[DEVICE]: %s, AD evt type %u, RSSI %i %s "
		    "C:%u S:%u D:%d SR:%u E:%u Prim: %s, Secn: %s\n",
		    le_addr, info->adv_type, info->rssi, name,
		    (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0,
		    (info->adv_props & BT_GAP_ADV_PROP_SCANNABLE) != 0,
		    (info->adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0,
		    (info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE) != 0,
		    (info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0,
		    phy2str(info->primary_phy), phy2str(info->secondary_phy));
}

static void scan_timeout(void)
{
	printf("Scan timeout\n");
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
	.timeout = scan_timeout,
};

static void conn_addr_str(struct bt_conn *conn, char *addr, size_t len)
{
	struct bt_conn_info info;

	if (bt_conn_get_info(conn, &info) < 0) {
		addr[0] = '\0';
		return;
	}

	switch (info.type) {
	case BT_CONN_TYPE_LE:
		bt_addr_le_to_str(info.le.dst, addr, len);
		break;
	}
}


static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));

	if (err) {
		printf("Failed to connect to %s (0x%02x)\n", addr,
			     err);
		goto done;
	}

	printf("Connected: %s", addr);

	if (!default_conn) {
		default_conn = bt_conn_ref(conn);
	}

done:
	/* clear connection reference for sec mode 3 pairing */
	if (pairing_conn) {
		bt_conn_unref(pairing_conn);
		pairing_conn = NULL;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));
	printf("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	if (default_conn == conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));

	if (!err) {
		printf("Security changed: %s level %u\n", addr,
			    level);
	} else {
		printf("Security failed: %s level %u reason %d\n",
			    addr, level, err);
	}
}
static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
			      const bt_addr_le_t *identity)
{
	char addr_identity[BT_ADDR_LE_STR_LEN];
	char addr_rpa[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
	bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));

	printf("Identity resolved %s -> %s\n", addr_rpa,
	      addr_identity);
}


static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
#if defined(CONFIG_BT_SMP)
	.identity_resolved = identity_resolved,
#endif
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
	.security_changed = security_changed,
#endif
};

static void cmd_ble_test(int argc, char **argv)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printf("Bluetooth init failed (err %d)", err);
		return ;
	}
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	bt_le_adv_test();
	bt_gatt_register_svc_test();
	bt_le_scan_cb_register(&scan_callbacks);
}

static int scan_silent = 1;
static void cmd_ble_scan(int argc, char **argv)
{

	struct bt_le_scan_param param = {
		.type       = BT_LE_SCAN_TYPE_ACTIVE,
		.options    = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval   = BT_GAP_SCAN_FAST_INTERVAL,
		.window     = BT_GAP_SCAN_FAST_WINDOW,
		.timeout    = 0, };

	int i = 0;
	int err;

	scan_silent = 0;

	while (++i < argc) {

		if (!strcmp(argv[i], "on")) {
			//default
		} else if (!strcmp(argv[i], "off")) {
			err = bt_le_scan_stop();
			if (err) {
				printf("Stopping scanning failed (err %d)\n", err);
				return ;
			} else {
				printf("Scan successfully stopped\n");
			}

			return ;
		} else if (!strcmp(argv[i], "passive")) {
			param.type       = BT_LE_SCAN_TYPE_PASSIVE;
			param.options = BT_LE_SCAN_OPT_NONE;
			param.interval   = 0x10;
			param.window     = 0x10;
			param.timeout    = 0;
		} else if (!strcmp(argv[i], "dups")) {
			param.options &= ~BT_LE_SCAN_OPT_FILTER_DUPLICATE;
		} else if (!strcmp(argv[i], "nodups")) {
			param.options |= BT_LE_SCAN_OPT_FILTER_DUPLICATE;
		} else if (!strcmp(argv[i], "wl")) {
			param.options |= BT_LE_SCAN_OPT_FILTER_WHITELIST;
		} else if (!strcmp(argv[i], "active")) {
			param.type = BT_LE_SCAN_TYPE_ACTIVE;
		} else if (!strcmp(argv[i], "coded")) {
			param.options |= BT_LE_SCAN_OPT_CODED;
		} else if (!strcmp(argv[i], "no-1m")) {
			param.options |= BT_LE_SCAN_OPT_NO_1M;
		} else if (!strcmp(argv[i], "timeout")) {
			if (++i == argc) {
				return ;
			}

			param.timeout = strtoul(argv[i], NULL, 16);
		} else if (!strcmp(argv[i], "hex")) {
			//scan_cb = cmd_scan_cb_hex;
		} else if (!strcmp(argv[i], "silent")) {
			scan_silent = 1;
		} else if (!strcmp(argv[i], "pps")) {
			scan_silent = 2;
		} else if (!strncmp(argv[i], "int=0x", 4)) {
			uint32_t num;
			int cnt = sscanf(argv[i] + 4, "%x", &num);
			if (cnt != 1) {
				printf("invalid param %s\n", argv[i] + 4);
				return ;
			}
			param.interval = num;
			printf("scan interval 0x%x\n", param.interval);
		} else if (!strncmp(argv[i], "win=0x", 4)) {
			uint32_t num;
			int cnt = sscanf(argv[i] + 4, "%x", &num);
			if (cnt != 1) {
				printf("invalid param %s\n", argv[i] + 4);
				return ;
			}
			param.window = num;
			printf("scan window 0x%x\n", param.window);
		} else {
			printf("invalid param %s\n", argv[i]);
			return ;
		}
	}

	err = bt_le_scan_start(&param, NULL);
	if (err) {
		printf("Bluetooth set scan failed "
			    "(err %d)\n", err);
		return ;
	} else {
		printf("Bluetooth scan enabled\n");
	}

	return ;

}
#ifdef CONFIG_BTSNOOP
static void bt_snoop(int argc, char **argv)
{
	if(argc < 2) {
		printf("para invalid\n");
		return ;
	}
	if (!strcmp(argv[1], "on")) {
		printf("bt_snoop on\n");
		btsnoop_start_up();
	}else if(!strcmp(argv[1], "off")) {
		printf("bt_snoop off\n");
		btsnoop_shut_down();
	}else {
		printf("para invalid\n");
	}
}


FINSH_FUNCTION_EXPORT_CMD(bt_snoop, bt_snoop, Console bt_snoop Command);
#endif
FINSH_FUNCTION_EXPORT_CMD(cmd_ble_test, ble_test, Console bt test Command);
FINSH_FUNCTION_EXPORT_CMD(cmd_ble_scan, ble_scan, Console ble scan Command);
