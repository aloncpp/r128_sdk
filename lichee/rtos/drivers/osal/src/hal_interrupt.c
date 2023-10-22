#include <stdio.h>
#include <interrupt.h>
#include <hal_interrupt.h>
#include <hal_status.h>
#include <port_misc.h>
#include <compiler.h>

#include <task.h>

int32_t arch_request_irq(int32_t irq, hal_irq_handler_t handler, void *data);
void arch_free_irq(int32_t irq);
void arch_enable_irq(int32_t irq);
void arch_disable_irq(int32_t irq);
void arch_irq_set_prioritygrouping(uint32_t group);
uint32_t arch_irq_get_prioritygrouping(void);
void arch_irq_set_priority(int32_t irq, uint32_t preemptpriority, uint32_t subpriority);
void arch_irq_get_priority(int32_t irq, uint32_t prioritygroup, uint32_t *p_preemptpriority, uint32_t *p_subpriority);
void arch_nvic_irq_set_priority(int32_t irq, uint32_t priority);
uint32_t arch_nvic_irq_get_priority(int32_t irq);
void arch_enable_all_irq(void);
void arch_disable_all_irq(void);
unsigned long arch_irq_is_disable(void);
unsigned long xport_interrupt_disable(void);
void xport_interrupt_enable(unsigned long flags);

__attribute__((weak))
void irq_init(void)
{
}

__attribute__((weak))
void arch_irq_set_prioritygrouping(uint32_t group)
{
	return;
}

__attribute__((weak))
uint32_t arch_irq_get_prioritygrouping(void)
{
	return 0;
}

__attribute__((weak))
void arch_irq_set_priority(int32_t irq, uint32_t preemptpriority, uint32_t subpriority)
{
	return;
}

__attribute__((weak))
void arch_irq_get_priority(int32_t irq, uint32_t prioritygroup, uint32_t *p_preemptpriority, uint32_t *p_subpriority)
{
	return;
}

__attribute__((weak))
void arch_nvic_irq_set_priority(int32_t irq, uint32_t priority)
{
	return;
}

__attribute__((weak))
uint32_t arch_nvic_irq_get_priority(int32_t irq)
{
	return 0;
}

__attribute__((weak))
void arch_clear_pending(int32_t irq)
{
}

__attribute__((weak))
int arch_is_pending(int32_t irq)
{
	return 0;
}

__attribute__((weak))
void arch_set_pending(int32_t irq)
{
}

__nonxip_text
void hal_interrupt_clear_pending(int32_t irq)
{
	arch_clear_pending(irq);
}

__nonxip_text
int32_t hal_interrupt_is_pending(int32_t irq)
{
	return arch_is_pending(irq);
}

__nonxip_text
void hal_interrupt_set_pending(int32_t irq)
{
	arch_set_pending(irq);
}

int32_t hal_request_irq(int32_t irq, hal_irq_handler_t handler, const char *name, void *data)
{
	return arch_request_irq(irq, handler, data);
}

void hal_free_irq(int32_t irq)
{
	arch_free_irq(irq);
}

__nonxip_text
int hal_enable_irq(int32_t irq)
{
	arch_enable_irq(irq);

	return HAL_OK;
}

__nonxip_text
void hal_disable_irq(int32_t irq)
{
	arch_disable_irq(irq);
}

void hal_irq_set_prioritygrouping(uint32_t group)
{
	arch_irq_set_prioritygrouping(group);
}

uint32_t hal_irq_get_prioritygrouping(void)
{
	return arch_irq_get_prioritygrouping();
}

void hal_irq_set_priority(int32_t irq, uint32_t preemptpriority, uint32_t subpriority)
{
	return arch_irq_set_priority(irq, preemptpriority, subpriority);
}

void hal_irq_get_priority(int32_t irq, uint32_t prioritygroup, uint32_t *p_preemptpriority, uint32_t *p_subpriority)
{
	return arch_irq_get_priority(irq, prioritygroup, p_preemptpriority, p_subpriority);
}

void hal_nvic_irq_set_priority(int32_t irq, uint32_t priority)
{
    return arch_nvic_irq_set_priority(irq, priority);
}

uint32_t hal_nvic_irq_get_priority(int32_t irq)
{
    return arch_nvic_irq_get_priority(irq);
}

__attribute__((section (".sram_text"), no_instrument_function))
uint32_t hal_interrupt_get_nest(void)
{
	return uGetInterruptNest();
}

__nonxip_text
void hal_interrupt_enable(void)
{
    if (hal_interrupt_get_nest() == 0) {
#ifndef CONFIG_SMP
        taskEXIT_CRITICAL();
#else
        taskEXIT_CRITICAL_FROM_ISR(0);
#endif
    } else {
        taskEXIT_CRITICAL_FROM_ISR(0);
    }
}

__nonxip_text
void hal_interrupt_disable(void)
{
    if (hal_interrupt_get_nest() == 0) {
#ifndef CONFIG_SMP
        taskENTER_CRITICAL();
#else
        taskENTER_CRITICAL_FROM_ISR();
#endif
    } else {
        taskENTER_CRITICAL_FROM_ISR();
    }
}

__nonxip_text
unsigned long hal_interrupt_disable_irqsave(void)
{
    unsigned long flag = 0;
    if (hal_interrupt_get_nest() == 0) {
#ifndef CONFIG_SMP
        taskENTER_CRITICAL();
#else
        taskENTER_CRITICAL(flag);
#endif
    } else {
        flag = taskENTER_CRITICAL_FROM_ISR();
    }
    return flag;
}

__nonxip_text
void hal_interrupt_enable_irqrestore(unsigned long flag)
{
    if (hal_interrupt_get_nest() == 0) {
#ifndef CONFIG_SMP
        taskEXIT_CRITICAL();
#else
        taskEXIT_CRITICAL(flag);
#endif
    } else {
        taskEXIT_CRITICAL_FROM_ISR(flag);
    }
}

__nonxip_text
unsigned long hal_interrupt_save(void)
{
    unsigned long flag = 0;
    if (hal_interrupt_get_nest() == 0) {
#ifndef CONFIG_SMP
        taskENTER_CRITICAL();
#else
        taskENTER_CRITICAL(flag);
#endif
    } else {
        flag = taskENTER_CRITICAL_FROM_ISR();
    }
    return flag;
}

__nonxip_text
void hal_interrupt_restore(unsigned long flag)
{
    if (hal_interrupt_get_nest() == 0) {
#ifndef CONFIG_SMP
        taskEXIT_CRITICAL();
#else
        taskEXIT_CRITICAL(flag);
#endif
    } else {
        taskEXIT_CRITICAL_FROM_ISR(flag);
    }
}

__nonxip_text
unsigned long hal_interrupt_is_disable(void)
{
    return arch_irq_is_disable();
}

void hal_interrupt_init(void)
{
	irq_init();
}
