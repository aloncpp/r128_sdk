#include "log_queue_block.h"
#include <stdlib.h>

int log_queue_init(log_queue_t *b)
{
    int ret = 0;
    b->front = 0;
    b->back = 0;
    b->size = 0;
    if ((ret = pthread_mutex_init(&b->mutex, 0)) != 0) {
        return ret;
    }
    if ((ret = pthread_cond_init(&b->cond, 0)) != 0) {
        return ret;
    }
    return 0;
}

int log_queue_push(log_queue_t *b, void *data)
{
    int ret = 0;
    if ((ret = pthread_mutex_lock(&b->mutex)) != 0) {
        return ret;
    }
    struct log_queue_node *n = malloc(sizeof(*n));
    if(n == NULL) {
        return -1;
    }
    n->data = data;
    n->next = 0;
    n->prev = b->back;
    if (b->back != 0) {
        b->back->next = n;
        b->back = n;
    } else {
        b->front = b->back = n;
    }
    b->size = b->size + 1;
    if ((ret = pthread_cond_signal(&b->cond)) != 0) {
        return ret;
    }
    if ((ret = pthread_mutex_unlock(&b->mutex)) != 0) {
        return ret;
    }
    return 0;
}

int log_queue_pop(log_queue_t *b, void **data)
{
    int ret = 0;
    if ((ret = pthread_mutex_lock(&b->mutex)) != 0) {
        return ret;
    }
    while (b->front == 0) {
        if ((ret = pthread_cond_wait(&b->cond, &b->mutex)) != 0) {
            // POSIX guarantees EINTR will not be returned
            return ret;
        }
    }
    struct log_queue_node *front = b->front;
    b->front = front->next;
    if (b->front != 0) {
        b->front->prev = 0;
    } else {
        b->back = 0;
    }
    *data = front->data;
    free(front);
    b->size = b->size - 1;
    if ((ret = pthread_mutex_unlock(&b->mutex)) != 0) {
        return ret;
    }
    return 0;
}

int log_queue_destroy(log_queue_t *b)
{
    int ret = 0;
    if ((ret = pthread_mutex_lock(&b->mutex)) != 0) {
        return ret;
    }
    while (b->front != 0) {
        struct log_queue_node *next = b->front->next;
        free(b->front);
        b->front = next;
    }
    if ((ret = pthread_mutex_unlock(&b->mutex)) != 0) {
        return ret;
    }
    if ((ret = pthread_mutex_destroy(&b->mutex)) != 0) {
        return ret;
    }
    if ((ret = pthread_cond_destroy(&b->cond)) != 0) {
        return ret;
    }
    return 0;
}

int log_queue_size(log_queue_t *b, unsigned long *size)
{
    int ret = 0;
    if ((ret = pthread_mutex_lock(&b->mutex)) != 0) {
        return ret;
    }
    *size = b->size;
    if ((ret = pthread_mutex_unlock(&b->mutex)) != 0) {
        return ret;
    }
    return 0;
}
