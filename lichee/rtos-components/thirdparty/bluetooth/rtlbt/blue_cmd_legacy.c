/*
 *  Copyright (C) 2019 Realtek Semiconductor Corporation.
 *
 *  Author: Alex Lu <alex_lu@realsil.com.cn>
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define DEBUG
#include "customer_api.h"
#include "blue_cmd.h"
#include "blue_cmd_legacy.h"

#include "app_common.h"

#include "bt_avrcp.h"
#include "bt_a2dp.h"

#include "trace_app.h"

#include "bt_gap.h"

#include "bt_gatt_server_dynamic.h"


#define BLUEZ_SRC_CONF  "/etc/bluetooth/main.conf"
#define BLUEZ_MAIN_CONF "/data/misc/bluetooth/main.conf"

#define INQ_SCAN    0
#define PG_SCAN     1

const char *type2str(uint8_t type)
{
    switch (type)
    {
    case INQ_SCAN:
        return "InquiryScan";
    case PG_SCAN:
        return "PageScan";
    default:
        return "Unknown";
    }
}
/* Open HCI device.
 * Returns device descriptor (dd). */
/*
static int hci_open_dev2(int dev_id)
{
    struct sockaddr_hci a;
    int dd, err;

    // Check for valid device id
    if (dev_id < 0) {
        errno = ENODEV;
        return -1;
    }

    // Create HCI socket
    dd = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_HCI);
    if (dd < 0)
        return dd;

    // Bind socket to the HCI device
    memset(&a, 0, sizeof(a));
    a.hci_family = AF_BLUETOOTH;
    a.hci_dev = dev_id;
    if (bind(dd, (struct sockaddr *) &a, sizeof(a)) < 0)
        goto failed;

    return dd;

failed:
    err = errno;
    close(dd);
    errno = err;

    return -1;
}

static int hci_close_dev2(int dd)
{
    return close(dd);
}

static int hci_send_cmd2(int dd, uint16_t ogf, uint16_t ocf,
        uint8_t plen, void *param)
{
    uint8_t type = HCI_COMMAND_PKT;
    hci_command_hdr hc;
    struct iovec iv[3];
    int ivn;

    hc.opcode = htobs(cmd_opcode_pack(ogf, ocf));
    hc.plen= plen;

    iv[0].iov_base = &type;
    iv[0].iov_len  = 1;
    iv[1].iov_base = &hc;
    iv[1].iov_len  = HCI_COMMAND_HDR_SIZE;
    ivn = 2;

    if (plen) {
        iv[2].iov_base = param;
        iv[2].iov_len  = plen;
        ivn = 3;
    }

    while (writev(dd, iv, ivn) < 0) {
        if (errno == EAGAIN || errno == EINTR)
            continue;
        return -1;
    }
    return 0;
}


// 0.625ms * N, N: 0x0012 to 0x1000
static int set_scan_act(uint8_t inq_pg, uint16_t window, uint16_t interval)
{
    int sk;
    uint16_t ogf, ocf;
    write_inq_activity_cp cp;

    if (inq_pg > 1) {
        pr_error("Invalid inq_pg %u", inq_pg);
        return -1;
    }

    sk = hci_open_dev2(0);
    if (sk < 0) {
        pr_error("HCI device open failed");
        return -2;
    }

    ogf = OGF_HOST_CTL;
    if (inq_pg == INQ_SCAN)
        ocf = OCF_WRITE_INQ_ACTIVITY;
    else
        ocf = 0x001c;
    cp.window = htobs(window);
    cp.interval = htobs(interval);

    if (window < 0x12 || window > 0x1000)
        pr_error("inquiry window out of range!\n");

    if (interval < 0x12 || interval > 0x1000)
        pr_error("inquiry interval out of range!\n");

    if (hci_send_cmd2(sk, ogf, ocf, sizeof(cp), &cp) < 0) {
        pr_error("Can't set inquiry parameters: %s (%d)\n",
             strerror(errno), errno);
        hci_close_dev2(sk);
        return -3;
    }

    pr_info("Set %s window %x interval %x successfully",
        type2str(inq_pg), window, interval);

    hci_close_dev2(sk);
}

// 0x00: Standard; 0x01: Interlaced
int set_scan_type(uint8_t inq_pg, uint8_t type)
{
    int sk;
    uint16_t ogf, ocf;

    if (inq_pg > 1) {
        pr_error("Invalid inq_pg %u", inq_pg);
        return -1;
    }

    if (type > 1) {
        pr_error("Type is not either Standard nor Interlaced");
        return -2;
    }

    sk = hci_open_dev2(0);
    if (sk < 0) {
        pr_error("HCI device open failed");
        return -3;
    }

    ogf = OGF_HOST_CTL;
    if (inq_pg == INQ_SCAN)
        ocf = OCF_WRITE_INQUIRY_SCAN_TYPE;
    else
        ocf = 0x0047;

    if (hci_send_cmd2(sk, ogf, ocf, sizeof(type), &type) < 0) {
        pr_error("Can't set %s type paramter: %s (%d)\n",
             type2str(inq_pg),
             strerror(errno), errno);
        hci_close_dev2(sk);
        return -4;
    }

    pr_info("Set %s type %u successfully", type2str(inq_pg), type);

    hci_close_dev2(sk);

    return 0;
}



void csm_test(void)
{
    int result;
    char addr[18];
    char *argv[16];

    argv[0] = addr;
    result = bta_submit_command_wait(BCMD_TEST, 1, (void **)argv);
    if (result < 0) {
        pr_error("Run command failed, %d", result);
    }

    pr_info("argv[0] %s", addr);
}

static int bcmd_test(int argc, void **argv)
{
    int i = 0;

    while (argc > 0) {
        pr_info("argv[%d] %p", i, argv[i]);
        i++;
        argc--;
    }

    bta_adapter_get_address(0, argv[0]);

    return 0;
}

static int bcmd_set_power(int argc, void **argv)
{
    struct mgmt *mgmt = rtb_mgmt_get();
    uint16_t index = 0;
    bool onoff = false;
    uint8_t val = 0x00;

    if (!mgmt)
        return -1;

    if (argc > 0)
        onoff = (bool)PTR_TO_UINT(argv[0]);

    if (onoff)
        val = 0x01;

    if (!mgmt_send(mgmt, MGMT_OP_SET_POWERED, index, sizeof(val), &val,
               NULL, NULL, NULL)) {
        pr_error("Unable to send set powerd cmd");
        return -1;
    }

    return 0;
}

static int bcmd_read_local_info(int argc, void **argv)
{
    struct bta_adapter *adapter;
    CSM_BT_DEV_INFO *dev_info = argv[0];

    adapter = bta_get_adapter(0);
    if (adapter) {
        strcpy(dev_info->name, adapter->name);
        strcpy(dev_info->bdAddr, adapter->address);
        return 0;
    } else
        return -1;
}

static int bcmd_read_remote_info(int argc, void **argv)
{
    struct bta_device *device;
    CSM_BT_DEV_INFO *dev_info = argv[0];

    device = bta_first_conn(0, 0);
    if (!device) {
        pr_error("No device connected");
        return -1;
    }

    dev_info->name[0] = '\0';
    if (device->name) {
        strncpy(dev_info->name, device->name, CSM_NAME_MAX_LEN - 1);
        dev_info->name[CSM_NAME_MAX_LEN - 1] = '\0';
    }
    dev_info->bdAddr[0] = '\0';
    if (device->address)
        strcpy(dev_info->bdAddr, device->address);

    return 0;
}

static int bcmd_set_scan_mode(int argc, void **argv)
{
    bool conn_flag;
    bool disc_flag;
    struct mgmt_mode mode;
    struct mgmt *mgmt = rtb_mgmt_get();
    uint16_t index = 0;
    struct mgmt_cp_set_discoverable {
        uint8_t    val;
        uint16_t timeout;
    } __packed cp;

    conn_flag = (bool)PTR_TO_UINT(argv[0]);
    disc_flag = (bool)PTR_TO_UINT(argv[1]);

    pr_info("---- Start conn_flag %d, disc_flag %d--------", conn_flag,
            disc_flag);

    if (conn_flag) {
        set_scan_act(PG_SCAN, 0x0012, 0x0400);
        set_scan_type(PG_SCAN, 1);
        mode.val = 0x01;
    } else
        mode.val = 0x00;
    if (!mgmt_send(mgmt, MGMT_OP_SET_CONNECTABLE, index,
               sizeof(mode), &mode, NULL, NULL, NULL)) {
        pr_error("Unable to set connectable %d", conn_flag);
        return -1;
    }

    cp.timeout = 0;
    if (disc_flag) {
        set_scan_act(INQ_SCAN, 0x0012, 0x0800);
        set_scan_type(INQ_SCAN, 1);
        cp.val = 0x01;
    } else
        cp.val = 0x00;

    if (!mgmt_send(mgmt, MGMT_OP_SET_DISCOVERABLE, index,
               sizeof(cp), &cp, NULL, NULL, NULL)) {
        pr_error("Unable to set discoverable %d", disc_flag);
        return -1;
    }
    pr_info("---- End ----");

    return 0;
}

static int bcmd_set_local_name(int argc, void **argv)
{
    struct mgmt *mgmt = rtb_mgmt_get();
    uint16_t index = 0;
    char *name;
    struct mgmt_cp_set_local_name {
        uint8_t name[249];
        uint8_t short_name[11];
    } __packed cp;
    char *filename = BLUEZ_MAIN_CONF;
    char *str;
    gsize length = 0;
    GKeyFile *key_file;
    char sys_cmd[128];

    name = argv[0];
    memset(&cp, 0, sizeof(cp));
    strncpy((char *)cp.name, name, sizeof(cp.name) - 1);

    // File doesn't exist, copy it from /etc/bluetooth/
    if (access(filename, F_OK) < 0) {
        pr_warning("%s was removed, copy from %s", filename, BLUEZ_SRC_CONF);
        snprintf(sys_cmd, sizeof(sys_cmd), "cp -f %s %s",
                 BLUEZ_SRC_CONF, filename);
        system(sys_cmd);
        if (access(filename, F_OK) < 0) {
            pr_error("Copy %s error", filename);
            goto set_local_name;
        }
    }

    key_file = g_key_file_new();
    g_key_file_load_from_file(key_file, filename,
            G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
    g_key_file_set_string(key_file, "General", "Name", name);
    str = g_key_file_to_data(key_file, &length, NULL);
    g_file_set_contents(filename, str, length, NULL);
    g_free(str);
    g_key_file_free(key_file);

set_local_name:
    if (mgmt_send(mgmt, MGMT_OP_SET_LOCAL_NAME, index, sizeof(cp), &cp,
              NULL, NULL, NULL) > 0)
        return 0;
    else
        return -1;
}

static int bcmd_start_discovery(int argc, void **argv)
{
    struct mgmt *mgmt = rtb_mgmt_get();

    return mgmt_start_discovery(mgmt);
}

static int bcmd_stop_discovery(int argc, void **argv)
{
    struct mgmt *mgmt = rtb_mgmt_get();

    return mgmt_stop_discovery(mgmt);
}

static int bcmd_unpair_device(int argc, void **argv)
{
    struct mgmt *mgmt = rtb_mgmt_get();
    struct mgmt_cp_unpair_device {
        struct mgmt_addr_info addr;
        uint8_t disconnect;
    } __packed cp;
    char *address;
    uint16_t index = 0;

    address = argv[0];
    util_str2addr(address, cp.addr.b);
    cp.addr.type = BDADDR_BREDR;
    cp.disconnect = 1;
    if (mgmt_send(mgmt, MGMT_OP_UNPAIR_DEVICE, index, sizeof(cp), &cp,
               NULL, NULL, NULL) > 0)
        return 0;
    else
        return -1;
}

int avrcp_connect(const char *address)
{
    char *uuid_tg = "0000110c-0000-1000-8000-00805f9b34fb";
    char *uuid_ct = "0000110e-0000-1000-8000-00805f9b34fb";

    if (bta_profile_connect(address, uuid_tg) < 0)
        return -1;
    if (bta_profile_connect(address, uuid_ct) < 0)
        return -1;

    return 0;
}

static int bcmd_a2dp_connect(int argc, void **argv)
{
    char *uuid = NULL;
    char *address;
    uint8_t profile;

    address = argv[0];
    profile = (uint8_t)PTR_TO_UINT(argv[1]);
    if (profile == LOCAL_A2DP_SINK)
        uuid = "0000110a-0000-1000-8000-00805f9b34fb";
    else
        uuid = "0000110b-0000-1000-8000-00805f9b34fb";

    bta_profile_connect(address, uuid);

    // avrcp_connect(address);

    return 0;
}

static int bcmd_a2dp_disconnect(int argc, void **argv)
{
    char *address;

    address = argv[0];

    return bta_device_disconnect(address);
}

static gint match_uuid(gconstpointer a, gconstpointer b)
{
    const char *uuid1 = (void *)a;
    const char *uuid2 = (void *)b;

    if (!strcmp(uuid1, uuid2))
        return 0;
    return 1;
}

static int bcmd_a2dp_paired_list(int argc, void **argv)
{
    CSM_A2DP_DEV_INFO_LIST *info_list;
    uint8_t profile;
    GList *l;
    GList *l2;
    const char *uuid;
    unsigned int num = 0;
    CSM_BT_DEV_INFO *devi;
    struct bta_adapter *adapter;

    info_list = argv[0];
    profile = (uint8_t)PTR_TO_UINT(argv[1]);
    memset(info_list, 0, sizeof(*info_list));

    // Use the first adapter
    adapter = bta_get_adapter(0);
    if (!adapter)
        return -1;

    if (profile == LOCAL_A2DP_SINK)
        uuid = "0000110a-0000-1000-8000-00805f9b34fb";
    else
        uuid = "0000110b-0000-1000-8000-00805f9b34fb";

    for (l2 = g_list_first(adapter->devices); l2; l2 = g_list_next(l2)) {
        struct bta_device *device;

        device = l2->data;
        if (!device)
            continue;
        if (!device->name || !device->address)
            continue;

        l = g_list_find_custom(device->uuids, uuid, match_uuid);
        if (!l)
            continue;
        devi = &info_list->device_list[num];
        strncpy(devi->name, device->name, sizeof(devi->name) - 1);
        strncpy(devi->bdAddr, device->address, sizeof(devi->bdAddr) - 1);
        num++;
    }

    info_list->dev_num = num;

    return 0;
}

static int bcmd_avrcp_connect(int argc, void **argv)
{
    return avrcp_connect((const char *)argv[0]);
}

static int bcmd_avrcp_disconnect(int argc, void **argv)
{
    struct bta_device *device;
    const char *uuid = AVRCP_TARGET_UUID;

    device = bta_first_conn(0, 0);
    if (!device) {
        pr_error("No device connected");
        return -1;
    }

    if (bta_profile_disconnect(device, uuid))
        return -1;
    uuid = AVRCP_REMOTE_UUID;
    if (bta_profile_disconnect(device, uuid))
        return -1;

    return 0;
}

static int bcmd_avrcp_send_passthr_cmd(int argc, void **argv)
{
    struct bta_device *device;
    CSM_AVRCP_CMD_TYPE cmd_type;

    cmd_type = (CSM_AVRCP_CMD_TYPE)PTR_TO_INT(argv[0]);

    device = bta_first_conn(0, 0);
    if (!device) {
        pr_error("No device connected");
        return -1;
    }

    if (!device->avrcp_connected) {
        pr_error("AVRCP is not connected");
        return -1;
    }

    return bta_avrcp_send_passthr(device, cmd_type);
}

static int bcmd_avrcp_change_absvol(int argc, void **argv)
{
    uint8_t vol;
    struct bta_device *device;
    CSM_A2DP_ROLE role;

    vol = (uint8_t)PTR_TO_UINT(argv[0]);
    role = (CSM_A2DP_ROLE)PTR_TO_INT(argv[1]);

    if (role != CSM_A2DP_ROLE_SINK && role != CSM_A2DP_ROLE_SRC) {
        pr_error("Invalid a2dp role %d", role);
        return -1;
    }

    device = bta_first_conn(0, 0);
    if (!device) {
        pr_error("No device connected");
        return -1;
    }

    if (!device->avrcp_connected) {
        pr_error("AVRCP is not connected");
        return -1;
    }

    if (role == CSM_A2DP_ROLE_SINK)
        return bta_sink_avrcp_change_absvol(vol);
    else
        return bta_src_avrcp_change_absvol(vol);
}


static int bta_cmd_legacy_process(uint16_t opcode, int argc, void **argv)
{
    int result = -1;

    pr_debug("opcode %d", opcode);

    switch (opcode) {
    case BCMD_TEST:
        result = bcmd_test(argc, argv);
        break;
    case BCMD_SET_POWER:
        result = bcmd_set_power(argc, argv);
        break;
    case BCMD_READ_LOCAL_INFO:
        result = bcmd_read_local_info(argc, argv);
        break;
    case BCMD_READ_REMOTE_INFO:
        result = bcmd_read_remote_info(argc, argv);
        break;
    case BCMD_SET_SCAN_MODE:
        result = bcmd_set_scan_mode(argc, argv);
        break;
    case BCMD_SET_LOCAL_NAME:
        result = bcmd_set_local_name(argc, argv);
        break;
    case BCMD_START_DISCOVERY:
        result = bcmd_start_discovery(argc, argv);
        break;
    case BCMD_STOP_DISCOVERY:
        result = bcmd_stop_discovery(argc, argv);
        break;
    case BCMD_UNPAIR_DEVICE:
        result = bcmd_unpair_device(argc, argv);
        break;
    case BCMD_A2DP_CONNECT:
        result = bcmd_a2dp_connect(argc, argv);
        break;
    case BCMD_A2DP_DISCONNECT:
        result = bcmd_a2dp_disconnect(argc, argv);
        break;
    case BCMD_A2DP_PAIRED_LIST:
        result = bcmd_a2dp_paired_list(argc, argv);
        break;
    case BCMD_AVRCP_CONNECT:
        result = bcmd_avrcp_connect(argc, argv);
        break;
    case BCMD_AVRCP_DISCONNECT:
        result = bcmd_avrcp_disconnect(argc, argv);
        break;
    case BCMD_AVRCP_SEND_PASSTHR_CMD:
        result = bcmd_avrcp_send_passthr_cmd(argc, argv);
        break;
    case BCMD_AVRCP_CHG_VOL:
        result = bcmd_avrcp_change_absvol(argc, argv);
        break;
    default:
        pr_err("Unknown bta command %04x", opcode);
        break;
    }

    return result;
}

void bta_register_legacy_handler(void)
{
    bta_register_handler(bta_cmd_legacy_process);
}
*/

static int bcmd_read_local_info(int argc, void **argv)
{
    bool ret = false;
    uint8_t addr[6];
    char bdAddr[CSM_BDADDR_MAX_LEN] = {0};
    char name[CSM_NAME_MAX_LEN] = {0};

    CSM_BT_DEV_INFO *dev_info = argv[0];

    ret = bt_gap_read_local_address(addr);
    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_read_local_info: failed to get address");
        return -1;
    }

    mac_bin_to_str(bdAddr, addr);
    memcpy(dev_info->bdAddr, bdAddr, CSM_BDADDR_MAX_LEN - 1);

    ret = bt_gap_read_local_name(name, CSM_NAME_MAX_LEN);
    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_read_local_info: failed to get name");
        return -1;
    }

    memcpy(dev_info->name, name, CSM_NAME_MAX_LEN - 1);

    return 0;
}

static int bcmd_read_remote_info(int argc, void **argv)
{
    bool ret = false;
    uint8_t addr[6];
    char bdAddr[CSM_BDADDR_MAX_LEN] = {0};
    char name[CSM_NAME_MAX_LEN] = {0};

    CSM_BT_DEV_INFO *dev_info = argv[0];

    ret = bt_gap_read_remote_address(addr);
    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_read_local_info: failed to get address");
        memset(dev_info, 0, sizeof(CSM_BT_DEV_INFO));
        return -1;
    }

    mac_bin_to_str(bdAddr, addr);
    memcpy(dev_info->bdAddr, bdAddr, CSM_BDADDR_MAX_LEN - 1);

    ret = bt_gap_read_remote_name(addr, name, CSM_NAME_MAX_LEN);
    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_read_local_info: failed to get name");
        return -1;
    }

    memcpy(dev_info->name, name, strlen(name));

    return 0;
}

static int bcmd_set_scan_mode(int argc, void **argv)
{
    bool ret;
    bool conn_flag;
    bool disc_flag;
    conn_flag = (bool)PTR_TO_UINT(argv[0]);
    disc_flag = (bool)PTR_TO_UINT(argv[1]);

    ret = bt_gap_set_scan_mode(conn_flag, disc_flag);
    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_set_scan_mode failed");
        return -1;
    }

    return 0;
}

static int bcmd_set_local_name(int argc, void **argv)
{
    bool ret;
    char *name;
    name = argv[0];

    ret = bt_gap_set_local_name(name);
    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_set_local_name failed");
        return -1;
    }

    return 0;
}

static int bcmd_start_discovery(int argc, void **argv)
{
    bool ret;

    ret = bt_gap_start_discovery();
    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_start_discovery failed");
        return -1;
    }

    return 0;
}

static int bcmd_stop_discovery(int argc, void **argv)
{
    bool ret;

    ret = bt_gap_stop_discovery();
    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_stop_discovery failed");
        return -1;
    }

    return 0;
}

static int bcmd_unpair_device(int argc, void **argv)
{
    bool ret;
    char *address;
    uint8_t adr_hex[6];
    address = argv[0];

    mac_str_to_bin(address, adr_hex);

    ret = bt_gap_unpair_device(adr_hex);
    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_unpair_device failed");
        return -1;
    }

    return 0;
}


static int bcmd_avrcp_connect(int argc, void **argv)
{
    bool ret;
    char *address;
    uint8_t remote_bd[6];

    address = argv[0];

    mac_str_to_bin(address, remote_bd);

    ret = bt_avrcp_connect_req(remote_bd);
    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_avrcp_connect failed");
        return -1;
    }

    return 0;
}

static int bcmd_avrcp_disconnect(int argc, void **argv)
{
    bool ret;
    uint8_t *bd_addr = bt_avrcp_get_dev_addr();

    if (bd_addr == NULL)
    {
        APP_PRINT_WARN0("No avrcp connected device found!");
        return -1;
    }

    APP_PRINT_INFO1("bcmd_avrcp_disconnect: address is %s", TRACE_BDADDR(bd_addr));

    ret = bt_avrcp_disconnect_req(bd_addr);
    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_avrcp_disconnect failed");
        return -1;
    }

    return 0;
}

static int bcmd_avrcp_send_passthr_cmd(int argc, void **argv)
{
    CSM_AVRCP_CMD_TYPE cmd_type;
    T_PASSTHROUGH_OP_ID op_id;
    uint8_t *bd_addr;

    bd_addr = bt_avrcp_get_dev_addr();
    if (bd_addr == NULL)
    {
        APP_PRINT_WARN0("No avrcp connected device found!");
        return -1;
    }

    cmd_type = (CSM_AVRCP_CMD_TYPE)PTR_TO_INT(argv[0]);
    op_id = avrcp_get_op_id_from_customer_cmd_type(cmd_type);

    if (op_id != PASSTHROUGH_ID_MAX)
    {
        APP_PRINT_WARN1("op_id: %d", (int)op_id);
        avrcp_app_send_cmd_passthrough(bd_addr, (T_AVRCP_KEY)op_id, true);
    }
    return 0;
}

static int bcmd_avrcp_change_absvol(int argc, void **argv)
{
    bool ret;
    uint8_t vol;
    CSM_A2DP_ROLE role;
    uint8_t *bd_addr;

    vol = (uint8_t)PTR_TO_UINT(argv[0]);
    role = (CSM_A2DP_ROLE)PTR_TO_INT(argv[1]);

    if (role != CSM_A2DP_ROLE_SINK)
    {
        APP_PRINT_WARN1("%s: only support a2dp sink", __func__);
        return -1;
    }

    bd_addr = bt_avrcp_get_dev_addr();
    if (bd_addr == NULL)
    {
        APP_PRINT_WARN0("No avrcp connected device found!");
        return -1;
    }


    ret = bt_avrcp_notify_volume_change_req(bd_addr, vol);
    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_avrcp_change_absvol failed");
        return -1;
    }

    return 0;
}

static int bcmd_a2dp_connect(int argc, void **argv)
{
    bool ret;
    char *uuid = NULL;
    char *address;
    uint8_t remote_bd[6];
    uint8_t profile;

    uint16_t ver = 0x0103;

    address = argv[0];
    mac_str_to_bin(address, remote_bd);

    profile = (uint8_t)PTR_TO_UINT(argv[1]);
    if (profile == LOCAL_A2DP_SINK)
    {
        uuid = "0000110a-0000-1000-8000-00805f9b34fb";
    }
    else
    {
        uuid = "0000110b-0000-1000-8000-00805f9b34fb";
    }

    (void)uuid;
    //do we support src???
    ret = bt_a2dp_connect_req(remote_bd, ver);
    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_a2dp_connect failed");
        return -1;
    }

    return 0;
}

static int bcmd_a2dp_disconnect(int argc, void **argv)
{
    bool ret;
    char *address;
    uint8_t remote_bd[6];

    address = argv[0];

    mac_str_to_bin(address, remote_bd);

    ret = bt_a2dp_disconnect_req(remote_bd);
    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_a2dp_disconnect failed");
        return -1;
    }

    return 0;
}

static int bcmd_a2dp_paired_list(int argc, void **argv)
{
    bool ret;
    CSM_A2DP_DEV_INFO_LIST *info_list;
    uint8_t profile;

    APP_PRINT_INFO0("bcmd_a2dp_paired_list");

    info_list = argv[0];
    profile = (uint8_t)PTR_TO_UINT(argv[1]);
    memset(info_list, 0, sizeof(*info_list));

    (void)profile;
    ret = bt_a2dp_get_paired_dev_list(info_list);
    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_a2dp_paired_list failed");
        return -1;
    }

    return 0;
}

int bcmd_le_gatt_server_init(int argc, void **argv)
{
    return 0;
}

int bcmd_le_gatt_server_deinit(int argc, void **argv)
{
    return 0;
}

int bcmd_le_gatt_server_add_service(int argc, void **argv)
{
    bool ret;
    int32_t server_if;
    char *uuid;
    uint8_t is_primary;
    int32_t number;

    server_if = (int32_t)PTR_TO_INT(argv[0]);
    uuid = (char *)argv[1];
    is_primary = (uint8_t)PTR_TO_UINT(argv[2]);
    number = (int32_t)PTR_TO_INT(argv[3]);

    ret = bt_gatt_server_dynamic_add_service(
              server_if, uuid, is_primary, number);

    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_le_gatt_server_add_service failed");
        return -1;
    }

    return 0;
}

int bcmd_le_gatt_server_add_char(int argc, void **argv)
{
    bool ret;
    int32_t server_if;
    int32_t service_handle;
    char *uuid;
    int32_t properties;
    int32_t permissions;

    server_if = (int32_t)PTR_TO_INT(argv[0]);
    service_handle = (int32_t)PTR_TO_INT(argv[1]);
    uuid = (char *)argv[2];
    properties = (int32_t)PTR_TO_UINT(argv[3]);
    permissions = (int32_t)PTR_TO_INT(argv[4]);

    ret = bt_gatt_server_dynamic_add_char(server_if,
                                          service_handle, uuid, properties, permissions);

    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_le_gatt_server_add_char failed");
        return -1;
    }

    return 0;
}

int bcmd_le_gatt_server_add_desc(int argc, void **argv)
{
    bool ret;
    int32_t server_if;
    int32_t service_handle;
    char *uuid;
    int32_t permissions;

    server_if = (int32_t)PTR_TO_INT(argv[0]);
    service_handle = (int32_t)PTR_TO_INT(argv[1]);
    uuid = (char *)argv[2];
    permissions = (int32_t)PTR_TO_INT(argv[3]);

    ret = bt_gatt_server_dynamic_add_desc(server_if,
                                          service_handle, uuid, permissions);

    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_le_gatt_server_add_desc failed");
        return -1;
    }

    return 0;
}

int bcmd_le_gatt_server_start_service(int argc, void **argv)
{
    bool ret;
    int32_t server_if;
    int32_t service_handle;
    int32_t transport;

    server_if = (int32_t)PTR_TO_INT(argv[0]);
    service_handle = (int32_t)PTR_TO_INT(argv[1]);
    transport = (int32_t)PTR_TO_INT(argv[2]);

    ret = bt_gatt_server_dynamic_start_service(server_if,
                                               service_handle, transport);

    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_le_gatt_server_start_service failed");
        return -1;
    }

    return 0;
}

int bcmd_le_gatt_server_stop_service(int argc, void **argv)
{
    /*
    int32_t server_if;
    int32_t service_handle;

    server_if = (int32_t)PTR_TO_INT(argv[0]);
    service_handle = (int32_t)PTR_TO_INT(argv[1]);
    */
    //not suppoted now
    return 0;
}

int bcmd_le_gatt_server_del_service(int argc, void **argv)
{
    /*
    int32_t server_if;
    int32_t service_handle;

    server_if = (int32_t)PTR_TO_INT(argv[0]);
    service_handle = (int32_t)PTR_TO_INT(argv[1]);
    */
    //not suppoted now
    return 0;
}

int bcmd_le_gatt_server_unregister(int argc, void **argv)
{
    /*
    int32_t server_if;

    server_if = (int32_t)PTR_TO_INT(argv[0]);
    */
    //not supported now
    return 0;
}

int bcmd_le_gatt_server_send_rsp(int argc, void **argv)
{
    bool ret;
    int32_t conn_id;
    int32_t trans_id;
    int32_t status;
    int32_t handle;
    char   *p_val;
    int32_t val_len;
    int32_t auth_req;

    conn_id = (int32_t)PTR_TO_INT(argv[0]);
    trans_id = (int32_t)PTR_TO_INT(argv[1]);
    status = (int32_t)PTR_TO_INT(argv[2]);
    handle = (int32_t)PTR_TO_INT(argv[3]);
    p_val = (char *)argv[4];
    val_len = (int32_t)PTR_TO_UINT(argv[5]);
    auth_req = (int32_t)PTR_TO_INT(argv[6]);

    ret = bt_gatt_server_dynamic_send_rsp(
              conn_id, trans_id, status, handle,
              p_val, val_len, auth_req);

    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_le_gatt_server_send_rsp failed");
        return -1;
    }

    return 0;
}

int bcmd_le_gatt_server_send_indication(int argc, void **argv)
{
    bool ret;
    int32_t server_if;
    int32_t handle;
    int32_t conn_id;
    int32_t fg_confirm;
    char   *p_val;
    int32_t val_len;

    server_if = (int32_t)PTR_TO_INT(argv[0]);
    handle = (int32_t)PTR_TO_INT(argv[1]);
    conn_id = (int32_t)PTR_TO_INT(argv[2]);
    fg_confirm = (int32_t)PTR_TO_INT(argv[3]);
    p_val = (char *)argv[4];
    val_len = (int32_t)PTR_TO_UINT(argv[5]);

    ret = bt_gatt_server_dynamic_send_indication(
              server_if, handle, conn_id,
              fg_confirm, p_val, val_len);

    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_le_gatt_server_send_indication failed");
        return -1;
    }

    return 0;
}

int bcmd_le_gatt_server_enable_adv(int argc, void **argv)
{
    bool ret;
    uint8_t en;

    en = (uint8_t)PTR_TO_UINT(argv[0]);

    ret = bt_gatt_server_dynamic_enable_adv(en);

    if (ret == false)
    {
        APP_PRINT_ERROR0("bcmd_le_gatt_server_enable_adv failed");
        return -1;
    }

    return 0;
}

int bta_cmd_legacy_process(uint16_t opcode, int argc, void **argv)
{
    int result = -1;

    pr_debug("opcode %d", opcode);

    switch (opcode)
    {

    case BCMD_AVRCP_CONNECT:
        result = bcmd_avrcp_connect(argc, argv);
        break;
    case BCMD_AVRCP_DISCONNECT:
        result = bcmd_avrcp_disconnect(argc, argv);
        break;

    case BCMD_AVRCP_SEND_PASSTHR_CMD:
        result = bcmd_avrcp_send_passthr_cmd(argc, argv);
        break;

    case BCMD_AVRCP_CHG_VOL:
        result = bcmd_avrcp_change_absvol(argc, argv);
        break;

    case BCMD_A2DP_CONNECT:
        result = bcmd_a2dp_connect(argc, argv);
        break;
    case BCMD_A2DP_DISCONNECT:
        result = bcmd_a2dp_disconnect(argc, argv);
        break;

    case BCMD_A2DP_PAIRED_LIST:
        result = bcmd_a2dp_paired_list(argc, argv);
        break;

    case BCMD_READ_LOCAL_INFO:
        result = bcmd_read_local_info(argc, argv);
        break;

    case BCMD_READ_REMOTE_INFO:
        result = bcmd_read_remote_info(argc, argv);
        break;

    case BCMD_SET_SCAN_MODE:
        result = bcmd_set_scan_mode(argc, argv);
        break;

    case BCMD_SET_LOCAL_NAME:
        result = bcmd_set_local_name(argc, argv);
        break;

    case BCMD_START_DISCOVERY:
        result = bcmd_start_discovery(argc, argv);
        break;

    case BCMD_STOP_DISCOVERY:
        result = bcmd_stop_discovery(argc, argv);
        break;

    case BCMD_UNPAIR_DEVICE:
        result = bcmd_unpair_device(argc, argv);
        break;

    case BCMD_GATTS_INIT:
        result = bcmd_le_gatt_server_init(argc, argv);
        break;

    case BCMD_GATTS_DEINIT:
        result = bcmd_le_gatt_server_deinit(argc, argv);
        break;

    case BCMD_GATTS_ADD_SERVICE:
        result = bcmd_le_gatt_server_add_service(argc, argv);
        break;

    case BCMD_GATTS_ADD_CHAR:
        result = bcmd_le_gatt_server_add_char(argc, argv);
        break;

    case BCMD_GATTS_ADD_DESC:
        result = bcmd_le_gatt_server_add_desc(argc, argv);
        break;

    case BCMD_GATTS_START_SERVICE:
        result = bcmd_le_gatt_server_start_service(argc, argv);
        break;

    case BCMD_GATTS_STOP_SERVICE:
        result = bcmd_le_gatt_server_stop_service(argc, argv);
        break;

    case BCMD_GATTS_DELETE_SERVICE:
        result = bcmd_le_gatt_server_del_service(argc, argv);
        break;

    case BCMD_GATTS_UNREGISTER_SERVICE:
        result = bcmd_le_gatt_server_unregister(argc, argv);
        break;

    case BCMD_GATTS_SEND_RSP:
        result = bcmd_le_gatt_server_send_rsp(argc, argv);
        break;

    case BCMD_GATTS_SEND_INDICATION:
        result = bcmd_le_gatt_server_send_indication(argc, argv);
        break;

    case BCMD_GATTS_ENABLE_ADV:
        result = bcmd_le_gatt_server_enable_adv(argc, argv);

    default:
        pr_err("Unknown bta command %04x", opcode);
        break;
    }

    return result;
}

