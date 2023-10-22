#include <string.h>
#include <osal/hal_interrupt.h>
#include <errno.h>
#include <hal/aw_list.h>
#include <osal/hal_timer.h>
#include <osal/hal_mem.h>
#include <hal_atomic.h>
#include <osal/hal_timer.h>

#include <pm_adapt.h>
#include <pm_debug.h>
#include <pm_task.h>
#include <pm_wakelock.h>

#undef   PM_DEBUG_MODULE
#define  PM_DEBUG_MODULE  PM_DEBUG_TASK

struct pm_task_timer
{
	struct pm_task *task;
	osal_timer_t timer;
	struct list_head node;
};

static struct list_head pm_task_timer_list = LIST_HEAD_INIT(pm_task_timer_list);
#ifndef CONFIG_ARCH_DSP
static HAL_SPIN_LOCK_INIT(pm_task_timer_lock);
#endif

static int             freeze_app_number;
static struct pm_task  pm_task_array[PM_TASK_MAX];
static struct pm_task  pm_task_app_array[PM_TASK_TOTAL_MAX];
static TaskStatus_t    pxTaskStatusArray[PM_TASK_TOTAL_MAX];

#if 0
static TimerHandle_t tsk_timer;
#endif

static struct pm_task *pm_task_check_inarray(TaskHandle_t xHandle)
{
	int i = 0;
        //uint32_t flags;
        struct pm_task   *ptr  = NULL;

	/* check exist in list.*/
	//flags = hal_interrupt_disable_irqsave();

	for (i=0; i<PM_TASK_MAX; i++) {
		ptr = &pm_task_array[i];
        	if (ptr && (ptr->status) && (ptr->xHandle == xHandle))
        	        goto out;
	}
	ptr = NULL;

out:
	//hal_interrupt_enable_irqrestore(flags);
        return ptr;
}

static struct pm_task *pm_task_find_space(void)
{
	int i = 0;
        //uint32_t flags;
        struct pm_task   *ptr  = NULL;

	/* check exist in list.*/
	//flags = hal_interrupt_disable_irqsave();

	for (i=0; i<PM_TASK_MAX; i++) {
		ptr = &pm_task_array[i];
        	if (ptr && !(ptr->status))
        	        goto out;
	}
	ptr = NULL;

out:
	//hal_interrupt_enable_irqrestore(flags);
        return ptr;
}

#if 0
//#include "FreeRTOS_POSIX/pthread.h"
extern  void *pthread_getxtaskhandle(pthread_t thread);
int pm_pthread_register(pthread_t thread, pm_task_type_t type)
{
	TaskHandle_t xHandle = pthread_getxtaskhandle(thread);
	return pm_task_register(xHandle, type);
}

int pm_pthread_unregister(pthread_t thread)
{
	TaskHandle_t xHandle = pthread_getxtaskhandle(thread);
	return pm_task_unregister(xHandle);
}
#endif

int pm_task_register(TaskHandle_t xHandle, pm_task_type_t type)
{
        //uint32_t flags;
	struct pm_task *ptr = NULL;


	if (!xHandle || !pm_task_type_notapp(type)) {
		pm_invalid();
		return -EINVAL;
	}

	ptr = pm_task_check_inarray(xHandle);
	if (ptr) {
		pm_err("Task %s(%p) register repeat.\n", \
				pcTaskGetName(xHandle), xHandle);
		return -EPERM;
	}

	//flags = hal_interrupt_disable_irqsave();
	ptr   = pm_task_find_space();
	if (ptr) {
		ptr->xHandle = xHandle;
		ptr->type    = type;
		ptr->status  = 1;
	}
	//hal_interrupt_enable_irqrestore(flags);

	pm_dbg("Task %s(%p) register ok.\n", \
			pcTaskGetName(xHandle), xHandle);

	return (ptr) ? 0 : -ENOMEM;
}

int pm_task_unregister(TaskHandle_t xHandle)
{
	struct pm_task *ptr = NULL;
        //uint32_t flags;

	if (!xHandle) {
		pm_invalid();
		return -EINVAL;
	}

	//flags = hal_interrupt_disable_irqsave();
	ptr = pm_task_check_inarray(xHandle);
	if (ptr) {
		ptr->xHandle = NULL;
		ptr->type    = 0;
		ptr->status  = 0;
	} else {
		pm_err("Task %s(%p) unregister not found.\n", \
				pcTaskGetName(xHandle), xHandle);
		//hal_interrupt_enable_irqrestore(flags);
		return -EPERM;
	}

	pm_dbg("Task %s(%p) unregister done.\n", \
			pcTaskGetName(xHandle), xHandle);
	//hal_interrupt_enable_irqrestore(flags);
	return 0;
}


#if 0
static const char *task_string[] = {"eRunning", "eReady", "eBlocked", "eSuspended", "eDeleted", "eInvalid"};
static const char *taskstatus2str(int ts)
{
	return task_string[ts];
}

static int pm_task_show_task_status(void)
{
	int i = 0;
	UBaseType_t uxArraySize = 0;
	TaskStatus_t *ptr = NULL;
	TaskHandle_t *tmpxHandle;

	memset(pxTaskStatusArray, 0,  PM_TASK_TOTAL_MAX * sizeof( TaskStatus_t ));
	uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, PM_TASK_TOTAL_MAX, NULL );
	pm_dbg("uxArraySize: %d\n", uxArraySize);

	printf("xHandle  Name             xstatus  TaskNum   cprio    bprio    rtcnt\n");
	for (i = 0; i < uxArraySize; i++) {
		ptr = &pxTaskStatusArray[i];
		tmpxHandle = &ptr->xHandle;
		printf("0x%8p %16s %8s %8d %8d %8d %8d\n",
				tmpxHandle,
				ptr->pcTaskName?ptr->pcTaskName:"NULL",
				taskstatus2str(ptr->eCurrentState),
				ptr->xTaskNumber,
				ptr->uxCurrentPriority,
				ptr->uxBasePriority,
				ptr->ulRunTimeCounter );
	}

	return 0;
}
#endif

static int pm_task_update_apptask(void)
{
	int i = 0, ipos = 0;
	//uint32_t flags;
	UBaseType_t uxArraySize = 0;
	struct pm_task *ptr = NULL;

	//flags = hal_interrupt_disable_irqsave();

#if 0
	pm_dbg("Run %s at %d.\n", __func__, __LINE__);
	uxArraySize = uxTaskGetNumberOfTasks();
	if (uxArraySize > PM_TASK_TOTAL_MAX) {
		goto err;
	}
#endif

	memset(pxTaskStatusArray, 0,  PM_TASK_TOTAL_MAX * sizeof( TaskStatus_t ));
	uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, PM_TASK_TOTAL_MAX, NULL );
	pm_dbg("uxArraySize: %d\n", uxArraySize);

	memset(pm_task_app_array, 0, PM_TASK_TOTAL_MAX * sizeof(pm_task_app_array[0]));

	for (i = 0; i < uxArraySize; i++) {
		switch (pxTaskStatusArray[i].eCurrentState) {
		case eRunning:
		case eReady:
		case eBlocked:
			ptr = pm_task_check_inarray(pxTaskStatusArray[i].xHandle);
			if (!ptr) {
				pm_task_app_array[ipos].type    = PM_TASK_TYPE_APP;
				pm_task_app_array[ipos].xHandle = pxTaskStatusArray[i].xHandle;
				pm_task_app_array[ipos].status  = 1;
				ipos ++;
			}
			break;
		case eSuspended:
		case eDeleted:
		case eInvalid:
		default:
			break;
		}

		if (ipos >= PM_TASK_TOTAL_MAX) {
			goto err;
		}
	}

	//hal_interrupt_enable_irqrestore(flags);
	return ipos;

err:
	pm_err("Task number(%d) more than max limit(%d).\n",\
		uxArraySize, PM_TASK_TOTAL_MAX);
	//hal_interrupt_enable_irqrestore(flags);
	return -ENOMEM;
}


int pm_task_freeze(pm_task_type_t type)
{
	int i = 0;
	int ret = 0;
        unsigned long flags;
	struct pm_task *ptr = NULL;
	int    uxArraySize  = 0;

	pm_log("Task Type(%d) freezing...\n", type);

	pm_wakelocks_block_hold_mutex();

	flags = hal_interrupt_disable_irqsave();
	switch (type) {
	case PM_TASK_TYPE_PM:
	case PM_TASK_TYPE_SYS:
	case PM_TASK_TYPE_DSP:
	case PM_TASK_TYPE_RISCV:
	case PM_TASK_TYPE_BT:
	case PM_TASK_TYPE_WLAN:
		for (i = 0; i < PM_TASK_MAX; i++) {
			ptr = &pm_task_array[i];
			if (ptr->status && (ptr->type == type)) {
				pm_dbg("Task %s(%p) suspend.\n",\
					pcTaskGetName(ptr->xHandle), ptr->xHandle);
				vTaskSuspend(ptr->xHandle);
			}
		}
		break;
	case PM_TASK_TYPE_APP:
		uxArraySize = pm_task_update_apptask();
		freeze_app_number = uxArraySize;
		for (i = 0; i < uxArraySize; i++) {
			ptr = &pm_task_app_array[i];
			if (ptr && (ptr->type == type)) {
				pm_dbg("Task %s(%p) suspend.\n",\
					pcTaskGetName(ptr->xHandle), ptr->xHandle);
				vTaskSuspend(ptr->xHandle);
			}
		}

#ifndef CONFIG_ARCH_DSP
		/* task restore at pm main procession */
		if (osal_timer_stop_all()) {
			pm_err("timer stop failed, suspend abort.\n");
			ret = -EFAULT;
		}
#endif

		break;
	default:
		pm_invalid();
		ret = -EINVAL;
		break;
	}
	hal_interrupt_enable_irqrestore(flags);

	pm_wakelocks_give_mutex();

	return 0;
}

int pm_task_restore(pm_task_type_t type)
{
	int i = 0;
	int ret = 0;
        unsigned long flags;
	struct pm_task *ptr = NULL;
	int    uxArraySize  = 0;

	pm_log("Task Type(%d) restoring.\n", type);

	flags = hal_interrupt_disable_irqsave();
	switch (type) {
	case PM_TASK_TYPE_PM:
	case PM_TASK_TYPE_SYS:
	case PM_TASK_TYPE_DSP:
	case PM_TASK_TYPE_RISCV:
	case PM_TASK_TYPE_BT:
	case PM_TASK_TYPE_WLAN:
		for (i = 0; i < PM_TASK_MAX; i++) {
			ptr = &pm_task_array[i];
			if (ptr->status && (ptr->type == type)) {
				pm_dbg("Task %s(%p) restore.\n",\
					pcTaskGetName(ptr->xHandle), ptr->xHandle);
				vTaskResume(ptr->xHandle);
			}
		}
		break;
	case PM_TASK_TYPE_APP:
#ifndef CONFIG_ARCH_DSP
		osal_timer_start_all();
#endif

		if (freeze_app_number > 0 && freeze_app_number < PM_TASK_TOTAL_MAX)
			uxArraySize  = freeze_app_number;
		else {
			pm_warn("skip restore app with a count(%d)\n", freeze_app_number);
			break;
		}

		for (i = 0; i < uxArraySize; i++) {
			ptr = &pm_task_app_array[i];
			if (ptr && (ptr->type == type)) {
				pm_dbg("Task %s(%p) restore.\n",\
					pcTaskGetName(ptr->xHandle), ptr->xHandle);
				vTaskResume(ptr->xHandle);
			}
		}

		break;
	default:
		pm_invalid();
		ret = -EINVAL;
		break;
	}
	hal_interrupt_enable_irqrestore(flags);

	return 0;

}

int pm_task_attach_timer(osal_timer_t timer, TaskHandle_t xHandle, uint32_t attach)
{
#ifndef CONFIG_ARCH_DSP
	unsigned long flags;
	struct list_head *list_node = NULL;
	struct list_head *list_save = NULL;
	struct list_head *list = &pm_task_timer_list;
	struct pm_task_timer *task_timer = NULL;
	struct pm_task *task = NULL;

	if (!timer)
		return -EINVAL;


	if (xHandle) {
		task = pm_task_check_inarray(xHandle);
		if (!task)
			return -ENODEV;
	}

	list_for_each_safe(list_node, list_save, list) {
		task_timer = __containerof(list_node, struct pm_task_timer, node);
		if (task_timer && (task_timer->timer == timer))
			break;
		task_timer = NULL;
	}

	if (!task_timer) {
		if (attach) {
			task_timer = hal_malloc(sizeof(struct pm_task_timer));
			if (!task_timer)
				return -ENOMEM;

			memset(task_timer, 0, sizeof(struct pm_task_timer));

			flags = hal_spin_lock_irqsave(&pm_task_timer_lock);
			task_timer->task = task;
			task_timer->timer = timer;
			list_add_tail(&task_timer->node, &pm_task_timer_list);
			hal_spin_unlock_irqrestore(&pm_task_timer_lock, flags);
			return osal_timer_remain(timer, attach);
		}
	} else if (!attach) {
		flags = hal_spin_lock_irqsave(&pm_task_timer_lock);
		task_timer->task = NULL;
		task_timer->timer = NULL;
		list_del(&task_timer->node);
		hal_spin_unlock_irqrestore(&pm_task_timer_lock, flags);
		hal_free(task_timer);
		return osal_timer_remain(timer, attach);
	} else {
		flags = hal_spin_lock_irqsave(&pm_task_timer_lock);
		task_timer->task = task;
		task_timer->timer = timer;
		hal_spin_unlock_irqrestore(&pm_task_timer_lock, flags);
		return osal_timer_remain(timer, attach);
	}
#endif

	return -EINVAL;
}


#if 0
static void tsk_timer_callback( TimerHandle_t xTimer )
{
	//pm_task_show_task_status();
	printf("$\n");
}
#endif

#define TASK_IDLE_NAME  "IDLE"
#define TASK_TIMR_NAME  "Tmr Svc"
int pm_task_init(void)
{
	int i;

	/* protect Idle/Timer task */
	TaskHandle_t  xtask;
	const char *sys_task[] = {TASK_IDLE_NAME, TASK_TIMR_NAME};
	for (i=0; i<sizeof(sys_task)/sizeof(sys_task[0]); i++) {
		xtask = xTaskGetHandle(sys_task[i]);
		if (NULL == xtask) {
			pm_err("Can't find task(%s)\n", sys_task[i]);
			continue;
		}
		pm_task_register(xtask, PM_TASK_TYPE_SYS);
	}

#if 0
        tsk_timer = xTimerCreate("pm_tsk_timer",
                                    OS_MSecsToTicks(500),
                                    pdTRUE,
                                    NULL,
                                    tsk_timer_callback);

        xTimerStart(tsk_timer, 10);
#endif

	return 0;
}

int pm_task_exit(void)
{
	return 0;
}

