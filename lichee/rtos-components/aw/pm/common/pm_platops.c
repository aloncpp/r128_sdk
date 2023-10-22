#ifdef CONFIG_KERNEL_FREERTOS
#include <FreeRTOS.h>
#else
#error "PM do not support the RTOS!!"
#endif
#include <task.h>
#include <errno.h>

#include "pm_debug.h"
#include "pm_base.h"
#include "pm_platops.h"

#undef  PM_DEBUG_MODULE
#define PM_DEBUG_MODULE  PM_DEBUG_PLATOPS

static suspend_ops_t *suspend_ops = NULL;

int pm_platops_register(suspend_ops_t *ops)
{
	pm_dbg("platops: register ops %s(%p) ok\n",
			(ops && ops->name)?ops->name:"UNKOWN", ops);

	suspend_ops = ops;

	return 0;
}

int pm_platops_call(suspend_ops_type_t type, suspend_mode_t mode)
{
	int ret = 0;

	//pm_trace_func("%d, %d", type, mode);

	if (!suspend_ops || !pm_suspend_mode_valid(mode)) {
		pm_invalid();
		return -EINVAL;
	}

#if 0
	/* check suspend*/
	ret = pm_suspend_assert();
	if (ret)
		return ret;
#endif

	switch (type) {
	case PM_SUSPEND_OPS_TYPE_VALID:
		if (suspend_ops->valid)
			ret = suspend_ops->valid(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_PRE_BEGIN:
		if (suspend_ops->pre_begin)
			ret = suspend_ops->pre_begin(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_BEGIN:
		if (suspend_ops->begin)
			ret = suspend_ops->begin(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_PREPARE:
		if (suspend_ops->prepare)
			ret = suspend_ops->prepare(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_PREPARE_LATE:
		if (suspend_ops->prepare_late)
			ret = suspend_ops->prepare_late(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_ENTER:
		if (suspend_ops->enter)
			ret = suspend_ops->enter(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_WAKE:
		if (suspend_ops->wake)
			ret = suspend_ops->wake(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_FINISH:
		if (suspend_ops->finish)
			ret = suspend_ops->finish(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_END:
		if (suspend_ops->end)
			ret = suspend_ops->end(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_RECOVER:
		if (suspend_ops->recover)
			ret = suspend_ops->recover(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_AGAIN:
		if (suspend_ops->again)
			ret = suspend_ops->again(mode);
		break;
	case PM_SUSPEND_OPS_TYPE_AGAIN_LATE:
		if (suspend_ops->again_late)
			ret = suspend_ops->again_late(mode);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

