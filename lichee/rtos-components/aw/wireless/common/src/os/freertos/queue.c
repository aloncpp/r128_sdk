#include <os_net_utils.h>
#include <os_net_queue.h>
#include <ring_buff.h>

os_net_status_t os_net_queue_create(os_net_queue_t *queue, uint32_t queue_len, uint32_t msg_size)
{
    if (ring_buff_init(queue, queue_len * msg_size) != 0) {
        return OS_NET_STATUS_NOMEM;
    }
    ring_buff_start(queue);
    return OS_NET_STATUS_OK;
}

os_net_status_t os_net_queue_send(os_net_queue_t *queue, void *msg, uint32_t msg_size, int timeout)
{
    if (ring_buff_put(queue, msg, msg_size, 0, RB_SG_NOMAL) == 0) {
        return OS_NET_STATUS_NOMEM;
    }
    return OS_NET_STATUS_OK;
}

os_net_status_t os_net_queue_receive(os_net_queue_t *queue, void *msg, uint32_t msg_size,
                                     int timeout)
{
    if (ring_buff_get(queue, msg, msg_size, timeout, 0) == 0) {
        return OS_NET_STATUS_FAILED;
    }
    return OS_NET_STATUS_OK;
}

os_net_status_t os_net_queue_delete(os_net_queue_t *queue)
{
    ring_buff_stop(queue);
    ring_buff_deinit(queue);

    return OS_NET_STATUS_OK;
}
