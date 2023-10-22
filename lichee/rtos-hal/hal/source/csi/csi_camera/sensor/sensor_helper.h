#ifndef __SENSOR__HELPER__H__
#define __SENSOR__HELPER__H__

#include <stdio.h>
#include "sunxi_hal_twi.h"
#include "hal_timer.h"
#include "hal_time.h"
#include "hal_log.h"
#include "sensor/camera_sensor.h"
#include "../csi.h"

#define REG_DLY             0xffff

#define TWI_BITS_8          8
#define TWI_BITS_16         16

typedef struct {
	twi_port_t twi_port;
	unsigned char addr_width;
	unsigned char data_width;
	unsigned char slave_addr;
	hal_clk_t mclk;
	hal_clk_t mclk_src;
	unsigned int mclk_rate;
	char name[32];
#ifdef CONFIG_PM
	uint8_t suspend;
	struct soc_device dev;
#endif
} sensor_private;


struct regval_list {
	unsigned short addr;
	unsigned short data;
};

#define SENSOR_DEV_DBG_EN   0
#if (SENSOR_DEV_DBG_EN == 1)
#define sensor_dbg(x, arg...) hal_log_info("[%s_debug]"x, SENSOR_NAME, ##arg)
#else
#define sensor_dbg(x, arg...)
#endif

#define sensor_err(x, arg...)  hal_log_err("[%s_err]"x, SENSOR_NAME, ##arg)
#define sensor_print(x, arg...) hal_log_info("[%s]"x, SENSOR_NAME, ##arg)

twi_status_t sensor_twi_init(twi_port_t port, unsigned char sccb_id);
twi_status_t sensor_twi_exit(twi_port_t port);

int sensor_read(sensor_private *sensor_priv, unsigned short addr, unsigned short *value);
int sensor_write(sensor_private *sensor_priv, unsigned short addr, unsigned short value);
int sensor_write_array(sensor_private *sensor_priv, struct regval_list *regs, int array_size);
void csi_set_mclk_rate(sensor_private *sensor_priv, unsigned int clk_rate);
int csi_set_mclk(sensor_private *sensor_priv, unsigned int on);

#endif

