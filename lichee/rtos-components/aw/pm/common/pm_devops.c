#include <errno.h>
#include <hal_time.h>
#include <inttypes.h>

#include "pm_base.h"
#include "pm_debug.h"
#include "pm_devops.h"
#include "pm_state.h"
#include "pm_wakesrc.h"
#include "pm_rpcfunc.h"
#include "pm_adapt.h"
#include "pm_init.h"

#undef   PM_DEBUG_MODULE
#define  PM_DEBUG_MODULE  PM_DEBUG_DEVOPS

#define pm_device_containerof(ptr_module) \
        __containerof(ptr_module, struct pm_device, node)

#ifdef CONFIG_ARCH_DSP
uint64_t hal_gettime_ns(void)
{
/* dsp does not implement this interface temporarily */

	return 0;
}
#endif

typedef int (*pm_devcallback_t)(struct pm_device *, suspend_mode_t);
typedef void (*pm_resumecall_t)(suspend_mode_t mode);

uint8_t prohibit_register = 1;
uint8_t devops_trace = 1;

static SemaphoreHandle_t pm_device_mutex = NULL;
static struct list_head pm_device_list = LIST_HEAD_INIT(pm_device_list);
static struct list_head pm_prepared_list = LIST_HEAD_INIT(pm_prepared_list);
static struct list_head pm_suspended_list = LIST_HEAD_INIT(pm_suspended_list);
static struct list_head pm_late_early_list = LIST_HEAD_INIT(pm_late_early_list);
static struct list_head pm_noirq_list = LIST_HEAD_INIT(pm_noirq_list);
static struct list_head* pm_devops_list[PM_DEVOPS_TYPE_SUSPEND_NOIRQ + 2];
static struct list_head pm_device_oldlist = LIST_HEAD_INIT(pm_device_oldlist);

static inline uint32_t pm_find_devlist(pm_devops_type_t type)
{
	return pm_devops_belong_resume(type)?(PM_DEVOPS_TYPE_MAX - type):(type);
}

static pm_resumecall_t pm_resumecall[] = {
	pm_devops_resume_noirq,
	pm_devops_resume_early,
	pm_devops_resume,
	pm_devops_complete
};

static inline pm_resumecall_t pm_find_typecall(pm_devops_type_t type)
{
	return pm_resumecall[type - PM_DEVOPS_TYPE_SUSPEND_NOIRQ - 1];
}

struct pm_suspend_stats pm_devsuspend_stats = {
	.name = "pm_devsuspend_stats",
	.last_failed_index = 0,
	.unit_failed = 0,
	.last_failed_errno = 0,
};

struct pm_suspend_stats pm_devresume_stats = {
	.name = "pm_devresume_stats",
	.last_failed_index = 0,
	.unit_failed = 0,
	.last_failed_errno = 0,
};

static const char *const_devops_type_string[] = {
	"PM_DEVOPS_TYPE_PREPARED",
	"PM_DEVOPS_TYPE_SUSPEND",
	"PM_DEVOPS_TYPE_SUSPEND_LATE",
	"PM_DEVOPS_TYPE_SUSPEND_NOIRQ",
	"PM_DEVOPS_TYPE_RESUME_NOIRQ",
	"PM_DEVOPS_TYPE_RESUME_EARLY",
	"PM_DEVOPS_TYPE_RESUME",
	"PM_DEVOPS_TYPE_COMPLETE",
	"PM_DEVOPS_TYPE_INVALID",
};

const char *pm_devops_type2string(int type)
{
	if (pm_devops_type_valid(type))
		return const_devops_type_string[type];
	else
		return const_devops_type_string[PM_DEVOPS_TYPE_MAX];
}

int pm_devops_init(void)
{
	pm_device_mutex = xSemaphoreCreateMutex();
	pm_report_register(&pm_devsuspend_stats);
	pm_report_register(&pm_devresume_stats);

	pm_devops_list[0] = &pm_device_list;
	pm_devops_list[1] = &pm_prepared_list;
	pm_devops_list[2] = &pm_suspended_list;
	pm_devops_list[3] = &pm_late_early_list;
	pm_devops_list[4] = &pm_noirq_list;

	prohibit_register = 0;

	return 0;
}

int pm_devops_exit(void)
{
	vSemaphoreDelete(pm_device_mutex);

	return 0;
}

static int pm_device_in_list(struct list_head *dev_node)
{
	struct list_head *node = NULL;
	struct list_head *list = &pm_device_list;
	struct list_head *list_save;
	struct pm_device *dev;

	list_for_each_safe(node, list_save, list) {
		dev = pm_device_containerof(node);
		if (dev_node == &dev->node)
			return 1;
	}

	return 0;
}

int pm_devops_register(struct pm_device *dev)
{
	if (prohibit_register)
		return -EBUSY;

	if (!dev || !dev->name) {
		pm_invalid();
		return -EINVAL;
	}

	if (pm_device_in_list(&dev->node)) {
		pm_err("pm device %s(%p) has already registered\n", dev->name, dev);
		return -EINVAL;
	}

	if (pdTRUE !=xSemaphoreTake(pm_device_mutex, portMAX_DELAY)) {
		pm_semapbusy(pm_device_mutex);
		return -EBUSY;
	}

	/* new node must insert pm_device_list head */
	list_add_tail(&dev->node, &pm_device_list);
	xSemaphoreGive(pm_device_mutex);

	pm_dbg("devops: register dev(%s) ok\n", dev->name);

	return 0;
}

int pm_devops_unregister(struct pm_device *dev)
{
	if (prohibit_register)
		return -EBUSY;

	if (!dev || !dev->name) {
		pm_invalid();
		return -EINVAL;
	}

	if (!pm_device_in_list(&dev->node)) {
		pm_err("pm device %s(%p) is not registered\n", dev->name, dev);
		return -EINVAL;
	}

	if (pdTRUE != xSemaphoreTake(pm_device_mutex, portMAX_DELAY)) {
		pm_semapbusy(pm_device_mutex);
		return -EBUSY;
	}

	list_del(&dev->node);
	xSemaphoreGive(pm_device_mutex);

	pm_dbg("devops: unregister dev(%s) done\n", dev->name);

	return 0;
}

static int pm_run_callback(pm_devcallback_t callback, struct pm_device *dev,
				pm_devops_type_t type, suspend_mode_t mode)
{
	int ret = 0;
	uint64_t starttime;
	uint64_t calltime;

	if (devops_trace) {
		pm_devops_start_trace(dev->name);
		starttime = hal_gettime_ns();
	}
	ret = callback(dev, mode);
	if (devops_trace) {
		calltime = hal_gettime_ns();
		pm_devops_end_trace(dev->name, ret, (calltime - starttime) / 1000LL);
	}
	if (ret)
		pm_err("pm_devops: %p failed, return %d\n", callback, ret);

	return ret;
}

static void pm_devops_call_resume(pm_devops_type_t type, suspend_mode_t mode)
{
	int ret = 0;
	uint32_t list_index;
	struct pm_device *dev = NULL;
	pm_devcallback_t callback = NULL;
	struct list_head *list = NULL;
	struct list_head *dest_list = NULL;


	list_index = pm_find_devlist(type);
	list = pm_devops_list[list_index];
	if (type == PM_DEVOPS_TYPE_COMPLETE) {
		dest_list = &pm_device_oldlist;
	} else
		dest_list = pm_devops_list[list_index - 1];

	xSemaphoreTake(pm_device_mutex, portMAX_DELAY);
	while (!list_empty(list)) {
		switch (type) {
		case PM_DEVOPS_TYPE_RESUME_NOIRQ:
			dev = pm_device_containerof(list->next);
			callback = (dev)?dev->ops->resume_noirq:NULL;
			break;
		case PM_DEVOPS_TYPE_RESUME_EARLY:
			dev = pm_device_containerof(list->next);
			callback = (dev)?dev->ops->resume_early:NULL;
			break;
		case PM_DEVOPS_TYPE_RESUME:
			dev = pm_device_containerof(list->next);
			callback = (dev)?dev->ops->resume:NULL;
			break;
		case PM_DEVOPS_TYPE_COMPLETE:
			dev = pm_device_containerof(list->next);
			callback = (dev)?dev->ops->complete:NULL;
			break;
		default:
			break;
		}
		xSemaphoreGive(pm_device_mutex);

		if (callback)
			ret = pm_run_callback(callback, dev, type, mode);

		if (ret) {
			pm_err("pm_device: %s(%p), opstype:%d failed.\n",\
				dev->name, dev, type);
			snprintf(pm_devresume_stats.last_failed_unit[pm_devresume_stats.last_failed_index],
				sizeof(pm_devresume_stats.last_failed_unit[0]),
				dev->name);
			pm_devresume_stats.last_failed_index++;
			pm_devresume_stats.last_failed_index %= REC_FAILED_NUM;
		}
		xSemaphoreTake(pm_device_mutex, portMAX_DELAY);
		list_move_tail(&dev->node, dest_list);
	}
	if (type == PM_DEVOPS_TYPE_COMPLETE)
		list_splice_init(dest_list, &pm_device_list);
	xSemaphoreGive(pm_device_mutex);

	if (ret)
		pm_devresume_stats.unit_failed++;
}

static int pm_devops_call_suspend(pm_devops_type_t type, suspend_mode_t mode)
{
	int ret = 0;
	uint32_t list_index;
	pm_devops_type_t resume_type;
	pm_resumecall_t pm_devops_devresume = NULL;
	struct pm_device *dev = NULL;
	pm_devcallback_t callback = NULL;
	struct list_head *list = NULL;

	list_index = pm_find_devlist(type);
	list = pm_devops_list[list_index];

	xSemaphoreTake(pm_device_mutex, portMAX_DELAY);
	while (!list_empty(list)) {
		switch (type) {
		case PM_DEVOPS_TYPE_PREPARED:
			dev = pm_device_containerof(list->prev);
			callback  = (dev)?dev->ops->prepared:NULL;
			resume_type = PM_DEVOPS_TYPE_COMPLETE;
			break;
		case PM_DEVOPS_TYPE_SUSPEND:
			dev = pm_device_containerof(list->prev);
			callback  = (dev)?dev->ops->suspend:NULL;
			resume_type = PM_DEVOPS_TYPE_RESUME;
			break;
		case PM_DEVOPS_TYPE_SUSPEND_LATE:
			dev = pm_device_containerof(list->prev);
			callback  = (dev)?dev->ops->suspend_late:NULL;
			resume_type = PM_DEVOPS_TYPE_RESUME_EARLY;
			break;
		case PM_DEVOPS_TYPE_SUSPEND_NOIRQ:
			dev = pm_device_containerof(list->prev);
			callback  = (dev)?dev->ops->suspend_noirq:NULL;
			resume_type = PM_DEVOPS_TYPE_RESUME_NOIRQ;
			break;
		default:
			break;
		}
		xSemaphoreGive(pm_device_mutex);

		if (callback)
			ret = pm_run_callback(callback, dev, type, mode);

		xSemaphoreTake(pm_device_mutex, portMAX_DELAY);
		if (ret) {
			pm_err("pm_device: %s(%p), opstype:%d failed.\n",\
				dev->name, dev, type);
			snprintf(pm_devsuspend_stats.last_failed_unit[pm_devsuspend_stats.last_failed_index],
				sizeof(pm_devsuspend_stats.last_failed_unit[0]),
				dev->name);
			pm_devsuspend_stats.last_failed_index++;
			pm_devsuspend_stats.last_failed_index %= REC_FAILED_NUM;
			break;
		}
		list_move(&dev->node, pm_devops_list[list_index + 1]);
	}
	xSemaphoreGive(pm_device_mutex);

	if (ret) {
		pm_devsuspend_stats.unit_failed++;
		pm_devops_devresume = pm_find_typecall(resume_type);
		pm_devops_devresume(mode);
	}

	return ret;
}

static int pm_devops_call(pm_devops_type_t type, suspend_mode_t mode)
{
	int ret = 0;

	pm_trace_func("%d, %d", type, mode);
	if (!pm_suspend_mode_valid(mode) || !pm_devops_type_valid(type))
		return -EINVAL;

	if (pm_devops_belong_suspend(type)) {
		/* check wakeup factors */
		ret = pm_suspend_assert();
		if (ret) {
			pm_trace_ret(pm_suspend_assert, ret);
			pm_devsuspend_stats.last_failed_errno = ret;
			return ret;
		}

		ret = pm_devops_call_suspend(type, mode);
	}

	if (pm_devops_belong_resume(type))
		pm_devops_call_resume(type, mode);

	return ret;
}

static void pm_devops_type_showtime(uint64_t starttime, pm_devops_type_t type,
					int error, const char *info)
{
	uint64_t calltime = hal_gettime_ns();

	pm_stage("%s of devices %s after %" PRIu64 " msecs%s%s\n",
		pm_devops_type2string(type),
		error ? "aborted" : "complete",
		(calltime - starttime) / 1000000LL,
		info ? " with info:" : "",
		info ? info : "");

	(void)calltime;
}

int pm_devops_prepared(suspend_mode_t mode)
{
	int ret;
	uint64_t starttime = hal_gettime_ns();
	pm_devops_type_t type = PM_DEVOPS_TYPE_PREPARED;

	/* before prohibition, wait until the device registion is completed  */
	xSemaphoreTake(pm_device_mutex, portMAX_DELAY);
	prohibit_register = 1;
	xSemaphoreGive(pm_device_mutex);

	ret = pm_devops_call(type, mode);
	pm_devops_type_showtime(starttime, type, ret, NULL);

	return ret;
}

int pm_devops_suspend(suspend_mode_t mode)
{
	int ret;
	uint64_t starttime = hal_gettime_ns();
	pm_devops_type_t type = PM_DEVOPS_TYPE_SUSPEND;

	ret = pm_devops_call(type, mode);
	pm_devops_type_showtime(starttime, type, ret, NULL);

	return ret;
}

int pm_devops_suspend_late(suspend_mode_t mode)
{
	int ret;
	uint64_t starttime = hal_gettime_ns();
	pm_devops_type_t type = PM_DEVOPS_TYPE_SUSPEND_LATE;

	ret = pm_devops_call(type, mode);
	pm_devops_type_showtime(starttime, type, ret, NULL);

	return ret;
}

int pm_devops_suspend_noirq(suspend_mode_t mode)
{
	int ret;
	uint64_t starttime = hal_gettime_ns();
	pm_devops_type_t type = PM_DEVOPS_TYPE_SUSPEND_NOIRQ;

	ret = pm_devops_call(type, mode);
	pm_devops_type_showtime(starttime, type, ret, NULL);

	return ret;
}

void pm_devops_resume_noirq(suspend_mode_t mode)
{
	uint64_t starttime = hal_gettime_ns();
	pm_devops_type_t type = PM_DEVOPS_TYPE_RESUME_NOIRQ;

	pm_devops_call(type, mode);
	pm_devops_type_showtime(starttime, type, 0, NULL);
}

void pm_devops_resume_early(suspend_mode_t mode)
{
	uint64_t starttime = hal_gettime_ns();
	pm_devops_type_t type = PM_DEVOPS_TYPE_RESUME_EARLY;

	pm_devops_call(type, mode);
	pm_devops_type_showtime(starttime, type, 0, NULL);
}

void pm_devops_resume(suspend_mode_t mode)
{
	uint64_t starttime = hal_gettime_ns();
	pm_devops_type_t type = PM_DEVOPS_TYPE_RESUME;

	pm_devops_call(type, mode);
	pm_devops_type_showtime(starttime, type, 0, NULL);
}

void pm_devops_complete(suspend_mode_t mode)
{
	uint64_t starttime = hal_gettime_ns();
	pm_devops_type_t type = PM_DEVOPS_TYPE_COMPLETE;

	xSemaphoreTake(pm_device_mutex, portMAX_DELAY);
	prohibit_register = 0;
	xSemaphoreGive(pm_device_mutex);

	pm_devops_call(type, mode);
	pm_devops_type_showtime(starttime, type, 0, NULL);
}
