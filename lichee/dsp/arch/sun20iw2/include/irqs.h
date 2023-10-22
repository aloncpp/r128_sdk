#ifndef __SUNXI_IRQS_H
#define __SUNXI_IRQS_H

#if defined(CONFIG_CORE_DSP0)
/*
 * DSP0 interrupt source
 */
#define SUNXI_DSP_IRQ_NMI		0
#define SUNXI_DSP_IRQ_DSP_TIMER1	1
#define SUNXI_DSP_IRQ_DSP_TIMER0	2
#endif /* CONFIG_CORE_DSP0 */

/*
 * R_INTC interrupt source
 */
#define RINTC_IRQ_MASK			0xffff0000

#endif /* __SUNXI_IRQS_H */
