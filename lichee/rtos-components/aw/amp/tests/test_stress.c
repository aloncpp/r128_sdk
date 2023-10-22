#include <stdlib.h>
#include <stdio.h>
#include <hal_cmd.h>

#include <sunxi_amp.h>

#include <FreeRTOS.h>
#include <task.h>

#include <hal_time.h>

#define MAX_TEST_NUM (100000)

int rv_misc_ioctrl(int cmd, void *data, int data_len);
int m33_misc_ioctrl(int cmd, void *data, int data_len);
int dsp_misc_ioctrl(int cmd, void *data, int data_len);

static int run_tests(int cmd, int num)
{
    int i;
    int *buffer = amp_align_malloc(8);
    if (!buffer)
    {
        printf("alloc memory failed!\r\n");
        return -1;;
    }

    for (i = 0; i < MAX_TEST_NUM; i++)
    {
        *buffer = num;
        *(buffer + 1) = i;
        switch (cmd)
        {
            case AMP_MISC_CMD_RV_CALL_M33_STRESS_TEST:
            case AMP_MISC_CMD_DSP_CALL_M33_STRESS_TEST:
            case AMP_MISC_CMD_RV_CALL_M33_CALL_RV_STRESS_TEST:
            case AMP_MISC_CMD_RV_CALL_M33_CALL_DSP_STRESS_TEST:
            case AMP_MISC_CMD_RV_CALL_M33_CALL_DSP_CALL_RV_STRESS_TEST:
                m33_misc_ioctrl(cmd, buffer, 8);
                break;
            case AMP_MISC_CMD_M33_CALL_RV_STRESS_TEST:
            case AMP_MISC_CMD_DSP_CALL_RV_STRESS_TEST:
                rv_misc_ioctrl(cmd, buffer, 8);
                break;
            case AMP_MISC_CMD_M33_CALL_DSP_STRESS_TEST:
            case AMP_MISC_CMD_RV_CALL_DSP_STRESS_TEST:
            case AMP_MISC_CMD_RV_CALL_DSP_CALL_RV_STRESS_TEST:
                dsp_misc_ioctrl(cmd, buffer, 8);
                break;
            default:
                break;
        }
    }

    amp_align_free(buffer);
    return 0;
}

static void amp_stress_thread_entry(void *arg)
{
    int num = (int)(unsigned long)arg;

#ifdef CONFIG_AMP_TEST_RV_CALL_M33
    printf("---------RV call CM33 stress test-----------\r\n");
    run_tests(AMP_MISC_CMD_RV_CALL_M33_STRESS_TEST, num);
#endif

#ifdef CONFIG_AMP_TEST_RV_CALL_DSP
    printf("---------RV call DSP stress test-----------\r\n");
    run_tests(AMP_MISC_CMD_RV_CALL_DSP_STRESS_TEST, num);
#endif

#ifdef CONFIG_AMP_TEST_DSP_CALL_RV
    printf("---------DSP call RV stress test-----------\r\n");
    run_tests(AMP_MISC_CMD_DSP_CALL_RV_STRESS_TEST, num);
#endif

#ifdef CONFIG_AMP_TEST_DSP_CALL_M33
    printf("---------DSP call M33 stress test-----------\r\n");
    run_tests(AMP_MISC_CMD_DSP_CALL_M33_STRESS_TEST, num);
#endif

#ifdef CONFIG_AMP_TEST_M33_CALL_RV
    printf("---------M33 call RV stress test-----------\r\n");
    run_tests(AMP_MISC_CMD_M33_CALL_RV_STRESS_TEST, num);
#endif

#ifdef CONFIG_AMP_TEST_M33_CALL_DSP
    printf("---------M33 call DSP stress test-----------\r\n");
    run_tests(AMP_MISC_CMD_M33_CALL_DSP_STRESS_TEST, num);
#endif

#ifdef CONFIG_AMP_TEST_RV_CALL_M33_CALL_RV
    printf("---------RV call M33 call RV stress test-----------\r\n");
    run_tests(AMP_MISC_CMD_RV_CALL_M33_CALL_RV_STRESS_TEST, num);
#endif

#ifdef CONFIG_AMP_TEST_RV_CALL_DSP_CALL_RV
    printf("---------RV call DSP call RV stress test-----------\r\n");
    run_tests(AMP_MISC_CMD_RV_CALL_DSP_CALL_RV_STRESS_TEST, num);
#endif

#ifdef CONFIG_AMP_TEST_RV_CALL_M33_CALL_DSP
    printf("---------RV call M33 call DSP stress test-----------\r\n");
    run_tests(AMP_MISC_CMD_RV_CALL_M33_CALL_DSP_STRESS_TEST, num);
#endif

#ifdef CONFIG_AMP_TEST_RV_CALL_M33_CALL_DSP_CALL_RV
    printf("---------RV call M33 call DSP call RV stress test-----------\r\n");
    run_tests(AMP_MISC_CMD_RV_CALL_M33_CALL_DSP_CALL_RV_STRESS_TEST, num);
#endif

    vTaskDelete(NULL);
}

int cmd_amp_stress_test(int argc, char *argv[])
{
    int i;
    int test_thread;
    char name[16];
    portBASE_TYPE ret;

    if (argc < 2) {
        printf("Please input test_thread, such as amp_stress_test 1.\n");
        return 0;
    }

    test_thread = strtol(argv[1], NULL, 0);

    for (i = 0; i < test_thread; i++)
    {
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name) - 1, "test-%d", i);
        ret = xTaskCreate(amp_stress_thread_entry, name, 1024, (void *)(unsigned long)i, 3, NULL);
        if (ret != pdPASS)
        {
            printf("create %s failed!\n", name);
        }
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_amp_stress_test, amp_stress_test, AMP Stress Test Command);
