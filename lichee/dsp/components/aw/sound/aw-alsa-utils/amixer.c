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
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <FreeRTOS.h>
#include <aw-alsa-lib/common.h>
#include <aw-alsa-lib/control.h>
#include "common.h"

#define AMIXER_COMMAND_HELP \
	"\r\namixer <[-c card_num]> <[cset/cget numid=xx]> <value>\r\n" \
	" eg: amixer -l\r\n" \
	" eg: amixer -c 0 controls\r\n" \
	" eg: amixer -c 0 cget numid=1\r\n" \
	" eg: amixer -c 2 cset numid=0 1\r\n"

static char g_card_name[64] = "audiocodec";

static void amixer_ctl_info_print(snd_ctl_info_t *info)
{
	if (info->type == SND_CTL_ELEM_TYPE_INTEGER) {
		printf("numid=%u, name=\'%s\'\n", info->id, info->name);
		printf("\t value=%lu, min=%d, max=%d\n",
			info->value, info->min, info->max);
	} else if (info->type == SND_CTL_ELEM_TYPE_ENUMERATED) {
		printf("numid=%u, name=\'%s\'\n", info->id, info->name);
		printf("\t value=%s, enum=", info->texts[info->value]);
		int i = 0;
		for (i = 0; i < info->items; i++) {
			printf("%s,", info->texts[i]);
		}
		printf("\n");
	} else {
		printf("numid=%u, name=\'%s\' type error:%d\n",
			info->id, info->name, info->type);
	}
}

static int amixer_controls(const char *card_name)
{
	int num = 0;
	int ret = 0;
	int i = 0;
	snd_ctl_info_t info;

	if ((card_name == NULL) || (!strcmp(card_name, "default")))
		card_name = "audiocodec";

	num = snd_ctl_num(card_name);
	if (num == 0) {
		printf("The card didn't have ctl.\n");
		return 0;
	} else if (num < 0)
		return -1;

	for (i = 0; i < num;i ++) {
		memset(&info, 0, sizeof(snd_ctl_info_t));
		ret = snd_ctl_get_bynum(card_name, i, &info);
		if (ret < 0) {
			printf("get %d elem failed\n", i);
			return ret;
		}
		amixer_ctl_info_print(&info);
	}
	return 0;
}

static int parser_ctl_numid(const char *cmd, int *id)
{
	char *id_str = "numid=";

	if (strncmp(cmd, id_str, strlen(id_str)) != 0)
		return -1;
	*id = atoi(cmd+strlen(id_str));
	return 0;
}

int amixer(int argc, char *argv[])
{
	int numid = 0;
	const char *card_name = NULL;
	snd_ctl_info_t info;
	const struct option long_option[] = {
		{"list", 0, NULL, 'l'},
		{"card", 1, NULL, 'c'},
		{"help", 0, NULL, 'h'},
		{NULL, 0, NULL, 0},
	};

	/* FIXME: fix get option error. */
	optind = 0;

	while (1) {
		int c;

		if ((c = getopt_long(argc, argv, "c:hl", long_option, NULL)) < 0) {
			break;
		}
		switch (c) {
		case 'l':
			asound_list();
			return 0;
		case 'c':
			{
				int card_index = 0;

				if (isdigit(*optarg)) {
					sscanf(optarg, "%d", &card_index);
					card_name = snd_card_name(card_index);
					if (card_name == NULL) {
						fprintf(stderr, "Invalid card number.\n");
					} else {
						strncpy(g_card_name, card_name, 64);
					}
				} else
					strncpy(g_card_name, optarg, 64);
				fprintf(stdout, "Card Name:%s.\n", g_card_name);
			}
			break;
		case 'h':
			printf("%s", AMIXER_COMMAND_HELP);
			return 0;
		default:
			fprintf(stderr, "Invalid switch or option needs an argument.\n");
			printf("%s", AMIXER_COMMAND_HELP);
		}
	}

	if ((argc - optind) <= 0) {
		return amixer_controls(g_card_name);
	}

	if (!strcmp("controls", argv[optind])) {
		return amixer_controls(g_card_name);
	}

	if (parser_ctl_numid(argv[optind + 1], &numid) != 0) {
		fprintf(stderr, "numid get failed.\n");
		return 1;
	}

	if (!strcmp("cget", argv[optind])) {
		if (!snd_ctl_get_bynum(g_card_name, numid, &info)) {
			amixer_ctl_info_print(&info);
			return 0;
		}
	} else if (!strcmp("cset", argv[optind])) {
		int value = -1;
		if (argc - optind > 2)
			value = atoi(argv[optind + 2]);
		if (value >= 0) {
			if (snd_ctl_get_bynum(g_card_name, numid, &info) != 0) {
				printf("snd_ctl_get failed\n");
				return -1;
			}
			snd_ctl_set_bynum(g_card_name, numid, value);
		}
		if (!snd_ctl_get_bynum(g_card_name, numid, &info)) {
			amixer_ctl_info_print(&info);
			return 0;
		}
	}

	return 0;
}
#ifdef CONFIG_COMPONENTS_FREERTOS_CLI
FINSH_FUNCTION_EXPORT_CMD(amixer, amixer, mixer test);
#endif
