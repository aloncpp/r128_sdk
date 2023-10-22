#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <stdio.h>

#include <hal_timer.h>

#define TIMER_NUM	1
#define TEST_COUNT	10

static SemaphoreHandle_t g_sem = NULL;
static int g_times = 0;

static void timer_handler(void *arg)
{
	BaseType_t ret;
	BaseType_t xHigherProTaskWoken = pdFALSE;

	ret = xSemaphoreGiveFromISR(g_sem, &xHigherProTaskWoken);
	if (ret == pdPASS) {
		portYIELD_FROM_ISR(xHigherProTaskWoken);
	}

	if (++g_times < TEST_COUNT)
		hal_arch_timer_update(TIMER_NUM, 1000 * g_times);
	else
		hal_arch_timer_update(TIMER_NUM, 0);
}

static int timer_test(void)
{
	if (hal_arch_timer_init(TIMER_NUM, timer_handler, NULL) < 0) {
		printf("Initialize arch timer%d failed!\n", TIMER_NUM);
		return -1;
	}

	if (hal_arch_timer_start(TIMER_NUM, 5000) < 0) {
		printf("Start arch timer%d failed!\n", TIMER_NUM);
		return -1;
	}

	return 0;
}

static void test_task(void *pdata)
{
	g_sem = xSemaphoreCreateBinary();
	if (!g_sem) {
		printf("Create semaphore failed!\n");
		goto end;
	}

	if (!timer_test()) {
		printf("Set arch timer test success! Begins after 5s...\n");
	} else {
		printf("Set arch timer test failed!\n");
		goto end;
	}

	while (1) {
		xSemaphoreTake(g_sem, portMAX_DELAY);
		printf("Ding Dong! %ds passed!\n", g_times);

		if (g_times == TEST_COUNT) {
			printf("Timer test success, exiting...\n");
			break;
		}
	}

end:
	vTaskDelete(NULL);
}

void arch_timer_test(void)
{
	uint32_t err = 0;

	err = xTaskCreate(test_task, "arch_timer", 0x1000, NULL, 1, NULL);
	if (err != pdPASS) {
		printf("Create arch timer test task failed!\n");
		return;
	}
}
