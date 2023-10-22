#ifndef POLL_H
#define POLL_H

#include <vfs.h>
#include <task.h>
#include <queue.h>
#include <waitqueue.h>

struct rt_poll_node;

struct rt_poll_table
{
    rt_pollreq_t req;
    uint32_t triggered; /* the waited thread whether triggered */
    TaskHandle_t polling_thread;
    struct rt_poll_node *nodes;
    QueueHandle_t xQueue;
};

struct rt_poll_node
{
    struct rt_wqueue_node wqn;
    struct rt_poll_table *pt;
    struct rt_poll_node *next;
};

#endif  /*POLL_H*/
