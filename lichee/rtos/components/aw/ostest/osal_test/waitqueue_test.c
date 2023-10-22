#include <stdlib.h>
#include <stdio.h>

#include <hal_cmd.h>
#include <hal_waitqueue.h>
#include <hal_time.h>
#include <hal_thread.h>

static hal_waitqueue_t wqueue_test;

static void wqtest_task1(void *para)
{
    while (1)
    {
        printf("comm = %s, before enter waitlist.\n", pcTaskGetName(xTaskGetCurrentTaskHandle()));
        hal_waitqueue_wait(&wqueue_test, 0, HAL_WAIT_FOREVER);
        printf("comm = %s, resume from waitlist.\n", pcTaskGetName(xTaskGetCurrentTaskHandle())) ;
    }
}

static void wqtest_task2(void *para)
{
    while (1)
    {
        vTaskDelay(1000);
        printf("comm = %s, wakeup waitlist.\n", pcTaskGetName(xTaskGetCurrentTaskHandle()));
        hal_waitqueue_wakeup(&wqueue_test, NULL);
    }
}

static int cmd_waitqueue_test(int argc, char **argv)
{
    void* thread;

    hal_waitqueue_init(&wqueue_test);

    thread = hal_thread_create(wqtest_task1, NULL, "wq1", 0x1000, 1);
    hal_thread_start(thread);

    thread = hal_thread_create(wqtest_task1, NULL, "wq2", 0x1000, 1);
    hal_thread_start(thread);

    thread = hal_thread_create(wqtest_task2, NULL, "wq3", 0x1000, 1);
    hal_thread_start(thread);

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_waitqueue_test, waitqueue_test, osal waitqueue test);
