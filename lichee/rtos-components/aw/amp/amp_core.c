#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sunxi_amp.h"
#include "amp_threadpool.h"
#include <hal_cache.h>
#include <sunxi_hal_common.h>

#ifdef CONFIG_COMPONENTS_PM
#include "pm_task.h"
#endif

static sunxi_amp_info amp_info;
static sunxi_amp_msg_ops *msg_ops;
extern sunxi_amp_msg_ops msgbox_ops;
extern sunxi_amp_msg_ops test_ops;

extern sunxi_amp_func_table *func_table[];

#ifdef CONFIG_AMP_MSG_HEAP
static sunxi_amp_msg amp_msg_heap[CONFIG_AMP_MSG_HEAP_SIZE];
static unsigned int amp_msg_heap_bitmap[CONFIG_AMP_MSG_HEAP_SIZE / (sizeof(int) * 8)];
#endif

void *amp_malloc(int size)
{
    /* return malloc(size); */
    return pvPortMalloc(size);
}

void amp_free(void *ptr)
{
    /* free(ptr); */
    vPortFree(ptr);
}

void *amp_align_malloc(int size)
{
    return hal_malloc_coherent(size);
}

void amp_align_free(void *ptr)
{
    hal_free_coherent(ptr);
}

sunxi_amp_msg_ops *get_msg_ops(void)
{
    return msg_ops;
}

sunxi_amp_info *get_amp_info(void)
{
    return &amp_info;
}

int hal_amp_msg_send(sunxi_amp_msg *msg)
{
    sunxi_amp_msg_ops *msg_ops = NULL;
    sunxi_amp_info *amp = NULL;

    msg_ops = get_msg_ops();
    amp = get_amp_info();

    if (!msg_ops || !amp)
    {
        amp_err("msg_ops/amp can not be NULL!\n");
        return -1;
    }
    msg_ops->send_to_queue(amp, msg);
    return 0;
}

int hal_amp_msg_recv(sunxi_amp_msg *msg)
{
    sunxi_amp_msg_ops *msg_ops = NULL;
    sunxi_amp_info *amp = NULL;

    msg_ops = get_msg_ops();
    amp = get_amp_info();

    if (!msg_ops || !amp)
    {
        amp_err("msg_ops/amp can not be NULL!\n");
        return -1;
    }
    msg_ops->receive_from_dev(amp, msg);
    return 0;
}

void _dump_amp_msg(sunxi_amp_msg *msg)
{
    if (msg)
    {
        amp_debug("--------msg--------\n");
        amp_debug("type  = %d\n", msg->type);
        amp_debug("rpcid = %d\n", msg->rpcid);
        amp_debug("prio  = %d\n", msg->prio);
        amp_debug("src   = %d\n", msg->src);
        amp_debug("dst   = %d\n", msg->dst);
        amp_debug("data  = 0x%08x\n", msg->data);
        amp_debug("flags = 0x%08x\n", msg->flags);
        amp_debug("-------------------\n");
    }
}

void _dump_amp_wait(sunxi_amp_wait *wait)
{
    if (wait)
    {
        amp_debug("--------wait--------\n");
        amp_debug("task  = 0x%08x\n", (uint32_t)wait->task);
        amp_debug("flags = 0x%08x\n", wait->flags);
        amp_debug("signal= 0x%08x\n", (uint32_t)wait->signal);
        amp_debug("-------------------\n");
    }
}

sunxi_amp_msg *allocate_amp_msg_memory(void)
{
    sunxi_amp_msg *msg = NULL;
#ifdef CONFIG_AMP_MSG_HEAP
    int i = 0;
    int pos_level1 = 0;
    int pos_level2 = 0;
    int pos = -1;

    taskENTER_CRITICAL();
    /* ret = xSemaphoreTake(amp_info.amp_msg_heap_mutex, portMAX_DELAY); */

    for (i = 0; i < ARRAY_SIZE(amp_msg_heap_bitmap); i++)
    {
        if (amp_msg_heap_bitmap[i] != 0L)
        {
            pos = ffs(amp_msg_heap_bitmap[i]) + i * sizeof(amp_msg_heap_bitmap[0]) * 8;
            break;
        }
    }
    if (pos >= 0 && pos < CONFIG_AMP_MSG_HEAP_SIZE)
    {
        msg = &amp_msg_heap[pos];
        pos_level1 = pos / (sizeof(int) * 8);
        pos_level2 = pos % (sizeof(int) * 8);
        amp_msg_heap_bitmap[pos_level1] &= ~(1 << pos_level2);
    }

    taskEXIT_CRITICAL();
    /* ret = xSemaphoreGive(amp_info.amp_msg_heap_mutex); */

    if (msg == NULL)
#endif
    {
        msg = amp_malloc(sizeof(sunxi_amp_msg));
    }

    return msg;
}

void free_amp_msg_memory(sunxi_amp_msg *msg)
{
#ifdef CONFIG_AMP_MSG_HEAP
    if (((unsigned long)msg > (unsigned long)&amp_msg_heap)
        && ((unsigned long)msg <= (unsigned long)&amp_msg_heap + sizeof(amp_msg_heap)))
    {
        /* ret = xSemaphoreTake(amp_info.amp_msg_heap_mutex, portMAX_DELAY); */
        taskENTER_CRITICAL();
        int pos = ((unsigned long)msg - (unsigned long)&amp_msg_heap) / sizeof(sunxi_amp_msg);
        int pos_level1 = pos / (sizeof(int) * 8);
        int pos_level2 = pos % (sizeof(int) * 8);
        amp_msg_heap_bitmap[pos_level1] |= (1 << pos_level2);
        taskEXIT_CRITICAL();
        /* ret = xSemaphoreGive(amp_info.amp_msg_heap_mutex); */
    }
    else
#endif
    {
        amp_free(msg);
    }
}

static int handle_recv_func_ret_msg(sunxi_amp_info *sunxi_amp, sunxi_amp_msg *msg)
{
    struct list_head *pos;
    struct list_head *q;

    taskENTER_CRITICAL();

    list_for_each_safe(pos, q, &amp_info.wait.i_list)
    {
        sunxi_amp_wait *tmp;
        tmp = list_entry(pos, sunxi_amp_wait, i_list);
        if (tmp->flags == msg->flags)
        {
            memcpy(&tmp->msg, msg, sizeof(sunxi_amp_msg));
            list_del(pos);
            taskEXIT_CRITICAL();
            amp_debug("resume task 0x%08x(%s)\n", (uint32_t)tmp->task, pcTaskGetName(tmp->task));

            dump_amp_wait(tmp);
            if (pdPASS != xSemaphoreGive(tmp->signal))
            {
                amp_debug("wait signal fail\n");
                return -1;
            }
            return 0;
        }
    }
    taskEXIT_CRITICAL();

    return -1;
}

static int handle_send_amp_msg(sunxi_amp_info *sunxi_amp, sunxi_amp_msg *msg)
{
    return msg_ops->send_to_dev(sunxi_amp, msg);
}

int handle_send_ret_value_msg(sunxi_amp_info *sunxi_amp, sunxi_amp_msg *msg,
                              sunxi_amp_func_table *item,
                              sunxi_amp_msg_args *args,
                              unsigned long data,
                              int type)
{
    sunxi_amp_msg *send_msg = NULL;
    sunxi_amp_msg_ops *msg_ops = NULL;

    msg_ops = get_msg_ops();
    send_msg = allocate_amp_msg_memory();
    if (!send_msg)
    {
        amp_err("malloc err\n");
        return -1;
    }
    /* memset(send_msg, 0, sizeof(sunxi_amp_msg)); */
    send_msg->type = type;
    send_msg->data = (uint32_t)data;
    send_msg->flags = msg->flags;
    send_msg->src  = msg->dst;
    send_msg->dst  = msg->src;
    msg_ops->send_to_queue(sunxi_amp, send_msg);
    free_amp_msg_memory(send_msg);
    return 0;
}

static int handle_recv_func_call_msg(sunxi_amp_info *sunxi_amp, sunxi_amp_msg *msg)
{
    int ret = -1;
    int args_num;
    unsigned long ret_value;

    sunxi_amp_func_table *item;

    sunxi_amp_func0 func0;
    sunxi_amp_func1 func1;
    sunxi_amp_func2 func2;
    sunxi_amp_func3 func3;
    sunxi_amp_func4 func4;
    sunxi_amp_func5 func5;
    sunxi_amp_func6 func6;
    sunxi_amp_func7 func7;
    sunxi_amp_func8 func8;

    int mod_id = (msg->rpcid >> 8) & 0xff;
    int func_id = msg->rpcid & 0x00ff;

    sunxi_amp_msg_args *args = (sunxi_amp_msg_args *)(unsigned long)msg->data;

    if (!args)
    {
        amp_err("msg err\n");
        return -1;
    }

    hal_dcache_invalidate((unsigned long)args, sizeof(sunxi_amp_msg_args));
    args_num = args->args_num;

    item = func_table[mod_id];
    if (!item)
    {
        amp_err("not support %d serivce\n", mod_id);
        return -1;
    }
    item += func_id;

    if (item->args_num != args_num)
    {
        amp_err("msg args num = %d, func args num = %d\n", args_num, item->args_num);
        return -1;
    }

    switch (args_num)
    {
        case 0:
            func0 = (sunxi_amp_func0)item->func;
            if (item->return_type == RET_NULL)
            {
                func0();;
            }
            else if (item->return_type == RET_POINTER)
            {
                ret_value = func0();
                ret = handle_send_ret_value_msg(sunxi_amp, msg, item, args, ret_value, MSG_DIRECT_FUNC_RET);
            }
            break;
        case 1:
            func1 = (sunxi_amp_func1)item->func;
            if (item->return_type == RET_NULL)
            {
                func1((void *)(unsigned long)args->args[0]);
            }
            else if (item->return_type == RET_POINTER)
            {
                ret_value = func1((void *)(unsigned long)args->args[0]);
                ret = handle_send_ret_value_msg(sunxi_amp, msg, item, args, ret_value, MSG_DIRECT_FUNC_RET);
            }
            break;
        case 2:
            func2 = (sunxi_amp_func2)item->func;
            if (item->return_type == RET_NULL)
            {
                func2((void *)(unsigned long)args->args[0], (void *)(unsigned long)args->args[1]);
            }
            else if (item->return_type == RET_POINTER)
            {
                ret_value = func2((void *)(unsigned long)args->args[0], (void *)(unsigned long)args->args[1]);
                ret = handle_send_ret_value_msg(sunxi_amp, msg, item, args, ret_value, MSG_DIRECT_FUNC_RET);
            }
            break;
        case 3:
            func3 = (sunxi_amp_func3)item->func;
            if (item->return_type == RET_NULL)
            {
                func3((void *)(unsigned long)args->args[0], (void *)(unsigned long)args->args[1], (void *)(unsigned long)args->args[2]);
            }
            else if (item->return_type == RET_POINTER)
            {
                ret_value = func3((void *)(unsigned long)args->args[0], (void *)(unsigned long)args->args[1], (void *)(unsigned long)args->args[2]);
                ret = handle_send_ret_value_msg(sunxi_amp, msg, item, args, ret_value, MSG_DIRECT_FUNC_RET);
            }
            break;
        case 4:
            func4 = (sunxi_amp_func4)item->func;
            if (item->return_type == RET_NULL)
            {
                func4((void *)(unsigned long)args->args[0], (void *)(unsigned long)args->args[1], (void *)(unsigned long)args->args[2],
                      (void *)(unsigned long)args->args[3]);
            }
            else if (item->return_type == RET_POINTER)
            {
                ret_value = func4((void *)(unsigned long)args->args[0], (void *)(unsigned long)args->args[1], (void *)(unsigned long)args->args[2],
                                  (void *)(unsigned long)args->args[3]);
                ret = handle_send_ret_value_msg(sunxi_amp, msg, item, args, ret_value, MSG_DIRECT_FUNC_RET);
            }
            break;
        case 5:
            func5 = (sunxi_amp_func5)item->func;
            if (item->return_type == RET_NULL)
            {
                func5((void *)(unsigned long)args->args[0], (void *)(unsigned long)args->args[1], (void *)(unsigned long)args->args[2],
                      (void *)(unsigned long)args->args[3], (void *)(unsigned long)args->args[4]);
            }
            else if (item->return_type == RET_POINTER)
            {
                ret_value = func5((void *)(unsigned long)args->args[0], (void *)(unsigned long)args->args[1], (void *)(unsigned long)args->args[2],
                                  (void *)(unsigned long)args->args[3], (void *)(unsigned long)args->args[4]);
                ret = handle_send_ret_value_msg(sunxi_amp, msg, item, args, ret_value, MSG_DIRECT_FUNC_RET);
            }
            break;
        case 6:
            func6 = (sunxi_amp_func6)item->func;
            if (item->return_type == RET_NULL)
            {
                func6((void *)(unsigned long)args->args[0], (void *)(unsigned long)args->args[1], (void *)(unsigned long)args->args[2],
                      (void *)(unsigned long)args->args[3], (void *)(unsigned long)args->args[4], (void *)(unsigned long)args->args[5]);
            }
            else if (item->return_type == RET_POINTER)
            {
                ret_value = func6((void *)(unsigned long)args->args[0], (void *)(unsigned long)args->args[1], (void *)(unsigned long)args->args[2],
                                  (void *)(unsigned long)args->args[3], (void *)(unsigned long)args->args[4], (void *)(unsigned long)args->args[5]);
                ret = handle_send_ret_value_msg(sunxi_amp, msg, item, args, ret_value, MSG_DIRECT_FUNC_RET);
            }
            break;
        case 7:
            func7 = (sunxi_amp_func7)item->func;
            if (item->return_type == RET_NULL)
            {
                func7((void *)(unsigned long)args->args[0], (void *)(unsigned long)args->args[1], (void *)(unsigned long)args->args[2],
                      (void *)(unsigned long)args->args[3], (void *)(unsigned long)args->args[4], (void *)(unsigned long)args->args[5],
                      (void *)(unsigned long)args->args[6]);
            }
            else if (item->return_type == RET_POINTER)
            {
                ret_value = func7((void *)(unsigned long)args->args[0], (void *)(unsigned long)args->args[1], (void *)(unsigned long)args->args[2],
                                  (void *)(unsigned long)args->args[3], (void *)(unsigned long)args->args[4], (void *)(unsigned long)args->args[5],
                                  (void *)(unsigned long)args->args[6]);
                ret = handle_send_ret_value_msg(sunxi_amp, msg, item, args, ret_value, MSG_DIRECT_FUNC_RET);
            }
            break;
        case 8:
            func8 = (sunxi_amp_func8)item->func;
            if (item->return_type == RET_NULL)
            {
                func8((void *)(unsigned long)args->args[0], (void *)(unsigned long)args->args[1], (void *)(unsigned long)args->args[2],
                      (void *)(unsigned long)args->args[3], (void *)(unsigned long)args->args[4], (void *)(unsigned long)args->args[5],
                      (void *)(unsigned long)args->args[6], (void *)(unsigned long)args->args[7]);
            }
            else if (item->return_type == RET_POINTER)
            {
                ret_value = func8((void *)(unsigned long)args->args[0], (void *)(unsigned long)args->args[1], (void *)(unsigned long)args->args[2],
                                  (void *)(unsigned long)args->args[3], (void *)(unsigned long)args->args[4], (void *)(unsigned long)args->args[5],
                                  (void *)(unsigned long)args->args[6], (void *)(unsigned long)args->args[7]);
                ret = handle_send_ret_value_msg(sunxi_amp, msg, item, args, ret_value, MSG_DIRECT_FUNC_RET);
            }
            break;
        default:
            amp_err("can not support args num %d\n", args_num);
            break;
    }
    return ret;
}

#if defined(CONFIG_AMP_FUNCCALL_THREAD) && !defined(CONFIG_AMP_FUNCALL_THREADPOOL_WAIT_RECVQUEUE)
static void run_func_call_entry(void *param)
{
    sunxi_amp_msg *msg = (sunxi_amp_msg *)param;
    sunxi_amp_info *amp = get_amp_info();

    int ret = -1;

#ifdef CONFIG_AMP_THREAD_PRIO_CHANGE
    int oldprio = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, msg->prio);
    amp_debug("reset prio:(%d)->(%d)\r\n", oldprio, msg->prio);
#endif

    if (msg->type == MSG_DIRECT_FUNC_CALL)
    {
        if (msg && amp)
        {
            ret = handle_recv_func_call_msg(amp, msg);
        }
        else
        {
            amp_err("msg or amp error!\n");
        }
    }
    else if (msg->type == MSG_SERIAL_FUNC_CALL)
    {
        MsgBufHeader *header = (MsgBufHeader *)(unsigned long)msg->data;
        MsgBuf *msgbuf = msg_buf_init(header);
        MsgBuf *sendBuffer = NULL;

        sunxi_amp_msg *pMsg = &header->msg;
        if (!msgbuf)
        {
            goto out;
        }

        int mod_id = (msg->rpcid >> 8) & 0xff;
        int func_id = msg->rpcid & 0x00ff;

        sunxi_amp_msg_args *args = (sunxi_amp_msg_args *)(unsigned long)msg->data;
        sunxi_amp_func_table *item = NULL;

        if (!args)
        {
            amp_err("msg err\n");
            goto out;
        }

        item = func_table[mod_id];
        if (!item)
        {
            amp_err("not support %d serivce\n", mod_id);
            goto out;
        }
        item += func_id;

        hal_dcache_invalidate((unsigned long)header, sizeof(*header));
        hal_dcache_invalidate((unsigned long)header, header->bufferSize);

        sunxi_amp_service_func func = item->func;
        func(pMsg, msgbuf, &sendBuffer);
        if (sendBuffer)
        {
            MsgBufHeader *sendheader = (MsgBufHeader *)sendBuffer->data;
            sendheader->msg.src = pMsg->dst;
            sendheader->msg.dst = pMsg->src;
            sendheader->msgSize = sendBuffer->wpos + sizeof(MsgBufHeader);
            sendheader->bufferSize = sendBuffer->size;
            hal_dcache_clean((unsigned long)sendheader, sendheader->bufferSize);
            handle_send_ret_value_msg(amp, pMsg, item, args, (unsigned long)sendBuffer->data, MSG_SERIAL_FUNC_RET);
            msg_buf_deinit(msgbuf);
            msg_buf_deinit(sendBuffer);
        }
    }
out:
    free_amp_msg_memory(msg);

#ifndef CONFIG_AMP_FUNCCALL_THREADPOOL
#ifdef CONFIG_COMPONENTS_PM
    pm_task_unregister(xTaskGetCurrentTaskHandle());
#endif
    vTaskDelete(NULL);
#endif

#ifdef CONFIG_AMP_THREAD_PRIO_CHANGE
    vTaskPrioritySet(NULL, oldprio);
    amp_debug("restore prio:(%d)\r\n", oldprio);
#endif

}

static int handle_func_call_by_thread(sunxi_amp_info *amp, sunxi_amp_msg *msg)
{
    int ret = 0;
    sunxi_amp_msg *pMsg;

    pMsg = allocate_amp_msg_memory();
    if (!pMsg)
    {
        amp_err("alloc msg failed!\n");
        return -1;
    }
    memcpy(pMsg, msg, sizeof(sunxi_amp_msg));

#ifdef CONFIG_AMP_FUNCCALL_THREADPOOL
    ret = amp_threadpool_add_task(amp_get_threadpool(), run_func_call_entry, (void *)pMsg);
    if (ret)
    {
        amp_err("amp create task failed.\n");
    }
#else
    TaskHandle_t xHandle = NULL;
#ifdef CONFIG_AMP_THREAD_PRIO_CHANGE
    int prio = msg->prio;
#else
    int prio = 31;
#endif
    ret = xTaskCreate(run_func_call_entry, "func", AMP_RPC_HANDLE_TASK_STACK_SIZE,
                      (void *)pMsg, prio, &xHandle);
    if (ret != pdPASS)
    {
        amp_err("amp create task failed.\n");
    }
#ifdef CONFIG_COMPONENTS_PM
    pm_task_register(xHandle, PM_TASK_TYPE_SYS);
#endif
#endif
    return 0;
}
#endif

static int handle_recv_free_buffer(sunxi_amp_info *sunxi_amp, sunxi_amp_msg *msg)
{
    int ret = -1;
    MsgBufHeader *header;

    header = (MsgBufHeader *)(unsigned long)msg->data;
    if (header)
    {
        amp_free(header);
        ret = 0;
    }
    return ret;
}

int handle_recv_amp_msg(sunxi_amp_info *sunxi_amp, sunxi_amp_msg *msg)
{
    int ret = -1;

    switch (msg->type)
    {
        case MSG_SERIAL_FUNC_CALL:
#ifdef CONFIG_AMP_FUNCCALL_THREAD
#ifdef CONFIG_AMP_FUNCALL_THREADPOOL_WAIT_RECVQUEUE
            ret = handle_recv_func_call_msg(sunxi_amp, msg);
#else
            ret = handle_func_call_by_thread(sunxi_amp, msg);
#endif
#else
            ret = handle_recv_func_call_msg(sunxi_amp, msg);
#endif
            break;
        case MSG_SERIAL_FUNC_RET:
            ret = handle_recv_func_ret_msg(sunxi_amp, msg);
            break;
        case MSG_DIRECT_FUNC_CALL:
#ifdef CONFIG_AMP_FUNCCALL_THREAD
#ifdef CONFIG_AMP_FUNCALL_THREADPOOL_WAIT_RECVQUEUE
            ret = handle_recv_func_call_msg(sunxi_amp, msg);
#else
            ret = handle_func_call_by_thread(sunxi_amp, msg);
#endif
#else
            ret = handle_recv_func_call_msg(sunxi_amp, msg);
#endif
            break;
        case MSG_DIRECT_FUNC_RET:
            ret = handle_recv_func_ret_msg(sunxi_amp, msg);
            break;
        case MSG_SERIAL_FREE_BUFFER:
            ret = handle_recv_free_buffer(sunxi_amp, msg);
            break;
        default:
            break;
    }
    return ret;
}

static void sunxi_amp_send_task(void *pvParameters)
{
    sunxi_amp_msg receive_val = {0};
    BaseType_t xStatus;
    sunxi_amp_info *sunxi_amp = pvParameters;

    const TickType_t xTicksToWait = portMAX_DELAY;

    for (;;)
    {
#if 0
        if ((ret = uxQueueMessagesWaiting(sunxi_amp->send_queue)) != 0)
        {
            amp_debug("QM Waiting:0x%0x\n", ret);
        }
#endif
        xStatus = xQueueReceive(sunxi_amp->send_queue, &receive_val, xTicksToWait);
        if (xStatus == pdPASS)
        {
            amp_debug("Received = 0x%0x\n", (uint32_t)&receive_val);
            dump_amp_msg(&receive_val);
            handle_send_amp_msg(sunxi_amp, &receive_val);
        }
    }
#ifdef CONFIG_COMPONENTS_PM
    pm_task_unregister(xTaskGetCurrentTaskHandle());
#endif
    vTaskDelete(NULL);
}

static void sunxi_amp_recv_task(void *pvParameters)
{
#ifndef CONFIG_AMP_FUNCALL_THREADPOOL_WAIT_RECVQUEUE
    sunxi_amp_msg receive_val = {0};
    BaseType_t xStatus;
    sunxi_amp_info *sunxi_amp = pvParameters;

    const TickType_t xTicksToWait = portMAX_DELAY;

    for (;;)
    {
        xStatus = xQueueReceive(sunxi_amp->recv_queue, &receive_val, xTicksToWait);
        if (xStatus == pdPASS)
        {
            amp_debug("Received = 0x%08x\n", (uint32_t)&receive_val);
            dump_amp_msg(&receive_val);
            handle_recv_amp_msg(sunxi_amp, &receive_val);
        }
    }
#endif
#ifdef CONFIG_COMPONENTS_PM
    pm_task_unregister(xTaskGetCurrentTaskHandle());
#endif
    vTaskDelete(NULL);
}

int amp_init(void)
{
    int ret = -1;

#ifdef CONFIG_DRIVERS_MSGBOX
    msg_ops = &msgbox_ops;
#else
    msg_ops = &test_ops;
#endif
    if (msg_ops->init)
    {
        msg_ops->init();
    }

    amp_info.amp_msg_heap_mutex = xSemaphoreCreateMutex();
    if (amp_info.amp_msg_heap_mutex == NULL)
    {
        amp_err("amp create mutex error!\n");
        return -1;
    }

    amp_info.send_queue = xQueueCreate(AMP_QUEUE_LENGTH, sizeof(sunxi_amp_msg));
    if (amp_info.send_queue == NULL)
    {
        amp_err("amp create queue error!\n");
        return -1;
    }
    amp_info.recv_queue = xQueueCreate(AMP_QUEUE_LENGTH, sizeof(sunxi_amp_msg));
    if (amp_info.recv_queue == NULL)
    {
        amp_err("amp create queue error!\n");
        goto err;
    }

    INIT_LIST_HEAD(&amp_info.wait.i_list);

    ret = xTaskCreate(sunxi_amp_send_task, "amp-send-task", AMP_SEND_TASK_STACK,
                      (void *)&amp_info, AMP_SEND_TASK_PRIO, &amp_info.sendTask);
    if (ret != pdPASS)
    {
        amp_err("amp create task failed.\n");
        ret = -1;
        goto err;
    }
#ifdef CONFIG_COMPONENTS_PM
    pm_task_register(amp_info.sendTask, PM_TASK_TYPE_SYS);
#endif

    ret = xTaskCreate(sunxi_amp_recv_task, "amp-recv-task", AMP_RECV_TASK_STACK,
                      (void *)&amp_info, AMP_RECV_TASK_PRIO, &amp_info.recvTask);
    if (ret != pdPASS)
    {
        amp_err("amp create task failed.\n");
        ret = -1;
        goto err;
    }
#ifdef CONFIG_COMPONENTS_PM
    pm_task_register(amp_info.recvTask, PM_TASK_TYPE_SYS);
#endif

#ifdef CONFIG_AMP_FUNCCALL_THREADPOOL
    amp_threadpool_init();
#endif

#ifdef CONFIG_AMP_BOOT_CORE_DETECT
extern void amp_boot_core_save();
    amp_boot_core_save();
#endif

    return 0;
err:
    if (amp_info.recv_queue)
    {
        vQueueDelete(amp_info.recv_queue);
    }
    if (amp_info.send_queue)
    {
        vQueueDelete(amp_info.send_queue);
    }
    if (amp_info.amp_msg_heap_mutex)
    {
        vSemaphoreDelete(amp_info.amp_msg_heap_mutex);
    }

    return ret;
}
