#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <hal_atomic.h>
#include <errno.h>

#include "pm_wakecnt.h"
#include "pm_wakesrc.h"
#include "pm_init.h"
#include "pm_debug.h"

#undef   PM_DEBUG_MODULE
#define  PM_DEBUG_MODULE  PM_DEBUG_WAKECNT

static volatile uint32_t cnt = 0;
static volatile uint32_t save_cnt = 0;
static volatile uint32_t inpr = 0;
static volatile uint32_t wakeup_voke = 0;
static volatile uint32_t subcnt = 0;
static volatile uint32_t save_subcnt = 0;

static hal_spinlock_t pm_wakecnt_lock;

int pm_wakecnt_init(void)
{
	hal_spin_lock_init(&pm_wakecnt_lock);

	return 0;
}

int pm_wakecnt_exit(void)
{
	hal_spin_lock_deinit(&pm_wakecnt_lock);

	return 0;
}

int pm_wakecnt_check(void)
{
	int ret;

	hal_spin_lock(&pm_wakecnt_lock);
	ret = ((cnt != save_cnt) || (inpr > 0));
	hal_spin_unlock(&pm_wakecnt_lock);

	return ret;;
}

int pm_wakecnt_subcnt_has_changed(void)
{
	int ret;

	hal_spin_lock(&pm_wakecnt_lock);
	ret = (subcnt != save_subcnt);
	hal_spin_unlock(&pm_wakecnt_lock);

	return ret;;
}

int pm_wakecnt_has_changed(void)
{
	int ret;

	hal_spin_lock(&pm_wakecnt_lock);
	ret = (cnt != save_cnt);
	hal_spin_unlock(&pm_wakecnt_lock);

	return ret;;
}

int pm_wakecnt_in_progress(void)
{
	int ret;

	hal_spin_lock(&pm_wakecnt_lock);
	ret = (inpr > 0);
	hal_spin_unlock(&pm_wakecnt_lock);

	return ret;;
}

uint32_t pm_wakecnt_stash(void)
{
	hal_spin_lock(&pm_wakecnt_lock);
	save_cnt = cnt;
	save_subcnt = subcnt;
	hal_spin_unlock(&pm_wakecnt_lock);

	return save_cnt;
}

/**
 * cnt is the judgement for the resume of the whole system, cnt changed means the whole system should be awake.
 * sub cnt is the judgement for the resume of the other core, cnt changed means all subcore should be awake.
 */
uint32_t pm_wakecnt_subcnt_update(void)
{
	hal_spin_lock(&pm_wakecnt_lock);
	save_subcnt = subcnt;
	hal_spin_unlock(&pm_wakecnt_lock);

	return save_subcnt;
}

void pm_stay_awake(int irq)
{
	struct pm_wakesrc *ws = NULL;

	ws = pm_wakesrc_find_enabled_by_irq(irq);
	if (!ws)
		return;

	hal_spin_lock(&ws->lock);
	hal_spin_lock(&pm_wakecnt_lock);
	if (!(ws->cnt.active_cnt != ws->cnt.relax_cnt)) {
		ws->cnt.active_cnt++;
		ws->cnt.event_cnt++;
		inpr++;
	}
	hal_spin_unlock(&pm_wakecnt_lock);
	hal_spin_unlock(&ws->lock);
}

void pm_relax(int irq, pm_relax_type_t wakeup)
{
	struct pm_wakesrc *ws = NULL;

	ws = pm_wakesrc_find_enabled_by_irq(irq);
	if (!ws)
		return;

	hal_spin_lock(&ws->lock);
	hal_spin_lock(&pm_wakecnt_lock);
	if (!(ws->cnt.active_cnt == ws->cnt.relax_cnt)) {
		ws->cnt.relax_cnt = ws->cnt.active_cnt;
		inpr--;

		if (ws->type == PM_WAKESRC_ALWAYS_WAKEUP) {
			ws->cnt.wakeup_cnt++;
			cnt++;
		} else if ((ws->type == PM_WAKESRC_MIGHT_WAKEUP) || (ws->type == PM_WAKESRC_SOFT_WAKEUP)) {
			if (wakeup == PM_RELAX_WAKEUP) {
				ws->cnt.wakeup_cnt++;
				cnt++;
			}
		}
	}
	hal_spin_unlock(&pm_wakecnt_lock);
	hal_spin_unlock(&ws->lock);
}

/* Report the wake event immediately, it will causes the call to stay_awake to be ignored. */
void pm_wakecnt_inc(int irq)
{
	struct pm_wakesrc *ws = NULL;

	ws = pm_wakesrc_find_enabled_by_irq(irq);
	if (!ws)
		return;

	hal_spin_lock(&ws->lock);
	hal_spin_lock(&pm_wakecnt_lock);
	if (ws->cnt.active_cnt != ws->cnt.relax_cnt)
		inpr--;
	ws->cnt.event_cnt++;
	ws->cnt.active_cnt++;
	ws->cnt.relax_cnt = ws->cnt.active_cnt;
	ws->cnt.wakeup_cnt++;
	cnt ++;
	hal_spin_unlock(&pm_wakecnt_lock);
	hal_spin_unlock(&ws->lock);
}

void pm_wakecnt_subcnt_inc(int irq)
{
	hal_spin_lock(&pm_wakecnt_lock);
	subcnt++;
	hal_spin_unlock(&pm_wakecnt_lock);
}
