#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "console.h"

#include "sunxi_hal_mbus.h"

#define MTOP_VERSION "0.5"


typedef struct NodeInfo {
	enum mbus_pmu type;
	char *name;
	unsigned int curcnt;
	unsigned int precnt;
	unsigned long long delta;
} NodeInfo;

typedef struct NodeUnit {
	int	 id;
	char *name;
	unsigned int div;
} NodeUnit;

/**
 * NOTE: we allway put totddr at first array whether this node exit or not,
 * fot the totddr is caculated every time.
 */
#if defined(CONFIG_ARCH_SUN20IW2P1)
static NodeInfo nodeInfo_sunxi[] = {
	{ MBUS_PMU_TOTAL  , "total", 0, 0, 0 },
	{ MBUS_PMU_CPU    , "m33"  , 0, 0, 0 },
	{ MBUS_PMU_RV_SYS , "c906" , 0, 0, 0 },
	{ MBUS_PMU_DSP_SYS, "hifi5", 0, 0, 0 },
	{ MBUS_PMU_DMA0   , "dma0" , 0, 0, 0 },
	{ MBUS_PMU_DMA1   , "dma1" , 0, 0, 0 },
	{ MBUS_PMU_CE     , "ce"   , 0, 0, 0 },
	{ MBUS_PMU_DE     , "de"   , 0, 0, 0 },
	{ MBUS_PMU_G2D    , "g2d"  , 0, 0, 0 },
	{ MBUS_PMU_MAHB   , "mahb" , 0, 0, 0 },
	{ MBUS_PMU_OTH    , "other", 0, 0, 0 },
};
#else
static NodeInfo nodeInfo_sunxi[] = {
	{ MBUS_PMU_TOTAL, "totddr", 0, 0, 0},
	{ MBUS_PMU_CPU  , "cpuddr", 0, 0, 0},
	{ MBUS_PMU_GPU  , "gpuddr", 0, 0, 0},
	{ MBUS_PMU_VE   , "de_ddr", 0, 0, 0},
	{ MBUS_PMU_DISP , "ve_ddr", 0, 0, 0},
	{ MBUS_PMU_OTH  , "othddr", 0, 0, 0},
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

static int mtop_read();
static void mtop_post();
static void mtop_update();

static void usage(char *program)
{
	fprintf(stdout, "\r\n");
	fprintf(stdout, "Usage: %s [-n iter] [-d delay] [-u unit]  [-h]\r\n", program);
	fprintf(stdout, "    -n NUM   Updates to show before exiting.\r\n");
	fprintf(stdout, "    -d NUM   Seconds to wait between update.\r\n");
	fprintf(stdout, "    -u unit  0-Byte, 1-KB, 2-MB. default :%s.\r\n", nodeUnit[unit].name);
	fprintf(stdout, "    -v Display mtop version.\r\n");
	fprintf(stdout, "    -h Display this help screen.\r\n");
	fprintf(stdout, "\r\n");
}

static void version(void)
{
	fprintf(stdout, "\r\n");
	fprintf(stdout, "mtop version: %s\r\n", MTOP_VERSION);
	fprintf(stdout, "\r\n");
}


static int mtop_read(void)
{
	int i;

	for (i = 1; i < nodeCnt; i++) {
		hal_mbus_pmu_get_value(nodeInfo[i].type, &nodeInfo[i].curcnt);
	}

	return 0;
}

static void mtop_post(void)
{
	int i;

	for (i = 1; i < nodeCnt; i++) {
		nodeInfo[i].precnt = nodeInfo[i].curcnt;
	}
}

static void mtop_update(void)
{
	int i;
	unsigned int cur_total;
	unsigned int average;

	cur_total = 0;

	nodeInfo[0].delta = 0;
	for (i = 1; i < nodeCnt; i++) {
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

	fprintf(stdout, "\r\nPlease enter 'mtop_exit' to quit the command!\r\n");
	fprintf(stdout, "total: %lu, ", total);
	fprintf(stdout, "num: %lu, ", idx);
	fprintf(stdout, "Max: %u, ", max);
	fprintf(stdout, "Average: %u\r\n", average);

	for (i = 0; i < nodeCnt; i++)
		fprintf(stdout, "%-7s ", nodeInfo[i].name);
	fprintf(stdout, "\r\n");
	for (i = 0; i < nodeCnt; i++)
		fprintf(stdout, "%-7lu ", nodeInfo[i].delta);
	fprintf(stdout, "\r\n");

	if (cur_total == 0)
		cur_total++;
	for (i = 0; i < nodeCnt; i++)
		fprintf(stdout, " %7.2f", (float)nodeInfo[i].delta*100/cur_total);
	fprintf(stdout, "\r\n\r\n");

}

static void mtop_start(void * param)
{
	mtop_read();
	mtop_post();

	while (iter == -1 || iter-- > 0) {
		sleep(delay);
		mtop_read();
		mtop_update();
		mtop_post();
		if (exit_flag == 1) {
			exit_flag = 0;
			break;
		}
	}

	vTaskDelete(NULL);
}

int cmd_mtop(int argc, char *argv[])
{
	int i;
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

	hal_mbus_pmu_enable();

	ret = xTaskCreate(mtop_start, (signed portCHAR *) "mtop",
	                  4096, NULL, HAL_THREAD_PRIORITY_HIGHEST - 1, NULL);
	if (ret != pdPASS) {
		printf("Error creating task, status was %d\r\n", ret);
		return -1;
	}

#if 0
	while(1) {
		char cRxed = 0;

        cRxed = getchar();
        if(cRxed == 'q' || cRxed == 3) {
			exit_flag = 1;
			return 0;
		}
	}
#endif

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_mtop, mtop, test bus width);

static int cmd_mtop_exit(int argc, char **argv)
{
    exit_flag = 1;
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_mtop_exit, mtop_exit, Mtop Exit);
