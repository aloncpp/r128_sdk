#ifndef __OS_NET_THREAD_H__
#define __OS_NET_THREAD_H__

#if (OS_NET_LINUX_OS || OS_NET_XRLINK_OS)
#include <pthread.h>
typedef pthread_t os_net_thread_t;
typedef uint8_t os_net_thread_prio_t;
typedef void *(*os_net_thread_func_t)(void *arg);
typedef pthread_t os_net_thread_pid_t;
#endif

#ifdef OS_NET_FREERTOS_OS
#include <pthread.h>
typedef pthread_t os_net_thread_t;
typedef uint8_t os_net_thread_prio_t;
typedef void *(*os_net_thread_func_t)(void *arg);
typedef pthread_t os_net_thread_pid_t;
#endif

#include <os_net_utils.h>

os_net_status_t os_net_thread_create(os_net_thread_t *thread_handle, const char *name,
                                     os_net_thread_func_t func, void *arg, os_net_thread_prio_t pri,
                                     uint32_t task_size);

os_net_status_t os_net_thread_delete(os_net_thread_t *thread_handle);

os_net_status_t os_net_thread_get_pid(os_net_thread_pid_t *pid);

#endif
