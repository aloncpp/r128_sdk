#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "sunxi_amp.h"
#include <hal_cache.h>

int rv_misc_ioctrl(int cmd, void *data, int data_len)
{
    int ret;
    void *args[3] = {0};

    args[0] = (void *)(unsigned long)cmd;
    args[1] = data;
    args[2] = (void *)(unsigned long)data_len;

    hal_dcache_clean((unsigned long)data, data_len);
    ret = func_stub(RPCCALL_RV_MISC(misc_ioctrl), 1, 3, (void *)&args);
    hal_dcache_invalidate((unsigned long)data, data_len);

    return ret;
}

int m33_misc_ioctrl(int cmd, void *data, int data_len)
{
    int ret;
    void *args[3] = {0};

    args[0] = (void *)(unsigned long)cmd;
    args[1] = data;
    args[2] = (void *)(unsigned long)data_len;

    hal_dcache_clean((unsigned long)data, data_len);
    ret = func_stub(RPCCALL_M33_MISC(misc_ioctrl), 1, 3, (void *)&args);
    hal_dcache_invalidate((unsigned long)data, data_len);

    return ret;
}

int dsp_misc_ioctrl(int cmd, void *data, int data_len)
{
    int ret;
    void *args[3] = {0};

    args[0] = (void *)(unsigned long)cmd;
    args[1] = data;
    args[2] = (void *)(unsigned long)data_len;

    hal_dcache_clean((unsigned long)data, data_len);
    ret = func_stub(RPCCALL_DSP_MISC(misc_ioctrl), 1, 3, (void *)&args);
    hal_dcache_invalidate((unsigned long)data, data_len);

    return ret;
}
