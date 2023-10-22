#include <stdio.h>
#include <interrupt.h>
#include <hal_interrupt.h>
#include <hal_status.h>
#include <sunxi_hal_common.h>

#include <portmacro.h>

#include <xtensa/hal.h>

#include <FreeRTOS.h>
#include <task.h>

int32_t hal_request_irq(int32_t irq, hal_irq_handler_t handler, const char *name, void *data)
{
	if (irq_request(irq, (interrupt_handler_t)handler, data) == FAIL) {
		return HAL_ERROR;
	}

	return HAL_OK;
}

void hal_free_irq(int32_t irq)
{
	if (irq_free(irq) == FAIL) {
		printf("irq free failure.\n");
	}
}

int hal_enable_irq(int32_t irq)
{
	return irq_enable(irq);
}

void hal_disable_irq(int32_t irq)
{
	irq_disable(irq);
}

extern unsigned port_interruptNesting;	/* defined in port.c */
uint32_t hal_interrupt_get_nest(void)
{
	return port_interruptNesting;
}

void hal_interrupt_enable(void)
{
    if (hal_interrupt_get_nest() == 0) {
        taskEXIT_CRITICAL();
    } else {
        taskEXIT_CRITICAL_FROM_ISR(0);
    }
}

void hal_interrupt_disable(void)
{
    if (hal_interrupt_get_nest() == 0) {
        taskENTER_CRITICAL();
    } else {
        taskENTER_CRITICAL_FROM_ISR();
    }
}

unsigned long hal_interrupt_disable_irqsave(void)
{
    unsigned long flag = 0;
    if (hal_interrupt_get_nest() == 0) {
        taskENTER_CRITICAL();
    } else {
        flag = taskENTER_CRITICAL_FROM_ISR();
    }
    return flag;
}

void hal_interrupt_enable_irqrestore(unsigned long flag)
{
    if (hal_interrupt_get_nest() == 0) {
        taskEXIT_CRITICAL();
    } else {
        taskEXIT_CRITICAL_FROM_ISR(flag);
    }
}

unsigned long hal_interrupt_is_disable(void)
{
    unsigned long ps;
    __asm__ volatile("rsr.ps %0\n" : "=r"(ps));
    if ((ps & 0xf) >= XCHAL_EXCM_LEVEL)
        return 1;
    return 0;
}
