
#include <stdlib.h>
#include <errno.h>
#include <console.h>

#include <mpu_wrappers.h>

#include "pm_suspend.h"
#include "pm_rpcfunc.h"
#include "pm_adapt.h"
#include "pm_debug.h"
#include "pm_testlevel.h"

#include "pm_wakelock.h"
#include "pm_wakecnt.h"
#include "pm_wakesrc.h"

#include "pm_devops.h"
#include "pm_subsys.h"
#include "pm_syscore.h"

#include "pm_notify.h"
#include "pm_task.h"
#include "pm_platops.h"
#include "pm_manager.h"

#undef   PM_DEBUG_MODULE
#define  PM_DEBUG_MODULE  PM_DEBUG_M33

int pm_trigger_suspend(suspend_mode_t mode);

int pm_enter_sleep(int argc, char **argv)
{
	int ret = 0;
	ret = pm_trigger_suspend(PM_MODE_SLEEP);
	printf("%s return %d\n", __func__, ret);

	return 0;
}

int pm_enter_standby(int argc, char **argv)
{
	int ret = 0;
	ret = pm_trigger_suspend(PM_MODE_STANDBY);
	printf("%s return %d\n", __func__, ret);

	return 0;
}

int pm_enter_hibernation(int argc, char **argv)
{
	int ret = 0;
	ret = pm_trigger_suspend(PM_MODE_HIBERNATION);
	printf("%s return %d\n", __func__, ret);

	return 0;
}

int pm_test(int argc, char **argv)
{
	int ret = 0;
	int lvl = 0;


	if (argc != 2)
		goto err_inval;

	lvl = atoi(argv[1]);

	printf("lvl: %d\n", lvl);
	if (!pm_suspend_testlevel_valid(lvl))
		goto err_inval;

	ret = pm_suspend_test_set(lvl);
	printf("%s return %d\n", __func__, ret);

	return 0;

err_inval:
	printf("%s args invalid.\n", __func__);
	printf("argc: %d.\n", argc);
	for (int i=0; i<argc; i++) {
		printf("argv[i]: %s.\n", argv[i]);
	}
	printf("\n");
	return -EINVAL;
}

FINSH_FUNCTION_EXPORT_CMD(pm_enter_sleep, sleep, PM APIs tests001)
FINSH_FUNCTION_EXPORT_CMD(pm_enter_standby, standby, PM APIs tests002)
FINSH_FUNCTION_EXPORT_CMD(pm_enter_hibernation, hibernation, PM APIs tests003)
FINSH_FUNCTION_EXPORT_CMD(pm_test, pm_test, PM APIs tests004)


