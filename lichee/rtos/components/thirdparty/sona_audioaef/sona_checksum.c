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

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include "console.h"
#include "sona_config.h"

void md5(unsigned char *input, int len, unsigned char output[16]);

static void print_help_msg(void)
{
	printf("Usage:\n");
	printf("sona_checksum [option] <built-in config>\n");
	printf("-h,--help	: print help message\n");
	printf("\n");
	printf("built-in configs:\n");
	sona_config_global_configs_print();
}

int cmd_sona_checksum(int argc, char **argv)
{
	int opt;
	const struct option long_opts[] = {
		{"help", no_argument, NULL, 'h'},
	};
	unsigned char checksum_result[16];
	sona_config_t *sona_config = NULL;
	size_t sona_config_size;

	while ((opt = getopt_long(argc, argv, "hm", long_opts, NULL)) != -1) {
		switch (opt) {
		case 'h':
			print_help_msg();
			return 0;
		default:
			printf("Invalid option: -%c\n", (char)opt);
			print_help_msg();
			return -1;
		}
	}

	if (argc - optind <= 0) {
		print_help_msg();
		return -1;
	}

	sona_config = sona_config_find(argv[optind]);
	if (!sona_config) {
		printf("sona config \"%s\" not found\n", argv[optind]);
		return -1;
	}

	sona_config_size = sona_config->end - sona_config->start;
	md5((unsigned char *)sona_config->start, sona_config_size, checksum_result);

	printf("md5 checksum of %s:\n", sona_config->name);
	for (int i = 0; i < 16; ++i) {
		printf ("%02x", checksum_result[i]);
	}
	printf("\n");

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_sona_checksum, sona_checksum,
		Calculate checksum of built-in sona config);
