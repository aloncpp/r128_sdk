#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <hal_cmd.h>
#include <hal_time.h>

#include <sunxi_hal_timer.h>
#include <task.h>

#if defined(CONFIG_ARCH_ARM)
#define TIMER_IN_PRINT_TEST SUNXI_TMR1
#elif defined(CONFIG_ARCH_RISCV)
#define TIMER_IN_PRINT_TEST SUNXI_TMR2
#elif defined(CONFIG_ARCH_DSP)
#define TIMER_IN_PRINT_TEST SUNXI_TMR3
#else
#define TIMER_IN_PRINT_TEST SUNXI_TMR3
#endif

void timer_irq_print_func(void *arg)
{
	static uint32_t print_cnt = 0;

	print_cnt++;

	printf("timer interrupt print\n");
	if (print_cnt == (uint32_t)arg)
	{
		printf("timer interrupt print end\n");
		hal_timer_stop(TIMER_IN_PRINT_TEST);
	}
}

#if defined(CONFIG_ARCH_ARM)
#define TEST_PRINT(fmt,...) printf("[ARM %s] "fmt, thread_name, ##__VA_ARGS__)
#elif defined(CONFIG_ARCH_RISCV)
#define TEST_PRINT(fmt,...) printf("[RISC-V %s] "fmt, thread_name, ##__VA_ARGS__)
#elif defined(CONFIG_ARCH_DSP)
#define TEST_PRINT(fmt,...) printf("[Xtensa %s] "fmt, thread_name, ##__VA_ARGS__)
#else
#define TEST_PRINT(fmt,...) printf("[unknown %s] "fmt, thread_name, ##__VA_ARGS__)
#endif

#define DEFAULT_PRINT_TEST_TIME 5
static void print_thread_entry(void *arg)
{
	const char *thread_name = pcTaskGetName(NULL);
	uint32_t task_priority = uxTaskPriorityGet(NULL);

	uint32_t test_time = (uint32_t)arg;
	if (!test_time)
	{
		TEST_PRINT("warning: test_time is 0, use default test_time\n");
		test_time = DEFAULT_PRINT_TEST_TIME;
	}

	TEST_PRINT("print test time: %us\n", test_time);
	TEST_PRINT("begin print test, task_priority: %u\n", task_priority);

	test_time *= 1000;

	TickType_t start_tick, end_tick, current_tick;
	start_tick = xTaskGetTickCount();
	end_tick = start_tick + pdMS_TO_TICKS(test_time);
	while(1)
	{
		current_tick = xTaskGetTickCount();
		if (current_tick >= end_tick)
			break;
		TEST_PRINT("main print test loop, will auto exit\n");
	}

	TEST_PRINT("print test end\n");
	vTaskDelete(NULL);
}

#define PRINT_TEST_TASK_NUM configMAX_PRIORITIES

int is_any_print_task_alive(void)
{
	TaskHandle_t handle = NULL;
	char task_name[15];
	int i;
	memset(task_name, 0, sizeof(task_name));

	for (i = 0; i < PRINT_TEST_TASK_NUM; i++)
	{
		snprintf(task_name, sizeof(task_name), "print_%d", i);
		handle = xTaskGetHandle(task_name);
		if (handle)
		{
			return 1;
		}
	}
	return 0;
}

void create_print_task(int task_num, unsigned long test_time, int is_same_priority, int priority)
{
	portBASE_TYPE ret;
	uint32_t task_thread_size = 2 * 1024 * sizeof(StackType_t);
	char task_name[15];
	int task_priority = priority;
	int i;
	const char *thread_name = pcTaskGetName(NULL);
	memset(task_name, 0, sizeof(task_name));

	for (i = 0; i < task_num; i++)
	{
		snprintf(task_name, sizeof(task_name), "print_%d", i);
		if (!is_same_priority)
			task_priority = i;

		ret = xTaskCreate(print_thread_entry, task_name, task_thread_size / sizeof(StackType_t), (void *)test_time, task_priority, NULL);
		if (ret != pdPASS)
		{
			TEST_PRINT("task '%s' create failed\n", task_name);
		}
		hal_msleep(100);
	}
}

void wait_print_test_end(void)
{
	while (1)
	{
		if (!is_any_print_task_alive())
		{
			break;
		}
		hal_msleep(500);
	}
}

int cmd_test_printf_api(int argc, char *argv[])
{
	char *ptr = NULL;
	errno = 0;
	const char *thread_name = pcTaskGetName(NULL);

	if (argc != 2)
	{
		TEST_PRINT("invalid input parameter num(%d)!\n", argc);
		return 0;
	}

	unsigned long test_time = strtoul(argv[1], &ptr, 10);
	if (errno || (ptr && *ptr != '\0'))
	{
		TEST_PRINT("invalid input parameter('%s')!\n", argv[1]);
		return 0;
	}

	vTaskSuspendAll();
	TEST_PRINT("print when scheduler is disable\n");
	xTaskResumeAll();

	taskENTER_CRITICAL();
	TEST_PRINT("print when scheduler and interrupt is disable\n");
	taskEXIT_CRITICAL();

	TEST_PRINT("begin different priority task print test\n");
	create_print_task(PRINT_TEST_TASK_NUM, test_time, 0, 0);

	wait_print_test_end();

	TEST_PRINT("begin same priority task print test\n");
	create_print_task(PRINT_TEST_TASK_NUM, test_time, 1, configMAX_PRIORITIES - 1);

#ifdef CONFIG_ARCH_RISCV
	wait_print_test_end();

	TEST_PRINT("begin interrupt and task print test\n");
	create_print_task(PRINT_TEST_TASK_NUM - 1, test_time, 0, 0);

	uint32_t irq_print_times = test_time * 1000 / 10;
	TEST_PRINT("timer interrupt print times: %d\n", irq_print_times);
	hal_timer_init(TIMER_IN_PRINT_TEST);
	hal_timer_status_t status = hal_timer_set_periodic(TIMER_IN_PRINT_TEST, 10000, timer_irq_print_func, (void *)irq_print_times);
	if (status != HAL_TIMER_STATUS_OK)
	{
		TEST_PRINT("setup timer failed, ret: %d\n", status);
	}

	wait_print_test_end();

	//multi-processor print
	TEST_PRINT("begin multi-processor print test\n");

	struct finsh_syscall* call;
	char *fork_cmd = "fork";
	call = finsh_syscall_lookup(fork_cmd);
	if(call == NULL)
	{
		TEST_PRINT("command '%s' no exist!\n", fork_cmd);
		return -1;
	}

	if(call->func == NULL)
	{
		TEST_PRINT("the entry of command '%s' no exist\n", fork_cmd);
		return -1;
	}

	//fork rpccli arm test_printf xxx
	char *call_argv[5];
	call_argv[0] = fork_cmd;
	call_argv[1] = "rpccli";
	call_argv[2] = "arm";
	call_argv[3] = "test_printf";
	call_argv[4] = argv[1];

	call->func(5, call_argv);

	call_argv[2] = "dsp";

	call->func(5, call_argv);

	create_print_task(PRINT_TEST_TASK_NUM, test_time, 0, 0);
#endif

	wait_print_test_end();
	TEST_PRINT("printf API test complete!\n");
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_printf_api, test_printf, printf API test with some possible race condition);
