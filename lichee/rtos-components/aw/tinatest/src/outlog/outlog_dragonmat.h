#ifndef __OUTLOG_DRAGONMAT_H
#define __OUTLOG_DRAGONMAT_H

struct dragonmat_data_t *dragonmat_module_init(volatile int *exit);
int dragonmat_module_exit(struct dragonmat_data_t *data);
#endif
