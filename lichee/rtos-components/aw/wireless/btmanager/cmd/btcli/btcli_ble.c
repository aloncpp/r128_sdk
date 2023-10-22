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
#include "cmd_util.h"
#include "ctype.h"
#include "btcli_common.h"
#include <bt_manager.h>

static enum cmd_status btcli_ble_help(char *cmd);
#define REASON_TO_STR(def)                                                                         \
    case BTMG_BLE_##def:                                                                           \
        return #def

static inline char *btcli_le_disconect_reason_to_str(gattc_disconnect_reason_t reason)
{
    switch (reason) {
        REASON_TO_STR(STATUS_CODE_SUCCESS);
        REASON_TO_STR(AUTHENTICATION_FAILURE);
        REASON_TO_STR(CONNECTION_TIMEOUT);
        REASON_TO_STR(REMOTE_USER_TERMINATED);
        REASON_TO_STR(LOCAL_HOST_TERMINATED);
        REASON_TO_STR(LMP_RESPONSE_TIMEOUT);
        REASON_TO_STR(FAILED_TO_BE_ESTABLISHED);
        REASON_TO_STR(UNKNOWN_OTHER_ERROR);
    default:
        return "UNKNOWN_OTHER_ERROR";
    }
}

static int btcli_addr_from_str(const char *str, btmg_addr_t *addr)
{
    int i, j;
    uint8_t tmp;

    if (strlen(str) != 17U) {
        return -1;
    }

    for (i = 0; i < 6; i++) {
        uint8_t hi, lo;
        char2hex(str[i * 3], &hi);
        char2hex(str[i * 3 + 1], &lo);
        if (hi < 0 || lo < 0) {
            return -1;
        }
        addr->val[i] = (uint8_t)(hi << 4 | lo);
    }
    return 0;
}

static int btcli_le_addr_from_str(const char *str, const char *type, btmg_le_addr_t *addr)
{
    int err;

    err = btcli_addr_from_str(str, &addr->addr);
    if (err < 0) {
        return err;
    }

    if (!strcmp(type, "public") || !strcmp(type, "(public)")) {
        addr->type = BTMG_LE_PUBLIC_ADDRESS;
    } else if (!strcmp(type, "random") || !strcmp(type, "(random)")) {
        addr->type = BTMG_LE_RANDOM_ADDRESS;
    } else if (!strcmp(type, "public-id") || !strcmp(type, "(public-id)")) {
        addr->type = BTMG_LE_PUBLIC_ADDRESS_ID;
    } else if (!strcmp(type, "random-id") || !strcmp(type, "(random-id)")) {
        addr->type = BTMG_LE_RANDOM_ADDRESS_ID;
    } else {
        return -1;
    }

    return 0;
}

static int btcli_le_addr_to_str(btmg_le_addr_t addr, char *str, int len)
{
    char type[10];

    switch (addr.type) {
    case BTMG_LE_RANDOM_ADDRESS:
        strcpy(type, "random");
        break;
    case BTMG_LE_PUBLIC_ADDRESS:
        strcpy(type, "public");
        break;
    case BTMG_LE_RANDOM_ADDRESS_ID:
        strcpy(type, "random-id");
        break;
    case BTMG_LE_PUBLIC_ADDRESS_ID:
        strcpy(type, "public-id");
        break;
    default:
        snprintk(type, sizeof(type), "0x%02x", addr.type);
        break;
    }

    return snprintk(str, len, "%02X:%02X:%02X:%02X:%02X:%02X (%s)", addr.addr.val[0],
                    addr.addr.val[1], addr.addr.val[2], addr.addr.val[3], addr.addr.val[4],
                    addr.addr.val[5], type);
}

int btcli_ble_set_adv_data(void)
{
    int index = 0;
    uint16_t uuid = 0x1234;
    btmg_adv_scan_rsp_data_t adv_data;

    adv_data.data[index] = 0x02; /* flag len */
    adv_data.data[index + 1] = 0x01; /* type for flag */
    adv_data.data[index + 2] = 0x1A; //0x05

    /* AD Type: Complete local name */
    // //reply in broadcast scan reply packet
    // char ble_name[] = "aw-btcli-ble";
    // index += adv_data.data[index] + 1;
    // adv_data.data[index] = strlen(ble_name) + 1; /* name len */
    // adv_data.data[index + 1] = 0x09; /* type for local name */
    // int name_len;
    // name_len = strlen(ble_name);
    // strcpy(&(adv_data.data[index + 2]), ble_name);

    index += adv_data.data[index] + 1;
    adv_data.data[index] = 0x03; /* uuid len */
    adv_data.data[index + 1] = 0x03; /* type for complete list of 16-bit uuid */
    adv_data.data[index + 2] = (char)(uuid & 0xFF);
    adv_data.data[index + 3] = (char)((uuid >> 8) & 0xFF);
    index += adv_data.data[index] + 1;

    adv_data.data_len = index;

    return btmg_le_set_adv_scan_rsp_data(&adv_data, NULL);
}

int btcli_ble_set_ext_adv_data(void)
{
    int index = 0;
    uint16_t uuid = 0x1234;
    btmg_adv_scan_rsp_data_t adv_data;

    adv_data.data[index] = 0x02; /* flag len */
    adv_data.data[index + 1] = 0x01; /* type for flag */
    adv_data.data[index + 2] = 0x06; //0x05

    index += adv_data.data[index] + 1;
    adv_data.data[index] = 0x02;
    adv_data.data[index + 1] = 0x0a; /* TX power level */
    adv_data.data[index + 2] = 0xeb;

    /* AD Type: Complete local name */
    //reply in broadcast scan reply packet
    // char ble_name[] = "aw-btcli-ble";
    // index += adv_data.data[index] + 1;
    // adv_data.data[index] = strlen(ble_name) + 1; /* name len */
    // adv_data.data[index + 1] = 0x09; /* type for local name */
    // int name_len;
    // name_len = strlen(ble_name);
    // strcpy(&(adv_data.data[index + 2]), ble_name);

    index += adv_data.data[index] + 1;
    adv_data.data[index] = 0x03; /* uuid len */
    adv_data.data[index + 1] = 0x03; /* type for complete list of 16-bit uuid */
    adv_data.data[index + 2] = (char)(uuid & 0xFF);
    adv_data.data[index + 3] = (char)((uuid >> 8) & 0xFF);
    index += adv_data.data[index] + 1;
    adv_data.data_len = index;

    return btmg_le_set_adv_scan_rsp_data(&adv_data, NULL);
}

int btcli_ble_advertise_on(void)
{
    btmg_le_adv_param_t adv_param;

    adv_param.interval_min = 0x0020;
    adv_param.interval_max = 0x01E0;
    adv_param.adv_type = BTMG_LE_ADV_IND;

    btmg_le_set_adv_param(&adv_param);
    btcli_ble_set_adv_data();
    return btmg_le_enable_adv(true);
}

int btcli_ble_ext_advertise_on(void)
{
    btmg_le_ext_adv_param_t ext_adv_param;

    ext_adv_param.prim_min_interval[0] = 0x00, ext_adv_param.prim_min_interval[1] = 0x00,
    ext_adv_param.prim_min_interval[2] = 0x30;
    ext_adv_param.prim_max_interval[0] = 0x00, ext_adv_param.prim_max_interval[1] = 0x00,
    ext_adv_param.prim_max_interval[2] = 0x30;

    btmg_le_set_ext_adv_param(&ext_adv_param);
    btcli_ble_set_ext_adv_data();
    return btmg_le_enable_ext_adv(true);
}

void btcli_ble_scan_cb(le_scan_cb_para_t *data)
{
    char addr[30];

    btcli_le_addr_to_str(data->addr, addr, sizeof(addr));
    CMD_DBG("[DEVICE]: %s, AD evt type %u, RSSI %i %s\n", addr, data->adv_type, data->rssi, data->name);
}

void btcli_ble_smp_passkey_display_cb(le_smp_passkey_display_para_t *data)
{
    CMD_DBG("%s, [DEVICE]: %d, passkey: %s\n", __func__, data->conn_id, data->passkey);
}

void btcli_ble_smp_passkey_confirm_cb(le_smp_passkey_confirm_para_t *data)
{
    int ret;

    CMD_DBG("%s, [DEVICE]: %d, passkey: %s\n", __func__, data->conn_id, data->passkey);

    ret = btmg_le_smp_passkey_confirm(data->conn_id); // 发送Pairing DHKey Check
    if (ret != 0) {
        CMD_DBG("Failed to send pairing confirm: %d\n", ret);
    } else {
        CMD_DBG("%s send pairing confirm \n");
    }
}

void btcli_ble_smp_passkey_enter_cb(le_smp_passkey_enter_para_t *data)
{
    char addr[30];

    btcli_le_addr_to_str(data->addr, addr, sizeof(addr));
    CMD_DBG("%s, [DEVICE]: %d, addr: %s\n", __func__, data->conn_id, addr);
    //TODO:输入密码
}

void btcli_ble_smp_cancel_cb(le_smp_cancel_para_t *data)
{
    char addr[30];

    btcli_le_addr_to_str(data->addr, addr, sizeof(addr));
    CMD_DBG("%s, [DEVICE]: %d, addr: %s\n", __func__, data->conn_id, addr);
}

void btcli_ble_smp_pairing_confirm_cb(le_smp_pairing_confirm_para_t *data)
{
    char addr[30];

    btcli_le_addr_to_str(data->addr, addr, sizeof(addr));
    CMD_DBG("%s, [DEVICE]: %d, addr: %s\n", __func__, data->conn_id, addr);
}

void btcli_ble_smp_pairing_failed_cb(le_smp_pairing_complete_para_t *data)
{
    char addr[30];

    btcli_le_addr_to_str(data->addr, addr, sizeof(addr));
    CMD_DBG("%s, [DEVICE]: %d, addr: %s, reason: %d\n", __func__, data->conn_id, addr, data->err);
}

void btcli_ble_smp_pairing_complete_cb(le_smp_pairing_complete_para_t *data)
{
    char addr[30];

    btcli_le_addr_to_str(data->addr, addr, sizeof(addr));
    CMD_DBG("%s, [DEVICE]: %d, addr: %s, bond result: %d\n", __func__, data->conn_id, addr,
            data->bonded);
}

void btcli_ble_connection_cb(le_connection_para_t *data)
{
    char addr[30];
    btcli_le_addr_to_str(data->addr, addr, 30);

    if (data->role == 0) {
        if (data->status == LE_CONNECTED) {
            CMD_DBG("gattc connect success,id=[%d] %s\n", data->conn_id, addr);
            btmg_le_gatt_mtu_exchange(data->conn_id);
        } else if (data->status == LE_CONNECT_FAIL){
            CMD_DBG("gattc connect failed,id=[%d] %s\n", data->conn_id, addr);
        } else if (data->status == LE_DISCONNECTED) {
            CMD_DBG("gattc disconnected, reason= %s, id=[%d] %s\n",
            btcli_le_disconect_reason_to_str(data->reason), data->conn_id, addr);
        }
    } else {
        if (data->status == LE_CONNECTED) {
            // The role is server, create a data database, and first obtain the maximum mtu
            btcli_gatts_attributedata_create(data->conn_id, 0);
            CMD_DBG("gatts connected,id=[%d] %s\n", data->conn_id, addr);
        } else if (data->status == LE_CONNECT_FAIL){
            CMD_DBG("gatts connect failed,id=[%d] %s\n", data->conn_id, addr);
        } else if (data->status == LE_DISCONNECTED) {
            // The role is server, after the client link is disconnected, the database deleted
            btcli_gatts_attributedata_delete(data->conn_id);
            CMD_DBG("gatts disconnected, reason[0x%02x]= %s, id=[%d] %s\n",
                    data->reason ,btcli_le_disconect_reason_to_str(data->reason), data->conn_id, addr);
        }
    }
}

enum cmd_status btcli_ble_scan(char *cmd)
{
    int argc;
    char *argv[5];
    int err;
    int i = 0;

    argc = cmd_parse_argv(cmd, argv, 5);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    btmg_le_scan_param_t scan_param = { 0 };
    {
        scan_param.scan_type = LE_SCAN_TYPE_ACTIVE;
        scan_param.scan_interval = 0x0320;
        scan_param.scan_window = 0x0190;
        scan_param.filter_duplicate = LE_SCAN_DUPLICATE_DISABLE;
        scan_param.filter_policy = LE_SCAN_FILTER_POLICY_ALLOW_ALL;
        scan_param.timeout = 0;
    }

    while (i < argc) {
        if (!cmd_strcmp(argv[i], "on")) {
            CMD_DBG("Use default scan parameters\n");
        } else if (!cmd_strcmp(argv[i], "off")) {
            err = btmg_le_scan_stop();
            if (err) {
                CMD_ERR("LE scan stop failed\n");
                return err;
            } else {
                CMD_DBG("LE scan stopped successfully\n");
            }
            return CMD_STATUS_OK;
        } else if (!cmd_strcmp(argv[i], "passive")) {
            scan_param.scan_type = LE_SCAN_TYPE_PASSIVE;
        } else if (!cmd_strcmp(argv[i], "dups")) {
            scan_param.filter_duplicate = LE_SCAN_DUPLICATE_DISABLE;
        } else if (!cmd_strcmp(argv[i], "nodups")) {
            scan_param.filter_duplicate = LE_SCAN_DUPLICATE_ENABLE;
        } else if (!cmd_strcmp(argv[i], "wl")) {
            scan_param.filter_policy = LE_SCAN_FILTER_POLICY_ONLY_WLIST;
        } else if (!cmd_strcmp(argv[i], "active")) {
            scan_param.scan_type = LE_SCAN_TYPE_ACTIVE;
        } else if (!cmd_strcmp(argv[i], "timeout")) {
            scan_param.timeout = strtoul(argv[i], NULL, 16);
        } else if (!cmd_strncmp(argv[i], "int=0x", 4)) {
            uint32_t num;
            int cnt = cmd_sscanf(argv[i] + 4, "%x", &num);
            if (cnt != 1) {
                CMD_ERR("invalid param %s\n", argv[i] + 4);
                return CMD_STATUS_INVALID_ARG;
            }
            scan_param.scan_interval = num;
            CMD_DBG("scan interval 0x%x\n", scan_param.scan_interval);
        } else if (!cmd_strncmp(argv[i], "win=0x", 4)) {
            uint32_t num;
            int cnt = cmd_sscanf(argv[i] + 4, "%x", &num);
            if (cnt != 1) {
                CMD_ERR("invalid param %s\n", argv[i] + 4);
                return CMD_STATUS_INVALID_ARG;
            }
            scan_param.scan_window = num;
            CMD_DBG("scan window 0x%x\n", scan_param.scan_window);
        } else {
            CMD_ERR("invalid param %s\n", argv[i]);
            return CMD_STATUS_INVALID_ARG;
        }
        i++;
    }
    err = btmg_le_scan_start(&scan_param);
    if (err) {
        CMD_ERR("LE scan start failed\n");
        return err;
    } else {
        CMD_DBG("LE scan started!\n");
    }

    return CMD_STATUS_OK;
}

enum cmd_status btcli_ble_adv(char *cmd)
{
    int argc;
    char *argv[5];
    int i = 0;
    int err;
    int start_adv = 0;
    btmg_adv_scan_rsp_data_t btmg_adv_data = { 0 };
    btmg_adv_scan_rsp_data_t btmg_scan_data = { 0 };

    argc = cmd_parse_argv(cmd, argv, 5);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    while (i < argc) {
        if (!cmd_strcmp(argv[i], "on")) {
            start_adv = 1;
        } else if (!cmd_strcmp(argv[i], "off")) {
            err = btmg_le_enable_adv(0);
            if (err) {
                CMD_ERR("Disable Advertising failed,(err %d)\n");
                return err;
            } else {
                CMD_DBG("Advertising stopped\n");
            }
            return 0;
        } else if (!cmd_strncmp(argv[i], "adv=", 4)) {
            btmg_adv_data.data_len = hex2bin(argv[i] + 4, strlen(argv[i]) - 4, btmg_adv_data.data,
                                             sizeof(btmg_adv_data.data));
            if (btmg_adv_data.data_len == 0) {
                CMD_ERR("No data set\n");
                return CMD_STATUS_INVALID_ARG;
            }
        } else if (!cmd_strncmp(argv[i], "scan=", 5)) {
            btmg_scan_data.data_len = hex2bin(argv[i] + 5, strlen(argv[i]) - 5, btmg_scan_data.data,
                                              sizeof(btmg_scan_data.data));
            if (btmg_scan_data.data_len == 0) {
                CMD_ERR("No data set\n");
                return CMD_STATUS_INVALID_ARG;
            }
        }
        i++;
    }
    btmg_le_set_adv_scan_rsp_data(&btmg_adv_data, &btmg_scan_data);
    if (start_adv = 1) {
        btmg_le_set_adv_param(NULL);
        err = btmg_le_enable_adv(true);
        CMD_DBG("Advertising started\n");
    }

    return CMD_STATUS_OK;
}

enum cmd_status btcli_ble_name(char *cmd)
{
    int argc;
    char *argv[1];
    int err;

    argc = cmd_parse_argv(cmd, argv, 1);

    if (argc < 1) {
        CMD_DBG("Bluetooth Local ble Name: %s\n", btmg_le_get_name());
        return CMD_STATUS_OK;
    }

    err = btmg_le_set_name(argv[0]);
    if (err) {
        CMD_DBG("Unable to set ble name %s (err %d)\n", argv[0], err);
        return CMD_STATUS_FAIL;
    }
    return CMD_STATUS_OK;
}

enum cmd_status btcli_ble_connect(char *cmd)
{
    int argc;
    char *argv[5];
    int err;
    bt_addr_le_t addr;

    argc = cmd_parse_argv(cmd, argv, 5);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    err = bt_addr_le_from_str(argv[0], argv[1], &addr);
    if (err) {
        CMD_ERR("Invalid peer address (err %d)\n", err);
        return CMD_STATUS_INVALID_ARG;
    }

    btmg_le_addr_t peer;
    peer.type = BTMG_LE_RANDOM_ADDRESS;

    btmg_le_conn_param_t conn_param;
    conn_param.min_conn_interval = 0x0010;
    conn_param.max_conn_interval = 0x0020;
    conn_param.slave_latency = 0x0000;
    conn_param.conn_sup_timeout = 0x0050;
    memcpy(peer.addr.val, addr.a.val, 6);

    if (addr.type == BT_ADDR_LE_PUBLIC) {
        peer.type = BTMG_LE_PUBLIC_ADDRESS;
    } else if (addr.type == BT_ADDR_LE_RANDOM) {
        peer.type = BTMG_LE_RANDOM_ADDRESS;
    } else {
        return CMD_STATUS_INVALID_ARG;
    }

    int i = 2;
    while (i < argc) {
        if (!cmd_strncmp(argv[i], "lat=", 4)) {
            uint32_t num;
            int cnt = cmd_sscanf(argv[i] + 4, "%x", &num);
            if (cnt != 1) {
                CMD_ERR("invalid param %s\n", argv[i] + 4);
                return CMD_STATUS_INVALID_ARG;
            }
            conn_param.slave_latency = (uint16_t)(num);
            CMD_DBG("conn latency 0x%x\n", conn_param.slave_latency);
        } else if (!cmd_strncmp(argv[i], "to=", 3)) {
            uint32_t num;
            int cnt = cmd_sscanf(argv[i] + 3, "%x", &num);
            if (cnt != 1) {
                CMD_ERR("invalid param %s\n", argv[i] + 4);
                return CMD_STATUS_INVALID_ARG;
            }
            conn_param.conn_sup_timeout = (uint16_t)(num);
            CMD_DBG("conn timeout 0x%x\n", conn_param.conn_sup_timeout);
        } else if (!cmd_strncmp(argv[i], "min=", 4)) {
            uint32_t num;
            int cnt = cmd_sscanf(argv[i] + 4, "%x", &num);
            if (cnt != 1) {
                CMD_ERR("invalid param %s\n", argv[i] + 4);
                return CMD_STATUS_INVALID_ARG;
            }
            conn_param.min_conn_interval = (uint16_t)(num);
            CMD_DBG("conn interval_min 0x%x\n", conn_param.min_conn_interval);
        } else if (!cmd_strncmp(argv[i], "max=", 4)) {
            uint32_t num;
            int cnt = cmd_sscanf(argv[i] + 4, "%x", &num);
            if (cnt != 1) {
                CMD_ERR("invalid param %s\n", argv[i] + 4);
                return CMD_STATUS_INVALID_ARG;
            }
            conn_param.max_conn_interval = (uint16_t)(num);
            CMD_DBG("conn interval_max 0x%x\n", conn_param.max_conn_interval);
        } else {
            CMD_ERR("invalid param %s\n", argv[i]);
            return CMD_STATUS_INVALID_ARG;
        }
        i++;
    }
    err = btmg_le_connect(&peer, &conn_param);
    if (err) {
        CMD_ERR("ble connect failed\n");
        return -1;
    }

    return CMD_STATUS_OK;
}

enum cmd_status btcli_ble_disconnect(char *cmd)
{
    int argc;
    char *argv[3];
    int err;
    uint8_t default_conn_id = 0;

    argc = cmd_parse_argv(cmd, argv, 3);

    if (default_conn_id >= 0 && argc < 3) {
        err = btmg_le_disconnect(default_conn_id, 0);
    } else {
        // bt_addr_le_t addr;
        // if (argc < 3) {
        //     shell_help(shell);
        //     return SHELL_CMD_HELP_PRINTED;
        // }
        // err = bt_addr_le_from_str(argv[1], argv[2], &addr);
        // if (err) {
        //     shell_error(shell, "Invalid peer address (err %d)", err);
        //     return err;
        // }
        // err = btmg_le_disconnect(0, 0);
    }
    return CMD_STATUS_OK;
}

enum cmd_status btcli_ble_connections(char *cmd)
{

    char addr[30];
    int dev_num = 0;

    btmg_le_get_connected_num(&dev_num);
    gattc_connected_list_para_t param[dev_num];
    btmg_le_get_connected_list(param);

    for (int i = 0; i < dev_num; i++) {
        btcli_le_addr_to_str(param[i].addr, addr, 30);
        CMD_DBG("DEVICE[%s],conn_id[%d]\n", addr, param[i].conn_id);
    }
    CMD_DBG("CONN DEVICE Total:%d\n", dev_num);
    return CMD_STATUS_OK;
}

#if defined(CONFIG_BT_WHITELIST)
enum cmd_status btcli_ble_wl_add(char *cmd)
{
    int argc;
    char *argv[2];
    int err;
    btmg_le_addr_t addr;

    argc = cmd_parse_argv(cmd, argv, 2);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    err = btcli_le_addr_from_str(argv[0], argv[1], &addr);
    if (err) {
        CMD_ERR("Invalid peer address (err %d)\n", err);
        return CMD_STATUS_INVALID_ARG;
    }

    err = btmg_le_whitelist_add(&addr);
    if (err) {
        CMD_ERR("Add to whitelist failed (err %d)\n", err);
        return CMD_STATUS_INVALID_ARG;
    }

    return CMD_STATUS_OK;
}

enum cmd_status btcli_ble_wl_rem(char *cmd)
{
    int argc;
    char *argv[2];
    int err;
    btmg_le_addr_t addr;

    argc = cmd_parse_argv(cmd, argv, 2);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    err = btcli_le_addr_from_str(argv[0], argv[1], &addr);
    if (err) {
        CMD_ERR("Invalid peer address (err %d)\n", err);
        return CMD_STATUS_INVALID_ARG;
    }

    err = btmg_le_white_list_remove(&addr);
    if (err) {
        CMD_ERR("Remove from whitelist failed (err %d)\n", err);
        return CMD_STATUS_INVALID_ARG;
    }

    return CMD_STATUS_OK;
}

enum cmd_status btcli_ble_wl_clear(char *cmd)
{
    int err;

    err = btmg_le_whitelist_clear();
    if (err) {
        CMD_ERR("Clearing whitelist failed (err %d)\n", err);
        return CMD_STATUS_FAIL;
    }

    return CMD_STATUS_OK;
}

enum cmd_status blecli_ble_wl_connect(char *cmd)
{
    int argc;
    char *argv[1];
    int err;
    btmg_le_conn_param_t conn_param;

    argc = cmd_parse_argv(cmd, argv, 1);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    conn_param.min_conn_interval = 0x0018;
    conn_param.max_conn_interval = 0x0028;
    conn_param.slave_latency = 0;
    conn_param.conn_sup_timeout = 400;

    if (!strcmp(argv[0], "on")) {
        err = btmg_le_connect_auto_start(&conn_param);
        if (err) {
            CMD_ERR("Auto connect failed (err %d)\n", err);
            return CMD_STATUS_OK;
        }
    } else if (!strcmp(argv[0], "off")) {
        err = btmg_le_connect_auto_stop();
        if (err) {
            CMD_ERR("Auto connect stop failed (err %d)\n", err);
            return CMD_STATUS_OK;
        }
    }

    return CMD_STATUS_OK;
}
#else //!defined(CONFIG_BT_WHITELIST)
enum cmd_status btcli_ble_auto_conn(char *cmd)
{
    int argc;
    char *argv[3];
    int err;
    btmg_le_addr_t addr;
    btmg_le_conn_param_t conn_param;

    argc = cmd_parse_argv(cmd, argv, 3);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    conn_param.min_conn_interval = 0x0018;
    conn_param.max_conn_interval = 0x0028;
    conn_param.slave_latency = 0;
    conn_param.conn_sup_timeout = 400;

    err = btmg_addr_le_from_str(argv[0], argv[1], &addr);
    if (err) {
        CMD_ERR("Invalid peer address (err %d)\n", err);
        return CMD_STATUS_INVALID_ARG;
    }

    if (argc < 4) {
        return btmg_le_set_auto_connect(&addr, &conn_param);
    } else if (!strcmp(argv[2], "on")) {
        return btmg_le_set_auto_connect(&addr, &conn_param);
    } else if (!strcmp(argv[2], "off")) {
        return btmg_le_set_auto_connect(&addr, NULL);
    }
}
#endif

#if defined(CONFIG_BT_SMP)
enum cmd_status btcli_ble_get_sec(char *cmd)
{
    int argc;
    char *argv[1];
    int conn_id = 0;

    argc = cmd_parse_argv(cmd, argv, 1);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    conn_id = strtoul(argv[0], NULL, 16);

    int sec = btmg_le_get_security(conn_id);
    CMD_ERR("conn_id=%d, security=%d\n", conn_id, sec);

    return CMD_STATUS_OK;
}

enum cmd_status btcli_ble_set_sec(char *cmd)
{
    int argc;
    char *argv[2];
    int sec, ret, conn_id = 0;

    argc = cmd_parse_argv(cmd, argv, 2);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    sec = strtoul(argv[0], NULL, 16);
    if (argc == 2) {
        conn_id = strtoul(argv[1], NULL, 16);
    }
    ret = btmg_le_set_security(conn_id, sec);
    if (ret) {
        CMD_ERR("conn_id=%d, security=%d failed\n", conn_id, sec);
        return CMD_STATUS_OK;
    }
    CMD_ERR("conn_id=%d, security=%d success\n", conn_id, sec);

    return CMD_STATUS_OK;
}

enum cmd_status btcli_ble_unpair(char *cmd)
{
    int argc;
    char *argv[2];
    int err;
    btmg_le_addr_t addr;

    argc = cmd_parse_argv(cmd, argv, 2);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    err = btcli_le_addr_from_str(argv[0], argv[1], &addr);
    if (err) {
        CMD_ERR("Invalid peer address (err %d)\n", err);
        return CMD_STATUS_INVALID_ARG;
    }

    err = btmg_le_unpair(&addr);
    if (err) {
        CMD_DBG("Unable to unpair addr %s (err %d)\n", argv[0], err);
        return CMD_STATUS_FAIL;
    }
    return CMD_STATUS_OK;

}
#endif

static const struct cmd_data ble_cmds[] = {
    { "scan",           btcli_ble_scan,          CMD_DESC("<on/off/passive> [int=0x0100] [win=0x0100] [silent]")},
    { "adv",            btcli_ble_adv,           CMD_DESC("<on/off> [adv=020106] [scan=]")},
    { "name",           btcli_ble_name,          CMD_DESC("[name]")},
    { "connect",        btcli_ble_connect,       CMD_DESC("<address: XX:XX:XX:XX:XX:XX> <type: (public|random)> [min=] [max=] [to=]")},
    { "disconnect",     btcli_ble_disconnect,    CMD_DESC("[none]")},
    { "connections",    btcli_ble_connections,   CMD_DESC("[none]")},
#if defined(CONFIG_BT_WHITELIST)
    { "wl_add",         btcli_ble_wl_add,        CMD_DESC("<address: XX:XX:XX:XX:XX:XX> <type: (public|random)>")},
    { "wl_rem",         btcli_ble_wl_rem,        CMD_DESC("<address: XX:XX:XX:XX:XX:XX> <type: (public|random)>")},
    { "wl_clear",       btcli_ble_wl_clear,      CMD_DESC("[none]")},
    { "wl_connect",     btcli_ble_connect,       CMD_DESC("<on, off>")},
#else
    { "auto_conn",      btcli_ble_auto_conn,     CMD_DESC("<address: XX:XX:XX:XX:XX:XX> <type: (public|random)> [value:on,off]")},
#endif
#if defined(CONFIG_BT_SMP)
    { "get_sec",        btcli_ble_get_sec,       CMD_DESC("[conn_id]")},
    { "set_sec",        btcli_ble_set_sec,       CMD_DESC("<security level: >=1 > [conn_id]")},
    { "unpair",         btcli_ble_unpair,        CMD_DESC("<address: XX:XX:XX:XX:XX:XX>")},
#endif
    { "help",           btcli_ble_help,          CMD_DESC(CMD_HELP_DESC)},
};

/* btcli ble help */
static enum cmd_status btcli_ble_help(char *cmd)
{
	return cmd_help_exec(ble_cmds, cmd_nitems(ble_cmds), 10);
}

enum cmd_status btcli_ble(char *cmd)
{
    return cmd_exec(cmd, ble_cmds, cmd_nitems(ble_cmds));
}
