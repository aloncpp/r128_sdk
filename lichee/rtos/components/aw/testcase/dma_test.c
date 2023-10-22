/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY¡¯S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS¡¯SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY¡¯S TECHNOLOGY.
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
#include "../../include/drivers/hal_dma.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tinatest.h>

int dma_test(int argc, char **argv)
{
	int i;
	unsigned long *hdma = NULL;
	char *buf1 = NULL,*buf2 = NULL;
	struct dma_slave_config config = {0};
	uint32_t size = 0;
	int ret;

	printf("run in dma test\n");

	buf1 = malloc(1024);
	if (buf1 == NULL) {
		printf("malloc buf1 error!\n");
		return -1;
	}

	buf2 = malloc(1024);
	if (buf2 == NULL) {
		free(buf1);
		printf("malloc buf2 error!\n");
		return -1;
	}

	memset(buf1, 0, 1024);
	memset(buf2, 0, 1024);

	for (i = 0;i < 1023; i++)
		buf1[i] = 'a';
	buf1[1023] = '\0';

	//sunxi_dma_init();				//init dma,should init once

	//request dma chan
	ret = hal_dma_chan_request(&hdma);

	if (ret == HAL_DMA_CHAN_STATUS_BUSY) {
		printf("dma channel busy!\n");
		free(buf1);
		free(buf2);
		return -1;
	}


	config.direction = DMA_MEM_TO_MEM;
	config.dst_addr = (uint32_t)buf1;
	config.src_addr = (uint32_t)buf2;
	config.dst_addr_width = DMA_SLAVE_BUSWIDTH_8_BYTES;
	config.src_addr_width = DMA_SLAVE_BUSWIDTH_8_BYTES;
	config.dst_maxburst = DMA_SLAVE_BURST_16;
	config.src_maxburst = DMA_SLAVE_BURST_16;
	config.slave_id = sunxi_slave_id(DRQDST_SDRAM, DRQSRC_SDRAM);

	ret = hal_dma_slave_config(hdma, &config);

	if (ret != HAL_DMA_STATUS_OK) {
		printf("dma config error,ret:%d\n", ret);
		free(buf1);
		free(buf2);
		return ret;
	}

	ret = hal_dma_prep_memcpy(hdma, (uint32_t)buf2, (uint32_t)buf1,(uint32_t)1024);
	if (ret != HAL_DMA_STATUS_OK) {
		printf("dma prep error,ret:%d\n", ret);
		free(buf1);
		free(buf2);
		return ret;
	}

	ret = hal_dma_start(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		printf("dma start error,ret:%d\n", ret);
		free(buf1);
		free(buf2);
		return ret;
	}

	while (hal_dma_tx_status(hdma, &size)!= 0);

	ret = hal_dma_stop(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		printf("dma stop error,ret:%d\n", ret);
		free(buf1);
		free(buf2);
		return ret;
	}

	ret = hal_dma_chan_free(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		printf("dma free error,ret:%d\n", ret);
		free(buf1);
		free(buf2);
		return ret;
	}

	InvalidDcacheRegion((void *)buf2, 1024);
	printf("buf1:%s\n",buf1);
	printf("buf2:%s\n",buf2);

	free(buf1);
	free(buf2);
	return 0;
}
tt_funclist_t tt_dmatest_funclist[] = {
	{"dma memcpy test", dma_test},
};

testcase_init_with_funclist(tt_dmatest_funclist, dmatest, testcase funclist test);

