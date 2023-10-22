#include <serial.h>
#include <interrupt.h>
#include <stdio.h>
#include <stdint.h>
#include <FreeRTOS.h>
#include <string.h>
#include <task.h>
#include "processor.h"
#include "interrupt.h"
#include <arch/arm/armv7a/mmu_cache.h>
#include "aw_version.h"
#include "sunxi_hal_htimer.h"

#include <compiler.h>

#ifdef CONFIG_COMPONENTS_MULTI_CONSOLE
#include <cli_console.h>
#endif

#ifdef CONFIG_DRIVERS_SPINOR
#include <sunxi_hal_spinor.h>
#endif

#ifdef CONFIG_COMPONENT_SPIFFS
#include <spiffs.h>
#endif

#ifdef CONFIG_COMPONENTS_AW_DEVFS
#include <devfs.h>
#endif

#ifdef CONFIG_COMPONENT_LITTLEFS
#include <littlefs.h>
#endif

#ifdef CONFIG_DRIVERS_MUTEKEY
#include <mutekey.h>
#endif
#include <hal_clk.h>

#ifdef CONFIG_COMPONENT_ENV
#include <env.h>
#endif

#ifdef CONFIG_KASAN
#include <kasan_rtos.h>
#endif

static void print_banner()
{
    printf("\r\n");
    printf(" *******************************************\r\n");
    printf(" **        Welcome to Tina-RT %s      **\r\n", TINA_RT_VERSION_NUMBER);
    printf(" ** Copyright (C) 2019-2020 AllwinnerTech **\r\n");
    printf(" **                                       **\r\n");
    printf(" **         starting FreeRTOS             **\r\n");
    printf(" *******************************************\r\n");
    printf("\r\n");
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

static void prvSetupHardware( void )
{
    init_gic();
    hal_dma_init();
    hal_gpio_init();
}

#ifdef configKERNEL_SUPPORT_SMP
void timer_tick_init(void);
void cpu1_app_entry(void*);
void cpu1_c_main(void)
{
    portBASE_TYPE ret;

    freert_spin_lock(&bootup_lock);

    timer_tick_init();

    ret = xTaskCreate(cpu1_app_entry, (signed portCHAR *) "init-thread-1", 4096, NULL, 31, NULL);
    if (ret != pdPASS)
    {
        SMP_DBG("Error creating task, status was %d\n", ret);
        soft_break();
    }

    //smp mode triggered.
    vTaskStartScheduler();

    CODE_UNREACHABLE();
}
#else
void cpu1_c_main(void)
{
}
#endif

#ifdef CONFIG_DRIVERS_SPINOR_CACHE
static TimerHandle_t xTimer_flush = NULL;
static void NorSyncTimerCallback( TimerHandle_t xTimerHandle )
{
	/* Avoid compiler warnings resulting from the unused parameter. */
	( void ) xTimerHandle;

	hal_spinor_sync();

	return;
}

void NorSyncTimerStop(void)
{
    if (xTimer_flush)
        xTimerStop( xTimer_flush, 0);
}

void NorSyncTimerStart(void)
{
    if (xTimer_flush)
        xTimerStart( xTimer_flush, 0);
}
#endif

void bsp_init(void)
{
#ifdef CONFIG_COMPONENT_CPLUSPLUS
    int cplusplus_system_init(void);
    cplusplus_system_init();
#endif

#ifdef CONFIG_COMPONENT_ULOG
    ulog_backend_init();
#endif

#ifdef CONFIG_DRIVERS_CCMU
    hal_clock_init();
#endif
#ifdef CONFIG_DRIVERS_HTIMER
    hal_htimer_init();
#endif
#ifdef CONFIG_DRIVERS_SOUND
    sunxi_soundcard_init();
#endif
#ifdef CONFIG_DRIVERS_SPINOR
    hal_spinor_init(0);
#ifdef CONFIG_DRIVERS_SPINOR_CACHE
    xTimer_flush = xTimerCreate( "NorSyncTimer", pdMS_TO_TICKS(2000), pdTRUE, NULL, NorSyncTimerCallback );
    if (xTimer_flush == NULL)
    {
        SMP_DBG("Create Timer for flush Failure.\n");
        soft_break();
        return;
    }
    else
    {
        NorSyncTimerStart();
    }
#endif
#endif

#ifdef CONFIG_DRIVERS_NAND_FLASH
	hal_nand_init();
#endif

    SMP_DBG("\nfilesystem init begin...\n");
#ifdef CONFIG_COMPONENTS_AW_DEVFS
    devfs_mount("/dev");
#endif

#ifdef CONFIG_COMPONENT_LITTLEFS
    littlefs_mount("/dev/UDISK", "/data", true);
    littlefs_mount("/dev/private", "/private", true);
#ifdef CONFIG_RESERVE_IMAGE_PART
#ifdef CONFIG_COMPONENT_ENV
    if (!fw_env_open()) {
        char *now = fw_getenv("rtosAB_now");
        fw_env_close();

        if (now) {
            if (!strncmp("A", now, 1)) {
                littlefs_mount("/dev/reserveA", CONFIG_RESERVE_FILESYSTEM_PATH, true);
            } else if (!strncmp("B", now, 1)) {
                littlefs_mount("/dev/reserveB", CONFIG_RESERVE_FILESYSTEM_PATH, true);
            }
        }
    }
#endif
#endif

#ifdef CONFIG_COMPONENTS_AW_SS
    littlefs_mount("/dev/secret", "/secret", true);
#endif

#elif CONFIG_COMPONENT_SPIFFS
    spiffs_mount("/dev/UDISK", "/data", true);

#ifdef CONFIG_COMPONENTS_AW_SS
    lfs_mount("/dev/secret", "/secret", true);
#endif

#endif

#ifdef CONFIG_COMPONENTS_RAMFS_AUTO_MOUNT
    ramfs_create_mount(CONFIG_COMPONENTS_RAMFS_PATH, CONFIG_COMPONENTS_RAMFS_SIZE * 1024);
#endif

    SMP_DBG("filesystem mount success.\n");
#ifdef CONFIG_COMPONENT_ENV
    fw_env_open();
#if 0
    char *upgrade_available;
    upgrade_available = fw_getenv("upgrade_available");
    if (strncmp(upgrade_available, "0", strlen("0")) != 0) {
        SMP_DBG("upgrade_available = %s, disable it.\n", upgrade_available);
        fw_env_write("upgrade_available", "0");
        fw_env_flush();
    }
#endif

    char *bootcount;
    bootcount = fw_getenv("bootcount");
    if (strncmp(bootcount, "0", strlen("0")) != 0) {
        SMP_DBG("bootcount = %s, clear it.\n", bootcount);
        fw_env_write("bootcount", "0");
        fw_env_flush();
    }

    fw_env_close();
#endif

#ifdef CONFIG_DRIVERS_USB
    sunxi_usb_init();
#endif

#ifdef CONFIG_BT_XR829
	int xr829_bt_driver_register(void);
	xr829_bt_driver_register();
#endif
    void trigger_cpu1_bootup(void);
    trigger_cpu1_bootup();
}

#ifdef CONFIG_RTT_SLAB
extern void rt_malloc_sethook(void (*hook)(void *ptr, uint32_t size));
static void heap_clear_hook(void *ptr, uint32_t size)
{
    if (ptr != NULL && size)
    {
    	memset(ptr, 0x00, size);
    }
    return;
}
#endif

void trigger_cpu1_bootup(void)
{
#ifdef configKERNEL_SUPPORT_SMP
    void xport_dcache_clean_all(void);
    void sencond_cpu_bootup(void);
    xport_dcache_clean_all();
    sencond_cpu_bootup();
#endif
}

void start_kernel(void)
{
    portBASE_TYPE ret;

#ifdef CONFIG_COMPONENTS_STACK_PROTECTOR
    void stack_protector_init(void);
    stack_protector_init();
#endif

#ifdef CONFIG_COMPONENTS_MULTI_CONSOLE
    multiple_console_init();
#endif

    /* Init heap */
    heap_init();

#ifdef CONFIG_KASAN
    kasan_init();
#endif

    /* Init hardware devices */
    prvSetupHardware();

    /* Setup kernel components */
    serial_init();
    print_banner();

#ifdef CONFIG_DRIVERS_MUTEKEY
    /* Jump to fel if mute key is detected*/
    mute_key_jump_fel();
#endif

    setbuf(stdout, 0);
    setbuf(stdin, 0);
    setvbuf(stdin, NULL, _IONBF, 0);

    printf("memory attribute: NORMAL_MEM = 0x%08x\n"
        "                     DEVICE_MEM = 0x%08x\n"
        "                    L1_COHERENT = 0x%08x\n"
        "                 L1_NONCOHERENT = 0x%08x\n"
        "                      L1_DEVICE = 0x%08x\n", \
        NORMAL_MEM, DEVICE_MEM, L1_COHERENT, L1_NONCOHERENT, L1_DEVICE);

#ifdef CONFIG_RTT_SLAB
    rt_malloc_sethook(heap_clear_hook);
#endif
    void cpu0_app_entry(void*);
    ret = xTaskCreate(cpu0_app_entry, (signed portCHAR *) "init-thread-0", 4096, NULL, 31, NULL);
    if (ret != pdPASS)
    {
        SMP_DBG("Error creating task, status was %d\n", ret);
        soft_break();
    }

    freert_spin_lock(&bootup_lock);
    vTaskStartScheduler();
    CODE_UNREACHABLE();
}
