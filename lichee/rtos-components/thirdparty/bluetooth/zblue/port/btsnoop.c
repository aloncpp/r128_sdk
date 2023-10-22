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
#include <stdlib.h>
#include <stdint.h>
#include "btsnoop.h"
#include <sys/time.h>
#include "kernel/os/os.h"
#include "save_log_by_uart.h"
#include <machine/endian.h>
//#define HCILOG_UART_ID UART0_ID

// Epoch in microseconds since 01/01/0000.
static const uint64_t BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;
static XR_OS_Mutex_t lock;

#ifndef CPU_LITTLE_ENDIAN
#define CPU_LITTLE_ENDIAN
#endif

uint16_t swap_byte_16(uint16_t x)
{
    return (((x & 0x00ffU) << 8) |
            ((x & 0xff00U) >> 8));
}

uint32_t swap_byte_32(uint32_t x)
{
    return (((x & 0x000000ffUL) << 24) |
            ((x & 0x0000ff00UL) << 8) |
            ((x & 0x00ff0000UL) >> 8) |
            ((x & 0xff000000UL) >> 24));
}

#ifndef ntohs
inline uint16_t ntohs(uint16_t x)
{
#ifdef CPU_LITTLE_ENDIAN
    return swap_byte_16(x);
#else
    return x;
#endif
}
#endif /* #ifndef ntohs */

#ifndef htons
inline uint16_t htons(uint16_t x)
{
#ifdef CPU_LITTLE_ENDIAN
    return swap_byte_16(x);
#else
    return x;
#endif
}
#endif /* #ifndef htons */

#ifndef ntohl
inline uint32_t ntohl(uint32_t x)
{
#ifdef CPU_LITTLE_ENDIAN
    return swap_byte_32(x);
#else
    return x;
#endif
}
#endif /* #ifndef ntohl*/

#ifndef htonl
inline uint32_t htonl(uint32_t x)
{
#ifdef CPU_LITTLE_ENDIAN
    return swap_byte_32(x);
#else
    return x;
#endif
}
#endif /* #ifndef htonl*/


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

typedef enum {
    kCommandPacket = 1,
    kAclPacket = 2,
    kScoPacket = 3,
    kEventPacket = 4
} packet_type_t;

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
	default:
		return;
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

    if (get_uart_save_log_type() == UART_SAVE_BT_LOG) {
        XR_OS_MutexLock(&lock, -1);

	#if SYNC_WITH_HCIDUMP
        uart_save_log_write((uint8_t *)&header, sizeof(btsnoop_header_t));
	#else
		uart_save_log_write((void *)&type, 1);
	#endif

        uart_save_log_write(packet, length_he - 1);

        XR_OS_MutexUnlock(&lock);
    }
}

// Module lifecycle functions
int btsnoop_start_up(void)
{
    int ret = -1;
    ret = uart_save_log_start_up(UART_SAVE_BT_LOG);
    if (ret != 0) {
        return ret;
    }

    XR_OS_MutexCreate(&lock);

    return 0;
}

int btsnoop_shut_down(void)
{
    XR_OS_MutexDelete(&lock);

    return uart_save_log_shut_down();
}
#if 0
// Interface function
void btsnoop_capture(const BT_HDR *buffer, bool is_received)
{
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
void btsnoop_capture(uint8_t type, const uint8_t *buf, bool is_received)
{
    btsnoop_write_packet(type, buf, is_received);
}
