#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <hal_interrupt.h>
#include <hal_thread.h>
#include <ringbuffer.h>

#define RB_EV_WRITE				(1 << 0)
#define RB_EV_READ_AVAIL			(1 << 1)

struct hal_ringbuffer {
	uint8_t *buffer;
	uint32_t length;
	uint32_t threshold;
	volatile uint32_t start;
	volatile uint32_t end;
	volatile bool isfull;
	hal_mutex_t mutex;
	hal_event_t event;
};

hal_ringbuffer_t hal_ringbuffer_init(int size)
{
	hal_ringbuffer_t rb = NULL;

#if 0
	if (hal_thread_is_in_critical_context()) {
		printf("%s Shoult not call in cirtical context\n", __func__);
		printf("nest:%d, running:%d, irq is disable:%d\n",
			hal_interrupt_get_nest() == 0,
			hal_thread_scheduler_is_running(),
			hal_interrupt_is_disable());
		return NULL;
	}
#endif

	if (size <= 0)
		return NULL;

	rb = hal_malloc(sizeof(*rb));
	if (!rb) {
		printf("ringbuffer: no memory\n");
		return NULL;
	}

	rb->buffer = hal_malloc(size);
	if (!rb->buffer) {
		printf("ringbuffer: no memory\n");
		goto err_out1;
	}
	rb->event = hal_event_create();
	if (!rb->event) {
		printf("ringbuffer: no memory\n");
		goto err_out2;
	}
	rb->mutex = hal_mutex_create();
	if (!rb->mutex) {
		printf("ringbuffer: no memory\n");
		goto err_out3;
	}

	rb->length = size;
	rb->start = 0;
	rb->end = 0;
	rb->threshold = 0;
	rb->isfull = false;

	return rb;

err_out3:
	hal_event_delete(rb->event);
err_out2:
	hal_free(rb->buffer);
err_out1:
	hal_free(rb);
	return NULL;
}

void hal_ringbuffer_release(hal_ringbuffer_t rb)
{
	if (hal_thread_is_in_critical_context()) {
		printf("%s Shoult not call in cirtical context\n", __func__);
		return;
	}

	if (!rb)
		return;

	if (rb->buffer)
		hal_free(rb->buffer);

	hal_mutex_delete(rb->mutex);
	hal_event_delete(rb->event);
	hal_free(rb);
	return;
}

void hal_ringbuffer_reset(hal_ringbuffer_t rb)
{
	hal_mutex_lock(rb->mutex);
	rb->start = rb->end = 0;
	rb->isfull = false;
	hal_mutex_unlock(rb->mutex);
}

int hal_ringbuffer_resize(hal_ringbuffer_t rb, int size)
{
	void *buf;

	buf = hal_malloc(size);
	if (!buf)
		return -ENOMEM;

	hal_mutex_lock(rb->mutex);
	if (rb->buffer)
		hal_free(rb->buffer);
	rb->buffer = buf;
	hal_mutex_unlock(rb->mutex);

	return 0;
}

uint32_t hal_ringbuffer_length(hal_ringbuffer_t rb)
{
	return rb->length;
}

uint32_t hal_ringbuffer_valid(hal_ringbuffer_t rb)
{
	if (rb->start > rb->end)
		return rb->start - rb->end;
	else
		return rb->length - (rb->end - rb->start);
}

bool hal_ringbuffer_is_full(hal_ringbuffer_t rb)
{
	return rb->isfull;
}

bool hal_ringbuffer_is_empty(hal_ringbuffer_t rb)
{
	if (rb->isfull)
		return false;

	if (rb->start != rb->end)
		return false;

	return true;
}

int hal_ringbuffer_get(hal_ringbuffer_t rb, void *buf, int size, unsigned int timeout)
{
	int len, cross = 0;

	if (!rb)
		return -1;

	hal_mutex_lock(rb->mutex);
	if (rb->isfull == true) {
		len = rb->length;
		goto cal_actual_size;
	}

	if (rb->end - rb->start == 0 && timeout != 0) {
		hal_mutex_unlock(rb->mutex);
		hal_event_wait(rb->event, RB_EV_WRITE,
						HAL_EVENT_OPTION_CLEAR | HAL_EVENT_OPTION_AND,
						timeout);
		hal_mutex_lock(rb->mutex);
	}

	len = rb->end - rb->start;
	if (len == 0) {
		hal_mutex_unlock(rb->mutex);
		return 0;
	} else if (len < 0) {
		len += rb->length;
	} else  if (len > rb->length) {
		printf("len=%d, error\n", len);
		return -EIO;
	}

cal_actual_size:
	len = len > size ? size : len;
	if (rb->start + len >= rb->length)
		cross = 1;

	if (cross != 0) {
		int first = rb->length - rb->start;
		memcpy(buf, rb->buffer + rb->start, first);
		memcpy(buf + first, rb->buffer, len - first);
		rb->start = len - first;
	} else {
		memcpy(buf, rb->buffer + rb->start, len);
		rb->start += len;
	}
	if (rb->isfull && len != 0)
		rb->isfull = false;

	hal_mutex_unlock(rb->mutex);

	if (rb->threshold > 0 && !rb->isfull && (hal_ringbuffer_valid(rb) >= rb->threshold)) {
		rb->threshold = 0;
		hal_event_set_bits(rb->event, RB_EV_READ_AVAIL);
	}
	return len;
}

int hal_ringbuffer_put(hal_ringbuffer_t rb, const void *buf, int size)
{
	int len, cross = 0;

	if (!rb)
		return -1;

	hal_mutex_lock(rb->mutex);

	if (rb->isfull == true) {
		hal_event_set_bits(rb->event, RB_EV_WRITE);
		hal_mutex_unlock(rb->mutex);
		return 0;
	}

	if (rb->start > rb->end)
		len = rb->start - rb->end;
	else
		len = rb->length - (rb->end - rb->start);
	len = len > size ? size : len;

	if (rb->end + len > rb->length)
		cross = 1;

	if (cross != 0) {
		int first = rb->length - rb->end;
		memcpy(rb->buffer + rb->end, buf, first);
		memcpy(rb->buffer, buf + first, len - first);
		rb->end = len - first;
	} else {
		memcpy(rb->buffer + rb->end, buf, len);
		rb->end += len;
		rb->end %= rb->length;
	}

	if (rb->end == rb->start && len != 0){
		rb->isfull = true;
	}

	hal_mutex_unlock(rb->mutex);

	if (rb->isfull || rb->start != rb->end)
		hal_event_set_bits(rb->event, RB_EV_WRITE);

	return len;
}

int hal_ringbuffer_force_put(hal_ringbuffer_t rb, const void *buf, int size)
{
	int len, remain, cross = 0;

	if (!rb)
		return -1;

	hal_mutex_lock(rb->mutex);

	if (size >= rb->length) {
		buf += size - rb->length;
		size = rb->length;
		rb->start = rb->end = 0;
		rb->isfull = false;
		goto put;
	}

	if (rb->isfull == true) {
		remain = 0;
	} else {
		if (rb->start > rb->end)
			remain = rb->start - rb->end;
		else
			remain = rb->length - (rb->end - rb->start);
	}
	if (remain < size) {
		/* make enought space */
		rb->start += size - remain;
		rb->start %= rb->length;
	}
put:
	if (rb->start > rb->end)
		len = rb->start - rb->end;
	else
		len = rb->length - (rb->end - rb->start);
	len = len > size ? size : len;

	if (rb->end + len > rb->length)
		cross = 1;

	if (cross != 0) {
		int first = rb->length - rb->end;

		memcpy(rb->buffer + rb->end, buf, first);
		memcpy(rb->buffer, buf + first, len - first);
		rb->end = len - first;
	} else {
		memcpy(rb->buffer + rb->end, buf, len);
		rb->end += len;
		rb->end %= rb->length;
	}

	if (rb->end == rb->start && len != 0)
		rb->isfull = true;

	hal_mutex_unlock(rb->mutex);

	if (rb->isfull || rb->start != rb->end)
		hal_event_set_bits(rb->event, RB_EV_WRITE);

	return len;
}

int hal_ringbuffer_wait_put(hal_ringbuffer_t rb, const void *buf, int size, int timeout)
{
	hal_mutex_lock(rb->mutex);

	if (rb->isfull ||
		(hal_ringbuffer_valid(rb) < size)) {
		rb->threshold = size;
		hal_mutex_unlock(rb->mutex);
		hal_event_wait(rb->event, RB_EV_READ_AVAIL,
					HAL_EVENT_OPTION_CLEAR | HAL_EVENT_OPTION_AND, timeout);
		hal_mutex_lock(rb->mutex);
	}
	hal_mutex_unlock(rb->mutex);
	return hal_ringbuffer_put(rb, buf, size);
}
