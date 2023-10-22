#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <aw_list.h>
#include <FreeRTOS.h>
#include <task.h>
#include <port_misc.h>
#include <stdarg.h>

#include <hal_cmd.h>
#include <hal_atomic.h>

#ifdef CONFIG_DEBUG_BACKTRACE
#include <backtrace.h>
#endif

typedef struct aw_malloc_context
{
    struct list_head    list;
    struct list_head    filter_thread_list;
    int32_t             init_flag;
    int32_t             open_flag;
} malloc_context_t;

typedef struct heap_buffer_node
{
    struct list_head  i_list;
    void              *ptr;
    uint32_t          size;
    uint32_t          entry;
    char              *name;
#ifdef CONFIG_DEBUG_BACKTRACE
    void              *caller[CONFIG_MEMORY_LEAK_BACKTRACE_LEVEL];
#endif
    void              *reent;
} heap_buffer_node_t;

extern void __internal_free(void *ptr);
extern void *__internal_malloc(uint32_t size);

extern void memleak_malloc_sethook(void (*hook)(void *ptr, uint32_t size));
extern void memleak_free_sethook(void (*hook)(void *ptr));

static int backtrace_level = 1;
static int memleak_filter_flag = 0;

static hal_spinlock_t memleak_lock;

static malloc_context_t g_mem_leak_list =
{
    .list       = LIST_HEAD_INIT(g_mem_leak_list.list),
    .filter_thread_list = LIST_HEAD_INIT(g_mem_leak_list.filter_thread_list),
    .init_flag   = 0,
    .open_flag  = 0,
};

typedef struct malloc_filter
{
    struct list_head  i_list;
    char name[configMAX_TASK_NAME_LEN];
}malloc_filter_t;

static void memleak_detect_init(void)
{
    uint32_t cpsr_flag;

    hal_spin_lock_init(&memleak_lock);

    hal_spin_lock(&memleak_lock);

    if (g_mem_leak_list.open_flag)
    {
        hal_spin_unlock(&memleak_lock);
        return;
    }
    INIT_LIST_HEAD(&g_mem_leak_list.list);
    INIT_LIST_HEAD(&g_mem_leak_list.filter_thread_list);
    g_mem_leak_list.init_flag = 1;

    hal_spin_unlock(&memleak_lock);
}

static void memleak_detect_enable(void)
{
    hal_spin_lock(&memleak_lock);

    if (g_mem_leak_list.open_flag)
    {
        hal_spin_unlock(&memleak_lock);
        return;
    }
    g_mem_leak_list.open_flag = 1;

    hal_spin_unlock(&memleak_lock);
}


static void memleak_detect_disable(void)
{
    struct list_head *pos;
    struct list_head *q;
    uint32_t count = 1;
    int i = 0;

    hal_spin_lock(&memleak_lock);

    list_for_each_safe(pos, q, &g_mem_leak_list.list)
    {
        heap_buffer_node_t *tmp;
        tmp = list_entry(pos, heap_buffer_node_t, i_list);
        printf("    %03d: ptr = 0x%08x, size = 0x%08x, thread = %s\n", \
                   count ++, (uint32_t)tmp->ptr, (uint32_t)tmp->size, tmp->name);
#ifdef CONFIG_DEBUG_BACKTRACE
        for (i = 0; i < CONFIG_MEMORY_LEAK_BACKTRACE_LEVEL; i++)
        {
            if (tmp->caller[i] != NULL)
            {
                printf("        backtrace : 0x%p\n", tmp->caller[i]);
            }
        }
#endif
        list_del(pos);
        free(tmp);
    }

    list_for_each_safe(pos, q, &g_mem_leak_list.filter_thread_list)
    {
        malloc_filter_t *tmp;
        tmp = list_entry(pos, malloc_filter_t, i_list);

        list_del(pos);
        free(tmp);
    }

    g_mem_leak_list.init_flag = 0;
    g_mem_leak_list.open_flag = 0;

    hal_spin_unlock(&memleak_lock);
    hal_spin_lock_deinit(&memleak_lock);
}

static void memleak_detect_show(void)
{
    struct list_head *pos;
    struct list_head *q;
    uint32_t count = 1;
    int i = 0;

    hal_spin_lock(&memleak_lock);

    list_for_each_safe(pos, q, &g_mem_leak_list.list)
    {
        heap_buffer_node_t *tmp;
        tmp = list_entry(pos, heap_buffer_node_t, i_list);
        printf("\t %03d: ptr = 0x%08x, size = 0x%08x, thread = %s\n", \
                   count ++, (uint32_t)tmp->ptr, (uint32_t)tmp->size, tmp->name);
#ifdef CONFIG_DEBUG_BACKTRACE
        for (i = 0; i < CONFIG_MEMORY_LEAK_BACKTRACE_LEVEL; i++)
        {
            if (tmp->caller[i] != NULL)
            {
                printf("                backtrace : 0x%p\n", tmp->caller[i]);
            }
        }
#endif
    }

    count = 1;
    list_for_each_safe(pos, q, &g_mem_leak_list.filter_thread_list)
    {
        malloc_filter_t *tmp;
        tmp = list_entry(pos, malloc_filter_t, i_list);

        printf("\t %03d: filter thread = %s\n", \
                   count ++, tmp->name);
    }

    hal_spin_unlock(&memleak_lock);
}

static void malloc_filter_add_threadname(const char * name)
{
    malloc_filter_t *new;

    hal_spin_lock(&memleak_lock);
    if(g_mem_leak_list.init_flag == 0)
    {
        hal_spin_unlock(&memleak_lock);
        return;
    }

    new = __internal_malloc(sizeof(malloc_filter_t));
    if (new == NULL)
    {
        hal_spin_unlock(&memleak_lock);
        return;
    }

    memset(new, 0x00, sizeof(malloc_filter_t));

    INIT_LIST_HEAD(&new->i_list);

    memcpy(new->name, name, strlen(name) >= configMAX_TASK_NAME_LEN ? configMAX_TASK_NAME_LEN - 1 : strlen(name));

    list_add(&new->i_list, &g_mem_leak_list.filter_thread_list);
    hal_spin_unlock(&memleak_lock);
}

static int memleak_filter_lookup(TaskHandle_t task)
{
    int filter_flag = 0;
    struct list_head *pos;
    struct list_head *q;

    hal_spin_lock(&memleak_lock);

    list_for_each_safe(pos, q, &g_mem_leak_list.filter_thread_list)
    {
        malloc_filter_t *tmp;
        tmp = list_entry(pos, malloc_filter_t, i_list);

        if(task)
        {
            if(!strcmp(tmp->name, pcTaskGetName(task)))
            {
                filter_flag = 1;
                break;
            }
        }
        else
        {
            if(!strcmp(tmp->name, "ISR"))
            {
                filter_flag = 1;
                break;
            }
        }
    }
    hal_spin_unlock(&memleak_lock);
    return filter_flag;
}

static void memleak_register_malloc_hook(void * ptr, uint32_t size)
{
    uint32_t caller;

    heap_buffer_node_t *new = NULL;

    hal_spin_lock(&memleak_lock);

    if (g_mem_leak_list.open_flag)
    {
        TaskHandle_t task = xTaskGetCurrentTaskHandle();

        if(memleak_filter_flag && !memleak_filter_lookup(task))
        {
            goto exit;
        }

        new = __internal_malloc(sizeof(heap_buffer_node_t));
        if (new == NULL)
        {
            goto exit;
        }

        memset(new, 0x00, sizeof(heap_buffer_node_t));

        INIT_LIST_HEAD(&new->i_list);

        new->size = size;
        new->ptr = ptr;
#ifdef CONFIG_DEBUG_BACKTRACE
        backtrace(NULL, new->caller, CONFIG_MEMORY_LEAK_BACKTRACE_LEVEL, 3, NULL);
#endif

        if(task)
        {
            new->name = pcTaskGetName(task);
        }
        else if(uGetInterruptNest())
        {
            new->name = "ISR";
        }

        list_add(&new->i_list, &g_mem_leak_list.list);
    }
exit:
    hal_spin_unlock(&memleak_lock);
}

static void memleak_register_free_hook(void *ptr)
{
    struct list_head *pos;
    struct list_head *q;
    heap_buffer_node_t   *tmp;
    int found = 0;

    hal_spin_lock(&memleak_lock);

    if (g_mem_leak_list.open_flag)
    {
        list_for_each_safe(pos, q, &g_mem_leak_list.list)
        {
            tmp = list_entry(pos, heap_buffer_node_t, i_list);
            if (tmp->ptr == ptr)
            {
                list_del(pos);
                __internal_free(tmp);
                found = 1;
                break;
            }
        }
#ifdef CONFIG_DOUBLE_FREE_CHECK
        if (found == 0 && ptr != NULL)
        {
            printf("double free checked!!!!\r\n");
            backtrace(NULL, NULL, 0, 0, printf);
        }
        else if (ptr == NULL)
        {
            printf("free null pointer!!!!\r\n");
            backtrace(NULL, NULL, 0, 0, printf);
        }
#endif
    }
    hal_spin_unlock(&memleak_lock);
}

#ifdef CONFIG_DOUBLE_FREE_CHECK
void memleak_double_free_check_init(void)
{
    memleak_detect_init();
    memleak_malloc_sethook(memleak_register_malloc_hook);
    memleak_free_sethook(memleak_register_free_hook);
    memleak_detect_enable();
}
#endif

static void print_memleak_usage(void)
{
    printf("Usage: memleak [<1/0/show>] [<thread_name1> <thread_name2> <thread_name3>]\n");
}

int cmd_memleak(int argc, char ** argv)
{
    int i;

    if(argc == 1)
    {
        print_memleak_usage();
        return -1;
    }

    if(!strcmp(argv[1], "0"))
    {
        memleak_detect_disable();
        memleak_malloc_sethook(NULL);
        memleak_free_sethook(NULL);
    }
    else if(!strcmp(argv[1], "1"))
    {
        memleak_detect_init();
        memleak_malloc_sethook(memleak_register_malloc_hook);
        memleak_free_sethook(memleak_register_free_hook);

        for(i = 2; i < argc; i++)
        {
            malloc_filter_add_threadname(argv[i]);
        }
        if(!list_empty(&g_mem_leak_list.filter_thread_list))
        {
            memleak_filter_flag = 1;
        }
        memleak_detect_enable();
    }
    else if (!strcmp(argv[1], "show"))
    {
        memleak_detect_show();
    }
    else
    {
        print_memleak_usage();
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_memleak, memleak, Memory Check);

int cmd_memallocate(int argc, char ** argv)
{
    struct list_head *pos;
    struct list_head *q;
    heap_buffer_node_t   *tmp;
    int i = 0;

    if(argc != 2)
    {
        printf("Usage: memallocate [<thread_name>]\n");
        return -1;
    }

    uint32_t count = 1;
    hal_spin_lock(&memleak_lock);

    if (g_mem_leak_list.open_flag)
    {
        list_for_each_safe(pos, q, &g_mem_leak_list.list)
        {
            tmp = list_entry(pos, heap_buffer_node_t, i_list);
            if (tmp->name && !strcmp(tmp->name, argv[1]))
            {
                printf("    %03d: ptr = 0x%08x, size = 0x%08x.\n", \
                   count ++, (uint32_t)tmp->ptr, (uint32_t)tmp->size);
#ifdef CONFIG_DEBUG_BACKTRACE
                for (i = 0; i < CONFIG_MEMORY_LEAK_BACKTRACE_LEVEL; i++)
                {
                    if (tmp->caller[i] != NULL)
                    {
                        printf("        backtrace : 0x%p\n", tmp->caller[i]);
                    }
                }
#endif
            }
        }
    }
    hal_spin_unlock(&memleak_lock);
}
FINSH_FUNCTION_EXPORT_CMD(cmd_memallocate, memallocate, Thread Memory Allocate Infomation);

void memleak_detect_start(int num, const char *thread_name, ...)
{
    va_list valist;
    char * para;

    if (g_mem_leak_list.open_flag)
    {
        printf("Memory Leak Check have been opened\n");
        return;
    }

    memleak_detect_init();
    memleak_malloc_sethook(memleak_register_malloc_hook);
    memleak_free_sethook(memleak_register_free_hook);

    if(thread_name == NULL || num <= 0)
    {
        memleak_detect_enable();
        return;
    }
    malloc_filter_add_threadname(thread_name);
    num--;

    va_start(valist, thread_name);

    while(num--)
    {
        para = va_arg(valist, char *);
        if(para == NULL)
        {
            break;
        }
        malloc_filter_add_threadname(para);
    }
    va_end(valist);

    if(!list_empty(&g_mem_leak_list.filter_thread_list))
    {
        memleak_filter_flag = 1;
    }

    memleak_detect_enable();
}

void memleak_detect_stop(void)
{
    memleak_detect_disable();
    memleak_malloc_sethook(NULL);
    memleak_free_sethook(NULL);
    memleak_filter_flag = 0;
}
