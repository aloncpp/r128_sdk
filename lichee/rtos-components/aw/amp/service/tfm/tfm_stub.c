#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hal_cache.h>

#include "sunxi_amp.h"

MAYBE_STATIC void tfm_sunxi_flashenc_set_region(uint8_t id, uint32_t saddr, uint32_t eaddr)
{
    void *args[3] = {0};
	args[0] = (void *)(uintptr_t)id;
	args[1] = (void *)(uintptr_t)saddr;
	args[2] = (void *)(uintptr_t)eaddr;
	func_stub(RPCCALL_TFM(tfm_sunxi_flashenc_set_region), 1, ARRAY_SIZE(args), (void *)&args);
}
MAYBE_STATIC void tfm_sunxi_flashenc_set_key(uint8_t id, uint32_t *key)
{
    void *args[2] = {0};
	args[0] = (void *)(uintptr_t)id;
	args[1] = (void *)(uintptr_t)key;
	func_stub(RPCCALL_TFM(tfm_sunxi_flashenc_set_key), 1 , ARRAY_SIZE(args), (void *)&args);
}
MAYBE_STATIC void tfm_sunxi_flashenc_set_ssk_key(uint8_t id)
{
    void *args[1] = {0};
	args[0] = (void *)(uintptr_t)id;
	func_stub(RPCCALL_TFM(tfm_sunxi_flashenc_set_ssk_key), 1 , ARRAY_SIZE(args), (void *)&args);
}
MAYBE_STATIC void tfm_sunxi_flashenc_enable(uint8_t id)
{
    void *args[1] = {0};
	args[0] = (void *)(uintptr_t)id;
	func_stub(RPCCALL_TFM(tfm_sunxi_flashenc_enable), 1, ARRAY_SIZE(args), (void *)&args);
}
MAYBE_STATIC void tfm_sunxi_flashenc_disable(uint8_t id)
{
    void *args[1] = {0};
	args[0] = (void *)(uintptr_t)id;
	func_stub(RPCCALL_TFM(tfm_sunxi_flashenc_disable), 1, ARRAY_SIZE(args), (void *)&args);
}
