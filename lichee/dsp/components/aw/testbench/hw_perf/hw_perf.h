#ifndef HW_PERF_H
#define HW_PERF_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <console.h>

#include "hal/hal_gpio.h"
//#include "arch/arm/mach/sun20iw2p1/platform-sun20iw2p1.h"
//#include "arch/arm/mach/sun20iw2p1/irqs-sun20iw2p1.h"

/* define for code is same on all platforms */

#ifdef CONFIG_ARCH_ARM_ARMV8M
#include "cmsis/core_cm33.h"

#define _perf_enable_irq()               do { asm volatile ("cpsie i"); } while (0);
#define _perf_disable_irq()              do { asm volatile ("cpsid i"); } while (0);

#elif defined CONFIG_ARCH_RISCV
#include "rv64/csr.h"

#define csr_set(csr, val)                   \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrs " __ASM_STR(csr) ", %0" \
                              : : "rK" (__v)            \
                              : "memory");          \
    })
#define csr_clear(csr, val)                 \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrc " __ASM_STR(csr) ", %0" \
                              : : "rK" (__v)            \
                              : "memory");          \
    })

#define _perf_enable_irq() csr_set(CSR_MSTATUS, SR_MIE)
#define _perf_disable_irq() csr_clear(CSR_MSTATUS, SR_MIE)

#elif defined CONFIG_ARCH_XTENSA

#endif

#define _perf_readl(addr)	    (*((volatile unsigned int *)(addr)))
#define _perf_writel(v, addr)	(*((volatile unsigned int *)(addr)) = (unsigned int)(v))

#ifdef CONFIG_ARCH_SUN20IW2P1

#define PERF_TEST_NUM 500

#define TEST_GPIO1_NUM 14
#define TEST_GPIO2_NUM 15

#define TEST_OUT_GPIO GPIOA(TEST_GPIO1_NUM)
#define TEST_IRQ_GPIO GPIOA(TEST_GPIO2_NUM)

#define _PERF_GPIO_DATA_REG 0x4004A410
#define _PERF_GPIO_DATA_REGN 0x4004A43C /* not exist */
#define _perf_gpio1_high()  _perf_writel(1<<TEST_GPIO1_NUM, _PERF_GPIO_DATA_REG)
#define _perf_gpio1_low()   _perf_writel(0<<TEST_GPIO1_NUM, _PERF_GPIO_DATA_REG)
#define _perf_gpio2_high()  _perf_writel(1<<TEST_GPIO2_NUM, _PERF_GPIO_DATA_REG)
#define _perf_gpio2_low()   _perf_writel(0<<TEST_GPIO2_NUM, _PERF_GPIO_DATA_REG)

#ifdef CONFIG_DRIVERS_TIMER
#include "sunxi_hal_timer.h"

#define _PERF_SUNXI_TMR_PBASE   SUNXI_TMR_PBASE
#define _PERF_TIMER_INTVAL_REG(val) (_PERF_SUNXI_TMR_PBASE + 0x10 * (val) + 0x14)
#define _PERF_TIMER_CNTVAL_REG(val) (_PERF_SUNXI_TMR_PBASE + 0x10 * (val) + 0x18)
#define _PERF_TIMER_INTVAL_VALUE    0x10000000
#define _PERF_TIMER_INTVAL_VALUE_T  0x1000

#endif

#ifdef CONFIG_DRIVERS_DMA
#include "hal_dma.h"

#define _PERF_DMA_DST_REG 0x40000114
#define _PERF_DMA_SRC_REG 0x40000108
#define _PERF_DMA_INTVAL_DST_T  0x1000

#define _PERF_DMA_TEST_CHANNEL  0

#endif

#endif /* CONFIG_ARCH_SUN20IW2P1 */

#ifdef CONFIG_XIP

#include "flash_mcu/hal_flash.h"

#define _PERF_MFLASH 0

#define _PERF_XIP_TEST_SIZE (200 * 1024)

#endif

void time_perf_init(void);
void time_perf_deinit(void);
uint32_t time_perf_tag(void); /* based on uS */
void time_perf_result(uint32_t bench_size, float time_use);
int32_t hw_perf_parse_val(char *valstring);

#endif
