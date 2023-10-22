#include "hw_perf.h"
#include <task.h>

#define __PSRAM_End 0x08000000
#define __PSRAM_Top 0x083fffff

/* #define CONFIG_HEAP_IN_LSPSRAM */
#ifdef CONFIG_HEAP_IN_LSPSRAM
#define psram_malloc malloc
#define psram_free free
#else
#define psram_malloc(sz) ((__PSRAM_End + sz) <= __PSRAM_Top ? __PSRAM_End : NULL)
#define psram_free
#endif

typedef int (*psram_read_write)(uint32_t write, uint32_t addr, uint8_t *buf, uint32_t len, void *arg);

/*
static int psram_dbus_dma_read_write(uint32_t write, uint32_t addr,
                                     uint8_t *buf, uint32_t len, void *arg)
{
	struct dma_slave_config config = {0};
	struct sunxi_dma_chan *hdma;
	uint32_t size = 0;
	int ret = -1;

	if (write)
		hal_dcache_clean((unsigned long)buf, len);

	ret = hal_dma_chan_request(&hdma);
	if (ret == HAL_DMA_CHAN_STATUS_BUSY) {
		printf("dma channel busy!");
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
		printf("dma config error, ret:%d", ret);
		return -1;
	}

	if (write)
		ret = hal_dma_prep_memcpy(hdma, (uint32_t)addr, (uint32_t)buf, len);
	else
		ret = hal_dma_prep_memcpy(hdma, (uint32_t)buf, (uint32_t)addr, len);

	if (ret != HAL_DMA_STATUS_OK) {
		printf("dma prepare error, ret:%d", ret);
		return -1;
	}

	ret = hal_dma_start(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		printf("dma start error, ret:%d", ret);
		return -1;
	}

	while (hal_dma_tx_status(hdma, &size)!= 0);

	ret = hal_dma_stop(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		printf("dma stop error, ret:%d", ret);
		return -1;
	}

	ret = hal_dma_chan_free(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		printf("dma free error, ret:%d", ret);
		return -1;
	}

	if (!write)
		hal_dcache_invalidate((unsigned long)buf, len);

	return 0;
}
*/
static int psram_dbus_cpu_read_write(uint32_t write, uint32_t addr,
                                     uint8_t *buf, uint32_t len, void *arg)
{
	if (write)
		memcpy((void *)addr, buf, len);
	else
		memcpy(buf, (void *)addr, len);

	return 0;
}

psram_read_write psram_rw_op[] = {
	psram_dbus_cpu_read_write,
	//psram_dbus_dma_read_write,
};

static void cmd_psram_bench_task(void *arg)
{
	int32_t err = 0;
	int *argv = arg;
	uint32_t mode, start_addr, type, loop;
	float throuth_mb;
	float time_use;
	int _l = 0, i = 0;

	mode = (uint32_t)argv[1];
	type = (uint32_t)argv[2];
	loop = (uint32_t)argv[3];
	printf("mode=%x type=%x loop=%x\n", mode, type, loop);

	time_perf_init();

	printf("\n");
	for (_l = 0; _l < loop; _l++) {
		for (i = 0; i < 20; i++) {
			int j;
			uint32_t bench_size = 1024 * (1 << i);
			uint32_t *buf = malloc(bench_size);
			if (!buf) {
				printf("%s test end for malloc buff failed.\n", __func__);
				break;
			}

			start_addr = (uint32_t)psram_malloc(bench_size);
			if (!start_addr) {
				printf("%s malloc start addr failed.\n", __func__);
				free(buf);
				break;
			}

			printf("%s start_addr:0x%0x\n", __func__, start_addr);
			for (j = 0; j < bench_size / 4; j++)
				buf[j] = j;

			if (type & 0x2) {
				//HAL_Dcache_DumpMissHit();
				//HAL_Dcache_CleanAll();
				time_perf_tag();
				err = psram_rw_op[mode](1, start_addr, (uint8_t *)buf,
				                        bench_size, NULL);
				//HAL_Dcache_CleanAll();
				time_use = (float)time_perf_tag() / 1000.0;
				//HAL_Dcache_DumpMissHit();
				if (time_use < 0.001)
					time_use = 0.001;
				if (err) {
					printf("write err!\n");
					goto next;
				} else {
					throuth_mb = bench_size * 1000 / 1024 / time_use / 1000;
					printf("%s write ok, %3d", __func__, bench_size / 1024);
					printf(" KB use:%3.3f ms, throughput:%2.3f MB/S\n",
					       time_use, throuth_mb);
				}
			}

			if (type & 0x4) {
				for (j = 0; j < bench_size / 4; j++)
					buf[j] = 0;

				//HAL_Dcache_DumpMissHit();
				//HAL_Dcache_CleanAll();
				time_perf_tag();
				err = psram_rw_op[mode](0, start_addr, (uint8_t *)buf,
				                        bench_size, NULL);
				//HAL_Dcache_CleanAll();
				time_use = (float)time_perf_tag() / 1000.0;
				//HAL_Dcache_DumpMissHit();
				if (time_use < 0.001)
					time_use = 0.001;
				if (err) {
					printf("read err!\n");
					goto next;
				} else {
					throuth_mb = bench_size * 1000 / 1024 / time_use / 1000;
				}

				err = 0;
				hal_dcache_clean((unsigned long)buf, bench_size);

				hal_dcache_invalidate((unsigned long)buf, bench_size);

				for (j = 0; j < bench_size / 4; j++) {
					if (buf[j] != j) {
						err = -1;
						break;
					}
				}
				if (err) {
					printf("bench_size:%d write data err:0x%x should:0x%x,"
					       " idx:%d!\n", bench_size, buf[j], j, j);
					j = j > 16 ? j - 16 : 0;
					/* print_hex_dump_words((const void *)&buf[j], 256); */
					goto next;
				}
				printf("%s read ok,  %3d", __func__, bench_size / 1024);
				printf(" KB use:%3.3f ms, throughput:%2.3f MB/S\n",
				       time_use, throuth_mb);
			}

next:
			free(buf);
			psram_free((void *)start_addr);
			if (err)
				break;
		}
	}

	printf("%s test end\n", __func__);
	time_perf_deinit();

	free(arg);
	vTaskDelete(NULL);
}

#define CMD_ARG_LEN  64

/* psram bench <m=0/1> <t=2/4/6> <n=num>
 * m: 0:DBUS CPU, 1:DBUS DMA
 * t: 2: write, 4: read, 6:write+read
 * n:num
 * psram bench m=0 t=0x6 n=1
 */
static int cmd_psram_bench_exec(int argc, char ** argv)
{
	TaskHandle_t thread;
	int *param;

	if (argc < 4) {
		printf("invalid argc:%d\n", argc);
		return -1;
	}
#ifdef CONFIG_ARCH_HAVE_DCACHE
	printf("should close CACHE for test accurately!\n");
#endif
	param = malloc(CMD_ARG_LEN);
	if (!param)
		return -1;

	param[0] = argc;
	sscanf(argv[1], "m=%d", &param[1]);
	sscanf(argv[2], "t=0x%x", &param[2]);
	sscanf(argv[3], "n=%d", &param[3]);

	if (xTaskCreate(cmd_psram_bench_task,
	                    "psram_bench",
	                    8 * 1024 / sizeof(StackType_t),
	                    param,
	                    configMAX_PRIORITIES / 2,
	                    &thread) != pdPASS) {
		printf("create psram bench test task failed\n");
		return -1;
	}

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_psram_bench_exec, hwp_psram_bench, psram throughput perf);
