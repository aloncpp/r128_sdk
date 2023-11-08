#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "interrupt.h"
#include <portmacro.h>
#include <cli_console.h>
#include <aw_version.h>

#ifdef CONFIG_COMPONENTS_AW_DEVFS
#include <devfs.h>
#endif

#ifdef CONFIG_COMPONENT_LITTLEFS
#include <littlefs.h>
#endif

#ifdef CONFIG_DRIVERS_XRADIO
#ifdef CONFIG_FREQ_TRIM_FROM_SDD
#include <sdd.h>
#include <hal_clk.h>
#endif
#endif

#ifdef CONFIG_COMPONENT_ULOG
#include <ulog.h>
#endif

#ifdef CONFIG_DRIVERS_MSGBOX
#include <hal_msgbox.h>
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "tinatest.h"
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
#include <AudioSystem.h>
#endif
#ifdef CONFIG_DRIVERS_POWER
#include <sunxi_hal_power.h>
#endif
#ifdef CONFIG_DRIVERS_POWER_PROTECT
#include <sunxi_hal_power_protect.h>
#endif

#ifdef CONFIG_COMPONENT_CLI
#include <console.h>
#endif

#ifdef CONFIG_COMPONENTS_WLAN_CSFC
#include <wlan_csfc.h>
#endif

#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
#include <wifimg.h>
#endif

#include <wifi_test.h>
#include <ledc_test.h>
#include <max4466.h>

extern int hal_flashc_init(void);
extern int pm_init(int argc, char **argv);
extern int amp_init(void);
extern int g2d_probe(void);
extern int lcd_fb_probe(void);
extern int disp_probe(void);
extern int AudioSystemInit(void);
extern int sunxi_soundcard_init(void);
extern int cmd_rtplayer_test(int argc, char ** argv);
extern int sunxi_gpadc_key_init(void);
#ifdef CONFIG_COMPONENT_THERMAL
extern int thermal_init(void);
#endif
#ifdef CONFIG_COMPONENTS_TCPIP
extern void cmd_tcpip_init(void);
#endif

static void print_banner()
{
    printf("\r\n");
    printf(" *******************************************\r\n");
    printf(" **    Welcome to R128 FreeRTOS %-10s**\r\n", TINA_RT_VERSION_NUMBER);
    printf(" ** Copyright (C) 2019-2022 AllwinnerTech **\r\n");
    printf(" **                                       **\r\n");
    printf(" **      starting riscv FreeRTOS V0.8     **\r\n");
    printf(" **    Date:%s, Time:%s    **\r\n", __DATE__, __TIME__);
    printf(" *******************************************\r\n");
    printf("\r\n");
}

#ifdef CONFIG_RTPLAYER_TEST
static void boot_play(void)
{
	int argc = 3;
	char *argv[] = {
		"rtplayer_test",
		"/data/boot.mp3",
		"no_shell_input",
	};
	cmd_rtplayer_test(3, argv);
}
#endif

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

#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
static struct ahw_alias_config g_r128_name_alias[] = {
        {
                "default", "playback", 0,
        },
        {
                "default", "amp-cap", 1,
        },
};

void *get_ahw_name_alias(int *num)
{
	*num = ARRAY_SIZE(g_r128_name_alias);

	return g_r128_name_alias;
}
#endif

void ledc_demo(void *arg)
{

    ledc_test_loop();

    vTaskDelete(NULL);
}

void app_demo(void *arg)
{
    int ret = false;

    ret = wifi_init();

    while (ret != 0)
    {
        /* code */
        ret = wifi_test();
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    http_test();

    vTaskDelete(NULL);
}

void cpu0_app_entry(void *param)
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

#ifdef CONFIG_COMPONENTS_AMP
    amp_init();
#endif

#ifdef CONFIG_DRIVERS_SPINOR
    flash_ready = !hal_spinor_init(0);
#elif defined(CONFIG_DRIVERS_FLASHC)
    flash_ready = !hal_flashc_init();
#elif defined(CONFIG_AMP_FLASHC_STUB)
    flash_ready = !hal_flashc_init();
#endif

#ifdef CONFIG_COMPONENT_LITTLEFS
    if (flash_ready)
    {
        littlefs_mount("/dev/UDISK", "/data", true);
        #ifdef CONFIG_COMPONENT_SECURE_STORAGE
        littlefs_mount("/dev/secret", "/secret", true);
        #endif
    }
    else
    {
        printf("flash not ready, skip mount\n");
    }
#endif

#ifdef CONFIG_DRIVERS_XRADIO
#ifdef CONFIG_FREQ_TRIM_FROM_SDD
    int ret;
    uint16_t value;
    struct sdd sdd;

    ret = sdd_request(&sdd);
    if (ret > 0) {
        struct sdd_ie *id = sdd_find_ie(&sdd, SDD_WLAN_DYNAMIC_SECT_ID, SDD_XTAL_TRIM_ELT_ID);
        if (id) {
            value = id->data[0] | (id->data[1] << 8);
            hal_clk_ccu_aon_set_freq_trim(value);
            printf("get xtal trim from SDD, set trim to %d %s!\n\n",
                   value, (value == hal_clk_ccu_aon_get_freq_trim()) ? "success" : "fail");
        } else {
            printf("sdd xtal trim element is not found!\n");
        }
    } else {
        printf("sdd is not found!\n");
    }
    sdd_release(&sdd);
#endif
#endif

#ifdef CONFIG_COMPONENTS_TCPIP
    //tcpip stack init
    cmd_tcpip_init();
#endif

#ifdef CONFIG_COMPONENTS_WLAN_CSFC
	// wifi fast connect
	wlan_csfc();
#endif

#ifdef CONFIG_DRAGONMAT
	if (!access(TINATEST_OK_FILE, R_OK)) {
		printf("tinatest found %s\n", TINATEST_OK_FILE);
	} else {
		printf("start tinatest\n");
		int tinatest(int argc, char **argv);
		tinatest(1, NULL);
	}
#endif

#if (defined CONFIG_TESTCASE_REBOOT) && (!defined CONFIG_DRAGONMAT)
	int tinatest(int argc, char **argv);
	char *argv[2];
	argv[0] = "tt";
	argv[1] = "reboottester";
	tinatest(2, argv);
#endif

#ifdef CONFIG_DRIVERS_SOUND
    sunxi_soundcard_init();
#endif

#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
    AudioSystemInit();
#endif

extern void sunxi_usb_init(void);
#ifdef CONFIG_DRIVERS_USB
    sunxi_usb_init();
#endif

#ifdef CONFIG_COMPONENT_CLI
    vCommandConsoleStart(0x1000, HAL_THREAD_PRIORITY_CLI, NULL);
#endif

#if (defined CONFIG_DRIVERS_SDMMC) && (defined CONFIG_COMPONENT_ELMFAT)
	sunxi_driver_sdmmc_init();
#endif

#ifdef CONFIG_DRIVERS_G2D
	g2d_probe();
#endif

#ifdef CONFIG_DRIVERS_SPILCD
	lcd_fb_probe();
#endif

#ifdef CONFIG_DISP2_SUNXI
	disp_probe();
#endif

#ifdef CONFIG_RTPLAYER_TEST
	boot_play();
#endif

#ifdef CONFIG_COMPONENT_THERMAL
	thermal_init();
#endif
#ifdef CONFIG_DRIVERS_POWER
    hal_power_init();
#endif

#ifdef CONFIG_DRIVERS_POWER_PROTECT
    sunxi_power_protect_init();
#endif

#ifdef CONFIG_DRIVERS_GPADC_KEY
       sunxi_gpadc_key_init();
#endif

#ifdef CONFIG_COMPONENTS_AW_OTA_V2
    if (ota_init()) {
        printf("no need to OTA!\n");
    } else {
        printf("ota_update by thread!\n");
        xTaskCreate(ota_task_thread, "ota_update_task", 8192, NULL, 0, NULL);
    }
#endif
    max4466_init();

    xTaskCreate(app_demo, "app_demo", 8192, NULL, 0, NULL);
    xTaskCreate(ledc_demo, "ledc_demo", 2048, NULL, configMAX_PRIORITIES-1, NULL);

    vTaskDelete(NULL);
}
