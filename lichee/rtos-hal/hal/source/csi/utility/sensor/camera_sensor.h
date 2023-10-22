/**
  * @file  camera_sensor.h
  * @author  XRADIO IOT WLAN Team
  */

/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CAMERA_SENSOR_H__
#define __CAMERA_SENSOR_H__

#include "hal_gpio.h"
#include "hal_def.h"
#include "sunxi_hal_twi.h"

#define MBUS_SEPARATE_SYNCS		(1 << 0)  //DC
#define MBUS_EMBEDDED_SYNCS		(1 << 1)  //BT656

#define MBUS_HSYNC_ACTIVE_HIGH		(1 << 2)
#define MBUS_HSYNC_ACTIVE_LOW		(1 << 3)
#define MBUS_VSYNC_ACTIVE_HIGH		(1 << 4)
#define MBUS_VSYNC_ACTIVE_LOW		(1 << 5)
#define MBUS_PCLK_SAMPLE_RISING		(1 << 6)
#define MBUS_PCLK_SAMPLE_FALLING	(1 << 7)

#define CSI_GPIO_HIGH        1
#define CSI_GPIO_LOW         0

#define MEDIA_BUS_FMT_UYVY8_2X8			0x2006
#define MEDIA_BUS_FMT_VYUY8_2X8			0x2007
#define MEDIA_BUS_FMT_YUYV8_2X8			0x2008
#define MEDIA_BUS_FMT_YVYU8_2X8			0x2009

typedef enum {
	LIGHT_AUTO,
	LIGHT_SUNNY,
	LIGHT_COLUDY,
	LIGHT_OFFICE,
	LIGHT_HOME,
} SENSOR_LightMode;

typedef enum {
	COLOR_SATURATION_0, /*!< -2 */
	COLOR_SATURATION_1, /*!< -1*/
	COLOR_SATURATION_2, /*!< The default */
	COLOR_SATURATION_3, /*!< 1 */
	COLOR_SATURATION_4, /*!< 2 */
} SENSOR_ColorSaturation;

typedef enum {
	BRIGHT_0,  /*!< birghtness -2 */
	BRIGHT_1,  /*!< -1 */
	BRIGHT_2,  /*!< The default */
	BRIGHT_3,  /*!< 1 */
	BRIGHT_4,  /*!< 2 */
} SENSOR_Brightness;

typedef enum {
	CONTARST_0, /*!< -2 */
	CONTARST_1, /*!< -1 */
	CONTARST_2, /*!< The default */
	CONTARST_3, /*!< 1 */
	CONTARST_4, /*!< 2 */
} SENSOR_Contarst;

/**
  * @brief effects.
  */
typedef enum {
	IMAGE_NOMAL,
	IMAGE_NEGATIVE,
	IMAGE_BLACK_WHITE,
	IMAGE_SLANT_RED,
	IMAGE_SLANT_GREEN,
	IMAGE_SLANT_BLUE,
	IMAGE_VINTAGE,
}SENSOR_SpecailEffects;

typedef struct {
	uint32_t width;
	uint32_t height;
} SENSOR_PixelSize;

typedef enum {
	YUV422_UYVY = 0,
	YUV422_VYUY,
	YUV422_YUYV,
	YUV422_YVYU,
	RGB565,
	RAW,
} SENSOR_PixelOutFmt;

typedef enum  {
	SENSOR_SET_CLK_POL    = 0,
	SENSOR_SET_OUTPUT_FMT,
	SENSOR_SET_PIXEL_SIZE,
	SENSOR_SET_SUBSAMP,

        /* TODO ... */
} SENSOR_IoctrlCmd;

struct sensor_power_cfg {
	gpio_pin_t reset_pin;
	gpio_pin_t pwdn_pin;
};

struct sensor_config {
	struct sensor_power_cfg pwcfg;
	twi_port_t twi_port;
};

struct sensor_win_size {
	unsigned int width;
	unsigned int height;
	unsigned int hoffset;
	unsigned int voffset;
	unsigned int mbus_code;
};

struct sensor_function {
	struct sensor_win_size *sensor_win_sizes;

	HAL_Status (*init)(struct sensor_config *cfg);
	HAL_Status (*deinit)(struct sensor_config *cfg);
	void (*g_mbus_config)(unsigned int *flags);
	void (*suspend)(void);
	void (*resume)(void);
	HAL_Status (*ioctl)(SENSOR_IoctrlCmd attr, uint32_t arg);
};

#endif

