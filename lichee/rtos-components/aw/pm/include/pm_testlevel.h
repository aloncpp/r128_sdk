
#ifndef _PM_TESTLEVEL_H_
#define _PM_TESTLEVEL_H_

#ifdef CONFIG_ARCH_SUN20IW2
#define GPRCM_SYS_PRIV_REG4		(0x40050210)
#define GPRCM_SYS_PRIV_REG5		(0x40050214)
#define GPRCM_SYS_PRIV_REG6		(0x40050218)

#define PM_STANDBY_STAGE_M33_REC_REG	(GPRCM_SYS_PRIV_REG6)
#define PM_STANDBY_STAGE_C906_REC_REG	(GPRCM_SYS_PRIV_REG5)
#define PM_STANDBY_STAGE_DSP_REC_REG	(GPRCM_SYS_PRIV_REG4)
#endif

#define PM_TEST_RECORDING_ENTER		(0xfcfc0000)

typedef enum suspend_testlevel {
	PM_SUSPEND_TEST_NONE = 0,
	PM_SUSPEND_TEST_FREEZER,
	PM_SUSPEND_TEST_DEVICE,
	PM_SUSPEND_TEST_PLATFORM,
	PM_SUSPEND_TEST_CPU,

	PM_SUSPEND_TEST_MAX,
	PM_SUSPEND_TEST_BASE = PM_SUSPEND_TEST_NONE,
} suspend_testlevel_t;

#define pm_suspend_testlevel_valid(_t) \
	((_t) >= PM_SUSPEND_TEST_BASE && (_t) < PM_SUSPEND_TEST_MAX)


int pm_suspend_test(suspend_testlevel_t level);
int pm_suspend_test_set(suspend_testlevel_t level);

/* PM standby test record */
int pm_test_standby_recording(void);
#ifdef CONFIG_COMPONENTS_PM_TEST_TOOLS
int pm_test_tools_init(void);
#endif /* CONFIG_COMPONENTS_PM_TEST_TOOLS */

#endif


