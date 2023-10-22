#include <stdio.h>
#include <stdint.h>
#include "app_common.h"
#include "console.h"
#include "customer_api.h"
#include "stack/bte.h"
#include "bt_log.h"
#include <string.h>

#define WIFI_SERVICE_UUID       "9E20"
#define WIFI_SSID_CHAR_UUID     "9E30"
#define WIFI_STATUS_CHAR_UUID   "9E34"
#define WIFI_STATUS_DESC_UUID   "2902"

/* save for testing */
static uint16_t service_handle;

#define USE_FILE_STORE 1

#if USE_FILE_STORE
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define BT_CONFIG "/data/bt_config"
#define CONFIG_SIZE 1600
#else
uint8_t  flash_temp[1400];
#endif
uint32_t ftl_save_to_storage(void *p_data, uint16_t offset, uint16_t size)
{
    uint8_t *p_data8 = (uint8_t *)p_data;
#if USE_FILE_STORE
	int fd;
	fd = open(BT_CONFIG,O_WRONLY);
	if(fd < 0) {
		BTMG_ERROR("open failed:%s %s",BT_CONFIG,strerror(errno));
		return 0;
	}
	if(lseek(fd,offset,SEEK_SET) == -1) {
		BTMG_ERROR("cannot seek:%s",strerror(errno));
		return 0;
	}
	if(write(fd,p_data8,size) != size) {
		BTMG_ERROR("write error:%s",strerror(errno));
		return 0;
	}
#else

    if ((offset & 0x3) || (size == 0) || (size & 0x3))
    {
        return 1;
    }
    memcpy(&flash_temp[offset], p_data8, size);
#endif
    return 0;
}

// return 0 success
// return !0 fail
uint32_t ftl_load_from_storage(void *p_data, uint16_t offset, uint16_t size)
{
    uint8_t *p_data8 = (uint8_t *)p_data;
#if !USE_FILE_STORE
    if ((offset & 0x3) || (size == 0) || (size & 0x3))
    {
        return 1;
    }
    memcpy(p_data8, &flash_temp[offset], size);
#else
	int fd;
	fd = open(BT_CONFIG,O_RDONLY);
	if(fd < 0) {
		BTMG_ERROR("open failed:%s %s",BT_CONFIG,strerror(errno));
		return 0;
	}
	if(lseek(fd,offset,SEEK_SET) == -1) {
		BTMG_ERROR("cannot seek:%s",strerror(errno));
		return 0;
	}
	if(read(fd,p_data8,size) != size) {
		BTMG_ERROR("read error:%s",strerror(errno));
		return 0;
	}
#endif
    return 0;
}
#if USE_FILE_STORE
void bt_ftl_init(void)
{
	int fd;
	char buff[CONFIG_SIZE];
	if(!access(BT_CONFIG, R_OK)) {
//	if(!access(BT_CONFIG,F_OK)) {
		BTMG_DEBUG("%s is exisits.",BT_CONFIG);
		return ;
	}
	BTMG_DEBUG("Create bt config:%s",BT_CONFIG);
	fd = open(BT_CONFIG,O_WRONLY|O_CREAT);
	if(fd < 0) {
		BTMG_ERROR("open failed:%s,%s",BT_CONFIG,strerror(errno));
		return;
	}
	memset(buff,0,CONFIG_SIZE);
	if(write(fd,buff,CONFIG_SIZE) != CONFIG_SIZE) {
		BTMG_ERROR("write failed:%s",strerror(errno));
		return ;
	}
}
#endif
static void hex_dump(char *pref, int width, unsigned char *buf, int len)
{
	register int i,n;

	for (i = 0, n = 1; i < len; i++, n++) {
		if (n == 1)
			printf("%s", pref);
		printf("%2.2X ", buf[i]);
		if (n == width) {
			printf("\n");
			n = 0;
		}
	}
	if (i && n!=1)
		printf("\n");
}

void onGattsInitCallback(INT32 serverIf)
{
	BTMG_INFO("GATT server id is %d", serverIf);
}

void onGattsAddServiceCallback(CSM_AG_GATTS_ADD_SRVC_RST_T * bt_gatts_add_srvc)
{
	bool primary = bt_gatts_add_srvc->srvc_id.is_primary;
	// handle is 16 bit wide, not 32.
	uint16_t handle = (uint16_t)bt_gatts_add_srvc->srvc_handle;

	BTMG_INFO("GATT server id is %d", bt_gatts_add_srvc->server_if);
	BTMG_INFO("SERVICE uuid: %s", bt_gatts_add_srvc->srvc_id.id.uuid);
	BTMG_INFO("%s service start handle is %d",
					primary ? "primary" : "secondary", handle);
	service_handle = handle; // save.
}

void onGattsAddCharCallback(CSM_AG_GATTS_ADD_CHAR_RST_T* bt_gatts_add_char)
{
	BTMG_INFO("GATT server id is %d", bt_gatts_add_char->server_if);
	BTMG_INFO("Char uuid: %s", bt_gatts_add_char->uuid);
	BTMG_INFO("service start handle is %d, char value handle is %d",
			bt_gatts_add_char->srvc_handle, bt_gatts_add_char->char_handle);
}

void onGattsAddDescCallback(CSM_AG_GATTS_ADD_DESCR_RST_T* bt_gatts_add_desc)
{
	BTMG_INFO("GATT server id is %d", bt_gatts_add_desc->server_if);
	BTMG_INFO("Char uuid: %s", bt_gatts_add_desc->uuid);
	BTMG_INFO("service start handle is %d, desc value handle is %d",
			bt_gatts_add_desc->srvc_handle, bt_gatts_add_desc->descr_handle);
}

void onGattsReqWrite(CSM_AG_GATTS_REQ_WRITE_RST_T *bt_gatts_req_write)
{
	BTMG_INFO("Write Req from %s for handle %d, trans_id: %d", bt_gatts_req_write->btaddr,
										bt_gatts_req_write->attr_handle,
										bt_gatts_req_write->trans_id);

	BTMG_INFO("Value:");
	hex_dump(" ", 20, bt_gatts_req_write->value, bt_gatts_req_write->length);

	//uint32_t handle = 0x0003; // this is the handle of test handle
	uint32_t handle = bt_gatts_req_write->attr_handle;
	char p_value[2] = "ab";
	uint32_t value_len = 2;
	uint32_t status = 0; // status could be 0x0b(read rsp) or 0x0d(read blob rsp)
	uint32_t auth_req = 0x00;
	CSM_sendResponse(0, bt_gatts_req_write->trans_id, status, handle,
										p_value, value_len, auth_req);
}

void onGattsReqRead(CSM_AG_GATTS_REQ_READ_RST_T *bt_gatts_req_read)
{
	BTMG_INFO("Read Req from %s for handle %d, trans_id: %d",
										bt_gatts_req_read->btaddr,
										bt_gatts_req_read->attr_handle,
										bt_gatts_req_read->trans_id);
	//uint32_t handle = 0x0003; // this is the handle of test handle
	uint32_t handle = bt_gatts_req_read->attr_handle;
	char p_value[2] = "ab";
	uint32_t value_len = 2;
	uint32_t status = 0; // status could be 0x0b(read rsp) or 0x0d(read blob rsp)
	uint32_t auth_req = 0x00;
	CSM_sendResponse(0, bt_gatts_req_read->trans_id, status, handle,
										p_value, value_len, auth_req);

}

void onGattsConnectionEventCallback(CSM_AG_GATTS_EVENT_T bt_gatts_connection_evt)
{
	BTMG_INFO("Event Received, Event code: %d", bt_gatts_connection_evt);
}

void onGattsStartServerCallback()
{
	BTMG_INFO("Service Started");
}

void onGattsStopServerCallback()
{
	BTMG_INFO("Service Stopped");
}

RTK_BT_APP_GATTS_CB_FUNC_T gatt_cbs = {
	.onGattsInitCallback = onGattsInitCallback,
	.onGattsAddServiceCallback = onGattsAddServiceCallback,
	.onGattsAddCharCallback = onGattsAddCharCallback,
	.onGattsAddDescCallback = onGattsAddDescCallback,
	.onGattsReqWrite = onGattsReqWrite,
	.onGattsReqRead = onGattsReqRead,
	.onGattsConnectionEventCallback = onGattsConnectionEventCallback,
	.onGattsStartServerCallback = onGattsStartServerCallback,
	.onGattsStopServerCallback = onGattsStopServerCallback,
};

static void cmd_init(const char *arg)
{
	BTMG_INFO("Init");
	CSM_init();
}

static void cmd_deinit(const char *arg)
{
	BTMG_INFO("DeInit");
	CSM_deinitGatts();
}


static void cmd_add_service(const char *arg)
{
	char *uuid;

	BTMG_INFO("add service");

	uuid = WIFI_SERVICE_UUID;
	CSM_addService(1, uuid, 1, 8);
}

static void cmd_add_char(const char *arg)
{
	char *uuid;
	uint32_t prop, perm;
	BTMG_INFO("add char");
	prop = 0x0a;
	perm = 0x03;

	CSM_addChar(1,(uint32_t)service_handle, WIFI_SSID_CHAR_UUID, prop, perm);
	CSM_addChar(1,(uint32_t)service_handle, WIFI_STATUS_CHAR_UUID, prop, perm);
}

static void cmd_add_desc(const char *arg)
{
	char *uuid;
	uint32_t perm;
	BTMG_INFO("add desc");
	uuid = WIFI_STATUS_CHAR_UUID;
	perm = 0x03;

	CSM_addDesc(1,(uint32_t)service_handle, uuid, perm);
}

static void cmd_start_service(const char *arg)
{
	CSM_startService(1,(uint32_t)service_handle, 0);
}

static void cmd_stop_service(const char *arg)
{
	CSM_stopService(1,(uint32_t)service_handle);
}

static void cmd_rsp(const char *arg)
{
	uint32_t handle = 0x0003; // this is the handle of test handle
	char p_value[2] = "ab";
	uint32_t value_len = 2;
	uint32_t status = 0x0b; // status could be 0x0b(read rsp) or 0x0d(read blob rsp)
	uint32_t auth_req = 0x00;
	CSM_sendResponse(0, 0, status, handle, p_value, value_len, auth_req);
}

static void cmd_notify(const char *arg)
{
	uint32_t handle = 0x0005;
	uint32_t notify = true;

	char *p_value = "abc";
	uint32_t value_len = 3;
	CSM_sendIndication(0, handle, 0, notify, p_value, value_len);
}

static void cmd_unreg_server(const char *arg)
{
	CSM_unregisterService(1);
}


void CSMGapPairCB(uint8_t result, char *address)
{
	BTMG_INFO("result %d , address:%s",result,address);
}

void CSMGapUnpairCB(int result, char *addr)
{
	BTMG_INFO("result %d , address:%s",result,addr);
}

void CSMGapScanCB(const char *name, const char *address)
{
	BTMG_INFO("name:%s , address:%s",name,address);
}

RTK_GAP_IMPL_CB_FUNC_T gap_impl_cb = {
	.gapPairCB = CSMGapPairCB,
	.gapUnpairCB = CSMGapUnpairCB,
	.gapGapScanCB = CSMGapScanCB,
};


void CSMA2dpStateCB(uint8_t state, char *address)
{
	BTMG_INFO("state:%d , address:%s",state,address);
}

void CSMA2dpStreamCB(uint8_t state)
{
	BTMG_INFO("state:%d",state);
}

RTK_A2DP_IMPL_CB_FUNC_T a2dp_impl_cb = {
	.a2dpStateCB = CSMA2dpStateCB,
	.a2dpStreamCB = CSMA2dpStreamCB,
};

void CSMAvrcpStateCB(uint8_t state)
{
	BTMG_INFO("state:%d",state);
}

void CSMAvrcpPlayStateCB(uint8_t state)
{
	BTMG_INFO("state:%d",state);

}

void CSMVolumeChangeCB(uint8_t direction)
{
	BTMG_INFO("direction:%d",direction);
}

void CSMAbsoluteVolumeCB(unsigned int vol)
{
	BTMG_INFO("vol:%d",vol);
	uint8_t volume;
	volume = vol * 31 / 127;
	BTMG_INFO("lineout set : %d",volume);
	snd_ctl_set("audiocodec","LINEOUT volume",volume);

}

void CSMAvrcpCmdSrcCB(uint8_t avrcp_cmd)
{
	BTMG_INFO("avrcp cmd:%d",avrcp_cmd);
}

RTK_AVRCP_IMPL_CB_FUNC_T avrcp_impl_cb = {
	.avrcpStateCB = CSMAvrcpStateCB,
	.avrcpPlayStateCB = CSMAvrcpPlayStateCB,
	.volumeChangeCB = CSMVolumeChangeCB,
	.absoluteVolumeCB = CSMAbsoluteVolumeCB,
	.avrcpCmdSrcCB = CSMAvrcpCmdSrcCB,
};

static void cmd_bt_test(const char *arg)
{
    CSM_BT_DEV_INFO dev_info;
	char bt_name[12] = {0};
#if USE_FILE_STORE
	bt_ftl_init();
#endif
	bt_trace_init();
	bte_init();
	app_init();
    CSM_gapGetLocalDevInfo(&dev_info);
	snprintf(bt_name,11,"bt_test_%s",dev_info.bdAddr+15);
	BTMG_DEBUG("bt mac: %s,setting bt name:%s\n",dev_info.bdAddr,bt_name);
	CSM_gapSetName(bt_name);
	GapCallbackRegister(&gap_impl_cb);
	A2dpCallbackRegister(&a2dp_impl_cb);
	AvrcpCallbackRegister(&avrcp_impl_cb);
	CSM_setCallback(&gatt_cbs);
	cmd_init(NULL);
	cmd_add_service(NULL);
	cmd_add_char(NULL);
//	cmd_add_desc(NULL);
	cmd_start_service(NULL);
//	CSM_enableAdv(1);
}

static void bt_avrcp_cmd(int argc,char **argv)
{
	char *cmd_type_str[] = {
		"play",
		"pause",
		"fwd",
		"bwd",
		"ffwd",
		"rwd",
		"stop",
		"vol_up",
		"vol_down",
	};

	CSM_AVRCP_CMD_TYPE avrcp_type;
	int i;

	if(argc < 2) {
		BTMG_ERROR("bt avrcp paragram error.");
		return ;
	}
	for(i=0;i<CSM_AVRCP_CMD_TYPE_MAX;i++) {
		if(strcmp(argv[1],cmd_type_str[i]) == 0) {
			avrcp_type = i;
			break;
		}
	}
	if(i>=CSM_AVRCP_CMD_TYPE_MAX) {
		BTMG_ERROR("avrcp cmd not support.");
		return ;
	}
	CSM_avrcpSendPassthroughCmd(avrcp_type);
}

FINSH_FUNCTION_EXPORT_CMD(cmd_bt_test, bt_test, Console bt test Command);
FINSH_FUNCTION_EXPORT_CMD(bt_avrcp_cmd,avrcp, Console bt bt avrcp Command);
