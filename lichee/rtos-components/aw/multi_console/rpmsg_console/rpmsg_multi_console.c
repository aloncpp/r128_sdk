#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <hal_queue.h>

#include "rpmsg_console.h"

#define RPMSG_BIND_NAME				"console"
#define RPMSG_CONSOLE_MAX			100

#define log					printf

static hal_mutex_t g_list_lock;
static LIST_HEAD(g_list);

static void rpmsg_create_console(void *arg)
{
	struct rpmsg_service *ser;
	struct rpmsg_ept_client *client = arg;

	ser = hal_malloc(sizeof(*ser));
	if (!ser) {
		hal_log_err("failed to alloc client entry\r\n");
		return;
	}
	memset(ser, 0, sizeof(*ser));

	client->priv = ser;

	log("create rpmsg%" PRId32 " console.\r\n", client->id);
	ser->shell = rpmsg_console_create(client->ept, client->id);
	ser->client = client;
	log("create rpmsg%" PRId32 " success.\r\n", client->id);

	hal_mutex_lock(g_list_lock);
	list_add(&ser->list, &g_list);
	hal_mutex_unlock(g_list_lock);

	hal_thread_stop(NULL);
}

static int rpmsg_bind_cb(struct rpmsg_ept_client *client)
{
	void *thread;

	log("rpmsg%" PRId32 ": binding\r\n", client->id);

	thread = hal_thread_create(rpmsg_create_console, client,
					"init", 8 * 1024, 5);
	if (thread != NULL) {
		hal_thread_start(thread);
		return 0;
	} else
		return -EFAULT;

	return 0;
}

static int rpmsg_unbind_cb(struct rpmsg_ept_client *client)
{
	struct rpmsg_service *ser = client->priv;

	log("rpmsg%" PRId32 ": unbinding\r\n", client->id);

	if (!ser)
		return -ENODEV;

	hal_mutex_lock(g_list_lock);
	list_del(&ser->list);
	hal_mutex_unlock(g_list_lock);

	if (!ser->shell)
		return -ENODEV;

	rpmsg_console_delete(ser->shell);

	hal_free(ser);

	return 0;
}

static int rpmsg_tmp_ept_cb(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	return 0;
}

int rpmsg_multi_console_init(void)
{
	g_list_lock = hal_mutex_create();
	if (!g_list_lock) {
		hal_log_err("failed to alloc mutex\r\n");
		return -ENOMEM;
	}

	rpmsg_client_bind(RPMSG_BIND_NAME, rpmsg_tmp_ept_cb, rpmsg_bind_cb,
					rpmsg_unbind_cb, RPMSG_CONSOLE_MAX, NULL);

	return 0;
}

static void rpmsg_multi_console_init_thread(void *arg)
{
	rpmsg_multi_console_init();
	hal_thread_stop(NULL);
}

int rpmsg_multi_console_init_async(void)
{
	void *thread;

	thread = hal_thread_create(rpmsg_multi_console_init_thread, NULL,
					"init", 8 * 1024, 5);
	if (thread != NULL)
		hal_thread_start(thread);

	return 0;
}

void rpmsg_multi_console_close(void)
{
	hal_mutex_delete(g_list_lock);
	rpmsg_client_unbind(RPMSG_BIND_NAME);
}

int rpmsg_multi_console_register(void)
{
	return rpmsg_multi_console_init_async();
}
void rpmsg_multi_console_unregister(void)
{
	rpmsg_multi_console_close();
}
