
#ifndef _PM_STATE_H_
#define _PM_STATE_H_

#include "hal/aw_list.h"

typedef enum {
	PM_STATUS_RUNNING = 0,
	PM_STATUS_SLEEPING,
	PM_STATUS_SLEEPED,
	PM_STATUS_ACTIVING,
	PM_STATUS_AGAINING,
	PM_STATUS_WAKEUPING,

	PM_STATUS_MAX,
	PM_STATUS_BASE = PM_STATUS_RUNNING,
} suspend_status_t;

#define pm_suspend_status_valid(_t) \
	((_t) >= PM_STATUS_BASE && (_t) < PM_STATUS_MAX)

typedef enum {
	PM_MOMENT_IRQ_DISABLE = 0,
	PM_MOMENT_CLK_SUSPEND,
	PM_MOMENT_CLK_RESUME,
	PM_MOMENT_IRQ_ENABLE,

	PM_MOMENT_MAX,
	PM_MOMENT_BASE = PM_MOMENT_IRQ_DISABLE,
} pm_moment_t;

#define pm_moment_valid(_t) \
	((_t) >= PM_MOMENT_BASE && (_t) < PM_MOMENT_MAX)

#define REC_FAILED_NUM	(2)
struct pm_suspend_stats {
	const char *name;
	char last_failed_unit[REC_FAILED_NUM][40];
	uint32_t last_failed_index;
	uint32_t unit_failed;
	int last_failed_errno;
	struct list_head node;
};
int pm_report_register(struct pm_suspend_stats *stats);
void pm_report_stats(void);

int pm_state_set(suspend_status_t status);
suspend_status_t pm_state_get(void);

void pm_moment_record(pm_moment_t moment, uint64_t timeval);
uint64_t pm_moment_interval_us(pm_moment_t start, pm_moment_t end);
void pm_moment_clear(uint32_t clear_mask);


#endif

