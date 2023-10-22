#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aw_list.h"
#include "tinatest.h"
#include "outlog.h"
#include "outlog_serial.h"
#include "outlog_dragonmat.h"

static struct list_head list_before_all;
static struct list_head list_before_one;
static struct list_head list_after_one_begin;
static struct list_head list_after_one_once;
static struct list_head list_after_one_end;
static struct list_head list_after_all;

static int cnt_sum;
static int cnt_before_all;
static int cnt_before_one;
static int cnt_after_one_begin;
static int cnt_after_one_once;
static int cnt_after_one_end;
static int cnt_after_all;

static char *module_name;

#define MODULE_LNODE(MODE) \
struct lnode_ ## MODE { \
    MODE func; \
    char * name; \
    struct list_head lnode; \
}

MODULE_LNODE(before_all);
MODULE_LNODE(before_one);
MODULE_LNODE(after_one_begin);
MODULE_LNODE(after_one_once);
MODULE_LNODE(after_one_end);
MODULE_LNODE(after_all);

#define MODULE_CALL_FUNC(MODE, ARGS_TYPE, ARGS_NAME) \
int outlog_call_ ## MODE (ARGS_TYPE *ARGS_NAME) \
{ \
    int ret = 0; \
    struct lnode_ ## MODE *m = NULL; \
    DEBUG("In outlog function call " #MODE "\n"); \
    if (cnt_ ## MODE <= 0) { \
        DEBUG("no module register in " # MODE "\n"); \
        return 0; \
    } \
    list_for_each_entry(m, &list_ ## MODE, lnode) { \
        DEBUG("%s: call " # MODE "\n", m->name); \
        if (m->func(ARGS_NAME) != 0 && ret == 0) { \
            ERROR("%s: call " # MODE " failed\n", m->name); \
            ret = -1; \
        } \
    } \
    return ret; \
}

MODULE_CALL_FUNC(before_all, struct list_head, TASK_LIST);
MODULE_CALL_FUNC(before_one, struct tt_struct, tt);
MODULE_CALL_FUNC(after_one_begin, struct tt_struct, tt);
MODULE_CALL_FUNC(after_one_once, struct tt_struct, tt);
MODULE_CALL_FUNC(after_one_end, struct tt_struct, tt);
MODULE_CALL_FUNC(after_all, struct list_head, TASK_LIST);


static inline void init_list(void)
{
    // init cnt
    cnt_sum = 0;
    cnt_before_all = 0;
    cnt_before_one = 0;
    cnt_after_one_begin = 0;
    cnt_after_one_once = 0;
    cnt_after_one_end = 0;
    cnt_after_all = 0;
    // init all list
    INIT_LIST_HEAD(&list_before_all);
    INIT_LIST_HEAD(&list_before_one);
    INIT_LIST_HEAD(&list_after_one_begin);
    INIT_LIST_HEAD(&list_after_one_once);
    INIT_LIST_HEAD(&list_after_one_end);
    INIT_LIST_HEAD(&list_after_all);
}

struct outlog_data_t{
#ifdef CONFIG_DRAGONMAT
	struct dragonmat_data_t *dragonmat_data;
#else
	void *reserved;
#endif
};

struct outlog_data_t *outlog_init(volatile int *exit)
{
	int ret = -1;

	DEBUG("initializing outlog\n");

	struct outlog_data_t *data = malloc(sizeof(struct outlog_data_t));
	if(!data){
		ERROR("malloc failed!\n");
		goto err1;
	}

	init_list();
#ifdef CONFIG_DRAGONMAT
	data->dragonmat_data = dragonmat_module_init(exit);
	if(!data->dragonmat_data){
		ERROR("dragonmat init failed\n");
		goto err2;
	}
#else
	ret = serial_module_init();
	if (ret < 0) {
		ERROR("outlog init failed\n");
		goto err2;
	}
#endif

	return data;
err2:
	free(data);
err1:
	return NULL;
}

int outlog_exit(struct outlog_data_t *data)
{
	if(data){
#ifdef CONFIG_DRAGONMAT
		dragonmat_module_exit(data->dragonmat_data);
#else
	//serial_module_exit();
#endif
		free(data);
	}
	return 0;
}

int outlog_register(
        before_all b_all,
        after_one_once a_one_once,
        after_one_end a_one_end,
        after_all a_all)
{
    return outlog_register_ex(
            b_all,
            NULL,
            NULL,
            a_one_once,
            a_one_end,
            a_all);
}

#define INIT_MODULE_LNODE(MODE, FUNC) \
    struct lnode_ ## MODE *m = malloc(sizeof(struct lnode_ ## MODE)); \
    if (m == NULL) { \
        ERROR("malloc failed \n"); \
        return -1; \
    } \
    m->func = FUNC; \
    m->name = module_name; \
    list_add_tail(&m->lnode, &list_ ## MODE); \
    cnt_ ## MODE ++; \
    DEBUG("%s: register to" # MODE "\n", module_name);


int outlog_register_ex(
        before_all b_all,
        before_one b_one,
        after_one_begin a_one_begin,
        after_one_once a_one_once,
        after_one_end a_one_end,
        after_all a_all)
{
    // do register
    if (b_all != NULL) {
        INIT_MODULE_LNODE(before_all, b_all);
    }
    if (b_one != NULL) {
        INIT_MODULE_LNODE(before_one, b_one);
    }
    if (a_one_begin != NULL) {
        INIT_MODULE_LNODE(after_one_begin, a_one_begin);
    }
    if (a_one_once != NULL) {
        INIT_MODULE_LNODE(after_one_once, a_one_once);
    }
    if (a_one_end != NULL) {
        INIT_MODULE_LNODE(after_one_end, a_one_end);
    }
    if (a_all != NULL) {
        INIT_MODULE_LNODE(after_all, a_all);
    }
    return 0;
}
