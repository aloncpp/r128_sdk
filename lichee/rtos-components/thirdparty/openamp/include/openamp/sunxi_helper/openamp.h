#ifndef __OPENAMP_H_
#define __OPENAMP_H_

#include <openamp/rpmsg.h>
#include <openamp/sunxi_helper/openamp_log.h>
#include <openamp/sunxi_helper/openamp_platform.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RPMSG_MAX_LEN			(512 - 16)

/**
 *	openamp_init
 *	this function will init openamp framework
 *	@return 0 success
 * */
int openamp_init(void);
int openamp_init_async(void);
void openamp_deinit(void);

/**
 *	openamp_ept_open
 *	this function will creat a endpoint used for transmit
 *	@name:	endpoint name
 *	@rpmsg_id:	rpmsg device id.define in rssource table
 *	@src_addr:	source addr
 *	@dst_addr:	target addr
 *	@priv:		ept private data
 *	@cd:		it will be called when endpoint recive data
 *	@unbind_cb:	it will be called when unbind
 *
 *	@return endpoint pointer
 * */
struct rpmsg_endpoint *openamp_ept_open(const char *name, int rpmsg_id,
				uint32_t src_addr, uint32_t dst_addr,void *priv,
				rpmsg_ept_cb cb, rpmsg_ns_unbind_cb unbind_cb);

void openamp_ept_close(struct rpmsg_endpoint *ept);

int openamp_rpmsg_send(struct rpmsg_endpoint *ept, void *data, uint32_t len);

#ifdef CONFIG_RPMSG_NOTIFY
int rpmsg_notify_init(void);
int rpmsg_notify(char *name, void *data, int len);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __OPENAMP_H_ */
