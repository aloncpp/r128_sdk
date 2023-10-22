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

#include "osi/allocator.h"
#include "osi/fixed_queue.h"
#include "osi/semaphore.h"
#include "osi/thread.h"

#ifdef CONFIG_COMPONENTS_BT_PM
#include "bt_pm.h"
#endif

struct osi_thread {
  XR_OS_Thread_t thread_handle;        /*!< Store the thread object */
  int  thread_id;                       /*!< May for some OS, such as Linux */
  bool stop;
  uint8_t work_queue_num;               /*!< Work queue number */
  fixed_queue_t **work_queues;          /*!< Point to queue array, and the priority inverse array index */
  osi_sem_t work_sem;
  osi_sem_t stop_sem;
};

struct osi_thread_start_arg {
  osi_thread_t *thread;
  osi_sem_t start_sem;
  int error;
};

typedef struct {
  osi_thread_func_t func;
  void *context;
} work_item_t;

static const size_t DEFAULT_WORK_QUEUE_CAPACITY = 100;

static void osi_thread_run(void *arg)
{
    struct osi_thread_start_arg *start = (struct osi_thread_start_arg *)arg;
    osi_thread_t *thread = start->thread;

    osi_sem_give(&start->start_sem);

    while (1) {
        int idx = 0;
        osi_sem_take(&thread->work_sem, OSI_SEM_MAX_TIMEOUT);
        if (thread->stop) {
            break;
        }
        while (!thread->stop && idx < thread->work_queue_num) {
            work_item_t *item = fixed_queue_dequeue(thread->work_queues[idx], 0);
            if (item) {
#ifdef CONFIG_COMPONENTS_BT_PM
                bt_pm_lock();
#endif
                item->func(item->context);
                osi_free(item);
                idx = 0;
#ifdef CONFIG_COMPONENTS_BT_PM
                bt_pm_unlock();
#endif
                continue;
            } else {
                idx++;
            }
        }
    }

    memset(&thread->thread_handle, 0, sizeof(thread->thread_handle));
    osi_sem_give(&thread->stop_sem);

    XR_OS_ThreadDelete(NULL);
}

static int osi_thread_join(osi_thread_t *thread, uint32_t wait_ms)
{
    assert(thread != NULL);
    return osi_sem_take(&thread->stop_sem, wait_ms);
}

static void osi_thread_stop(osi_thread_t *thread)
{
    int ret;

    assert(thread != NULL);

    //stop the thread
    thread->stop = true;
    osi_sem_give(&thread->work_sem);

    //join
    ret = osi_thread_join(thread, 1000); //wait 1000ms

    //if join failed, delete the task here
    if (ret != 0 && XR_OS_ThreadIsValid(&thread->thread_handle)) {
        XR_OS_ThreadDelete(&thread->thread_handle);
    }
}

//in linux, the stack_size, priority and core may not be set here, the code will be ignore the arguments
osi_thread_t *osi_thread_create(const char *name, size_t stack_size, int priority, osi_thread_core_t core, uint8_t work_queue_num)
{
    int ret;
    osi_thread_t *thread;
    struct osi_thread_start_arg start_arg = {0};

    if (stack_size <= 0 ||
            core < OSI_THREAD_CORE_0 || core > OSI_THREAD_CORE_AFFINITY ||
            work_queue_num <= 0) {
        return NULL;
    }

    thread = (osi_thread_t *)osi_malloc(sizeof(osi_thread_t));
    if (thread == NULL) {
        goto _err;
    }

    thread->stop = false;
    thread->work_queue_num = work_queue_num;
    thread->work_queues = (fixed_queue_t **)osi_malloc(sizeof(fixed_queue_t *) * work_queue_num);
    if (thread->work_queues == NULL) {
        goto _err;
    }

    for (int i = 0; i < thread->work_queue_num; i++) {
        thread->work_queues[i] = fixed_queue_new(DEFAULT_WORK_QUEUE_CAPACITY);
        if (thread->work_queues[i] == NULL) {
            goto _err;
        }
    }

    ret = osi_sem_new(&thread->work_sem, 1, 0);
    if (ret != 0) {
        goto _err;
    }

    ret = osi_sem_new(&thread->stop_sem, 1, 0);
    if (ret != 0) {
        goto _err;
    }

    start_arg.thread = thread;
    ret = osi_sem_new(&start_arg.start_sem, 1, 0);
    if (ret != 0) {
        goto _err;
    }

    memset(&thread->thread_handle, 0, sizeof(thread->thread_handle));
    if (XR_OS_ThreadCreate(&thread->thread_handle, name, osi_thread_run, &start_arg, priority, stack_size)) {
        goto _err;
    }

    osi_sem_take(&start_arg.start_sem, OSI_SEM_MAX_TIMEOUT);
    osi_sem_free(&start_arg.start_sem);

    return thread;

_err:

    if (thread) {
        // if (start_arg.start_sem) {
        //     osi_sem_free(&start_arg.start_sem);
        // }

        if (XR_OS_ThreadIsValid(&thread->thread_handle)) {
            XR_OS_ThreadDelete(&thread->thread_handle);
        }

        for (int i = 0; i < thread->work_queue_num; i++) {
            if (thread->work_queues[i]) {
                fixed_queue_free(thread->work_queues[i], osi_free_func);
            }
        }

        if (thread->work_queues) {
            osi_free(thread->work_queues);
        }

        if (osi_sem_is_valid(&thread->work_sem)) {
            osi_sem_free(&thread->work_sem);
        }

        if (osi_sem_is_valid(&thread->stop_sem)) {
            osi_sem_free(&thread->stop_sem);
        }

        osi_free(thread);
    }

    return NULL;
}

void osi_thread_free(osi_thread_t *thread)
{
    if (!thread)
        return;

    osi_thread_stop(thread);

    for (int i = 0; i < thread->work_queue_num; i++) {
        if (thread->work_queues[i]) {
            fixed_queue_free(thread->work_queues[i], osi_free_func);
        }
    }

    if (thread->work_queues) {
        osi_free(thread->work_queues);
    }

    if (osi_sem_is_valid(&thread->work_sem)) {
        osi_sem_free(&thread->work_sem);
    }

    if (osi_sem_is_valid(&thread->stop_sem)) {
        osi_sem_free(&thread->stop_sem);
    }


    osi_free(thread);
}

bool osi_thread_post(osi_thread_t *thread, osi_thread_func_t func, void *context, int queue_idx, uint32_t timeout)
{
    assert(thread != NULL);
    assert(func != NULL);

    if (queue_idx >= thread->work_queue_num) {
        return false;
    }

    work_item_t *item = (work_item_t *)osi_malloc(sizeof(work_item_t));
    if (item == NULL) {
        return false;
    }
    item->func = func;
    item->context = context;

    if (fixed_queue_enqueue(thread->work_queues[queue_idx], item, timeout) == false) {
        osi_free(item);
        return false;
    }

    osi_sem_give(&thread->work_sem);

    return true;
}

#if 0
/*Priority setting and get task name is not supported by XR_OS interface now */
bool osi_thread_set_priority(osi_thread_t *thread, int priority)
{
    assert(thread != NULL);

    vTaskPrioritySet(thread->thread_handle, priority);
    return true;
}

const char *osi_thread_name(osi_thread_t *thread)
{
    assert(thread != NULL);

    return pcTaskGetTaskName(thread->thread_handle);
}
#endif

int osi_thread_queue_wait_size(osi_thread_t *thread, int wq_idx)
{
    if (wq_idx < 0 || wq_idx >= thread->work_queue_num) {
        return -1;
    }

    return fixed_queue_length(thread->work_queues[wq_idx]);
}
