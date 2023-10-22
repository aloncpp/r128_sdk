#include <stdint.h>

extern volatile uint32_t ulPortInterruptNesting[];
extern int cur_cpu_id(void);

uint32_t uGetInterruptNest(void)
{
    return ulPortInterruptNesting[cur_cpu_id()];
}
