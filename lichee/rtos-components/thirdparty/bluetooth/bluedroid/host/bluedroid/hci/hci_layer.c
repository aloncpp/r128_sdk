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
#include <string.h>
#include "common/bt_defs.h"
#include "common/bt_trace.h"
#include "stack/hcidefs.h"
#include "stack/hcimsgs.h"
#include "stack/btu.h"
#include "hci/bt_vendor_lib.h"
#include "hci/hci_internals.h"
#include "hci/hci_hal.h"
#include "hci/hci_layer.h"
#include "osi/allocator.h"
#include "hci/packet_fragmenter.h"
#include "hci/buffer_allocator.h"
#include "osi/list.h"
#include "osi/alarm.h"
#include "osi/thread.h"
#include "osi/mutex.h"
#include "osi/fixed_queue.h"
//#include "hci/btsnoop.h"
// #include "hci/low_power_manager.h"

#ifndef HCI_HOST_TASK_PINNED_TO_CORE
#define HCI_HOST_TASK_PINNED_TO_CORE    (TASK_PINNED_TO_CORE)
#endif
#ifndef HCI_HOST_TASK_STACK_SIZE
#define HCI_HOST_TASK_STACK_SIZE        (2048 + EXTRA_TX_STACK + BT_TASK_EXTRA_STACK_SIZE)
#endif
#ifndef HCI_HOST_TASK_PRIO
#define HCI_HOST_TASK_PRIO              (BT_TASK_MAX_PRIORITIES - 2)
#endif
#ifndef HCI_HOST_TASK_NAME
#define HCI_HOST_TASK_NAME              "hciT"
#endif

typedef struct {
    uint16_t opcode;
    future_t *complete_future;
    command_complete_cb complete_callback;
    command_status_cb status_callback;
    void *context;
    BT_HDR *command;
} waiting_command_t;

typedef struct {
    bool timer_is_set;
    osi_alarm_t *command_response_timer;
    list_t *commands_pending_response;
    osi_mutex_t commands_pending_response_lock;
} command_waiting_response_t;

typedef struct {
    int command_credits;
    fixed_queue_t *command_queue;
    fixed_queue_t *packet_queue;

    command_waiting_response_t cmd_waiting_q;

    /*
      non_repeating_timer_t *command_response_timer;
      list_t *commands_pending_response;
      osi_mutex_t commands_pending_response_lock;
    */
} hci_host_env_t;

#if 0
typedef enum hci_packetize_state {
    HCI_BRAND_NEW,
    HCI_PACKET_TYPE,
    HCI_PREAMBLE,
    HCI_PAYLOAD,
} hci_packetize_state_t;

typedef struct {
#define HCI_PACKET_TYPE_SIZE (1) // packet indication domain size
#define PREAMBLE_BUFFER_SIZE (4) // max preamble size, ACL
    hci_packetize_state_t state;
    uint16_t bytes_remaining;
    uint8_t preamble[HCI_PACKET_TYPE_SIZE + PREAMBLE_BUFFER_SIZE];
    uint16_t index;
    BT_HDR *buffer;
} packet_receive_data_t;

#define PACKET_TYPE_TO_INBOUND_INDEX(type) ((type) - 2)
#define PACKET_TYPE_TO_INDEX(type) ((type) - 1)

static const uint8_t preamble_sizes_for_type[] = {
    0,
    HCI_COMMAND_PREAMBLE_SIZE,
    HCI_ACL_PREAMBLE_SIZE,
    HCI_SCO_PREAMBLE_SIZE,
    HCI_EVENT_PREAMBLE_SIZE
};

const uint8_t packet_length_offset_for_type[] = {
    0,
    HCI_LENGTH_OFFSET_CMD,
    HCI_LENGTH_OFFSET_ACL,
    HCI_LENGTH_OFFSET_SCO,
    HCI_LENGTH_OFFSET_EVT
};

static const uint16_t outbound_event_types[] = {
    MSG_HC_TO_STACK_HCI_ERR,
    MSG_HC_TO_STACK_HCI_ACL,
    MSG_HC_TO_STACK_HCI_SCO,
    MSG_HC_TO_STACK_HCI_EVT
};

static inline int32_t HciGetPacketLengthForType(enum HciPacketType type, const uint8_t* preamble) {
    uint8_t offset = packet_length_offset_for_type[type];
    if (type != HCI_PACKET_TYPE_ACL_DATA)
        return preamble[offset];
    return (((preamble[offset + 1]) << 8) | preamble[offset]);
}
#endif

// Using a define here, because it can be stringified for the property lookup
static const uint32_t COMMAND_PENDING_TIMEOUT = 8000;

// Our interface
static bool interface_created;
static hci_t interface;
static hci_host_env_t hci_host_env;
static osi_thread_t *hci_host_thread;

#if CONFIG_HCI_LAYER_RX
//static OS_Thread_t hci_rx_thread;
static osi_thread_t *hci_rx_thread;
#endif //CONFIG_HCI_LAYER_RX

static bool hci_host_startup_flag;

// Modules we import and callbacks we export
static const hci_hal_t *hal;
static const hci_hal_callbacks_t hal_callbacks;
static const packet_fragmenter_t *packet_fragmenter;
static const packet_fragmenter_callbacks_t packet_fragmenter_callbacks;
// // static const low_power_manager_t *low_power_manager;

static int hci_layer_init_env(void);
static void hci_layer_deinit_env(void);
static void hci_host_thread_handler(void *arg);
static void event_command_ready(fixed_queue_t *queue);
static void event_packet_ready(fixed_queue_t *queue);
static void restart_command_waiting_response_timer(command_waiting_response_t *cmd_wait_q);
static void command_timed_out(void *context);
#if CONFIG_HCI_LAYER_RX
static void hal_says_packet_ready(BT_HDR *packet);
#endif //CONFIG_HCI_LAYER_RX
static bool filter_incoming_event(BT_HDR *packet);
static serial_data_type_t event_to_data_type(uint16_t event);
static waiting_command_t *get_waiting_command(command_opcode_t opcode);
static void dispatch_reassembled(BT_HDR *packet);

// Module lifecycle functions
int hci_start_up(void)
{
    if (hci_layer_init_env()) {
        goto error;
    }

    hci_host_thread = osi_thread_create(HCI_HOST_TASK_NAME, HCI_HOST_TASK_STACK_SIZE, HCI_HOST_TASK_PRIO, HCI_HOST_TASK_PINNED_TO_CORE, 2);
    if (hci_host_thread == NULL) {
        goto error;
    }

#if CONFIG_HCI_LAYER_RX
    //memset(&hci_rx_thread, 0, sizeof(hci_rx_thread));
    //if (OS_ThreadCreate(&hci_rx_thread, HCI_H4_TASK_NAME, hal_says_packet_ready, NULL, HCI_H4_TASK_PRIO, HCI_H4_TASK_STACK_SIZE) != OS_OK) {
    //    goto error;
    //}
    hci_rx_thread = osi_thread_create(HCI_H4_TASK_NAME, HCI_H4_TASK_STACK_SIZE, HCI_H4_TASK_PRIO, HCI_HOST_TASK_PINNED_TO_CORE, 2);
    if (hci_rx_thread == NULL) {
        goto error;
    }
#endif //CONFIG_HCI_LAYER_RX

    packet_fragmenter->init(&packet_fragmenter_callbacks);

    //hal->open(&hal_callbacks, hci_host_thread);
    hal->open(&hal_callbacks, hci_rx_thread);

    // low_power_manager->init();

    hci_host_startup_flag = true;
    return 0;

error:
    hci_shut_down();
    return -1;
}

void hci_shut_down(void)
{
    hci_host_startup_flag  = false;
    hci_layer_deinit_env();

    packet_fragmenter->cleanup();

    // low_power_manager->cleanup();

    /*
       !!! this might cause to some problem, there's a better way same as bluedroid:
       hal_says_packet_ready() trigger by hal, and should tell hal finished after
       received a packet, hal trigger is realized by uart HAL_UART_EnableRxCallback
       and tell hal_says_packet_ready thread to receice but not call it in interrupt.

       delete rx task example bug:
       1. might cause to memory leak.
       2. might cause to uart driver non-stable.
    */
    hal->close();

    osi_thread_free(hci_host_thread);
    hci_host_thread = NULL;
#if CONFIG_HCI_LAYER_RX
    osi_thread_free(hci_rx_thread);
    hci_rx_thread = NULL;
#endif
}


bool hci_host_task_post(uint32_t timeout)// task_post_status_t hci_host_task_post(task_post_t timeout)
{
    return osi_thread_post(hci_host_thread, hci_host_thread_handler, NULL, 0, timeout);
}

static int hci_layer_init_env(void)
{
    command_waiting_response_t *cmd_wait_q;

    // The host is only allowed to send at most one command initially,
    // as per the Bluetooth spec, Volume 2, Part E, 4.4 (Command Flow Control)
    // This value can change when you get a command complete or command status event.
    hci_host_env.command_credits = 1;
    hci_host_env.command_queue = fixed_queue_new(QUEUE_SIZE_MAX);
    if (hci_host_env.command_queue) {
        fixed_queue_register_dequeue(hci_host_env.command_queue, event_command_ready);
    } else {
        HCI_TRACE_ERROR("%s unable to create pending command queue.", __func__);
        return -1;
    }

    hci_host_env.packet_queue = fixed_queue_new(QUEUE_SIZE_MAX);
    if (hci_host_env.packet_queue) {
        fixed_queue_register_dequeue(hci_host_env.packet_queue, event_packet_ready);
    } else {
        HCI_TRACE_ERROR("%s unable to create pending packet queue.", __func__);
        return -1;
    }

    // Init Commands waiting response list and timer
    cmd_wait_q = &hci_host_env.cmd_waiting_q;
    cmd_wait_q->timer_is_set = false;
    cmd_wait_q->commands_pending_response = list_new(NULL);
    if (!cmd_wait_q->commands_pending_response) {
        HCI_TRACE_ERROR("%s unable to create list for commands pending response.", __func__);
        return -1;
    }
    osi_mutex_new(&cmd_wait_q->commands_pending_response_lock);
    cmd_wait_q->command_response_timer = osi_alarm_new("cmd_rsp_to", command_timed_out, cmd_wait_q, COMMAND_PENDING_TIMEOUT);
    if (!cmd_wait_q->command_response_timer) {
        HCI_TRACE_ERROR("%s unable to create command response timer.", __func__);
        return -1;
    }
#if (BLE_50_FEATURE_SUPPORT == TRUE)
    btsnd_hcic_ble_sync_sem_init();
#endif // #if (BLE_50_FEATURE_SUPPORT == TRUE)

    return 0;
}

static void hci_layer_deinit_env(void)
{
    command_waiting_response_t *cmd_wait_q;

    if (hci_host_env.command_queue) {
        fixed_queue_free(hci_host_env.command_queue, osi_free_func);
    }
    if (hci_host_env.packet_queue) {
        fixed_queue_free(hci_host_env.packet_queue, osi_free_func);
    }

    cmd_wait_q = &hci_host_env.cmd_waiting_q;
    list_free(cmd_wait_q->commands_pending_response);
    osi_mutex_free(&cmd_wait_q->commands_pending_response_lock);
    osi_alarm_free(cmd_wait_q->command_response_timer);
    cmd_wait_q->command_response_timer = NULL;
#if (BLE_50_FEATURE_SUPPORT == TRUE)
    btsnd_hcic_ble_sync_sem_deinit();
#endif // #if (BLE_50_FEATURE_SUPPORT == TRUE)
}

static void hci_host_thread_handler(void *arg)
{
    /*
     * Previous task handles RX queue and two TX Queues, Since there is
     * a RX Thread Task in H4 layer which receives packet from driver layer.
     * Now HCI Host Task has been optimized to only process TX Queue
     * including command and data queue. And command queue has high priority,
     * All packets will be directly copied to single queue in driver layer with
     * H4 type header added (1 byte).
     */

    /*Now Target only allowed one packet per TX*/
//      printf("QUEUE\n");
    BT_HDR *pkt = packet_fragmenter->fragment_current_packet();
    if (pkt != NULL) {
        // low_power_manager->wake_assert();
        packet_fragmenter->fragment_and_dispatch(pkt);
        // low_power_manager->transmit_done();
    } else {
        if (!fixed_queue_is_empty(hci_host_env.command_queue) &&
                hci_host_env.command_credits > 0) {
            fixed_queue_process(hci_host_env.command_queue);
        } else if (!fixed_queue_is_empty(hci_host_env.packet_queue)) {
            fixed_queue_process(hci_host_env.packet_queue);
        } else {
        }
    }
}

static void transmit_command(
    BT_HDR *command,
    command_complete_cb complete_callback,
    command_status_cb status_callback,
    void *context)
{
    uint8_t *stream;
    waiting_command_t *wait_entry = osi_calloc(sizeof(waiting_command_t));
    if (!wait_entry) {
        HCI_TRACE_ERROR("%s couldn't allocate space for wait entry.", __func__);
        return;
    }

    stream = command->data + command->offset;
    STREAM_TO_UINT16(wait_entry->opcode, stream);
    wait_entry->complete_callback = complete_callback;
    wait_entry->status_callback = status_callback;
    wait_entry->command = command;
    wait_entry->context = context;

    // Store the command message type in the event field
    // in case the upper layer didn't already
    command->event = MSG_STACK_TO_HC_HCI_CMD;
    HCI_TRACE_DEBUG("HCI Enqueue Comamnd opcode=0x%x\n", wait_entry->opcode);

    fixed_queue_enqueue(hci_host_env.command_queue, wait_entry, FIXED_QUEUE_MAX_TIMEOUT);
    hci_host_task_post(TASK_POST_BLOCKING);

}

static future_t *transmit_command_futured(BT_HDR *command)
{
    waiting_command_t *wait_entry = osi_calloc(sizeof(waiting_command_t));
    assert(wait_entry != NULL);

    future_t *future = future_new();

    uint8_t *stream = command->data + command->offset;
    STREAM_TO_UINT16(wait_entry->opcode, stream);
    wait_entry->complete_future = future;
    wait_entry->command = command;

    // Store the command message type in the event field
    // in case the upper layer didn't already
    command->event = MSG_STACK_TO_HC_HCI_CMD;

    fixed_queue_enqueue(hci_host_env.command_queue, wait_entry, FIXED_QUEUE_MAX_TIMEOUT);
    hci_host_task_post(OSI_THREAD_MAX_TIMEOUT);
    return future;
}

static void transmit_downward(uint16_t type, void *data)
{
    if (type == MSG_STACK_TO_HC_HCI_CMD) {
        transmit_command((BT_HDR *)data, NULL, NULL, NULL);
        HCI_TRACE_WARNING("%s legacy transmit of command. Use transmit_command instead.\n", __func__);
    } else {
        fixed_queue_enqueue(hci_host_env.packet_queue, data, FIXED_QUEUE_MAX_TIMEOUT);
    }

    hci_host_task_post(OSI_THREAD_MAX_TIMEOUT);
}


// Command/packet transmitting functions
static void event_command_ready(fixed_queue_t *queue)
{
    waiting_command_t *wait_entry = NULL;
    command_waiting_response_t *cmd_wait_q = &hci_host_env.cmd_waiting_q;

    wait_entry = fixed_queue_dequeue(queue, FIXED_QUEUE_MAX_TIMEOUT);

    if(wait_entry->opcode == HCI_HOST_NUM_PACKETS_DONE){
        // low_power_manager->wake_assert();
        packet_fragmenter->fragment_and_dispatch(wait_entry->command);
        // low_power_manager->transmit_done();
        osi_free(wait_entry->command);
        osi_free(wait_entry);
        return;
    }
    hci_host_env.command_credits--;
    // Move it to the list of commands awaiting response
    osi_mutex_lock(&cmd_wait_q->commands_pending_response_lock, OSI_MUTEX_MAX_TIMEOUT);
    list_append(cmd_wait_q->commands_pending_response, wait_entry);
    osi_mutex_unlock(&cmd_wait_q->commands_pending_response_lock);

    // Send it off
    // low_power_manager->wake_assert();
    packet_fragmenter->fragment_and_dispatch(wait_entry->command);
    // low_power_manager->transmit_done();

    restart_command_waiting_response_timer(cmd_wait_q);
}

static void event_packet_ready(fixed_queue_t *queue)
{
    BT_HDR *packet = (BT_HDR *)fixed_queue_dequeue(queue, FIXED_QUEUE_MAX_TIMEOUT);
    // The queue may be the command queue or the packet queue, we don't care

    // low_power_manager->wake_assert();
    packet_fragmenter->fragment_and_dispatch(packet);
    // low_power_manager->transmit_done();
}

// Callback for the fragmenter to send a fragment
static void transmit_fragment(BT_HDR *packet, bool send_transmit_finished)
{
    uint16_t event = packet->event & MSG_EVT_MASK;
    serial_data_type_t type = event_to_data_type(event);

    BTTRC_DUMP_BUFFER(false, packet->data + packet->offset, packet->len, type);//todo

#if 0
    btsnoop_capture(type, packet, false);
#endif

    hal->transmit_data(type, packet->data + packet->offset, packet->len);

    if (event != MSG_STACK_TO_HC_HCI_CMD && send_transmit_finished) {
        osi_free(packet);
    }
}

static void fragmenter_transmit_finished(BT_HDR *packet, bool all_fragments_sent)
{
    if (all_fragments_sent) {
        osi_free(packet);
    } else {
        // This is kind of a weird case, since we're dispatching a partially sent packet
        // up to a higher layer.
        // TODO(zachoverflow): rework upper layer so this isn't necessary.
        //osi_free(packet);

        /* dispatch_reassembled(packet) will send the packet back to the higher layer
           when controller buffer is not enough. hci will send the remain packet back
           to the l2cap layer and saved in the Link Queue (p_lcb->link_xmit_data_q).
           The l2cap layer will resend the packet to lower layer when controller buffer
           can be used.
        */

        dispatch_reassembled(packet);
        //data_dispatcher_dispatch(interface.event_dispatcher, packet->event & MSG_EVT_MASK, packet);
    }
}

static void restart_command_waiting_response_timer(command_waiting_response_t *cmd_wait_q)
{
    osi_mutex_lock(&cmd_wait_q->commands_pending_response_lock, OSI_MUTEX_MAX_TIMEOUT);
    if (cmd_wait_q->timer_is_set) {
        osi_alarm_cancel(cmd_wait_q->command_response_timer);
        cmd_wait_q->timer_is_set = false;
    }
    if (!list_is_empty(cmd_wait_q->commands_pending_response)) {
        osi_alarm_set(cmd_wait_q->command_response_timer, COMMAND_PENDING_TIMEOUT);
        cmd_wait_q->timer_is_set = true;
    }
    osi_mutex_unlock(&cmd_wait_q->commands_pending_response_lock);
}

static void command_timed_out(void *context)
{
    command_waiting_response_t *cmd_wait_q = (command_waiting_response_t *)context;
    waiting_command_t *wait_entry;

    osi_mutex_lock(&cmd_wait_q->commands_pending_response_lock, OSI_MUTEX_MAX_TIMEOUT);
    wait_entry = (list_is_empty(cmd_wait_q->commands_pending_response) ?
                  NULL : list_front(cmd_wait_q->commands_pending_response));
    osi_mutex_unlock(&cmd_wait_q->commands_pending_response_lock);

    if (wait_entry == NULL) {
        HCI_TRACE_ERROR("%s with no commands pending response", __func__);
    } else
        // We shouldn't try to recover the stack from this command timeout.
        // If it's caused by a software bug, fix it. If it's a hardware bug, fix it.
    {
        HCI_TRACE_ERROR("%s hci layer timeout waiting for response to a command. opcode: 0x%x", __func__, wait_entry->opcode);
    }
}

#if CONFIG_HCI_LAYER_RX
#if 0
#if (C2H_FLOW_CONTROL_INCLUDED == TRUE)
void hci_packet_complete(BT_HDR *packet) {
    uint8_t type, num_handle;
    uint16_t handle;
    uint16_t handles[MAX_L2CAP_LINKS + 4];
    uint16_t num_packets[MAX_L2CAP_LINKS + 4];
    uint8_t *stream = packet->data + packet->offset;
    tL2C_LCB  *p_lcb = NULL;

    STREAM_TO_UINT8(type, stream);
    if (type == DATA_TYPE_ACL/* || type == DATA_TYPE_SCO*/) {
        STREAM_TO_UINT16(handle, stream);
        handle = handle & HCI_DATA_HANDLE_MASK;
        p_lcb = l2cu_find_lcb_by_handle(handle);
        if (p_lcb) {
            p_lcb->completed_packets++;
        }
        if (1/* xr_vhci_host_check_send_available() */){
            num_handle = l2cu_find_completed_packets(handles, num_packets);
            if (num_handle > 0){
                btsnd_hcic_host_num_xmitted_pkts (num_handle, handles, num_packets);
            }
        } else {
            //Send HCI_Host_Number_of_Completed_Packets next time.
        }

    }
}
#endif ///C2H_FLOW_CONTROL_INCLUDED == TRUE

// Event/packet receiving functions
// this function is same as bluedroid hal_says_packet_ready(), but this is not triggered by hal.
static void hal_says_packet_ready(void *arg)
{
    packet_receive_data_t incoming_packets;
    packet_receive_data_t *incoming = &incoming_packets;
#ifdef CONFIG_BT_DUAL_HOST
    static uint8_t pkt[2048];
    static uint32_t i;
#endif

    int32_t ret = 0;
    incoming->state = HCI_BRAND_NEW;

    while (1)
    {
        switch (incoming->state)
        {
        case HCI_BRAND_NEW:
            memset(incoming, 0, sizeof(*incoming));
#ifdef CONFIG_BT_DUAL_HOST
            memset(pkt, 0, 2048);
            i = 0;
#endif

        case HCI_PACKET_TYPE:
            ret = hal->read_data(incoming->preamble, HCI_PACKET_TYPE_SIZE);
            if (ret != HCI_PACKET_TYPE_SIZE) {
                HCI_TRACE_DEBUG("h4 read data timeout(%d)", ret);
                incoming->state = HCI_BRAND_NEW;
                break;
            }
            if ((incoming->preamble[0] > HCI_PACKET_TYPE_EVENT) | (incoming->preamble[0] == HCI_PACKET_TYPE_UNKNOWN)) {
                HCI_TRACE_ERROR("invalid packet type0x%02x", incoming->preamble[0]);
                incoming->state = HCI_BRAND_NEW;
                break;
            }
#ifdef CONFIG_BT_DUAL_HOST
            memcpy(pkt + i, incoming->preamble, HCI_PACKET_TYPE_SIZE);
            i += HCI_PACKET_TYPE_SIZE;
#endif

            incoming->index += HCI_PACKET_TYPE_SIZE;
            incoming->bytes_remaining = preamble_sizes_for_type[incoming->preamble[0]];
            incoming->state = HCI_PREAMBLE;

            HCI_TRACE_DEBUG("rx hci type(%d)\n", incoming->preamble[0]);
            break;

        case HCI_PREAMBLE:
            ret = hal->read_data(incoming->preamble + incoming->index, incoming->bytes_remaining);
            if (ret != incoming->bytes_remaining) {
                HCI_TRACE_ERROR("h4 read data failed(%d), should read(%d)", ret, incoming->bytes_remaining);
                incoming->state = HCI_BRAND_NEW;
                break;
            }
#ifdef CONFIG_BT_DUAL_HOST
            memcpy(pkt + i, incoming->preamble + incoming->index, incoming->bytes_remaining);
            i += incoming->bytes_remaining;
#endif

            incoming->index += incoming->bytes_remaining;

            incoming->bytes_remaining = HciGetPacketLengthForType((enum HciPacketType)incoming->preamble[0], &incoming->preamble[HCI_PACKET_TYPE_SIZE]);

            incoming->buffer = (BT_HDR *)buffer_allocator->alloc(BT_HDR_SIZE + incoming->index + incoming->bytes_remaining);
            if (incoming->buffer == NULL) {
                HCI_TRACE_ERROR("no memory to receive!");
                incoming->state = HCI_BRAND_NEW;
                break;
            }
            //bluedroid receive packet without indication byte.
            incoming->buffer->offset = HCI_PACKET_TYPE_SIZE;
            incoming->buffer->len = incoming->index + incoming->bytes_remaining - HCI_PACKET_TYPE_SIZE;
            incoming->buffer->layer_specific = 0;
            incoming->buffer->event = outbound_event_types[PACKET_TYPE_TO_INDEX(incoming->preamble[0])];
            memcpy(incoming->buffer->data, incoming->preamble, incoming->index);

            incoming->state = HCI_PAYLOAD;
            break;

        case HCI_PAYLOAD:
            ret = hal->read_data(incoming->buffer->data + incoming->index, incoming->bytes_remaining);
            if (ret != incoming->bytes_remaining) {
                HCI_TRACE_ERROR("h4 read data failed(%d), should read(%d)", ret, incoming->bytes_remaining);
                buffer_allocator->free(incoming->buffer->data);
                incoming->state = HCI_BRAND_NEW;
                break;
            }
#ifdef CONFIG_BT_DUAL_HOST
            memcpy(pkt + i, incoming->buffer->data + incoming->index, incoming->bytes_remaining);
            i += incoming->bytes_remaining;
#endif

            BTTRC_DUMP_BUFFER(true, incoming->buffer->data + incoming->buffer->offset, incoming->buffer->len, incoming->preamble[0]);//todo
#if CONFIG_HCILOG_ENABLE
            {
                uint8_t is_vendor_specific_event = 0;

                if (incoming->buffer->event == MSG_HC_TO_STACK_HCI_EVT) {
                    UINT8 *p = (UINT8 *)(incoming->buffer + 1) + incoming->buffer->offset;
                    UINT8 hci_evt_code = *(p);

                    if (hci_evt_code == HCI_VENDOR_SPECIFIC_EVT) {
                        is_vendor_specific_event = 1;
                    }

                }

                if (!is_vendor_specific_event) {
#ifdef CONFIG_BT_DRIVERS_LIB
#else
                    btsnoop_capture(incoming->buffer, true);
#endif
                }
            }
#endif

#if (C2H_FLOW_CONTROL_INCLUDED == TRUE)
            /* TODO: for what? why doing this? */
            incoming->buffer->offset--;
            incoming->buffer->len++;
            hci_packet_complete(incoming->buffer);
            incoming->buffer->offset++;
            incoming->buffer->len--;
#endif

            if (incoming->buffer->event != MSG_HC_TO_STACK_HCI_EVT) {
                packet_fragmenter->reassemble_and_dispatch(incoming->buffer);
            } else if (!filter_incoming_event(incoming->buffer)) {
                dispatch_reassembled(incoming->buffer);
            }
#ifdef CONFIG_BT_DUAL_HOST
            int blec_hci_c2h(unsigned char hci_type, const unsigned char * buff, unsigned int offset, unsigned int len);
            blec_hci_c2h(pkt[0], pkt + 1, 0, i - 1);
#endif

            incoming->state = HCI_BRAND_NEW;
            break;

        default:
            incoming->state = HCI_BRAND_NEW;
            break;
        }

    }
}
#else //CONFIG_HCI_LAYER_RX

#ifdef CONFIG_BT_DUAL_HOST
static unsigned char event_type_to_packet_type(uint16_t event)
{
    switch (event)
    {
    case MSG_HC_TO_STACK_HCI_ACL:
        return DATA_TYPE_ACL;
    case MSG_HC_TO_STACK_HCI_SCO:
        return DATA_TYPE_SCO;
    case MSG_HC_TO_STACK_HCI_EVT:
        return DATA_TYPE_EVENT;
    case MSG_HC_TO_STACK_HCI_ERR:
    default:
        return 0;
    }
}
#endif

// Event/packet receiving functions
static void hal_says_packet_ready(BT_HDR *packet)
{
#if 0
#ifdef CONFIG_BT_VHCI
    void btsnoop_capture(uint8_t type, const BT_HDR * buffer, bool is_received);
    btsnoop_capture(event_type_to_packet_type(packet->event), packet, 1);
#endif
#endif

    if (packet->event != MSG_HC_TO_STACK_HCI_EVT) {
        packet_fragmenter->reassemble_and_dispatch(packet);
    } else if (!filter_incoming_event(packet)) {
        dispatch_reassembled(packet);
    }
}
#endif
#endif //CONFIG_HCI_LAYER_RX

// Returns true if the event was intercepted and should not proceed to
// higher layers. Also inspects an incoming event for interesting
// information, like how many commands are now able to be sent.
static bool filter_incoming_event(BT_HDR *packet)
{
    waiting_command_t *wait_entry = NULL;
    uint8_t *stream = packet->data + packet->offset;
    uint8_t event_code;
    command_opcode_t opcode;

    STREAM_TO_UINT8(event_code, stream);
    STREAM_SKIP_UINT8(stream); // Skip the parameter total length field

    HCI_TRACE_DEBUG("Receive packet event_code=0x%x\n", event_code);

    if (event_code == HCI_COMMAND_COMPLETE_EVT) {
        STREAM_TO_UINT8(hci_host_env.command_credits, stream);
        STREAM_TO_UINT16(opcode, stream);
        wait_entry = get_waiting_command(opcode);
        if (!wait_entry) {
#ifdef CONFIG_BT_DUAL_HOST
            HCI_TRACE_DEBUG("%s command complete event with no matching command. opcode: 0x%x.", __func__, opcode);
#else
            HCI_TRACE_WARNING("%s command complete event with no matching command. opcode: 0x%x.", __func__, opcode);
#endif
        } else if (wait_entry->complete_callback) {
            wait_entry->complete_callback(packet, wait_entry->context);
#if (BLE_50_FEATURE_SUPPORT == TRUE)
            BlE_SYNC *sync_info =  btsnd_hcic_ble_get_sync_info();
            if(!sync_info) {
                HCI_TRACE_WARNING("%s sync_info is NULL. opcode = 0x%x", __func__, opcode);
            } else {
                if (sync_info->sync_sem && sync_info->opcode == opcode) {
                    osi_sem_give(&sync_info->sync_sem);
                    sync_info->opcode = 0;
                }
            }
#endif // #if (BLE_50_FEATURE_SUPPORT == TRUE)
        } else if (wait_entry->complete_future) {
            future_ready(wait_entry->complete_future, packet);
        }

        goto intercepted;
    } else if (event_code == HCI_COMMAND_STATUS_EVT) {
        uint8_t status;
        STREAM_TO_UINT8(status, stream);
        STREAM_TO_UINT8(hci_host_env.command_credits, stream);
        STREAM_TO_UINT16(opcode, stream);

        // If a command generates a command status event, it won't be getting a command complete event

        wait_entry = get_waiting_command(opcode);
        if (!wait_entry) {
#ifdef CONFIG_BT_DUAL_HOST
            HCI_TRACE_DEBUG("%s command complete event with no matching command. opcode: 0x%x.", __func__, opcode);
#else
            HCI_TRACE_WARNING("%s command status event with no matching command. opcode: 0x%x", __func__, opcode);
#endif
        } else if (wait_entry->status_callback) {
            wait_entry->status_callback(status, wait_entry->command, wait_entry->context);
        }

        goto intercepted;
    }

    return false;
intercepted:
    restart_command_waiting_response_timer(&hci_host_env.cmd_waiting_q);

    /*Tell HCI Host Task to continue TX Pending commands*/
    if (hci_host_env.command_credits &&
            !fixed_queue_is_empty(hci_host_env.command_queue)) {
        hci_host_task_post(OSI_THREAD_MAX_TIMEOUT);
    }

    if (wait_entry) {
        // If it has a callback, it's responsible for freeing the packet
        if (event_code == HCI_COMMAND_STATUS_EVT ||
                (!wait_entry->complete_callback && !wait_entry->complete_future)) {
            osi_free(packet);
        }

        // If it has a callback, it's responsible for freeing the command
        if (event_code == HCI_COMMAND_COMPLETE_EVT || !wait_entry->status_callback) {
            osi_free(wait_entry->command);
        }

        osi_free(wait_entry);
    } else {
        osi_free(packet);
    }

    return true;
}

// Callback for the fragmenter to dispatch up a completely reassembled packet
static void dispatch_reassembled(BT_HDR *packet)
{
    // Events should already have been dispatched before this point
    //Tell Up-layer received packet.
    if (btu_task_post(SIG_BTU_HCI_MSG, packet, OSI_THREAD_MAX_TIMEOUT) == false) {
        osi_free(packet);
    }
}

// Misc internal functions

// TODO(zachoverflow): we seem to do this a couple places, like the HCI inject module. #centralize
static serial_data_type_t event_to_data_type(uint16_t event)
{
    if (event == MSG_STACK_TO_HC_HCI_ACL) {
        return DATA_TYPE_ACL;
    } else if (event == MSG_STACK_TO_HC_HCI_SCO) {
        return DATA_TYPE_SCO;
    } else if (event == MSG_STACK_TO_HC_HCI_CMD) {
        return DATA_TYPE_COMMAND;
    } else {
        HCI_TRACE_ERROR("%s invalid event type, could not translate 0x%x\n", __func__, event);
    }

    return 0;
}

static waiting_command_t *get_waiting_command(command_opcode_t opcode)
{
    command_waiting_response_t *cmd_wait_q = &hci_host_env.cmd_waiting_q;
    osi_mutex_lock(&cmd_wait_q->commands_pending_response_lock, OSI_MUTEX_MAX_TIMEOUT);

    for (const list_node_t *node = list_begin(cmd_wait_q->commands_pending_response);
            node != list_end(cmd_wait_q->commands_pending_response);
            node = list_next(node)) {
        waiting_command_t *wait_entry = list_node(node);
        if (!wait_entry || wait_entry->opcode != opcode) {
            continue;
        }

        list_remove(cmd_wait_q->commands_pending_response, wait_entry);

        osi_mutex_unlock(&cmd_wait_q->commands_pending_response_lock);
        return wait_entry;
    }

    osi_mutex_unlock(&cmd_wait_q->commands_pending_response_lock);
    return NULL;
}

static void init_layer_interface()
{
    if (!interface_created) {
        // // interface.send_low_power_command = low_power_manager->post_command;
        interface.transmit_command = transmit_command;
        interface.transmit_command_futured = transmit_command_futured;
        interface.transmit_downward = transmit_downward;
        interface_created = true;
    }
}

static const hci_hal_callbacks_t hal_callbacks = {
    hal_says_packet_ready
};

static const packet_fragmenter_callbacks_t packet_fragmenter_callbacks = {
    transmit_fragment,
    dispatch_reassembled,
    fragmenter_transmit_finished
};

const hci_t *hci_layer_get_interface()
{
    hal = hci_hal_h4_get_interface();
    packet_fragmenter = packet_fragmenter_get_interface();
    // // low_power_manager = low_power_manager_get_interface();

    init_layer_interface();
    return &interface;
}

