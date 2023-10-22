#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <os_net_utils.h>
#include <os_net_thread.h>

os_net_status_t os_net_thread_create(os_net_thread_t *thread_handle, const char *name,
                                     os_net_thread_func_t func, void *arg, os_net_thread_prio_t pri,
                                     uint32_t task_size)
{
#if 1
	pthread_attr_t attr;
    pthread_attr_init(&attr);
    struct sched_param sched;
    sched.sched_priority = pri;
    pthread_attr_setschedparam(&attr, &sched);
    pthread_attr_setstacksize(&attr, task_size);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(thread_handle, &attr, func, arg) != 0) {
        return OS_NET_STATUS_NOMEM;
    }
#endif
#if 0
    if (pthread_create(thread_handle, NULL, func, arg) != 0) {
        return OS_NET_STATUS_NOMEM;
    }
#endif
#ifdef _GNU_SOURCE
    if (pthread_setname_np(*thread_handle, name, strlen(name)) != 0) {
        OS_NET_ERROR("set name failed\n");
        return OS_NET_STATUS_NOMEM;
    }
#endif
    return OS_NET_STATUS_OK;
}

os_net_status_t os_net_thread_delete(os_net_thread_t *thread_handle)
{
    if (pthread_cancel(*thread_handle) != 0) {
        return OS_NET_STATUS_FAILED;
    }
    if (pthread_join(*thread_handle, NULL) != 0) {
        return OS_NET_STATUS_FAILED;
    }
    return OS_NET_STATUS_OK;
}

os_net_status_t os_net_thread_get_pid(os_net_thread_pid_t *pid)
{
    *pid = pthread_self();
    return OS_NET_STATUS_OK;
}
