/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
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

#include <console.h>

#define MSGBOX_RISCV 2
#define MSGBOX_DSP   1
#define MSGBOX_ARM   0

#ifndef printfFromISR
#define printfFromISR printf
#endif


#define MSGBOX_DEMO_HELP \
	"\nmsgbox_demo [-s MESSAGE] [-r [-t TIMEOUT]]\n\n" \
	"OPTIONS:\n" \
	" -s MESSAGE : send MESSAGE\n" \
	" -r         : receive messages\n" \
	" -t TIMEOUT : exit in TIMEOUT seconds\n" \
	"e.g.\n" \
	" msgbox_demo -s \"hello\"   : send string \"hello\"\n" \
	" msgbox_demo -r           : receive messages (default in 10 seconds)\n" \
	" msgbox_demo -r -t 20     : receive messages in 20 seconds\n" \
	"\n"

#define RECEIVE_QUEUE_LENGTH 16
#define RECEIVE_QUEUE_WAIT_MS 100

#define MSGBOX_DEMO_RECV_CHANNEL 0
#define MSGBOX_DEMO_SEND_CHANNEL 0

struct msgbox_demo {
	struct msg_channel *recv_ch;
	struct msg_channel *send_ch;
	QueueHandle_t recv_queue;
	volatile int send_finish;
};

static int recv_from_cpu_callback(uint32_t data, void *private_data)
{
	/*
	 * NOTE:
	 *  Please use "printfFromISR" rather than "printf". Furthermore,
	 *  in the msgbox channel callback, we should use the "xxFromISR" APIs
	 *  rather than common APIs for all system calls.
	 */
	printfFromISR("Receive callback (data: 0x%x)\n", data);
	BaseType_t xHigherProTaskWoken = pdFALSE;
	struct msgbox_demo *instance= private_data;
	BaseType_t ret = xQueueSendFromISR(instance->recv_queue, &data, &xHigherProTaskWoken);
	if (ret == errQUEUE_FULL) {
		printfFromISR("recv_queue is full\n");
		return -1;
	}
	if (ret == pdPASS) {
		portYIELD_FROM_ISR(xHigherProTaskWoken);
	}

	return 0;
}

static int send_to_cpu_callback(unsigned long data, void *private_data)
{
	/*
	 * Similar to "recv_from_cpu_callback", we should use "xxFromISR" APIs
	 * rather than common APIs.
	 */
	printfFromISR("Send callback\n");

	struct msgbox_demo *instance = private_data;

	instance->send_finish = 1;
	return 0;
}

static int msgbox_demo_send(struct msgbox_demo *instance,
		struct msg_endpoint * medp,
		unsigned char *data, size_t len)
{
	int result;

	printf("send :");
	for (int i=0; i<len; i++)
		printf(" %x", *(data+i));
	printf("\n");
#if 0
	while (!instance->send_finish);

	instance->send_finish = 0;
#endif
	result = hal_msgbox_channel_send(medp, data, len);
#if 0
	while (!instance->send_finish);
#endif

	return result;
}

struct msg_endpoint sedp;
static int cmd_msgbox_demo(int argc, char *argv[])
{
	int ret = 0;
	int c;
	struct msg_endpoint *medp = &sedp;
	struct msgbox_demo demo_cpu = {
		.recv_ch = MSGBOX_DEMO_RECV_CHANNEL,
		.send_ch = MSGBOX_DEMO_SEND_CHANNEL,
		.recv_queue = NULL,
		.send_finish = 1,
	};
	TickType_t start_ticks, current_ticks;
	int do_send = 0;
	const char *data_send= NULL;
	int do_recv = 0;
	int timeout_sec = 10;
	uint32_t data_recv;
	int remote = MSGBOX_ARM;
	int recv_channel = -1;
	int send_channel = -1;

	if (argc <= 1) {
		printf(MSGBOX_DEMO_HELP);
		ret = -1;
		goto out;
	}

	while ((c = getopt(argc, argv, "hs:rt:")) != -1) {
		switch (c) {
		case 'h' :
			printf(MSGBOX_DEMO_HELP);
			ret = 0;
			goto out;
		case 's':
			do_send = 1;
			data_send = optarg;
			break;
		case 'r':
			do_recv = 1;
			break;
		case 't':
			timeout_sec = atoi(optarg);
			break;
		default:
			printf(MSGBOX_DEMO_HELP);
			ret = -1;
			goto out;
		}
	}

	medp->private = &demo_cpu;

	if (do_recv) {
		medp->rec = (void *)recv_from_cpu_callback;
		demo_cpu.recv_queue = xQueueCreate(RECEIVE_QUEUE_LENGTH, sizeof(uint32_t));
		if (!demo_cpu.recv_queue) {
			printf("Failed to create receive queue\n");
			/* ret = -ENOMEM; */
			ret = -1;
			goto out;
		}
		recv_channel = MSGBOX_DEMO_RECV_CHANNEL;
	}
	if (do_send) {
		send_channel = MSGBOX_DEMO_SEND_CHANNEL;
	}

#if defined(CONFIG_PROJECT_R128_BOOT_C906) //arm to rv
	remote = MSGBOX_RISCV;
#elif defined(CONFIG_PROJECT_R128_C906_MINI) //rv to arm
	remote = MSGBOX_ARM;
#endif

	hal_msgbox_alloc_channel(medp, remote, recv_channel, send_channel);

	if (do_send) {
		msgbox_demo_send(&demo_cpu, medp, (unsigned char *)data_send, strlen(data_send));
	}

	if (do_recv) {

		printf("msgbox_demo will exit in %d seconds\n", timeout_sec);

		start_ticks = xTaskGetTickCount();
		printf("start_ticks: %u\n", start_ticks);

		while (1) {
			if (pdTRUE == xQueueReceive(demo_cpu.recv_queue, &data_recv,
						RECEIVE_QUEUE_WAIT_MS / portTICK_PERIOD_MS)) {
				printf("Received from queue: 0x%x\n", data_recv);
			}
			current_ticks = xTaskGetTickCount();
			if ((current_ticks - start_ticks) * portTICK_PERIOD_MS
					>= timeout_sec * 1000) {
				printf("current_ticks: %u\n", current_ticks);
				break;
			}
		}
	}

	printf("msgbox_demo exited\n");
	hal_msgbox_free_channel(medp);
	ret = 0;

delete_recv_queue:
	if (do_recv) {
		vQueueDelete(demo_cpu.recv_queue);
	}
release_channel:
	/* TODO: release allocated channels */
	if (do_recv) {
	}
	if (do_send) {
	}
out:
	return ret;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_msgbox_demo, msgbox_demo, msgbox demo);
