#include <openamp/virtual_driver/virt_uart.h>


#define RPMSG_SERVICE_NAME              "rpmsg-tty-channel"

static int virt_uart_read_cb(struct rpmsg_endpoint *ept, void *data,
			    size_t len, uint32_t src, void *priv)
{
	int i = 0;
	printf("[%s:0x%x->0x%x] Received data (len: %zu):\n",
					ept->name, ept->addr, ept->dest_addr, len);
	for (i = 0; i < len; ++i) {
			printf(" 0x%x,", *((uint8_t *)(data) + i));
	}
	printf("\n");

	return RPMSG_SUCCESS;
}

int virt_uart_init(void)
{
	int status;
	/* Create a endpoint for rmpsg communication */
	status = openamp_platform_create_rpmsg_ept(RPMSG_VDEV0,
			RPMSG_SERVICE_NAME, RPMSG_ADDR_ANY,RPMSG_ADDR_ANY,
			virt_uart_read_cb, NULL);

	return status;
}

