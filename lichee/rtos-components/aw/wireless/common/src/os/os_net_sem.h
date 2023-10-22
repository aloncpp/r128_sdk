
#ifndef __OS_NET_SEM_H__
#define __OS_NET_SEM_H__

#if (OS_NET_LINUX_OS || OS_NET_XRLINK_OS)
#include <semaphore.h>
typedef uint32_t os_net_time_t;
typedef sem_t os_net_sem_t;
#endif

#ifdef OS_NET_FREERTOS_OS
#include <semaphore.h>
typedef uint32_t os_net_time_t;
typedef sem_t os_net_sem_t;
#endif
os_net_status_t os_net_sem_create(os_net_sem_t *sem, uint32_t init_count, uint32_t max_count);
os_net_status_t os_net_sem_delete(os_net_sem_t *sem);

os_net_status_t os_net_sem_wait(os_net_sem_t *sem, os_net_time_t wait_ms);
os_net_status_t os_net_sem_release(os_net_sem_t *sem);
#endif
