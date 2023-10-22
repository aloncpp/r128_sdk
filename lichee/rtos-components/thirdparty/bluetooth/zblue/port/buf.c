/*
********************************************************************************
* @file    buf.c
* @brief   This file is one of the zephyr original source code.
********************************************************************************
*/

/* buf.c - Buffer management */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <misc/byteorder.h>

#include <net/buf.h>

#if defined(CONFIG_NET_BUF_LOG)
#define SYS_LOG_DOMAIN "net/buf"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_NET_BUF_LEVEL
#include <logging/sys_log.h>

#define NET_BUF_DBG(fmt, ...) SYS_LOG_DBG("(%p) " fmt, __func__/*k_current_get()*/, \
					  ##__VA_ARGS__)
#define NET_BUF_ERR(fmt, ...) SYS_LOG_ERR(fmt, ##__VA_ARGS__)
#define NET_BUF_WARN(fmt, ...) SYS_LOG_WRN(fmt,	##__VA_ARGS__)
#define NET_BUF_INFO(fmt, ...) SYS_LOG_INF(fmt,  ##__VA_ARGS__)
#define NET_BUF_ASSERT(cond) do { if (!(cond)) {			  \
			NET_BUF_ERR("assert: '" #cond "' failed"); \
		} } while (0)
#else

#define NET_BUF_DBG(fmt, ...)
#define NET_BUF_ERR(fmt, ...)
#define NET_BUF_WARN(fmt, ...)
#define NET_BUF_INFO(fmt, ...)
#define NET_BUF_ASSERT(cond)
#endif /* CONFIG_NET_BUF_LOG */

#if CONFIG_NET_BUF_WARN_ALLOC_INTERVAL > 0
#define WARN_ALLOC_INTERVAL K_SECONDS(CONFIG_NET_BUF_WARN_ALLOC_INTERVAL)
#else
#define WARN_ALLOC_INTERVAL K_FOREVER
#endif

#ifndef CONFIG_BLEHOST_Z_ITERABLE_SECTION
extern struct net_buf_pool hci_cmd_pool;
extern struct net_buf_pool hci_rx_pool;
extern struct net_buf_pool acl_tx_pool;
extern struct net_buf_pool disc_pool;
#if defined(CONFIG_BT_MESH)
extern struct net_buf_pool adv_buf_pool;
#if (CONFIG_BT_MESH_FRIEND)
extern struct net_buf_pool friend_buf_pool;
#endif
#endif
#if CONFIG_BT_ATT_PREPARE_COUNT > 0
extern struct net_buf_pool prep_pool;
#endif
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
extern struct net_buf_pool acl_in_pool;
#endif
#if CONFIG_BT_L2CAP_TX_FRAG_COUNT > 0
extern struct net_buf_pool frag_pool;
#endif
#if defined(CONFIG_BT_CONN)
extern struct net_buf_pool num_complete_pool;
#endif
#if defined(CONFIG_BT_DISCARDABLE_BUF_COUNT)
extern struct net_buf_pool discardable_pool;
#endif
struct net_buf_pool *net_buf_pool_list[] = {
    &hci_cmd_pool,
    &hci_rx_pool,
    &acl_tx_pool,
    &disc_pool,
#if defined(CONFIG_BT_MESH)
    &adv_buf_pool,
#if (CONFIG_BT_MESH_FRIEND)
    &friend_buf_pool,
#endif
#endif
#if CONFIG_BT_ATT_PREPARE_COUNT > 0
    &prep_pool,
#endif
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
    &acl_in_pool,
#endif
#if CONFIG_BT_L2CAP_TX_FRAG_COUNT > 0
    &frag_pool,
#endif
#if defined(CONFIG_BT_CONN)
    &num_complete_pool,
#endif
#if defined(CONFIG_BT_DISCARDABLE_BUF_COUNT)
    &discardable_pool,
#endif
};
#else
/* Linker-defined symbol bound to the static pool structs */
extern struct net_buf_pool _net_buf_pool_list[];
#endif

struct net_buf_pool *net_buf_pool_get(int id)
{
#ifndef CONFIG_BLEHOST_Z_ITERABLE_SECTION
	return net_buf_pool_list[id];
#else
	return &_net_buf_pool_list[id];
#endif
}

//#define CONFIG_NET_BUF_DEBUG_POOL &hci_rx_pool

#if defined(CONFIG_BT_VAR_MEM_DYNC_ALLOC)
void net_buf_pool_data_mem_alloc(void)
{
	size_t alloc_size;
	size_t count;
	size_t data_size;

	Z_STRUCT_SECTION_FOREACH(net_buf_pool, pool) {
		count = pool->buf_count;
		data_size = ((struct net_buf_pool_fixed *)(pool->alloc->alloc_data))->data_size;

		alloc_size = count * data_size;

		((struct net_buf_pool_fixed *)(pool->alloc->alloc_data))->data_pool = (uint8_t *)k_malloc(alloc_size);
		if (((struct net_buf_pool_fixed *)(pool->alloc->alloc_data))->data_pool == NULL) {
			NET_BUF_ERR("Alloc data pool error(%p)", pool);
			return ;
		}
		memset(((struct net_buf_pool_fixed *)(pool->alloc->alloc_data))->data_pool, 0, alloc_size);
		pool->__bufs = (struct net_buf *)k_malloc(sizeof(struct net_buf) * count);
		if (pool->__bufs == NULL) {
			NET_BUF_ERR("Alloc bufs error(%p)", pool);
			return ;
		}
		memset(pool->__bufs, 0, sizeof(struct net_buf) * count);
	}
}

void net_buf_pool_data_mem_free(void)
{
	Z_STRUCT_SECTION_FOREACH(net_buf_pool, pool) {
		k_free(pool->__bufs);
		pool->__bufs = NULL;
		k_free(((struct net_buf_pool_fixed *)(pool->alloc->alloc_data))->data_pool);
		((struct net_buf_pool_fixed *)(pool->alloc->alloc_data))->data_pool = NULL;
	}
}
#endif

#if defined(CONFIG_BT_DEINIT)
void net_buf_pool_fixed_deinit(struct net_buf_pool *pool, uint32_t _count)
{
#if defined(CONFIG_NET_BUF_POOL_USAGE)
	pool->free = (struct k_lifo)Z_LIFO_DEINITIALIZER(pool->free);
	pool->uninit_count = _count;
	pool->avail_count = _count,
#else
	pool->free = (struct k_lifo)Z_LIFO_DEINITIALIZER(pool->free);
	pool->uninit_count = _count;
#endif /* CONFIG_NET_BUF_POOL_USAGE */
}
#endif

static int pool_id(struct net_buf_pool *pool)
{
#ifndef CONFIG_BLEHOST_Z_ITERABLE_SECTION
	int i = 0;

	while( net_buf_pool_list[i] != pool )
		i++;

	return i;
#else
	return pool - _net_buf_pool_list;
#endif
}

int net_buf_id(struct net_buf *buf)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

	return buf - pool->__bufs;
}

static inline struct net_buf *pool_get_uninit(struct net_buf_pool *pool,
					      uint16_t uninit_count)
{
	struct net_buf *buf;

	buf = &pool->__bufs[pool->buf_count - uninit_count];

	buf->pool_id = pool_id(pool);

	return buf;
}

void net_buf_reset(struct net_buf *buf)
{
	NET_BUF_ASSERT(buf->flags == 0);
	NET_BUF_ASSERT(buf->frags == NULL);

	net_buf_simple_reset(&buf->b);
}

#if 1 //Wewisetek Modified
#else //Origin
static uint8_t *generic_data_ref(struct net_buf *buf, uint8_t *data)
{
	uint8_t *ref_count;

	ref_count = data - 1;
	(*ref_count)++;

	return data;
}

static uint8_t *mem_pool_data_alloc(struct net_buf *buf, size_t *size,
				 s32_t timeout)
{
	struct net_buf_pool *buf_pool = net_buf_pool_get(buf->pool_id);
	struct k_mem_pool *pool = buf_pool->alloc->alloc_data;
	struct k_mem_block block;
	uint8_t *ref_count;

	/* Reserve extra space for k_mem_block_id and ref-count (uint8_t) */
	if (k_mem_pool_alloc(pool, &block,
			     sizeof(struct k_mem_block_id) + 1 + *size,
			     timeout)) {
		return NULL;
	}

	/* save the block descriptor info at the start of the actual block */
	memcpy(block.data, &block.id, sizeof(block.id));

	ref_count = (uint8_t *)block.data + sizeof(block.id);
	*ref_count = 1U;

	/* Return pointer to the byte following the ref count */
	return ref_count + 1;
}

static void mem_pool_data_unref(struct net_buf *buf, uint8_t *data)
{
	struct k_mem_block_id id;
	uint8_t *ref_count;

	ref_count = data - 1;
	if (--(*ref_count)) {
		return;
	}

	/* Need to copy to local variable due to alignment */
	memcpy(&id, ref_count - sizeof(id), sizeof(id));
	k_mem_pool_free_id(&id);
}

const struct net_buf_data_cb net_buf_var_cb = {
	.alloc = mem_pool_data_alloc,
	.ref   = generic_data_ref,
	.unref = mem_pool_data_unref,
};
#endif

static uint8_t *fixed_data_alloc(struct net_buf *buf, size_t *size, s32_t timeout)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);
	const struct net_buf_pool_fixed *fixed = pool->alloc->alloc_data;

	*size = MIN(fixed->data_size, *size);

	return fixed->data_pool + fixed->data_size * net_buf_id(buf);
}

static void fixed_data_unref(struct net_buf *buf, uint8_t *data)
{
	/* Nothing needed for fixed-size data pools */
}

const struct net_buf_data_cb net_buf_fixed_cb = {
	.alloc = fixed_data_alloc,
	.unref = fixed_data_unref,
};

#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)

static uint8_t *heap_data_alloc(struct net_buf *buf, size_t *size, s32_t timeout)
{
	uint8_t *ref_count;

	ref_count = k_malloc(1 + *size);
	if (!ref_count) {
		return NULL;
	}

	*ref_count = 1U;

	return ref_count + 1;
}

static void heap_data_unref(struct net_buf *buf, uint8_t *data)
{
	uint8_t *ref_count;

	ref_count = data - 1;
	if (--(*ref_count)) {
		return;
	}

	k_free(ref_count);
}

static const struct net_buf_data_cb net_buf_heap_cb = {
	.alloc = heap_data_alloc,
	.ref   = generic_data_ref,
	.unref = heap_data_unref,
};

const struct net_buf_data_alloc net_buf_heap_alloc = {
	.cb = &net_buf_heap_cb,
};

#endif /* CONFIG_HEAP_MEM_POOL_SIZE > 0 */

static uint8_t *data_alloc(struct net_buf *buf, size_t *size, s32_t timeout)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

	return pool->alloc->cb->alloc(buf, size, timeout);
}

static uint8_t *data_ref(struct net_buf *buf, uint8_t *data)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

	return pool->alloc->cb->ref(buf, data);
}

static void data_unref(struct net_buf *buf, uint8_t *data)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

	if (buf->flags & NET_BUF_EXTERNAL_DATA) {
		return;
	}

	pool->alloc->cb->unref(buf, data);
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_alloc_len_debug(struct net_buf_pool *pool, size_t size,
					s32_t timeout, const char *func,
					int line)
#else
struct net_buf *net_buf_alloc_len(struct net_buf_pool *pool, size_t size,
				  s32_t timeout)
#endif
{
	u32_t alloc_start = k_uptime_get_32();
	struct net_buf *buf;
	unsigned int key;

	NET_BUF_ASSERT(pool);

	NET_BUF_DBG("%s():%d: pool %p size %d timeout %d", func, line, pool,
		    size, timeout);

	/* We need to lock interrupts temporarily to prevent race conditions
	 * when accessing pool->uninit_count.
	 */
	key = irq_lock();

	/* If there are uninitialized buffers we're guaranteed to succeed
	 * with the allocation one way or another.
	 */
	 NET_BUF_DBG("%d", __LINE__);
	if (pool->uninit_count) {
		uint16_t uninit_count;
		NET_BUF_DBG("%d", __LINE__);
		/* If this is not the first access to the pool, we can
		 * be opportunistic and try to fetch a previously used
		 * buffer from the LIFO with K_NO_WAIT.
		 */
		if (pool->uninit_count < pool->buf_count) {
			buf = k_lifo_get(&pool->free, K_NO_WAIT);
			NET_BUF_DBG("%d buf %p", __LINE__, buf);
			if (buf) {
				irq_unlock(key);
				goto success;
			}
		}
		NET_BUF_DBG("%d", __LINE__);
		uninit_count = pool->uninit_count--;
		irq_unlock(key);

		buf = pool_get_uninit(pool, uninit_count);
		goto success;
	}

	irq_unlock(key);
	NET_BUF_DBG("%d", __LINE__);
#if defined(CONFIG_NET_BUF_LOG) && SYS_LOG_LEVEL >= SYS_LOG_LEVEL_WARNING
	if (timeout == K_FOREVER) {
		uint32_t ref = k_uptime_get_32();
		buf = k_lifo_get(&pool->free, K_NO_WAIT);
		NET_BUF_DBG("%d buf %p", __LINE__, buf);
		while (!buf) {
#if defined(CONFIG_NET_BUF_POOL_USAGE)
			NET_BUF_WARN("%s():%d: Pool %s low on buffers.",
				     func, line, pool->name);
#else
			NET_BUF_WARN("%s():%d: Pool %p low on buffers.",
				     func, line, pool);
#endif
			buf = k_lifo_get(&pool->free, WARN_ALLOC_INTERVAL);
#if defined(CONFIG_NET_BUF_POOL_USAGE)
			NET_BUF_WARN("%s():%d: Pool %s blocked for %u secs",
				     func, line, pool->name,
				     (k_uptime_get_32() - ref) / MSEC_PER_SEC);
#else
			NET_BUF_WARN("%s():%d: Pool %p blocked for %u secs",
				     func, line, pool,
				     (k_uptime_get_32() - ref) / MSEC_PER_SEC);
#endif
		}
	} else {
		buf = k_lifo_get(&pool->free, timeout);
	}
#else
	buf = k_lifo_get(&pool->free, timeout);
#endif
	if (!buf) {
		NET_BUF_ERR("%s():%d: Failed to get free buffer", func, line);
		return NULL;
	}

success:
	NET_BUF_DBG("allocated buf %p", buf);

	if (size) {
#if __ASSERT_ON
		size_t req_size = size;
		(void)req_size;
#endif
		if (timeout != K_NO_WAIT && timeout != K_FOREVER) {
			u32_t diff = k_uptime_get_32() - alloc_start;

			timeout -= MIN(timeout, diff);
		}

		buf->__buf = data_alloc(buf, &size, timeout);
		if (!buf->__buf) {
			NET_BUF_ERR("%s():%d: Failed to allocate data",
				    func, line);
			net_buf_destroy(buf);
			return NULL;
		}

#if __ASSERT_ON
		NET_BUF_ASSERT(req_size <= size);
#endif
	} else {
		buf->__buf = NULL;
	}

	buf->ref   = 1U;
	buf->flags = 0U;
	buf->frags = NULL;
	buf->size  = size;
	net_buf_reset(buf);

#if defined(CONFIG_NET_BUF_POOL_USAGE)
	pool->avail_count--;
	NET_BUF_ASSERT(pool->avail_count >= 0);
#endif
	return buf;
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_alloc_fixed_debug(struct net_buf_pool *pool,
					  s32_t timeout, const char *func,
					  int line)
{
	const struct net_buf_pool_fixed *fixed = pool->alloc->alloc_data;
	NET_BUF_DBG("%d", __LINE__);
	return net_buf_alloc_len_debug(pool, fixed->data_size, timeout, func,
				       line);
}
#else
struct net_buf *net_buf_alloc_fixed(struct net_buf_pool *pool, s32_t timeout)
{
	const struct net_buf_pool_fixed *fixed = pool->alloc->alloc_data;
	NET_BUF_DBG("%d", __LINE__);
	return net_buf_alloc_len(pool, fixed->data_size, timeout);
}
#endif

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_alloc_with_data_debug(struct net_buf_pool *pool,
					      void *data, size_t size,
					      s32_t timeout, const char *func,
					      int line)
#else
struct net_buf *net_buf_alloc_with_data(struct net_buf_pool *pool,
					void *data, size_t size,
					s32_t timeout)
#endif
{
	struct net_buf *buf;

#if defined(CONFIG_NET_BUF_LOG)
	buf = net_buf_alloc_len_debug(pool, 0, timeout, func, line);
#else
	buf = net_buf_alloc_len(pool, 0, timeout);
#endif
	if (!buf) {
		return NULL;
	}

	net_buf_simple_init_with_data(&buf->b, data, size);
	buf->flags = NET_BUF_EXTERNAL_DATA;

	return buf;
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_get_debug(struct k_fifo *fifo, s32_t timeout,
				  const char *func, int line)
#else
struct net_buf *net_buf_get(struct k_fifo *fifo, s32_t timeout)
#endif
{
	struct net_buf *buf, *frag;

	NET_BUF_DBG("%s():%d: fifo %p timeout %d", func, line, fifo, timeout);

	buf = k_fifo_get(fifo, timeout);
	if (!buf) {
		return NULL;
	}

	NET_BUF_DBG("%s():%d: buf %p fifo %p", func, line, buf, fifo);

	/* Get any fragments belonging to this buffer */
	for (frag = buf; (frag->flags & NET_BUF_FRAGS); frag = frag->frags) {
		frag->frags = k_fifo_get(fifo, K_NO_WAIT);
		NET_BUF_ASSERT(frag->frags);

		/* The fragments flag is only for FIFO-internal usage */
		frag->flags &= ~NET_BUF_FRAGS;
	}

	/* Mark the end of the fragment list */
	frag->frags = NULL;

	return buf;
}

void net_buf_simple_init_with_data(struct net_buf_simple *buf,
				   void *data, size_t size)
{
	buf->__buf = data;
	buf->data  = data;
	buf->size  = size;
	buf->len   = size;
}

void net_buf_simple_reserve(struct net_buf_simple *buf, size_t reserve)
{
	NET_BUF_ASSERT(buf);
	NET_BUF_ASSERT(buf->len == 0);
	NET_BUF_DBG("buf %p reserve %u", buf, reserve);

	buf->data = buf->__buf + reserve;
}

void net_buf_slist_put(sys_slist_t *list, struct net_buf *buf)
{
	struct net_buf *tail;
	unsigned int key;

	NET_BUF_ASSERT(list);
	NET_BUF_ASSERT(buf);

	for (tail = buf; tail->frags; tail = tail->frags) {
		tail->flags |= NET_BUF_FRAGS;
	}

	key = irq_lock();
	sys_slist_append_list(list, &buf->node, &tail->node);
	irq_unlock(key);
}

struct net_buf *net_buf_slist_get(sys_slist_t *list)
{
	struct net_buf *buf, *frag;
	unsigned int key;

	NET_BUF_ASSERT(list);

	key = irq_lock();
	buf = (void *)sys_slist_get(list);
	irq_unlock(key);

	if (!buf) {
		return NULL;
	}

	/* Get any fragments belonging to this buffer */
	for (frag = buf; (frag->flags & NET_BUF_FRAGS); frag = frag->frags) {
		key = irq_lock();
		frag->frags = (void *)sys_slist_get(list);
		irq_unlock(key);

		NET_BUF_ASSERT(frag->frags);

		/* The fragments flag is only for list-internal usage */
		frag->flags &= ~NET_BUF_FRAGS;
	}

	/* Mark the end of the fragment list */
	frag->frags = NULL;

	return buf;
}

void net_buf_put(struct k_fifo *fifo, struct net_buf *buf)
{
	struct net_buf *tail;

	NET_BUF_ASSERT(fifo);
	NET_BUF_ASSERT(buf);

	for (tail = buf; tail->frags; tail = tail->frags) {
		tail->flags |= NET_BUF_FRAGS;
	}

	k_fifo_put_list(fifo, buf, tail);
}

#if defined(CONFIG_NET_BUF_LOG)
void net_buf_unref_debug(struct net_buf *buf, const char *func, int line)
#else
void net_buf_unref(struct net_buf *buf)
#endif
{
	NET_BUF_ASSERT(buf);

	while (buf) {
		struct net_buf *frags = buf->frags;
		struct net_buf_pool *pool;

#if defined(CONFIG_NET_BUF_LOG)
		if (!buf->ref) {
			NET_BUF_ERR("%s():%d: buf %p double free", func, line,
				    buf);
			return;
		}
#endif
		NET_BUF_DBG("buf %p ref %u pool_id %u frags %p", buf, buf->ref,
			    buf->pool_id, buf->frags);

		if (--buf->ref > 0) {
			return;
		}

		if (buf->__buf) {
			data_unref(buf, buf->__buf);
			buf->__buf = NULL;
		}

		buf->data = NULL;
		buf->frags = NULL;

		pool = net_buf_pool_get(buf->pool_id);

#if defined(CONFIG_NET_BUF_POOL_USAGE)
		pool->avail_count++;
		NET_BUF_ASSERT(pool->avail_count <= pool->buf_count);
#endif

		if (pool->destroy) {
			pool->destroy(buf);
		} else {
			net_buf_destroy(buf);
		}

		buf = frags;
	}
}

struct net_buf *net_buf_ref(struct net_buf *buf)
{
	NET_BUF_ASSERT(buf);

	NET_BUF_DBG("buf %p (old) ref %u pool_id %u",
		    buf, buf->ref, buf->pool_id);
	buf->ref++;
	return buf;
}

struct net_buf *net_buf_clone(struct net_buf *buf, s32_t timeout)
{
	u32_t alloc_start = k_uptime_get_32();
	struct net_buf_pool *pool;
	struct net_buf *clone;

	NET_BUF_ASSERT(buf);

	pool = net_buf_pool_get(buf->pool_id);

	clone = net_buf_alloc_len(pool, 0, timeout);
	if (!clone) {
		return NULL;
	}

	/* If the pool supports data referencing use that. Otherwise
	 * we need to allocate new data and make a copy.
	 */
	if (pool->alloc->cb->ref && !(buf->flags & NET_BUF_EXTERNAL_DATA)) {
		clone->__buf = data_ref(buf, buf->__buf);
		clone->data = buf->data;
		clone->len = buf->len;
		clone->size = buf->size;
	} else {
		size_t size = buf->size;

		if (timeout != K_NO_WAIT && timeout != K_FOREVER) {
			u32_t diff = k_uptime_get_32() - alloc_start;

			timeout -= MIN(timeout, diff);
		}

		clone->__buf = data_alloc(clone, &size, timeout);
		if (!clone->__buf || size < buf->size) {
			net_buf_destroy(clone);
			return NULL;
		}

		clone->size = size;
		clone->data = clone->__buf + net_buf_headroom(buf);
		net_buf_add_mem(clone, buf->data, buf->len);
	}

	return clone;
}

struct net_buf *net_buf_frag_last(struct net_buf *buf)
{
	NET_BUF_ASSERT(buf);

	while (buf->frags) {
		buf = buf->frags;
	}

	return buf;
}

void net_buf_frag_insert(struct net_buf *parent, struct net_buf *frag)
{
	NET_BUF_ASSERT(parent);
	NET_BUF_ASSERT(frag);

	if (parent->frags) {
		net_buf_frag_last(frag)->frags = parent->frags;
	}
	/* Take ownership of the fragment reference */
	parent->frags = frag;
}

struct net_buf *net_buf_frag_add(struct net_buf *head, struct net_buf *frag)
{
	NET_BUF_ASSERT(frag);

	if (!head) {
		return net_buf_ref(frag);
	}

	net_buf_frag_insert(net_buf_frag_last(head), frag);

	return head;
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_frag_del_debug(struct net_buf *parent,
				       struct net_buf *frag,
				       const char *func, int line)
#else
struct net_buf *net_buf_frag_del(struct net_buf *parent, struct net_buf *frag)
#endif
{
	struct net_buf *next_frag;

	NET_BUF_ASSERT(frag);

	if (parent) {
		NET_BUF_ASSERT(parent->frags);
		NET_BUF_ASSERT(parent->frags == frag);
		parent->frags = frag->frags;
	}

	next_frag = frag->frags;

	frag->frags = NULL;

#if defined(CONFIG_NET_BUF_LOG)
	net_buf_unref_debug(frag, func, line);
#else
	net_buf_unref(frag);
#endif

	return next_frag;
}

size_t net_buf_linearize(void *dst, size_t dst_len, struct net_buf *src,
			 size_t offset, size_t len)
{
	struct net_buf *frag;
	size_t to_copy;
	size_t copied;

	len = MIN(len, dst_len);

	frag = src;

#if 0
	/* clear dst */
	memset(dst, 0, dst_len);
#endif

	/* find the right fragment to start copying from */
	while (frag && offset >= frag->len) {
		offset -= frag->len;
		frag = frag->frags;
	}

	/* traverse the fragment chain until len bytes are copied */
	copied = 0;
	while (frag && len > 0) {
		to_copy = MIN(len, frag->len - offset);
		memcpy((uint8_t *)dst + copied, frag->data + offset, to_copy);

		copied += to_copy;

		/* to_copy is always <= len */
		len -= to_copy;
		frag = frag->frags;

		/* after the first iteration, this value will be 0 */
		offset = 0;
	}

	return copied;
}

/* This helper routine will append multiple bytes, if there is no place for
 * the data in current fragment then create new fragment and add it to
 * the buffer. It assumes that the buffer has at least one fragment.
 */
size_t net_buf_append_bytes(struct net_buf *buf, size_t len,
			    const void *value, s32_t timeout,
			    net_buf_allocator_cb allocate_cb, void *user_data)
{
	struct net_buf *frag = net_buf_frag_last(buf);
	size_t added_len = 0;
	const uint8_t *value8 = value;

	do {
		uint16_t count = MIN(len, net_buf_tailroom(frag));

		net_buf_add_mem(frag, value8, count);
		len -= count;
		added_len += count;
		value8 += count;

		if (len == 0) {
			return added_len;
		}

		frag = allocate_cb(timeout, user_data);
		if (!frag) {
			return added_len;
		}

		net_buf_frag_add(buf, frag);
	} while (1);

	/* Unreachable */
	return 0;
}

#if defined(CONFIG_NET_BUF_SIMPLE_LOG)
#define NET_BUF_SIMPLE_DBG(fmt, ...) NET_BUF_DBG(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_ERR(fmt, ...) NET_BUF_ERR(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_WARN(fmt, ...) NET_BUF_WARN(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_INFO(fmt, ...) NET_BUF_INFO(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_ASSERT(cond) NET_BUF_ASSERT(cond)
#else
#define NET_BUF_SIMPLE_DBG(fmt, ...)
#define NET_BUF_SIMPLE_ERR(fmt, ...)
#define NET_BUF_SIMPLE_WARN(fmt, ...)
#define NET_BUF_SIMPLE_INFO(fmt, ...)
#define NET_BUF_SIMPLE_ASSERT(cond)
#endif /* CONFIG_NET_BUF_SIMPLE_LOG */

void net_buf_simple_clone(const struct net_buf_simple *original,
			  struct net_buf_simple *clone)
{
	memcpy(clone, original, sizeof(struct net_buf_simple));
}

void *net_buf_simple_add(struct net_buf_simple *buf, size_t len)
{
	uint8_t *tail = net_buf_simple_tail(buf);

	NET_BUF_SIMPLE_DBG("buf %p len %d", buf, len);

	NET_BUF_SIMPLE_ASSERT(net_buf_simple_tailroom(buf) >= len);

	buf->len += len;
	return tail;
}

void *net_buf_simple_add_mem(struct net_buf_simple *buf, const void *mem,
			     size_t len)
{
	NET_BUF_SIMPLE_DBG("buf %p len %d", buf, len);

	return memcpy(net_buf_simple_add(buf, len), mem, len);
}

uint8_t *net_buf_simple_add_u8(struct net_buf_simple *buf, uint8_t val)
{
	uint8_t *u8;

	NET_BUF_SIMPLE_DBG("buf %p val 0x%02x", buf, val);

	u8 = net_buf_simple_add(buf, 1);
	*u8 = val;

	return u8;
}

void net_buf_simple_add_le16(struct net_buf_simple *buf, uint16_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_le16(val, net_buf_simple_add(buf, sizeof(val)));
}

void net_buf_simple_add_be16(struct net_buf_simple *buf, uint16_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_be16(val, net_buf_simple_add(buf, sizeof(val)));
}

void net_buf_simple_add_le24(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_le24(val, net_buf_simple_add(buf, 3));
}

void net_buf_simple_add_be24(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_be24(val, net_buf_simple_add(buf, 3));
}

void net_buf_simple_add_le32(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_le32(val, net_buf_simple_add(buf, sizeof(val)));
}

void net_buf_simple_add_be32(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_be32(val, net_buf_simple_add(buf, sizeof(val)));
}

void net_buf_simple_add_le48(struct net_buf_simple *buf, uint64_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %" PRIu64, buf, val);

	sys_put_le48(val, net_buf_simple_add(buf, 6));
}

void net_buf_simple_add_be48(struct net_buf_simple *buf, uint64_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %" PRIu64, buf, val);

	sys_put_be48(val, net_buf_simple_add(buf, 6));
}

void net_buf_simple_add_le64(struct net_buf_simple *buf, uint64_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %" PRIu64, buf, val);

	sys_put_le64(val, net_buf_simple_add(buf, sizeof(val)));
}

void net_buf_simple_add_be64(struct net_buf_simple *buf, uint64_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %" PRIu64, buf, val);

	sys_put_be64(val, net_buf_simple_add(buf, sizeof(val)));
}

void *net_buf_simple_remove_mem(struct net_buf_simple *buf, size_t len)
{
	NET_BUF_SIMPLE_DBG("buf %p len %d", buf, len);

	NET_BUF_SIMPLE_ASSERT(buf->len >= len);

	buf->len -= len;
	return buf->data + buf->len;
}

uint8_t net_buf_simple_remove_u8(struct net_buf_simple *buf)
{
	uint8_t val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = *(uint8_t *)ptr;

	return val;
}

uint16_t net_buf_simple_remove_le16(struct net_buf_simple *buf)
{
	uint16_t val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((uint16_t *)ptr);

	return sys_le16_to_cpu(val);
}

uint16_t net_buf_simple_remove_be16(struct net_buf_simple *buf)
{
	uint16_t val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((uint16_t *)ptr);

	return sys_be16_to_cpu(val);
}

uint32_t net_buf_simple_remove_le24(struct net_buf_simple *buf)
{
	struct uint24 {
		uint32_t u24 : 24;
	} __packed val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((struct uint24 *)ptr);

	return sys_le24_to_cpu(val.u24);
}

uint32_t net_buf_simple_remove_be24(struct net_buf_simple *buf)
{
	struct uint24 {
		uint32_t u24 : 24;
	} __packed val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((struct uint24 *)ptr);

	return sys_be24_to_cpu(val.u24);
}

uint32_t net_buf_simple_remove_le32(struct net_buf_simple *buf)
{
	uint32_t val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((uint32_t *)ptr);

	return sys_le32_to_cpu(val);
}

uint32_t net_buf_simple_remove_be32(struct net_buf_simple *buf)
{
	uint32_t val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((uint32_t *)ptr);

	return sys_be32_to_cpu(val);
}

uint64_t net_buf_simple_remove_le48(struct net_buf_simple *buf)
{
	struct uint48 {
		uint64_t u48 : 48;
	} __packed val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((struct uint48 *)ptr);

	return sys_le48_to_cpu(val.u48);
}

uint64_t net_buf_simple_remove_be48(struct net_buf_simple *buf)
{
	struct uint48 {
		uint64_t u48 : 48;
	} __packed val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((struct uint48 *)ptr);

	return sys_be48_to_cpu(val.u48);
}

uint64_t net_buf_simple_remove_le64(struct net_buf_simple *buf)
{
	uint64_t val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((uint64_t *)ptr);

	return sys_le64_to_cpu(val);
}

uint64_t net_buf_simple_remove_be64(struct net_buf_simple *buf)
{
	uint64_t val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((uint64_t *)ptr);

	return sys_be64_to_cpu(val);
}

void *net_buf_simple_push(struct net_buf_simple *buf, size_t len)
{
	NET_BUF_SIMPLE_DBG("buf %p len %d", buf, len);

	NET_BUF_SIMPLE_ASSERT(net_buf_simple_headroom(buf) >= len);

	buf->data -= len;
	buf->len += len;
	return buf->data;
}

void *net_buf_simple_push_mem(struct net_buf_simple *buf, const void *mem,
			      size_t len)
{
	NET_BUF_SIMPLE_DBG("buf %p len %d", buf, len);

	return memcpy(net_buf_simple_push(buf, len), mem, len);
}

void net_buf_simple_push_le16(struct net_buf_simple *buf, uint16_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_le16(val, net_buf_simple_push(buf, sizeof(val)));
}

void net_buf_simple_push_be16(struct net_buf_simple *buf, uint16_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_be16(val, net_buf_simple_push(buf, sizeof(val)));
}

void net_buf_simple_push_u8(struct net_buf_simple *buf, uint8_t val)
{
	uint8_t *data = net_buf_simple_push(buf, 1);

	*data = val;
}

void net_buf_simple_push_le24(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_le24(val, net_buf_simple_push(buf, 3));
}

void net_buf_simple_push_be24(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_be24(val, net_buf_simple_push(buf, 3));
}

void net_buf_simple_push_le32(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_le32(val, net_buf_simple_push(buf, sizeof(val)));
}

void net_buf_simple_push_be32(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_be32(val, net_buf_simple_push(buf, sizeof(val)));
}

void net_buf_simple_push_le48(struct net_buf_simple *buf, uint64_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %" PRIu64, buf, val);

	sys_put_le48(val, net_buf_simple_push(buf, 6));
}

void net_buf_simple_push_be48(struct net_buf_simple *buf, uint64_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %" PRIu64, buf, val);

	sys_put_be48(val, net_buf_simple_push(buf, 6));
}

void net_buf_simple_push_le64(struct net_buf_simple *buf, uint64_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %" PRIu64, buf, val);

	sys_put_le64(val, net_buf_simple_push(buf, sizeof(val)));
}

void net_buf_simple_push_be64(struct net_buf_simple *buf, uint64_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %" PRIu64, buf, val);

	sys_put_be64(val, net_buf_simple_push(buf, sizeof(val)));
}

void *net_buf_simple_pull(struct net_buf_simple *buf, size_t len)
{
	NET_BUF_SIMPLE_DBG("buf %p len %d", buf, len);

	NET_BUF_SIMPLE_ASSERT(buf->len >= len);

	buf->len -= len;
	return buf->data += len;
}

void *net_buf_simple_pull_mem(struct net_buf_simple *buf, size_t len)
{
	void *data = buf->data;

	NET_BUF_SIMPLE_DBG("buf %p len %u", buf, len);

	NET_BUF_SIMPLE_ASSERT(buf->len >= len);

	buf->len -= len;
	buf->data += len;

	return data;
}

uint8_t net_buf_simple_pull_u8(struct net_buf_simple *buf)
{
	uint8_t val;

	val = buf->data[0];
	net_buf_simple_pull(buf, 1);

	return val;
}

uint16_t net_buf_simple_pull_le16(struct net_buf_simple *buf)
{
	uint16_t val;

	val = UNALIGNED_GET((uint16_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_le16_to_cpu(val);
}

uint16_t net_buf_simple_pull_be16(struct net_buf_simple *buf)
{
	uint16_t val;

	val = UNALIGNED_GET((uint16_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_be16_to_cpu(val);
}

uint32_t net_buf_simple_pull_le24(struct net_buf_simple *buf)
{
	struct uint24 {
		uint32_t u24:24;
	} __packed val;

	val = UNALIGNED_GET((struct uint24 *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_le24_to_cpu(val.u24);
}

uint32_t net_buf_simple_pull_be24(struct net_buf_simple *buf)
{
	struct uint24 {
		uint32_t u24:24;
	} __packed val;

	val = UNALIGNED_GET((struct uint24 *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_be24_to_cpu(val.u24);
}

uint32_t net_buf_simple_pull_le32(struct net_buf_simple *buf)
{
	uint32_t val;

	val = UNALIGNED_GET((uint32_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_le32_to_cpu(val);
}

uint32_t net_buf_simple_pull_be32(struct net_buf_simple *buf)
{
	uint32_t val;

	val = UNALIGNED_GET((uint32_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_be32_to_cpu(val);
}

uint64_t net_buf_simple_pull_le48(struct net_buf_simple *buf)
{
	struct uint48 {
		uint64_t u48:48;
	} __packed val;

	val = UNALIGNED_GET((struct uint48 *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_le48_to_cpu(val.u48);
}

uint64_t net_buf_simple_pull_be48(struct net_buf_simple *buf)
{
	struct uint48 {
		uint64_t u48:48;
	} __packed val;

	val = UNALIGNED_GET((struct uint48 *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_be48_to_cpu(val.u48);
}

uint64_t net_buf_simple_pull_le64(struct net_buf_simple *buf)
{
	uint64_t val;

	val = UNALIGNED_GET((uint64_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_le64_to_cpu(val);
}

uint64_t net_buf_simple_pull_be64(struct net_buf_simple *buf)
{
	uint64_t val;

	val = UNALIGNED_GET((uint64_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_be64_to_cpu(val);
}

size_t net_buf_simple_headroom(struct net_buf_simple *buf)
{
	return buf->data - buf->__buf;
}

size_t net_buf_simple_tailroom(struct net_buf_simple *buf)
{
	return buf->size - net_buf_simple_headroom(buf) - buf->len;
}

#if defined(CONFIG_BT_DEINIT)
void bt_buf_reset(void)
{
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	NET_BUF_POOL_FIXED_DEINIT(&acl_in_pool, CONFIG_BT_ACL_RX_COUNT, ACL_IN_SIZE, report_completed_packet);
#endif

	NET_BUF_POOL_FIXED_DEINIT(&hci_rx_pool, CONFIG_BT_RX_BUF_COUNT,
		BT_BUF_RX_SIZE, NULL);

#if defined(CONFIG_BT_CONN)
	NET_BUF_POOL_FIXED_DEINIT(&num_complete_pool, 1, NUM_COMLETE_EVENT_SIZE, NULL);
#endif

#if defined(CONFIG_BT_DISCARDABLE_BUF_COUNT)
	NET_BUF_POOL_FIXED_DEINIT(&discardable_pool, CONFIG_BT_DISCARDABLE_BUF_COUNT,
		DISCARDABLE_EVENT_SIZE, NULL);
#endif
}
#endif
