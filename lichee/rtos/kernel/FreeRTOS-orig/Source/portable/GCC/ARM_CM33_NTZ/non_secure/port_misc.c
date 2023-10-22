#include <stdint.h>
#include <irqs.h>
#include <platform.h>
#include <core_cm33.h>
#include <compiler.h>

volatile uint32_t ulPortInterruptNesting = 0;

void enter_interrupt_handler(void)
{
    ulPortInterruptNesting++;
}

void exit_interrupt_handler(void)
{
    ulPortInterruptNesting--;
}

__attribute__((section (".sram_text"), no_instrument_function))
uint32_t uGetInterruptNest(void)
{
    return __get_IPSR() & 0x1ff;
    /* return ulPortInterruptNesting; */
}
