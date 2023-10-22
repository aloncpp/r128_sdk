#ifndef __OS_NET_MUTEX_H__
#define __OS_NET_MUTEX_H__

#if (OS_NET_LINUX_OS || OS_NET_XRLINK_OS)
#include <pthread.h>
typedef pthread_mutex_t os_net_mutex_t;
typedef struct {
    pthread_mutex_t mutex;
    pthread_mutexattr_t attr;
} os_net_recursive_mutex_t;
#endif


#ifdef OS_NET_FREERTOS_OS
#include <pthread.h>
typedef pthread_mutex_t os_net_mutex_t;
typedef struct {
    pthread_mutex_t mutex;
    pthread_mutexattr_t attr;
} os_net_recursive_mutex_t;
#endif

os_net_status_t os_net_mutex_create(os_net_mutex_t *mutex);

os_net_status_t os_net_mutex_delete(os_net_mutex_t *mutex);

os_net_status_t os_net_mutex_lock(os_net_mutex_t *mutex);

os_net_status_t os_net_mutex_unlock(os_net_mutex_t *mutex);

os_net_status_t os_net_mutex_recursive_create(os_net_recursive_mutex_t *mutex);

os_net_status_t os_net_mutex_recursive_delete(os_net_recursive_mutex_t *mutex);

os_net_status_t os_net_mutex_recursive_lock(os_net_recursive_mutex_t *mutex);

os_net_status_t os_net_mutex_recursive_unlock(os_net_recursive_mutex_t *mutex);
#endif
