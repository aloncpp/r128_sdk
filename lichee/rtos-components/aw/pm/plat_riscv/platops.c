#include <errno.h>
#include <hal_osal.h>
#include <osal/hal_interrupt.h>

#include <io.h>
#include "pm_suspend.h"
#include "pm_debug.h"
#include "pm_wakecnt.h"
#include "pm_wakesrc.h"
#include "pm_subsys.h"
#include "pm_syscore.h"
#include "pm_platops.h"
#include "pm_systeminit.h"

extern void _rv_cpu_suspend(void);
extern void _rv_cpu_save_boot_flag(void);
extern void _rv_cpu_resume(void);
extern long rv_sleep_flag;

static int pm_riscv_valid(suspend_mode_t mode)
{
	int ret = 0;

	switch (mode) {
	case PM_MODE_SLEEP:
	case PM_MODE_STANDBY:
	case PM_MODE_HIBERNATION:
		break;
	case PM_MODE_ON:
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}


static int pm_riscv_begin(suspend_mode_t mode)
{
	int ret = 0;

	switch (mode) {
	case PM_MODE_SLEEP:
	case PM_MODE_STANDBY:
	case PM_MODE_HIBERNATION:
		/* suspend all subsys */
		//ret = pm_subsys_suspend_sync(mode);
		break;
	case PM_MODE_ON:
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}


static int pm_riscv_prepare(suspend_mode_t mode)
{

	return 0;
}


static int pm_riscv_prepare_late(suspend_mode_t mode)
{
	int ret = 0;

	switch (mode) {
	case PM_MODE_SLEEP:
	case PM_MODE_STANDBY:
	case PM_MODE_HIBERNATION:
		ret = pm_suspend_assert();
		break;
	case PM_MODE_ON:
	default:
		ret = -EINVAL;
		break;
	}

	return ret;

}


static int set_finished_dummy(void)
{
	writel(0x16aaf5f5, 0x40050000 + 0x200);

	return 0;
}

extern void hal_icache_init(void);
extern void hal_dcache_init(void);
static int pm_riscv_enter(suspend_mode_t mode)
{
	int ret = 0;

	switch (mode) {
	case PM_MODE_SLEEP:
	case PM_MODE_STANDBY:
	case PM_MODE_HIBERNATION:
		if (mode != PM_MODE_HIBERNATION)
			_rv_cpu_save_boot_flag();
		_rv_cpu_suspend();

		if (rv_sleep_flag) {
			pm_dbg("suspened\n");
			hal_dcache_clean_all();
			set_finished_dummy();
			while (readl(0x40050000 + 0x200)) {
				__asm__ __volatile__("wfi");
			}
			_rv_cpu_resume();
		}
		while (readl(0x40050000 + 0x200));
		pm_dbg("resume\n");
		break;
	case PM_MODE_ON:
	default:
		ret = -EINVAL;
		break;
	}

	if (mode != PM_MODE_SLEEP)
		pm_systeminit();

	return ret;
}


static int pm_riscv_wake(suspend_mode_t mode)
{
	int ret = 0;

	switch (mode) {
	case PM_MODE_SLEEP:
		break;
	case PM_MODE_STANDBY:
		/* pm_syscore_resume(mode); //call by pm framework */
		break;
	case PM_MODE_HIBERNATION:
		break;
	case PM_MODE_ON:
	default:
		ret = -EINVAL;
		break;
	}

	return ret;

}


static int pm_riscv_finish(suspend_mode_t mode)
{
	int ret = 0;

	switch (mode) {
	case PM_MODE_SLEEP:
		break;
	case PM_MODE_STANDBY:
		break;
	case PM_MODE_HIBERNATION:
		break;
	case PM_MODE_ON:
	default:
		ret = -EINVAL;
		break;
	}

	return ret;

}


static int pm_riscv_end(suspend_mode_t mode)
{
	int ret = 0;

	switch (mode) {
	case PM_MODE_SLEEP:
		break;
	case PM_MODE_STANDBY:
		//pm_wakesrc_complete();
		//pm_subsys_resume_sync(mode);
		break;
	case PM_MODE_HIBERNATION:
		break;
	case PM_MODE_ON:
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}


static int pm_riscv_recover(suspend_mode_t mode)
{
	int ret = 0;

	switch (mode) {
	case PM_MODE_SLEEP:
		break;
	case PM_MODE_STANDBY:
		break;
	case PM_MODE_HIBERNATION:
		break;
	case PM_MODE_ON:
	default:
		ret = -EINVAL;
		break;
	}

	return ret;

}


static int pm_riscv_again(suspend_mode_t mode)
{
	int ret = 0;

	switch (mode) {
	case PM_MODE_SLEEP:
		break;
	case PM_MODE_STANDBY:
		break;
	case PM_MODE_HIBERNATION:
		break;
	case PM_MODE_ON:
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static suspend_ops_t pm_riscv_suspend_ops = {
	.name  = "pm_riscv_suspend_ops",
	.valid = pm_riscv_valid,
	.begin = pm_riscv_begin,
	.prepare = pm_riscv_prepare,
	.prepare_late = pm_riscv_prepare_late,
	.enter = pm_riscv_enter,
	.wake = pm_riscv_wake,
	.finish = pm_riscv_finish,
	.end = pm_riscv_end,
	.recover = pm_riscv_recover,
	.again = pm_riscv_again,
};

int pm_riscv_platops_init(void)
{
	return pm_platops_register(&pm_riscv_suspend_ops);
}

int pm_riscv_platops_deinit(void)
{
	return pm_platops_register(NULL);
}



