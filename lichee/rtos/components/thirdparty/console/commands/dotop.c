#include <stdio.h>
#include <stdint.h>
#include <FreeRTOS.h>
#include <task.h>
#include <portable.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <console.h>
#include <aw_list.h>
#include <context.h>

#ifndef configAPPLICATION_NORMAL_PRIORITY
#define configAPPLICATION_NORMAL_PRIORITY (15)
#endif

__attribute__((weak)) size_t xPortGetTotalHeapSize( void )
{
    return CONFIG_TOTAL_HEAP_SIZE;
}

#ifdef CONFIG_HEAP_MULTIPLE
#include <aw_malloc.h>
// sync from cmd_free
static void show_heap_info(int heapID, uint32_t totalsize, char *ram_name)
{
    uint32_t freesize = 0;
    uint32_t minfreesize = 0;

    totalsize = aw_xPortGetTotalHeapSize(heapID);
    freesize = aw_xPortGetFreeHeapSize(heapID);
    minfreesize = aw_xPortGetMinimumEverFreeHeapSize(heapID);

    printf( "       %s Heap:\n", ram_name);
    printf( "           Total Size : %8ld Bytes    (%5ld KB  100.00%%)\n"
            "                 Free : %8ld Bytes    (%5ld KB  %6.2f%%)\n"
            "             Min Free : %8ld Bytes    (%5ld KB  %6.2f%%)\n",
        totalsize, totalsize >> 10,
        freesize, freesize >> 10, freesize * 100.0f / totalsize,
        minfreesize, minfreesize >> 10, minfreesize * 100.0f / totalsize);
    printf("\r\n");
}
static void show_memory_info(void)
{
#ifdef CONFIG_SRAM_HEAP
    show_heap_info(SRAM_HEAP_ID, CONFIG_SRAM_HEAP_SIZE, "sram");
#endif
#ifdef CONFIG_DRAM_HEAP
    show_heap_info(DRAM_HEAP_ID, CONFIG_DRAM_HEAP_SIZE, "dram");
#endif
#ifdef CONFIG_LPSRAM_HEAP
    show_heap_info(LPSRAM_HEAP_ID, CONFIG_LPSRAM_HEAP_SIZE, "lpsram");
#endif
#ifdef CONFIG_HPSRAM_HEAP
    show_heap_info(HPSRAM_HEAP_ID, CONFIG_HPSRAM_HEAP_SIZE, "hpsram");
#endif
}
#else
static void show_memory_info(void)
{
    uint32_t totalsize = 0;
    uint32_t freesize = 0;
    uint32_t minfreesize = 0;

    totalsize = xPortGetTotalHeapSize();
    freesize = xPortGetFreeHeapSize();
    minfreesize = xPortGetMinimumEverFreeHeapSize();

    printf( "\n       KiB Mem: %5ld total, %5ld used, %5ld free, %5ld min_free\n",
            totalsize >> 10, (totalsize - freesize) >> 10, freesize >> 10, minfreesize >> 10);
    printf( "       Total Heap Size : %8ld Bytes    ( 100.00%% )\n"
            "                  Free : %8ld Bytes    ( %6.2f%% )\n"
            "              Min Free : %8ld Bytes    ( %6.2f%% )\n",
            totalsize,
            freesize, freesize * 100.0 / totalsize,
            minfreesize, minfreesize * 100.0 / totalsize);
}
#endif

static int exit_flag = 0;
static int gDelay = 0;

typedef struct {
    uint32_t taskid;
    uint32_t cpuid;
    uint32_t counter;
    uint32_t status; /* 1-alive; 0-dead */
    struct list_head list;
} tasklist_t;
static LIST_HEAD(gTaskListHead);
static unsigned int gLastCPUCounter[configNR_CPUS] = {0};
static unsigned int gNewCPUCounter[configNR_CPUS] = {0};
static unsigned int gLastContextSwitches[configNR_CPUS] = {0};
static unsigned int gNewContextSwitches[configNR_CPUS] = {0};

static tasklist_t *tasklist_find(TaskStatus_t *task)
{
    tasklist_t *t = NULL;

    list_for_each_entry(t, &gTaskListHead, list) {
#ifdef CONFIG_SMP
        if (t->cpuid == task->bind_cpu &&
            t->taskid == task->xTaskNumber)
            return t;
#else
        if (t->taskid == task->xTaskNumber)
            return t;
#endif
    }
    return NULL;
}

static void tasklist_insert(TaskStatus_t *task)
{
    tasklist_t *t = NULL;

    t = tasklist_find(task);
    if (t != NULL) {
        t->counter = task->ulRunTimeCounter;
        t->status = 1;
        return;
    }
    t = pvPortMalloc(sizeof(tasklist_t));
    if (!t) {
        printf("no memory\n");
        return;
    }
    INIT_LIST_HEAD(&t->list);
    t->taskid = task->xTaskNumber;
#ifdef CONFIG_SMP
    t->cpuid = task->bind_cpu;
#else
    t->cpuid = 0;
#endif
    t->counter = task->ulRunTimeCounter;
    t->status = 1;
    list_add_tail(&t->list, &gTaskListHead);
}

static void tasklist_clear_status(void)
{
    tasklist_t *t = NULL;
    list_for_each_entry(t, &gTaskListHead, list) {
        t->status = 0;
    }
}

static void tasklist_update_info(void)
{
    tasklist_t *t = NULL, *tmp = NULL;
    list_for_each_entry_safe(t, tmp, &gTaskListHead, list) {
        if (t->status == 0) {
            list_del(&t->list);
            vPortFree(t);
        }
    }
}

__attribute__((weak)) TickType_t xTaskGetTickCountCore( uint32_t cpuid )
{
	return xTaskGetTickCount();
}

__attribute__((weak)) TickType_t xTaskGetContextSwitchesCore( uint32_t cpuid )
{
#ifndef CONFIG_SMP
extern uint32_t ulTaskSwitchedInTime;
    return ulTaskSwitchedInTime;
#else
    return 0;
#endif
}

static void tasklist_update_cpu_counter(void)
{
    int i;
    for (i = 0; i< configNR_CPUS; i++)
        gLastCPUCounter[i] = gNewCPUCounter[i];
}

static void tasklist_update_new_cpu_counter(void)
{
    int i;
    for (i = 0; i< configNR_CPUS; i++)
        gNewCPUCounter[i] = xTaskGetTickCountCore(i);
}

TickType_t xTaskGetContextSwitchesCore( uint32_t cpuid );
static void tasklist_update_ctxt(void)
{
    int i;
    for (i = 0; i< configNR_CPUS; i++)
        gLastContextSwitches[i] = gNewContextSwitches[i];
}

static void tasklist_update_new_ctxt(void)
{
    int i;
    for (i = 0; i< configNR_CPUS; i++)
        gNewContextSwitches[i] = xTaskGetContextSwitchesCore(i);
}

static float tasklist_cal_cpu_usage(TaskStatus_t *task)
{
    unsigned int delta_t = 0, delta_c = 0;
    tasklist_t *t = NULL;
#ifdef CONFIG_SMP
	int bind_cpu = task->bind_cpu;
#else
	int bind_cpu = 0;
#endif

    t = tasklist_find(task);
    delta_c = gNewCPUCounter[bind_cpu] - gLastCPUCounter[bind_cpu];
    if (!t)
        delta_t = task->ulRunTimeCounter;
    else
        delta_t = task->ulRunTimeCounter - t->counter;

    /* If there is no Context Switches, set the Running task CPU usage to 100% */
    if (gNewContextSwitches[bind_cpu] == gLastContextSwitches[bind_cpu])
        if (task->eCurrentState == eRunning)
            delta_t = delta_c;

    if(delta_t >= delta_c)
        delta_t = delta_c;

    return ((float)delta_t * 100) / ((float)delta_c);
}

static void tasklist_clean_all(void)
{
    tasklist_t *t = NULL, *tmp = NULL;
    list_for_each_entry_safe(t, tmp, &gTaskListHead, list) {
            list_del(&t->list);
            vPortFree(t);
    }
    INIT_LIST_HEAD(&gTaskListHead);
    memset(gLastCPUCounter, 0, sizeof(gLastCPUCounter));
    memset(gNewCPUCounter, 0, sizeof(gNewCPUCounter));
}

int cmp( const void *a , const void *b  )
{
    TaskStatus_t *c = (TaskStatus_t *)a;
    TaskStatus_t *d = (TaskStatus_t *)b;

#ifdef CONFIG_SMP
    if(c->bind_cpu != d->bind_cpu)
        return c->bind_cpu - d->bind_cpu;
#endif

#if( configGENERATE_RUN_TIME_STATS == 1 )
    return (tasklist_cal_cpu_usage(d) > tasklist_cal_cpu_usage(c)) ? 1 : -1;
#else
    return 0;
#endif
}

static void monitor_start(void)
{
    uint8_t *ptr = NULL;
    char *stat = NULL;
    uint32_t cpsr_flag;
    float stk_usage = 0.0f, cpu_usage = 0.0f;
    int pxStackSize = 0;
    void *entry = NULL;

    TaskStatus_t *pxTaskStatusArray;
    UBaseType_t uxArraySize, x, i;

#ifdef CONFIG_SMP
    taskENTER_CRITICAL(cpsr_flag);
#else
    taskENTER_CRITICAL();
#endif

    uxArraySize = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

    if( pxTaskStatusArray != NULL )
    {
        memset(pxTaskStatusArray, 0, uxArraySize * sizeof( TaskStatus_t ));
        uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, NULL );
    }

#ifdef CONFIG_SMP
    taskEXIT_CRITICAL(cpsr_flag);
#else
    taskEXIT_CRITICAL();
#endif

    printf("\r\n");
    printf("    -----------------------------------------------TSK Usage Report----------------------------------------------------------------------\n");
    printf("\n");

    if( pxTaskStatusArray != NULL )
    {
        tasklist_update_new_cpu_counter();
        tasklist_update_new_ctxt();

        qsort(pxTaskStatusArray, uxArraySize, sizeof(TaskStatus_t), cmp);

        for( i = 0; i < configNR_CPUS; i++ )
        {
            printf("        CPU%d:    num     entry       stat   prio     tcb       stacksize  stkusg    stackWaterMark  cputime   command\n", i);

            for( x = 0; x < uxArraySize; x++ )
            {
                TaskStatus_t *temp = &pxTaskStatusArray[x];
                uint8_t status = pxTaskStatusArray[ x ].eCurrentState;
                if (status == eRunning)
                {
                    stat = "running";
                }
                else if (status == eSuspended)
                {
                    stat = "suspend";
                }
                else if (status == eDeleted)
                {
                    stat = "delete";
                }
                else if (status == eReady)
                {
                    stat = "ready";
                }
                else if (status == eBlocked)
                {
                    stat = "block";
                }
                else
                {
                    stat = "unknown";
                }
                switch_ctx_regs_t *regs_ctx;
                regs_ctx = (switch_ctx_regs_t *)(temp->pxTopOfStack);

#ifdef CONFIG_SMP
#if portSTACK_GROWTH < 0
                uint8_t * ptr = (uint8_t *)temp->pxTopOfStack;
                stk_usage = (float)(temp->pxStackSize - ((uint32_t) ptr - (uint32_t) temp->pxStackBase)) * 100 / (float)temp->pxStackSize;
#else
                uint8_t * ptr = (uint8_t *)temp->pxEndOfStack;
                stk_usage = (float)(temp->pxStackSize - ((uint32_t) temp->pxStackBase - (uint32_t)ptr)) * 100 / (float)temp->pxStackSize;
#endif
#endif

#if( configGENERATE_RUN_TIME_STATS == 1 )
                cpu_usage = tasklist_cal_cpu_usage(temp);
#endif

#ifdef CONFIG_SMP
                pxStackSize = temp->pxStackSize;
                entry = temp->entry;
                if (temp->bind_cpu == i)
                {
#endif
                    printf("               %4ld   0x%08lx %9s %4d   0x%08lx  %8ld    %5.2f%%     %8ld       %6.2f%%   %s\n", \
                        temp->xTaskNumber,
                        (unsigned long)entry,
                        stat,
                        temp->uxCurrentPriority,
                        (unsigned long)temp->xHandle,
                        pxStackSize,
                        stk_usage,
                        temp->usStackHighWaterMark * sizeof(StackType_t),
                        cpu_usage,
                        temp->pcTaskName);
#ifdef CONFIG_SMP
                }
                else
                    continue;
#endif
            }
            printf("\n");
        }
        /* update tasklist info */
        tasklist_clear_status();
        for (x = 0; x < uxArraySize; x++) {
            tasklist_insert(&pxTaskStatusArray[x]);
        }
        tasklist_update_info();
        tasklist_update_cpu_counter();
        tasklist_update_ctxt();
    }
    printf("    ------------------------------------------------Memory information--------------------------------------------------------------------\n");
    show_memory_info();
    printf("    --------------------------------------------------------------------------------------------------------------------------------------\n");
    /*  printf("\n    Please enter Ctrl-C or 'q' to quit the command!\n"); */
    printf("\n    Please enter 'top_exit' to quit the command!\n");
    vPortFree(pxTaskStatusArray);
}

static void monitor_thread_entry(void * param)
{
    const TickType_t timeout =  (gDelay*1000) / portTICK_PERIOD_MS;

    while(1)
    {
        printf("\e[1;1H\e[2J");
        monitor_start();
        vTaskDelay(timeout);
        if(exit_flag == 1)
        {
            exit_flag = 0;
            break;
        }
    }
    tasklist_clean_all();
    vTaskDelete(NULL);
}

int cmd_top(int argc, char ** argv)
{
    TaskHandle_t perf_task;
    portBASE_TYPE ret;
    int c = 0;

    optind = 0;
    gDelay = 3;
    exit_flag = 0;

    while ((c = getopt(argc, argv, "d:")) != -1) {
        switch (c) {
        case 'd':
            gDelay = atoi(optarg);
            break;
        default:
            break;
        }
    }

    ret = xTaskCreate(monitor_thread_entry, (signed portCHAR *) "top", 4096, NULL, configAPPLICATION_NORMAL_PRIORITY, &perf_task);
    if (ret != pdPASS)
    {
        printf("Error creating task, status was %d\n", ret);
        return -1;
    }
#if 0
#if defined(CONFIG_COMPONENT_CLI) || defined(CONFIG_COMPONENT_FINSH_CLI)
    while(1)
    {
        char cRxed = 0;

        cRxed = getchar();
        if(cRxed == 'q' || cRxed == 3)
        {
            exit_flag = 1;
            return 0;
        }
    }
#endif
#endif
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_top, top, Performance monitor);

static int cmd_top_exit(int argc, char **argv)
{
    exit_flag = 1;
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_top_exit, top_exit, Top Exit);
