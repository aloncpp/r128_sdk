#include "hw_perf.h"

static uint32_t _sram_bench_prepare(void)
{
	uint32_t bench_size = 0;
	uint32_t *buf;

	printf("1: set this test code to xip to use ICache, func@%p\n", _sram_bench_prepare);
	printf("2: set sram not in cache area\n");
	printf("3: use second test result not first!\n");

	for (int i = 16; i < 10000; i += 8) {
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

int cmd_hw_perf_sram_w(int argc, char ** argv)
{
	uint32_t bench_size = 0;
	uint32_t *buf;

	bench_size = _sram_bench_prepare();
	buf = malloc(bench_size + 8);
	if (!buf) {
		printf("test faild for malloc faild\n");
		return -1;
	}
	vTaskDelay(100);
	bench_size /= 4;
	_perf_disable_irq();
	_perf_gpio2_low();
	for (int i = 0; i < bench_size; i++)
		_perf_writel(i, buf + i);
	_perf_gpio2_high();
	_perf_enable_irq();
	vTaskDelay(20);
	printf("%s test sram:%p end\n", __func__, buf);
	free(buf);
	hal_gpio_set_data(TEST_OUT_GPIO, 1);
	hal_gpio_set_data(TEST_IRQ_GPIO, 1);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_sram_w, hwp_sramw, sram write perf);

int cmd_hw_perf_sram_ra(int argc, char ** argv)
{
	int val = 0;
	uint32_t bench_size = 0;
	uint32_t *buf;

	bench_size = _sram_bench_prepare();
	buf = malloc(bench_size + 8);
	if (!buf) {
		printf("test faild for malloc faild\n");
		return -1;
	}
	vTaskDelay(100);
	bench_size /= 4;
	_perf_disable_irq();
	_perf_gpio2_low();
	for (int i = 0; i < bench_size; i++)
		val += _perf_readl(buf + i);
	_perf_gpio2_high();
	_perf_enable_irq();
	vTaskDelay(20);
	printf("%s test sram:%p end\n", __func__, buf);
	free(buf);
	hal_gpio_set_data(TEST_OUT_GPIO, 1);
	hal_gpio_set_data(TEST_IRQ_GPIO, 1);

	return val;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_sram_ra, hwp_sramr, sram readadd perf);

int cmd_hw_perf_sram_rw(int argc, char ** argv)
{
	uint32_t bench_size = 0;
	uint32_t *buf;

	bench_size = _sram_bench_prepare();
	buf = malloc(bench_size + 8);
	if (!buf) {
		printf("test faild for malloc faild\n");
		return -1;
	}
	vTaskDelay(100);
	bench_size /= 4;
	_perf_disable_irq();
	_perf_gpio2_low();
	for (int i = 0; i < bench_size; i++)
		_perf_writel(_perf_readl(buf + i), buf + i + 1);
	_perf_gpio2_high();
	_perf_enable_irq();
	vTaskDelay(20);
	printf("%s test sram:%p end\n", __func__, buf);
	free(buf);
	hal_gpio_set_data(TEST_OUT_GPIO, 1);
	hal_gpio_set_data(TEST_IRQ_GPIO, 1);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_sram_rw, hwp_sramrw, sram readwrite perf);
