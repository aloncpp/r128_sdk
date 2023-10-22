#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <../../hal/source/spinor/inter.h>

#include "sunxi_amp.h"
#include <hal_cache.h>

static int _nor_read(unsigned int addr, char *buf, unsigned int size)
{
	int ret;
    ret = nor_read(addr, buf, size);
    hal_dcache_clean((unsigned long)buf, size);
    return ret;
}

static int _nor_write(unsigned int addr, char *buf, unsigned int size)
{
    int ret;
    hal_dcache_invalidate((unsigned long)buf, size);
    ret = nor_write(addr, buf, size);
    hal_dcache_invalidate((unsigned long)buf, size);
    return ret;
}

static int _nor_erase(unsigned int addr, unsigned int size)
{
    return nor_erase(addr, size);
}

static int _nor_ioctrl(int cmd, void *buf, int size)
{
    int ret;
    ret = nor_ioctrl(cmd, buf, size);
    hal_dcache_clean((unsigned long)buf, size);
    return ret;
}

sunxi_amp_func_table flashc_table[] =
{
    {.func = (void *)&_nor_read, .args_num = 3, .return_type = RET_POINTER},
    {.func = (void *)&_nor_write, .args_num = 3, .return_type = RET_POINTER},
    {.func = (void *)&_nor_erase, .args_num = 2, .return_type = RET_POINTER},
    {.func = (void *)&_nor_ioctrl, .args_num = 3, .return_type = RET_POINTER},
};
