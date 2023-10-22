#include <errno.h>
#include <io.h>

#include "pm_adapt.h"
#include "pm_debug.h"
#include "pm_suspend.h"
#include "pm_notify.h"
#include "pm_wakelock.h"
#include "pm_subsys.h"
#include "pm_task.h"
#include "pm_rpcfunc.h"

#undef   PM_DEBUG_MODULE
#define  PM_DEBUG_MODULE  PM_DEBUG_M33

int dsp_notify_cb(suspend_mode_t mode, pm_event_t event, void *arg)
{
	int ret = 0;

	switch (event) {
	case PM_EVENT_SYS_PERPARED:
	case PM_EVENT_SYS_FINISHED:
		ret = pm_msgtodsp_trigger_notify(mode, event);
		break;
	default:
		break;
	}

	return ret;
}

static pm_notify_t dsp_notify = {
	.name = "dsp_notify",
	.pm_notify_cb = dsp_notify_cb,
	.arg = NULL,
};

static pm_subsys_t dsp_subsys = {
	.name = "dsp_subsys",
	.status = PM_SUBSYS_STATUS_NORMAL,
	.xQueue_Signal = NULL,
	.xQueue_Result = NULL,
	.xHandle_Signal = NULL,
	.xHandle_Watch = NULL,
};

static int dsp_trigger_suspend(suspend_mode_t mode)
{
	return pm_msgtodsp_trigger_suspend(mode);
}

#define PM_PMU_REG_BASE				(0x40051400)
#define PM_SYS_LP_STATUS_REG		(PM_PMU_REG_BASE + 0x0104)
#define PM_SYS_LP_STATUS_DSP_ALIVE	(0x1 << 12)

#define GPRCM_DSP_BOOT_FLAG_REG	(0x40050000 + 0x01cc)
#define GPRCM_DSP_BOOT_ADDR_REG	(0x40050000 + 0x01d0)
#define GPRCM_DSP_BOOT_ARG_REG	(0x40050000 + 0x01d4)
extern int sun20i_boot_dsp_with_start_addr(uint32_t dsp_start_addr);
extern int __sun20i_boot_dsp_with_start_addr(uint32_t dsp_start_addr);
int pm_boot_dsp(void)
{
	if (readl(PM_SYS_LP_STATUS_REG) & PM_SYS_LP_STATUS_DSP_ALIVE)
	{
		pm_log("---DSP alive, skip boot---\n");
		return 0;
	}

	uint32_t boot_addr = readl(GPRCM_DSP_BOOT_ADDR_REG);
	pm_log("---boot dsp with start_addr: 0x%08x---\n", boot_addr);
	return __sun20i_boot_dsp_with_start_addr(boot_addr);
}

#define GPRCM_SYS_PRIV_REG1	(0x40050204)
#define DSP_SUSPEND_OK		(PM_SUBSYS_ACTION_OK_FLAG)
#define DSP_SUSPEND_FAILED	(PM_SUBSYS_ACTION_FAILED_FLAG)
#define DSP_RESUME_TRIGGER	(0x0)
#define DSP_RESUME_OK		(0x16aa0000)
int dsp_trigger_resume(suspend_mode_t mode)
{
	int ret;

	ret = pm_boot_dsp();
	if (ret)
		pm_err("resume boot dsp failed\n");
	else
		writel(DSP_RESUME_TRIGGER, GPRCM_SYS_PRIV_REG1);
}

static void dsp_subsys_do_signal(void *arg)
{
	int ret;
	pm_subsys_msg_t msg;
	BaseType_t xReturned;
	pm_subsys_t *subsys = (pm_subsys_t *) arg;

	while(1) {
		if (pdPASS == xQueueReceive(subsys->xQueue_Signal, &msg, portMAX_DELAY)) {
			switch (msg.action) {
			/* suspend signal */
			case PM_SUBSYS_ACTION_TO_SUSPEND:
				if (PM_SUBSYS_STATUS_NORMAL != subsys->status) {
					pm_warn("SUSPEND: subsys(%s) status is %d, will not be suspened.\n",
						subsys->name ? subsys->name : "NULL", subsys->status);
					msg.action = PM_SUBSYS_RESULT_NOTHING;
					pm_subsys_report_result(PM_SUBSYS_ID_DSP, &msg);
					break;
				}
				subsys->status = PM_SUBSYS_STATUS_SUSPENDING;
				ret = dsp_trigger_suspend(msg.mode);
				if (ret) {
					subsys->status = PM_SUBSYS_STATUS_NORMAL;
					msg.action = PM_SUBSYS_RESULT_SUSPEND_REJECTED;
					pm_subsys_report_result(PM_SUBSYS_ID_DSP, &msg);
					pm_err("subsys(%s) trigger suspend(%s) failed(%d).\n",
							subsys->name ? subsys->name : "NULL",
							pm_mode2string(msg.mode),
							ret);
				}
				break;
			/* resume signal */
			case PM_SUBSYS_ACTION_TO_RESUME:
				if (PM_SUBSYS_STATUS_SUSPENDED != subsys->status) {
					pm_warn("RESUME: subsys(%s) status is %d, will not be resumed.\n",
						subsys->name ? subsys->name : "NULL", subsys->status);
					msg.action = PM_SUBSYS_RESULT_NOTHING;
					pm_subsys_report_result(PM_SUBSYS_ID_DSP, &msg);
					break;
				}
				subsys->status = PM_SUBSYS_STATUS_RESUMING;
				ret = dsp_trigger_resume(msg.mode);
				if (ret) {
					pm_err("subsys(%s) trigger resume(%s) failed(%d).\n",
							subsys->name ? subsys->name : "NULL",
							pm_mode2string(msg.mode),
							ret);
				}
				break;
			case PM_SUBSYS_ACTION_KEEP_AWAKE:
				subsys->status = PM_SUBSYS_STATUS_KEEP_AWAKE;
				msg.action = PM_SUBSYS_RESULT_NOTHING;
				pm_subsys_report_result(PM_SUBSYS_ID_DSP, &msg);
				pm_warn("subsys(%s) keeps awake\n", subsys->name ? subsys->name : "NULL");
				break;
			case PM_SUBSYS_ACTION_TO_NORMAL:
				subsys->status = PM_SUBSYS_STATUS_NORMAL;
				msg.action = PM_SUBSYS_RESULT_NOTHING;
				pm_subsys_report_result(PM_SUBSYS_ID_DSP, &msg);
				pm_log("subsys(%s) change status to normal\n", subsys->name ? subsys->name : "NULL");
				break;
			default:
				break;
			}
		}
	}
}

static pm_subsys_result_t dsp_check_to_suspend_result(void)
{
	if (readl(GPRCM_SYS_PRIV_REG1) == DSP_SUSPEND_OK) {
		writel(PM_SUBSYS_ACTION_HAS_GOT_RESULT, GPRCM_SYS_PRIV_REG1);
		return PM_SUBSYS_RESULT_SUSPEND_OK;
	}

	/* failed */
	if (readl(GPRCM_SYS_PRIV_REG1) == DSP_SUSPEND_FAILED) {
		writel(PM_SUBSYS_ACTION_HAS_GOT_RESULT, GPRCM_SYS_PRIV_REG1);
		return PM_SUBSYS_RESULT_SUSPEND_FAILED;
	}

	return PM_SUBSYS_RESULT_NOP;
}

static pm_subsys_result_t dsp_check_to_resume_result(void)
{
	if (readl(GPRCM_SYS_PRIV_REG1) == DSP_RESUME_OK) {
		writel(PM_SUBSYS_ACTION_HAS_GOT_RESULT, GPRCM_SYS_PRIV_REG1);
		return PM_SUBSYS_RESULT_RESUME_OK;
	}

	/* failed */

	return PM_SUBSYS_RESULT_NOP;
}

static void dsp_subsys_do_watch(void *arg)
{
	int ret;
	pm_subsys_msg_t msg;
	pm_subsys_t *subsys = (pm_subsys_t *) arg;
	const TickType_t xFrequency = 10;
	TickType_t xLastWakeTime;
	
	while (1) {
		xLastWakeTime = xTaskGetTickCount();
		vTaskDelayUntil( &xLastWakeTime, xFrequency );

		switch (subsys->status) {
		case PM_SUBSYS_STATUS_SUSPENDING:
			ret = dsp_check_to_suspend_result();
			switch (ret)  {
			case PM_SUBSYS_RESULT_SUSPEND_OK:
				subsys->status = PM_SUBSYS_STATUS_SUSPENDED;
				msg.action = PM_SUBSYS_RESULT_SUSPEND_OK;
				pm_subsys_report_result(PM_SUBSYS_ID_DSP, &msg);
				break;
			case PM_SUBSYS_RESULT_SUSPEND_FAILED:
				subsys->status = PM_SUBSYS_STATUS_NORMAL;
				msg.action = PM_SUBSYS_RESULT_SUSPEND_FAILED;
				pm_subsys_report_result(PM_SUBSYS_ID_DSP, &msg);
				break;
			default:
				break;
			}
			break;
		case PM_SUBSYS_STATUS_RESUMING:
			ret = dsp_check_to_resume_result();
			switch (ret)  {
			case PM_SUBSYS_RESULT_RESUME_OK:
				subsys->status = PM_SUBSYS_STATUS_NORMAL;
				msg.action = PM_SUBSYS_RESULT_RESUME_OK;
				pm_subsys_report_result(PM_SUBSYS_ID_DSP, &msg);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
}

int dsp_subsys_init(void)
{
	int ret = 0;
	BaseType_t xReturned;
	pm_subsys_t *subsys = &dsp_subsys;

	subsys->xQueue_Signal = xQueueCreate(1, sizeof( pm_subsys_msg_t ));
	if (NULL == subsys->xQueue_Signal) {
		pm_err("Create dsp subsys Signal xQueue failed.\n");
		ret = -EPERM;
		goto err_xQueue_Signal;
	}

	subsys->xQueue_Result = xQueueCreate(1, sizeof( pm_subsys_msg_t ));
	if (NULL == subsys->xQueue_Result) {
		pm_err("Create dsp subsys Result xQueue failed.\n");
		ret = -EPERM;
		goto err_xQueue_Result;
	}

	xReturned = xTaskCreate(
			dsp_subsys_do_signal,
			"dsp_subsys_signal",
			(1 * 1024) / sizeof(StackType_t),
			(void *)subsys,
			PM_TASK_PRIORITY,
			(TaskHandle_t * const)&subsys->xHandle_Signal);
	if (pdPASS != xReturned) {
		pm_err("create dsp signal thread failed\n");
		ret = -EPERM;
		goto err_xHandle_Signal;
	}

	xReturned = xTaskCreate(
			dsp_subsys_do_watch,
			"dsp_subsys_watch",
			(1 * 1024) / sizeof(StackType_t),
			(void *)subsys,
			PM_TASK_PRIORITY,
			(TaskHandle_t * const)&subsys->xHandle_Watch);

	if (pdPASS != xReturned) {
		pm_err("create dsp watch thread failed\n");
		ret = -EPERM;
		goto err_xHandle_Watch;
	}

	/* the task can not freeze. */
	ret = pm_task_register(subsys->xHandle_Signal, PM_TASK_TYPE_PM);
	if (ret) {
		pm_err("register dsp signal pm_task failed\n");
		goto err_Register_Signal_task;
	}

	ret = pm_task_register(subsys->xHandle_Watch, PM_TASK_TYPE_PM);
	if (ret) {
		pm_err("register dsp watch pm_task failed\n");
		goto err_Register_Watch_task;
	}

	/* register subsys driver to pm*/
	ret = pm_subsys_register(PM_SUBSYS_ID_DSP, subsys);
	if (ret) {
		pm_err("register dsp subsys failed\n");
		goto err_Register_subsys_dsp;
	}

	goto out;

err_Register_subsys_dsp:
	pm_task_unregister(subsys->xHandle_Watch);
err_Register_Watch_task:
	pm_task_unregister(subsys->xHandle_Signal);
err_Register_Signal_task:
	vTaskDelete(subsys->xHandle_Watch);
err_xHandle_Watch:
	vTaskDelete(subsys->xHandle_Signal);
err_xHandle_Signal:
	vQueueDelete(subsys->xQueue_Result);
err_xQueue_Result:
	vQueueDelete(subsys->xQueue_Signal);
err_xQueue_Signal:
out:
	return ret;
}

int pm_dsp_init(void)
{
	pm_notify_register(&dsp_notify);

	dsp_subsys_init();

	return 0;
}
