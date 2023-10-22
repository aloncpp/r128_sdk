/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2016-12-28     Bernard      first version
 * 2018-03-09     Bernard      Add protection for pt->triggered.
 */
#include <stdint.h>
#include <waitqueue.h>
#include <io_multi_poll.h>
#include <poll.h>

static int __wqueue_pollwake(struct rt_wqueue_node *wait, void *key)
{
    struct rt_poll_node *pn;

    if (key && !((uint32_t)key & wait->key))
        return -1;

    pn = container_of(wait, struct rt_poll_node, wqn);
    pn->pt->triggered = 1;

    return 0;
}

static void _poll_add(rt_wqueue_t *wq, rt_pollreq_t *req)
{
    struct rt_poll_table *pt;
    struct rt_poll_node *node;

    node = (struct rt_poll_node *)rt_malloc(sizeof(struct rt_poll_node));
    if (node == RT_NULL)
    {
        return;
    }

    memset(node, 0x00, sizeof(*node));
    pt = container_of(req, struct rt_poll_table, req);

    rt_list_init(&(node->wqn.list));
    node->wqn.key = req->_key;
    node->wqn.polling_thread = pt->polling_thread;
    node->wqn.wakeup = __wqueue_pollwake;
    node->next = pt->nodes;
    node->pt = pt;
    pt->nodes = node;
    rt_wqueue_add(wq, &node->wqn);
}

static int poll_table_init(struct rt_poll_table *pt)
{
    pt->req._proc = _poll_add;
    pt->triggered = 0;
    pt->nodes = RT_NULL;
    pt->polling_thread = xTaskGetCurrentTaskHandle();
    pt->xQueue = xQueueCreate(8, sizeof(long));
    if (pt->xQueue) {
	return 0;
    }
    return -1;
}

static int32_t rt_tick_from_millisecond(int32_t ms)
{
    int32_t tick;

    if (ms < 0)
    {
        tick = portMAX_DELAY;
    }
    else
    {
        tick = configTICK_RATE_HZ * (ms / 1000);
        tick += (configTICK_RATE_HZ * (ms % 1000) + 999) / 1000;
    }

    /* return the calculated tick */
    return tick;
}
static int poll_wait_timeout(struct rt_poll_table *pt, int msec)
{
    int32_t timeout;
    int ret = 0;
    TaskHandle_t thread;
    rt_base_t level;
    TimerHandle_t xTimer;
    void *buffer;

    thread = pt->polling_thread;

    int32_t rt_tick_from_millisecond(int32_t ms);
    timeout = rt_tick_from_millisecond(msec);

    level = vPortEnterCritical();

    if (timeout != 0 && !pt->triggered)
    {
        if (timeout > 0)
        {
		vPortExitCritical(level);
		xQueueReceive(pt->xQueue, &buffer, timeout);
		level = vPortEnterCritical();
        }
    }

    ret = !pt->triggered;
    vPortExitCritical(level);
    return ret;
}

static int do_pollfd(struct pollfd *pollfd, rt_pollreq_t *req)
{
    int mask = 0;
    int infd;
    int localfd;

    infd = pollfd->fd;

    if (infd >= 0)
    {
        vfs_t *f = vfs_fd_get(infd, &localfd);
        mask = POLLNVAL;

        if (f)
        {
            mask = POLLMASK_DEFAULT;
            if (f->poll)
            {
                req->_key = pollfd->events | POLLERR | POLLHUP;

                mask = f->poll(localfd, req);
            }
            /* Mask out unneeded events. */
            mask &= pollfd->events | POLLERR | POLLHUP;
        }
    }
    pollfd->revents = mask;
    return mask;
}

static int poll_do(struct pollfd *fds, nfds_t nfds, struct rt_poll_table *pt, int msec)
{
    int num;
    int istimeout = 0;
    int n;
    struct pollfd *pf;

    if (msec == 0)
    {
        pt->req._proc = RT_NULL;
        istimeout = 1;
    }

    while (1)
    {
        pf = fds;
        num = 0;

        for (n = 0; n < nfds; n ++)
        {
            if (do_pollfd(pf, &pt->req))
            {
                num ++;
                pt->req._proc = RT_NULL;
            }
            pf ++;
        }

        pt->req._proc = RT_NULL;

        if (num || istimeout) {
		break;
	}

        if (poll_wait_timeout(pt, msec)) {
            istimeout = 1;
	}
    }

    return num;
}

static void poll_teardown(struct rt_poll_table *pt)
{
    struct rt_poll_node *node, *next;

    next = pt->nodes;
    while (next)
    {
        node = next;
        rt_wqueue_remove(&node->wqn);
        next = node->next;
        rt_free(node);
    }
    if (pt->xQueue) {
	vQueueDelete(pt->xQueue);
    }
}

int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    int num;
    struct rt_poll_table table;

    if (poll_table_init(&table)) {
	return -1;
    }

    num = poll_do(fds, nfds, &table, timeout);

    poll_teardown(&table);

    return num;
}

