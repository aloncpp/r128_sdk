#ifndef _RPMSG_CONSOLE_H_
#define _RPMSG_CONSOLE_H_

#include <hal_queue.h>
#include <hal_thread.h>
#include <openamp/sunxi_helper/openamp.h>
#include <openamp/sunxi_helper/rpmsg_master.h>

#include "../multi_console_internal.h"

struct rpmsg_message {
	unsigned int command;
	unsigned int data_length;
};

struct rpmsg_packet {
	struct rpmsg_message msg;
	char data[512 - 16 - sizeof(struct rpmsg_message)];
};

struct rpmsg_shell {
	struct rpmsg_endpoint *ept;
	cli_dev_ops raw;
	hal_mailbox_t mb_rx;
	cli_console *console;
	char name[32];
#ifdef CONFIG_RPMSG_CONSOLE_CACHE
	uint32_t cache_len;
	char cmd_cache[CLI_CONSOLE_MAX_INPUT_SIZE];
#endif
};

struct rpmsg_service {
	struct rpmsg_shell *shell;
	struct rpmsg_ept_client *client;
	struct list_head list;
};

struct rpmsg_shell *
rpmsg_console_create(struct rpmsg_endpoint *ept, uint32_t id);
void rpmsg_console_delete(struct rpmsg_shell *shell);

#define rpmsg_err(fmt, args...) \
    printf("[RPMSG_ERR][%s:%d]" fmt, __FUNCTION__, __LINE__, ##args)

#ifdef RPMSG_DEBUG
#define rpmsg_debug(fmt, args...) \
    printf("[RPMSG_DBG][%s:%d]" fmt, __FUNCTION__, __LINE__, ##args)
#else
#define rpmsg_debug(fmt, args...)
#endif

#endif
