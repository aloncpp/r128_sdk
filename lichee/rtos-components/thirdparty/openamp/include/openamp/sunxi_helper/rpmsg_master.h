#ifndef _RPMSG_MASTER_H_
#define _RPMSG_MASTER_H_

#include <stdint.h>
#include <stdio.h>
#include <openamp/sunxi_helper/openamp.h>

#define RPMSG_MAX_NAME_LEN		32

struct rpmsg_ept_client {
	uint32_t id;		/* unique id for every client */
	char name[RPMSG_MAX_NAME_LEN];
	struct rpmsg_endpoint *ept; /* ept->priv can used by user */
	void *priv;			/* user used */
};
typedef int (*rpmsg_func_cb)(struct rpmsg_ept_client *client);

int rpmsg_ctrldev_create(void);
void rpmsg_ctrldev_init_thread(void *arg);
void rpmsg_ctrldev_release(void);
int rpmsg_client_bind(const char *name, rpmsg_ept_cb cb, rpmsg_func_cb bind,
				rpmsg_func_cb unbind, uint32_t cnt, void *priv);
void rpmsg_client_unbind(const char *name);
void rpmsg_eptldev_close(struct rpmsg_ept_client *client);

#endif
