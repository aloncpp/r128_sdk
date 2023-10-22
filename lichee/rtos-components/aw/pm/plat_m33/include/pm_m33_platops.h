
#ifndef _PM_M33_PLATOPS_H_
#define _PM_M33_PLATOPS_H_

#include "pm_suspend.h"
#include "pm_platops.h"


struct arm_CMX_core_regs {
	unsigned int msp;
	unsigned int psp;
	unsigned int psr; /* don't restore */
	unsigned int primask;
	unsigned int faultmask;
	unsigned int basepri;
	unsigned int control;
	unsigned int gpregs[0]; /* used only for debug, reserve 64 bytes */
};

int pm_m33_platops_init(void);
int pm_m33_platops_deinit(void);
int pm_set_time_to_wakeup_ms(unsigned int ms);

#ifndef CONFIG_PM_SIMULATED_RUNNING
int pm_lpsram_para_init(void);
int pm_hpsram_para_init(void);
#endif

#endif


