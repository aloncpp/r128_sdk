#include <stdlib.h>
#include <string.h>
#include <osal/hal_interrupt.h>
#include <errno.h>
#include <hal_time.h>

#include <hal/aw_list.h>
#include "pm_debug.h"
#include "pm_state.h"
#include "pm_adapt.h"

#define PM_REPORT	("PM_REPORT")
#define pm_susnpend_stats_containerof(ptr_module) \
        __containerof(ptr_module, struct pm_suspend_stats, node)
static struct list_head pm_report_list = LIST_HEAD_INIT(pm_report_list);

static suspend_status_t pm_suspend_status = PM_STATUS_RUNNING;

static uint64_t moment_record[PM_MOMENT_MAX] = {0};

int pm_state_set(suspend_status_t status)
{
	if (!pm_suspend_status_valid(status))
		return -EINVAL;

	pm_suspend_status = status;

	return 0;
}

suspend_status_t pm_state_get(void)
{
	return pm_suspend_status;
}

int pm_report_register(struct pm_suspend_stats *stats) {
	if (!stats || !stats->name)
		return -EINVAL;

	/* new stats insert as head */
	list_add(&stats->node, &pm_report_list);

	return 0;
}

void pm_report_stats(void) {
	struct pm_suspend_stats *stats = NULL;
	struct list_head *node = NULL;
	struct list_head *head = &pm_report_list;

	list_for_each(node, head) {
		stats = pm_susnpend_stats_containerof(node);
		if (stats->unit_failed) {
			pm_err("%s[%s]: unit failed cnt %d, last fail unit0: %s\n", PM_REPORT,
				stats->name, stats->unit_failed, stats->last_failed_unit[0]);
			if (stats->last_failed_unit[1] && (stats->last_failed_index == 1))
				pm_err("last fail unit1: %s\n", stats->last_failed_unit[1]);
			stats->last_failed_index = 0;
			stats->unit_failed = 0;
		}

		if (stats->last_failed_errno) {
			pm_err("%s[%s]: last fail errno: %d\n", PM_REPORT,
				stats->name, stats->last_failed_errno);
			stats->last_failed_errno = 0;
		}
	}
}

void pm_moment_record(pm_moment_t moment, uint64_t timeval)
{
	if (!pm_moment_valid(moment)) {
		pm_err("record moment %d invalid\n", moment);
		return;
	}

	if (!moment_record[moment]) {
		if (timeval)
			moment_record[moment] = timeval;
		else
			moment_record[moment] = hal_gettime_ns();

		pm_dbg("pm record moment %d\n", moment);
	} else {
		pm_err("pm moment %d has been recorded\n", moment);
	}
}

uint64_t pm_moment_interval_us(pm_moment_t start, pm_moment_t end)
{

	if (!pm_moment_valid(start) || !pm_moment_valid(end) || end < start) {
		pm_err("pm moment interval invalid, start: %d, end: %d\n", start, end);
		return 0;
	}

	if (!moment_record[start]) {
		pm_err("start moment %d is unrecorded, get interval: 0\n", start);
		return 0;
	}

	if (!moment_record[end]) {
		pm_err("end moment %d is unrecorded, get interval: 0\n", end);
		return 0;
	}

	return ((moment_record[end] - moment_record[start]) / 1000LL);
}

void pm_moment_clear(uint32_t clear_mask)
{
	int i;

	for (i = PM_MOMENT_BASE; i < PM_MOMENT_MAX; i++) {
		if (clear_mask & (0x1 << i)) {
			moment_record[i] = 0;
			pm_dbg("clear moment record %d\n", i);
		}
	}

}
