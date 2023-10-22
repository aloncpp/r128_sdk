#include <xtensa_api.h>

#include <sunxi_hal_common.h>
#include <irqs.h>
#include <interrupt.h>
#include <hal_intc.h>

#ifdef CONFIG_DRIVERS_INTC

s32 irq_request(u32 irq_no, interrupt_handler_t hdle, void *arg)
{
	xt_handler old;
	s32 ret;

	if ((irq_no & RINTC_IRQ_MASK) == RINTC_IRQ_MASK) { /* intc interrupt */
		ret = install_isr(irq_no ^ RINTC_IRQ_MASK, (__pISR_hdle_t)hdle, arg);
	} else { /* dsp interrupt */
		old = xt_set_interrupt_handler(irq_no, (xt_handler)hdle, arg);
		ret = old ? OK : FAIL;
	}

	return ret;
}

s32 irq_free(u32 irq_no)
{
	xt_handler old;
	s32 ret;

	if ((irq_no & RINTC_IRQ_MASK) == RINTC_IRQ_MASK) { /* intc interrupt */
		ret = uninstall_isr(irq_no ^ RINTC_IRQ_MASK, NULL);
	} else { /* dsp interrupt */
		old = xt_set_interrupt_handler(irq_no, (xt_handler)NULL, NULL);
		ret = old ? OK : FAIL;
	}
	return ret;
}

s32 irq_enable(u32 irq_no)
{
	s32 ret = OK;

	if ((irq_no & RINTC_IRQ_MASK) == RINTC_IRQ_MASK) /* intc interrupt */
		ret = interrupt_enable(irq_no ^ RINTC_IRQ_MASK);
	else /* dsp interrupt */
		xt_ints_on(1 << irq_no);

	return ret;
}

s32 irq_disable(u32 irq_no)
{
	s32 ret = OK;

	if ((irq_no & RINTC_IRQ_MASK) == RINTC_IRQ_MASK) /* intc interrupt */
		ret = interrupt_disable(irq_no ^ RINTC_IRQ_MASK);
	else /* dsp interrupt */
		xt_ints_off(1 << irq_no);

	return ret;
}

#else /* not defined CONFIG_DRIVERS_INTC */

s32 irq_request(u32 irq_no, interrupt_handler_t hdle, void *arg)
{
	xt_handler old;

	old = xt_set_interrupt_handler(irq_no, (xt_handler)hdle, arg);

	return old ? OK : FAIL;
}

s32 irq_free(u32 irq_no)
{
	xt_handler old;

	old = xt_set_interrupt_handler(irq_no, (xt_handler)NULL, NULL);

	return old ? OK : FAIL;
}

s32 irq_enable(u32 irq_no)
{
	xt_ints_on(1 << irq_no);

	return OK;
}

s32 irq_disable(u32 irq_no)
{
	xt_ints_off(1 << irq_no);

	return OK;
}

#endif /* CONFIG_DRIVERS_INTC */
