
#ifndef SYSTEM_GULE_H
#define SYSTEM_GULE_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <FreeRTOS.h>
#include <portmacro.h>
#include <FreeRTOSConfig.h>
#include <task.h>
#include <port_misc.h>
#include <queue.h>
#include <semphr.h>

typedef portBASE_TYPE rt_err_t;
typedef portBASE_TYPE rt_base_t;
typedef uint32_t rt_uint32_t;
typedef int32_t  rt_int32_t;
typedef uint8_t  rt_uint8_t;
typedef size_t   rt_size_t;
typedef int8_t   rt_bool_t;
typedef TaskHandle_t rt_thread_t;

#define RT_TRUE 1
#define RT_FALSE 0
#define RT_NULL NULL

#define RT_EOK                          0               /**< There is no error */
#define RT_ERROR                        1               /**< A generic error happens */
#define RT_ETIMEOUT                     2               /**< Timed out */
#define RT_EFULL                        3               /**< The resource is full */
#define RT_EEMPTY                       4               /**< The resource is empty */
#define RT_ENOMEM                       5               /**< No memory */
#define RT_ENOSYS                       6               /**< No system */
#define RT_EBUSY                        7               /**< Busy */
#define RT_EIO                          8               /**< IO error */
#define RT_EINTR                        9               /**< Interrupted system call */
#define RT_EINVAL                       10              /**< Invalid argument */

#define RT_NAME_MAX 32
#define RT_ALIGN_SIZE 4

#define RT_WAITING_FOREVER              portMAX_DELAY   /**< Block forever until get resource. */
#define RT_WAITING_NO                   0               /**< Non-block. */

#define ULOG_ASYNC_OUTPUT_BUF_SIZE (16 * 1024)
#define ULOG_ASYNC_OUTPUT_THREAD_PRIORITY (configAPPLICATION_NORMAL_PRIORITY - 1)
#define ULOG_ASYNC_OUTPUT_THREAD_STACK 0x1000

#define RT_WEAK __attribute__((weak))

#define RT_ASSERT(x) configASSERT(x)
#define rt_memset memset
#define rt_snprintf snprintf
#define rt_strlen strlen
#define rt_strnlen strnlen
#define rt_strncpy strncpy
#define rt_kprintf printf
#define rt_strncmp strncmp
#define rt_free free
#define rt_memcpy memcpy
#define rt_strstr strstr
#define rt_malloc malloc
#define rt_thread_delay vTaskDelay

#define rt_tick_get xTaskGetTickCount
#define rt_hw_interrupt_enable vPortExitCritical
#define rt_hw_interrupt_disable vPortEnterCritical
#define rt_interrupt_get_nest uGetInterruptNest
#define rt_thread_delete(x) vTaskDelete(NULL)

#define rt_mutex_release xSemaphoreGive
#define rt_mutex_take xSemaphoreTake
#define rt_mutex_detach vSemaphoreDelete

#define rt_sem_release xSemaphoreGive
#define rt_sem_take xSemaphoreTake
#define rt_sem_detach vSemaphoreDelete

#define ULOG_OUTPUT_LVL  CONFIG_LOG_LEVEL

#define rt_inline static __inline

#define rt_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

/**
 * @ingroup BasicDef
 *
 * @def RT_ALIGN(size, align)
 * Return the most contiguous size aligned at specified width. RT_ALIGN(13, 4)
 * would return 16.
 */
#define RT_ALIGN(size, align)           (((size) + (align) - 1) & ~((align) - 1))

/**
 * @ingroup BasicDef
 *
 * @def RT_ALIGN_DOWN(size, align)
 * Return the down number of aligned at specified width. RT_ALIGN_DOWN(13, 4)
 * would return 12.
 */
#define RT_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))

/**
 * Single List structure
 */
struct rt_slist_node
{
    struct rt_slist_node *next;                         /**< point to next node. */
};
typedef struct rt_slist_node rt_slist_t;                /**< Type for single list. */

/**
 * @brief initialize a single list
 *
 * @param l the single list to be initialized
 */
rt_inline void rt_slist_init(rt_slist_t *l)
{
    l->next = RT_NULL;
}

rt_inline void rt_slist_append(rt_slist_t *l, rt_slist_t *n)
{
    struct rt_slist_node *node;

    node = l;
    while (node->next) node = node->next;

    /* append the node to the tail */
    node->next = n;
    n->next = RT_NULL;
}

rt_inline void rt_slist_insert(rt_slist_t *l, rt_slist_t *n)
{
    n->next = l->next;
    l->next = n;
}

rt_inline unsigned int rt_slist_len(const rt_slist_t *l)
{
    unsigned int len = 0;
    const rt_slist_t *list = l->next;
    while (list != RT_NULL)
    {
        list = list->next;
        len ++;
    }

    return len;
}

rt_inline rt_slist_t *rt_slist_remove(rt_slist_t *l, rt_slist_t *n)
{
    /* remove slist head */
    struct rt_slist_node *node = l;
    while (node->next && node->next != n) node = node->next;

    /* remove node */
    if (node->next != (rt_slist_t *)0) node->next = node->next->next;

    return l;
}

rt_inline rt_slist_t *rt_slist_first(rt_slist_t *l)
{
    return l->next;
}

rt_inline rt_slist_t *rt_slist_tail(rt_slist_t *l)
{
    while (l->next) l = l->next;

    return l;
}

rt_inline rt_slist_t *rt_slist_next(rt_slist_t *n)
{
    return n->next;
}

rt_inline int rt_slist_isempty(rt_slist_t *l)
{
    return l->next == RT_NULL;
}

/**
 * @brief get the struct for this single list node
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define rt_slist_entry(node, type, member) \
    rt_container_of(node, type, member)

/**
 * rt_slist_for_each - iterate over a single list
 * @pos:    the rt_slist_t * to use as a loop cursor.
 * @head:   the head for your single list.
 */
#define rt_slist_for_each(pos, head) \
    for (pos = (head)->next; pos != RT_NULL; pos = pos->next)

/**
 * rt_slist_for_each_entry  -   iterate over single list of given type
 * @pos:    the type * to use as a loop cursor.
 * @head:   the head for your single list.
 * @member: the name of the list_struct within the struct.
 */
#define rt_slist_for_each_entry(pos, head, member) \
    for (pos = rt_slist_entry((head)->next, typeof(*pos), member); \
         &pos->member != (RT_NULL); \
         pos = rt_slist_entry(pos->member.next, typeof(*pos), member))

/**
 * rt_slist_first_entry - get the first element from a slist
 * @ptr:    the slist head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the slist_struct within the struct.
 *
 * Note, that slist is expected to be not empty.
 */
#define rt_slist_first_entry(ptr, type, member) \
    rt_slist_entry((ptr)->next, type, member)

/**
 * rt_slist_tail_entry - get the tail element from a slist
 * @ptr:    the slist head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the slist_struct within the struct.
 *
 * Note, that slist is expected to be not empty.
 */
#define rt_slist_tail_entry(ptr, type, member) \
    rt_slist_entry(rt_slist_tail(ptr), type, member)

#endif  /*SYSTEM_GULE_H*/
