#include <private/mpu_wrappers.h>
#include "./hw_perf.h"

#define MEASURE_BY_GPIO 0
#define MEASURE_BY_TIMER 1

#define HWP_R 1
#define HWP_W 2
#define HWP_RW 3

static uint32_t _sram_bench_prepare(void)
{
	uint32_t bench_size = 0;
	uint32_t *buf;
	int i = 0;

	printf("1: set this test code to xip to use ICache, func@%p\n", _sram_bench_prepare);
	printf("2: set sram not in cache area\n");
	printf("3: use second test result not first!\n");

	for (i = 16; i < 10000; i += 8) {
		bench_size = 1024 * i;
		buf = malloc(bench_size);
		if (!buf) {
			bench_size = 1024 * (i - 8);
			printf("test %dKB size\n", bench_size / 1024);
			break;
		}
		free(buf);
	}

	return bench_size;
}

int cmd_hw_perf_sram_common(int argc, char ** argv)
{
	uint32_t bench_size = 0;
	uint32_t u32_bench_size = 0;
	uint32_t *buf;
	uint32_t malloc_flag = 0;
	uint32_t test_case = 0;
	int val = 0, test_cnt = 0, i = 0;
#ifdef MEASURE_BY_TIMER
	float time_use;
	//float throuth_mb;
#endif

	printf("%s@%p\n", argv[0], cmd_hw_perf_sram_common);
	if (strcmp(argv[0], "hwp_sramr") == 0)
		test_case = HWP_R;
	else if (strcmp(argv[0], "hwp_sramw") == 0)
		test_case = HWP_W;
	else if (strcmp(argv[0], "hwp_sramrw") == 0)
		test_case = HWP_RW;

	if (argc >= 3) {
		buf = hw_perf_parse_val(argv[1]);
		bench_size = hw_perf_parse_val(argv[2]);
	} else {
		bench_size = _sram_bench_prepare();
		buf = malloc(bench_size + 8);
		if (!buf) {
			printf("test faild for malloc faild\n");
			return -1;
		}
		malloc_flag = 1;
	}

	printf("test:%d bench buf:0x%p size:%d\n", test_case, buf, bench_size);
	vTaskDelay(100);
	u32_bench_size = bench_size / 4;
	//_perf_disable_irq();
#ifdef MEASURE_BY_TIMER
	time_perf_init();
#endif
	for (test_cnt = 0; test_cnt < 3; test_cnt++) {
#ifdef MEASURE_BY_GPIO
		_perf_gpio2_low();
#endif
#ifdef MEASURE_BY_TIMER
		time_perf_tag();
#endif
		if (test_case == HWP_R) {
			for (i = 0; i < u32_bench_size; i++)
				val = _perf_readl(buf + i);
		} else if (test_case == HWP_W) {
			for (i = 0; i < u32_bench_size; i++)
				_perf_writel(i, buf + i);
		} else if (test_case == HWP_RW) {
			for (i = 0; i < u32_bench_size; i++)
				_perf_writel(_perf_readl(buf + i), buf + i + 1);
		} else {
			printf("test case not valid!\n");
		}
#ifdef MEASURE_BY_TIMER
		time_use = (float)time_perf_tag() / 1000.0;
#endif
#ifdef MEASURE_BY_GPIO
		_perf_gpio2_high();
#endif
#ifdef MEASURE_BY_TIMER
		time_perf_result(bench_size, time_use);
#endif
	}
#ifdef MEASURE_BY_TIMER
	time_perf_deinit();
#endif
	//_perf_enable_irq();
	vTaskDelay(20);
	if (malloc_flag)
		free(buf);
#ifdef MEASURE_BY_GPIO
	hal_gpio_set_data(TEST_OUT_GPIO, 1);
	hal_gpio_set_data(TEST_IRQ_GPIO, 1);
#endif

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_sram_common, hwp_sramw, sram write perf);
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_sram_common, hwp_sramr, sram readadd perf);
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_sram_common, hwp_sramrw, sram readwrite perf);
