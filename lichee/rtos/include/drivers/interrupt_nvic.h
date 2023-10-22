/**
 * NVIC.h - definition for registers of generic interrupt controller
 * date:    2012/2/12 22:34:41
 * author:  Aaron<leafy.myeh@allwinnertech.com>
 * history: V0.1
 *          2012-3-21 9:13:43 remapping fpga irq: gpu, rtc domain etc.
 */
#ifndef __INTERRUPT_NVIC_H
#define __INTERRUPT_NVIC_H

#define NVIC_SET_EN(n)      (NVIC_BASE + 0x000 + (n) * 0x04) /* set enable register 0~15 */
#define NVIC_CLR_EN(n)      (NVIC_BASE + 0x080 + (n) * 0x04) /* clear enable register 0~15 */
#define NVIC_PEND_SET(n)    (NVIC_BASE + 0x100 + (n) * 0x04) /* set pending register 0~15 */
#define NVIC_PEND_CLR(n)    (NVIC_BASE + 0x180 + (n) * 0x04) /* clear pending register 0~15 */
#define NVIC_PEND_ACTIVE(n) (NVIC_BASE + 0x200 + (n) * 0x04) /* active bit register 0~15 */
#define NVIC_PRIORITY(n)    (NVIC_BASE + 0x300 + (n) * 0x04) /* active bit register 0~15 */

#define NVIC_SECUREFAULT	 (7)

#define NVIC_PERIPH_IRQ_OFFSET (16)

typedef enum {
	INT_TG_SEC 		= 0,
	INT_TG_NONSEC 	= 1
} INT_TG_STATE;

void null_exception(void);
void HardFaultHandler(void);
void BusFaultHandler(void);
void SecureFaultHandler(void);
void MemoryFaultHandler(void);
void UsageFaultHandler(void);
#endif
