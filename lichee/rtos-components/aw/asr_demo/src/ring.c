#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include <hal_mem.h>
#include "ring.h"
#include "data_save.h"

#define CONFIG_RING_TEST_CMD
#ifdef CONFIG_RING_TEST_CMD
#include <console.h>
#endif

struct ring_t {
	volatile unsigned int recv_cnt;
	volatile unsigned int send_cnt;
	unsigned int size;
	unsigned char *data;
	int cnt;
	char *name;
	void *ds_send;
	void *ds_recv;
};

void ring_destroy(void *_hdl)
{
	struct ring_t *hdl = (struct ring_t *)_hdl;

	if(hdl) {
		if (hdl->ds_recv) {
			data_save_destroy(hdl->ds_recv);
			hdl->ds_recv = NULL;
		}
		if (hdl->ds_send) {
			data_save_destroy(hdl->ds_send);
			hdl->ds_send = NULL;
		}
		if(hdl->name) {
			hal_free(hdl->name);
			hdl->name = NULL;
		}
		if (hdl->data) {
			hal_free(hdl->data);
			hdl->data = NULL;
		}
		hal_free(hdl);
	}
}

void *ring_create(unsigned int size, const char *name, struct ring_debug_info_t *info)
{
	struct ring_t *hdl = hal_malloc(sizeof(*hdl));

	if (!hdl) {
		printf("no memory!\n");
		goto err;
	}
	memset(hdl, 0, sizeof(*hdl));

	hdl->data = hal_malloc(size);
	if (!hdl->data) {
		printf("no memory!\n");
		goto err;
	}
	hdl->size = size;

	if (name) {
		int len = strlen(name) + 1;
		hdl->name = hal_malloc(len);
		if (!hdl->name) {
			printf("no memory!\n");
			goto err;
		}
		memcpy(hdl->name, name, len);
	}

	if (info && (info->port > 0 || info->file_path)) {
		hdl->ds_recv = data_save_create(name, info->file_path, info->port);
		if (!hdl->ds_recv) {
			printf("no memory!\n");
			goto err;
		}
	}

	return hdl;
err:
	ring_destroy(hdl);
	return NULL;
}

int ring_send_data(void *_hdl, void *_data, int _size)
{
	struct ring_t *ring = (struct ring_t *)_hdl;

	unsigned char *from = (unsigned char *)_data;
	unsigned int size = (unsigned int)_size;
	unsigned char *to = &ring->data[ring->send_cnt % ring->size];
	unsigned int limit = ring->size - ring->send_cnt % ring->size;

	if (size > ring->size) {
		printf("%s size too large! %d > %d\n", __func__, size, ring->size);
		return -1;
	}

	if (size > limit) {
		memcpy(to, from, limit);
		size -= limit;
		from += limit;
		ring->send_cnt += limit;
		to = &ring->data[0];
	}

	memcpy(to, from, size);
	ring->send_cnt += size;

	if (_size && ring->ds_send)
		data_save_request(ring->ds_send, _data, _size, 200);

	return _size;
}

static inline unsigned int ring_recv_check(struct ring_t *ring)
{
	unsigned int overrun_size = 0;
	while(1) {
		if ((ring->send_cnt - ring->recv_cnt) > (ring->size * 2)) {
			ring->recv_cnt += ring->size;
			overrun_size += ring->size;
		} else if ((ring->send_cnt - ring->recv_cnt) > ring->size) {
			int add = ring->send_cnt - ring->recv_cnt - ring->size;
			ring->recv_cnt += add;
			overrun_size += add;
			break;
		} else {
			break;
		}
	}
	if (overrun_size)
		printf("ring:%s overrun %u bytes...\n", ring->name ? ring->name : "<NULL>", overrun_size);

	return overrun_size;
}

int ring_recv_data(void *_hdl, void *_data, int _size)
{
	struct ring_t *ring = (struct ring_t *)_hdl;

	unsigned int overrun_size = ring_recv_check(ring);
	unsigned char *to = (unsigned char *)_data;
	unsigned int size = (unsigned int)_size;
	unsigned char *from = &ring->data[ring->recv_cnt % ring->size];
	unsigned int limit = ring->size - ring->recv_cnt % ring->size;

	if ((ring->send_cnt - ring->recv_cnt) == 0) {
		return 0;
	} else if ((ring->send_cnt - ring->recv_cnt) < _size) {
		_size = ring->send_cnt - ring->recv_cnt;
		size = _size;
	}

	if (size > ring->size) {
		printf("ring:%s size too large! %d > %d\n", ring->name ? ring->name : "<NULL>", size, ring->size);
		return -1;
	}

	if (size > limit) {
		memcpy(to, from, limit);
		size -= limit;
		to += limit;
		ring->recv_cnt += limit;
		from = &ring->data[0];
	}

	memcpy(to, from, size);
	ring->recv_cnt += size;

	if (_size && ring->ds_recv)
		data_save_request(ring->ds_recv, _data, _size, 200);

	return _size;
}

int ring_recv_drop(void *_hdl)
{
	struct ring_t *ring = (struct ring_t *)_hdl;

	unsigned int drop = ring->send_cnt - ring->recv_cnt;
	ring->recv_cnt += drop;
	printf("ring:%s drop %u bytes...\n", ring->name ? ring->name : "<NULL>", drop);

	return drop;
}

int ring_dump(void *_hdl)
{
	struct ring_t *ring = (struct ring_t *)_hdl;

	printf("size: %u\n", ring->size);
	printf("send_cnt: %u\n", ring->send_cnt);
	printf("recv_cnt: %u\n", ring->recv_cnt);
	printf("data: %p\n", ring->data);
	for(unsigned int i = 0; i < ring->size; i++) {
		printf("%02x", ring->data[i]);
		if (!((i+1)%32))
			printf("\n");
	}
	printf("\n");

	return 0;
}

int ring_recv_done(void *_hdl)
{
	struct ring_t *ring = (struct ring_t *)_hdl;

	if (ring->ds_recv) {
		data_save_flush(ring->ds_recv, 200);
	}
	if (ring->ds_send) {
		data_save_flush(ring->ds_send, 200);
	}
	printf("ring:%s done \n", ring->name ? ring->name : "<NULL>");

	return 0;
}

#ifdef CONFIG_RING_TEST_CMD
int cmd_ring_test(int argc, char ** argv)
{
	unsigned int send_size = 0;
	unsigned int recv_size = 0;
	int ret = -1;
	void *ring = ring_create(32 * 1024, "ring_test", NULL);
	unsigned char data[256];

	if (!ring) {
		printf("no memory!\n");
		goto exit;
	}

	for (int i = 0; i < 256; i++) {
		data[i] = 0xff - i;
	}

	ring_recv_drop(ring);

	while(send_size <= (64 * 1024)) {
		for (int i = 1; i < 256; i++) {
			if (send_size > (64 * 1024))
				break;
			unsigned char *ptr = &data[send_size % 256];
			unsigned int max_len = 256 - (send_size % 256);
			unsigned int len = i > max_len ? max_len : i;

			printf("send %u bytes\n", len);
			ring_send_data(ring, ptr, len);
			send_size += len;
		}
	}
	printf("total sned %u bytes\n", send_size);
	ring_dump(ring);
	while(send_size) {
		for (int i = 1; i < 256; i++) {
			int len = ring_recv_data(ring, data, i);
			if (0 == len) {
				send_size = 0;
				break;
			}
			printf("recv %u bytes, %02x\n", len, data[0]);
			recv_size += len;
		}
	}
	printf("total recv %u bytes\n", recv_size);

	ring_recv_done(ring);
	ret = 0;
exit:
	if (ring) {
		ring_destroy(ring);
	}
	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_ring_test, ring_test, ring test);
#endif