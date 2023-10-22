#include <FreeRTOS.h>
#include <task.h>
#include <stdlib.h>
#include <string.h>
#include <osal/hal_interrupt.h>
#include <errno.h>
#include <console.h>

#include <pm_adapt.h>
#include <pm_debug.h>
#include <pm_suspend.h>
#include <pm_wakecnt.h>
#include <pm_wakesrc.h>
#include <pm_wakelock.h>
#include <pm_testlevel.h>
#include <pm_devops.h>
#include <pm_syscore.h>
#include <pm_notify.h>
#include <pm_task.h>
#include <pm_platops.h>
#include <pm_subsys.h>
#include <pm_state.h>
#ifdef CONFIG_COMPONENTS_VIRT_LOG
#include <virt_log.h>
#endif
#ifdef CONFIG_ARCH_DSP
#include <aw_io.h>
#endif

#undef   PM_DEBUG_MODULE
#define  PM_DEBUG_MODULE  PM_DEBUG_SUSPEND

#ifndef CONFIG_COMPONENTS_VIRT_LOG
extern void printf_lock(void);
extern void printf_unlock(void);
#endif

static void pm_suspend_task(void *nouse);
static int pm_suspend_devices_and_enter(suspend_mode_t mode);
static int pm_suspend_enter(suspend_mode_t mode);

static suspend_mode_t suspend_mode = PM_MODE_ON;

typedef enum {
	PM_VIRT_LOG_EARLY_STAGE = 0,
	PM_VIRT_LOG_LATE_STAGE,

	PM_VIRT_LOG_STAGE_MAX,
	PM_VIRT_LOG_STAGE_BASE = PM_VIRT_LOG_EARLY_STAGE,
} pm_virt_log_stage_t;
static pm_virt_log_stage_t virt_log_stage = PM_VIRT_LOG_EARLY_STAGE;

struct pm_suspend_stats pm_errno_stats = {
	.name = "pm_errno_stats",
	.last_failed_index = 0,
	.unit_failed = 0,
	.last_failed_errno = 0,
};

/* Standby stage and time record */
void pm_record_stage(uint32_t val, uint8_t cover)
{
	uint32_t rec_reg;
	uint32_t rec_val;

	if (!pm_test_standby_recording())
		return;

#ifdef CONFIG_COMPONENTS_PM_CORE_M33
	rec_reg = PM_STANDBY_STAGE_M33_REC_REG;
#endif

#ifdef CONFIG_COMPONENTS_PM_CORE_RISCV
	rec_reg = PM_STANDBY_STAGE_C906_REC_REG;
#endif

#ifdef CONFIG_COMPONENTS_PM_CORE_DSP
	rec_reg = PM_STANDBY_STAGE_DSP_REC_REG;
#endif

	rec_val = (cover) ? (val) : (readl(rec_reg) | val);
	writel(rec_val, rec_reg);
}

void pm_suspend_mode_change(suspend_mode_t mode)
{
	suspend_mode_t tmp_mode = suspend_mode;

	if (!pm_suspend_mode_valid(mode))
		return;

	suspend_mode = mode;
	pm_warn("suspend_mode(%d) changes to mode(%d)\n", tmp_mode, suspend_mode);
}

static void pm_virt_log(int virt)
{
	if (virt) {
#ifdef CONFIG_COMPONENTS_VIRT_LOG
		virt_log_enable(1);
#else
		printf_lock();
#endif
	} else {
#ifdef CONFIG_COMPONENTS_VIRT_LOG
		virt_log_enable(0);
#else
		printf_unlock();
#endif
	}
}

static int cmd_pm_set_virt_log(int argc, char **argv)
{
	if ((argc != 2) || (atoi(argv[1]) < PM_VIRT_LOG_STAGE_BASE) || (atoi(argv[1]) >= PM_VIRT_LOG_STAGE_MAX)) {
		pm_err("%s: Invalid params for pm_set_virt_log\n", __func__);
		return -EINVAL;
	}

	virt_log_stage = atoi(argv[1]);

	pm_warn("PM set virt_log_stage to %d\n", virt_log_stage);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_pm_set_virt_log, pm_set_virt_log, pm_test_tools)

static struct pm_suspend_thread_t *pm_suspend_thread;
int  pm_suspend_init(void)
{
	BaseType_t xReturned;

	if (pm_suspend_thread) {
		pm_err("thread start again\n");
		return -EPERM;
	}

	pm_suspend_thread = malloc(sizeof(struct pm_suspend_thread_t));
	if (!pm_suspend_thread) {
		pm_err("thread malloc failed\n");
		return -EPERM;
	}

	memset(pm_suspend_thread, 0, sizeof(struct pm_suspend_thread_t));

	pm_suspend_thread->xQueue = xQueueCreate( 1, sizeof( uint32_t ) );
	if (NULL == pm_suspend_thread->xQueue) {
		pm_err("create queue failed\n");
		return -EPERM;
	}

	xReturned = xTaskCreate(pm_suspend_task, "pm_suspend", (10 * 1024) / sizeof(StackType_t), NULL,
		PM_TASK_PRIORITY, (TaskHandle_t * const)&pm_suspend_thread->xHandle);
	if (pdPASS != xReturned) {
		pm_err("create thread failed\n");
		return -EPERM;
	}

	/* the task can not freeze. */
	pm_task_register(pm_suspend_thread->xHandle, PM_TASK_TYPE_PM);

	return 0;
}

int pm_suspend_exit(void)
{
	if (!pm_suspend_thread) {
		pm_err("thread stop again\n");
		return -EPERM;
	}

	if (pm_suspend_thread->xHandle)
		vTaskDelete(pm_suspend_thread->xHandle);

	vQueueDelete(pm_suspend_thread->xQueue);

	free(pm_suspend_thread);
	pm_suspend_thread = NULL;

	return 0;
}

static void pm_suspend_task(void *nouse)
{
	int ret;
	uint32_t mode;

	while (1) {
		if (xQueueReceive(pm_suspend_thread->xQueue, &mode, portMAX_DELAY) != pdPASS)
			continue;

		pm_trace_info("suspend begin.");
		pm_state_set(PM_STATUS_SLEEPING);
		suspend_mode = mode;

#ifdef CONFIG_COMPONENTS_PM_CORE_M33
		/* notify system suspend. */
		ret = pm_notify_event(mode, PM_EVENT_SYS_PERPARED);
		if (ret) {
			pm_err("pm entry notify returns return: %d, suspend abort\n", ret);
			ret = 0;
			pm_state_set(PM_STATUS_RUNNING);
			pm_trace_info("suspend end.");
			continue;
		}
#endif
		switch (mode) {
		case PM_MODE_ON:
			break;
		case PM_MODE_SLEEP:
		case PM_MODE_STANDBY:
		case PM_MODE_HIBERNATION:
			ret = pm_suspend_devices_and_enter(mode);
			pm_errno_stats.last_failed_errno = ret;
			pm_report_stats();
			break;
		default:
			pm_err("%s: Undefined suspend mode(%d)\n", __func__, mode);
			break;
		}
#ifdef CONFIG_COMPONENTS_PM_CORE_M33
		pm_notify_event(mode, PM_EVENT_SYS_FINISHED);
#endif

#ifdef CONFIG_COMPONENTS_PM_CORE_RISCV
               if (ret) {
                       writel(PM_SUBSYS_ACTION_FAILED_FLAG, RV_ACTION_REC_REG);
                       pm_err("riscv suspend failed\n");
               } else
                       writel(RV_RESUME_OK, RV_ACTION_REC_REG);
#endif

#ifdef CONFIG_COMPONENTS_PM_CORE_DSP
               if (ret) {
                       writel(PM_SUBSYS_ACTION_FAILED_FLAG, DSP_ACTION_REC_REG);
                       pm_err("dsp suspend failed\n");
               } else
                       writel(DSP_RESUME_OK, DSP_ACTION_REC_REG);
#endif

		pm_state_set(PM_STATUS_RUNNING);
		pm_trace_info("suspend end.");
	}

	return ;
}

static int pm_suspend_devices_and_enter(suspend_mode_t src_mode)
{
	int ret = 0;
	int mode = src_mode;

	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x0, 1);

	/* notify pm prepare event */
	ret = pm_notify_event(mode, PM_EVENT_PERPARED);
	if (ret)
		goto label_pre_begin_out;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x1, 1);

	ret = pm_platops_call(PM_SUSPEND_OPS_TYPE_PRE_BEGIN, mode);
	if (ret)
		goto label_notify_sys_finish;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x2, 1);

	/*freeze all task except PM/WLAN/BT is registered etc...*/
	ret = pm_task_freeze(PM_TASK_TYPE_APP);
	if (ret)
		goto label_restore_app;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x3, 1);

	/*debug*/
	if (pm_suspend_test(PM_SUSPEND_TEST_FREEZER))
		goto label_restore_app;

lable_suspend_again_late:
	if (suspend_mode != mode) {
		pm_warn("suspend mode switch to %d, last mode: %d\n", suspend_mode, mode);
		mode = suspend_mode;
	}

	/* subsys suspend */
	ret = pm_platops_call(PM_SUSPEND_OPS_TYPE_BEGIN, mode);
	if (ret)
		goto label_restore_app;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x4, 1);

	if (suspend_mode != mode) {
		pm_warn("suspend mode switch to %d, last mode: %d\n", suspend_mode, mode);
		mode = suspend_mode;
	}

	if (virt_log_stage == PM_VIRT_LOG_EARLY_STAGE) {
		pm_log("swtich to virtual log\n");
		pm_virt_log(1);
	}

	/*try to suspend some devices.*/
	ret = pm_devops_prepared(mode);
	if (ret)
		goto label_ops_end;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x5, 1);

	ret = pm_devops_suspend(mode);
	if (ret)
		goto label_ops_recover;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x6, 1);

	/*debug*/
	if (pm_suspend_test(PM_SUSPEND_TEST_DEVICE))
		goto label_dev_resume;


	ret = pm_platops_call(PM_SUSPEND_OPS_TYPE_PREPARE, mode);
	if (ret)
		goto label_dev_resume;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x7, 1);

lable_suspend_again:
	ret = pm_suspend_enter(mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x700, 1);
	/*
	 * again when all things occurred:
	 * 1, there isn't an error occurred, such as ret==0.
	 * 2, pm_platops_again think we should again, such as return true.
	 * or resume system whatever:
	 * 1, an error occurred.
	 * 2, pm_platops_again think can resume.
	 */
	if (!ret && pm_platops_call(PM_SUSPEND_OPS_TYPE_AGAIN, mode)) {
		pm_trace_info("suspend again.");
		pm_state_set(PM_STATUS_AGAINING);
		goto lable_suspend_again;
	}

	pm_trace_info("suspend resume.");
	pm_state_set(PM_STATUS_WAKEUPING);
	pm_platops_call(PM_SUSPEND_OPS_TYPE_FINISH, mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x800, 1);

label_dev_resume:
	pm_devops_resume(mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x900, 1);

label_dev_complete:
	pm_devops_complete(mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xa00, 1);

	if (virt_log_stage == PM_VIRT_LOG_EARLY_STAGE) {
		pm_log("restore form virtual log\n");
		pm_virt_log(0);
	}

label_ops_end:
	pm_platops_call(PM_SUSPEND_OPS_TYPE_END, mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xb00, 1);
	if (!ret && pm_platops_call(PM_SUSPEND_OPS_TYPE_AGAIN_LATE, mode)) {
		pm_trace_info("suspend again late.");
		pm_state_set(PM_STATUS_AGAINING);
		goto lable_suspend_again_late;
	}
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xc00, 1);

label_restore_app:
	pm_task_restore(PM_TASK_TYPE_APP);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xd00, 1);
	pm_platops_call(PM_SUSPEND_OPS_TYPE_POST_END, mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xe00, 1);

label_notify_sys_finish:
	pm_notify_event(mode, PM_EVENT_FINISHED);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xf00, 1);

label_pre_begin_out:
	return ret;

label_ops_recover:
	pm_platops_call(PM_SUSPEND_OPS_TYPE_RECOVER, mode);
	goto label_dev_complete;
}


#ifndef CONFIG_COMPONENTS_PM_CORE_DSP
extern void arch_disable_all_irq(void);
extern void arch_enable_all_irq(void);
#endif
static int pm_suspend_enter(suspend_mode_t mode)
{
	int  ret = 0;

	/*there try to suspend all devices.*/
	ret = pm_devops_suspend_late(mode);
	if (ret)
		goto lable_out;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x8, 1);

	/* hal interrupt should be called in pairs */
	hal_interrupt_disable();
#ifndef CONFIG_COMPONENTS_PM_CORE_DSP
	/* Ensure that global interrupt is disabled when poweron.
	 * hal_interrupt_disable() only adjusts the priority in the M33/C906.
	 */
	arch_disable_all_irq();
#endif
	pm_moment_clear((0x1 << PM_MOMENT_IRQ_DISABLE) | (0x1 << PM_MOMENT_CLK_SUSPEND) \
			| (0x1 << PM_MOMENT_CLK_RESUME) | (0x1 << PM_MOMENT_IRQ_ENABLE) );
	pm_moment_record(PM_MOMENT_IRQ_DISABLE, 0);

	/*there try to close all devices irq except wakeupsrc irq.*/
	ret = pm_devops_suspend_noirq(mode);
	if (ret)
		goto lable_enable_interrupt;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x9, 1);

	/*debug*/
	if (pm_suspend_test(PM_SUSPEND_TEST_PLATFORM))
		goto lable_dev_resume_noirq;

	if (virt_log_stage == PM_VIRT_LOG_LATE_STAGE) {
		pm_log("restore form virtual log\n");
		pm_virt_log(1);
	}

	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xa, 1);

	ret = pm_syscore_suspend(mode);
	if (ret)
		goto lable_dev_resume_noirq;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xb, 1);

	ret = pm_platops_call(PM_SUSPEND_OPS_TYPE_PREPARE_LATE, mode);
	if (ret)
		goto lable_syscore_resume;
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0xc, 1);

	/*debug*/
	if (pm_suspend_test(PM_SUSPEND_TEST_CPU))
		goto lable_ops_wake;

	/* no debug and no pending*/
	pm_state_set(PM_STATUS_SLEEPED);
	ret = pm_platops_call(PM_SUSPEND_OPS_TYPE_ENTER, mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x100, 1);


lable_ops_wake:
	pm_state_set(PM_STATUS_ACTIVING);
	pm_platops_call(PM_SUSPEND_OPS_TYPE_WAKE, mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x200, 1);

lable_syscore_resume:
	pm_syscore_resume(mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x300, 1);

	if (virt_log_stage == PM_VIRT_LOG_LATE_STAGE) {
		pm_log("restore form virtual log\n");
		pm_virt_log(0);
	}
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x400, 1);

lable_dev_resume_noirq:
	pm_devops_resume_noirq(mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x500, 1);

lable_enable_interrupt:
	pm_moment_record(PM_MOMENT_IRQ_ENABLE, 0);
#ifndef CONFIG_COMPONENTS_PM_CORE_DSP
	arch_enable_all_irq();
#endif
	hal_interrupt_enable();

	pm_devops_resume_early(mode);
	pm_record_stage(PM_TEST_RECORDING_ENTER | 0x600, 1);

lable_out:
	return ret;
}

int pm_suspend_request(suspend_mode_t mode)
{
	uint32_t xmode = mode;
	BaseType_t xReturned;

	if (pm_platops_call(PM_SUSPEND_OPS_TYPE_VALID, mode))
		return -EINVAL;

	xReturned = xQueueSend(pm_suspend_thread->xQueue, &xmode, 0);

	return (pdPASS == xReturned) ? 0 : -EBUSY;
}


