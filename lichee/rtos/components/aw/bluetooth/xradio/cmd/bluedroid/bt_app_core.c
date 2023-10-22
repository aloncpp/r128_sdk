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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "kernel/os/os_queue.h"
#include "kernel/os/os_thread.h"
#include "bt_app_core.h"

#define loge(format, arg...) printf("Err : <%s : %d> " format, __func__, __LINE__, ##arg)
#define logw(format, arg...) printf("Warn: <%s : %d> " format, __func__, __LINE__, ##arg)
#define logi(format, arg...) printf("Info: <%s : %d> " format, __func__, __LINE__, ##arg)
#define logd(format, arg...) printf("Dbg : <%s : %d> " format, __func__, __LINE__, ##arg)

static void bt_app_task_handler(void *arg);
static bool bt_app_send_msg(bt_app_msg_t *msg);
static void bt_app_work_dispatched(bt_app_msg_t *msg);

static XR_OS_Queue_t s_bt_app_task_queue;
static XR_OS_Thread_t s_bt_app_thread;

bool bt_app_work_dispatch(bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len, bt_app_copy_cb_t p_copy_cback)
{
	logd("%s event 0x%x, param len %d\n", __func__, event, param_len);

	bt_app_msg_t msg;
	memset(&msg, 0, sizeof(bt_app_msg_t));

	msg.sig = BT_APP_SIG_WORK_DISPATCH;
	msg.event = event;
	msg.cb = p_cback;

	if (param_len == 0) {
		return bt_app_send_msg(&msg);
	} else if (p_params && param_len > 0) {
		if ((msg.param = malloc(param_len)) != NULL) {
			memcpy(msg.param, p_params, param_len);
			/* check if caller has provided a copy callback to do the deep copy */
			if (p_copy_cback) {
				p_copy_cback(&msg, msg.param, p_params);
			}
			return bt_app_send_msg(&msg);
		}
	}

	return false;
}

static bool bt_app_send_msg(bt_app_msg_t *msg)
{
	if (msg == NULL) {
		return false;
	}

	if (XR_OS_QueueSend(&s_bt_app_task_queue, msg, 10) != XR_OS_OK) {
		loge("%s xQueue send failed\n", __func__);
		return false;
	}
	return true;
}

static void bt_app_work_dispatched(bt_app_msg_t *msg)
{
	if (msg->cb) {
		msg->cb(msg->event, msg->param);
	}
}

static void bt_app_task_handler(void *arg)
{
	bt_app_msg_t msg;
	for (;;) {
		if (XR_OS_OK == XR_OS_QueueReceive(&s_bt_app_task_queue, &msg, XR_OS_WAIT_FOREVER)) {
			logd("%s, sig 0x%x, 0x%x\n", __func__, msg.sig, msg.event);
			switch (msg.sig) {
			case BT_APP_SIG_WORK_DISPATCH:
				bt_app_work_dispatched(&msg);
				break;
			default:
				logw("%s, unhandled sig: %d", __func__, msg.sig);
				break;
			} // switch (msg.sig)

			if (msg.param) {
				free(msg.param);
			}
		}
	}
}

void bt_app_task_start_up(void)
{
	memset(&s_bt_app_task_queue, 0, sizeof(s_bt_app_task_queue));
	XR_OS_QueueCreate(&s_bt_app_task_queue, 10, sizeof(bt_app_msg_t));

	memset(&s_bt_app_thread, 0, sizeof(s_bt_app_thread));
	XR_OS_ThreadCreate(&s_bt_app_thread, "BtAppT", bt_app_task_handler, NULL, 29/*OS_PRIORITY_NORMAL - 3*/, 8192);

	return;
}

void bt_app_task_shut_down(void)
{
	if (XR_OS_ThreadIsValid(&s_bt_app_thread)) {
		XR_OS_ThreadDelete(&s_bt_app_thread);
	}

	if (XR_OS_QueueIsValid(&s_bt_app_task_queue)) {
		XR_OS_QueueDelete(&s_bt_app_task_queue);
	}
}
