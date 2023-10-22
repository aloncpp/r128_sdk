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
#include <sunxi_hal_spi.h>

#define PKT_HEAD_LEN 5

#define OP_MASK		0
#define ADDR_MASK_0	1
#define ADDR_MASK_1	2
#define LEN_MASK_0	3
#define LEN_MASK_1	4

#define SUNXI_OP_WRITE	0x01
#define SUNXI_OP_READ	0x03

#define PKT_HEAD_DELAY	100
#define PKT_XFER_DELAY	500

#define KB (1024)
#define MB (1024*KB)
#define US (1)
#define MS (1000*US)
#define S  (1000*MS)

struct sunxi_spi_slave_head {
	unsigned int op_code;
	unsigned int addr;
	unsigned int len;
};

static int verbose;

static void hex_dump(const void *src, size_t length, size_t line_size,
		     char *prefix)
{
	int i = 0;
	const unsigned char *address = src;
	const unsigned char *line = address;
	unsigned char c;

	printf("%s | ", prefix);
	while (length-- > 0) {
		printf("%02X ", *address++);
		if (!(++i % line_size) || (length == 0 && i % line_size)) {
			if (length == 0) {
				while (i++ % line_size)
					printf("__ ");
			}
			printf(" |");
			while (line < address) {
				c = *line++;
				printf("%c", (c < 32 || c > 126) ? '.' : c);
			}
			printf("|\n");
			if (length > 0)
				printf("%s | ", prefix);
		}
	}
}

static void show_transfer_info(unsigned long size, unsigned long time)
{
	double rate;

	printf("total size   : ");
	if (size >= MB) {
		printf("%.2lf MB", (double)size/(double)MB);
	} else if (size >= KB) {
		printf("%.2lf KB", (double)size/(double)KB);
	} else {
		printf("%lu B", size);
	}
	printf("\n");

	printf("total time   : ");
	if (time >= S) {
		printf("%.2lf s", (double)time/(double)S);
	} else if (time >= MS) {
		printf("%.2lf ms", (double)time/(double)MS);
	} else if (time >= US) {
		printf("%.2lf us", (double)time/(double)US);
	} else {
		printf("%lu ns", time);
	}
	printf("\n");

	rate = ((double)size / (double)MB) / ((double)time / (double)S);
	printf("averange rate: %.2lf MB/s\n", rate);
}

static int transfer_pkg_create(char *buf, struct sunxi_spi_slave_head *head)
{
	buf[OP_MASK] = head->op_code;
	buf[ADDR_MASK_0] = (head->addr >> 8) & 0xff;
	buf[ADDR_MASK_1] = head->addr & 0xff;
	buf[LEN_MASK_0] = (head->len >> 8) & 0xff;
	buf[LEN_MASK_1] = head->len & 0xff;

	return 0;
}

static int transfer_slave_package(hal_spi_master_port_t port, struct sunxi_spi_slave_head *head, char *tx_buf, char *rx_buf)
{
	char head_buf[PKT_HEAD_LEN];
	hal_spi_master_transfer_t tr[2];
	int i;
	int ret;

	memset(tr, 0, sizeof(tr));

	transfer_pkg_create(head_buf, head);
	if (verbose) {
		printf("package head : { ");
		for (i = 0; i < PKT_HEAD_LEN; i++) {
			printf("0x%02x ", head_buf[i]);
		}
		printf("}\n");
	}

	tr[0].tx_buf = (uint8_t *)head_buf;
	tr[0].tx_nbits = SPI_NBITS_SINGLE;
	tr[0].tx_len = sizeof(head_buf);
    tr[0].tx_single_len = sizeof(head_buf);
	tr[0].rx_buf = (uint8_t *)NULL;
	tr[0].rx_nbits = 0;
    tr[0].rx_len = 0;

    tr[1].tx_buf = (uint8_t *)tx_buf;
	tr[1].tx_nbits = SPI_NBITS_SINGLE;
	tr[1].tx_len = head->len;
    tr[1].tx_single_len = head->len;
	tr[1].rx_buf = (uint8_t *)rx_buf;
	tr[1].rx_nbits = SPI_NBITS_SINGLE;
    tr[1].rx_len = head->len;

    hal_spi_xfer(port, &tr[0]);
    hal_usleep(PKT_HEAD_DELAY);
    hal_spi_xfer(port, &tr[1]);

	return 0;
}

static int transfer_slave(hal_spi_master_port_t port, uint32_t addr, uint32_t size)
{
	struct sunxi_spi_slave_head pkt_head;
	char *tx_buf = NULL;
	char *rx_buf = NULL;
	struct timeval start, end;
	unsigned long nsec = 0;
	int i;

	tx_buf = hal_malloc(size);
	srand(time(0));
	for (i = 0; i < size; i++)
		tx_buf[i] = random() % 256;

	rx_buf = hal_malloc(size);
	memset(rx_buf, 0, size);

	gettimeofday(&start, NULL);
	// Write forward
	pkt_head.op_code = SUNXI_OP_WRITE;
	pkt_head.addr = addr;
	pkt_head.len = size;
	transfer_slave_package(port, &pkt_head, tx_buf, NULL);
	hal_usleep(PKT_XFER_DELAY);
	// Read back
	pkt_head.op_code = SUNXI_OP_READ;
	pkt_head.addr = addr;
	pkt_head.len = size;
	transfer_slave_package(port, &pkt_head, NULL, rx_buf);
	gettimeofday(&end, NULL);
	// Debug
	if (verbose) {
		hex_dump(tx_buf, size, 32, "TX");
		hex_dump(rx_buf, size, 32, "RX");
	}
	// Compare buffer
	if (memcmp(tx_buf, rx_buf, size))
		printf("rx/tx buffer is not same, compare error!!!\n");
	else
		nsec += (1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec);

	free(tx_buf);
	free(rx_buf);

	return nsec;
}

static void print_usage(const char *name)
{
    hal_log_info("Usage:");
	hal_log_info("\t%s <port> <freq> <addr> <size> <loop> [debug]", name);
}

static int cmd_test_spi_slave(int argc, const char **argv)
{
    hal_spi_master_port_t  port;
    hal_spi_master_config_t cfg;
    uint32_t addr, size;
    int loop = 1;
	unsigned long usec;
	unsigned long total_usec = 0;
	unsigned long total_size = 0;
    int i;

    if (argc < 6) {
		print_usage(argv[0]);
		return -1;
	}

    memset(&cfg, 0, sizeof(cfg));
    port = strtol(argv[1], NULL, 0);
	if (port < 0 && port > HAL_SPI_MASTER_MAX) {
		hal_log_err("spi port %d not exist", port);
		return -1;
	}

    addr = strtol(argv[3], NULL, 0);
    size = strtol(argv[4], NULL, 0);
    loop = strtol(argv[5], NULL, 0);

    if (argc == 7 && strcmp(argv[6], "debug") == 0)
        verbose = 1;
    else 
        verbose = 0;

	hal_log_info("run spi slave test");

    cfg.clock_frequency = strtol(argv[2], NULL, 0);
    cfg.slave_port = HAL_SPI_MASTER_SLAVE_0;
    cfg.cpha = HAL_SPI_MASTER_CLOCK_PHASE0;
    cfg.cpol = HAL_SPI_MASTER_CLOCK_POLARITY0;
    hal_spi_init(port, &cfg);

	hal_log_info("max speed: %u Hz (%u kHz)", cfg.clock_frequency, cfg.clock_frequency/1000);
	hal_log_info("op addr : %d", addr);
	hal_log_info("op size : %d", size);

    if (size) {
		for (i = 0; i < loop; i++) {
			usec = transfer_slave(port, addr, size);
			if (usec) {
				total_usec += usec;
				total_size += (size * 2);
			}
		}
		show_transfer_info(total_size, total_usec);
		printf("averange time: %.2lf us\n", (double)total_usec/(double)US/(double)(loop));
	}

    hal_spi_deinit(port);

    hal_log_info("spi slave test finish");

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_spi_slave, hal_spi_slave_test, spi hal slave tests)