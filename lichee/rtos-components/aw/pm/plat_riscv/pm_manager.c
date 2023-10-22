
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

#include "pm_rpcfunc.h"

extern int pm_riscv_platops_init(void);
int pm_init(int argc, char **argv)
{
	/* creat timer*/
	pm_wakelocks_init();

	/* before arch initialization of hardware devices
	pm_wakecnt_init();
	pm_wakesrc_init();
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
	pm_riscv_platops_init();

#ifdef CONFIG_COMPONENTS_PM_TEST_TOOLS
	pm_test_tools_init();
#endif

	/* creat pm_task and register it, then create queue*/
	pm_suspend_init();

	return 0;
}


