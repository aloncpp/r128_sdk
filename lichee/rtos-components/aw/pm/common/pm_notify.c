#include <errno.h>

#include "pm_base.h"
#include "pm_notify.h"
#include "pm_debug.h"
#include "pm_base.h"

#undef  PM_DEBUG_MODULE
#define PM_DEBUG_MODULE  PM_DEBUG_NOTIFY

static SemaphoreHandle_t pm_notify_mutex = NULL;
static int          pm_notify_used[PM_NOTIFY_ARRAY_MAX]  = {0};
static pm_notify_t *pm_notify_array[PM_NOTIFY_ARRAY_MAX] = {0};

int pm_notify_init(void)
{
	pm_notify_mutex = xSemaphoreCreateMutex();
	return 0;
}

int pm_notify_exit(void)
{
		vSemaphoreDelete(pm_notify_mutex);
		return 0;
}


static int pm_notify_check_invalid(pm_notify_t *nt)
{
	if (!nt || !nt->name || !nt->pm_notify_cb) {
		pm_invalid();
		return -EINVAL;
	}

	return 0;
}

int pm_notify_register(pm_notify_t *nt)
{
	int i   = 0;
	int ret = 0;

	ret = pm_notify_check_invalid(nt);
	if (ret) {
		pm_invalid();
		return ret;
	}

	if (pdTRUE != xSemaphoreTake(pm_notify_mutex, portMAX_DELAY)) {
		pm_semapbusy(pm_notify_mutex);
		return -EBUSY;
	}

	for (i=0; i < PM_NOTIFY_ARRAY_MAX; i++) {
		if (PM_NOTIFY_USED_MAGIC == pm_notify_used[i])
			continue;

		pm_notify_array[i] = nt;
		pm_notify_used[i]  = PM_NOTIFY_USED_MAGIC;
		break;
	}

	if (PM_NOTIFY_ARRAY_MAX == i) {
		pm_err("Alloc failed, pm_notify more than %d.\n", PM_NOTIFY_ARRAY_MAX);
		ret = -EPERM;
	} else
		ret = i;

	xSemaphoreGive(pm_notify_mutex);

	return ret;
}

int pm_notify_unregister(int id)
{
	int ret = 0;

	if (id < 0 || id >= PM_NOTIFY_ARRAY_MAX)
		return -EINVAL;

	ret = pm_notify_check_invalid(pm_notify_array[id]);
	if (ret || PM_NOTIFY_USED_MAGIC != pm_notify_used[id]) {
		pm_warn("pm_notify invalid when unregister.\n");
	}

	if (pdTRUE != xSemaphoreTake(pm_notify_mutex, portMAX_DELAY)) {
		pm_semapbusy(pm_notify_mutex);
		return -EBUSY;
	}

	pm_notify_used[id] = 0;
	pm_notify_array[id] = NULL;

	xSemaphoreGive(pm_notify_mutex);

	return 0;
}


int pm_notify_event(suspend_mode_t mode, pm_event_t event)
{
	int i     = 0;
	int ret   = 0;
	int fail  = 0;
	int restore_event = -1;

	if (pdTRUE != xSemaphoreTake(pm_notify_mutex, portMAX_DELAY)) {
		pm_semapbusy(pm_notify_mutex);
		return -EBUSY;
	}

	if (!pm_event_valid(event)) {
		pm_err("invalid pm notify event: %d\n", event);
		return -EINVAL;
	}

	for (i=0; i < PM_NOTIFY_ARRAY_MAX; i++) {
		if (PM_NOTIFY_USED_MAGIC == pm_notify_used[i]) {
			ret = pm_notify_array[i]->pm_notify_cb(mode, event, pm_notify_array[i]->arg);
			if (ret) {
				fail = 1;
				pm_err("Execute pm_notify(%s) event(%d) failed.\n",
						pm_notify_array[i]->name ? pm_notify_array[i]->name :"unkown-notify", event);
				break;
			}
			pm_notify_array[i]->has_notify |= (1 << event);
		}
	}

	if (fail) {
		switch (event) {
		case PM_EVENT_SYS_PERPARED:
			restore_event = PM_EVENT_SYS_FINISHED;
			break;
		case PM_EVENT_PERPARED:
			restore_event = PM_EVENT_FINISHED;
			break;
		default:
			pm_err("unknown notify event type failed\n");
			break;
		}

		if (restore_event >= 0) {
			for (i=0; i < PM_NOTIFY_ARRAY_MAX; i++) {
				if ((PM_NOTIFY_USED_MAGIC != pm_notify_used[i]) || !(pm_notify_array[i]->has_notify & (1 << event)))
					continue;
				pm_notify_array[i]->pm_notify_cb(mode, restore_event, pm_notify_array[i]->arg);
				if (ret)
					pm_err("Execute pm_notify(%s) event(%d) failed.\n",
						pm_notify_array[i]->name ? pm_notify_array[i]->name :"unkown-notify", restore_event);
			}
		}
	}

	for (i=0; i < PM_NOTIFY_ARRAY_MAX; i++) {
		if ((PM_NOTIFY_USED_MAGIC != pm_notify_used[i]) || !(pm_notify_array[i]->has_notify & (1 << event)))
			continue;
		pm_notify_array[i]->has_notify &= ~(1 << event);
	}

	xSemaphoreGive(pm_notify_mutex);

	return fail?-EPERM:0;
}

