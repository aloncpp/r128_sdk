#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "interrupt.h"
#include <portmacro.h>
#include <console.h>
#include <aw_version.h>

#ifdef CONFIG_DRIVERS_MSGBOX
#include <hal_msgbox.h>
#endif

#ifdef CONFIG_COMPONENT_ULOG
#include <ulog.h>
#endif

#include "FreeRTOS.h"
#include "task.h"
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
#include <AudioSystem.h>
#endif

#ifdef CONFIG_DRIVERS_POWER
#include <sunxi_hal_power.h>
#endif



#ifdef CONFIG_COMPONENT_SECURE_STORAGE
extern int secret_storage_init(uint32_t *aes_key);
#endif
#ifdef CONFIG_FLASHC_ENC
extern int flashc_enc_xip_init(uint8_t *nonce_key, uint32_t *aes_key);
#endif
#ifdef CONFIG_DRIVERS_LSPSRAM
extern int platform_psram_chip_config(void);
#endif
extern int hal_flashc_init(void);
extern int pm_init(int argc, char **argv);
extern int amp_init(void);
#ifdef CONFIG_COMPONENTS_TCPIP
extern void cmd_tcpip_init(void);
#endif
extern void sun20i_boot_c906(void);
extern void sun20i_boot_dsp(void);

static void print_banner()
{
    printf("\r\n");
    printf(" *******************************************\r\n");
    printf(" **    Welcome to R128 FreeRTOS %-10s**\r\n", TINA_RT_VERSION_NUMBER);
    printf(" ** Copyright (C) 2019-2022 AllwinnerTech **\r\n");
    printf(" **                                       **\r\n");
    printf(" **      starting armv8m FreeRTOS V0.8    **\r\n");
    printf(" **    Date:%s, Time:%s    **\r\n", __DATE__, __TIME__);
    printf(" *******************************************\r\n");
    printf("\r\n");
}

#ifdef CONFIG_COMPONENTS_AW_OTA_V2
extern int ota_update(void);
extern int ota_init(void);
static void ota_task_thread(void *arg)
{
    int ret = 0;
    printf("ota_update by thread!\n");
    ret = ota_update();
    vTaskDelete(NULL);
}
#endif

void cpu0_app_entry(void * param)
{
    int flash_ready = 0;
	(void)param;

    print_banner();
#ifdef CONFIG_COMPONENTS_AW_DEVFS
    devfs_mount("/dev");
#endif

#ifdef CONFIG_COMPONENTS_PM
    pm_init(1, NULL);
#endif

#ifdef CONFIG_DRIVERS_SPINOR
    flash_ready = !hal_spinor_init(0);
#elif defined(CONFIG_DRIVERS_FLASHC)
    flash_ready = !hal_flashc_init();
#endif

    if (flash_ready) {
#ifdef CONFIG_FLASHC_ENC
    uint32_t enc_aes_key[4] = {0x12345678, 0x12345678, 0x12345678, 0x12345678};
    uint8_t nonce_key[6] = {1, 2, 3, 4, 5, 6};
    flashc_enc_xip_init(nonce_key, enc_aes_key);
#endif
#ifdef CONFIG_COMPONENT_SECURE_STORAGE
    uint32_t ss_aes_key[4]  = {0x12345679, 0x12345679, 0x12345679, 0x12345679};
    secret_storage_init(ss_aes_key);
#endif
#ifdef CONFIG_COMPONENT_LITTLEFS
        littlefs_mount("/dev/UDISK", "/data", true);
#elif defined(CONFIG_COMPONENT_SPIFFS)
        spiffs_mount("/dev/UDISK", "/data", true);
#endif
    }
    else
        printf("flash not ready, skip mount\n");

#ifdef CONFIG_COMPONENTS_TCPIP
    //tcpip stack init
    cmd_tcpip_init();
#endif

#ifdef CONFIG_ARCH_ARMV8M_DEFAULT_BOOT_DSP
    sun20i_boot_dsp();
#endif

#ifdef CONFIG_COMPONENTS_AMP
    amp_init();
#endif
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
    AudioSystemInit();
#endif

#ifdef CONFIG_COMPONENT_CLI
    vCommandConsoleStart(0x1000, HAL_THREAD_PRIORITY_CLI, NULL);
#endif

#ifdef CONFIG_DRIVERS_SDMMC
    sunxi_driver_sdmmc_init();
#endif
#ifdef CONFIG_DRIVERS_POWER
    hal_power_init();
#endif
#ifdef CONFIG_ARCH_ARMV8M_DEFAULT_BOOT_RISCV
    sun20i_boot_c906();
#endif
#ifdef CONFIG_DRIVERS_LSPSRAM
    platform_psram_chip_config();
#endif

#ifdef CONFIG_COMPONENTS_AW_OTA_V2
    if (ota_init()) {
        printf("no need to OTA!\n");
    } else {
        printf("ota_update by thread!\n");
        xTaskCreate(ota_task_thread, "ota_update_task", 8192, NULL, 0, NULL);
    }
#endif

    vTaskDelete(NULL);
}
