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
#include <errno.h>
#include "wv_log.h"
#include <unistd.h>
#include <hal_cmd.h>
#include "wv_network.h"
#include "wv_camera.h"
#include "wv_communication.h"

static void usage(void)
{
	printf("Usage: wireless_video [option]\n");
	printf("    -h,    help\n");
	printf("    -d,    debug level\n");
	printf("    -n,    netlink mode(ble/softap/xconfig)\n");
	printf("\n");
}

static int app_init(char * link_mode)
{
	wv_log_set_level(L_DEBUG);

	if (network_init(link_mode) < 0) {
		LOG_D("network init fail");
		return -1;
	}

	image_capture_start();
	tcp_server_start();

	return 0;
}

static int app_deinit(void)
{
	return 0;
}

int cmd_wireless_video(int argc, char *argv[])
{
	int optc;
	char *link_mode = NULL;

	if (argc < 2) {
		usage();
		return 0;
	}

	while ((optc = getopt(argc, argv, "p:d:h:n:b::")) != -1) {
		switch (optc) {
		case 'd':
			printf("debug log xxx\n");
			break;
		case 'h':
			usage();
			break;
		case 'n':
			link_mode = optarg;
			app_init(link_mode);
			break;
		default:
			usage();
			break;
		}
	}

    return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_wireless_video, wireless_video, wirelsee video demo);
