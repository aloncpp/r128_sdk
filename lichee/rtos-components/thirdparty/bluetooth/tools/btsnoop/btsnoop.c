/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
//#include "hci/btsnoop.h"
//#include "hci/hci_layer.h"
//#include "common/bt_trace.h"
//#include "common/bt_target.h"
#include <sys/time.h>
//#include "osi/mutex.h"
#include "save_log_by_uart.h"

//todo
#include <stdlib.h>
#include <stdint.h>
#include "btsnoop.h"
#include <sys/time.h>
#include "kernel/os/os.h"
#include <machine/endian.h>

//#define LOG_TAG "bt_snoop"
//#define HCILOG_UART_ID UART1_ID

/* Message event mask across Host/Controller lib and stack */
#define MSG_EVT_MASK                    0xFF00 /* eq. BT_EVT_MASK */
#define MSG_SUB_EVT_MASK                0x00FF /* eq. BT_SUB_EVT_MASK */

/* Message event ID passed from Host/Controller lib to stack */
#define MSG_HC_TO_STACK_HCI_ERR        0x1300 /* eq. BT_EVT_TO_BTU_HCIT_ERR */
#define MSG_HC_TO_STACK_HCI_ACL        0x1100 /* eq. BT_EVT_TO_BTU_HCI_ACL */
#define MSG_HC_TO_STACK_HCI_SCO        0x1200 /* eq. BT_EVT_TO_BTU_HCI_SCO */
#define MSG_HC_TO_STACK_HCI_EVT        0x1000 /* eq. BT_EVT_TO_BTU_HCI_EVT */
#define MSG_HC_TO_STACK_L2C_SEG_XMIT   0x1900 /* eq. BT_EVT_TO_BTU_L2C_SEG_XMIT */

/* Message event ID passed from stack to vendor lib */
#define MSG_STACK_TO_HC_HCI_ACL        0x2100 /* eq. BT_EVT_TO_LM_HCI_ACL */
#define MSG_STACK_TO_HC_HCI_SCO        0x2200 /* eq. BT_EVT_TO_LM_HCI_SCO */
#define MSG_STACK_TO_HC_HCI_CMD        0x2000 /* eq. BT_EVT_TO_LM_HCI_CMD */

// Epoch in microseconds since 01/01/0000.
static const uint64_t BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;

typedef XR_OS_Mutex_t osi_mutex_t;
#define osi_mutex_new                 XR_OS_MutexCreate
#define osi_mutex_lock                XR_OS_MutexLock
#define osi_mutex_unlock              XR_OS_MutexUnlock
#define osi_mutex_free                XR_OS_MutexDelete

#define OSI_MUTEX_MAX_TIMEOUT         XR_OS_WAIT_FOREVER

static osi_mutex_t lock;
static uint32_t g_btsnoop_cfg;
const static struct save_log_iface *g_save_if;

typedef enum {
    kCommandPacket = 1,
    kAclPacket = 2,
    kScoPacket = 3,
    kEventPacket = 4
} packet_type_t;

typedef struct {
    uint32_t length_original;
    uint32_t length_captured;
    uint32_t flags;
    uint32_t dropped_packets;
    uint32_t time_hi;
    uint32_t time_lo;
    uint8_t type;
} __attribute__((__packed__)) btsnoop_header_t;

// Internal functions
static uint64_t btsnoop_timestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    //Timestamp is in microseconds.
    uint64_t timestamp = tv.tv_sec * 1000 * 1000LL;
    timestamp += tv.tv_usec;
    timestamp += BTSNOOP_EPOCH_DELTA;
    return timestamp;
}

static uint8_t btsnoop_ready = 0;

static void btsnoop_write_packet(uint8_t type, const uint8_t *packet, bool is_received)
{
    int length_he = 0;
    int flags = 0;

    switch (type) {
    case kCommandPacket:
        length_he = packet[2] + 4;
        flags = 2;
        break;
    case kAclPacket:
        length_he = (packet[3] << 8) + packet[2] + 5;
        flags = is_received;
        break;
    case kScoPacket:
        length_he = packet[2] + 4;
        flags = is_received;
        break;
    case kEventPacket:
        length_he = packet[1] + 3;
        flags = 3;
        break;
    }

    btsnoop_header_t header;
    header.length_original = __htonl(length_he);
    header.length_captured = header.length_original;
    header.flags = __htonl(flags);
    header.dropped_packets = 0;

    uint64_t timestamp = btsnoop_timestamp();
    header.time_hi = __htonl(timestamp >> 32);
    header.time_lo = __htonl(timestamp & 0xFFFFFFFF);
    header.type = type;

    osi_mutex_lock(&lock, OSI_MUTEX_MAX_TIMEOUT);

    if (!(g_btsnoop_cfg & BTSNOOP_CFG_WITHOUT_PACKET_HEADER))
        g_save_if->write((uint8_t *)&header, sizeof(btsnoop_header_t));
    else
        g_save_if->write((void *)&type, 1);
    g_save_if->write(packet, length_he - 1);

    osi_mutex_unlock(&lock);
}

int btsnoop_file_header(void)
{
    uint32_t temp;

    g_save_if->write("btsnoop", 8);
    temp = __htonl(1);
    g_save_if->write(&temp, 4);
    temp = __htonl(1002);
    g_save_if->write(&temp, 4);

    return 0;
}

// Module lifecycle functions
int btsnoop_start_up(const struct save_log_iface *iface, uint32_t cfg)
{
    int ret = -1;

    if (iface == NULL)
        return ret;

    g_btsnoop_cfg = cfg;
    g_save_if = iface;

    if (!(g_btsnoop_cfg & BTSNOOP_CFG_WITHOUT_FILE_HEADER))
        btsnoop_file_header();

    ret = 0;
    if (ret != 0) {
        return ret;
    }

    osi_mutex_new(&lock);

    btsnoop_ready = 1;

    return 0;
}

int btsnoop_shut_down(void)
{
    osi_mutex_free(&lock);

    g_save_if->shut_down();

    btsnoop_ready = 0;

    return 0;
}

// Interface function
#ifdef CONFIG_BT_DRIVERS_LIB
void btsnoop_capture(uint8_t type, const uint8_t *packet, bool is_received)
{
    if (!btsnoop_ready)
        return;
    btsnoop_write_packet(type, packet, is_received);
}
#else
#ifdef CONFIG_DRIVER_R128
void btsnoop_capture(uint8_t type, const BT_HDR * buffer, bool is_received)
{
    if (!btsnoop_ready)
        return;
    btsnoop_write_packet(type, (const uint8_t *)(buffer->data + buffer->offset), is_received);
}
#else
void btsnoop_capture(const BT_HDR *buffer, bool is_received)
{
    if (!btsnoop_ready)
        return;
    const uint8_t *p = buffer->data + buffer->offset;

    switch (buffer->event & MSG_EVT_MASK) {
    case MSG_HC_TO_STACK_HCI_EVT:
        btsnoop_write_packet(kEventPacket, p, false);
        break;
    case MSG_HC_TO_STACK_HCI_ACL:
    case MSG_STACK_TO_HC_HCI_ACL:
        btsnoop_write_packet(kAclPacket, p, is_received);
        break;
    case MSG_HC_TO_STACK_HCI_SCO:
    case MSG_STACK_TO_HC_HCI_SCO:
        btsnoop_write_packet(kScoPacket, p, is_received);
        break;
    case MSG_STACK_TO_HC_HCI_CMD:
        btsnoop_write_packet(kCommandPacket, p, true);
        break;
    }
}
#endif
#endif

