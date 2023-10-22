#include "hw_perf.h"

#ifdef CONFIG_XIP
extern char __XIP_Base[];
extern char __XIP_End[];

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

int cmd_hw_perf_xip_ra(int argc, char ** argv)
{
	int val = 0;
	uint32_t bench_size = 0;
	uint32_t *buf;

	bench_size = _PERF_XIP_TEST_SIZE;
	buf = __XIP_End;

	vTaskDelay(100);
	bench_size /= 4;
	_perf_disable_irq();
	_perf_gpio2_low();
	for (int i = 0; i < bench_size; i++)
		val += _perf_readl(buf + i);
	_perf_gpio2_high();
	_perf_enable_irq();
	vTaskDelay(20);
	printf("%s test xip:%p end\n", __func__, buf);
	hal_gpio_set_data(TEST_OUT_GPIO, 1);
	hal_gpio_set_data(TEST_IRQ_GPIO, 1);

	return val;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_xip_ra, hwp_xipr, xip readadd perf);

static void cmd_flash_bench_task(void *arg)
{
	int32_t err = 0;
	int *argv = arg;
	uint32_t mode, start_addr, loop;
	uint32_t cnt, erase_kb = 4;
	float throuth_mb;
	float time_use;
	FlashEraseMode size_type = FLASH_ERASE_4KB;

	mode = (uint32_t)argv[1];
	start_addr = (uint32_t)argv[2];
	loop = (uint32_t)argv[3];
	printf("mode=%x addr=%x loop=%x\n", mode, start_addr, loop);
	printf("set this test code to SRAM, func@%p\n", cmd_flash_bench_task);

	time_perf_init();

	printf("\n");
	for (int _l = 0; _l < loop; _l++) {
		for (int i = 0; i < 20; i++) {
			int j;
			uint32_t bench_size = 1024 * (1 << i);
			uint32_t *buf = malloc(bench_size);

			if (!buf) {
				break;
			}

			for (j = 0; j < bench_size / 4; j++)
				buf[j] = j;

			if (mode & 0x1) {
				cnt = 1;
				if (bench_size <= 4*1024) {
					size_type = FLASH_ERASE_4KB;
					erase_kb = 4;
				} else if (bench_size <= 32*1024) {
					size_type = FLASH_ERASE_32KB;
					erase_kb = 32;
				} else {
					size_type = FLASH_ERASE_64KB;
					cnt = DIV_ROUND_UP(bench_size, 64*1024);
					erase_kb = 64 * cnt;
				}

				time_perf_tag();
				err = HAL_Flash_Erase(_PERF_MFLASH, size_type, start_addr, cnt);
				time_use = (float)time_perf_tag() / 1000.0;
				if (time_use < 0.001)
					time_use = 0.001;
				if (err) {
					printf("erase err!\n");
					goto next;
				} else {
					throuth_mb = erase_kb * 1024 / time_use / 1000;
					printf("%s erase ok, %3d", __func__, erase_kb);
					printf(" KB use:%3.3f ms, throughput:%2.3f MB/S\n",
						   time_use, throuth_mb);
				}
			}

			if (mode & 0x2) {
				time_perf_tag();
				err = HAL_Flash_Write(_PERF_MFLASH, start_addr,
				                      (const unsigned char *)buf, bench_size);
				time_use = (float)time_perf_tag() / 1000.0;
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

			if (mode & 0x4) {
				for (j = 0; j < bench_size / 4; j++)
					buf[j] = 0;

				time_perf_tag();
				err = HAL_Flash_Read(_PERF_MFLASH, start_addr, (uint8_t *)buf,
				                     bench_size);
				time_use = (float)time_perf_tag() / 1000.0;
				if (time_use < 0.001)
					time_use = 0.001;
				if (err) {
					printf("read err!\n");
					goto next;
				} else {
					throuth_mb = bench_size * 1000 / 1024 / time_use / 1000;
				}

				err = 0;
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
					print_hex_dump_bytes((const void *)&buf[j], 256);
					goto next;
				}
				printf("%s read ok,  %3d", __func__, bench_size / 1024);
				printf(" KB use:%3.3f ms, throughput:%2.3f MB/S\n",
				       time_use, throuth_mb);
			}

next:
			free(buf);
			if (err)
				break;
		}
	}

out:
	printf("%s test end\n", __func__);
	time_perf_deinit();

	free(arg);
	vTaskDelete(NULL);
}

#define CMD_ARG_LEN  64

/* flash bench m=<mode> <a=0xaddr> n=<loop_num>
 *  <mode>: 1: erase, 2: write, 4: read, other is add
 * flash bench m=0x7 a=0x100000 n=1
 */
static int cmd_flash_bench_exec(int argc, char ** argv)
{
	TaskHandle_t thread;
	int *param;

	if (argc < 4) {
		printf("invalid argc:%d\n", argc);
		return -1;
	}
	param = malloc(CMD_ARG_LEN);
	if (!param)
		return -1;

	param[0] = argc;
	sscanf(argv[1], "m=0x%x", &param[1]);
	sscanf(argv[2], "a=0x%x", &param[2]);
	sscanf(argv[3], "n=%d", &param[3]);

	if (xTaskCreate(cmd_flash_bench_task,
	                    "flash_bench",
	                    16 * 1024 / sizeof(StackType_t),
	                    param,
	                    configMAX_PRIORITIES / 2,
	                    &thread) != pdPASS) {
		printf("create flash bench test task failed\n");
		return -1;
	}

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_flash_bench_exec, hwp_flash_bench, flash throughput perf);

#endif
