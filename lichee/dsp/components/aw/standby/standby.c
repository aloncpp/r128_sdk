/*
 * for standby core
 */
#include <FreeRTOS.h>
#include <limits.h>
#include <task.h>
#include <standby.h>

#include "cpucfg.h"
#include "drivers/hal_watchdog.h"
#include "rtc_dbg.h"
#include "errno.h"
#include "message_manage.h"
#include "platform.h"

static int standby_enter(struct platform_standby *ps)
{
	void *pd = ps->platform_data;

	if (ps->pre_standby_enter)
		ps->pre_standby_enter(pd);

	if (ps->standby_enter)
		ps->standby_enter(ps->msg);

	if (ps->ddr_suspend)
		ps->ddr_suspend(pd);

	if (ps->bus_suspend)
		ps->bus_suspend(pd);

	if (ps->pll_suspend)
		ps->pll_suspend(pd);

	if (ps->osc_suspend)
		ps->osc_suspend(pd);

	if (ps->power_suspend)
		ps->power_suspend(pd);

	if (ps->post_standby_enter)
		ps->post_standby_enter(pd);

	return 0;
}

static int standby_exit(struct platform_standby *ps)
{
	void *pd = ps->platform_data;

	if (ps->pre_standby_exit)
		ps->pre_standby_exit(pd);

	if (ps->power_resume)
		ps->power_resume(pd);

	if (ps->osc_resume)
		ps->osc_resume(pd);

	if (ps->pll_resume)
		ps->pll_resume(pd);

	if (ps->bus_resume)
		ps->bus_resume(pd);

	if (ps->ddr_resume)
		ps->ddr_resume(pd);

	if (ps->standby_exit)
		ps->standby_exit(ps->msg);

	if (ps->post_standby_exit)
		ps->post_standby_exit(pd);

	return 0;
}

int cpu_op(struct message *pmessage)
{
	uint32_t mpidr = pmessage->paras[0];
	uint32_t entrypoint = pmessage->paras[1];
	uint32_t cpu_state = pmessage->paras[2];
	uint32_t cluster_state = pmessage->paras[3]; /* unused variable */
	uint32_t system_state = pmessage->paras[4];
	struct platform_standby *ps = platform_standby_get();

	LOG("mpidr:%x, entrypoint:%x; cpu_state:%x, cluster_state:%x, system_state:%x\n",
		mpidr, entrypoint, cpu_state, cluster_state, system_state);
	if (cpu_state == arisc_power_on) {
		set_secondary_entry(entrypoint, mpidr);
		cpu_power_up(0, mpidr);
	} else if (cpu_state == arisc_power_off) {
		if (entrypoint) {
			if (system_state == arisc_power_off) {
				standby_enter(ps);
				standby_wait_wakeup(ps);
				standby_exit(ps);
			} else if (cluster_state == arisc_power_off) {

			} else {

			}
		} else {
			cpu_power_down(0, mpidr);
		}
	}

	return 0;
}

void __attribute__((weak)) system_shutdown(void)
{
	/* clk_suspend(); */

	/* pmu_shutdown(); */
}

void __attribute__((weak)) hal_reset_cpu(void)
{
	return;
}

static void system_reset(void)
{
	hal_reset_cpu();
}


int sys_op(struct message *pmessage)
{
	u32 state = pmessage->paras[0];

	LOG("state:%x\n", state);

	switch (state) {
	case arisc_system_shutdown:
		{
			save_state_flag(REC_SHUTDOWN | 0x101);
			system_shutdown();
			break;
		}
	case arisc_system_reset:
	case arisc_system_reboot:
		{
			save_state_flag(REC_SHUTDOWN | 0x102);
			system_reset();
			break;
		}
	default:
		{
			WRN("invaid system power state (%d)\n", state);
			return -EINVAL;
		}
	}

	return 0;
}

