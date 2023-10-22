#ifndef __LOG_QUEUE_BLOCK_H__
#define __LOG_QUEUE_BLOCK_H__

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

#include <pthread.h>

struct log_queue_node {
    struct log_queue_node *next;
    struct log_queue_node *prev;
    void *data;
};

typedef struct log_queue {
    struct log_queue_node *front;
    struct log_queue_node *back;
    unsigned long size;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} log_queue_t;

/*
 * Initializes blocking queue instance.
 * 
 * Returns 0 on success.
 * Returns error code on error.
 *  See pthread_{mutex,cond}_init
 */
int log_queue_init(log_queue_t *b);

/*
 * Pushes data into blocking queue.
 * 
 * Returns 0 on success.
 * Returns error code on error.
 *  See pthread_mutex_[un]lock
 */
int log_queue_push(log_queue_t *b, void *data);

/*
 * Pops data from blocking queue.
 * Blocks until data available.
 * 
 * Returns 0 on success and *data is set.
 * Returns error code on error and *data is not modified.
 *  See pthread_{mutex_[un]lock,cond_wait}
 */
int log_queue_pop(log_queue_t *b, void **data);

/*
 * Destroy instance.
 * Free all heap allocated nodes.
 * 
 * Returns 0 on success.
 * Returns error code on error.
 *  See pthread_{mutex,cond}_destroy
 */
int log_queue_destroy(log_queue_t *b);

/*
 * Thread-safely get queue size.
 * Sets *size with queue size.
 * 
 * Returns 0 on success.
 */
int log_queue_size(log_queue_t *b, unsigned long *size);

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /* __LOG_QUEUE_BLOCK_H__ */