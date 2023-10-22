#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <hal_thread.h>
#include <hal_queue.h>
#include <hal_sem.h>
#include <hal_mem.h>
#include <adb_forward.h>
#include "crc32.h"

//#define CONFIG_DATA_SAVE_TEST_CMD
#ifdef CONFIG_DATA_SAVE_TEST_CMD
#include <console.h>
#endif

#define SEM_TIMEOUT_MS		(1000)

struct data_save_request_t {
	void *data;
	int size;
	int cnt;
};

struct data_save_t {
	void *thread;
	hal_sem_t sem;
	hal_queue_t queue;
	int cnt;
	int run;
	char *file_path;
	int port;
};

static void data_save_to_file_thread(void *arg)
{
	struct data_save_t *hdl = (struct data_save_t *)arg;
	int fd = -1;
	int cnt = -1;
	int ret;
	struct data_save_request_t data_save;
	char file_path[32];

	if (hal_sem_post(hdl->sem))
		printf("hal_sem_post failed!\n");

	while (hdl->run) {
		memset(&data_save, 0, sizeof(data_save));
		if(0 > hal_queue_recv(hdl->queue, &data_save, 200)) {
			//hal_msleep(10);
			continue;
		}
		if (cnt != data_save.cnt || !data_save.data) {
			cnt = data_save.cnt;
			if (fd >= 0) {
				close(fd);
				printf("close file_path: %s, fd: %d\n", file_path, fd);
			}
			fd = -1;
		}
		if (!data_save.data)
			continue;
		if (fd < 0) {
			snprintf(file_path, 32, "%s%d.raw", hdl->file_path, cnt);
			fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
			printf("open file_path: %s, fd: %d\n", file_path, fd);
		}
		if (fd >= 0) {
			int need_size = data_save.size;
			unsigned char *data = (unsigned char *)data_save.data;
			int write_size = 0;
			while( (need_size > 0) && (ret = write(fd, &data[write_size], need_size))) {
				if (ret < 0) {
					printf("write %s failed - (%d)\n", file_path, ret);
					break;
				}
				write_size += ret;
				need_size -= ret;
			}
		}
		if (data_save.data)
			hal_free(data_save.data);
	}

	if (fd >= 0)
		close(fd);

	if (hal_sem_post(hdl->sem))
		printf("hal_sem_post failed!\n");

	printf("%s exit\n", __func__);
	hdl->thread = NULL;
	hal_thread_stop(NULL);
}

static inline void data_save_to_file_destroy(void *_hdl)
{
	struct data_save_t *hdl = (struct data_save_t *)_hdl;

	if(hdl) {
		if (hdl->queue) {
			while(!hal_is_queue_empty(hdl->queue))
				hal_msleep(100);
		}

		if (hdl->thread) {
			hdl->run = 0;
			while (0 > hal_sem_timedwait(hdl->sem, pdMS_TO_TICKS(SEM_TIMEOUT_MS))) {
				printf("wait timeout!\n");
				//goto err;
			}
		}
		if (hdl->queue) {
			hal_queue_delete(hdl->queue);
			hdl->queue = NULL;
		}
		if (hdl->sem) {
			hal_sem_delete(hdl->sem);
			hdl->sem = NULL;
		}
		if(hdl->file_path) {
			hal_free(hdl->file_path);
			hdl->file_path = NULL;
		}
		hal_free(hdl);
	}
}

static inline void *data_save_to_file_create(const char *name, const char *file_path)
{
	struct data_save_t *hdl = hal_malloc(sizeof(*hdl));

	if (!hdl) {
		printf("no memory!\n");
		goto err;
	}
	memset(hdl, 0, sizeof(*hdl));

	int len = strlen(file_path) + 1;
	hdl->file_path = hal_malloc(len);
	if (!hdl->file_path) {
		printf("no memory!\n");
		goto err;
	}
	memcpy(hdl->file_path, file_path, len);

	hdl->sem = hal_sem_create(0);
	if (!hdl->sem) {
		printf("no memory!\n");
		goto err;
	}

	hdl->queue = hal_queue_create(name, sizeof(struct data_save_request_t), 20);
	if (!hdl->queue) {
		printf("no memory!\n");
		goto err;
	}

	hdl->run = 1;
	hdl->thread = hal_thread_create(data_save_to_file_thread, hdl, name, 512, 1);
	if (!hdl->thread) {
		printf("no memory!\n");
		goto err;
	}
	hal_thread_start(hdl->thread);

	while (0 > hal_sem_timedwait(hdl->sem, pdMS_TO_TICKS(SEM_TIMEOUT_MS))) {
		printf("wait timeout!\n");
		//goto err;
	}

	return hdl;
err:
	data_save_to_file_destroy(hdl);
	return NULL;
}
static inline int check_all_zero(const unsigned char *p, unsigned int size){
	for (unsigned int i = 0; i < size; i++) {
		if (p[i] != 0)
			return 0;
	}
	return 1;
}
static void data_save_to_adb_thread(void *arg)
{
	struct data_save_t *hdl = (struct data_save_t *)arg;
	int port = -1;
	int cnt = -1;
	unsigned int crc32 = 0;
	int ret;
	struct data_save_request_t data_save;

	if (hal_sem_post(hdl->sem))
		printf("hal_sem_post failed!\n");

	while (hdl->run) {
		memset(&data_save, 0, sizeof(data_save));
		if(0 > hal_queue_recv(hdl->queue, &data_save, 200)) {
			//hal_msleep(10);
			continue;
		}
		if (cnt != data_save.cnt || !data_save.data) {
			cnt = data_save.cnt;
			if (port >= 0) {
				adb_forward_end(port);
				printf("close port: %d, crc32: %08x\n", port, crc32);
				crc32 = 0;
			}
			port = -1;
		}
		if (!data_save.data)
			continue;
		if (port < 0) {
			if (0 > adb_forward_create_with_rawdata(hdl->port))
				port = -1;
			else
				port = hdl->port;
			crc32 = 0;
			printf("open port: %d\n", port);
		}
		if (port >= 0) {
			crc32 = gen_crc32(crc32, data_save.data, data_save.size);
			if (ret = adb_forward_send(port, data_save.data, data_save.size)) {
				printf("write port %d failed - (%d)\n", port, ret);
			}
		}
		if (data_save.data)
			hal_free(data_save.data);
	}

	if (port >= 0)
		adb_forward_end(port);

	if (hal_sem_post(hdl->sem))
		printf("hal_sem_post failed!\n");

	printf("%s exit\n", __func__);
	hdl->thread = NULL;
	hal_thread_stop(NULL);
}

static inline void data_save_to_adb_destroy(void *_hdl)
{
	struct data_save_t *hdl = (struct data_save_t *)_hdl;

	if(hdl) {
		if (hdl->queue) {
			while(!hal_is_queue_empty(hdl->queue))
				hal_msleep(100);
		}

		if (hdl->thread) {
			hdl->run = 0;
			while (0 > hal_sem_timedwait(hdl->sem, pdMS_TO_TICKS(SEM_TIMEOUT_MS))) {
				printf("wait timeout!\n");
				//goto err;
			}
		}
		if (hdl->queue) {
			hal_queue_delete(hdl->queue);
			hdl->queue = NULL;
		}
		if (hdl->sem) {
			hal_sem_delete(hdl->sem);
			hdl->sem = NULL;
		}
		if(hdl->file_path) {
			hal_free(hdl->file_path);
			hdl->file_path = NULL;
		}
		hal_free(hdl);
	}
}

static inline void *data_save_to_adb_create(const char *name, int port)
{
	struct data_save_t *hdl = hal_malloc(sizeof(*hdl));

	if (!hdl) {
		printf("no memory!\n");
		goto err;
	}
	memset(hdl, 0, sizeof(*hdl));

	hdl->port = port;

	hdl->sem = hal_sem_create(0);
	if (!hdl->sem) {
		printf("no memory!\n");
		goto err;
	}

	hdl->queue = hal_queue_create(name, sizeof(struct data_save_request_t), 20);
	if (!hdl->queue) {
		printf("no memory!\n");
		goto err;
	}

	hdl->run = 1;
	hdl->thread = hal_thread_create(data_save_to_adb_thread, hdl, name, 512, 1);
	if (!hdl->thread) {
		printf("no memory!\n");
		goto err;
	}
	hal_thread_start(hdl->thread);

	while (0 > hal_sem_timedwait(hdl->sem, pdMS_TO_TICKS(SEM_TIMEOUT_MS))) {
		printf("wait timeout!\n");
		//goto err;
	}

	return hdl;
err:
	data_save_to_adb_destroy(hdl);
	return NULL;
}


void data_save_destroy(void *_hdl)
{
	struct data_save_t *hdl = (struct data_save_t *)_hdl;

	if (hdl && hdl->port > 0)
		data_save_to_adb_destroy(hdl);
	else
		data_save_to_file_destroy(hdl);
}

void *data_save_create(const char *name, const char *file_path, int port)
{
	if (port > 0)
		return data_save_to_adb_create(name, port);
	else
		return data_save_to_file_create(name, file_path);
}

int data_save_request(void *_hdl, void *data, int size, int timeout_ms)
{
	struct data_save_t *hdl = (struct data_save_t *)_hdl;
	struct data_save_request_t data_save;

	data_save.cnt = hdl->cnt;
	data_save.size = size;
	data_save.data = hal_malloc(size);
	if (!data_save.data) {
		printf("no memory!\n");
		return -1;
	}
	memcpy(data_save.data, data, size);
	if(0 > hal_queue_send_wait(hdl->queue, &data_save, timeout_ms)) {
		printf("data_save sned failed!\n");
		hal_free(data_save.data);
		return -1;
	}

	return 0;
}

int data_save_flush(void *_hdl, int timeout_ms)
{
	struct data_save_t *hdl = (struct data_save_t *)_hdl;
	struct data_save_request_t data_save;

	hdl->cnt++;

	data_save.cnt = hdl->cnt;
	data_save.size = 0;
	data_save.data = NULL;
	if(0 > hal_queue_send_wait(hdl->queue, &data_save, timeout_ms)) {
		printf("data_save flush failed!\n");
		return -1;
	}

	return 0;
}

#ifdef CONFIG_DATA_SAVE_TEST_CMD
int cmd_data_save_test(int argc, char ** argv)
{
	int port = 23333;
	int ms = 3 * 1000;
	int cycle = 5;
	const int ms_per_send = 10;
	int cur_ms = 0;
	int cur_cycle = 0;
	int ret = -1;
	short data[16000 * ms_per_send * 4 / 1000];//10ms 16khz 16bit 4ch 的数据
	void *ds = data_save_create("ds_test", NULL, port);
	unsigned char cnt = 0;

	if (!ds) {
		printf("no memory!\n");
		goto exit;
	}

	while (cur_cycle < cycle) {
		while (cur_ms < ms) {
			cnt++;
			if (cnt == 0)
				cnt++;
			memset(data, cnt, sizeof(data));
			data_save_request(ds, data, sizeof(data), 200);
			hal_msleep(ms_per_send / 2);
			cur_ms += ms_per_send / 2;
		}
		data_save_flush(ds, 200);
		hal_msleep(1500);
		cur_cycle++;
		cur_ms = 0;
		printf("%u\n", cur_cycle);
	}
	ret = 0;
exit:
	data_save_destroy(ds);
	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_data_save_test, data_save_test, data save test);

int cmd_adb_forward_test(int argc, char ** argv)
{
	int port = 23333;
	int ms = 3 * 1000;
	int cycle = 5;
	const int ms_per_send = 80;
	int cur_ms = 0;
	int cur_cycle = 0;
	int ret = -1;
	short data[16000 * ms_per_send * 4 / 1000];//10ms 16khz 16bit 4ch 的数据
	unsigned char cnt = 0;

	while (cur_cycle < cycle) {
		if (0 > adb_forward_create_with_rawdata(port)) {
			printf("adb_forward_create_with_rawdata failed\n");
			goto exit;
		}
		hal_msleep(500);
		while (cur_ms < ms) {
			cnt++;
			if (cnt == 0)
				cnt++;
			memset(data, cnt, sizeof(data));
			adb_forward_send(port, data, sizeof(data));
			hal_msleep(ms_per_send);
			cur_ms += ms_per_send;
		}
		adb_forward_end(port);
		hal_msleep(1500);
		cur_cycle++;
		cur_ms = 0;
		printf("%u\n", cur_cycle);
	}
	ret = 0;
exit:
	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_adb_forward_test, adb_forward_test, adb forward test);
#endif
