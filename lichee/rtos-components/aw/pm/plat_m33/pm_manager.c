
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
#include "pm_init.h"

#include "pm_m33_platops.h"
#include "pm_m33_wakesrc.h"
#include "pm_rpcfunc.h"
#include "pm_testlevel.h"

#undef   PM_DEBUG_MODULE
#define  PM_DEBUG_MODULE  PM_DEBUG_M33

int pm_trigger_suspend(suspend_mode_t mode)
{
	uint32_t m33 = 0, rv = 0, dsp = 0;

	if (!pm_suspend_mode_valid(mode))
		return -EINVAL;

	m33 = pm_wakelocks_refercnt(1);
#ifdef CONFIG_PM_SUBSYS_RISCV_SUPPORT
	rv  = pm_wakelocks_getcnt_riscv(1);
#endif
#ifdef CONFIG_PM_SUBSYS_DSP_SUPPORT
	dsp  = pm_wakelocks_getcnt_dsp(1);
#endif

	if (m33 + rv + dsp)
		return -EPERM;

	return pm_suspend_request(mode);
}


int pm_init(int argc, char **argv)
{
	/* creat timer*/
	pm_wakelocks_init();

	/* before arch initialization of hardware devices */
	/* pm_wakesrc_init();
	   pm_wakecnt_init();
	*/

	/* create queue set*/
	pm_subsys_init();

	/* creat mutex, put pm_devops_init() and pm_syscore_init() before arch initialization of hardware devices
	pm_syscore_init();

	pm_devops_init();
	*/

	/* creat mutex*/
	pm_notify_init();

	/* protect Idle/Timer task */
	pm_task_init();

	/* register ops  */
	pm_m33_platops_init();

#ifdef CONFIG_PM_SUBSYS_RISCV_SUPPORT
	extern int pm_riscv_init(void);
	pm_riscv_init();
#endif

#ifdef CONFIG_PM_SUBSYS_DSP_SUPPORT
	extern int pm_dsp_init(void);
	pm_dsp_init();
#endif

	pm_wakeup_ops_m33_init();

#ifdef CONFIG_COMPONENTS_PM_TEST_TOOLS
	pm_test_tools_init();
#endif

#ifndef CONFIG_PM_SIMULATED_RUNNING
	extern int pm_lpsram_para_init(void);
	extern int pm_hpsram_para_init(void);
	pm_lpsram_para_init();
	pm_hpsram_para_init();
#endif

	/* creat pm_task and register it, then create queue*/
	pm_suspend_init();

	return 0;
}


