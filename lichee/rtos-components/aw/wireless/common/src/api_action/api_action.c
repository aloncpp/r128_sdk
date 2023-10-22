#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <os_net_utils.h>
#include <api_action.h>
#include <stdarg.h>
#include <hal_time.h>

typedef struct {
    uint16_t opcode;
    void **call_arg;
    void **cb_arg;
} act_trans_args;

typedef struct {
    uint8_t id;
    uint8_t opcode;
    void **call_arg;
    void **cb_arg;
} act_msg_t;

/*
 * 1.ACT_FUNC_NUM is currently set to 20
 * 2.bt   use 0 ~ 9 table number
 * 3.wifi use 10 ~ 19 table number
 * 4.If other modules want to use the api action, needs to define the table number
 */
static act_func_t *act_funcs[ACT_FUNC_NUM] = { 0 };

static os_net_status_t act_task_post(act_handle_t *handle, act_msg_t **msg,
                                     act_post_timeout_t timeout)
{
    if (msg == NULL) {
        return OS_NET_STATUS_PARAM_INVALID;
    }

    if (os_net_queue_send(&handle->queue, msg, sizeof(act_msg_t *), timeout) != OS_NET_STATUS_OK) {
        return OS_NET_STATUS_BUSY;
    }

    return OS_NET_STATUS_OK;
}

static os_net_status_t act_transfer_signal(act_handle_t *handle)
{
    return os_net_sem_release(&handle->sem);
}

static os_net_status_t act_transfer_wait(act_handle_t *handle)
{
    return os_net_sem_wait(&handle->sem, OS_NET_WAIT_FOREVER);
}

os_net_status_t act_transfer_handle(act_handle_t *handle, uint8_t id, act_trans_args *trans)
{
    act_msg_t *msg;

    os_net_thread_pid_t pid;

    if (handle->enable == false) {
        return OS_NET_STATUS_NOT_READY;
    }

    msg = (act_msg_t *)malloc(sizeof(act_msg_t));
    if (msg == NULL) {
        return OS_NET_STATUS_NOMEM;
    }
    memset(msg, 0, sizeof(act_msg_t));

    msg->id = id;
    msg->opcode = trans->opcode;

    if (NULL == act_funcs[id]) {
        if (msg != NULL)
            free(msg);
        return OS_NET_STATUS_PARAM_INVALID;
    }

    //OS_NET_DEBUG("start - id:%d,opcode:%d,%s", id, msg->opcode, act_funcs[id][msg->opcode].name);

    os_net_thread_get_pid(&pid);

    //running in the tranfer thread
    if (handle->pid == pid && act_funcs[msg->id][msg->opcode].action) {
        if (act_funcs[msg->id]) {
            act_funcs[msg->id][msg->opcode].action(trans->call_arg, trans->cb_arg);
        }
        if (msg != NULL)
            free(msg);
        return OS_NET_STATUS_OK;
    }

    if (handle->is_sync) {
        os_net_mutex_recursive_lock(&handle->mutex);
        if (trans->cb_arg) {
            msg->cb_arg = trans->cb_arg;
        }
    }

    if (trans->call_arg) {
        msg->call_arg = trans->call_arg;
    }

    if (act_task_post(handle, &msg, OS_NET_TASK_POST_BLOCKING) != OS_NET_STATUS_OK) {
        if (msg != NULL)
            free(msg);
        return OS_NET_STATUS_BUSY;
    }

    if (handle->is_sync) {
        if (act_transfer_wait(handle) != OS_NET_STATUS_OK) {
            if (msg != NULL)
                free(msg);
            return OS_NET_STATUS_BUSY;
        }
    }

    if (handle->is_sync) {
        os_net_mutex_recursive_unlock(&handle->mutex);
    }

    //OS_NET_DEBUG("end - id:%d,opcode:%d,%s", id, msg->opcode, act_funcs[id][msg->opcode].name);

    if (msg != NULL)
        free(msg);

    return OS_NET_STATUS_OK;
}

os_net_status_t act_transfer(act_handle_t *handle, uint8_t id, uint16_t opcode, int call_arg_len,
                             int cb_arg_len, ...)
{
    va_list ap;
    int tot_arg = 0;
    void **call_argv = NULL;
    void **cb_argv = NULL;
    act_trans_args args;
    int i = 0;
    int j = 0;
    os_net_status_t status;

    if (call_arg_len == 0 && cb_arg_len == 0)
        goto handle;

    if (call_arg_len > 0) {
        call_argv = (void **)malloc(sizeof(void *) * call_arg_len);
		if(call_argv == NULL) {
            status = OS_NET_STATUS_NOMEM;
            goto error;
        }
    }

    if (cb_arg_len > 0) {
        cb_argv = (void **)malloc(sizeof(void *) * cb_arg_len);
		if(cb_argv == NULL) {
            status = OS_NET_STATUS_NOMEM;
            goto error;
        }
    }

    tot_arg = call_arg_len + cb_arg_len;
	cb_arg_len = tot_arg;
    va_start(ap, cb_arg_len);

    for (i = 0; i < tot_arg; i++) {
        if (i < call_arg_len)
            call_argv[i] = va_arg(ap, void *);
        else
            cb_argv[j++] = va_arg(ap, void *);
    }

handle:

    args.opcode = opcode;
    args.call_arg = call_argv;
    args.cb_arg = cb_argv;

    status = act_transfer_handle(handle, id, &args);

    va_end(ap);

error:
    if (call_arg_len > 0) {
        if(call_argv != NULL) {
            free(call_argv);
        }
    }

    if (cb_arg_len > 0) {
        if(cb_argv != NULL) {
            free(cb_argv);
        }
    }

    return status;
}

static void *os_thread_handler(void *arg)
{
    act_msg_t *msg;

    act_handle_t *handle = (act_handle_t *)arg;

    if (NULL == handle) {
        return NULL;
    }

    os_net_thread_get_pid(&handle->pid);

    for (;;) {
        if (OS_NET_STATUS_OK ==
            os_net_queue_receive(&handle->queue, &msg, sizeof(act_msg_t *), OS_NET_WAIT_FOREVER)) {
            //OS_NET_DEBUG("msg context(%d,%d)", msg->id, msg->opcode);

            if (act_funcs[msg->id] && act_funcs[msg->id][msg->opcode].action) {
                act_funcs[msg->id][msg->opcode].action(msg->call_arg, msg->cb_arg);
            }

            if (handle->is_sync) {
                act_transfer_signal(handle);
            }
#if 0
			if(msg.call_arg) {
				free(msg.call_arg);
			}
#endif
        }
    }
    return NULL;
}

os_net_status_t act_register_handler(act_handle_t *handle, uint8_t id, act_func_t *action)
{
    if (id >= ACT_FUNC_NUM) {
        return OS_NET_STATUS_NOMEM;
    }

    act_funcs[id] = action;

    return OS_NET_STATUS_OK;
}

os_net_status_t act_init(act_handle_t *handle, const char *identifier, bool is_sync)
{
    handle->is_sync = is_sync;
    if (handle->is_sync) {
        if (os_net_mutex_recursive_create(&handle->mutex) != OS_NET_STATUS_OK) {
            OS_NET_ERROR("create mutex failed.\n");
            return OS_NET_STATUS_NOMEM;
        }
        if (os_net_sem_create(&handle->sem, 0, OS_NET_TASK_QUEUE_LEN) != OS_NET_STATUS_OK) {
            OS_NET_ERROR("create action sem failed.\n");
            return OS_NET_STATUS_NOMEM;
        }
    }

    if (os_net_queue_create(&handle->queue, OS_NET_TASK_QUEUE_LEN, sizeof(act_msg_t)) !=
        OS_NET_STATUS_OK) {
        OS_NET_ERROR("create action queue failed.\n");
        return OS_NET_STATUS_NOMEM;
    }

    if (os_net_thread_create(&handle->thread, identifier, os_thread_handler, handle,
                             OS_NET_TASK_PRIO, OS_NET_TASK_STACK_SIZE) != OS_NET_STATUS_OK) {
        return OS_NET_STATUS_NOMEM;
    }

    handle->enable = true;

    return OS_NET_STATUS_OK;
}

os_net_status_t act_deinit(act_handle_t *handle)
{
    if (handle->enable == false)
        return OS_NET_STATUS_NOT_READY;

    os_net_thread_delete(&handle->thread);
    hal_usleep(10);
    os_net_queue_delete(&handle->queue);
    if (handle->is_sync) {
        os_net_sem_delete(&handle->sem);
        os_net_mutex_recursive_delete(&handle->mutex);
    }
    handle->enable = false;
}
