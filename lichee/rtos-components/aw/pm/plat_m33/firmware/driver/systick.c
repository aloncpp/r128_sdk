#include "type.h"

#define M33_SYST_CSR   0xe000e010
#define M33_SYST_RVR   0xe000e014
#define M33_SYST_CVR   0xe000e018
#define M33_SYST_CALIB 0xe000e01c

#define M33_SCB_CPUID  0xe000ed00
#define M33_SCB_ICSR   0xe000ed04
#define M33_SCB_VTOR   0xe000ed08
#define M33_SCB_AIRCR  0xe000ed0c
#define M33_SCB_SCR    0xe000ed10
#define M33_SCB_CCR    0xe000ed14

enum {
	CONTEXT_SYSTICK_CSR,
	CONTEXT_SYSTICK_RVR,

	CONTEXT_TOTAL_NUMBER,
};

u32 volatile cpu_systick_context[CONTEXT_TOTAL_NUMBER];

void cpu_disable_systick(void)
{
	volatile u32 value;

	/*save reload value*/
	cpu_systick_context[CONTEXT_SYSTICK_CSR] = readl(M33_SYST_CSR);
	cpu_systick_context[CONTEXT_SYSTICK_RVR] = readl(M33_SYST_RVR);

	/*close timer, intterupt*/
	value = readl(M33_SYST_CSR);
	value = value & ~0x3;
	writel(value, M33_SYST_CSR);

	/*clear pending*/
	value = readl(M33_SCB_ICSR);
	value = value & (0x1<<25);
	writel(value, M33_SCB_ICSR);
}

void cpu_enable_systick(void)
{
	volatile u32 value;

	/* restore reload value*/
	writel(cpu_systick_context[CONTEXT_SYSTICK_RVR], M33_SYST_RVR);

	/*clear counter*/
	writel(0x0, M33_SYST_CVR);

	/* restore control*/
	writel(cpu_systick_context[CONTEXT_SYSTICK_CSR], M33_SYST_CSR);
}

