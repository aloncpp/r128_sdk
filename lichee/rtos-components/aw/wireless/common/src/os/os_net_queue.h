#ifndef __OS_NET_QUEUE_H__
#define __OS_NET_QUEUE_H__

#if (OS_NET_LINUX_OS || OS_NET_XRLINK_OS)
#include <ring_buff.h>
typedef ring_buff_t os_net_queue_t;
#endif

#ifdef OS_NET_FREERTOS_OS
#include <ring_buff.h>
typedef ring_buff_t os_net_queue_t;
#endif

os_net_status_t os_net_queue_create(os_net_queue_t *queue, uint32_t queue_len, uint32_t msg_size);
os_net_status_t os_net_queue_send(os_net_queue_t *queue, void *msg, uint32_t msg_size, int timeout);
os_net_status_t os_net_queue_receive(os_net_queue_t *queue, void *msg, uint32_t msg_size,
                                     int timeout);
os_net_status_t os_net_queue_delete(os_net_queue_t *queue);

#endif
