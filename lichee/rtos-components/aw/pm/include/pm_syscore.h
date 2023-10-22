
#ifndef _PM_SYSCORE_H_
#define _PM_SYSCORE_H_

#include "pm_base.h"
#include "hal/aw_list.h"

#define COMMON_SYSCORE	(0x1)

struct syscore_ops {
	const char *name;
	int common_syscore;
	struct list_head node;
	void *data;
	int  (*suspend)(void *data, suspend_mode_t mode);
	void (*resume) (void *data, suspend_mode_t mode);
};

int pm_syscore_init(void);
int pm_syscore_exit(void);
int pm_syscore_register(struct syscore_ops *ops);
int pm_syscore_unregister(struct syscore_ops *ops);
int pm_common_syscore_suspend(suspend_mode_t mode);
int pm_common_syscore_resume(suspend_mode_t mode);
int pm_syscore_suspend(suspend_mode_t mode);
int pm_syscore_resume(suspend_mode_t mode);


#endif

