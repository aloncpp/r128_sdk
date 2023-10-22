#include <stdlib.h>
#include <stdio.h>

#include <hal_cmd.h>
#include <hal_workqueue.h>

static hal_workqueue *wqueue;
static hal_work work1;
static hal_work work2;

static void work_func(hal_work *work, void *data)
{
    printf("work:%s\n",data);
}

static int cmd_workqueue_test(int argc, char **argv)
{
    wqueue = hal_workqueue_create("test", 0x1000, 6);
    if (wqueue == NULL) {
        return -1;
    }
    hal_work_init(&work1, work_func, "work1");
    hal_work_init(&work2, work_func, "work2");
    hal_workqueue_dowork(wqueue, &work1);
    hal_workqueue_dowork(wqueue, &work2);
    hal_workqueue_destroy(wqueue);
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_workqueue_test, workqueue_test, osal workqueue test);
