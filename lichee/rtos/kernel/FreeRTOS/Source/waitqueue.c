/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018/06/26     Bernard      Fix the wait queue issue when wakeup a soon
 *                             to blocked thread.
 */

#include <stdint.h>
#include <waitqueue.h>

void rt_wqueue_add(rt_wqueue_t *queue, struct rt_wqueue_node *node)
{
    rt_base_t level;

    level = vPortEnterCritical();
    rt_list_insert_before(&(queue->waiting_list), &(node->list));
    vPortExitCritical(level);
}

void rt_wqueue_remove(struct rt_wqueue_node *node)
{
    rt_base_t level;

    level = vPortEnterCritical();
    rt_list_remove(&(node->list));
    vPortExitCritical(level);
}

int __wqueue_default_wake(struct rt_wqueue_node *wait, void *key)
{
    return 0;
}

extern uint32_t uGetInterruptNest(void);
void rt_wqueue_wakeup(rt_wqueue_t *queue, void *key)
{
    rt_base_t level;
    register int need_schedule = 0;

    rt_list_t *queue_list;
    struct rt_list_node *node;
    struct rt_wqueue_node *entry;
    long data = 0;
    BaseType_t pxHigherPriorityTaskWoken;

    queue_list = &(queue->waiting_list);

    level = vPortEnterCritical();
    /* set wakeup flag in the queue */
    queue->flag = RT_WQ_FLAG_WAKEUP;

    if (!(rt_list_isempty(queue_list)))
    {
        for (node = queue_list->next; node != queue_list; node = node->next)
        {
            entry = list_entry(node, struct rt_wqueue_node, list);
            if (entry->wakeup(entry, key) == 0)
            {
		vPortExitCritical(level);
		if (uGetInterruptNest()) {
			xQueueSendFromISR(queue->xQueue, &data, &pxHigherPriorityTaskWoken);
		} else {
			xQueueSend(queue->xQueue, &data, portMAX_DELAY);
		}
		level = vPortEnterCritical();
                need_schedule = 1;
                rt_wqueue_remove(entry);
                break;
            }
        }
    }
    vPortExitCritical(level);

    if (need_schedule)
    {
	void vTaskScheduerEnable(void);
        vTaskScheduerEnable();
    }
}

