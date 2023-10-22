#include <stdlib.h>
#include <stdint.h>
#include <hal_queue.h>
#include <hal_uart.h>
#include <inttypes.h>
#include "rpmsg_console.h"

#ifdef CONFIG_MULTI_CONSOLE_DEBUG
static const char *cmd_str[] = {
	"open", "close", "write",
};
#endif

static int rpmsg_ept_callback(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	int i;
	struct rpmsg_shell *shell = ept->priv;
	struct rpmsg_packet *pack = data;

	if (len < sizeof(struct rpmsg_message)) {
		console_debug("%s Received(len:%d)\n", ept->name, len);
		return 0;
	}

	if (!pack->msg.data_length) {
		console_debug("empty message\r\n");
		return 0;
	} else
		console_debug("%s: %s:data=%s\r\n", shell->name, cmd_str[pack->msg.command], pack->data);

	for (i = 0; i < pack->msg.data_length; i++)
		hal_mailbox_send(shell->mb_rx, pack->data[i]);

	return 0;
}

static int rpmsg_console_write_no_cache(const void *buf, size_t len, void *priv)
{
	int i;
	const uint8_t *data = buf;
	struct rpmsg_shell *shell = priv;

	if (len > RPMSG_MAX_LEN) {
		size_t remain = len;
		for (i = 0; i < len;) {
			if (remain > RPMSG_MAX_LEN) {
				openamp_rpmsg_send(shell->ept, (void *)&data[i], RPMSG_MAX_LEN);
				i += RPMSG_MAX_LEN;
				remain -= RPMSG_MAX_LEN;
			} else {
				openamp_rpmsg_send(shell->ept, (void *)&data[i], remain);
				i += remain;
				remain = 0;
			}
		}
	} else
		openamp_rpmsg_send(shell->ept, (void *)buf, len);

	return len;
}
static int rpmsg_console_write(const void *buf, size_t len, void *priv)
{
	struct rpmsg_shell *shell = priv;

#ifdef CONFIG_RPMSG_CONSOLE_CACHE
	size_t i = 0;
	const uint8_t *data = buf;

	if ((shell->cache_len + len) >= CLI_CONSOLE_MAX_INPUT_SIZE)
		goto _clean;

	while (i < len) {
		if (data[i++] == '\n')
			goto _clean;
	}
	memcpy(shell->cmd_cache + shell->cache_len, buf, len);
	shell->cache_len += len;

	return len;
_clean:
	rpmsg_console_write_no_cache(shell->cmd_cache, shell->cache_len, priv);
	shell->cache_len = 0;
	return rpmsg_console_write_no_cache(buf, len, priv);
#endif

	return rpmsg_console_write_no_cache(buf, len, priv);
}

static int rpmsg_console_read(void *buf, size_t len, void *priv, uint32_t timeout)
{
	int i, ret;
	unsigned int value;
	char *data = buf;
	struct rpmsg_shell *shell = priv;

	i = 0;
	while (len > i) {
		ret = hal_mailbox_recv(shell->mb_rx, &value, timeout);
		if (!ret)
			data[i++] = (char)value;
		else
			break;
	}
	return i;
}

static int rpmsg_console_init(void *priv)
{
	console_debug("rpmsg_console init\r\n");

	return 0;
}

static int rpmsg_console_deinit(void *priv)
{
	struct rpmsg_shell *shell = priv;

	console_debug("rpmsg_console close\r\n");

	hal_mailbox_delete(shell->mb_rx);
	hal_free(shell);

	return 0;
}

struct rpmsg_shell *
rpmsg_console_create(struct rpmsg_endpoint *ept, uint32_t id)
{
	int ret;
	struct rpmsg_shell *shell = NULL;

	shell = hal_malloc(sizeof(*shell));
	if (!shell)
		return NULL;

	shell->mb_rx = hal_mailbox_create("console-mb-rx", 128);
	if (!shell->mb_rx) {
		rpmsg_err("create mb_rx failed\r\n");
		return NULL;
	}
	ret = snprintf(shell->name, 32, "rpmsg%" PRId32 "-console", id);
	if (ret < 0) {
		rpmsg_err("snprintf console name failed!\r\n");
		hal_mailbox_delete(shell->mb_rx);
		return NULL;
	}

	ept->cb           = rpmsg_ept_callback;
	ept->priv         = shell;
	shell->ept        = ept;
	shell->raw.write  = rpmsg_console_write;
	shell->raw.read   = rpmsg_console_read;
	shell->raw.init   = rpmsg_console_init;
	shell->raw.deinit = rpmsg_console_deinit;
	shell->raw.priv   = shell;
	shell->raw.echo   = 0;
	shell->raw.task   = 1;
	shell->raw.prefix = NULL;
#ifdef CONFIG_RPMSG_CONSOLE_CACHE
	shell->cache_len  = 0;
#endif

	shell->console = cli_console_create(&shell->raw, shell->name);
	if (!cli_console_check_invalid(shell->console))
		return NULL;

	return shell;
}

void rpmsg_console_delete(struct rpmsg_shell *shell)
{
	cli_console_set_exit(shell->console);
}
