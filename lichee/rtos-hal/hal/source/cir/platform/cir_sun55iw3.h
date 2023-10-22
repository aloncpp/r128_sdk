#ifndef __IRRX_SUN55IW3_H__
#define __IRRX_SUN55IW3_H__

#include <hal_interrupt.h>

#if defined(CONFIG_ARCH_RISCV)
#define SUNXI_IRQ_IRADC		MAKE_IRQn(93, 4)
#endif

#define SUNXI_IRADC_PBASE	0x02005000
#define IRADC_PIN		GPIOH(19)
#define IR_MUXSEL		2
#define IR_DRVSEL		2

#endif /* __IRRX_SUN55IW3_H__ */
