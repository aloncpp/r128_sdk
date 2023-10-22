
#include <stdlib.h>
#include <string.h>
#include <osal/hal_interrupt.h>

#include <errno.h>

#include "pm_adapt.h"
#include "pm_debug.h"
#include "pm_suspend.h"
#include "pm_wakecnt.h"
#include "pm_wakesrc.h"
#include "pm_wakelock.h"
#include "pm_testlevel.h"
#include "pm_devops.h"
#include "pm_syscore.h"
#include "pm_notify.h"
#include "pm_task.h"
#include "pm_platops.h"
#include "pm_subsys.h"
#include "pm_rpcfunc.h"

/*
 * The following functions are called by other modules,
 * So arguments should be of the full type.
 * Check the parameter conversion here.
 *  pointer,enum etc  -> int
 */
#ifdef CONFIG_AMP_PMOFM33_STUB
int pm_set_wakesrc(int id, int core, int status)
{
	return rpc_pm_set_wakesrc(id, core, status);
}

int pm_trigger_suspend(suspend_mode_t mode)
{
	return rpc_pm_trigger_suspend((int)mode);
}

int pm_report_subsys_action(int subsys_id, int action)
{
	return rpc_pm_report_subsys_action(subsys_id, action);
}

int pm_subsys_soft_wakeup(int affinity, int irq, int action)
{
	return rpc_pm_subsys_soft_wakeup(affinity, irq, action);
}
#endif

/*
 * The following functions are called by the AMP-RPC module,
 * and the argument should be int.
 * Check the parameter conversion here.
 *  int  ->  pointer,enum etc
 */
#ifdef CONFIG_AMP_PMOFRV_SERVICE
int rpc_pm_wakelocks_getcnt_riscv(int stash)
{
	return pm_wakelocks_refercnt(!!stash);
}

int rpc_pm_msgtorv_trigger_notify(int mode, int event)
{
	int ret = 0;

	ret = pm_notify_event(mode, event);
	if (ret) {
		pm_err("%s(%d): pm notify event %d return failed value: %d\n",
			__func__, __LINE__, event, ret);
		return ret;
	}

	return ret;
}

int rpc_pm_msgtorv_trigger_suspend(int mode)
{
	return pm_suspend_request(mode);
}

int rpc_pm_msgtorv_check_subsys_assert(int mode)
{
	int ret = 0;

	ret = pm_suspend_assert();

	return ret;
}

uint8_t soft_wakesrc_reported = 0;
int rpc_pm_msgtorv_check_wakesrc_num(int type)
{
	int ret = 0;
	int num = 0;

	/* soft wakesrc must be enabled before system suspend entry to stop the suspend trigger */
	num = pm_wakesrc_type_check_num(type);
	if (type == PM_WAKESRC_SOFT_WAKEUP) {
		if (!soft_wakesrc_reported && num) {
			ret = pm_report_subsys_action(PM_SUBSYS_ID_RISCV, PM_SUBSYS_ACTION_KEEP_AWAKE);
			if (ret) {
				pm_err("%s(%d): pm_report_subsys_action failed, return: %d\n", __func__, __LINE__, ret);
			} else
				soft_wakesrc_reported = 1;
		} else if (soft_wakesrc_reported && !num) {
			ret = pm_report_subsys_action(PM_SUBSYS_ID_RISCV, PM_SUBSYS_ACTION_TO_NORMAL);
			if (ret) {
				pm_err("%s(%d): pm_report_subsys_action failed, return: %d\n", __func__, __LINE__, ret);
			} else
				soft_wakesrc_reported = 0;
		}
	}

	return num;
}
#endif

