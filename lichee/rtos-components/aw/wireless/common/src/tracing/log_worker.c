#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include "log_worker.h"
#include "log_queue_block.h"

struct worker_work {
    void *(*function)(void *);
    void *arg;
    void **ret;
};

static log_queue_t worker_queue;
static pthread_t worker_handle;

int log_worker_init()
{
    if (log_queue_init(&worker_queue) != 0) {
        return -1;
    }
    if (pthread_create(&worker_handle, NULL, log_worker_loop, NULL)) {
        return -1;
    }
    return 0;
}

int log_worker_deinit()
{
    log_queue_destroy(&worker_queue);
    pthread_cancel(worker_handle);
    return 0;
}

void *log_worker_loop(void *arg)
{
    struct worker_work work;
    struct worker_work *work_ptr = &work;

    // if (!&worker_queue)
    //     return;

    while (1) {
        log_queue_pop(&worker_queue, (void *)&work_ptr);
        if (work.function) {
            *work.ret = work.function(work.arg);
        }
    }
    log_queue_destroy(&worker_queue);
    return NULL;
}

int log_worker_schedule(void *(*function)(void *), void *arg, void **ret)
{
    struct worker_work work;

    if (!function) {
        return ENOEXEC;
    }

    work.function = function;
    work.arg = arg;
    work.ret = ret;
    if (log_queue_push(&worker_queue, &work) != 0) {
        return ENOMEM;
    }

    return 0;
}
