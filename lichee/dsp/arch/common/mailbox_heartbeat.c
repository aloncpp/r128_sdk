/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the people's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR
 * MPEGLA, ETC.) IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE
 * TO OBTAIN ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES. ALLWINNER SHALL
 * HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
 * PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
 * THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
 * OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <FreeRTOS.h>
#include <queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <awlog.h>
#include <hal_msgbox.h>
#include <delay.h>
#include <task.h>
#include <console.h>

struct msgbox_demo {
	int remote_id;
	int read_ch;
	int write_ch;
	QueueHandle_t recv_queue;
};


void mailbox_heartbeat_thread(void *param)
{
	int ret;
	struct msgbox_demo demo= {
		.remote_id = 0,		/* arm id = 0 */
		.read_ch = 2,		/* arm mailbox-heartbeat tx channel = 2 */
		.write_ch = 3,		/* arm mailbox-heartbeat rx channel = 3 */
		.recv_queue = NULL,
	};
	struct msg_endpoint ept;
	const char *data_send= "x";	/* unuse msg, only use msgbox interrupt */

	ret = hal_msgbox_alloc_channel(&ept, demo.remote_id, demo.read_ch, demo.write_ch);
	if (ret) {
		printf("Failed to allocate msgbox channel\n");
		goto alloc_err;
	}

	while (1) {
		ret = hal_msgbox_channel_send(&ept, (unsigned char *)data_send, strlen(data_send));
		if (ret != 0) {
			printf("Failed to send message\n");
			goto send_err;
		}
		msleep(1000);
	}


send_err:
	hal_msgbox_free_channel(&ept);
alloc_err:
	vTaskDelete(NULL);
	return;
}

int mailbox_heartbeat_thread_init()
{
	portBASE_TYPE ret;

	ret = xTaskCreate(mailbox_heartbeat_thread, (const char *) "heartbeat", 1024, NULL, 3, NULL);
	if (ret != pdPASS) {
		printf("Error creating mailbox-heartbeat task, status is %d\n", ret);
		return ret;
	}

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(mailbox_heartbeat_thread_init, mailbox_heartbeat_init, init mailbox heartbeat);
