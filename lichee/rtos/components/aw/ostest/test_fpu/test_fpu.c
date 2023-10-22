#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <hal_cmd.h>
#include <hal_time.h>
#include <awlog.h>

#include <fenv.h>

static int exit_flag = 0;
extern void set_fpu_register_test(void *src, void *out, int count, int reserved);

#include "csr.h"

#define FPU_TEST_PRINT(fmt,...) printf("[%s] "fmt, thread_name, ##__VA_ARGS__)

void fpu_test_thread_sleep(void)
{
    const char *thread_name = pcTaskGetName(NULL);

    int time = rand()%100 + 1;
    FPU_TEST_PRINT("sleep %d(ms)\r\n", time);
    hal_msleep(time);
}

typedef struct
{
    unsigned long busy_waiting_count;
    int rounding_mode;
} fpu_test_para_t;

static uint32_t s_fpu_test_thread_init_integer = 0;
static void run_fpu_test(void *param)
{
    const char *thread_name = pcTaskGetName(NULL);
    FPU_TEST_PRINT("-------now begin test---------\n");

    uint32_t integer_part = s_fpu_test_thread_init_integer;
    s_fpu_test_thread_init_integer++;

    int success_cnt = 0;
    fpu_test_para_t *test_para = (fpu_test_para_t *)param;

    unsigned long busy_waiting_count = test_para->busy_waiting_count;
    FPU_TEST_PRINT("busy_waiting_count=%d, target rounding mode: %d, sizeof(float)=%d, sizeof(double)=%d, init_interger_part=%u\n",
        busy_waiting_count, test_para->rounding_mode, sizeof(float), sizeof(double), integer_part);

#ifdef CONFIG_ARCH_RISCV
    uint64_t fcsr_reg;
    fcsr_reg = csr_read(0x003);
    FPU_TEST_PRINT("fcsr_reg: 0x%lx, rounding mode field: %u\n", fcsr_reg, (fcsr_reg >> 5) & 0x7);
#endif

    int ret = fesetround(test_para->rounding_mode);
    if (ret)
    {
        FPU_TEST_PRINT("Warning: set rounding mode failed, ret: %d\n", ret);
    }

    int rounding_mode_before_test, rounding_mode_after_test;
    rounding_mode_before_test = fegetround();
    rounding_mode_after_test = 0;

    FPU_TEST_PRINT("current rounding mode: %d\n", rounding_mode_before_test);

    while (1)
    {
        int i = 0;
        int j = 0;

#ifdef CONFIG_ARCH_RISCV
        uint32_t test[32] = {0};
        float test_float[32];
#else
        uint32_t test[16] = {0};
        float test_float[16];
#endif

        /* Base on IEEE754 floating unit, the binary value of 1.0f is 0x3F800000,
         * maybe you can get it from assembly code!
         * */
        test_float[0] = integer_part + 0.0f;
        test_float[1] = integer_part + 1.0f;
        test_float[2] = integer_part + 2.0f;
        test_float[3] = integer_part + 3.0f;
        test_float[4] = integer_part + 4.0f;
        test_float[5] = integer_part + 5.0f;
        test_float[6] = integer_part + 6.0f;
        test_float[7] = integer_part + 7.0f;
        test_float[8] = integer_part + 8.0f;
        test_float[9] = integer_part + 9.0f;
        test_float[10] = integer_part + 10.0f;
        test_float[11] = integer_part + 11.0f;
        test_float[12] = integer_part + 12.0f;
        test_float[13] = integer_part + 13.0f;
        test_float[14] = integer_part + 14.0f;
        test_float[15] = integer_part + 15.0f;
#ifdef CONFIG_ARCH_RISCV
        test_float[16] = integer_part + 16.0f;
        test_float[17] = integer_part + 17.0f;
        test_float[18] = integer_part + 18.0f;
        test_float[19] = integer_part + 19.0f;
        test_float[20] = integer_part + 20.0f;
        test_float[21] = integer_part + 21.0f;
        test_float[22] = integer_part + 22.0f;
        test_float[23] = integer_part + 23.0f;
        test_float[24] = integer_part + 24.0f;
        test_float[25] = integer_part + 25.0f;
        test_float[26] = integer_part + 26.0f;
        test_float[27] = integer_part + 27.0f;
        test_float[28] = integer_part + 28.0f;
        test_float[29] = integer_part + 29.0f;
        test_float[30] = integer_part + 30.0f;
        test_float[31] = integer_part + 31.0f;
#endif
        if (exit_flag)
        {
            goto exit;
        }

#ifdef CONFIG_ARCH_RISCV
        set_fpu_register_test(&test_float, &test, busy_waiting_count, 0);
#else
        set_fpu_register_test(0, &test, busy_waiting_count, 0);
#endif
        rounding_mode_after_test = fegetround();

        integer_part++;

        if (rounding_mode_after_test != rounding_mode_before_test)
        {
            FPU_TEST_PRINT("FPU rounding mode test error!!! integer_part: %u\n", integer_part);
            FPU_TEST_PRINT("rounding_mode_before_test: %d, rounding_mode_after_test: %d\n", rounding_mode_before_test, rounding_mode_after_test);
#ifdef CONFIG_ARCH_RISCV
            fcsr_reg = csr_read(0x003);
            FPU_TEST_PRINT("fcsr_reg: 0x%lx, rm field: %u\n", fcsr_reg, (fcsr_reg >> 5) & 0x7);
#endif
            goto exit;
        }

        for (i = 0; i < sizeof(test) / sizeof(test[0]); i++)
        {
            if (memcmp(&test[i], &test_float[i], sizeof(test[i])))
            {
                FPU_TEST_PRINT("FPU register %d data test error!!! integer_part: %u\n", i, integer_part);
                for (j = 0; j < sizeof(test) / sizeof(test[0]); j++)
                {
                    printf("current FPU register %d data:%f\n", j, (float)test[j]);
                }
                for (j = 0; j < sizeof(test_float) / sizeof(test_float[0]); j++)
                {
                    printf("origin FPU register %d data:%f\n", j, (float)test_float[j]);
                }
                printf("\ncurrent:\n");
                aw_hexdump((void *)test, sizeof(test));
                printf("\norigin:\n");
                aw_hexdump((void *)test_float, sizeof(test_float));
                goto exit;
            }
        }
        FPU_TEST_PRINT("success_cnt: %d\n", success_cnt++);
    }
exit:
    vTaskDelete(NULL);
}

static fpu_test_para_t s_test_para1, s_test_para2;

int cmd_test_fpu(int argc, char **argv)
{
    int ret;

    srand((unsigned int)(time(NULL)));

    exit_flag = 0;

	s_fpu_test_thread_init_integer = 0;

    s_test_para1.busy_waiting_count = 1;
    s_test_para1.rounding_mode = FE_TONEAREST;
    ret = xTaskCreate(run_fpu_test, "fpu-test-1", 4096,
                      (void *)&s_test_para1, 31, NULL);
    if (ret != pdPASS)
    {
        exit_flag = 1;
        printf("create task failed.\n");
        return -1;
    }

    s_test_para2.busy_waiting_count = 240000;
    s_test_para2.rounding_mode = FE_TOWARDZERO;
    ret = xTaskCreate(run_fpu_test, "fpu-test-2", 4096,
                      (void *)&s_test_para2, 31, NULL);
    if (ret != pdPASS)
    {
        exit_flag = 1;
        printf("create task failed.\n");
        return -1;
    }
    while (1)
    {
        char cRxed = 0;

        cRxed = getchar();
        if (cRxed == 'q' || cRxed == 3)
        {
            exit_flag = 1;
            return 0;
        }
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_fpu, test_fpu, test fpu);
