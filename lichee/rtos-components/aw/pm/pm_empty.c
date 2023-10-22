#include "include/pm_base.h"
#include "include/pm_devops.h"
#include "include/pm_task.h"
#include "include/pm_wakecnt.h"
#include "include/pm_wakelock.h"
#include "include/pm_wakesrc.h"
#include <osal/hal_timer.h>

/* devops  */
int pm_devops_register(struct pm_device *dev)
{
	return 0;
}
int pm_devops_unregister(struct pm_device *dev)
{
	return 0;
}

/* pm task */
int pm_task_register(TaskHandle_t xHandle, pm_task_type_t type)
{
	return 0;
}

int pm_task_unregister(TaskHandle_t xHandle)
{
	return 0;
}

int pm_task_attach_timer(osal_timer_t timer, TaskHandle_t xHandle, uint32_t attach)
{
	return 0;
}

/* wakecnt */
void pm_wakecnt_inc(int irq) {}

void pm_stay_awake(int irq) {}

void pm_relax(int irq, pm_relax_type_t wakeup) {}

/* wakelock */
void pm_wakelocks_setname(struct wakelock *wl, const char *name) {}

int  pm_wakelocks_acquire(struct wakelock *wl, enum pm_wakelock_t type, uint32_t timeout)
{
	return 0;
}

int  pm_wakelocks_release(struct wakelock *wl)
{
	return 0;
}
void pm_wakelocks_showall(void) {}

/* wakesrc */
int pm_wakesrc_register(const int irq, const char *name, const unsigned int type)
{
	return 0;
}

int pm_wakesrc_unregister(int irq)
{
	return 0;
}

int pm_set_wakeirq(const int irq)
{
	return 0;
}
int pm_clear_wakeirq(const int irq)
{
	return 0;
}

int pm_wakesrc_is_disabled(int irq)
{
	return 0;
}
