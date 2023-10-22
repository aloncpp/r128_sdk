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

#include <hal_log.h>
#include <hal_cmd.h>
#include <hal_mem.h>
#include <hal_cache.h>
#include <hal_dma.h>
#include <hal_timer.h>

#include <sunxi_hal_common.h>

#define DMA_TEST_LEN	1024
static void dma_test_cb(void *param)
{
	hal_log_info("DMA finished, callback to do something...\n");
}

int cmd_test_dma(int argc, char **argv)
{
	int ret, i;
	struct sunxi_dma_chan *hdma = NULL;
	char *buf1 = NULL,*buf2 = NULL;
	struct dma_slave_config config = {0};
	uint32_t size = 0;

	hal_log_info("run in dma test");

	buf2 = hal_malloc_coherent(DMA_TEST_LEN);
	buf1 = hal_malloc_coherent(DMA_TEST_LEN);

	if (buf1 == NULL) {
		hal_log_err("malloc buf1 error!");
		goto end;
	}

	if (buf2 == NULL) {
		hal_log_err("malloc buf2 error!");
		goto end;
	}

	memset(buf1, 0, DMA_TEST_LEN);
	memset(buf2, 0, DMA_TEST_LEN);

	for (i = 0;i < DMA_TEST_LEN; i++)
		buf1[i] = i & 0xff;

	hal_dcache_clean_invalidate((unsigned long)buf1, DMA_TEST_LEN);
	hal_dcache_clean_invalidate((unsigned long)buf2, DMA_TEST_LEN);

	/* request dma chan */
	ret = hal_dma_chan_request(&hdma);
	if (ret == HAL_DMA_CHAN_STATUS_BUSY) {
		hal_log_err("dma channel busy!");
		goto end;
	}

	/* register dma callback */
	ret = hal_dma_callback_install(hdma, dma_test_cb, hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("register dma callback failed!");
		goto end;
	}

	config.direction = DMA_MEM_TO_MEM;
	config.dst_addr_width = DMA_SLAVE_BUSWIDTH_8_BYTES;
	config.src_addr_width = DMA_SLAVE_BUSWIDTH_8_BYTES;
	config.dst_maxburst = DMA_SLAVE_BURST_16;
	config.src_maxburst = DMA_SLAVE_BURST_16;
	config.slave_id = sunxi_slave_id(DRQDST_SDRAM, DRQSRC_SDRAM);

	ret = hal_dma_slave_config(hdma, &config);

	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma config error, ret:%d", ret);
		goto end;
	}

	ret = hal_dma_prep_memcpy(hdma, (unsigned long)buf2, (unsigned long)buf1, DMA_TEST_LEN);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma prep error, ret:%d", ret);
		goto end;
	}

	ret = hal_dma_start(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma start error, ret:%d", ret);
		goto end;
	}

	hal_sleep(1);
	while (hal_dma_tx_status(hdma, &size)!= 0);

	ret = hal_dma_stop(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma stop error, ret:%d", ret);
		goto end;
	}

	ret = hal_dma_chan_free(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma free error, ret:%d", ret);
		goto end;
	}

	hal_dcache_invalidate((unsigned long)buf2, DMA_TEST_LEN);

	hal_log_info("src buf:\n");
	for (i = 0;i < DMA_TEST_LEN; i++) {
		if (i % 16 == 0)
			printf("\n");
		printf("%02x ", buf1[i]);
	}
	printf("\n\n\n");
	hal_log_info("dst buf:\n");
	for (i = 0;i < DMA_TEST_LEN; i++) {
		if (i % 16 == 0)
			printf("\n");
		printf("%02x ", buf2[i]);
	}
	printf("\n\n\n");

	if (memcmp(buf1, buf2, DMA_TEST_LEN) != 0)
		printf("dma test fail\n");
	else
		printf("dma test pass\n");

end:
	hal_free_coherent(buf1);
	hal_free_coherent(buf2);

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_dma, hal_dma, dma hal APIs tests)
