#include <stdio.h>
#include <hal_log.h>
#include <hal_cmd.h>

#include <osal/hal_interrupt.h>
#include <arch/arm/mach/sun20iw2p1/irqs-sun20iw2p1.h>

#include <sunxi_hal_rtc.h>

#include "pm_wakesrc.h"
#include "pm_m33_wakesrc.h"

#define PM_RTC_BASE 0x40051000

#define PM_RTC_YYMMDD  (PM_RTC_BASE + 0x10)
#define PM_RTC_HHMMSS  (PM_RTC_BASE + 0x14)

#define ALM0_CNT       (PM_RTC_BASE + 0x20)
#define ALM0_CUR       (PM_RTC_BASE + 0x24)
#define ALM0_EN        (PM_RTC_BASE + 0x28)
#define ALM0_IRQEN     (PM_RTC_BASE + 0x2c)
#define ALM0_IRQST     (PM_RTC_BASE + 0x30)

#define RTC_ALM0_IRQ (10)
#define RTC_ALM1_IRQ (11)

#ifndef printfFromISR
#define printfFromISR printf
#endif

static hal_irqreturn_t alarm0_callback(void *no)
{
	printf("call %s\n", __func__);

	//writel(0x1, 0x40051000+0x30);

	/*close irq*/
	hal_disable_irq(RTC_ALM0_IRQ);

	writel( 0, 0x40051000+0x28);

	return 0;
}

static hal_irqreturn_t alarm1_callback(void *no)
{
	printf("call %s\n", __func__);

	//writel(0x1, 0x40051000+0x4c);

	hal_disable_irq(RTC_ALM1_IRQ);

	writel( 0, 0x40051000+0x48);
	return 0;
}




static int cmd_rtc_alarm_init(int argc, char **argv)
{
    int ret ;
    ret = hal_request_irq(10, alarm0_callback, "alarm0", NULL);
    if (ret != 10)
        printf("Err: run %s at %d\n", __func__, __LINE__);

    pm_wakesrc_active(PM_WAKEUP_SRC_ALARM0, 0);

    ret = hal_request_irq(11, alarm1_callback, "alarm1", NULL);
    if (ret != 11)
        printf("Err: run %s at %d\n", __func__, __LINE__);

    pm_wakesrc_active(PM_WAKEUP_SRC_ALARM1, 0);


    printf("alarm init ok!\n");

}

static int cmd_rtc_alarm0(int argc, char **argv)
{
    uint32_t val;

    // why it is 1min3s??
    writel(10, 0x40051000+0x20);

    // enable irq en
    writel( 1, 0x40051000+0x2c);
    // enable alarm0
    writel( 1, 0x40051000+0x28);
    // enable wakeup
    val = readl(0x40051000 + 0x50);
    writel(0x1, 0x40051000 + 0x50);

    // enable nvic
    hal_enable_irq(RTC_ALM0_IRQ);

    printf("alarm0 ok!\n");

    return 0;
}

static int cmd_rtc_alarm1(int argc, char **argv)
{
    uint32_t val;

    //read time
    val = readl(0x40051000+0x14);

    // 1 min
    val = (val + 0x100) & 0x00ffffff;
    writel(val, 0x40051000+0x40);

    // enable irq en
    writel(0x7f, 0x40051000+0x44);

    // enable alarm0
    writel( 1, 0x40051000+0x48);

    // enable wakeup
    val = readl(0x40051000 + 0x50);
    writel(0x2, 0x40051000 + 0x50);

    // enable nvic
    hal_enable_irq(RTC_ALM1_IRQ);

    printf("alarm1 ok!\n");

    return 0;
}


FINSH_FUNCTION_EXPORT_CMD(cmd_rtc_alarm_init,  alarm_init,  alarm in rtc tools)
FINSH_FUNCTION_EXPORT_CMD(cmd_rtc_alarm0,  alarm0,  alarm in rtc tools)
FINSH_FUNCTION_EXPORT_CMD(cmd_rtc_alarm1,  alarm1,  alarm in rtc tools)


