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
#include <stdio.h>
#include <string.h>
#include "hci_list.h"
#include "bt_lib.h"
#include "hci_distribute.h"

//handle and command link list
static hci_list_t *handle_list = NULL;
static hci_list_t *command_list = NULL;

/*define hci type*/
#define HCI_COMMAND                                 0X01
#define HCI_ACL                                     0X02
#define HCI_EVENT                                   0x04

/*define OGF*/
#define LE_OGF                                      0x20

/*define event code*/
#define CONNECTION_EVENT                            0x01
#define DISCONNECT_EVENT                            0x05
#define ENCRYPTION_CHANGE_EVENT                     0x08
#define ENHANCE_CONNECTION_EVENT                    0x0A
#define READ_REMOTE_VERSION_INFORMATION             0x0C
#define COMMAND_COMPLETE_EVENT                      0x0E
#define COMMAND_STATUS_EVENT                        0x0F
#define HARDWARE_ERROR_EVENT                        0x10
#define NUMBER_OF_COMPLETED_PACKET_EVENT            0x13
#define HCI_DATA_BUFFER_OVERFLOW_EVENT              0x1A
#define ENCRYPTION_KEY_REFRESH_COMPLETE_EVENT       0x30
#define HCI_LE_META_EVENT                           0x3E
#define AUTHENTICATED_PAYLOAD_TIMEOUT_EXPIRED_EVENT 0x57

#define HCI_STATUS_SUCCESS                           (0)

#define CAL_HANDLE(handle, byte1, byte2) \
                   do { \
                       handle = ((byte2 & 0x0f) << 8) | byte1; \
                   } while(0);

#define CAL_OPCODE(opcode, byte1, byte2) \
                   do { \
                       opcode = (byte2 << 8) | byte1; \
                   } while(0);

/* define traces for BTLIB */
#define HCI_DIST_TRACE_LEVEL_ERROR   (0)
#define HCI_DIST_TRACE_LEVEL_WARNING (1)
#define HCI_DIST_TRACE_LEVEL_DEBUG   (2)

#if defined(CONFIG_BT_DIST_MODE_CORRESPOND_DEBUG)
#define HCI_DIST_INITIAL_TRACE_LEVEL HCI_DIST_TRACE_LEVEL_DEBUG
#else
#define HCI_DIST_INITIAL_TRACE_LEVEL CONFIG_BT_DRIVERS_LIB_LOG_LEVEL
#endif

#define HCI_DIST_TRACE_ERROR(fmt, args...)       {if (HCI_DIST_INITIAL_TRACE_LEVEL >= HCI_DIST_TRACE_LEVEL_ERROR)   printf("<BT_HCI_DIST_E>" fmt, ##args);}
#define HCI_DIST_TRACE_WARNING(fmt, args...)     {if (HCI_DIST_INITIAL_TRACE_LEVEL >= HCI_DIST_TRACE_LEVEL_WARNING) printf("<BT_HCI_DIST_W>" fmt, ##args);}
#define HCI_DIST_TRACE_DEBUG(fmt, args...)       {if (HCI_DIST_INITIAL_TRACE_LEVEL >= HCI_DIST_TRACE_LEVEL_DEBUG)   printf("<BT_HCI_DIST_D>" fmt, ##args);}
#define HCI_DIST_TRACE_DEBUG_LOC()               {if (HCI_DIST_INITIAL_TRACE_LEVEL >= HCI_DIST_TRACE_LEVEL_DEBUG)   printf("<BT_HCI_DIST_D_LOC>%s %d\n", __func__, __LINE__);}

#if defined(CONFIG_BT_DIST_MODE_CORRESPOND_DEBUG)
typedef struct hci_record {
    uint32_t rec_num;
    uint32_t b_rec;
    uint32_t z_rec;
    uint32_t b_send;
    uint32_t z_send;
} hci_record_t;
static hci_record_t debug_hci_record;
#define HCI_RECORD_MAX (0xFFFFFFFF)

void hci_record_show(void)
{
    HCI_DIST_TRACE_DEBUG("======show hci=====\n");
    HCI_DIST_TRACE_DEBUG("rec_num is %d\n", debug_hci_record.rec_num);
    HCI_DIST_TRACE_DEBUG("b_rec is %d\n", debug_hci_record.b_rec);
    HCI_DIST_TRACE_DEBUG("z_rec is %d\n", debug_hci_record.z_rec);
    HCI_DIST_TRACE_DEBUG("b_send is %d\n", debug_hci_record.b_send);
    HCI_DIST_TRACE_DEBUG("z_send is %d\n", debug_hci_record.z_send);
}

void hci_recive_record(uint8_t feature)
{
    if (debug_hci_record.rec_num != HCI_RECORD_MAX) {
        debug_hci_record.rec_num++;
    } else {
        HCI_DIST_TRACE_WARNING("recive total num is full\n");
        debug_hci_record.rec_num = 0;
    }

    if (feature == BT_FEATURES_BR) {
        if (debug_hci_record.b_rec != HCI_RECORD_MAX) {
            debug_hci_record.b_rec++;
        } else {
            HCI_DIST_TRACE_WARNING("bluedroid recive num is full\n");
            debug_hci_record.b_rec = 0;
        }
    } else if (feature == BT_FEATURES_BLE) {
        if (debug_hci_record.z_rec != HCI_RECORD_MAX) {
            debug_hci_record.z_rec++;
        } else {
            HCI_DIST_TRACE_WARNING("zephyr recive num is full\n");
            debug_hci_record.z_rec = 0;
        }
    } else if (feature == BT_FEATURES_DUAL) {
        if (debug_hci_record.b_rec != HCI_RECORD_MAX) {
            debug_hci_record.b_rec++;
        } else {
            HCI_DIST_TRACE_WARNING("bluedroid recive num is full\n");
            debug_hci_record.b_rec = 0;
        }

        if (debug_hci_record.z_rec != HCI_RECORD_MAX) {
            debug_hci_record.z_rec++;
        } else {
            HCI_DIST_TRACE_WARNING("zephyr recive num is full\n");
            debug_hci_record.z_rec = 0;
        }
    } else {
            HCI_DIST_TRACE_ERROR("record recive error type\n");
    }
}

void hci_send_record(uint8_t feature)
{
    if (feature == BT_FEATURES_BR) {
        if (debug_hci_record.b_send != HCI_RECORD_MAX) {
            debug_hci_record.b_send++;
        } else {
            HCI_DIST_TRACE_WARNING("bluedroid send num is full\n");
            debug_hci_record.b_send = 0;
        }
    } else if (feature == BT_FEATURES_BLE) {
        if (debug_hci_record.z_send != HCI_RECORD_MAX) {
            debug_hci_record.z_send++;
        } else {
            HCI_DIST_TRACE_WARNING("zephyr send num is full\n");
            debug_hci_record.z_send = 0;
        }
    } else if (feature == BT_FEATURES_DUAL) {
        if (debug_hci_record.b_send != HCI_RECORD_MAX) {
            debug_hci_record.b_send++;
        } else {
            HCI_DIST_TRACE_WARNING("bluedroid recive num is full\n");
            debug_hci_record.b_send = 0;
        }

        if (debug_hci_record.z_send != HCI_RECORD_MAX) {
            debug_hci_record.z_send++;
        } else {
            HCI_DIST_TRACE_WARNING("zephyr recive num is full\n");
            debug_hci_record.z_send = 0;
        }

    } else {
        HCI_DIST_TRACE_ERROR("hci_write error!\n");
    }

}
#endif

int hci_distribute(const unsigned char *buff)
{
    uint32_t handle = 0;
    uint32_t opcode = 0;
    size_t loc = 0;

    if (buff[0] == HCI_EVENT) {
        switch (buff[1]) {
        case HCI_LE_META_EVENT:
            HCI_DIST_TRACE_DEBUG("le event buff is %x %x %x\n", buff[2], buff[3], buff[4]);

            if ((buff[3] == CONNECTION_EVENT) ||
                (buff[3] == ENHANCE_CONNECTION_EVENT) &&
                (buff[4] == HCI_STATUS_SUCCESS)) {
                CAL_HANDLE(handle, buff[5], buff[6]);
                HCI_DIST_TRACE_DEBUG("le handle is %d\n", handle);

                hci_list_append(handle_list, &handle);
            }
            return BT_FEATURES_BLE;

        case DISCONNECT_EVENT:
            HCI_DIST_TRACE_DEBUG("disconnection event!\n");
            CAL_HANDLE(handle, buff[4], buff[5]);
            HCI_DIST_TRACE_DEBUG("handle is %d\n", handle);

            loc = hci_list_value_exist(handle_list, handle);

            if (loc != 0) {
                hci_list_remove_node(handle_list, loc);
                return BT_FEATURES_BLE;
            } else {
                return BT_FEATURES_BR;
            }

        case HARDWARE_ERROR_EVENT:
        case HCI_DATA_BUFFER_OVERFLOW_EVENT:
            return BT_FEATURES_DUAL;

        case ENCRYPTION_CHANGE_EVENT:
        case READ_REMOTE_VERSION_INFORMATION:
        case NUMBER_OF_COMPLETED_PACKET_EVENT:
        case ENCRYPTION_KEY_REFRESH_COMPLETE_EVENT:
        case AUTHENTICATED_PAYLOAD_TIMEOUT_EXPIRED_EVENT:
            CAL_HANDLE(handle, buff[4], buff[5]);
            HCI_DIST_TRACE_DEBUG("-handle is %d event is 0x%x\n", handle, buff[1]);

            loc = hci_list_value_exist(handle_list, handle);

            if (loc != 0) {
                return BT_FEATURES_BLE;
            } else {
                return BT_FEATURES_BR;
            }

        case COMMAND_COMPLETE_EVENT:
            CAL_OPCODE(opcode, buff[4], buff[5]);
            HCI_DIST_TRACE_DEBUG("complete event opcode is 0x%4x\n", opcode);

            loc = hci_list_value_exist(command_list, opcode);

            if (loc != 0) {
                hci_list_remove_node(command_list, loc);
                return BT_FEATURES_BLE;
            } else if (buff[5] == LE_OGF){
                return BT_FEATURES_BLE;
            } else {
                return BT_FEATURES_BR;
            }

        case COMMAND_STATUS_EVENT:
            CAL_OPCODE(opcode, buff[5], buff[6]);
            HCI_DIST_TRACE_DEBUG("status event opcode is 0x%4x\n", opcode);

            loc = hci_list_value_exist(command_list, opcode);

            if (loc != 0) {
                hci_list_remove_node(command_list, loc);
                return BT_FEATURES_BLE;
            } else if (buff[6] == LE_OGF){
                return BT_FEATURES_BLE;
            } else {
                return BT_FEATURES_BR;
            }

        default:
            return BT_FEATURES_BR;
        }
    } else if (buff[0] == HCI_ACL) {
        CAL_HANDLE(handle, buff[1], buff[2]);
        HCI_DIST_TRACE_DEBUG("acl handle is %u\n", handle);

        loc = hci_list_value_exist(handle_list, handle);

        if (loc != 0) {
            return BT_FEATURES_BLE;
        } else {
            return BT_FEATURES_BR;
        }
    } else {
        HCI_DIST_TRACE_DEBUG("other type\n");
        return BT_FEATURES_BR;
    }
}

void hci_distribute_enter_list(uint8_t feature, uint8_t ocf, uint8_t ogf, uint8_t hci_type)
{
    uint32_t opcode = 0;
    if (feature == BT_FEATURES_BLE && hci_type == HCI_COMMAND) {
        CAL_OPCODE(opcode, ocf, ogf);
        HCI_DIST_TRACE_DEBUG("record 0x%x\n", opcode);

        hci_list_append(command_list, &opcode);
    }
}

int hci_distribute_init(void)
{
    command_list = hci_list_new(HCI_OPCODE_TYPE);
    if (command_list == NULL) {
        HCI_DIST_TRACE_ERROR("command list create failed\n");
        return -1;
    }

    handle_list = hci_list_new(HCI_HANDLE_TYPE);
    if (handle_list == NULL) {
        HCI_DIST_TRACE_ERROR("handle list create failed\n");
        hci_list_free(command_list);
        return -1;
    }

#if defined(CONFIG_BT_DIST_MODE_CORRESPOND_DEBUG)
    memset(&debug_hci_record, 0, sizeof(hci_record_t));
#endif

    return 0;
}

void hci_distribute_deinit(void)
{
#if defined(CONFIG_BT_DIST_MODE_CORRESPOND_DEBUG)
    memset(&debug_hci_record, 0, sizeof(hci_record_t));
#endif

    hci_list_free(command_list);
    hci_list_free(handle_list);
    command_list = NULL;
    handle_list = NULL;
}

