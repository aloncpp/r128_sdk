#include <os_net_utils.h>
#include <os_net_mutex.h>

os_net_status_t os_net_mutex_create(os_net_mutex_t *mutex)
{
    if (pthread_mutex_init(mutex, NULL) != 0) {
        return OS_NET_STATUS_NOMEM;
    }
    return OS_NET_STATUS_OK;
}

os_net_status_t os_net_mutex_delete(os_net_mutex_t *mutex)
{
    if (pthread_mutex_destroy(mutex) != 0) {
        return OS_NET_STATUS_FAILED;
    }
    return OS_NET_STATUS_OK;
}

os_net_status_t os_net_mutex_lock(os_net_mutex_t *mutex)
{
    pthread_mutex_lock(mutex);
    return OS_NET_STATUS_OK;
}

os_net_status_t os_net_mutex_unlock(os_net_mutex_t *mutex)
{
    pthread_mutex_unlock(mutex);
    return OS_NET_STATUS_OK;
}

os_net_status_t os_net_mutex_recursive_create(os_net_recursive_mutex_t *mutex)
{
    if (pthread_mutexattr_init(&mutex->attr) != 0) {
        return OS_NET_STATUS_NOMEM;
    }
    if (pthread_mutexattr_settype(&mutex->attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
        return OS_NET_STATUS_NOMEM;
    }
    if (pthread_mutex_init(&mutex->mutex, NULL) != 0) {
        return OS_NET_STATUS_NOMEM;
    }
    pthread_mutexattr_destroy(&mutex->attr);
    return OS_NET_STATUS_OK;
}

os_net_status_t os_net_mutex_recursive_delete(os_net_recursive_mutex_t *mutex)
{
    if (pthread_mutex_destroy(&mutex->mutex) != 0) {
        return OS_NET_STATUS_FAILED;
    }
    return OS_NET_STATUS_OK;
}

os_net_status_t os_net_mutex_recursive_lock(os_net_recursive_mutex_t *mutex)
{
    pthread_mutex_lock(&mutex->mutex);
    return OS_NET_STATUS_OK;
}

os_net_status_t os_net_mutex_recursive_unlock(os_net_recursive_mutex_t *mutex)
{
    pthread_mutex_unlock(&mutex->mutex);
    return OS_NET_STATUS_OK;
}
