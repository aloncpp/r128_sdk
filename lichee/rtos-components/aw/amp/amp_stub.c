#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <reent.h>

#include "sunxi_amp.h"
#include <console.h>

#include <hal_cache.h>

unsigned long wait_ret_value(sunxi_amp_wait *wait)
{
    xSemaphoreTake(wait->signal, portMAX_DELAY);
    return wait->msg.data;
}

unsigned long wait_ret_value_msgbuf(sunxi_amp_wait *wait)
{
    xSemaphoreTake(wait->signal, portMAX_DELAY);
    MsgBuf *msgbuf = msg_buf_init((MsgBufHeader *)(unsigned long)wait->msg.data);
    MsgBufHeader *header = (MsgBufHeader *)msgbuf->data;
    hal_dcache_invalidate((unsigned long)header, sizeof(*header));
    hal_dcache_invalidate((unsigned long)header, header->bufferSize);
    dump_amp_msg(&header->msg);
    return (unsigned long)msgbuf;
}

int remote_serial_func_call_free_buffer(sunxi_amp_msg *msg, MsgBuf *msgBuffer)
{
    sunxi_amp_msg send_msg;

    memset(&send_msg, 0, sizeof(send_msg));

    send_msg.type = MSG_SERIAL_FREE_BUFFER;
    send_msg.rpcid = 0;
    send_msg.src = msg->dst;
    send_msg.dst = msg->src;
    send_msg.data = (uint32_t)(unsigned long)msgBuffer->data;
    hal_amp_msg_send(&send_msg);
    return 0;
}
/*
 * func_stub:
 */
unsigned long func_stub_test(uint32_t id, MsgBuf *msgBuffer, int haveRet, int stub_args_num, void *stub_args[])
{
    sunxi_amp_msg msg;
    sunxi_amp_msg_args args;
    sunxi_amp_wait wait;
    sunxi_amp_info *amp_info;
    MsgBufHeader *header;
    long ret = 0;
    int i;

    amp_info = get_amp_info();

    memset(&msg, 0, sizeof(msg));
    memset(&args, 0, sizeof(args));
    memset(&wait, 0, sizeof(wait));

    msg.type = MSG_SERIAL_FUNC_CALL;
    msg.rpcid = (id & 0xffff);
    msg.src = (id >> 24) & 0xf;
    msg.dst = (id >> 28) & 0xf;
    msg.data = (uint32_t)(unsigned long)msgBuffer->data;
    msg.flags = (uint32_t)(unsigned long)xTaskGetCurrentTaskHandle();

    args.args_num = stub_args_num;

    for (i = 0; i < stub_args_num; i++)
    {
        args.args[i] = 0;
    }
    header = (MsgBufHeader *)(msgBuffer->data);
    memcpy(&header->msg, &msg, sizeof(sunxi_amp_msg));
    header->msgSize = msgBuffer->wpos + sizeof(MsgBufHeader);
    header->bufferSize = msgBuffer->size;
    hal_dcache_clean((unsigned long)msgBuffer->data, msgBuffer->size);

    if (haveRet)
    {
        wait.flags = msg.flags;
        wait.task = xTaskGetCurrentTaskHandle();
        wait.signal = xSemaphoreCreateCounting(0xffffffffU, 0);
        if (wait.signal == NULL)
        {
            amp_err("create signal failed\n");
            return -1;
        }
        INIT_LIST_HEAD(&wait.i_list);
        taskENTER_CRITICAL();
        list_add(&wait.i_list, &amp_info->wait.i_list);
        taskEXIT_CRITICAL();
        hal_amp_msg_send(&msg);
        ret = wait_ret_value_msgbuf(&wait);
        vSemaphoreDelete(wait.signal);
    }
    else
    {
        hal_amp_msg_send(&msg);
    }
    return ret;
}

/*
 * func_stub:
 */
unsigned long func_stub(uint32_t id, int haveRet, int stub_args_num, void *stub_args[])
{
    sunxi_amp_msg msg;
    sunxi_amp_msg_args __attribute__((aligned(64))) args;
    sunxi_amp_wait wait;
    sunxi_amp_info *amp_info;
    int ret = 0;
    int i;

    amp_info = get_amp_info();

    memset(&msg, 0, sizeof(msg));
    memset(&args, 0, sizeof(args));
    memset(&wait, 0, sizeof(wait));

    msg.type = MSG_DIRECT_FUNC_CALL;
    msg.rpcid = (id & 0xffff);
    msg.src = (id >> 26) & 0xf;
    msg.dst = (id >> 29) & 0xf;
    msg.prio = uxTaskPriorityGet(NULL);
    msg.data = (uint32_t)(unsigned long)&args;
    msg.flags = (uint32_t)(unsigned long)xTaskGetCurrentTaskHandle();

    args.args_num = stub_args_num;

    for (i = 0; i < stub_args_num; i++)
    {
        args.args[i] = (uint32_t)(unsigned long)stub_args[i];
    }
    hal_dcache_clean((unsigned long)&args, sizeof(args));

    if (haveRet)
    {
        wait.flags = msg.flags;
        wait.task = xTaskGetCurrentTaskHandle();
        wait.signal = xSemaphoreCreateCounting(0xffffffffU, 0);
        if (wait.signal == NULL)
        {
            amp_err("create signal failed\n");
            return -1;
        }
        INIT_LIST_HEAD(&wait.i_list);
        taskENTER_CRITICAL();
        list_add(&wait.i_list, &amp_info->wait.i_list);
        taskEXIT_CRITICAL();
        hal_amp_msg_send(&msg);
        ret = wait_ret_value(&wait);
        vSemaphoreDelete(wait.signal);
    }
    else
    {
        hal_amp_msg_send(&msg);
    }
    return ret;
}

