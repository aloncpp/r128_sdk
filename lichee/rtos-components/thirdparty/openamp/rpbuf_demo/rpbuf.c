#include <errno.h>
#include <openamp/sunxi_helper/openamp.h>
#include <openamp/sunxi_helper/openamp_platform.h>
#include <rpbuf.h>
#include <hal_thread.h>
#include <hal_mutex.h>

static uint8_t is_init = 0;
static hal_mutex_t lock = NULL;

int rpbuf_init(void)
{
	int ret = 0;
	struct rpmsg_device *rpmsg_dev;
	struct rpbuf_controller *controller;
	struct rpbuf_service *service;

	if (lock == NULL)
		lock = hal_mutex_create();

	hal_mutex_lock(lock);

	if (is_init) {
		hal_mutex_unlock(lock);
		return 0;
	}

	if (openamp_init()) {
		printf("(%s:%d) Failed to init openamp framework\n", __func__, __LINE__);
		ret = -EBUSY;
		goto err_out;
	}

	rpmsg_dev = openamp_sunxi_get_rpmsg_vdev(0);
	if (!rpmsg_dev) {
		printf("(%s:%d) Failed to get rpmsg device 0\n", __func__, __LINE__);
		ret = -ENOENT;
		goto err_out;
	}

	controller = rpbuf_init_controller(0, (void *)rpmsg_dev, RPBUF_ROLE_SLAVE, NULL, NULL);
	if (!controller) {
		printf("(%s:%d) rpbuf_init_controller failed\n", __func__, __LINE__);
		goto err_out;
	}

	service = rpbuf_init_service(0, (void *)rpmsg_dev);
	if (!service) {
		printf("(%s:%d) rpbuf_init_service failed\n", __func__, __LINE__);
		goto err_deinit_controller;
	}

	is_init = 1;

	hal_mutex_unlock(lock);

	return 0;

err_deinit_controller:
	rpbuf_deinit_controller(controller);
err_out:
	hal_mutex_unlock(lock);
	return ret;
}

static void rpbuf_init_thread(void *arg)
{
	rpbuf_init();
	hal_thread_stop(NULL);
}

int rpbuf_init_async(void)
{
	void *thread;

	thread = hal_thread_create(rpbuf_init_thread, NULL,
					"rpbuf_init", 4 * 1024, 8);
	if (thread)
		hal_thread_start(thread);

	return 0;
}

void rpbuf_deinit(void)
{
	struct rpbuf_controller *controller = rpbuf_get_controller_by_id(0);
	struct rpbuf_service *service = rpbuf_get_service_by_id(0);

	rpbuf_deinit_controller(controller);
	rpbuf_deinit_service(service);
	is_init = 0;
	hal_mutex_delete(lock);
	lock = NULL;
}
