#include <stdint.h>

volatile uint32_t ulPortInterruptNesting = 0;

void enter_interrupt_handler(void)
{
    ulPortInterruptNesting++;
}

void exit_interrupt_handler(void)
{
    ulPortInterruptNesting--;
}

__attribute__((no_instrument_function))
uint32_t uGetInterruptNest(void)
{
    return ulPortInterruptNesting;
}
