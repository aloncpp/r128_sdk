#ifndef HAL_RINGBUFFER_H
#define HAL_RINGBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_mem.h>
#include <hal_thread.h>
#include <hal_mutex.h>
#include <hal_sem.h>
#include <hal_event.h>

struct hal_ringbuffer;
typedef struct hal_ringbuffer *hal_ringbuffer_t;

hal_ringbuffer_t hal_ringbuffer_init(int size);
void hal_ringbuffer_release(hal_ringbuffer_t rb);
int hal_ringbuffer_resize(hal_ringbuffer_t rb, int size);
uint32_t hal_ringbuffer_length(hal_ringbuffer_t rb);
uint32_t hal_ringbuffer_valid(hal_ringbuffer_t rb);
bool hal_ringbuffer_is_full(hal_ringbuffer_t rb);
bool hal_ringbuffer_is_empty(hal_ringbuffer_t rb);
int hal_ringbuffer_get(hal_ringbuffer_t rb, void *buf, int size, unsigned int timeout);
int hal_ringbuffer_put(hal_ringbuffer_t rb, const void *buf, int size);
int hal_ringbuffer_force_put(hal_ringbuffer_t rb, const void *buf, int size);
int hal_ringbuffer_wait_put(hal_ringbuffer_t rb, const void *buf, int size, int timeout);

#ifdef __cplusplus
}
#endif
#endif
