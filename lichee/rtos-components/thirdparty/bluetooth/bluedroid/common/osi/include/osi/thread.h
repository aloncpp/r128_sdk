// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __THREAD_H__
#define __THREAD_H__

#include "kernel/os/os_queue.h"
#include "kernel/os/os_thread.h"
#include "common/bt_defs.h"
#include "xr_task.h"

//#define portBASE_TYPE int

struct bt_task_evt {
    uint32_t    sig;    //task sig
    void       *par;    //point to task param
    void       *cb;     //point to function cb
    void       *arg;    //point to function arg
};
typedef struct bt_task_evt BtTaskEvt_t;

typedef bt_status_t (* BtTaskCb_t)(void *arg);

typedef enum {
    SIG_HCI_HAL_RECV_PACKET = 0,
    SIG_HCI_HAL_NUM,
} SIG_HCI_HAL_t;


typedef enum {
    SIG_HCI_HOST_SEND_AVAILABLE = 0,
    SIG_HCI_HOST_NUM,
} SIG_HCI_HOST_t;

// typedef enum {
//     SIG_BTU_START_UP = 0,
//     SIG_BTU_HCI_MSG,
//     SIG_BTU_BTA_MSG,
//     SIG_BTU_BTA_ALARM,
//     SIG_BTU_GENERAL_ALARM,
//     SIG_BTU_ONESHOT_ALARM,
//     SIG_BTU_L2CAP_ALARM,
//     SIG_BTU_NUM,
// } SIG_BTU_t;

//#define TASK_PINNED_TO_CORE             (CONFIG_BLUEDROID_PINNED_TO_CORE < portNUM_PROCESSORS ? CONFIG_BLUEDROID_PINNED_TO_CORE : tskNO_AFFINITY)

//#define HCI_HOST_TASK_PINNED_TO_CORE    (TASK_PINNED_TO_CORE)
// #define HCI_HOST_TASK_STACK_SIZE        (4096 + BT_TASK_EXTRA_STACK_SIZE)
#define HCI_HOST_TASK_PRIO              (XR_OS_PRIORITY_HIGH)
// #define HCI_HOST_TASK_NAME              "hciHostT"
#define HCI_HOST_QUEUE_LEN              40 /* tx: 40 * 16 bytes = 0.6K + all packet size */

//#define HCI_H4_TASK_PINNED_TO_CORE      (TASK_PINNED_TO_CORE)
#define HCI_H4_TASK_STACK_SIZE          (4096 + BT_TASK_EXTRA_STACK_SIZE)
#define HCI_H4_TASK_PRIO                (XR_OS_PRIORITY_HIGH)
#define HCI_H4_TASK_NAME                "hciH4T"
#define HCI_H4_QUEUE_LEN                1

//#define BTU_TASK_PINNED_TO_CORE         (TASK_PINNED_TO_CORE)
//#define BTU_TASK_STACK_SIZE             (4096 + BT_TASK_EXTRA_STACK_SIZE)
#define BTU_TASK_PRIO                   (XR_OS_PRIORITY_ABOVE_NORMAL)
#define BTU_TASK_NAME                   "btuT"
#define BTU_QUEUE_LEN                   50 /* rx: 50 * 16 bytes = 0.8K + all packet size */

//#define BTC_TASK_PINNED_TO_CORE         (TASK_PINNED_TO_CORE)
#define BTC_TASK_STACK_SIZE             (CONFIG_BT_BTC_TASK_STACK_SIZE + BT_TASK_EXTRA_STACK_SIZE * 2)	//by menuconfig
#define BTC_TASK_NAME                   "btiT"
#define BTC_TASK_PRIO                   (XR_OS_PRIORITY_NORMAL)
#define BTC_TASK_QUEUE_LEN              90  /* 60 * 8 bytes = 0.5K */

//#define BTC_A2DP_SINK_TASK_PINNED_TO_CORE     (TASK_PINNED_TO_CORE)
#define BTC_A2DP_SINK_TASK_STACK_SIZE         (CONFIG_A2DP_SINK_TASK_STACK_SIZE + BT_TASK_EXTRA_STACK_SIZE) // by menuconfig
#define BTC_A2DP_SINK_TASK_NAME               "BtA2dSinkT"
#define BTC_A2DP_SINK_TASK_PRIO               (XR_OS_PRIORITY_ABOVE_NORMAL)
#define BTC_A2DP_SINK_DATA_QUEUE_LEN          (3)
#define BTC_A2DP_SINK_CTRL_QUEUE_LEN          (5)
#define BTC_A2DP_SINK_TASK_QUEUE_SET_LEN      (BTC_A2DP_SINK_DATA_QUEUE_LEN + BTC_A2DP_SINK_CTRL_QUEUE_LEN)

//#define BTC_A2DP_SOURCE_TASK_PINNED_TO_CORE   (TASK_PINNED_TO_CORE)
#define BTC_A2DP_SOURCE_TASK_STACK_SIZE       (CONFIG_A2DP_SOURCE_TASK_STACK_SIZE + BT_TASK_EXTRA_STACK_SIZE) // by menuconfig
#define BTC_A2DP_SOURCE_TASK_NAME             "BtA2dSourceT"
#define BTC_A2DP_SOURCE_TASK_PRIO             (XR_OS_PRIORITY_ABOVE_NORMAL)
#define BTC_A2DP_SOURCE_DATA_QUEUE_LEN        (3)
#define BTC_A2DP_SOURCE_CTRL_QUEUE_LEN        (5)
#define BTC_A2DP_SOURCE_TASK_QUEUE_SET_LEN    (BTC_A2DP_SOURCE_DATA_QUEUE_LEN + BTC_A2DP_SOURCE_CTRL_QUEUE_LEN)

#define TASK_POST_NON_BLOCKING          (0)
#define TASK_POST_BLOCKING              (uint32_t)(OS_WAIT_FOREVER)
typedef uint32_t task_post_t;           /* Timeout of task post return, unit TICK */

typedef enum {
    TASK_POST_SUCCESS = 0,
    TASK_POST_FAIL,
} task_post_status_t;


bool btu_task_post(uint32_t sig, void *param, task_post_t timeout);
bool hci_host_task_post(task_post_t timeout);
task_post_status_t hci_hal_h4_task_post(task_post_t timeout);





#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "osi/semaphore.h"
#include "xr_task.h"
#include "bt_common.h"


#define OSI_THREAD_MAX_TIMEOUT (uint32_t)OSI_SEM_MAX_TIMEOUT

struct osi_thread;

typedef struct osi_thread osi_thread_t;

typedef void (*osi_thread_func_t)(void *context);

typedef enum {
    OSI_THREAD_CORE_0 = 0,
    OSI_THREAD_CORE_1,
    OSI_THREAD_CORE_AFFINITY,
} osi_thread_core_t;

/*
 * brief: Create a thread or task
 * param name: thread name
 * param stack_size: thread stack size
 * param priority: thread priority
 * param core: the CPU core which this thread run, OSI_THREAD_CORE_AFFINITY means unspecific CPU core
 * param work_queue_num: speicify queue number, the queue[0] has highest priority, and the priority is decrease by index
 * return : if create successfully, return thread handler; otherwise return NULL.
 */
osi_thread_t *osi_thread_create(const char *name, size_t stack_size, int priority, osi_thread_core_t core, uint8_t work_queue_num);

/*
 * brief: Destroy a thread or task
 * param thread: point of thread handler
 */
void osi_thread_free(osi_thread_t *thread);

/*
 * brief: Post an msg to a thread and told the thread call the function
 * param thread: point of thread handler
 * param func: callback function that called by target thread
 * param context: argument of callback function
 * param queue_idx: the queue which the msg send to
 * param timeout: post timeout, OSI_THREAD_MAX_TIMEOUT means blocking forever, 0 means never blocking, others means block millisecond
 * return : if post successfully, return true, otherwise return false
 */
bool osi_thread_post(osi_thread_t *thread, osi_thread_func_t func, void *context, int queue_idx, uint32_t timeout);

/*
 * brief: Set the priority of thread
 * param thread: point of thread handler
 * param priority: priority
 * return : if set successfully, return true, otherwise return false
 */
bool osi_thread_set_priority(osi_thread_t *thread, int priority);

/* brief: Get thread name
 * param thread: point of thread handler
 * return: constant point of thread name
 */
const char *osi_thread_name(osi_thread_t *thread);

/* brief: Get the size of the specified queue
 * param thread: point of thread handler
 * param wq_idx: the queue index of the thread
 * return: queue size
 */
int osi_thread_queue_wait_size(osi_thread_t *thread, int wq_idx);


#endif /* __THREAD_H__ */
