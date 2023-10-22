/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
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
#include <unistd.h>
#include <console.h>
#include <aw_common.h>
#include <sunxi_hal_usb.h>

#include "u_audio.h"

int8_t g_uacd_debug_mask = 1;

static inline void uacd_version(void)
{
	printf("uacd version:%s, compiled on: %s %s\n", UACD_VERSION, __DATE__, __TIME__);
}

int usb_gadget_uac_init();
int uacd_main(void)
{
	int ret = 0;

	uacd_version();

	ret = usb_gadget_uac_init();
	if (ret < 0) {
		printf("usb_gadget_uac_init failed\n");
		goto err;
	}
	ret = usb_gadget_function_enable("uac");
	if (ret < 0) {
		printf("enable adb function failed");
		goto err;
	}

err:
	return 0;
}


static void usage(void)
{
	printf("Usgae: uacd [option]\n");
	printf("-v,          uacd version\n");
	printf("-h,          uacd help\n");
	printf("-d,          uacd debug level\n");
	printf("\n");
}
int cmd_uacd(int argc, char *argv[])
{
	int ret = 0, c;

	optind = 0;
	while ((c = getopt(argc, argv, "vhd:")) != -1) {
		switch (c) {
		case 'v':
			uacd_version();
			return 0;
		case 'd':
			g_uacd_debug_mask = atoi(optarg);
			return 0;
		case 'h':
		default:
			usage();
			return 0;
		}
	}

	ret = uacd_main();

	return ret;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_uacd, uacd, uacd service);
