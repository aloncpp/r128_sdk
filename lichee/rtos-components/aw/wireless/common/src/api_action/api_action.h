#ifndef __API_ACTION_H_
#define __API_ACTION_H_

#include <os_net_utils.h>
#include <os_net_queue.h>
#include <os_net_sem.h>
#include <os_net_mutex.h>
#include <os_net_thread.h>

typedef struct {
    int (*action)(void **call_argv, void **cb_argv);
    const char *name;
} act_func_t;

typedef uint32_t act_post_timeout_t;

typedef struct {
    os_net_queue_t queue;
    os_net_thread_t thread;
    os_net_thread_pid_t pid;
    os_net_sem_t sem;
    os_net_recursive_mutex_t mutex;
    bool is_sync;
    bool enable;
} act_handle_t;

/*
 * 1.ACT_FUNC_NUM is currently set to 20
 * 2.bt   use 0 ~ 9 table number
 * 3.wifi use 10 ~ 19 table number
 * 4.If other modules want to use the api action, needs to define the table number
 */
#define ACT_FUNC_NUM 20

os_net_status_t act_register_handler(act_handle_t *handle, uint8_t id, act_func_t *action);

os_net_status_t act_init(act_handle_t *handle, const char *identifier, bool is_sync);

os_net_status_t act_deinit(act_handle_t *handle);

os_net_status_t act_transfer(act_handle_t *handle, uint8_t id, uint16_t opcode, int call_arg_len,
                             int cb_arg_len, ...);

#define OS_NET_TASK_QUEUE_LEN (24)
#define OS_NET_TASK_STACK_SIZE (4096)
#define OS_NET_TASK_PRIO (0)

#endif /*__ACTION_H_*/
