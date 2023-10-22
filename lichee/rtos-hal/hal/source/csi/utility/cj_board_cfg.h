
#ifndef __CJ_BOARD_CFG_H__
#define __CJ_BOARD_CFG_H__

#include <stdio.h>
#include "hal_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CJ_DEV_DBG_EN   0
#if (CJ_DEV_DBG_EN == 1)
#define cj_dbg(x, arg...) hal_log_info("[csi_jpeg_debug]"x, ##arg)
#else
#define cj_dbg(x, arg...)
#endif

#define cj_err(x, arg...)  hal_log_err("[csi_jpeg_err]"x, ##arg)
#define cj_warn(x, arg...) hal_log_warn("[csi_jpeg_warn]"x, ##arg)
#define cj_print(x, arg...) hal_log_info("[csi_jpeg]"x, ##arg)


#ifdef CONFIG_DRIVER_SYSCONFIG
#define SYS_GPIO_NUM 14

#define SET_CSI0 "csi0"
#define KEY_CSI_DEV_TWI_ID "vip_dev0_twi_id"
#define KEY_CSI_DEV_PWDN_GPIO "vip_dev0_pwdn"
#define KEY_CSI_DEV_RESET_GPIO "vip_dev0_reset"
#else
/* sensor cfg */
#define SENSOR_I2C_ID           TWI_MASTER_0  // refer pinmux
#define SENSOR_RESET_PIN        GPIO_PA15
#define SENSOR_POWERDOWN_PIN    GPIO_PA14  //GPIO_PA14
#endif

#if defined CONFIG_CSI_CAMERA_GC0308
#include "sensor/drv_gc0308.h"
#define SENSOR_FUNC_INIT		hal_gc0308_init
#define SENSOR_FUNC_DEINIT		hal_gc0308_deinit
#define SENSOR_FUNC_IOCTL		hal_gc0308_ioctl
#define SENSOR_FUNC_MBUD_CFG		hal_gc0308_g_mbus_config
#define SENSOR_FUNC_WIN_SIZE		hal_gc0308_win_sizes
#elif defined CONFIG_CSI_CAMERA_GC0328c
#include "sensor/drv_gc0328c.h"
#define SENSOR_FUNC_INIT		hal_gc0328c_init
#define SENSOR_FUNC_DEINIT		hal_gc0328c_deinit
#define SENSOR_FUNC_IOCTL		hal_gc0328c_ioctl
#define SENSOR_FUNC_MBUD_CFG		hal_gc0328c_g_mbus_config
#define SENSOR_FUNC_WIN_SIZE		hal_gc0328c_win_sizes
#elif defined CONFIG_CSI_CAMERA_GC2145
#include "sensor/drv_gc2145.h"
#define SENSOR_FUNC_INIT		hal_gc2145_init
#define SENSOR_FUNC_DEINIT		hal_gc2145_deinit
#define SENSOR_FUNC_IOCTL		hal_gc2145_ioctl
#define SENSOR_FUNC_MBUD_CFG		hal_gc2145_g_mbus_config
#define SENSOR_FUNC_WIN_SIZE		hal_gc2145_win_sizes
#endif

#define CSI_JPEG_CLK		96*1000*1000

#ifdef __cplusplus
}
#endif
#endif

