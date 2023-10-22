#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "console.h"

#include "sunxi_hal_ahb.h"
#include <hal_time.h>

#define MAHB_VERSION "0.5"
#define CONFIG_NUM    3

typedef struct NodeInfo {
	enum ahb_huster type;
	char *name;
	unsigned int curcnt;
	unsigned int precnt;
	unsigned long long delta;
	int flags;
} NodeInfo;

typedef struct NodeUnit {
	int	 id;
	char *name;
	unsigned int div;
} NodeUnit;

#if defined(CONFIG_ARCH_SUN20IW2P1)
static NodeInfo nodeInfo_sunxi[] = {
	{ 0           , "total"  , 0, 0, 0, 0},
	{ AHB_CPU0    , "cpu0"  , 0, 0, 0, -1},
	{ AHB_RISCV   , "rv", 0, 0, 0, -1},
	{ AHB_DSP     , "dsp" , 0, 0, 0, -1},
	{ AHB_CPU1    , "cpu1" , 0, 0, 0, -1},
	{ AHB_CE      , "ce" , 0, 0, 0, -1},
	{ AHB_DMA0    , "dma0"   , 0, 0, 0, -1},
	{ AHB_DMA1    , "dma1"   , 0, 0, 0, -1},
	{ AHB_CSI     , "csi"  , 0, 0, 0, -1},
	{ AHB_ST0     , "st0" , 0, 0, 0, -1},
	{ AHB_ST1     , "st1", 0, 0, 0, -1},
};
#else
static NodeInfo nodeInfo_sunxi[] = {
};
#endif

static NodeUnit nodeUnit_sunxi[] = {
	{ 0, "Byte",  1 },
	{ 1, "KB",  1024 },
	{ 2, "MB",  1024*1024 },
};


static NodeInfo *nodeInfo;
static NodeUnit *nodeUnit;

static unsigned int max;
static unsigned long long total;
static unsigned long long idx;

static int nodeCnt;

static int delay;
static int iter;
static int unit;

static int exit_flag = 0;

static int mahb_read();
static void mahb_post();
static void mahb_update();

static void usage(char *program)
{
	fprintf(stdout, "\r\n");
	fprintf(stdout, "Usage: %s [-n iter] [-d delay] [-u unit]  [-h]\r\n", program);
	fprintf(stdout, "    -n NUM   Updates to show before exiting.\r\n");
	fprintf(stdout, "    -d NUM   Seconds to wait between update.\r\n");
	fprintf(stdout, "    -u unit  0-Byte, 1-KB, 2-MB. default :%s.\r\n", nodeUnit[unit].name);
	fprintf(stdout, "    -c config the master to display.such as -c 1 3 4\r\n");
	fprintf(stdout, "       1.cpu0    2.rv    3.dsp    4.cpu1    5.ce\r\n");
	fprintf(stdout, "       6.dma0    7.dma1  8.csi    9.st0    10.st1\r\n");
	fprintf(stdout, "    -v Display mahb version.\r\n");
	fprintf(stdout, "    -h Display this help screen.\r\n");
	fprintf(stdout, "\r\n");
}

static void version(void)
{
	fprintf(stdout, "\r\n");
	fprintf(stdout, "mahb version: %s\r\n", MAHB_VERSION);
	fprintf(stdout, "\r\n");
}

static void nodeinfo_config_enable(int *config)
{
	int i, j;

	fprintf(stdout, "\r\n");
	for (j = 0; j < CONFIG_NUM; j++) {
		for (i = 1; i < nodeCnt; i++) {
			if (i == config[j]) {
				nodeInfo[i].flags = 0;
				fprintf(stdout, "display master:%s\r\n", nodeInfo[i].name);
				break;
			}
		}
	}
	fprintf(stdout, "\r\n");
}

static void nodeinfo_config_disable(void)
{
	int i;
	for (i = 1; i < nodeCnt; i++)
		nodeInfo[i].flags = -1;
}


static int mahb_read(void)
{
	int i;

	for (i = 1; i < nodeCnt; i++) {
		if (nodeInfo[i].flags == -1)
			continue;
		hal_ahb_huster_get_value(nodeInfo[i].type, &nodeInfo[i].curcnt);
	}

	return 0;
}

static void mahb_post(void)
{
	int i;

	for (i = 1; i < nodeCnt; i++) {
		if (nodeInfo[i].flags == -1)
			continue;
		nodeInfo[i].precnt = nodeInfo[i].curcnt;
	}
}

static void mahb_update(void)
{
	int i;
	unsigned int cur_total;
	unsigned int average;

	cur_total = 0;

	nodeInfo[0].delta = 0;
	for (i = 1; i < nodeCnt; i++) {
		if (nodeInfo[i].flags == -1)
			continue;
		if (nodeInfo[i].precnt <= nodeInfo[i].curcnt)
			nodeInfo[i].delta = nodeInfo[i].curcnt - nodeInfo[i].precnt;
		else
			nodeInfo[i].delta = (unsigned int)((nodeInfo[i].curcnt + (unsigned long long)(2^32)) - nodeInfo[i].precnt);

		nodeInfo[i].delta /= nodeUnit[unit].div;
		cur_total += nodeInfo[i].delta;
	}
	nodeInfo[0].delta = cur_total;

	if (cur_total > max)
		max = cur_total;
	total += cur_total;
	idx++;
	average = total / idx;

	fprintf(stdout, "\r\nPlease enter Ctrl-C or 'q' to quit the command!\r\n");
	fprintf(stdout, "total: %lu, ", total);
	fprintf(stdout, "num: %lu, ", idx);
	fprintf(stdout, "Max: %u, ", max);
	fprintf(stdout, "Average: %u\r\n", average);

	for (i = 0; i < nodeCnt; i++) {
		if (nodeInfo[i].flags == -1)
			continue;
		fprintf(stdout, "%-7s ", nodeInfo[i].name);
	}
	fprintf(stdout, "\r\n");
	for (i = 0; i < nodeCnt; i++) {
		if (nodeInfo[i].flags == -1)
			continue;
		fprintf(stdout, "%-7lu ", nodeInfo[i].delta);
	}
	fprintf(stdout, "\r\n");
	if (cur_total == 0)
		cur_total++;
	for (i = 0; i < nodeCnt; i++) {
		if (nodeInfo[i].flags == -1)
			continue;
		fprintf(stdout, " %7.2f", (float)nodeInfo[i].delta*100/cur_total);
	}
	fprintf(stdout, "\r\n\r\n");

}

static void mahb_start(void * param)
{
	int i;

	hal_ahb_huster_enable();
	hal_msleep(1);

	mahb_read();
	mahb_post();

	while (iter == -1 || iter-- > 0) {
		sleep(delay);
		mahb_read();
		mahb_update();
		mahb_post();
		if (exit_flag == 1) {
			exit_flag = 0;
			break;
		}
	}
	for (i = 1; i < nodeCnt; i++) {
		if (nodeInfo[i].flags == -1)
			continue;
		hal_ahb_disable_id_chan(nodeInfo[i].type);
	}
	nodeinfo_config_disable();
	vTaskDelete(NULL);
}

int cmd_mahb(int argc, char *argv[])
{
	int i , j = 0;
	int config[CONFIG_NUM];
	unsigned long value, bandwidth;
	portBASE_TYPE ret;

	nodeCnt = sizeof(nodeInfo_sunxi)/sizeof(nodeInfo_sunxi[0]);
	nodeInfo = nodeInfo_sunxi;
	nodeUnit = nodeUnit_sunxi;

	total = 0;
	idx = 0;
	max = 0;
	unit = 0;
	delay = 1;
	iter = -1;

	for (i = 0; i < CONFIG_NUM; i++)
		config[i] = i + 1;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-n")) {
			if (i + 1 >= argc) {
				fprintf(stderr, "Option -n expects an argument.\r\n");
				usage(argv[0]);
				return -1;
			}
			iter = atoi(argv[++i]);
			// FIXME
			continue;
		}

		if (!strcmp(argv[i], "-d")) {
			if (i + 1 >= argc) {
				fprintf(stderr, "Option -d expects an argument.\r\n");
				usage(argv[0]);
				return -1;
			}
			delay = atoi(argv[++i]);
			// FIXME
			continue;
		}

		if (!strcmp(argv[i], "-u")) {
			if (i + 1 >= argc) {
				fprintf(stderr, "Option -n expects an argument.\r\n");
				usage(argv[0]);
				return -1;
			}
			unit = atoi(argv[++i]);
			// FIXME
			continue;
		}

		if (!strcmp(argv[i], "-c")) {
			if (i + 3 >= argc) {
				fprintf(stderr, "Option -c expects an argument.\r\n");
				usage(argv[0]);
				return -1;
			}

			for (; j < 3; j++)
			{
				config[j] = strtol(argv[++i], NULL, 0);
			}
			// FIXME
			continue;
		}


		if (!strcmp(argv[i], "-v")) {
			version();
			return 0;
		}

		if (!strcmp(argv[i], "-h")) {
			usage(argv[0]);
			return 0;
		}

		fprintf(stderr, "Invalid argument \"%s\".\r\n", argv[i]);
		usage(argv[0]);
		return -1;
	}

	fprintf(stdout, "\r\n");
	fprintf(stdout, "iter: %d\r\n", iter);
	fprintf(stdout, "dealy: %d\r\n", delay);
	fprintf(stdout, "unit: %s\r\n", nodeUnit[unit].name);
	fprintf(stdout, "\r\n");

	nodeinfo_config_enable(config);
	ret = xTaskCreate(mahb_start, (signed portCHAR *) "mahb", 4096, NULL, 0, NULL);
	if (ret != pdPASS) {
		printf("Error creating task, status was %d\r\n", ret);
		return -1;
	}
	while(1) {
		char cRxed = 0;

        cRxed = getchar();
        if(cRxed == 'q' || cRxed == 3) {
			exit_flag = 1;
			return 0;
		}
	}

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_mahb, mahb, test bus width);

