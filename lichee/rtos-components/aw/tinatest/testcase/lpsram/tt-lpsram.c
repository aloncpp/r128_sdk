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
#include <string.h>

#include <tinatest.h>
#include <hal_log.h>
#include <hal_cmd.h>
#include <hal_mem.h>
#include <hal_cache.h>
#include <hal_dma.h>
#include <hal_time.h>

#define DMA_DATA_LEN	CONFIG_LPSRAM_DATA_LEN
#define TIMES		CONFIG_LPSRAM_TIMES
#define STRESS		CONFIG_LPSRAM_STRESS
#define MODE		0x3

int data_len_g = 0;
int stress = 0;
int max = 0;
uint32_t mode_g = 0;

static void show_help(void)
{
	printf("lpsram test app:\n"
		"'m' test mode.r=1/w=2/rw=3.\
		eg -m 1(test lpsram dma write to lpsram)\n"
		"'M' test max malloc data\n"
		"'s' data len.eg:1M/2M/etc.\n"
		"'y' stress test\n"
		"'h' get help\n");
}

static int parse_opts(int argc, char **argv)
{
	char *endprt = "k";

	mode_g = 0;
	data_len_g = 0;
	max = 0;
	stress = 0;

	while (1) {
		int c;

		c = getopt(argc, argv, "hMms:y");

		if (c == -1) {
			break;
		}

		switch (c) {
		case 'M':
			max = 1;
			break;
		case 'm':
			mode_g = atoi(optarg);
			if (!(mode_g & 3)) {
				printf("Wrong args, please input 1/2/3\n");
				printf("Use default mode = 0x3, test r&w\n");
				show_help();
			}
			break;
		case 's':
			data_len_g = strtoul(optarg, &endprt, 0);
			if (data_len_g > 4 * 1024) {
				printf("It could not test more than 4M Bytes\n");
				printf("Uesd default data_len_g = 512k\n");
				data_len_g = 512;
			}
			break;
		case 'y':
			stress = 1;
			break;
		case 'h':
			show_help();
			return -1;
		default:
			show_help();
			return -1;
		}
	}
	return 0;
}

static uint32_t _lpsram_max_mem(void)
{
	uint32_t size = 0;
	uint32_t *buf = NULL;
	int i;

	for (i = 16; i < 10000; i += 8) {
		size = 1024 * i;
		buf = malloc(size);
		if (!buf) {
			size = 1024 * (i - 8);
			printf("lpsram test %dKB size\n", size / 1024);
			break;
		}
		free(buf);
	}
	return size;
}
/*
 * function:lpsram_get_len
 * useage:  must run after getopts
 */
static int lpsram_get_len(int argc, char **argv)
{
	char *endprt = "k";
	int data_len = data_len_g;

	//parse_opts(argc, argv);
	printf("initialize data_len:%d defconfig:%s\n", data_len, DMA_DATA_LEN);
	if (data_len == 0)
		data_len = strtoul(DMA_DATA_LEN, &endprt, 0);

	data_len = data_len * 1024;
	if (max == 1) {
		data_len = _lpsram_max_mem();
		data_len += 8;
		data_len = data_len / 2;
	}
	//data_len = 1024 * 100;
	//data_len = 128;
	//printf("the last data_len:%dk\n", data_len / 1024);

	return data_len;
}

static void lpsram_cb(void *para)
{
	hal_log_info("Lpsram use dma finished, callback...");
}

static int dma_transfer(struct sunxi_dma_chan *hdma,
		char *buf1, char *buf2, int len)
{
	struct dma_slave_config config = {0};
	int ret = -1, i = 0;
	uint32_t size = 0;

	ret = hal_dma_chan_request(&hdma);
	if (ret == HAL_DMA_CHAN_STATUS_BUSY) {
		hal_log_err("dma channel busy!");
		return -1;
	}

	ret = hal_dma_callback_install(hdma, lpsram_cb, hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("register dma callback failed!");
		return -1;
	}

	config.direction = DMA_MEM_TO_MEM;
	config.dst_addr_width = DMA_SLAVE_BUSWIDTH_8_BYTES;
	config.src_addr_width = DMA_SLAVE_BUSWIDTH_8_BYTES;
	config.dst_maxburst = DMA_SLAVE_BURST_16;
	config.src_maxburst = DMA_SLAVE_BURST_16;
	config.slave_id = sunxi_slave_id(DRQDST_LSPSRAM, DRQSRC_LSPSRAM);

	ret = hal_dma_slave_config(hdma, &config);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma config error, ret:%d", ret);
		return -1;
	}

	ret = hal_dma_prep_memcpy(hdma, (unsigned long)buf2, (unsigned long)buf1, len);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma prepare error, ret:%d", ret);
		return -1;
	}

	ret = hal_dma_start(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma start error, ret:%d", ret);
		return -1;
	}

	hal_sleep(1);
	while (hal_dma_tx_status(hdma, &size)!= 0);

	ret = hal_dma_stop(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma stop error, ret:%d", ret);
		return -1;
	}

	ret = hal_dma_chan_free(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma free error, ret:%d", ret);
		return -1;
	}
/* #define LPSRAM_DEBUG */
#ifdef LPSRAM_DEBUG
	hal_dcache_invalidate((unsigned long)buf2, len);
	hal_log_info("src buf:\n");
	for (i = 0; i < len; i++)
		printf("0x%x ", buf1[i]);
	hal_log_info("\ndst buf:\n");
	for (i = 0; i < len; i++)
		printf("0x%x ", buf2[i]);
	printf("\n");
#endif
	hal_dcache_invalidate((unsigned long)buf2, len);
	ret = memcmp(buf1, buf2, len);
	if (ret != 0) {
		hal_log_err("dma transfer data error!\n");
		return -1;
	}

	return 0;
}
#ifdef CONFIG_DEFAULT_SRAM_HEAP
static int tt_lpsram_dmaread(int argc, char **argv)
{
	int ret = 0, i = 0, len;
	struct sunxi_dma_chan *hdma = NULL;
	char *buf1 = (char *)0x8100000;
	char *buf2 = NULL;

	hal_log_info("------run in lpsram dma 'read' test------");
	hal_log_info("test read data from lpsram and write to sram");

	len = lpsram_get_len(argc, argv);
begin_read:
	buf2 = malloc(len);
	if (buf2 == NULL) {
		hal_log_err("malloc %d buf2 failed\n", len);
		goto read_f;
	}

	memset(buf1, 0, len);
	memset(buf2, 0, len);

	for (i = 0; i < len; i++)
		buf1[i] = i & 0xff;

	printf("The read buf addr:0x%08x, from %s\n", buf1,
			((int)(uint32_t *)buf1>0x8000000) ? "psram" : "sram");
	printf("The write buf addr:%p, from %s\n", buf2,
			((int)(uint32_t *)buf2>0x8000000) ? "psram" : "sram");

	ret = dma_transfer(hdma, buf1, buf2, len);
	if (ret < 0) {
		hal_log_err("dma_transfer for lpsram failed\n");
		goto read_buf2;
	}

	hal_log_info("lpsram dma test read success!");

	free(buf2);

	if (stress == 1) {
		goto begin_read;
	}

	return 0;

read_buf2:
	free(buf2);
read_f:
	hal_log_err("lpsram dma test read failed\n");

	return -1;
}

static int tt_lpsram_dmawrite(int argc, char **argv)
{
	int ret = 0, i = 0, len;
	struct sunxi_dma_chan *hdma = NULL;
	char *buf1 = NULL;
	char *buf2 = (char *)0x8100000;

	hal_log_info("------run in lpsram dma 'write' test------");
	hal_log_info("test read data from sram and write to lpsram");

	len = lpsram_get_len(argc, argv);

begin_write:
	buf1 = malloc(len);
	if (buf1 == NULL) {
		hal_log_err("malloc %d buf2 failed\n", len);
		goto write_f;
	}

	memset(buf1, 0, len);
	memset(buf2, 0, len);

	for (i = 0; i < len; i++)
		buf1[i] = i & 0xff;

	printf("The read buf addr:%p, from %s\n", buf1,
			((int)(uint32_t *)buf1>0x8000000) ? "psram" : "sram");
	printf("The write buf addr:%p, from %s\n", buf2,
			((int)(uint32_t *)buf2>0x8000000) ? "psram" : "sram");

	ret = dma_transfer(hdma, buf1, buf2, len);
	if (ret < 0) {
		hal_log_err("dma_transfer for lpsram failed\n");
		goto write_buf2;
	}

	hal_log_info("lpsram dma test write success!");

	free(buf1);

	if (stress == 1) {
		goto begin_write;
	}

	return 0;

write_buf2:
	free(buf2);
write_f:
	hal_log_err("lpsram dma test write failed\n");
	return -1;
}
#else
static int tt_lpsram_dmaread(int argc, char **argv)
{
	printf("Please set heap in sram before run this lpsarm dma test demo\n");
	return -1;
}

static int tt_lpsram_dmawrite(int argc, char **argv)
{
	printf("Please set heap in sram before run this lpsarm dma test demo\n");
	return -1;
}
#endif

static int tt_lpsram_dmarw(int argc, char **argv)
{
	int ret = 0, i = 0, len;
	struct sunxi_dma_chan *hdma = NULL;
	char *buf1 = NULL, *buf2 = NULL;

	hal_log_info("------run in lpsram dma 'rw' test------");
	hal_log_info("tenst read data from lpsram and write to lpsram");

	len = lpsram_get_len(argc, argv);

begin_dma:
	buf1 = hal_malloc_coherent_align(len, 64);
	if (buf1 == NULL) {
		hal_log_err("malloc %d buf1 failed\n", len);
		goto fail;
	}

	buf2 = hal_malloc_coherent_align(len, 64);
	if (buf2 == NULL) {
		hal_log_err("malloc %d buf2 failed\n", len);
		goto buf1_fail;
	}

	memset(buf1, 0, len);
	memset(buf2, 0, len);

	for (i = 0; i < len; i++)
		buf1[i] = i & 0xff;

	printf("The read buf addr:%p, from %s\n", buf1,
			((long)(uint32_t *)buf1 > 0x8000000) ? "psram" : "sram");
	printf("The write buf addr:%p, from %s\n", buf2,
			((long)(uint32_t *)buf2 > 0x8000000) ? "psram" : "sram");

	hal_dcache_clean_invalidate((unsigned long)buf1, len);
	hal_dcache_clean_invalidate((unsigned long)buf2, len);

	ret = dma_transfer(hdma, buf1, buf2, len);
	if (ret < 0) {
		hal_log_err("dma_transfer for lpsram failed\n");
		goto buf2_fail;
	}

	hal_log_info("lpsram dma test success!\n");

	hal_free_coherent_align(buf2);
	hal_free_coherent_align(buf1);

	if (stress == 1) {
		goto begin_dma;
	}

	return 0;

buf2_fail:
	hal_free_coherent_align(buf2);
buf1_fail:
	hal_free_coherent_align(buf1);
fail:
	hal_log_err("lpsram dma test failed\n");
	return -1;
}

int tt_lpsram_dmatest(int argc, char **argv)
{
	uint32_t mode = 0;
	int ret = 0;
	/* parse_opts(argc, argv); */

	mode = (uint32_t)mode_g;
	if (mode == 0)
		mode = 3;
	switch (mode) {
	case 1:
		ret = tt_lpsram_dmaread(argc, argv);
		break;
	case 2:
		ret = tt_lpsram_dmawrite(argc, argv);
		break;
	case 3:
		ret = tt_lpsram_dmarw(argc, argv);
		break;
	default:
		show_help();
		break;
	}
	return ret;
}
/* FINSH_FUNCTION_EXPORT_CMD(tt_lpsram_dmatest, lpsram_dma, lpsram dma hal APIs tests) */
//testcase_init(tt_lpsramtest, lpsram_dmatester, lpsram_dmatester for tinatest);

int lpsram_demo(int argc, char **argv)
{
	int ret = -1, i = 0, cnt = 0, data_len = 0;
	char *buf1 = NULL, *buf2 = NULL;

	hal_log_info("run in lpsram demo test");

	/* parse_opts(argc, argv); */
	data_len = lpsram_get_len(argc, argv);
	if (data_len == 0)
		data_len = 1 * 1024;

	if (max == 1) {
		data_len = _lpsram_max_mem();
		data_len += 8;
		data_len = data_len / 2;
	}

	printf("test %d Bytes lpsram demo\n", data_len);

demo_begin:
	buf1 = malloc(data_len);
	if (buf1 == NULL) {
		hal_log_err("malloc buf1 failed\n");
		goto demo_failed;
	}
	buf2 = malloc(data_len);
	if (buf2 == NULL) {
		hal_log_err("malloc buf2 failed\n");
		goto demo_buf1;
	}

	memset(buf1, 0, data_len);
	memset(buf2, 0, data_len);

	for (i = 0; i < data_len; i++) {
		if (cnt == 0) {
			if (i % 2 == 0)
				buf1[i] = 0x5a;
			else
				buf1[i] = 0xa5;
		} else if (cnt == 1) {
			buf1[i] = i & 0xff;
		} else if (cnt == 2) {
			if (i % 2 == 0)
				buf1[i] = 0x00;
			else
				buf1[i] = 0xff;
		}
	}

	printf("test buf data is 0x%x and 0x%x\n", buf1[0], buf1[1]);

	memcpy(buf2, buf1, data_len);

	ret = memcmp(buf1, buf2, data_len);
	if (ret != 0) {
		hal_log_err("copy buf1 data to buf2 failed\n");
		goto demo_buf2;
	}

	++cnt;

	if (cnt == 3)
		cnt = 0;

	free(buf1);
	free(buf2);

	if (stress == 1) {
		goto demo_begin;
	}

	return 0;

demo_buf2:
	free(buf2);
demo_buf1:
	free(buf1);
demo_failed:
	hal_log_err("lpsram demo test failed\n");

	return -1;
}
/* FINSH_FUNCTION_EXPORT_CMD(lpsram_demo, lpsram_demo, lpsram demo hal APIs tests) */

int tt_lpsramtest(int argc, char **argv)
{
	int times = 0, mode = 0, ret = 0, stress = 0;
	int i = 0, j = 0;
	char *endprt = "k";

	mode = MODE;
	times = strtoul(TIMES, NULL, 0);
	stress = strtoul(STRESS, NULL, 0);
	data_len_g = strtoul(DMA_DATA_LEN, &endprt, 0);

	printf("========Running lpsram cpu/dma read/write test========\n");
	printf("test data len:%dk\n", data_len_g);
	printf("test mode:%d\n", mode);
	printf("stress test:%d\n", stress);
	if (stress == 0)
		printf("loop times:%d\n", times);

	for (i = 0; i < times; i++) {
		printf("%d times test cpu rw\n", j);
		ret = lpsram_demo(0, NULL);
		if (ret < 0) {
			printf("lpsram cpu test failed, exit!\n");
			break;
		}
		printf("%d times test dma rw\n", j);
		ret = tt_lpsram_dmatest(0, NULL);
		if (ret < 0) {
			printf("lpsram dma test failed, exit!\n");
			break;
		}
		j++;
		if ((i == times - 1) && (stress == 1))
			i = 0;
	}

	return ret;
}
testcase_init(tt_lpsramtest, lpsramtester, lpsramtester for tinatest);
