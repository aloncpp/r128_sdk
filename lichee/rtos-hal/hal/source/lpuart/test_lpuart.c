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
#include <hal_log.h>
#include <hal_cmd.h>
#include <hal_timer.h>
#include <hal_lpuart.h>

#define LPUART LPUART_1
#define LPUART_TRANSFER_DATA_LEN 10
#define LPUART_WAIT_FOREVER      0xffffffffU
#define LPUART_RDR (0x0014)
static lpuart_priv_t g_lpuart_info;
static void cmd_usage(void)
{
	printf("Usage:\n"
		"\t hal_lpuart <port> <baudrate>\n");
}

static int lpuart_transfer_data_test(lpuart_port_t port)
{
//	const unsigned long lpuart_base = sunxi_lpuart_port[port];
//	hal_writeb(0, lpuart_base + LPUART_RDR);
	return 0;
}

static int lpuart_compare_data_test(lpuart_port_t port)
{
	/* compare register */
	printf("compare data test begin\n");
	return 0;
}

int cmd_test_lpuart(int argc, char **argv)
{
	if (argc != 3) {
		cmd_usage();
		return -1;
	}

	lpuart_port_t port;
	uint32_t baudrate;

	port = strtol(argv[1], NULL, 0);
	baudrate = strtol(argv[2], NULL, 0);
	baudrate = 9600;

	if (hal_lpuart_init(port) != SUNXI_HAL_OK) {
		printf("Fail to init lpuart\n");
		return -1;
	}

	lpuart_transfer_data_test(port);
	lpuart_compare_data_test(port);

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_lpuart, hal_lpuart, lpuart hal APIs tests)
