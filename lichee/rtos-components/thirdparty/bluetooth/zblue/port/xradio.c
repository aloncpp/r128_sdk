#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <hal_gpio.h>
#include <hal_uart.h>
#include "xradio.h"
#include "platform_bsp.h"

#include "bluetooth/bluetooth.h"
#include "drivers/bluetooth/hci_driver.h"

static const struct xr_bluetooth_rf *xr_rf = NULL;
static const struct xr_bluetooth_uart *xr_uart = NULL;

static int g_fd = -1;
static struct k_thread rx_thread_data;
static K_THREAD_STACK_DEFINE(rx_thread_stack, CONFIG_BT_RX_STACK_SIZE);

static unsigned short check_sum16(void *data, unsigned int len)
{
	unsigned short *p = data;
	unsigned short cs = 0;

	while (len > 1) {
		cs += *p++;
		len -= 2;
	}

	if (len)
		cs += *(unsigned char *)p;

	return cs;
}

static int data_send(uint8_t *buf, size_t count)
{
	return xr_uart->send_data(buf, count);
}

static int data_recv(uint8_t *buf, size_t count)
{
	return xr_uart->receive_data(buf, count);
}

static int data_recv_nowait(uint8_t *buf, size_t count)
{
	return xr_uart->receive_data_no_block(buf, count, 0);
}

static int data_recv_timeout(uint8_t *buf, size_t count, int timeout)
{
	return xr_uart->receive_data_no_block(buf, count, timeout);
}

static int brom_cmd_send_syncword(void)
{
	uint8_t syncword[3] = { CMD_SYNC_WORD };
	int ret;
	int sync_count = 50;

	data_recv_nowait(syncword, 2);

	do {
		syncword[0] = CMD_SYNC_WORD;
		ret = data_send(syncword, 1);
		if (ret < 1)
			return -1;
		else if (ret == 0)
			continue;

		usleep(10 * 1000);

		ret = data_recv_nowait(syncword, 2);
		if (ret == 2 && ((syncword[0] == 'O') && (syncword[1] == 'K')))
			break;
	} while (sync_count--);

	return ret == 2 ? 0 : -1;
}

static int brom_cmd_recv_ack(void)
{
	cmd_ack_t ack = {};
	int ret;

	ret = data_recv_timeout((uint8_t *)&ack, sizeof(ack) - 1, XRADIO_RECV_TIMEOUT);
	if (ret != sizeof(ack) - 1)
		return -1;

	/* check header */
	if (!HEADER_MAGIC_VALID(&ack.h))
		return -1;

	if (ack.h.flags & CMD_HFLAG_ERROR)
		return -ack.err;

	if ((ack.h.flags & CMD_HFLAG_ACK) == 0)
		return -1;

	/* convert network byte order to host byte order */
	ack.h.payload_len = SWAP32(ack.h.payload_len);
	ack.h.checksum    = SWAP16(ack.h.checksum);
	if (ack.h.payload_len != 0)
		return -1;

	if ((ack.h.flags & CMD_HFLAG_CHECK) &&
			(check_sum16(&ack, MB_CMD_HEADER_SIZE)) != 0xffff)
		return -1;

	return 0;
}

static int brom_cmd_recv_ack_timeout(void)
{
	cmd_ack_t ack = {};
	int ret;

	ret = data_recv_timeout((uint8_t *)&ack, sizeof(ack) - 1, XRADIO_RECV_TIMEOUT);
	if (ret != sizeof(ack) - 1)
		return -1;

	/* check header */
	if (!HEADER_MAGIC_VALID(&ack.h))
		return -1;

	if (ack.h.flags & CMD_HFLAG_ERROR)
		return -ack.err;

	if ((ack.h.flags & CMD_HFLAG_ACK) == 0)
		return -1;

	/* convert network byte order to host byte order */
	ack.h.payload_len = SWAP32(ack.h.payload_len);
	ack.h.checksum    = SWAP16(ack.h.checksum);
	if (ack.h.payload_len != 0)
		return -1;

	if ((ack.h.flags & CMD_HFLAG_CHECK) &&
			(check_sum16(&ack, MB_CMD_HEADER_SIZE)) != 0xffff)
		return -1;

	return 0;
}

static int brom_cmd_send_baudrate(int speed)
{
	int payload_len = sizeof(cmd_sys_setuart_t) - sizeof(cmd_header_t);
	int lcr = speed | (3 << 24);
	cmd_sys_setuart_t cmd = {};
	int ret;

	/* fill header */
	FILL_HEADER_MAGIC(&cmd.h);
	cmd.h.flags = CMD_HFLAG_CHECK;
	cmd.h.payload_len = payload_len;

	/* fill command id */
	cmd.cmdid = CMD_ID_SETUART;
	cmd.lcr = lcr;
	cmd.h.checksum = ~check_sum16(&cmd, MB_CMD_HEADER_SIZE + payload_len);

	/* convert host byte order to network byte order */
	cmd.h.payload_len = SWAP32(cmd.h.payload_len);
	cmd.h.checksum    = SWAP16(cmd.h.checksum);
	cmd.lcr           = SWAP32(cmd.lcr);

	ret = data_send((uint8_t *)&cmd, sizeof(cmd));
	if (ret != sizeof(cmd))
		return -EBADF;

	ret = brom_cmd_recv_ack_timeout();
	if (ret < 0)
		return ret;

	xr_uart->set_baudrate(speed);

	return brom_cmd_send_syncword();
}

static int brom_cmd_send_stream(void *addr, int len, void *data)
{
	int payload_len = sizeof(cmd_seq_wr_t) - sizeof(cmd_header_t);
	cmd_seq_wr_t cmd = {};
	int ret;

	/* fill header */
	FILL_HEADER_MAGIC(&cmd.h);
	cmd.h.flags = CMD_HFLAG_CHECK;
	cmd.h.payload_len = payload_len;

	/* fill command id */
	cmd.cmdid = CMD_ID_SEQWR;
	cmd.addr = (intptr_t)addr;
	cmd.dlen = len;
	cmd.dcs = ~check_sum16(data, len);
	cmd.h.checksum = ~check_sum16(&cmd, MB_CMD_HEADER_SIZE + payload_len);

	/* convert host byte order to network byte order */
	cmd.h.payload_len = SWAP32(cmd.h.payload_len);
	cmd.addr          = SWAP32(cmd.addr);
	cmd.dlen          = SWAP32(cmd.dlen);
	cmd.dcs           = SWAP16(cmd.dcs);
	cmd.h.checksum    = SWAP16(cmd.h.checksum);

	ret = data_send((uint8_t *)&cmd, sizeof(cmd));
	if (ret != sizeof(cmd))
		return -EBADF;

	ret = brom_cmd_recv_ack();
	if (ret < 0)
		return ret;

	ret = data_send(data, len);
	if (ret != len)
		return -EBADF;

	return brom_cmd_recv_ack();
}

static int brom_set_pc(unsigned int pc)
{
	int payload_len = sizeof(cmd_sys_t) - sizeof(cmd_header_t);
	cmd_sys_t cmd = {};
	int ret;

	/* fill header */
	FILL_HEADER_MAGIC(&cmd.h);
	cmd.h.flags = CMD_HFLAG_CHECK;
	cmd.h.payload_len = payload_len;

	/* fill command id */
	cmd.cmdid = CMD_ID_SETPC;
	cmd.val = pc;
	cmd.h.checksum = ~check_sum16(&cmd, MB_CMD_HEADER_SIZE + payload_len);

	/* convert host byte order to network byte order */
	cmd.h.payload_len = SWAP32(cmd.h.payload_len);
	cmd.h.checksum    = SWAP16(cmd.h.checksum);
	cmd.val           = SWAP32(cmd.val);

	ret = data_send((uint8_t *)&cmd, sizeof(cmd));
	if (ret != sizeof(cmd))
		return -EBADF;

	return brom_cmd_recv_ack();
}

static int brom_load_firmware(void)
{
	void *addr = (intptr_t)BT_LOAD_ADDR;
	size_t remain, req;
	struct stat buf;
	int fd = -1, ret = -1;
	void *data = NULL;

#ifdef CONFIG_BT_XR829
	fd = open(XR829_FIRMWARE, O_RDONLY);
#elif CONFIG_BT_XR819S
	fd = open(XR819S_FIRMWARE, O_RDONLY);
#endif

	if (fd < 0)
		return fd;

	fstat(fd, &buf);

	remain = buf.st_size;

	if (remain <= 0)
		goto bail;

	data = malloc(SZ_1K);
	if (data == NULL) {
		ret = -ENOMEM;
		goto bail;
	}

	while (remain > 0) {
		req = remain > SZ_1K ? SZ_1K : remain;
		ret = read(fd, data, req);
		if (ret != req) {
			ret = -EIO;
			goto bail;
		}

		ret = brom_cmd_send_stream(addr, req, data);
		if (ret < 0)
			goto bail;

		addr += req;
		remain -= req;
	}

	brom_set_pc((intptr_t)BT_LOAD_ADDR);

bail:
	free(data);
	close(fd);

	return ret;
}

static int brom_hci_final_reset(void)
{
	uint8_t hci_reset[4] = "\x01\x03\x0c\x00";
	uint8_t hci_update_baud_rate[8] = "\x01\x18\xfc\x04\x60\xE3\x16\x00";
	uint8_t hci_write_bd_addr[13] = "\x01\x0a\xfc\x09\x02\x00\x06\xAE\xC9\x69\x75\x22\x22";
	uint8_t ack[7];
	int ret;

	ret = data_send(hci_reset, sizeof(hci_reset));
	if (ret < 0)
		return ret;

	ret = data_recv(ack, sizeof(ack));
	if (ret < 0)
		return ret;

#ifdef CONFIG_BT_XR829
	ret = data_send(hci_update_baud_rate, sizeof(hci_update_baud_rate));
	if (ret < 0)
		return ret;

	ret = data_recv(ack, sizeof(ack));
	if (ret < 0)
		return ret;

	xr_uart->set_baudrate(XRADIO_ONCHIP_BAUDRATE);
	usleep(50 * 1000);
#endif

	ret = data_send(hci_write_bd_addr, sizeof(hci_write_bd_addr));
	if (ret < 0)
		return ret;

	ret = data_recv(ack, sizeof(ack));
	if (ret < 0)
		return ret;

	return 0;
}

static volatile int xradio_deinit = 0;
static void rx_thread(void *p1, void *p2, void *p3)
{
	int hdr_len, data_len, ret;
	struct net_buf *buf;
	uint8_t type;
	union {
		struct bt_hci_evt_hdr evt;
		struct bt_hci_acl_hdr acl;
	} hdr;

	for (;;) {
		if (xradio_deinit == 1) {
			break;
		}
		ret = data_recv(&type, 1);
		if (ret != 1)
			break;

		if (type != H4_EVT && type != H4_ACL)
			continue;

		hdr_len = (type == H4_EVT) ?
			sizeof(struct bt_hci_evt_hdr) :
			sizeof(struct bt_hci_acl_hdr);

		ret = data_recv((uint8_t *)&hdr, hdr_len);
		if (ret != hdr_len)
			break;

		if (type == H4_EVT) {
			buf = bt_buf_get_evt(hdr.evt.evt, false, K_FOREVER);
			data_len = hdr.evt.len;
		} else {
			buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
			data_len = hdr.acl.len;
		}

		if (buf == NULL)
			break;

		if (data_len > buf->size) {
			net_buf_unref(buf);
			continue;
		}

		memcpy(buf->data, &hdr, hdr_len);

		bt_buf_set_type(buf, type == H4_EVT ? BT_BUF_EVT : BT_BUF_ACL_IN);

		ret = data_recv(buf->data + hdr_len, data_len);
		if (ret != data_len)
			break;

		if(hdr.evt.evt == BT_HCI_EVT_CMD_COMPLETE) {
			HOSTMINI_LOG("[HCI EVENT COMPT] [Opcode]:%02x%02x\n",
					buf->data[hdr_len+2], buf->data[hdr_len+1]);
		} else if(hdr.evt.evt == BT_HCI_EVT_CMD_COMPLETE) {
			HOSTMINI_LOG("[HCI EVENT STATUS] [Opcode]:%02x%02x\n",
					buf->data[hdr_len+3], buf->data[hdr_len+2]);
		}

		net_buf_add(buf, hdr_len + data_len);
#ifdef CONFIG_BTSNOOP
		void btsnoop_capture(uint8_t type, const uint8_t *buf, bool is_received);
		btsnoop_capture(type, buf->data, 1);
#endif
		bt_recv(buf);
	}

	printf("INVALID BUF FORMAT\n");
}

static int h4_open(void)
{
	int ret;

	xr_rf = xradio_get_platform_rf();
	xr_uart = xradio_get_platform_uart();

	if (g_fd > 0)
		return OK;

	ret = xr_uart->init();
	if (ret != 0)
		return ret;

	xr_uart->disable_flowcontrol();
	xr_rf->init();

rfreset:

	xr_rf->reset();

	printf("brom cmd send sync word.\n");
	ret = brom_cmd_send_syncword();

	if (ret < 0 && XRADIO_RF_RESET_CEASELESS) {
		printf("sync timeout reset rf.\n");
		goto rfreset;
	} else if(ret < 0){
		printf("sync timeout faild.\n");
		goto fail;
	}

	printf("brom change baudrate.\n");
	ret = brom_cmd_send_baudrate(XRADIO_ONCHIP_BAUDRATE);

	if (ret < 0)
		goto fail;

	printf("Download firmware......\n");
	ret = brom_load_firmware();

	if (ret < 0)
		goto fail;

	printf("Download firmware success.\n");
	xr_uart->set_baudrate(XRADIO_DEFAULT_BAUDRATE);
	xr_uart->enable_flowcontrol();
	printf("hci send reset cmd......\n");
	brom_hci_final_reset();
	printf("hci send reset cmd success.\n");
	k_thread_create(&rx_thread_data,NULL,"zbt_h4_rx",
			CONFIG_BT_RX_STACK_SIZE,
			rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_RX_PRIO), 0, K_NO_WAIT);

	return 0;

fail:
	return ret;
}

static int h4_send(struct net_buf *buf)
{
	uint8_t *type;
	int ret;

	type = net_buf_push(buf, 1);

	switch (bt_buf_get_type(buf)) {
		case BT_BUF_ACL_OUT:
			*type = H4_ACL;
			break;
		case BT_BUF_CMD:
			HOSTMINI_LOG("[HCI CMD][Opcode]:%02x%02x\n", buf->data[2], buf->data[1]);
			*type = H4_CMD;
			break;
		default:
			ret = -EINVAL;
			goto bail;
	}

#ifdef CONFIG_BTSNOOP
	if(*type == H4_ACL || *type == H4_CMD) {
		void btsnoop_capture(uint8_t type, const uint8_t *buf, bool is_received);
		btsnoop_capture(*type, buf->data+1, 0);
	}
#endif
	ret = data_send(buf->data, buf->len);
	if (ret != buf->len)
		ret = -EINVAL;

bail:
	net_buf_unref(buf);

	return ret < 0 ? ret : 0;
}

static struct bt_hci_driver driver = {
	.name = "H:4",
	.bus  = BT_HCI_DRIVER_BUS_UART,
	.open = h4_open,
	.send = h4_send,
};

int xradio_bt_driver_register(void)
{
	xradio_deinit = 0;

	return bt_hci_driver_register(&driver);
}

#if defined(CONFIG_BT_DEINIT)
void xradio_bt_driver_unregister(void)
{
	uint8_t hci_zero[40] = {0};

	xradio_deinit = 1;
	memset(hci_zero, 0, 40);

	xr_rf->reset();
	usleep(10 * 1000);
	xr_uart->set_loopback(true);

	data_send(hci_zero, sizeof(hci_zero));
	data_recv_timeout(hci_zero,40,2);
	data_recv_timeout(hci_zero,40,2);
	xr_uart->set_loopback(false);

	usleep(10 * 1000);
	xr_uart->deinit();
}

int bt_is_deinit(void)
{
	return (xradio_deinit==1)?(1):(0);
}
#endif

#ifdef CONFIG_AW_BTETF_TOOL

static struct k_thread        etf_rx_thread_data;
typedef int (*ETF_C2H_CB)(unsigned char hci_type,
					const unsigned char * buff,
					unsigned int offset,
					unsigned int len);
static ETF_C2H_CB etf_rx_callback;

static void etf_rx_thread(void *p1, void *p2, void *p3)
{
	int hdr_len, data_len, ret;
	uint8_t type;
	union {
		struct bt_hci_evt_hdr evt;
		struct bt_hci_acl_hdr acl;
	} hdr;

	char buf1[1024];

	for (;;) {
		ret = data_recv(&type, 1);
		if (ret != 1)
			break;

		if (type != H4_EVT && type != H4_ACL)
			continue;

		hdr_len = (type == H4_EVT) ?
			sizeof(struct bt_hci_evt_hdr) :
			sizeof(struct bt_hci_acl_hdr);

		ret = data_recv((uint8_t *)&hdr, hdr_len);
		if (ret != hdr_len)
			break;

		if (type == H4_EVT) {
			data_len = hdr.evt.len;
		} else {
			printf("error\n");
		}

		if (data_len > 1024) {
			printf("error\n");
			continue;
		}
		memcpy(buf1, (uint8_t *)&hdr, hdr_len);
		ret = data_recv(buf1 + hdr_len, data_len);
		if (ret != data_len)
			break;

		if(hdr.evt.evt == BT_HCI_EVT_CMD_COMPLETE) {
			HOSTMINI_LOG("[HCI EVENT COMPT] [Opcode]:%02x%02x\n",
					buf1[hdr_len+2], buf1[hdr_len+1]);
		} else if(hdr.evt.evt == BT_HCI_EVT_CMD_COMPLETE) {
			HOSTMINI_LOG("[HCI EVENT STATUS] [Opcode]:%02x%02x\n",
					buf1[hdr_len+3], buf1[hdr_len+2]);
		}

		printf("[%s] type = %d\nrecv: ",__FUNCTION__,type);
		for(uint16_t i = 0; i < hdr_len + data_len; i++) {
			printf("%02X ", *(buf1+i));
			if ((i%16 == 0) && (i!= 0))
				printf("\n");
		}
		if (etf_rx_callback != NULL) {
			etf_rx_callback(type, buf1, 0, hdr_len + data_len);
		} else {
			printf("ETF CALLBACK UNREGISTER!\n");
		}

#ifdef CONFIG_BTSNOOP
		void btsnoop_capture(uint8_t type, const uint8_t *buf, bool is_received);
		btsnoop_capture(type, buf1, 1);
#endif
	}

	printf("INVALID BUF FORMAT\n");
}

static int etf_h4_open(ETF_C2H_CB etf_cb)
{
	int ret;

	if (g_fd > 0)
		return OK;

	xr_rf = xradio_get_platform_rf();
	xr_uart = xradio_get_platform_uart();

	ret = xr_uart->init();
	if (ret != 0)
		return ret;

	xr_uart->disable_flowcontrol();
	xr_rf->init();
	xr_rf->reset();

	printf("brom cmd send sync word.\n");

	ret = brom_cmd_send_syncword();
	if (ret < 0)
		goto bail;

	printf("brom change baudrate.\n");
	ret = brom_cmd_send_baudrate(XRADIO_ONCHIP_BAUDRATE);
	if (ret < 0)
		goto bail;

	printf("Download firmware......\n");
	ret = brom_load_firmware();
	if (ret < 0)
		goto bail;
	printf("Download firmware success.\n");

	xr_uart->set_baudrate(XRADIO_DEFAULT_BAUDRATE);
	xr_uart->enable_flowcontrol();

	printf("hci send reset cmd......\n");
	brom_hci_final_reset();
	printf("hci send reset cmd success.\n");
	if (etf_cb != NULL) {
		etf_rx_callback = etf_cb;
	} else {
		printf("error have no callback\n");
	}
	k_thread_create(&etf_rx_thread_data, NULL, "zbt_etf_h4_rx",
			CONFIG_BT_RX_STACK_SIZE,
			etf_rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_RX_PRIO), 0, K_NO_WAIT);
	return 0;

bail:
	return ret;
}

int etf_h4_send(unsigned char hci_type, const unsigned char *buff, unsigned int offset, unsigned int len)
{
	int ret;

#ifdef CONFIG_BTSNOOP
	if(buff[0] == H4_ACL || buff[0] == H4_CMD) {
		void btsnoop_capture(uint8_t type, const uint8_t *buf, bool is_received);
		btsnoop_capture(buff[0], buff + 1, 0);
	}
#endif
	printf("send: ");
	for(uint16_t i = 0; i < len; i++) {
		printf("%02X ", *(buff+i));
		if ((i%16 == 0) && (i!= 0))
			printf("\n");
	}
	ret = data_send(buff, len);
	if (ret != len)
		ret = -EINVAL;

bail:
	return ret < 0 ? ret : 0;
}

int xr829_bt_etf_driver_register(ETF_C2H_CB etf_cb)
{
	return etf_h4_open(etf_cb);
}
#endif
