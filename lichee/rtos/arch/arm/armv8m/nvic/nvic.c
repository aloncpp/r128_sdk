#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <interrupt.h>
#include <irqs.h>
#include <platform.h>
#include <memory.h>
#include <core_cm33.h>
#include <io.h>
#include <compiler.h>

#include <interrupt_nvic.h>

#ifdef CONFIG_COMPONENTS_PM
#include "pm_base.h"
#include "pm_syscore.h"
#include "pm_state.h"
#endif

#define printk printf

#define NVIC_PERIPH_IRQ_OFFSET (16)
#define NVIC_IRQ_NUM (SUNXI_IRQ_MAX + NVIC_PERIPH_IRQ_OFFSET)

void enter_interrupt_handler(void);
void exit_interrupt_handler(void);

extern uint32_t irq_vector[NVIC_IRQ_NUM];

struct arch_irq_desc {
    hal_irq_handler_t handle_irq;
    void *data;
};

static struct arch_irq_desc arch_irqs_desc[SUNXI_IRQ_MAX];

hal_irqreturn_t nvic_null_handler(void *data)
{
	return HAL_IRQ_ERR;
}

void nvic_common_handler(void)
{
	int id = (__get_IPSR() & 0x1ff) - NVIC_PERIPH_IRQ_OFFSET;

#ifdef CONFIG_COMPONENTS_PM
	if (pm_state_get() != PM_STATUS_RUNNING)
		pm_hal_record_wakeup_irq(id);
#endif

	enter_interrupt_handler();
	if (arch_irqs_desc[id].handle_irq &&
			arch_irqs_desc[id].handle_irq != nvic_null_handler) {
		arch_irqs_desc[id].handle_irq(arch_irqs_desc[id].data);
	} else {
		printf("no handler for irq %d\n", id);
	}
	exit_interrupt_handler();
	return;
}

#define CPU_160M_100US_CYCLES 	(5300)

static void cpu_delay(volatile int cycles)
{
   while(cycles--);
}

void null_exception(void)
{
	printk("No irq registered handler for this calling !!\n");
	printk("nvic en0 %x\n", readl(NVIC_SET_EN(0)));
	printk("nvic en1 %x\n", readl(NVIC_SET_EN(1)));
	printk("nvic act %x\n", readl(NVIC_PEND_ACTIVE(0)));
	printk("nvic act %x\n", readl(NVIC_PEND_ACTIVE(1)));
	cpu_delay(CPU_160M_100US_CYCLES * 2);//wait print
}

/* hard fault handler */
void HardFaultHandler(void)
{
	printk("assert a hard fault!\n");
	printk("MSP =0x%08x\n",__get_MSP());
	printk("xPSR=0x%08x\n",__get_xPSR());
	printk("CFSR=0x%08x\n",SCB->CFSR);

	printk("system will be reset!\n");
	cpu_delay(CPU_160M_100US_CYCLES * 2);//wait print
	//writel(0x1101, WDOG_MODE);//reset system
	//while(readl(WDOG_MODE) & 1);
	while(1);
}

/* memory error handler */
void MemoryFaultHandler(void)
{
	printk("assert a memory fault!\n");
	printk("system will be reset!\n");
	cpu_delay(CPU_160M_100US_CYCLES * 2);//wait print
	//writel(0x1101, WDOG_MODE);
	//while(readl(WDOG_MODE) & 1);
	while(1);
}

/* bus error handler */
void BusFaultHandler(void)
{
	printk("assert a bus fault!\n");
	printk("system will be reset!\n");
	cpu_delay(CPU_160M_100US_CYCLES * 2);//wait print
	//writel(0x1101, WDOG_MODE);
	//while(readl(WDOG_MODE) & 1);
	while(1);
}

/* usage error handler */
void UsageFaultHandler(void)
{
	printk("assert a usage fault!\n");
	printk("system will be reset!\n");
	cpu_delay(CPU_160M_100US_CYCLES * 2);//wait print
	//writel(0x1101, WDOG_MODE);
	//while(readl(WDOG_MODE) & 1);
	while(1);
}

/* SecureFault handler */
__attribute__((weak)) void SecureFaultHandler(void)
{
	printk("assert a Secure Fault!\n");
	printk("system will be reset!\n");
	cpu_delay(CPU_160M_100US_CYCLES * 2);//wait print
	//writel(0x1101, WDOG_MODE);
	//while(readl(WDOG_MODE) & 1);
	while(1);
}

int32_t irq_request(uint32_t irq_no, hal_irq_handler_t hdle, void *data)
{
	uint32_t *vectors = (uint32_t *)SCB->VTOR;

	if (irq_no < NVIC_IRQ_NUM) {
		vectors[irq_no + NVIC_PERIPH_IRQ_OFFSET] = (uint32_t)hdle;
		return (int32_t)irq_no;
	}
	printk("Wrong irq NO.(%d) to request !!\n", irq_no);
	return -1;
}

int32_t arch_request_irq(int32_t irq, hal_irq_handler_t handler, void *data)
{
	if (irq + NVIC_PERIPH_IRQ_OFFSET < NVIC_IRQ_NUM) {
		if (handler && arch_irqs_desc[irq].handle_irq == nvic_null_handler) {
			arch_irqs_desc[irq].handle_irq = handler;
			arch_irqs_desc[irq].data = data;
		}
		return irq;
	}
	printk("Wrong irq NO.(%d) to request !!\n", irq);
	return -1;
}

void arch_free_irq(int32_t irq)
{
	uint32_t *vectors = (uint32_t *)SCB->VTOR;

	if (irq + NVIC_PERIPH_IRQ_OFFSET < NVIC_IRQ_NUM)
		arch_irqs_desc[irq].handle_irq = nvic_null_handler;

	return;
}

void arch_enable_irq(int32_t irq)
{
	if (irq + NVIC_PERIPH_IRQ_OFFSET >= NVIC_IRQ_NUM) {
		printf("irq NO.(%d) > NVIC_IRQ_NUM(%d) !!\n", irq, NVIC_IRQ_NUM);
		return;
	}
	NVIC_EnableIRQ(irq);
}

void arch_disable_irq(int32_t irq)
{
	if (irq + NVIC_PERIPH_IRQ_OFFSET >= NVIC_IRQ_NUM) {
		printk("irq NO.(%d) > NVIC_IRQ_NUM(%d) !!\n", irq, NVIC_IRQ_NUM);
		return;
	}

	NVIC_DisableIRQ(irq);
}

void arch_irq_set_prioritygrouping(uint32_t group)
{
	NVIC_SetPriorityGrouping(group);
}

uint32_t arch_irq_get_prioritygrouping(void)
{
	return NVIC_GetPriorityGrouping();
}

void arch_irq_set_priority(int32_t irq, uint32_t preemptpriority, uint32_t subpriority)
{
	uint32_t group, priority;

	group = hal_irq_get_prioritygrouping();
	priority = NVIC_EncodePriority (group, preemptpriority, subpriority);

	NVIC_SetPriority(irq, priority);
}

void arch_irq_get_priority(int32_t irq, uint32_t prioritygroup, uint32_t *p_preemptpriority, uint32_t *p_subpriority)
{
	uint32_t priority;

	priority = NVIC_GetPriority(irq);
	NVIC_DecodePriority (priority, prioritygroup, p_preemptpriority, p_subpriority);
}

void arch_nvic_irq_set_priority(int32_t irq, uint32_t priority)
{
	NVIC_SetPriority(irq, priority);
}

uint32_t arch_nvic_irq_get_priority(int32_t irq)
{
	return NVIC_GetPriority(irq);
}

uint32_t arch_nvic_get_enable_irq(int32_t irq)
{
	if (irq + NVIC_PERIPH_IRQ_OFFSET >= NVIC_IRQ_NUM) {
		printk("irq NO.(%d) > NVIC_IRQ_NUM(%d) !!\n", irq, NVIC_IRQ_NUM);
		return 0;
	}

	return NVIC_GetEnableIRQ(irq);
}

hal_irq_handler_t irq_get_handler(uint32_t irq_no)
{
    uint32_t *vectors = (uint32_t*)SCB->VTOR;

    return (hal_irq_handler_t)(vectors[irq_no + NVIC_PERIPH_IRQ_OFFSET]);
}

int32_t irq_free(uint32_t irq_no)
{
	uint32_t *vectors = (uint32_t *)SCB->VTOR;

	if (irq_no < NVIC_IRQ_NUM) {
		vectors[irq_no + NVIC_PERIPH_IRQ_OFFSET] = (uint32_t)null_exception;
		return (int32_t)irq_no;
	}
	printk("Wrong irq NO.(%d) to free !!\n", irq_no);
	return -1;
}

__nonxip_text
int32_t irq_enable(uint32_t irq_no)
{
	if (irq_no + NVIC_PERIPH_IRQ_OFFSET >= NVIC_IRQ_NUM) {
		printk("irq NO.(%d) > NVIC_IRQ_NUM(%d) !!\n", irq_no, NVIC_IRQ_NUM);
		return -1;
	}
	NVIC_EnableIRQ(irq_no);

	return 0;
}

__nonxip_text
int32_t irq_disable(uint32_t irq_no)
{
	if (irq_no + NVIC_PERIPH_IRQ_OFFSET >= NVIC_IRQ_NUM) {
		printk("irq NO.(%d) > NVIC_IRQ_NUM(%d) !!\n", irq_no, NVIC_IRQ_NUM);
		return -1;
	}

	NVIC_DisableIRQ(irq_no);

	return 0;
}

void arch_enable_all_irq(void)
{
    asm volatile ("cpsie i");
}

void arch_disable_all_irq(void)
{
    asm volatile ("cpsid i");
}

unsigned long arch_irq_is_disable(void)
{
    if (__get_BASEPRI() || __get_PRIMASK() || __get_FAULTMASK())
        return 1;
    return 0;
}

void arch_set_pending(uint32_t irq_no)
{
	NVIC_SetPendingIRQ(irq_no);
}

int arch_is_pending(uint32_t irq_no)
{
	return (int)(NVIC_GetPendingIRQ(irq_no));
}

void arch_clear_pending(uint32_t irq_no)
{
	NVIC_ClearPendingIRQ(irq_no);
}

#ifdef CONFIG_COMPONENTS_PM
#define M33_INTERRUPT_SET_ENABLE_IRQ_BASE	(0xe000e100)
#define M33_INTERRUPT_SET_ENABLE_IRQ_LEN	(0x3c)
#define M33_NVIC_ISER_NUM			(16)
#define M33_INTERRUPT_PRIORITY_BASE		(0xe000e400)
#define M33_INTERRUPT_PRIORITY_LEN		(0x1dc)
#define M33_NVIC_IPR_NUM			(120)
#define M33_SYSTEM_HANDLER_PRIORITY_REG		(0xe000ed20)
static uint32_t interrupt_set_enable_reg[M33_NVIC_ISER_NUM];
static uint32_t interrupt_priority_reg[M33_NVIC_IPR_NUM];
static uint32_t system_handler_priority;
static int nvic_suspend(void *data, suspend_mode_t mode)
{
	int i = 0;
	uint32_t *pdata = (uint32_t *)0;
	uint32_t start = 0;

	pdata = (uint32_t *)M33_INTERRUPT_SET_ENABLE_IRQ_BASE;
	start = (uint32_t)pdata;
	i = 0;
	while (pdata < (uint32_t *)(start + M33_INTERRUPT_SET_ENABLE_IRQ_LEN)) {
		if (i == M33_NVIC_ISER_NUM)
			return -1;

		interrupt_set_enable_reg[i] = *pdata;
		pdata++;
		i++;
	}

	pdata = (uint32_t *)M33_INTERRUPT_PRIORITY_BASE;
	start = (uint32_t)pdata;
	i = 0;
	while (pdata < (uint32_t *)(start + M33_INTERRUPT_PRIORITY_LEN)) {
		if (i == M33_NVIC_IPR_NUM)
			return -1;

		interrupt_priority_reg[i] = *pdata;
		pdata++;
		i++;
	}

	system_handler_priority = readl(M33_SYSTEM_HANDLER_PRIORITY_REG);

	return 0;
}

static void nvic_resume(void *data, suspend_mode_t mode)
{
	int i = 0;
	uint32_t *pdata = (uint32_t *)0;
	uint32_t start = 0;

	SCB->VTOR = (uint32_t)irq_vector;
	for (i = NVIC_PERIPH_IRQ_OFFSET; i < NVIC_IRQ_NUM; i++)
		irq_disable(i - NVIC_PERIPH_IRQ_OFFSET);

	/* restore priority group */
	writel(system_handler_priority, M33_SYSTEM_HANDLER_PRIORITY_REG);

	/* restore irq priority */
	pdata = (uint32_t *)M33_INTERRUPT_PRIORITY_BASE;
	start = (uint32_t)pdata;
	i = 0;
	while (pdata < (uint32_t *)(start + M33_INTERRUPT_PRIORITY_LEN)) {
		*pdata = interrupt_priority_reg[i];
		pdata++;
		i++;
	}

	/* Enable some system fault exceptions */
	SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk |
	              SCB_SHCSR_MEMFAULTENA_Msk;
	SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;

	/* restore irq enable */
	pdata = (uint32_t *)M33_INTERRUPT_SET_ENABLE_IRQ_BASE;
	start = (uint32_t)pdata;
	i = 0;
	while (pdata < (uint32_t *)(start + M33_INTERRUPT_SET_ENABLE_IRQ_LEN)) {
		*pdata = interrupt_set_enable_reg[i];
		pdata++;
		i++;
	}
}

static struct syscore_ops nvic_syscore_ops = {
	.name = "nvic_syscore_ops",
	.suspend = nvic_suspend,
	.resume = nvic_resume,
};
#endif /* CONFIG_COMPONENTS_PM */

void irq_init(void)
{
	int i;

	SCB->VTOR = (uint32_t)irq_vector;

	for (i = NVIC_PERIPH_IRQ_OFFSET; i < NVIC_IRQ_NUM; i++) {
		irq_disable(i - NVIC_PERIPH_IRQ_OFFSET);
		hal_nvic_irq_set_priority(i - NVIC_PERIPH_IRQ_OFFSET, CONFIG_ARCH_ARM_ARMV8M_IRQ_DEFAULT_PRIORITY);
		irq_vector[i] = (uint32_t)nvic_common_handler;
		arch_irqs_desc[i - NVIC_PERIPH_IRQ_OFFSET].handle_irq = nvic_null_handler;
		arch_irqs_desc[i - NVIC_PERIPH_IRQ_OFFSET].data = NULL;
	}

	/* Enable some system fault exceptions */
	SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk |
	              SCB_SHCSR_MEMFAULTENA_Msk;
	SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;

#ifdef CONFIG_COMPONENTS_PM
	int ret;
	ret = pm_syscore_register(&nvic_syscore_ops);
	if (ret)
		printf("WARNING: nvic syscore ops registers failed\n");
#endif
}

