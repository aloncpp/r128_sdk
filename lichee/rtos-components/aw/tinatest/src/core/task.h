#ifndef TT_TASK_H
#define TT_TASK_H

int tt_testcase_list_init(int argc, char *argv[]);
struct tt_data_t *tt_conn_init(volatile int *exit);
int tt_conn_exit(struct tt_data_t *data);
int tt_run_testcase(struct tt_data_t *data);

#endif
