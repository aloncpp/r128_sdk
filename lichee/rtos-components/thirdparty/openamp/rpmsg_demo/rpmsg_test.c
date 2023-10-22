#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <hal_osal.h>
#include <hal_sem.h>
#include <hal_cache.h>
#include <hal_msgbox.h>
#include <openamp/sunxi_helper/openamp.h>
#include <openamp/sunxi_helper/msgbox_ipi.h>

#define RPMSG_SERVICE_NAME "sunxi,rpmsg_test"
#define MSG "Hello,Linux"

struct rpmsg_test {
	const char *name;
	uint32_t src;
	uint32_t dst;
	struct rpmsg_endpoint *ept;
	int unbound;
};

struct rpmsg_test *chip= NULL;

static int rpmsg_ept_callback(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	printf("%s recv %s\n", ept->name, (char *)data);

	return 0;
}

static void rpmsg_unbind_callback(struct rpmsg_endpoint *ept)
{
	struct rpmsg_test *chip = ept->priv;

	chip->unbound = 1;

	printf("%s is destroyed\n", ept->name);
}

static void cmd_rpmsg_test_thread(void *arg)
{
	chip = hal_malloc(sizeof(*chip));
	if (!chip) {
		printf("Failed to malloc memory\r\n");
		goto out;
	}

	if (openamp_init()) {
		printf("Failed to init openamp framework\r\n");
		goto out;
	}

	chip->name = RPMSG_SERVICE_NAME;
	chip->unbound = 0;
	chip->src = RPMSG_ADDR_ANY;
	chip->dst = RPMSG_ADDR_ANY;

	chip->ept = openamp_ept_open(chip->name, 0, chip->src, chip->dst,
					chip, rpmsg_ept_callback, rpmsg_unbind_callback);
	if (!chip->ept) {
		printf("Failed to Create Endpoint\r\n");
		goto out;
	}

out:
	hal_thread_stop(NULL);
}

static int rpmsg_test_init(void)
{
	void *thread;

	thread = hal_thread_create(cmd_rpmsg_test_thread, NULL, "rpmsg_test", 2048, 8);
	if(!thread)
		hal_thread_start(thread);
	else
		printf("fail to create rpmsg hearbeat thread\r\n");

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(rpmsg_test_init, rpmsg_test_init, init rpmsg test);

static int rpmsg_test_send(void)
{
	openamp_rpmsg_send(chip->ept, MSG, strlen(MSG));

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(rpmsg_test_send, rpmsg_test_send, rpmsg test send);
