#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <aw_version.h>
#include <irqs.h>
#include <platform.h>
#include <memory.h>

#include <serial.h>
#include <interrupt.h>

#ifdef CONFIG_DRIVERS_GPIO
#include <hal_gpio.h>
#endif
#ifdef CONFIG_DRIVERS_UART
#include <hal_uart.h>
#endif
#ifdef CONFIG_DRIVERS_MSGBOX
#include <hal_msgbox.h>
#endif
#ifdef CONFIG_DRIVERS_CCMU
#include <hal_clk.h>
#endif
#include <hal_cache.h>

#include "excep.h"

#ifdef CONFIG_COMPONENTS_AW_SYS_CONFIG_SCRIPT
#include <hal_cfg.h>
#endif

#include <compiler.h>

#ifdef CONFIG_DRIVERS_SPINOR
#include <sunxi_hal_spinor.h>
#endif

#ifdef CONFIG_DRIVERS_MSGBOX
#include <hal_msgbox.h>
#endif

#ifdef CONFIG_COMPONENTS_OPENAMP
extern int openamp_init(void);
#endif
#ifdef CONFIG_COMPONENTS_RPBUF
extern int rpbuf_init(void);
#endif
#ifdef CONFIG_RPMSG_CLIENT
#include <openamp/sunxi_helper/rpmsg_master.h>
#endif
void timekeeping_init(void);

int start_kernel_flag = 1;

void E906_Default_IRQHandler(void)
{
    printf("error IRQHandler!!!\r\n");
    while(1);
}

static void print_banner()
{
    printf("\r\n");
    printf(" *******************************************\r\n");
    printf(" ** Welcome to T736-E906 FreeRTOS %-10s**\r\n", TINA_RT_VERSION_NUMBER);
    printf(" ** Copyright (C) 2019-2021 AllwinnerTech **\r\n");
    printf(" **                                       **\r\n");
    printf(" **      starting riscv FreeRTOS          **\r\n");
    printf(" *******************************************\r\n");
    printf("\r\n");
    printf("Date:%s, Time:%s\n", __DATE__, __TIME__);
}

__weak void sunxi_dma_init(void)
{
    return;
}

__weak void sunxi_gpio_init(void)
{
    return;
}

__weak int sunxi_soundcard_init(void)
{
    return 0;
}

__weak void heap_init(void)
{
    return;
}

static void prvSetupHardware(void)
{
    timekeeping_init();

#ifdef CONFIG_DRIVERS_CCMU
    hal_clock_init();
#endif

#ifdef CONFIG_DRIVERS_GPIO
    hal_gpio_init();
#endif
    hal_uart_init(0);
    serial_init();
}

void systeminit(void)
{
}

void start_kernel(void)
{
	portBASE_TYPE ret;
#ifdef CONFIG_COMPONENTS_STACK_PROTECTOR
	void stack_protector_init(void);
	stack_protector_init();
#endif

	extern void hardware_config(void);
	hardware_config();

	systeminit();

	/* Init heap */
	heap_init();

	/* Init hardware devices */
	prvSetupHardware();
	/* Setup kernel components */
	print_banner();

	setbuf(stdout, 0);
	setbuf(stdin, 0);
	setvbuf(stdin, NULL, _IONBF, 0);

#ifdef CONFIG_COMPONENT_CPLUSPLUS
	/* It should be called after the stdout is ready, otherwise the std:cout can't work  */
    int cplusplus_system_init(void);
    cplusplus_system_init();
#endif

#ifdef CONFIG_ARCH_HAVE_ICACHE
	hal_icache_init();
	printf("init Icache\n");
#endif

#ifdef CONFIG_ARCH_HAVE_DCACHE
	hal_dcache_init();
	printf("init Dcache\n");
#endif

#ifdef CONFIG_DRIVERS_MSGBOX
	hal_msgbox_init();
#endif

#ifdef CONFIG_COMPONENTS_OPENAMP
	openamp_init();
#endif

#ifdef CONFIG_COMPONENTS_RPBUF
	rpbuf_init();
#endif

#ifdef CONFIG_RPMSG_CLIENT
	rpmsg_ctrldev_create();
#endif

    start_kernel_flag = 0;

	extern void cpu0_app_entry(void *);
	ret = xTaskCreate(cpu0_app_entry, (signed portCHAR *) "init-thread-0", 1024, NULL, 31, NULL);
	if (ret != pdPASS)
	{
		printf("Error creating task, status was %d\n", ret);
		while (1);
	}

	vTaskStartScheduler();
}
