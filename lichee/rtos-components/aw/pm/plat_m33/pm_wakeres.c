
#include "pm_debug.h"
#include "pm_adapt.h"
#include "pm_wakesrc.h"
#include "pm_m33_wakesrc.h"

#undef  PM_DEBUG_MODULE
#define PM_DEBUG_MODULE  PM_DEBUG_WAKESRC

static uint32_t pwrcfg, clkcfg, anacfg;

int pm_wakeres_update(void)
{
	int i = 0;
	struct pm_wakesrc_settled *ws;

	for (i=PM_WAKEUP_SRC_BASE; i < PM_WAKEUP_SRC_MAX; i++) {
		ws = pm_wakesrc_get_by_id(i);
		if (pm_wakesrc_get_active() & (0x1 << i)) {
			pwrcfg |= ws->res.pwrcfg;
			clkcfg |= ws->res.clkcfg;
			anacfg |= ws->res.anacfg;
		}
	}

	//pm_log("%s: pwrcfg:%x, clkcfg: %x, anacfg: %x\n", pwrcfg, clkcfg, anacfg);

	return 0;
}

uint32_t pm_wakeres_get_pwrcfg(void)
{
	return pwrcfg;
}

uint32_t pm_wakeres_get_clkcfg(void)
{
	return clkcfg;
}

uint32_t pm_wakeres_get_anacfg(void)
{
	return anacfg;
}


