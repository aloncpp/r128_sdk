/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <hal_log.h>
#include <hal_cmd.h>
#include <hal_mem.h>
#include <hal_timer.h>
#include <hal_thread.h>
#include <sunxi_hal_spi.h>

#ifdef CONFIG_ARCH_SUN20IW2
#define SPI_SLAVE_THREAD_STACK_SIZE 4096
#else
#define SPI_SLAVE_THREAD_STACK_SIZE 8192
#endif

#define SLAVE_CACHE_MAX (4096)

#define PKT_HEAD_LEN 5
#define OP_MASK		0
#define ADDR_MASK_0	1
#define ADDR_MASK_1	2
#define LEN_MASK_0	3
#define LEN_MASK_1	4

#define SUNXI_OP_WRITE	0x01
#define SUNXI_OP_READ	0x03
#define SUNXI_OP_HEAD	0xff

enum sunxi_spi_slave_status {
	SUNXI_SPI_SLAVE_NONE = 0,
	SUNXI_SPI_SLAVE_RUNNING,
	SUNXI_SPI_SLAVE_RETRY,
	SUNXI_SPI_SLAVE_STOP,
};

struct sunxi_spi_slave_head {
	u8 op_code;
	u16 addr;
	u16 len;
};

struct sunxi_spi_slave_frame {
	u8 data[PKT_HEAD_LEN];
	struct sunxi_spi_slave_head pkt_head;
	u8 *tx_buf;
	u8 *rx_buf;
};

struct sunxi_spi_slave_cache {
	hal_spinlock_t buffer_lock;
	u8 *buffer;
	u32 size;
};

struct sunxi_spi_slave_test {
	hal_spi_master_port_t  port;
	hal_spi_master_config_t cfg;
	hal_sem_t semaphore_finished;
	hal_spi_master_transfer_t xfer;
	struct sunxi_spi_slave_frame frame;
	struct sunxi_spi_slave_cache cache;
	enum sunxi_spi_slave_status status;
	hal_thread_t thread_handle;
	char task_name[256];
};

static struct sunxi_spi_slave_test spi_slave_test[HAL_SPI_MASTER_MAX];

static bool sunxi_spi_dump_data(const uint8_t *buf, uint32_t offset, uint32_t len)
{
	int col = 16;
	int line = len / col;
	int last = len % col;
	int i, j;
	uint8_t *buffer = (int8_t *)buf + offset;

	for (i = 0; i < line; i++) {
		printf("%08X: ", i + offset);
		for (j = 0; j < col; j++) {
			printf("%02x ", buffer[col * i + j]);
		}
		printf("\n");
	}

	printf("%08X: ", col * line + offset);
	for (j = 0; j < last; j++) {
		printf("%02x ", buffer[col * line + j]);
	}
	printf("\n");
}

int sunxi_spi_init_slave_data(struct sunxi_spi_slave_test *slave, u8 pattern)
{
	memset(slave->cache.buffer, pattern, slave->cache.size);
	return 0;
}

static bool sunxi_spi_slave_has_ptk_head(struct sunxi_spi_slave_head *head)
{
	if (head->op_code || head->addr || head->len)
		return true;

	return false;
}

static void sunxi_spi_slave_head_data_parse(unsigned char *data, struct sunxi_spi_slave_head *head)
{
	head->op_code = data[OP_MASK];
	head->addr = (data[ADDR_MASK_0] << 8) | data[ADDR_MASK_1];
	head->len = (data[LEN_MASK_0] << 8) | data[LEN_MASK_1];
}

static void sunxi_spi_slave_head_data_clear(unsigned char *data, int len)
{
	memset(data, 0, len);
}

static int sunxi_spi_slave_set_cache_data(struct sunxi_spi_slave_test *slave,
									struct sunxi_spi_slave_head *head, u8 *buf)
{
	struct sunxi_spi_slave_cache *cache = &slave->cache;
	int real_size = head->len;

	if (cache->size < head->addr) {
		hal_log_err("Set data addr over range");
		return 0;
	}

	if (cache->size < head->addr + head->len) {
		real_size = cache->size - head->addr;
		hal_log_err("Write size %d over range, some of data will be lost, real size to write is %d",
				head->len, real_size);
	}

	hal_spin_lock(&cache->buffer_lock);
	memcpy(cache->buffer + head->addr, buf, real_size);
	hal_spin_unlock(&cache->buffer_lock);

	return 0;
}

static int sunxi_spi_slave_get_cache_data(struct sunxi_spi_slave_test *slave,
										struct sunxi_spi_slave_head *head, u8 *buf)
{
	struct sunxi_spi_slave_cache *cache = &slave->cache;
	int real_size = head->len;

	if (cache->size < head->addr) {
		hal_log_err("Get data addr over range");
		return 0;
	}

	if (cache->size < head->addr + head->len) {
		real_size = cache->size - head->addr;
		hal_log_err("Read size %d over range, some of data will be lost, real size to read is %d",
			head->len, real_size);
	}

	hal_spin_lock(&cache->buffer_lock);
	memcpy(buf, cache->buffer + head->addr, real_size);
	hal_spin_unlock(&cache->buffer_lock);

	return 0;
}

static int sunxi_spi_slave_test_submit(struct sunxi_spi_slave_test *slave)
{
	struct sunxi_spi_slave_head *pkt_head = &slave->frame.pkt_head;
	int ret;

	sunxi_spi_slave_head_data_parse(slave->frame.data, pkt_head);

	if (!sunxi_spi_slave_has_ptk_head(pkt_head)) {
		hal_log_debug("No Package head, wait revice from master");
		pkt_head->op_code = SUNXI_OP_HEAD;
		slave->xfer.rx_buf = slave->frame.data;
		slave->xfer.rx_len = sizeof(slave->frame.data);
	} else {
		sunxi_spi_slave_head_data_clear(slave->frame.data, sizeof(slave->frame.data));
		hal_log_debug("op=0x%x addr=0x%x len=0x%x", pkt_head->op_code, pkt_head->addr, pkt_head->len);

		switch (pkt_head->op_code) {
		case SUNXI_OP_WRITE:
			slave->frame.rx_buf = hal_malloc(pkt_head->len);
			slave->xfer.rx_buf = slave->frame.rx_buf;
			slave->xfer.tx_buf = NULL;
			slave->xfer.rx_len = pkt_head->len;
			break;
		case SUNXI_OP_READ:
			slave->frame.tx_buf = hal_malloc(pkt_head->len);
			slave->xfer.tx_buf = slave->frame.tx_buf;
			slave->xfer.rx_buf = NULL;
			slave->xfer.tx_len = pkt_head->len;
			sunxi_spi_slave_get_cache_data(slave, pkt_head, (u8 *)slave->xfer.tx_buf);
			hal_log_debug("sunxi slave get package operation read, send write buffer");
			// sunxi_spi_dump_data(slave->xfer.tx_buf, 0, slave->xfer.len);
			break;
		default:
			hal_log_debug("unknown op code %d, wait revice from master", pkt_head->op_code);
			sunxi_spi_slave_head_data_clear(slave->frame.data, sizeof(slave->frame.data));
			pkt_head->op_code = SUNXI_OP_HEAD;
			slave->xfer.rx_buf = slave->frame.data;
			slave->xfer.tx_buf = NULL;
			slave->xfer.rx_len = sizeof(slave->frame.data);
			break;
		}
	}

	return hal_spi_xfer(slave->port, &slave->xfer);
}

static void spi_slave_driver_thread(void *pArg)
{
	struct sunxi_spi_slave_test *slave = (struct sunxi_spi_slave_test *)pArg;
	struct sunxi_spi_slave_head *pkt_head;
	int ret;

	while (1) {
		ret = sunxi_spi_slave_test_submit(slave);
		if (ret != SPI_MASTER_OK) {
			switch (slave->status) {
			case SUNXI_SPI_SLAVE_RETRY:
				hal_log_warn("slave transfer retry");
				sunxi_spi_slave_head_data_clear(slave->frame.data, sizeof(slave->frame.data));
				goto retry;
				break;
			case SUNXI_SPI_SLAVE_STOP:
				hal_log_warn("slave transfer stop");
				goto terminate;
				break;
			default:
				hal_log_err("error status %d and ret %d", slave->status, ret);
				break;
			}
		}
		
		pkt_head = &slave->frame.pkt_head;
		switch (pkt_head->op_code) {
		case SUNXI_OP_HEAD:
			hal_log_debug("sunxi slave get package head");
			// sunxi_spi_dump_data(slave->xfer.rx_buf, 0, slave->xfer.len);
			break;
		case SUNXI_OP_WRITE:
			hal_log_debug("sunxi slave get package operation write, recv read buffer");
			// sunxi_spi_dump_data(slave->xfer.rx_buf, 0, slave->xfer.len);
			sunxi_spi_slave_set_cache_data(slave, pkt_head, slave->xfer.rx_buf);
			hal_free(slave->xfer.rx_buf);
			slave->xfer.rx_buf = NULL;
			slave->frame.rx_buf = NULL;
			break;
		case SUNXI_OP_READ:
			hal_log_debug("send write buffer done");
			hal_free(slave->xfer.tx_buf);
			slave->xfer.tx_buf = NULL;
			slave->frame.tx_buf = NULL;
			break;
		default:
			hal_log_debug("sunxi slave get op_code filed");
			sunxi_spi_slave_head_data_clear(slave->frame.data, sizeof(slave->frame.data));
			break;
		}
	retry:
		memset(&slave->xfer, 0, sizeof(slave->xfer));
	}

terminate:
	hal_sem_post(slave->semaphore_finished);
}

static int spi_slave_driver_abort(hal_spi_master_port_t port)
{
	struct sunxi_spi_slave_test *slave = &spi_slave_test[port];

	hal_log_info("slave transfer abort");

	slave->status = SUNXI_SPI_SLAVE_RETRY;
	hal_spi_slave_abort(port);

	return 0;
}

static int spi_slave_driver_dump(hal_spi_master_port_t port, int addr, int size)
{
	struct sunxi_spi_slave_test *slave = &spi_slave_test[port];

	if (addr > slave->cache.size || addr + size > slave->cache.size) {
		hal_log_err("dump addr/size out of bounds");
		return -1;
	}

	sunxi_spi_dump_data(slave->cache.buffer, addr, size);

	return 0;
}

static int spi_slave_driver_probe(hal_spi_master_port_t port, uint32_t freq)
{
	struct sunxi_spi_slave_test *slave = &spi_slave_test[port];

	slave->port = port;
	slave->cfg.clock_frequency = freq;
	slave->cfg.slave_port = HAL_SPI_MASTER_SLAVE_0;
	slave->cfg.cpha = HAL_SPI_MASTER_CLOCK_PHASE0;
	slave->cfg.cpol = HAL_SPI_MASTER_CLOCK_POLARITY0;
	slave->cfg.slave = true;
	if (SPI_MASTER_OK != hal_spi_init(slave->port, &slave->cfg)) {
		hal_log_err("spi init failed");
		return -1;
	}

	slave->semaphore_finished = hal_sem_create(0);
	if (slave->semaphore_finished == NULL)
	{
		hal_log_err("[spi%d] creating semaphore_finished failed", slave->port);
		return -1;
	}

	hal_spin_lock_init(&slave->cache.buffer_lock);
	slave->cache.size = SLAVE_CACHE_MAX;
	slave->cache.buffer = hal_malloc(slave->cache.size);
	if (!slave->cache.buffer) {
		hal_log_err("alloc slave cache memory failed (size %d)", slave->cache.size);
		return -1;
	}

	sunxi_spi_init_slave_data(slave, 0xff);

	snprintf(slave->task_name, sizeof(slave->task_name), "spi%d-slave-task\0", slave->port);
	slave->thread_handle = hal_thread_create(spi_slave_driver_thread, slave, slave->task_name,
							SPI_SLAVE_THREAD_STACK_SIZE, HAL_THREAD_PRIORITY_SYS);
	if (slave->thread_handle == NULL) {
		hal_log_err("create thread %s failed", slave->task_name);
		return -1;
	}

	slave->status = SUNXI_SPI_SLAVE_RUNNING;

	hal_thread_start(slave->thread_handle);

	return 0;
}

static int spi_slave_driver_remove(hal_spi_master_port_t port)
{
	struct sunxi_spi_slave_test *slave = &spi_slave_test[port];

	slave->status = SUNXI_SPI_SLAVE_STOP;
	hal_spi_slave_abort(port);
	hal_sem_wait(slave->semaphore_finished);
	hal_thread_stop(slave->thread_handle);
	hal_free(slave->cache.buffer);
	hal_spin_lock_deinit(&slave->cache.buffer_lock);
	hal_spi_deinit(slave->port);

	return 0;
}

static void print_usage(const char *name)
{
	hal_log_info("Usage:");
	hal_log_info("\t%s probe <port> <freq>", name);
	hal_log_info("\t%s remove <port>", name);
	hal_log_info("\t%s abort <port>", name);
	hal_log_info("\t%s dump <port> <addr> <size>", name);
}

static int cmd_spi_slave_driver(int argc, const char **argv)
{
	hal_spi_master_port_t port;
	uint32_t freq;
	int addr, size;

	if (argc < 3) {
		print_usage(argv[0]);
		return -1;
	}

	port = strtol(argv[2], NULL, 0);
	if (port < 0 && port > HAL_SPI_MASTER_MAX) {
		hal_log_err("spi port %d not exist", port);
		return -1;
	}

	if (!strcmp(argv[1], "probe")) {
		freq = strtol(argv[3], NULL, 0);
		spi_slave_driver_probe(port, freq);
	}
	else if (!strcmp(argv[1], "remove"))
		spi_slave_driver_remove(port);
	else if (!strcmp(argv[1], "abort"))
		spi_slave_driver_abort(port);
	else if (!strcmp(argv[1], "dump")) {
		addr = strtol(argv[3], NULL, 0);
		size = strtol(argv[4], NULL, 0);
		spi_slave_driver_dump(port, addr, size);
	}
	else
		print_usage(argv[0]);

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_spi_slave_driver, hal_spi_slave_driver, spi hal slave driver test)