#include <stdint.h>
#include <os_net_utils.h>
#include <os_net_sem.h>

os_net_status_t os_net_sem_create(os_net_sem_t *sem, uint32_t init_count, uint32_t max_count)
{
    if (sem_init(sem, 0, init_count) != 0)
        return OS_NET_STATUS_NOMEM;
    return OS_NET_STATUS_OK;
}

os_net_status_t os_net_sem_delete(os_net_sem_t *sem)
{
    if (sem_destroy(sem) != 0) {
        return OS_NET_STATUS_FAILED;
    }
    return OS_NET_STATUS_OK;
}

os_net_status_t os_net_sem_wait(os_net_sem_t *sem, os_net_time_t wait_ms)
{
    if (sem_wait(sem) != 0) {
        return OS_NET_STATUS_FAILED;
    }
    return OS_NET_STATUS_OK;
}

os_net_status_t os_net_sem_release(os_net_sem_t *sem)
{
    if (sem_post(sem) != 0) {
        return OS_NET_STATUS_FAILED;
    }
    return OS_NET_STATUS_OK;
}
