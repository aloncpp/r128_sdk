#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hal_cache.h>

#include "sunxi_amp.h"

static void _tfm_sunxi_flashenc_set_region(uint8_t id, uint32_t saddr, uint32_t eaddr)
{
    extern void tfm_sunxi_flashenc_set_region(uint8_t id, uint32_t saddr, uint32_t eaddr);
    tfm_sunxi_flashenc_set_region(id, saddr, eaddr);
}

static void _tfm_sunxi_flashenc_set_key(uint8_t id, uint32_t *key)
{
	extern void tfm_sunxi_flashenc_set_key(uint8_t id, uint32_t *key);
    tfm_sunxi_flashenc_set_key(id, key);
}

static void _tfm_sunxi_flashenc_set_ssk_key(uint8_t id)
{
	extern void tfm_sunxi_flashenc_set_ssk_key(uint8_t id);
    tfm_sunxi_flashenc_set_ssk_key(id);
}


static void _tfm_sunxi_flashenc_enable(uint8_t id)
{
    extern void tfm_sunxi_flashenc_enable(uint8_t id);
    tfm_sunxi_flashenc_enable(id);
}

static void _tfm_sunxi_flashenc_disable(uint8_t id)
{
    extern void tfm_sunxi_flashenc_disable(uint8_t id);
    tfm_sunxi_flashenc_disable(id);
}

sunxi_amp_func_table tfm_table[] = {
	{.func = (void *)&_tfm_sunxi_flashenc_set_region, .args_num = 3, .return_type = RET_POINTER},
	{.func = (void *)&_tfm_sunxi_flashenc_set_key, .args_num = 2, .return_type = RET_POINTER},
	{.func = (void *)&_tfm_sunxi_flashenc_set_ssk_key, .args_num = 1, .return_type = RET_POINTER},
	{.func = (void *)&_tfm_sunxi_flashenc_enable, .args_num = 1, .return_type = RET_POINTER},
	{.func = (void *)&_tfm_sunxi_flashenc_disable, .args_num = 1, .return_type = RET_POINTER},
};

