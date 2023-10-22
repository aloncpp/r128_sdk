#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sunxi_amp.h"
#include <barrier.h>

#include <hal_cache.h>

int dsp_misc_ioctrl(int cmd, void *data, int data_len);
int rv_misc_ioctrl(int cmd, void *data, int data_len);
int m33_misc_ioctrl(int cmd, void *data, int data_len);

static int _misc_ioctrl(int cmd, void *data, int data_len)
{
    int ret = 0;
    int *buffer;
    hal_dcache_invalidate((unsigned long)data, data_len);

    switch (cmd)
    {
        case AMP_MISC_CMD_RV_CALL_M33_STRESS_TEST:
        case AMP_MISC_CMD_DSP_CALL_M33_STRESS_TEST:
        case AMP_MISC_CMD_M33_CALL_RV_STRESS_TEST:
        case AMP_MISC_CMD_DSP_CALL_RV_STRESS_TEST:
        case AMP_MISC_CMD_M33_CALL_DSP_STRESS_TEST:
        case AMP_MISC_CMD_RV_CALL_DSP_STRESS_TEST:
            buffer = (int *)data;
            if ((*(buffer + 1) % 100) == 0)
                printf("%s receive cmd(%d) (%d:%d)\r\n", SELF_NAME, cmd, *buffer, *(buffer + 1));
            break;
        case AMP_MISC_CMD_RV_CALL_M33_CALL_DSP_STRESS_TEST:
#ifdef CONFIG_ARCH_ARM_CORTEX_M33
            dsp_misc_ioctrl(cmd, data, data_len);
#elif defined(CONFIG_ARCH_DSP)
            buffer = (int *)data;
            if ((*(buffer + 1) % 100) == 0)
                printf("%s receive cmd(%d) (%d:%d)\r\n", SELF_NAME, cmd, *buffer, *(buffer + 1));
#endif
            break;
        case AMP_MISC_CMD_RV_CALL_M33_CALL_RV_STRESS_TEST:
#ifdef CONFIG_ARCH_ARM_CORTEX_M33
            rv_misc_ioctrl(cmd, data, data_len);
#elif defined(CONFIG_ARCH_RISCV_C906)
            buffer = (int *)data;
            if ((*(buffer + 1) % 100) == 0)
                printf("%s receive cmd(%d) (%d:%d)\r\n", SELF_NAME, cmd, *buffer, *(buffer + 1));
#endif
            break;
        case AMP_MISC_CMD_RV_CALL_DSP_CALL_RV_STRESS_TEST:
#if defined(CONFIG_ARCH_DSP)
            rv_misc_ioctrl(cmd, data, data_len);
#elif defined(CONFIG_ARCH_RISCV_C906)
            buffer = (int *)data;
            if ((*(buffer + 1) % 100) == 0)
                printf("%s receive cmd(%d) (%d:%d)\r\n", SELF_NAME, cmd, *buffer, *(buffer + 1));
#endif
            break;
        case AMP_MISC_CMD_RV_CALL_M33_CALL_DSP_CALL_RV_STRESS_TEST:
#ifdef CONFIG_ARCH_ARM_CORTEX_M33
            dsp_misc_ioctrl(cmd, data, data_len);
#elif defined(CONFIG_ARCH_DSP)
            rv_misc_ioctrl(cmd, data, data_len);
#elif defined(CONFIG_ARCH_RISCV_C906)
            buffer = (int *)data;
            if ((*(buffer + 1) % 100) == 0)
                printf("%s receive cmd(%d) (%d:%d)\r\n", SELF_NAME, cmd, *buffer, *(buffer + 1));
#endif
            break;
        default:
            break;
    }
    hal_dcache_clean((unsigned long)data, data_len);

    return ret;
}

sunxi_amp_func_table misc_table[] =
{
    {.func = (void *) &_misc_ioctrl, .args_num = 3, .return_type = RET_POINTER},
};
