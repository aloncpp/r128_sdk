#include "hw_perf.h"

static int _init_flg;

static void _gpio_irq_func(void);

#ifdef CONFIG_ARCH_ARM_ARMV8M

void _perf_set_irqhandler(uint32_t IRQn, hal_irq_handler_t handler)
{
    uint32_t *vectors = (uint32_t *)SCB->VTOR;

    vectors[IRQn + 16] = (uint32_t)handler;

    NVIC_EnableIRQ(IRQn);
}

#elif defined CONFIG_ARCH_RISCV

void _perf_set_irqhandler(uint32_t IRQn, hal_irq_handler_t handler)
{
    hal_request_irq(IRQn, handler, NULL, NULL);
}

#elif defined CONFIG_ARCH_XTENSA

#endif

void _timer_test_cb(void *arg)
{
	printf("%s,%d @%p\n", __func__, __LINE__, _timer_test_cb);
}

static void _dma_test_cb(void *param)
{
	printf("DMA finished @%p\n", _dma_test_cb);
}

int cmd_hw_perf_gpio_init(int argc, char ** argv)
{
	uint32_t irq_num;

	if (!_init_flg) {
		hal_gpio_set_pull(TEST_OUT_GPIO, GPIO_PULL_UP);
		hal_gpio_set_direction(TEST_OUT_GPIO, GPIO_DIRECTION_OUTPUT);
		hal_gpio_set_driving_level(TEST_OUT_GPIO, GPIO_DRIVING_LEVEL1);
		hal_gpio_set_data(TEST_OUT_GPIO, 1);

		hal_gpio_set_pull(TEST_IRQ_GPIO, GPIO_PULL_UP);
		hal_gpio_set_direction(TEST_IRQ_GPIO, GPIO_DIRECTION_OUTPUT);
		hal_gpio_set_driving_level(TEST_IRQ_GPIO, GPIO_DRIVING_LEVEL1);
		hal_gpio_set_data(TEST_IRQ_GPIO, 1);

		_perf_set_irqhandler(GPIOA_IRQn, _gpio_irq_func);
		_init_flg = 1;
	}

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_gpio_init, hwp_init, hardware perf init gpio);

int cmd_hw_perf_irq(int argc, char ** argv)
{
	_perf_set_irqhandler(GPIOA_IRQn, _gpio_irq_func);
	printf("set _gpio_irq_func to SRAM/XIP/PSRAM/... to test CPU get and exe"
	       " first instruction time with Cache Enable(Hit/Miss)/Disable\n");
	printf("%s,%d @%p\n", __func__, __LINE__, cmd_hw_perf_irq);
	vTaskDelay(100);
	_perf_disable_irq();
#ifdef CONFIG_ARCH_ARM_ARMV8M
	NVIC_SetPendingIRQ(GPIOA_IRQn);
#endif
	_perf_gpio1_high();
	_perf_enable_irq();
	vTaskDelay(20);
	hal_gpio_set_data(TEST_OUT_GPIO, 1);
	vTaskDelay(20);
	hal_gpio_set_data(TEST_IRQ_GPIO, 1);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_irq, hwp_irq, gpio irq perf);

/* gpio apb */
int cmd_hw_perf_gpio_w(int argc, char ** argv)
{
	vTaskDelay(100);
	_perf_disable_irq();
	_perf_gpio2_low();
	for (int i = 0; i < PERF_TEST_NUM; i++)
		_perf_gpio1_high();
	_perf_gpio2_high();
	_perf_enable_irq();
	vTaskDelay(20);
	hal_gpio_set_data(TEST_OUT_GPIO, 1);
	vTaskDelay(20);
	hal_gpio_set_data(TEST_IRQ_GPIO, 1);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_gpio_w, hwp_gpiow, gpio wrtie perf);

int cmd_hw_perf_gpio_ra(int argc, char ** argv)
{
	int val = 0;
	vTaskDelay(100);
	_perf_disable_irq();
	_perf_gpio2_low();
	for (int i = 0; i < PERF_TEST_NUM; i++)
		val += _perf_readl(_PERF_GPIO_DATA_REG);
	_perf_gpio2_high();
	_perf_enable_irq();
	vTaskDelay(20);
	hal_gpio_set_data(TEST_OUT_GPIO, 1);
	vTaskDelay(20);
	hal_gpio_set_data(TEST_IRQ_GPIO, 1);

	return val;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_gpio_ra, hwp_gpior, gpio readadd perf);

int cmd_hw_perf_gpio_rw(int argc, char ** argv)
{
	int val = 0;
	vTaskDelay(100);
	_perf_disable_irq();
	_perf_gpio2_low();
	for (int i = 0; i < PERF_TEST_NUM; i++)
		_perf_writel(_perf_readl(_PERF_GPIO_DATA_REG), _PERF_GPIO_DATA_REGN);
	_perf_gpio2_high();
	_perf_enable_irq();
	vTaskDelay(20);
	hal_gpio_set_data(TEST_OUT_GPIO, 1);
	vTaskDelay(20);
	hal_gpio_set_data(TEST_IRQ_GPIO, 1);

	return val;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_gpio_rw, hwp_gpiorw, gpio readwrite perf);

#ifdef CONFIG_DRIVERS_TIMER

/* timer apb */
int cmd_hw_perf_timer_w(int argc, char ** argv)
{
	sunxi_timer_init(SUNXI_TMR0);
	vTaskDelay(50);
	_perf_disable_irq();
	_perf_gpio2_low();
	for (int i = 0; i < PERF_TEST_NUM; i++)
		_perf_writel(_PERF_TIMER_INTVAL_VALUE_T, _PERF_TIMER_INTVAL_REG(SUNXI_TMR0));
	_perf_gpio2_high();
	_perf_enable_irq();
	vTaskDelay(20);
	sunxi_timer_uninit(SUNXI_TMR0);
	hal_gpio_set_data(TEST_OUT_GPIO, 1);
	vTaskDelay(20);
	hal_gpio_set_data(TEST_IRQ_GPIO, 1);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_timer_w, hwp_timerw, timer wrtie perf);

int cmd_hw_perf_timer_ra(int argc, char ** argv)
{
	int val;
	sunxi_timer_init(SUNXI_TMR0);
	sunxi_timer_set_oneshot(_PERF_TIMER_INTVAL_VALUE, SUNXI_TMR0, _timer_test_cb, NULL);
	vTaskDelay(50);
	_perf_disable_irq();
	_perf_gpio2_low();
	for (int i = 0; i < PERF_TEST_NUM; i++)
		val = _perf_readl(_PERF_TIMER_INTVAL_REG(SUNXI_TMR0));
	_perf_gpio2_high();
	_perf_enable_irq();
	vTaskDelay(20);
	sunxi_timer_uninit(SUNXI_TMR0);
	hal_gpio_set_data(TEST_OUT_GPIO, 1);
	vTaskDelay(20);
	hal_gpio_set_data(TEST_IRQ_GPIO, 1);

	return val;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_timer_ra, hwp_timerr, timer read perf);

int cmd_hw_perf_timer_rw(int argc, char ** argv)
{
	int val = 0;
	sunxi_timer_init(SUNXI_TMR0);
	sunxi_timer_set_oneshot(_PERF_TIMER_INTVAL_VALUE, SUNXI_TMR0, _timer_test_cb, NULL);
	vTaskDelay(50);
	_perf_disable_irq();
	_perf_gpio2_low();
	for (int i = 0; i < PERF_TEST_NUM; i++)
		_perf_writel(_perf_readl(_PERF_TIMER_CNTVAL_REG(SUNXI_TMR0)), _PERF_TIMER_INTVAL_REG(SUNXI_TMR0));
	_perf_gpio2_high();
	_perf_enable_irq();
	vTaskDelay(20);
	sunxi_timer_uninit(SUNXI_TMR0);
	hal_gpio_set_data(TEST_OUT_GPIO, 1);
	vTaskDelay(20);
	hal_gpio_set_data(TEST_IRQ_GPIO, 1);

	return val;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_timer_rw, hwp_timerrw, timer readwrite perf);
#endif

#ifdef CONFIG_DRIVERS_DMA
/* dma ahb */
int cmd_hw_perf_dma_w(int argc, char ** argv)
{
	int ret;
	struct sunxi_dma_chan *hdma = NULL;
	struct dma_slave_config config = {0};
	ret = hal_dma_chan_request(&hdma);
	if (ret == HAL_DMA_CHAN_STATUS_BUSY) {
		printf("dma channel busy!");
		return -1;
	}
	if (hdma->chan_count != _PERF_DMA_TEST_CHANNEL) {
		printf("dma channel:%d err!", hdma->chan_count);
		hal_dma_chan_free(hdma);
		return -1;
	}

	vTaskDelay(50);
	_perf_disable_irq();
	_perf_gpio2_low();
	for (int i = 0; i < PERF_TEST_NUM; i++)
		_perf_writel(_PERF_DMA_INTVAL_DST_T, _PERF_DMA_DST_REG);
	_perf_gpio2_high();
	_perf_enable_irq();
	vTaskDelay(20);
	hal_dma_chan_free(hdma);
	hal_gpio_set_data(TEST_OUT_GPIO, 1);
	vTaskDelay(20);
	hal_gpio_set_data(TEST_IRQ_GPIO, 1);

	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_dma_w, hwp_dmaw, dma write perf);

int cmd_hw_perf_dma_ra(int argc, char ** argv)
{
	int val;
	int ret;
	struct sunxi_dma_chan *hdma = NULL;
	struct dma_slave_config config = {0};
	ret = hal_dma_chan_request(&hdma);
	if (ret == HAL_DMA_CHAN_STATUS_BUSY) {
		printf("dma channel busy!");
		return -1;
	}
	if (hdma->chan_count != _PERF_DMA_TEST_CHANNEL) {
		printf("dma channel:%d err!", hdma->chan_count);
		hal_dma_chan_free(hdma);
		return -1;
	}

	vTaskDelay(50);
	_perf_disable_irq();
	_perf_gpio2_low();
	for (int i = 0; i < PERF_TEST_NUM; i++)
		val = _perf_readl(_PERF_DMA_DST_REG);
	_perf_gpio2_high();
	_perf_enable_irq();
	vTaskDelay(20);
	hal_dma_chan_free(hdma);
	hal_gpio_set_data(TEST_OUT_GPIO, 1);
	vTaskDelay(20);
	hal_gpio_set_data(TEST_IRQ_GPIO, 1);

	return val;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_dma_ra, hwp_dmara, dma readadd perf);

int cmd_hw_perf_dma_rw(int argc, char ** argv)
{
	int val = 0;
	int ret;
	struct sunxi_dma_chan *hdma = NULL;
	struct dma_slave_config config = {0};
	ret = hal_dma_chan_request(&hdma);
	if (ret == HAL_DMA_CHAN_STATUS_BUSY) {
		printf("dma channel busy!");
		return -1;
	}
	if (hdma->chan_count != _PERF_DMA_TEST_CHANNEL) {
		printf("dma channel:%d err!", hdma->chan_count);
		hal_dma_chan_free(hdma);
		return -1;
	}

	vTaskDelay(50);
	_perf_disable_irq();
	_perf_gpio2_low();
	for (int i = 0; i < PERF_TEST_NUM; i++)
		_perf_writel(_perf_readl(_PERF_DMA_SRC_REG), _PERF_DMA_DST_REG);
	_perf_gpio2_high();
	_perf_enable_irq();
	vTaskDelay(20);
	hal_dma_chan_free(hdma);
	hal_gpio_set_data(TEST_OUT_GPIO, 1);
	vTaskDelay(20);
	hal_gpio_set_data(TEST_IRQ_GPIO, 1);

	return val;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_hw_perf_dma_rw, hwp_dmarw, dma readwrite perf);
#endif

static void _gpio_irq_func(void)
{
	_perf_gpio2_low();

	printf("%s,%d @%p\n", __func__, __LINE__, _gpio_irq_func);
}

