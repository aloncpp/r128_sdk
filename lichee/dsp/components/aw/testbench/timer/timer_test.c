#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <stdio.h>
#include <stdint.h>

#include <interrupt.h>
#include <hal_timer.h>

#define TEST_COUNT	10

static SemaphoreHandle_t g_sem = NULL;
static int g_times = 0;

static void timer_handler(void *data)
{
	BaseType_t ret;
	BaseType_t xHigherProTaskWoken = pdFALSE;
	ret = xSemaphoreGiveFromISR(g_sem, &xHigherProTaskWoken);
	if (ret == pdPASS) {
		portYIELD_FROM_ISR(xHigherProTaskWoken);
	}
}

static int timer_demo(void)
{
	uint32_t irq_num;

	hal_timer_init();

	hal_timer_set_pres(TIMER0, TMR_2_PRES);
	hal_timer_set_mode(TIMER0, TMR_CONTINUE_MODE);
	hal_timer_set_interval(TIMER0, 0x0EE0);

	irq_num = hal_timer_id_to_irq(TIMER0);
	printf("irq_num = %d\n", irq_num);

	hal_timer_irq_request(irq_num, timer_handler, 0, NULL);
	hal_timer_irq_enable(irq_num);

	return 0;
}

static void test_task(void *pdata)
{
	uint32_t irq_num = hal_timer_id_to_irq(TIMER0);

	g_sem = xSemaphoreCreateBinary();
	if (!g_sem) {
		printf("Create semaphore failed!\n");
		goto end;
	}

	if (!timer_demo()) {
		printf("Set timer test success! Begins...\n");
	} else {
		printf("Set timer test failed!\n");
		goto end;
	}

	while (1) {
		xSemaphoreTake(g_sem, portMAX_DELAY);
		printf("Ding Dong! %d times!\n", g_times);

		if (g_times == TEST_COUNT) {
			hal_timer_irq_disable(irq_num);
			hal_timer_irq_free(irq_num);

			printf("Timer test success, exiting...\n");
			break;
		}
	}

end:
	vTaskDelete(NULL);
}

void timer_test(void)
{
	uint32_t err = 0;

	err = xTaskCreate(test_task, "timer_test", 0x1000, NULL, 1, NULL);
	if (err != pdPASS) {
		printf("Create timer test task failed!\n");
		return;
	}
}
