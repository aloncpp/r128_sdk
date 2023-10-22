
#ifndef _PM_WAKECNT_H_
#define _PM_WAKECNT_H_

#include "pm_wakesrc.h"

typedef enum {
	PM_RELAX_SLEEPY = 0,
	PM_RELAX_WAKEUP,

	PM_RELAX_MAX,
	PM_RELAX_BASE = PM_RELAX_SLEEPY,
} pm_relax_type_t;

/* int pm_wakecnt_init(void); */
int pm_wakecnt_exit(void);
uint32_t pm_wakecnt_stash(void);
uint32_t pm_wakecnt_subcnt_update(void);

int pm_wakecnt_check(void);
int pm_wakecnt_has_changed(void);
int pm_wakecnt_in_progress(void);

void pm_wakecnt_inc(int irq);
void pm_stay_awake(int irq);
void pm_relax(int irq, pm_relax_type_t wakeup);

int pm_wakecnt_subcnt_has_changed(void);
void pm_wakecnt_subcnt_inc(int irq);


#endif

