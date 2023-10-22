/*
 * Allwinnertech rtos generic interrupt controller file.
 * Copyright (C) 2019  Allwinnertech Co., Ltd. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "FreeRTOSConfig.h"
#include "aw_types.h"
#include "serial.h"
#include "hw_memmap.h"
#include "interrupt.h"
#include "portmacro.h"
#include <stdio.h>
#include <interrupt_gic.h>

#include <sunxi_hal_common.h>

#define GICC_IAR_INT_ID_MASK	(0x3ff)

static __inline void sr32(u32 addr, u32 start_bit, u32 num_bits, u32 value)
{
    u32 tmp, msk = (1 << num_bits) - 1;
    tmp = readl(addr) & ~(msk << start_bit);
    tmp |= value << start_bit;
    writel(tmp, addr);
}

struct irq_desc{
    hal_irq_handler_t handle_irq;
    void *data;
};
static struct irq_desc irqs_desc[GIC_IRQ_NUM];

#define MAGIC_UNFROZEN  (0x23233232)
#define MAGIC_FROZEN    (0x51AB51AB)

static volatile int freeze_flag = MAGIC_UNFROZEN;

static hal_irqreturn_t null_irq_hdle(void *arg2)
{
	SMP_DBG("No irq registered handler for this calling !!\n");

	return HAL_IRQ_OK;
}

int check_freeze_flag(void)
{
    if (freeze_flag == MAGIC_FROZEN) {
        return 1;
    }
    return 0;
}

hal_irqreturn_t schedule_ipi_handler(void * p)
{
	SMP_DBG("%s line %d cpu %d.\n", __func__, __LINE__, cur_cpu_id());

	return HAL_IRQ_OK;
}

s32 arch_request_irq(u32 irq_no, hal_irq_handler_t hdle, void *data)
{
	if (irq_no < GIC_IRQ_NUM) {
		/* only set once in smp mode. */
		if(hdle && irqs_desc[irq_no].handle_irq == null_irq_hdle) {
    			irqs_desc[irq_no].handle_irq = hdle;
    			irqs_desc[irq_no].data = data;
    		}

		return irq_no;
	}
	SMP_DBG("Wrong irq NO.(%d) to request !!\n", irq_no);
	return -1;
}

s32 arch_free_irq(u32 irq_no)
{
	if (irq_no < GIC_IRQ_NUM) {
		irqs_desc[irq_no].handle_irq = null_irq_hdle;
    		irqs_desc[irq_no].data = NULL;
		return irq_no;
	}
	SMP_DBG("Wrong irq NO.(%d) to free !!\n", irq_no);
	return -1;
}

void arch_enable_all_irq(void)
{
    asm volatile ("cpsie i");
}

void arch_disable_all_irq(void)
{
	asm volatile ("cpsid i");
}

s32 arch_enable_irq(u32 irq_no)
{
	u32 base;
	u32 base_os;
	u32 bit_os;

	if (irq_no >= GIC_IRQ_NUM) {
		SMP_DBG("irq NO.(%d) > GIC_IRQ_NUM(%d) !!\n", irq_no, GIC_IRQ_NUM);
		return -1;
	}

	base_os = irq_no >> 5; // 除32
	base = GIC_SET_EN(base_os);
	bit_os = irq_no & 0x1f; // %32
	sr32(base, bit_os, 1, 1);

	return 0;
}

s32 arch_disable_irq(u32 irq_no)
{
	u32 base;
	u32 base_os;
	u32 bit_os;

	if (irq_no >= GIC_IRQ_NUM) {
		SMP_DBG("irq NO.(%d) > GIC_IRQ_NUM(%d) !!\n", irq_no, GIC_IRQ_NUM);
		return -1;
	}

	base_os = irq_no >> 5; // 除32
	base = GIC_CLR_EN(base_os);
	bit_os = irq_no & 0x1f; // %32
#if 0
	sr32(base, bit_os, 1, 1);
#else
	writel((1 << bit_os), base);
#endif

	return 0;
}

unsigned long arch_irq_is_disable(void)
{
    unsigned long result;
    __asm__ volatile ("mrs %0, cpsr" : "=r" (result) );
    if (result & 0x80)
    {
	    return 1;
    }
    return 0;
}

/*
 * Set irq to target CPUs; one irq has max 8 target CPUs
 * One Interrupt Processor Targets Register contains 4 interrupts configuration
 * One bit means one relevant CPU interface
 * 0bxxxxxxx1 means CPU interface 0; 0bxxxxxx1x means CPU interface 1
 */
s32 set_irq_target(u32 irq_no, u32 target)
{
	u32 base;
	u32 base_os;
	u32 bit_os;

	if (irq_no >= GIC_IRQ_NUM) {
		SMP_DBG("irq NO.(%d) > GIC_IRQ_NUM(%d) !!\n", irq_no, GIC_IRQ_NUM);
		return -1;
	}

	if(irq_no <= GIC_SRC_SGI15){
		base_os = irq_no >> 2; // 除4
		base = GIC_SGI_PROC_TARG(base_os);
		bit_os = (irq_no % 4) << 3; // %4
	}
	else if((irq_no >= GIC_SRC_PPI0) && (irq_no <= GIC_SRC_PPI15)){
		irq_no -= 16;
		base_os = irq_no >> 2; // 除4
		base = GIC_PPI_PROC_TARG(base_os);
		bit_os = (irq_no % 4) << 3; // %4
	}
	else if((irq_no >= 32) && (irq_no < GIC_IRQ_NUM)){
		irq_no -= 32;
		base_os = irq_no >> 2; // 除4
		base = GIC_SPI_PROC_TARG(base_os);
		bit_os = (irq_no % 4) << 3; // %4
	}
	else {
		/* to fix codestatic error */
		SMP_DBG("irq NO.(%d) not support\n", irq_no);
		return -1;
	}

	sr32(base, bit_os, 8, target);

	return 0;
}

extern volatile uint32_t ulPortYieldRequired[];
void gic_sgi_handler(u32 ulICCIAR)
{
	int id, cpu;
	int processor_id = cur_cpu_id();
	uint32_t cpu_sr;

	id  = ulICCIAR & GICC_IAR_INT_ID_MASK;
	cpu = (ulICCIAR >> 10) & 0x7;

	char *ipi_msg[] = {
		"IPI wakeup",
		"IPI schedu",
		"IPI rpcall",
		"IPI others",
		"IPI unknow"
	};

	int judge_critical(void);

	id = (id >= IPI_OTHERS ? IPI_OTHERS : id);
	switch (id)
	{
		case IPI_WAKEUP:
			break;
		case IPI_SCHEDU:
			if(!judge_critical()) {
				ulPortYieldRequired[processor_id] = 1;
			}
			break;
		case IPI_RPCALL:
			break;
		case IPI_GDBCMD:
			{
#ifdef CONFIG_COMPONENTS_WATCHPOINT
				extern void gdbcmd_ipi_handler(void);
				gdbcmd_ipi_handler();
#endif
			}
			break;
		case IPI_FREEZE:
			isb();
			freeze_flag = MAGIC_FROZEN;
			isb();
			while(1) {
				__asm__ __volatile__("wfe":::"memory");
			}
			break;
		case IPI_OTHERS:
			break;
		default:
			if(!judge_critical()) {
				ulPortYieldRequired[processor_id] = 1;
			}
			break;
	}

	*((volatile uint32_t *)GIC_SGI_PEND_CLR(id / 4)) |= \
		((1 << processor_id) << ((id % 4) * 8));
}

void gic_ppi_handler(u32 id)
{
	SMP_DBG("gic_ppi_handler: %d\n", id);
	if (irqs_desc[id].handle_irq && irqs_desc[id].handle_irq != null_irq_hdle) {
		irqs_desc[id].handle_irq(irqs_desc[id].data);
	}
}

void gic_spi_handler(u32 id)
{

	SMP_DBG("gic_spi_handler: %d\n", id);
	if (irqs_desc[id].handle_irq && irqs_desc[id].handle_irq != null_irq_hdle) {
                irqs_desc[id].handle_irq(irqs_desc[id].data);
	}
}

void gic_clear_pending(u32 id)
{
	u32 base;
	u32 base_os;
	u32 bit_os;

	base_os = id >> 5; // 除32
	base = GIC_PEND_CLR(base_os);
	bit_os = id & 0x1f; // %32
	writel(1<<bit_os, base);
}

#if 0
void gic_irq_handler(void)
{
	u32 idnum;

	idnum = readl(GIC_INT_ACK_REG);
	idnum &= GICC_IAR_INT_ID_MASK;

	if (idnum == 1023) {
		SMP_DBG("spurious irq !!\n");
		return;
	}
	if (idnum >= GIC_IRQ_NUM) {
		SMP_DBG("irq NO.(%d) > GIC_IRQ_NUM(%d) !!\n", idnum, GIC_IRQ_NUM-32);
		return;
	}

	if (idnum < 16)
	{
		gic_sgi_handler(idnum);
	}
	else if (idnum < 32)
	{
		gic_ppi_handler(idnum);
	}
	else
	{
		gic_spi_handler(idnum);
	}

	writel(idnum, GIC_END_INT_REG);
	writel(idnum, GIC_DEACT_INT_REG);
	gic_clear_pending(idnum);

//	SMP_DBG("gic_irq_handler: %d\n", idnum);
}
#endif

void vApplicationFPUSafeIRQHandler( u32 ulICCIAR )
{
    	/*volatile unsigned int *irno =(volatile unsigned int *) \
	 * portICCIAR_INTERRUPT_ACKNOWLEDGE_REGISTER_ADDRESS;*/

	uint32_t idnum = ulICCIAR;
	idnum &= GICC_IAR_INT_ID_MASK;
#if ( configUSE_NEWLIB_REENTRANT == 1 )
	extern void vSwitchOutReentFromISR(void);
	vSwitchOutReentFromISR();
#endif
	if (idnum == 1023)
	{
		SMP_DBG("spurious irq !!\n");
		return;
	}
	if (idnum >= GIC_IRQ_NUM)
	{
		SMP_DBG("irq NO.(%d) > GIC_IRQ_NUM(%d) !!\n", idnum, GIC_IRQ_NUM-32);
		return;
	}
	/*if(idnum == 0) idnum = 30;*/
	if (idnum < 16)
	{
		gic_sgi_handler(ulICCIAR);
	}
	else if (idnum < 32)
	{
		gic_ppi_handler(idnum);
	}
	else
	{
		gic_spi_handler(idnum);
	}
#if ( configUSE_NEWLIB_REENTRANT == 1 )
	extern void vSwitchInReentFromISR(void);
	vSwitchInReentFromISR();
#endif
}

void  OS_CPU_ExceptHndlr (u32  except_id)
{
#if 0
    switch (except_id) {
        case OS_CPU_ARM_EXCEPT_IRQ:
        	gic_irq_handler();
             break;

        case OS_CPU_ARM_EXCEPT_FIQ:
        	gic_irq_handler();
             break;

        case OS_CPU_ARM_EXCEPT_RESET:
        case OS_CPU_ARM_EXCEPT_UNDEF_INSTR:
        case OS_CPU_ARM_EXCEPT_SWI:
        case OS_CPU_ARM_EXCEPT_DATA_ABORT:
        case OS_CPU_ARM_EXCEPT_PREFETCH_ABORT:
        case OS_CPU_ARM_EXCEPT_ADDR_ABORT:
        default:
             SMP_DBG("irq error except_id = %d\n",except_id);                             /* See Note #1.                                              */
             break;
    }
#endif
}
static void gic_distributor_init(void)
{
	u32 cpumask = 0x01010101;
	u32 gic_irqs;
	u32 i;

	/*SMP_DBG("GIC_DIST_CON=0x%08x.\n", GIC_DIST_CON);*/

	writel(0, GIC_DIST_CON);

	/* check GIC hardware configutation */
	gic_irqs = ((readl(GIC_CON_TYPE) & 0x1f) + 1) * 32;
	if (gic_irqs > 1020)
		gic_irqs = 1020;
	else if (gic_irqs < GIC_IRQ_NUM) {
		/*SMP_DBG("GIC parameter config xx error, only support %d"*/
				/*" irqs < %d(spec define)!!\n", gic_irqs, GIC_IRQ_NUM);*/
		return ;
	}

	/*SMP_DBG("GIC Support max %d interrupts.\n", gic_irqs);*/

	/*  Set ALL interrupts as group1(non-secure) interrupts */
	unsigned int max_irq = readl(GIC_DIST_BASE + 0x004) & 0x1f;
	unsigned int it_lines_number = max_irq;

	for(i = 0; i <= it_lines_number; i++) {
		writel(0xffffffff, GIC_DIST_BASE + 0x80 + i * 4);
	}

	/* set trigger type to be level-triggered, active low */
	for (i=GIC_SRC_SGI(0); i<GIC_IRQ_NUM; i+=16)
		writel(0, GIC_IRQ_MOD_CFG(i>>4));				//除16
	/* set priority */
	for (i=GIC_SRC_SGI(0); i<GIC_IRQ_NUM; i+=4)
		writel(0xa0a0a0a0, GIC_SPI_PRIO((i-32)>>2));	//除4
	/* set processor target */
	for (i=GIC_SRC_SGI(0); i<GIC_IRQ_NUM; i+=4)
		writel(cpumask, GIC_SPI_PROC_TARG((i-32)>>2));	//除4

	/* disable all interrupts */
	for (i=GIC_SRC_SGI(0); i<GIC_IRQ_NUM; i+=32)
		writel(0xffffffff, GIC_CLR_EN(i>>5));	//除32

	/* clear all interrupt active state */
	for (i=GIC_SRC_SGI(0); i<GIC_IRQ_NUM; i+=32)
		writel(0xffffffff, GIC_ACT_CLR(i>>5));	//除32


	writel(0x3, GIC_DIST_CON);
	/*soft_break();*/
}

void gic_cpuif_init(void)
{
    	int i = 0;

	writel(0, GIC_CPU_IF_CTRL);

	writel(0xffffffff, GIC_DIST_BASE + 0x80);

	/*
	 * Deal with the banked PPI and SGI interrupts - disable all
	 * PPI interrupts, ensure all SGI interrupts are enabled.
	 */
	writel(0xffff0000, GIC_CLR_EN0);
	writel(0x0000ffff, GIC_SET_EN0);

	/* Set priority on PPI and SGI interrupts */
	for (i=0; i<16; i+=4)
	{
		writel(0xa0a0a0a0, GIC_SGI_PRIO(i>>2));
	}
	for (i=16; i<32; i+=4)
	{
		writel(0xa0a0a0a0, GIC_PPI_PRIO((i-16)>>2));
	}

#define GICC_CTLR_ENABLEGRP0    (1 << 0)
#define GICC_CTLR_ENABLEGRP1    (1 << 1)
#define GICC_CTLR_FIQEN         (1 << 3)
	writel(0xff, GIC_INT_PRIO_MASK);
	writel(GICC_CTLR_ENABLEGRP0 | GICC_CTLR_ENABLEGRP1 | GICC_CTLR_FIQEN, GIC_CPU_IF_CTRL);
	/*soft_break();*/
}

void init_gic(void)
{
	u32 i;
	for (i=0; i<GIC_IRQ_NUM; i++) {
		irqs_desc[i].handle_irq = null_irq_hdle;
    		irqs_desc[i].data = NULL;
    	}

	//gic_distributor_init();
	//gic_cpuif_init();
}

void register_sgi(void)
{
    hal_request_irq(0, NULL, NULL, NULL);
    hal_enable_irq(0);
}

void main_init_gic(void)
{
    	volatile unsigned int sdbbp = 1;
	gic_cpuif_init();
	/*while(sdbbp);*/
	gic_distributor_init();
	/*while(sdbbp);*/
	register_sgi();
}
