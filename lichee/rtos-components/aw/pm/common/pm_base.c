
#include <stdlib.h>
#include <string.h>
#include <osal/hal_interrupt.h>
#include <errno.h>

#include "pm_base.h"
#include "pm_debug.h"
#include "pm_suspend.h"
#include "pm_wakesrc.h"
#include "pm_wakecnt.h"
#include "pm_wakelock.h"
#include "pm_wakecnt.h"

#undef  PM_DEBUG_MODULE
#define PM_DEBUG_MODULE  PM_DEBUG_BASE

struct pm_wakeup_ops *pm_wakeup_ops = NULL;

static const char *const_mode_string[] = {
	"PM_MODE_ON",
	"PM_MODE_SLEEP",
	"PM_MODE_STANDBY",
	"PM_MODE_HIBERNATION",
	"PM_MODE_INVALID",
};

const char *pm_mode2string(int mode)
{
	if (pm_suspend_mode_valid(mode))
		return const_mode_string[mode];
	else
		return const_mode_string[PM_MODE_MAX];
}

int pm_wakeup_ops_register(struct pm_wakeup_ops *ops)
{
	if (pm_wakeup_ops) {
		pm_err("(%s) pm_wakeup_ops has been regitstered\n", __func__);
		return -EFAULT;
	}

	pm_wakeup_ops = ops;

	return 0;
}

int pm_hal_get_wakeup_event(uint32_t *event)
{
	if (pm_wakeup_ops && pm_wakeup_ops->get_wakeup_event) {
		*event = pm_wakeup_ops->get_wakeup_event();
		return 0;
	}

	return -ENODEV;
}

int pm_hal_get_wakeup_irq(int *irq)
{
	if (pm_wakeup_ops && pm_wakeup_ops->get_wakeup_irq) {
		*irq = pm_wakeup_ops->get_wakeup_irq();
		return 0;
	}

	return -ENODEV;
}

int pm_hal_record_wakeup_irq(const int irq)
{
	if (pm_wakeup_ops && pm_wakeup_ops->irq_is_wakeup_armed) {
		if (!pm_wakeup_ops->irq_is_wakeup_armed(irq))
			return -EINVAL;

		if (pm_wakeup_ops && pm_wakeup_ops->record_wakeup_irq) {
			pm_wakeup_ops->record_wakeup_irq(irq);
			return 0;
		}
	}
	return -ENODEV;
}

int pm_hal_clear_wakeup_irq(void)
{
	if (pm_wakeup_ops && pm_wakeup_ops->clear_wakeup_irq) {
		pm_wakeup_ops->clear_wakeup_irq();
		return 0;
	}

	return -ENODEV;
}

int pm_hal_set_time_to_wakeup_ms(unsigned int ms)
{
	int ret;

	if (pm_wakeup_ops && pm_wakeup_ops->set_time_to_wakeup_ms) {
		ret = pm_wakeup_ops->set_time_to_wakeup_ms(ms);
		return ret;
	}

	return -ENODEV;
}

int pm_suspend_assert(void)
{
	int ret = 0;

	if (pm_wakeup_ops && pm_wakeup_ops->check_wakeup_event) {
		/* read pending in the noirq environment */
		ret = pm_wakeup_ops->check_wakeup_event();
		if (ret) {
			pm_warn("wakesrc(0x%08x) abort\n", ret);
			goto out;
		}
	}

	ret = pm_wakelocks_refercnt(0);
	if (ret) {
		pm_warn("wakelock(%d) abort\n", ret);
		goto out;
	}

	ret = pm_wakecnt_check();
	if (ret) {
		pm_warn("wakecnt(%d) abort\n", ret);
		goto out;
	}

out:
#if 0
	/* wakeup irq does not mean we must to wakeup.
	 * For example, the interaction with the wlan IC may generate interrupts.
	 */
	if (pm_wakeup_ops && pm_wakeup_ops->check_suspend_abort)
		return (ret || pm_wakeup_ops->check_suspend_abort())?-EBUSY:0;
	else
#endif
		return ret?-EBUSY:0;
}

