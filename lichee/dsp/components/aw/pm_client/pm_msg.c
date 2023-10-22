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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <awlog.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>

#include <console.h>
#include <hal_msgbox.h>

#include "aw_io.h"
#include "pm_msg.h"

static struct msg_endpoint pm_edp;
static pm_msg_t pm_msg_inst;

/*
 * can't define USE_TX_DONE_CALLBACK, because driver can't support tx_done
 * callbcak
 * */
//#define USE_TX_DONE_CALLBACK

//#define PM_MSG_DEBUG
#if PM_MSG_DEBUG
static void dbg_save_status_to_rtc(uint32_t value)
{
	writel(value, 0x07090000 | 0x100 | 0x10);
}
#else
#define  dbg_save_status_to_rtc(_x)   do{}while(0)
#endif


static void pm_msg_recv_callback(uint32_t data, void *private_data)
{
	/*
	 * NOTE:
	 *  Please use "printfFromISR" rather than "printf". Furthermore,
	 *  in the msgbox channel callback, we should use the "xxFromISR" APIs
	 *  rather than common APIs for all system calls.
	 */
#if PM_MSG_DEBUG
	printfFromISR("Receive callback (data: 0x%lx)\n", data);
#endif

	pm_msg_t *instance = &pm_msg_inst;
	BaseType_t xHigherPriorityTaskWoken = 0;

	xQueueSendFromISR(instance->xQueue, &data, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

#if USE_TX_DONE_CALLBACK
static void  pm_msg_send_callback(void *private_data)
{
	/*
	 * Similar to "recv_from_cpu_callback", we should use "xxFromISR" APIs
	 * rather than common APIs.
	 */
#if PM_MSG_DEBUG
	printfFromISR("Send callback\n");
#endif

	pm_msg_t *instance = &pm_msg_inst;
	BaseType_t xHigherPriorityTaskWoken = 0;

	xSemaphoreGiveFromISR(instance->xSemaphore, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
#endif

static int pm_msgbox_send(pm_msg_t *instance,
		unsigned char *data, size_t len)
{
	int ret;

	if (!instance->inited) {
		printf ("need to init first.\n");
		return -EPERM;
	}

#if USE_TX_DONE_CALLBACK
	if (pdTRUE != xSemaphoreTake(instance->xSemaphore, 100)) {
		printf ("%s timeout Take xSemaphore\n", __func__);
		return -EBUSY;
	}
#endif

	ret = hal_msgbox_channel_send(instance->edp, data, len);

	return ret;
}


#define MSGBOX_IRQNO     3
#define STANDBY_FREQ     24000000

static int  pm_dsp_standby(void)
{
#ifdef CONFIG_AW_DSPFREQ
	int  ret  = 0;
	int  freq = -1;
#endif
	uint32_t irq_status = 0;


	// freeze others task.
	dbg_save_status_to_rtc(0xabba0000 | __LINE__);
	vTaskSuspendAll();

#ifdef CONFIG_AW_DSPFREQ
	// run in lower frequency
	freq = dsp_get_freq();
	printf("Set frequency: %d Hz to %d Hz.\n", freq, STANDBY_FREQ);
	ret = dsp_set_freq(STANDBY_FREQ);
	if (ret < 0)
		printf("Error: Set frequency, return %d.\n", ret);
#endif

	// disables all interrupts except NMI by PS register.
	dbg_save_status_to_rtc(0xabba0000 | __LINE__);
	xthal_disable_interrupts();

	// get enable interrupts from interrupt controller, INTENABLE register.
	dbg_save_status_to_rtc(0xabba0000 | __LINE__);
	irq_status =  xthal_get_intenable();

	// mark all irq except msgbox by interrupt controller, msgbox irq is 3.
	dbg_save_status_to_rtc(0xabba0000 | __LINE__);
	xthal_set_intenable(irq_status & (0x1<<MSGBOX_IRQNO));

	// enter waiti mode.
	// Interrupts at levels above 2 can wakeup.
	dbg_save_status_to_rtc(0xabba0000 | __LINE__);
	__asm__ volatile("waiti 2\n" ::: "memory");

	dbg_save_status_to_rtc(0xabba0000 | __LINE__);
	xthal_set_intenable(irq_status);

	dbg_save_status_to_rtc(0xabba0000 | __LINE__);
	xthal_enable_interrupts();

#ifdef CONFIG_AW_DSPFREQ
	ret = dsp_set_freq(freq);
	if (ret < 0)
		printf("Error: Set frequency, return %d.\n", ret);
#endif

	dbg_save_status_to_rtc(0xabba0000 | __LINE__);
	xTaskResumeAll();

	dbg_save_status_to_rtc(0xabba0000 | __LINE__);

	return 0;
}

static void pm_msgbox_dotask(void *nouse)
{
	uint32_t data = 0x12345678;

	for (;;) {
		if (pdPASS == xQueueReceive(pm_msg_inst.xQueue, &data, portMAX_DELAY)) {
			printf("[RECV MSG]: 0x%08x\n", data);
			pm_msgbox_send(&pm_msg_inst, (unsigned char *)&data, sizeof(data));

			/* enter waiti mode and wait msg*/
			switch (data) {
			case PM_DSP_POWER_SUSPEND:
				pm_dsp_standby();
				printf("resumed done!...\n");
				break;
			case PM_DSP_POWER_RESUME:
				break;
			default:
				break;
			}
		}
	}
}

int pm_standby_service_init(void)
{
	int ret = 0;

	if (pm_msg_inst.inited) {
		printf("Have been inited.\n");
		goto out;
	}

	pm_msg_inst.xQueue = xQueueCreate(MSGBOX_PM_XQUEUE_LENTH, sizeof(uint32_t));
	if (!pm_msg_inst.xQueue) {
		printf("Failed to create recv queue\n");
		ret = -ENOMEM;
		goto failed_create_queue;
	}

#if USE_TX_DONE_CALLBACK
	pm_msg_inst.xSemaphore = xSemaphoreCreateMutex();
	if (!pm_msg_inst.xSemaphore) {
		printf("Failed to create Mutex\n");
		ret = -ENOMEM;
		goto failed_create_mutex;
	}
#endif

	pm_edp.rec     = pm_msg_recv_callback;
#if USE_TX_DONE_CALLBACK
	pm_edp.tx_done = pm_msg_send_callback;
#endif
	pm_edp.private = &pm_msg_inst;
	ret = hal_msgbox_alloc_channel(&pm_edp,
				MSGBOX_PM_REMOTE,
				MSGBOX_PM_RECV_CHANNEL,
				MSGBOX_PM_SEND_CHANNEL);
	if (ret)
		goto failed_alloc_channel;

	ret = xTaskCreate( pm_msgbox_dotask,
			"pm",
			(1 * 1024)/sizeof(StackType_t),
			&pm_msg_inst,
			3, // prio
			(TaskHandle_t * const)&pm_msg_inst.xHandle);

	if (ret != pdPASS)
		goto failed_create_task;

	pm_msg_inst.edp = &pm_edp;
	pm_msg_inst.inited = 1;
out:
	dbg_save_status_to_rtc(0xabba0000);
	printf("%s build at: %s %s\n", __func__, __DATE__, __TIME__);
	return ret;

failed_create_task:
failed_alloc_channel:
	vSemaphoreDelete(pm_msg_inst.xSemaphore);
#if USE_TX_DONE_CALLBACK
failed_create_mutex:
#endif
	vQueueDelete(pm_msg_inst.xQueue);
failed_create_queue:
	return ret;
}



