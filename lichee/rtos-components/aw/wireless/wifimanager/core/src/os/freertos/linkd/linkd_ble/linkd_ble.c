#include <wifimg.h>
#include <wifi_log.h>
#include <linkd.h>

#include "bt_manager.h"
#include "kernel/os/os.h"

static btmg_callback_t btmg_cb;
static btmg_gatt_db_t *db;
static int char_handle;

#define BLINK_SERVICE_UUID	0xFF01
#define BLINK_CHAR_RX_UUID	0xFF02
#define BLINK_CHAR_TX_UUID	0xFF03
#define CCCD_UUID           0x2902

#define BLINK_TIMEOUT_MS    300000

#define BLINK_MSG_LEN_MAX             (108)
#define BLINK_MSG_SYNC                (0x5A6E3C6F)
#define BLINK_MSG_VERSION             (0x10)
#define BLINK_MSG_SERVICE_ID          (0x0B)

#define BLINK_MSG_VERSION_INDEX       (4)
#define BLINK_MSG_FMT_INDEX           (5)
#define BLINK_MSG_SSID_INDEX          (9)

#define BLINK_MSG_FMT_SUBC            (0)

#define BLINK_IND_LEN_MAX             (4)
#define BLINK_IND_SYNC                (0x0A)
#define BLINK_IND_STATUS_DEFAULT      (0x10)
#define BLINK_IND_STATUS_COMPLETE     (0x11)
#define BLINK_IND_STATUS_CONNECT      (0x12)

typedef enum {
	BLINK_INDICATE_SUCCESS = 0,
	BLINK_INDICATE_FAIL,
} blink_indicate_t;

typedef void (*set_state_callback)(blink_indicate_t state);

typedef struct blink_param {
	set_state_callback cb;
} blink_param_t;

typedef struct blink_result {
	uint8_t ssid[SSID_MAX_LEN];
	uint8_t ssid_len;
	uint8_t passphrase[PSK_MAX_LEN + 1]; /* ASCII string ending with '\0' */
	uint8_t passphrase_len;
} blink_result_t;

typedef enum {
	BLINK_STATE_SUCCESS = 0,
	BLINK_STATE_FAIL,
} blink_state_t;

typedef enum {
	BLINK_OK      = 0,     /* success */
	BLINK_ERROR   = -1,    /* general error */
	BLINK_BUSY    = -2,    /* device or resource busy */
	BLINK_TIMEOUT = -3,    /* wait timeout */
	BLINK_INVALID = -4,    /* invalid argument */
} blink_ret_t;

typedef enum {
	BLINK_STATUS_NORMAL = 0,
	BLINK_STATUS_READY,
	BLINK_STATUS_BUSY,
	BLINK_STATUS_TIMEOUT,
	BLINK_STATUS_COMPLETE,
} blink_status_t;

typedef struct blink_priv {
	blink_status_t status;
	XR_OS_Semaphore_t sem;
	uint8_t indicate;
	uint8_t indValue[BLINK_IND_LEN_MAX];
	uint8_t message[BLINK_MSG_LEN_MAX];
	set_state_callback cb;
} blink_priv_t;

static blink_priv_t *blink = NULL;

static int blink_parse_message(uint8_t *value, uint8_t *buf, uint16_t len, uint16_t offset);

void bt_blink_gatt_connection_cb(le_connection_para_t *data)
{
	if (data->status == LE_CONNECTED) {
		WMG_WARNG("gatts connected%s\n");
	} else if (data->status == LE_CONNECT_FAIL){
		WMG_WARNG("gatts connect failed\n");
	} else if (data->status == LE_DISCONNECTED) {
		WMG_WARNG("gatts disconnected\n");
	}
}

void bt_blink_gatt_char_write_req_cb(gatts_char_write_req_t *data)
{
	int ret = 0;
	blink_priv_t *priv = blink;

	WMG_DEBUG("\n");

	if (data->value_len > BLINK_MSG_LEN_MAX) {
		WMG_DEBUG("exec blink buffer!\n");
		return;
	}

	char recv_data[data->value_len + 1];
	memcpy(recv_data, data->value, data->value_len);
	recv_data[data->value_len] = '\0';
	WMG_DEBUG("receive char write: [conn_id=%d][handle=0x%04X][len=%d][value=%s]\n", data->conn_id,
			data->attr_handle, data->value_len,  recv_data);

	blink_parse_message((uint8_t *)priv->message, (uint8_t *)data->value, data->value_len, 0);
}

void bt_blink_gatt_ccc_cfg_cb(gatts_ccc_cfg_t *data)
{
	blink_priv_t *priv = blink;

	char_handle = data->attr_handle;
	WMG_DEBUG("char_handle:0x%02X\n", char_handle);
	priv->indicate = 1;
}

static void le_set_adv_param(void)
{
	btmg_le_adv_param_t adv_param;

	adv_param.interval_min = 0x0020;
	adv_param.interval_max = 0x01E0;
	adv_param.adv_type = BTMG_LE_ADV_IND;
	adv_param.filter = BTMG_LE_PROCESS_ALL_REQ;

	btmg_le_set_adv_param(&adv_param);
}

static int le_set_adv_data(const char *ble_name, uint16_t uuid)
{
	int index = 0;
	btmg_adv_scan_rsp_data_t adv_data;

	adv_data.data[index] = 0x02;
	adv_data.data[index + 1] = 0x01;
	adv_data.data[index + 2] = 0x1A;

	index += adv_data.data[index] + 1;

	adv_data.data[index] = strlen(ble_name) + 1;
	adv_data.data[index + 1] = 0x09;
	int name_len;
	name_len = strlen(ble_name);
	strcpy(&(adv_data.data[index + 2]), ble_name);
	index += adv_data.data[index] + 1;

	adv_data.data[index] = 0x03;
	adv_data.data[index + 1] = 0x03;
	adv_data.data[index + 2] = (char)(uuid &  0xFF);
	adv_data.data[index + 3] = (char)((uuid >> 8) & 0xFF);
	index += adv_data.data[index] + 1;

	adv_data.data_len = index;

	return btmg_le_set_adv_scan_rsp_data(&adv_data, NULL);
}

static int bt_gatt_server_register_blink_svc(void)
{
	if (db != NULL) {
		WMG_ERROR("gatt already registered\n");
		return 0;
	}

	btmg_uuid_t uuid;
	btmg_gatt_properties_t prop;
	btmg_gatt_permission_t perm = BTMG_GATT_PERM_READ | BTMG_GATT_PERM_WRITE;

	db = btmg_gatt_attr_create(6); //CHAR:2, other:1

	uuid.type = BTMG_UUID_16;
	uuid.value.u16 = BLINK_SERVICE_UUID;
	btmg_gatt_attr_primary_service(db, uuid); // +1

	uuid.value.u16 = BLINK_CHAR_RX_UUID;
	prop = BTMG_GATT_CHRC_READ | BTMG_GATT_CHRC_WRITE | BTMG_GATT_CHRC_AUTH;
	btmg_gatt_attr_characteristic(db, uuid, prop, perm); // +2

	uuid.value.u16 = BLINK_CHAR_TX_UUID;
	prop = BTMG_GATT_CHRC_READ | BTMG_GATT_CHRC_WRITE | BTMG_GATT_CHRC_NOTIFY | BTMG_GATT_CHRC_INDICATE;
	btmg_gatt_attr_characteristic(db, uuid, prop, perm); // +2
	btmg_gatt_attr_ccc(db, perm);                        // +1
	btmg_gatt_register_service(db);

	return 0;
}

static int bt_init(void)
{
	btmg_set_loglevel(BTMG_LOG_LEVEL_DEBUG);

	btmg_cb.btmg_gatts_cb.conn_cb = bt_blink_gatt_connection_cb;
	btmg_cb.btmg_gatts_cb.char_write_req_cb = bt_blink_gatt_char_write_req_cb;
	btmg_cb.btmg_gatts_cb.ccc_cfg_cb = bt_blink_gatt_ccc_cfg_cb;

	btmg_core_init();
	btmg_register_callback(&btmg_cb);
	btmg_set_profile(BTMG_GATT_SERVER);
	btmg_adapter_enable(true);
	bt_gatt_server_register_blink_svc();

	return 0;
}

static int bt_deinit(void)
{
	btmg_gatt_unregister_service(db);
	btmg_gatt_attr_destory(db);
	btmg_adapter_enable(false);
	btmg_core_deinit();
	btmg_unregister_callback();

	return 0;
}

static int blink_parse_message(uint8_t *value, uint8_t *buf, uint16_t len, uint16_t offset)
{
	blink_priv_t *priv = blink;
	uint8_t *message = buf;
	uint32_t sync = (uint32_t)message[0] << 24 | (uint32_t)message[1] << 16 |
					(uint32_t)message[2] << 8 | message[3];
	uint8_t version = message[BLINK_MSG_VERSION_INDEX];

	if (priv->status != BLINK_STATUS_COMPLETE &&
		sync == BLINK_MSG_SYNC && version == BLINK_MSG_VERSION) {
		WMG_DEBUG("\n");
		if (message[BLINK_MSG_FMT_INDEX] == BLINK_MSG_FMT_SUBC) {
			/* TO DO.. */
			return -1;
		}
		memset(value, 0, BLINK_MSG_LEN_MAX);
		memcpy(value + offset, buf, len);
		*(value + offset + len) = '\0';

		XR_OS_ThreadSuspendScheduler();
		priv->status = BLINK_STATUS_COMPLETE;
		XR_OS_ThreadResumeScheduler();
		XR_OS_SemaphoreRelease(&priv->sem);
	}

	return 0;
}

static inline int blink_adv_start(void)
{
	le_set_adv_param();
	le_set_adv_data("aw_bt_blink", 0xff01);

	return btmg_le_enable_adv(true);
}

static inline int blink_adv_stop(void)
{
	return btmg_le_enable_adv(false);
}

blink_ret_t blink_start(blink_param_t *param)
{
	blink_priv_t *priv = blink;

	WMG_DEBUG("\n");

	if (priv != NULL) {
		WMG_WARNG("blink has started\n");
		return BLINK_ERROR;
	}
	if (param == NULL) {
		WMG_ERROR("invalid param\n");
		return BLINK_INVALID;
	}

	priv = malloc(sizeof(blink_priv_t));
	if (priv == NULL) {
		WMG_ERROR("malloc fail\n");
		return BLINK_ERROR;
	}
	memset(priv, 0, sizeof(blink_priv_t));

	bt_init();

	if (blink_adv_start() != 0) {
		WMG_ERROR("adv start fail\n");
		goto err;
	}

	if (XR_OS_SemaphoreCreateBinary(&priv->sem) != XR_OS_OK) {
		WMG_ERROR("sem create fail\n");
		goto err;
	}

	priv->cb = param->cb;
	priv->status = BLINK_STATUS_READY;
	blink = priv;

	return BLINK_OK;

err:
	free(priv);
	return BLINK_ERROR;
}

blink_ret_t blink_wait(uint32_t timeout)
{
	blink_priv_t *priv = blink;

	if (!priv || priv->status != BLINK_STATUS_READY) {
		WMG_ERROR("blink not ready\n");
		return BLINK_ERROR;
	}

	if (timeout <= 0) {
		if (priv->status == BLINK_STATUS_COMPLETE)
			return BLINK_OK;
		return BLINK_ERROR;
	}

	if (XR_OS_SemaphoreWait(&priv->sem, timeout) != XR_OS_OK){
		WMG_WARNG("sem wait fail\n");
		return BLINK_TIMEOUT;
	}

	if (priv->status != BLINK_STATUS_COMPLETE)
		return BLINK_ERROR;

	return BLINK_OK;
}

blink_ret_t blink_get_result(blink_result_t *result)
{
	blink_priv_t *priv = blink;
	if (!priv || !result) {
		WMG_ERROR("invalid state or param\n");
		return BLINK_ERROR;
	}
	if (priv->status != BLINK_STATUS_COMPLETE) {
		WMG_WARNG("blink not completed\n");
		return BLINK_ERROR;
	}

	result->ssid_len = priv->message[BLINK_MSG_SSID_INDEX];
	result->passphrase_len = priv->message[priv->message[BLINK_MSG_SSID_INDEX] + BLINK_MSG_SSID_INDEX + 1];

	if (result->ssid_len > SSID_MAX_LEN || result->ssid_len == 0 ||
		result->passphrase_len > PSK_MAX_LEN || result->passphrase_len == 0) {
		WMG_WARNG("invalid len\n");
		return BLINK_ERROR;
	}
	memcpy(result->ssid, priv->message + BLINK_MSG_SSID_INDEX + 1, result->ssid_len);
	memcpy(result->passphrase, priv->message + priv->message[BLINK_MSG_SSID_INDEX] + BLINK_MSG_SSID_INDEX + 2,
			result->passphrase_len);
	result->ssid[result->ssid_len] = '\0';
	result->passphrase[result->passphrase_len] = '\0';

	return BLINK_OK;
}

blink_ret_t blink_set_state(blink_state_t state)
{
	blink_priv_t *priv = blink;

	WMG_WARNG("\n");
	if (priv == NULL) {
		WMG_WARNG("not init\n");
		return BLINK_ERROR;
	}

	if (priv->indicate > 0) {
		priv->indValue[0] = BLINK_IND_SYNC;
		priv->indValue[1] = BLINK_MSG_SERVICE_ID;
		priv->indValue[2] = (priv->status == BLINK_STATUS_COMPLETE) ?
							BLINK_IND_STATUS_COMPLETE : BLINK_IND_STATUS_DEFAULT;
		priv->indValue[3] = (state == BLINK_STATE_SUCCESS) ?
							BLINK_IND_STATUS_CONNECT : BLINK_IND_STATUS_DEFAULT;

		btmg_gatts_notify(0, char_handle, &(priv->indValue[0]), sizeof(priv->indValue));

		return BLINK_OK;
	}

	return BLINK_ERROR;
}

blink_ret_t blink_stop(void)
{
	blink_priv_t *priv = blink;

	if (priv == NULL) {
		WMG_WARNG("blink not started\n");
		return BLINK_ERROR;
	}
	if (priv->status == BLINK_STATUS_BUSY) {
		WMG_WARNG("blink busy\n");
		return BLINK_BUSY;
	}

	blink_adv_stop();
	btmg_le_disconnect(0, 0);

	XR_OS_SemaphoreDelete(&priv->sem);
	bt_deinit();
	free(blink);
	blink = NULL;

	return BLINK_OK;
}

static void blink_init(void)
{
	WMG_INFO("\n");

	blink_param_t param;
	memset(&param, 0, sizeof(blink_param_t));
	blink_start(&param);
}

void *_ble_mode_main_loop(void *arg)
{
	WMG_INFO("support ble mode config net\n");
	proto_main_loop_para_t *main_loop_para = (proto_main_loop_para_t *)arg;
	wifi_linkd_result_t linkd_result;

	blink_ret_t ret;
	blink_result_t result;

	blink_init();

	if (blink_wait(BLINK_TIMEOUT_MS) != BLINK_OK) {
		WMG_ERROR("time out\n");
		blink_set_state(BLINK_STATE_FAIL);
		main_loop_para->result_cb(NULL);
		return NULL;
	}

	ret = blink_get_result(&result);
	if (ret != BLINK_OK) {
		WMG_ERROR("get result fail\n");
		blink_set_state(BLINK_STATE_FAIL);
		main_loop_para->result_cb(NULL);
		return NULL;
	}
	WMG_INFO("ssdi = '%s';password = '%s'\n", result.ssid, result.passphrase);

	blink_set_state(BLINK_STATE_SUCCESS);

	blink_stop();

	linkd_result.ssid = result.ssid;
	linkd_result.psk = result.passphrase;
	main_loop_para->result_cb(&linkd_result);

	return NULL;
}
