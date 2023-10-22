/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <aw_list.h>
#include <hal_mutex.h>
#include <hal_mem.h>
#include <hal_time.h>
#include <rpbuf.h>


#define RPBUF_LOG(controller_id, buf_name, buf_len, fmt, args...) \
	printf("[%d|%s|%d] " fmt, controller_id, buf_name, buf_len, ##args)

struct rpbuf_buffer_entry {
	int controller_id;
	struct rpbuf_buffer *buffer;
	struct list_head list;
};

//static unsigned long g_tran_time = 0;

LIST_HEAD(rpbuf_buffers);
static hal_mutex_t rpbuf_buffers_mutex;

static struct rpbuf_buffer_entry *find_buffer_entry(const char *name)
{
	struct rpbuf_buffer_entry *buf_entry;

	list_for_each_entry(buf_entry, &rpbuf_buffers, list) {
		if (0 == strcmp(rpbuf_buffer_name(buf_entry->buffer), name))
			return buf_entry;
	}
	return NULL;
}

static void rpbuf_buffer_available_cb(struct rpbuf_buffer *buffer, void *priv)
{
	printf("buffer \"%s\" is available\n", rpbuf_buffer_name(buffer));
}

static int rpbuf_buffer_rx_cb(struct rpbuf_buffer *buffer,
		void *data, int data_len, void *priv)
{
	int i;

	printf("buffer \"%s\" received data (addr: %p, offset: %d, len: %d):\n",
			rpbuf_buffer_name(buffer), rpbuf_buffer_va(buffer),
			data - rpbuf_buffer_va(buffer), data_len);
	for (i = 0; i < data_len; ++i)
		printf(" 0x%x,", *((uint8_t *)(data) + i));
	printf("\n");

	return 0;
}

static int rpbuf_buffer_destroyed_cb(struct rpbuf_buffer *buffer, void *priv)
{
	printf("buffer \"%s\": remote buffer destroyed\n", rpbuf_buffer_name(buffer));

	return 0;
}

static const struct rpbuf_buffer_cbs rpbuf_cbs = {
	.available_cb = rpbuf_buffer_available_cb,
	.rx_cb = rpbuf_buffer_rx_cb,
	.destroyed_cb = rpbuf_buffer_destroyed_cb,
};

int rpbuf_common_init()
{
	int ret = 0;
	if (!rpbuf_buffers_mutex) {
		rpbuf_buffers_mutex = hal_mutex_create();
		if (!rpbuf_buffers_mutex) {
			printf("hal_mutex_create failed\n");
			ret = -1;
		}
	}
	return ret;
}

void rpbuf_common_deinit()
{
	if (rpbuf_buffers_mutex)
		hal_mutex_delete(rpbuf_buffers_mutex);

	rpbuf_buffers_mutex = NULL;
}

int rpbuf_common_create(int controller_id, const char *name, int len, bool sync, const struct rpbuf_buffer_cbs* rpbuf_cb, void *priv)
{
	int ret;
	struct rpbuf_buffer_entry *buf_entry = NULL;
	struct rpbuf_controller *controller = NULL;
	struct rpbuf_buffer *buffer = NULL;


	hal_mutex_lock(rpbuf_buffers_mutex);
	buf_entry = find_buffer_entry(name);
	if (buf_entry) {
		printf("Buffer named \"%s\" already exists\n", name);
		hal_mutex_unlock(rpbuf_buffers_mutex);
		ret = -EINVAL;
		goto err_out;
	}
	hal_mutex_unlock(rpbuf_buffers_mutex);

	buf_entry = hal_malloc(sizeof(struct rpbuf_buffer_entry));
	if (!buf_entry) {
		RPBUF_LOG(controller_id, name, len,
				"Failed to allocate memory for buffer entry\n");
		ret = -ENOMEM;
		goto err_out;
	}
	buf_entry->controller_id = controller_id;

	controller = rpbuf_get_controller_by_id(controller_id);
	if (!controller) {
		RPBUF_LOG(controller_id, name, len,
				"Failed to get controller%d, controller_id\n", controller_id);
		ret = -ENOENT;
		goto err_free_buf_entry;
	}

	if (!rpbuf_cb)
		rpbuf_cb = &rpbuf_cbs;

	buffer = rpbuf_alloc_buffer(controller, name, len, NULL, rpbuf_cb, priv);
	if (!buffer) {
		RPBUF_LOG(controller_id, name, len, "rpbuf_alloc_buffer failed\n");
		ret = -ENOENT;
		goto err_free_buf_entry;
	}
	buf_entry->buffer = buffer;

	if (sync)
		rpbuf_buffer_set_sync(buffer, sync);

	hal_mutex_lock(rpbuf_buffers_mutex);
	list_add_tail(&buf_entry->list, &rpbuf_buffers);
	hal_mutex_unlock(rpbuf_buffers_mutex);

	return 0;

err_free_buf_entry:
	hal_free(buf_entry);
err_out:
	return ret;
}

int rpbuf_common_destroy(const char *name)
{
	int ret;
	struct rpbuf_buffer_entry *buf_entry;
	struct rpbuf_buffer *buffer;

	if (rpbuf_buffers_mutex)
		hal_mutex_lock(rpbuf_buffers_mutex);

	buf_entry = find_buffer_entry(name);
	if (!buf_entry) {
		printf("Buffer named \"%s\" not found\n", name);
		hal_mutex_unlock(rpbuf_buffers_mutex);
		ret = -EINVAL;
		goto err_out;
	}
	buffer = buf_entry->buffer;

	ret = rpbuf_free_buffer(buffer);
	if (ret < 0) {
		RPBUF_LOG(buf_entry->controller_id,
				rpbuf_buffer_name(buffer),
				rpbuf_buffer_len(buffer),
				"rpbuf_free_buffer failed\n");
		hal_mutex_unlock(rpbuf_buffers_mutex);
		goto err_out;
	}

	list_del(&buf_entry->list);
	hal_free(buf_entry);

	hal_mutex_unlock(rpbuf_buffers_mutex);

	return 0;

err_out:
	return ret;
}

int rpbuf_common_transmit(const char *name, void *data, int data_len, int offset)
{
	int ret;
	struct rpbuf_buffer_entry *buf_entry;
	struct rpbuf_buffer *buffer;

	hal_mutex_lock(rpbuf_buffers_mutex);
	buf_entry = find_buffer_entry(name);
	if (!buf_entry) {
		printf("Buffer named \"%s\" not found\n", name);
		hal_mutex_unlock(rpbuf_buffers_mutex);
		ret = -EINVAL;
		goto err_out;
	}
	buffer = buf_entry->buffer;

	/*
	 * Before putting data to buffer or sending buffer to remote, we should
	 * ensure that the buffer is available.
	 */
	if (!rpbuf_buffer_is_available(buffer)) {
		RPBUF_LOG(buf_entry->controller_id,
				rpbuf_buffer_name(buffer),
				rpbuf_buffer_len(buffer),
				"buffer not available\n");
		hal_mutex_unlock(rpbuf_buffers_mutex);
		ret = -EACCES;
		goto err_out;
	}

	if (data_len > rpbuf_buffer_len(buffer)) {
		RPBUF_LOG(buf_entry->controller_id,
				rpbuf_buffer_name(buffer),
				rpbuf_buffer_len(buffer),
				"data length too long\n");
		hal_mutex_unlock(rpbuf_buffers_mutex);
		ret = -EACCES;
		goto err_out;
	}
	memcpy(rpbuf_buffer_va(buffer) + offset, data, data_len);

	ret = rpbuf_transmit_buffer(buffer, offset, data_len);
	if (ret < 0) {
		RPBUF_LOG(buf_entry->controller_id,
				rpbuf_buffer_name(buffer),
				rpbuf_buffer_len(buffer),
				"rpbuf_transmit_buffer failed\n");
		hal_mutex_unlock(rpbuf_buffers_mutex);
		goto err_out;
	}
/*
	printf("rpbuf %s, time: %ld, delta time: %ld,transmit to remote\n",\
		rpbuf_buffer_name(buffer),OSTICK_TO_MS(hal_tick_get()),  OSTICK_TO_MS(hal_tick_get()) - g_tran_time);
	g_tran_time = OSTICK_TO_MS(hal_tick_get());
*/
	hal_mutex_unlock(rpbuf_buffers_mutex);

	return 0;

err_out:
	return ret;
}

void rpbuf_common_list_buffers(void)
{
	struct rpbuf_buffer_entry *buf_entry;
	struct rpbuf_buffer *buffer;

	hal_mutex_lock(rpbuf_buffers_mutex);

	printf("\nCreated buffers:\n");
	list_for_each_entry(buf_entry, &rpbuf_buffers, list) {
		buffer = buf_entry->buffer;
		printf("  [%d|%s] id: %d, va: %p, pa: 0x%x, len: %d\n",
				buf_entry->controller_id,
				rpbuf_buffer_name(buffer), rpbuf_buffer_id(buffer),
				rpbuf_buffer_va(buffer), rpbuf_buffer_pa(buffer),
				rpbuf_buffer_len(buffer));
	}
	printf("\n");

	hal_mutex_unlock(rpbuf_buffers_mutex);
}

