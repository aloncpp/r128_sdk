
#ifndef _PM_PLATOPS_H_
#define _PM_PLATOPS_H_

#include "pm_base.h"

typedef struct {
	char *name;
	int (*valid) (suspend_mode_t mode);
	int (*pre_begin) (suspend_mode_t mode);
	int (*begin) (suspend_mode_t mode);
	int (*prepare) (suspend_mode_t mode);
	int (*prepare_late) (suspend_mode_t mode);
	int (*enter) (suspend_mode_t mode);
	int (*wake) (suspend_mode_t mode);
	int (*finish) (suspend_mode_t mode);
	int (*end) (suspend_mode_t mode);
	int (*recover) (suspend_mode_t mode);
	int (*again) (suspend_mode_t mode);
	int (*again_late) (suspend_mode_t mode);
} suspend_ops_t;

typedef enum {
	PM_SUSPEND_OPS_TYPE_VALID = 0,

	PM_SUSPEND_OPS_TYPE_PREPARED_NOTIFY,
	PM_SUSPEND_OPS_TYPE_FINISHED_NOTIFY,

	PM_SUSPEND_OPS_TYPE_PRE_BEGIN,
	PM_SUSPEND_OPS_TYPE_BEGIN,
	PM_SUSPEND_OPS_TYPE_PREPARE,
	PM_SUSPEND_OPS_TYPE_PREPARE_LATE,
	PM_SUSPEND_OPS_TYPE_ENTER,
	PM_SUSPEND_OPS_TYPE_WAKE,
	PM_SUSPEND_OPS_TYPE_FINISH,
	PM_SUSPEND_OPS_TYPE_END,
	PM_SUSPEND_OPS_TYPE_POST_END,
	PM_SUSPEND_OPS_TYPE_RECOVER,
	PM_SUSPEND_OPS_TYPE_AGAIN,
	PM_SUSPEND_OPS_TYPE_AGAIN_LATE,

	PM_SUSPEND_OPS_TYPE_MAX,
	PM_SUSPEND_OPS_TYPE_BASE = PM_SUSPEND_OPS_TYPE_VALID,
} suspend_ops_type_t;



int pm_platops_register(suspend_ops_t *ops);
int pm_platops_call(suspend_ops_type_t type, suspend_mode_t mode);

#endif

