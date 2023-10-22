#include <stdio.h>
#include <hal_log.h>
#include <hal_cmd.h>
#include <sunxi_hal_rtc.h>

#include <osal/hal_interrupt.h>
#include <arch/arm/mach/sun20iw2p1/irqs-sun20iw2p1.h>

#include "pm_wakesrc.h"
#include "pm_wakecnt.h"
#include "pm_m33_wakesrc.h"

#define PM_PMU_REG_BASE 0x40051400

#ifndef printfFromISR
#define printfFromISR printf
#endif

static hal_irqreturn_t wup_callback(void *data)
{
	volatile u32 val = 0x0;

	/*pm_wakecnt_inc();*/
	printf("call %s\n", __func__);

	/*readl irq status and timer val*/
	val = readl(PM_PMU_REG_BASE + 0x10c);
	if (val & (0x1<<31)) {
		printf("wup timer 0x%x\n", val);

		/*clear pending*/
		writel(0x80000000, PM_PMU_REG_BASE + 0x10c);
		while(readl(PM_PMU_REG_BASE + 0x10c) != 0);
		/*disable wup timer irq*/
		writel(0x0, PM_PMU_REG_BASE + 0x108);
	}

	val = readl(PM_PMU_REG_BASE + 0x118);
	if (val & 0x3ff) {
		printf("wup io 0x%x\n", val);

		/*clear pending*/
		writel(val, PM_PMU_REG_BASE + 0x118);
		/*disable wup io irq*/
		writel(val, PM_PMU_REG_BASE + 0x120);
	}

	return 0;
}

/*
 * WKIO0 --- PA11 
 * WKIO1 --- PA12
 * WKIO2 --- PA13
 * WKIO3 --- PA14
 * WKIO4 --- PA18
 * WKIO5 --- PA19
 * WKIO6 --- PA20
 * WKIO7 --- PA21
 * WKIO8 --- PA22
 * WKIO9 --- PA23
 */

#define GPIO_REG_BASE  0x4004a400
#define PMU_REG_BASE   0x40051400


int cmd_wupio(int argc, char **argv)
{
	int ret;
	int val  = -1;
	uint32_t after, before, addr;
	
	if (argc != 2) {
		printf("invalid param(argc:%d).\n", argc);
		return -1;
	}

	val = atoi(argv[1]);
	if (val < 0 | val > 9) {
		printf("invalid param(val:%d, argv[1]:%s).\n", val, argv[1]);
		return -1;
	}
	printf("set wkio: %d\n", val);

        pm_wakesrc_active(PM_WAKEUP_SRC_WKIOx(val), 0);

	ret = hal_request_irq(0, wup_callback, "wake_up", NULL);
	if (ret)
	    printf("Err: run %s at %d\n", __func__, __LINE__);

	printf("wupio ok!\n");

	return 0;
}



static int cmd_wuptimer(int argc, char **argv)
{
    int ret ;
    uint32_t val;
    static   int timer_irq = 0;


    ret = hal_request_irq(0, wup_callback, "wake_up", NULL);
    if (ret)
	    printf("Err: run %s at %d\n", __func__, __LINE__);


    // write timer val
    writel(320000,  PM_PMU_REG_BASE + 0x10c);

    // enable alarm0
    writel(0x1<<31, PM_PMU_REG_BASE + 0x108);

    // enable nvic
    hal_enable_irq(0);

    pm_wakesrc_active(PM_WAKEUP_SRC_WKTMR, 0);

    printf("wuptimer ok!\n");

    return 0;
}



FINSH_FUNCTION_EXPORT_CMD(cmd_wupio, wupio, pm tools)
FINSH_FUNCTION_EXPORT_CMD(cmd_wuptimer, wuptimer, pm tools)

