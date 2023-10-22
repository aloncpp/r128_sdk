#include <stdlib.h>
#include <errno.h>
#include <console.h>

#include <mpu_wrappers.h>

#include "pm_debug.h"
#include "pm_suspend.h"

#ifdef CONFIG_AMP_PMOFM33_STUB
extern int pm_trigger_suspend(suspend_mode_t mode);
#else
int pm_trigger_suspend(suspend_mode_t mode) {
	pm_err("do not support trigger system suspend\n");
	return 0;
}
#endif
int pm_enter_sleep(int argc, char **argv)
{
	int ret = 0;
	pm_raw("the dsp to suspend\n");
	ret = pm_suspend_request(PM_MODE_SLEEP);
	pm_raw("%s return %d\n", __func__, ret);

	return 0;
}

int pm_enter_standby(int argc, char **argv)
{
	int ret = 0;
	pm_raw("the dsp to suspend\n");
	ret = pm_suspend_request(PM_MODE_STANDBY);
	pm_raw("%s return %d\n", __func__, ret);

	return 0;
}

int pm_enter_hibernation(int argc, char **argv)
{
	int ret = 0;
	ret = pm_trigger_suspend(PM_MODE_HIBERNATION);
	pm_raw("%s return %d\n", __func__, ret);

	return 0;
}

#if 0
/* if core is not standby control core, pm_test is used for debugging only */
int pm_test(int argc, char **argv)
{
	int ret = 0;
	int lvl = 0;


	if (argc != 2)
		goto err_inval;

	lvl = atoi(argv[1]);

	pm_raw("lvl: %d\n", lvl);
	if (!pm_suspend_testlevel_valid(lvl))
		goto err_inval;

	ret = pm_suspend_test_set(lvl);
	pm_raw("%s return %d\n", __func__, ret);

	return 0;

err_inval:
	pm_raw("%s args invalid.\n", __func__);
	pm_raw("argc: %d.\n", argc);
	for (int i=0; i<argc; i++) {
		pm_raw("argv[i]: %s.\n", argv[i]);
	}
	pm_raw("\n");
	return -EINVAL;
}
FINSH_FUNCTION_EXPORT_CMD(pm_test, pm_test, PM APIs tests004)
#endif

FINSH_FUNCTION_EXPORT_CMD(pm_enter_sleep, sleep, PM APIs tests001)
FINSH_FUNCTION_EXPORT_CMD(pm_enter_standby, standby, PM APIs tests002)
FINSH_FUNCTION_EXPORT_CMD(pm_enter_hibernation, hibernation, PM APIs tests003)

