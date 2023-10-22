#include <stdlib.h>
#include <hal_cache.h>
#include "hw_perf.h"

void nop_instruction(void);

static int cmd_cache_penalty(int argc, char ** argv)
{
    printf("cache penalty:func@0x%08x\n", &cmd_cache_penalty);
    _perf_disable_irq();
    hal_icache_invalidate_all();

    _perf_gpio2_low();
    nop_instruction();
    _perf_gpio2_high();

    _perf_gpio2_low();
    nop_instruction();
    _perf_gpio2_high();

    _perf_enable_irq();
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_cache_penalty, cache_penalty, penalty);
