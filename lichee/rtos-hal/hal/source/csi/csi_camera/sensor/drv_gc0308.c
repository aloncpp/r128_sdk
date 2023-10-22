/**
  * @file  drv_gc0308.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sunxi_hal_twi.h"

#include "sensor_helper.h"

#define MCLK              (24*1000*1000)
#define VREF_POL          MBUS_VSYNC_ACTIVE_HIGH
#define HREF_POL          MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           MBUS_PCLK_SAMPLE_FALLING

#define ADDR_WIDTH        TWI_BITS_8
#define DATA_WIDTH        TWI_BITS_8

#define GC0308_SCCB_ID			0x21 //GC0308 ID 0x21
#define GC0308_CHIP_ID			0x9b
#define GC0308_IIC_CLK_FREQ		100000

#define SENSOR_NAME "gc0308"

static sensor_private gsensor_private;

static sensor_private *sensor_getpriv()
{
	return &gsensor_private;
}

//#define WIN_MODE_QVGA
//#define SUBSAMPLE_1_2

//??ʼ???Ĵ??????м?????Ӧ??ֵ
static struct regval_list gc0308_init_reg_tbl[]=
{
	{0xfe,0x00},
	//MCLK=24MHz 10fps
	{0x0f, 0x00},
	{0x01, 0x6a},
	{0x02, 0x70},
//	{0x0f, 0xc0},  // vb=1595
//	{0x01, 0x6a},
//	{0x02, 0x76},

	/* 消除flicker问题 */
	//exp step
	{0xe2, 0x00},
	{0xe3, 0x96},
	//exp level
	{0xe4, 0x02},
	{0xe5, 0x58},
	{0xe6, 0x02},
	{0xe7, 0x58},
	{0xe8, 0x02},
	{0xe9, 0x58},
	{0xea, 0x02},
	{0xeb, 0x58},

//	{0xfe,0x00},
	{0xec,0x20},

#ifdef WIN_MODE_QVGA
	//QVGA
	{0x05,0x00},
	{0x06,0x78},
	{0x07,0x00},
	{0x08,0xa0},
	{0x09,0x00}, //240
	{0x0a,0xf8},
	{0x0b,0x01}, //320
	{0x0c,0x48},
#else
	//VGA
	{0x05,0x00},
	{0x06,0x00},
	{0x07,0x00},
	{0x08,0x00},
	{0x09,0x01},//reg09,reg0a决定win_height的大小，win_height=0x1e8=488(实际???88-8=480)
	{0x0a,0xe8},
	{0x0b,0x02},//reg0b,reg0c决定win_width的大小，win_width=0x288=648(实际???48-8=640)
	{0x0c,0x88},
#endif

#if 0
	/* Crop window mode for QVGA */
	{0x46,0x80},
	{0x47,0x78},
	{0x48,0xa0},
	{0x49,0x00},
	{0x4a,0xf0},
	{0x4b,0x01},
	{0x4c,0x40},
#endif

	{0x0d,0x02},
	{0x0e,0x02},
	{0x10,0x26},
	{0x11,0x0d},
	{0x12,0x2a},
	{0x13,0x00},
	{0x14,0x11},
	{0x15,0x0a},
	{0x16,0x05},
	{0x17,0x01},
	{0x18,0x44},
	{0x19,0x44},
	{0x1a,0x2a},
	{0x1b,0x00},
	{0x1c,0x49},
	{0x1d,0x9a},
//	{0x1d,0xf8},  // 0x9a
	{0x1e,0x61},
//	{0x1f,0x17},  // 0x16
	{0x1f,0x2a},  // 0x16
	{0x20,0x7f},
	{0x21,0xfa},
	{0x22,0x57},
	{0x24,0xa2},  // YCbYCr
//	{0x25,0x0f},
	{0x26,0x03},  // pclk=1; hsync=1, vsync=1
//	{0x26,0x07},  // invert pclk
	{0x28,0x00},  // MCLK不分频
	{0x2d,0x0a},
//	{0x2e,0x02},  // colorbar, for test
	{0x2f,0x01},
	{0x30,0xf7},
	{0x31,0x50},
	{0x32,0x00},
	{0x33,0x28},
	{0x34,0x2a},
	{0x35,0x28},
	{0x39,0x04},
	{0x3a,0x20},
	{0x3b,0x20},
	{0x3c,0x00},
	{0x3d,0x00},
	{0x3e,0x00},
	{0x3f,0x00},
	{0x50,0x14},  // 0x14
	{0x52,0x41},
	{0x53,0x80},
	{0x54,0x80},
	{0x55,0x80},
	{0x56,0x80},
	{0x8b,0x20},
	{0x8c,0x20},
	{0x8d,0x20},
	{0x8e,0x14},
	{0x8f,0x10},
	{0x90,0x14},
	{0x91,0x3c},
	{0x92,0x50},
	//{0x8b,0x10},
	//{0x8c,0x10},
	//{0x8d,0x10},
	//{0x8e,0x10},
	//{0x8f,0x10},
	//{0x90,0x10},
	//{0x91,0x3c},
	//{0x92,0x50},
	{0x5d,0x12},
	{0x5e,0x1a},
	{0x5f,0x24},
	{0x60,0x07},
	{0x61,0x15},
	{0x62,0x08},  // 0x08
	{0x64,0x03},  // 0x03
	{0x66,0xe8},
	{0x67,0x86},
	{0x68,0x82},
	{0x69,0x18},
	{0x6a,0x0f},
	{0x6b,0x00},
	{0x6c,0x5f},
	{0x6d,0x8f},
	{0x6e,0x55},
	{0x6f,0x38},
	{0x70,0x15},
	{0x71,0x33},
	{0x72,0xdc},
	{0x73,0x00},
	{0x74,0x02},
	{0x75,0x3f},
	{0x76,0x02},
	{0x77,0x38},  // 0x47
	{0x78,0x88},
	{0x79,0x81},
	{0x7a,0x81},
	{0x7b,0x22},
	{0x7c,0xff},
	{0x93,0x48},  // color matrix default
	{0x94,0x02},
	{0x95,0x07},
	{0x96,0xe0},
	{0x97,0x40},
	{0x98,0xf0},
	{0xb1,0x40},
	{0xb2,0x40},
	{0xb3,0x40},  // 0x40
	{0xb6,0xe0},
	{0xbd,0x38},
	{0xbe,0x36},
	{0xd0,0xCB},
	{0xd1,0x10},
	{0xd2,0x90},
	{0xd3,0x48},
	{0xd5,0xF2},
	{0xd6,0x16},
	{0xdb,0x92},
	{0xdc,0xA5},
	{0xdf,0x23},
	{0xd9,0x00},
	{0xda,0x00},
	{0xe0,0x09},
	{0xed,0x04},
	{0xee,0xa0},
	{0xef,0x40},
	{0x80,0x03},

	{0x9F,0x10},
	{0xA0,0x20},
	{0xA1,0x38},
	{0xA2,0x4e},
	{0xA3,0x63},
	{0xA4,0x76},
	{0xA5,0x87},
	{0xA6,0xa2},
	{0xA7,0xb8},
	{0xA8,0xca},
	{0xA9,0xd8},
	{0xAA,0xe3},
	{0xAB,0xeb},
	{0xAC,0xf0},
	{0xAD,0xF8},
	{0xAE,0xFd},
	{0xAF,0xFF},

	{0xc0,0x00},
	{0xc1,0x10},
	{0xc2,0x1c},
	{0xc3,0x30},
	{0xc4,0x43},
	{0xc5,0x54},
	{0xc6,0x65},
	{0xc7,0x75},
	{0xc8,0x93},
	{0xc9,0xB0},
	{0xca,0xCB},
	{0xcb,0xE6},
	{0xcc,0xFF},
	{0xf0,0x02},
	{0xf1,0x01},
	{0xf2,0x02},
	{0xf3,0x30},

#ifdef WIN_MODE_QVGA
	//Measure window for QVGA
	{0xf7,0x28},
	{0xf8,0x1E},
	{0xf9,0x50},
	{0xfa,0x3C},
#else
	//Measure window for VGA
//	{0xf7,0x12},
//	{0xf8,0x0a},
//	{0xf9,0x9f},
//	{0xfa,0x78},
	{0xf7,0x04},
	{0xf8,0x02},
	{0xf9,0x98},
	{0xfa,0x78},
#endif

	{0xfe,0x01},
	{0x00,0xf5},
	{0x02,0x20},
	{0x04,0x10},
	{0x05,0x08},
	{0x06,0x20},
	{0x08,0x0a},
	{0x0a,0xa0},
	{0x0b,0x60},
	{0x0c,0x08},
	{0x0e,0x44},
	{0x0f,0x32},
	{0x10,0x41},
	{0x11,0x37},
	{0x12,0x22},
	{0x13,0x19},
	{0x14,0x44},
	{0x15,0x44},
	{0x16,0xc2},
	{0x17,0xA8},
	{0x18,0x18},
	{0x19,0x50},
	{0x1a,0xd8},
	{0x1b,0xf5},
	{0x70,0x40},
	{0x71,0x58},
	{0x72,0x30},
	{0x73,0x48},
	{0x74,0x20},
	{0x75,0x60},
	{0x77,0x20},
	{0x78,0x32},
	{0x30,0x03},
	{0x31,0x40},
	{0x32,0x10},
	{0x33,0xe0},
	{0x34,0xe0},
	{0x35,0x00},
	{0x36,0x80},
	{0x37,0x00},
	{0x38,0x04},
	{0x39,0x09},
	{0x3a,0x12},
	{0x3b,0x1C},
	{0x3c,0x28},
	{0x3d,0x31},
	{0x3e,0x44},
	{0x3f,0x57},
	{0x40,0x6C},
	{0x41,0x81},
	{0x42,0x94},
	{0x43,0xA7},
	{0x44,0xB8},
	{0x45,0xD6},
	{0x46,0xEE},
	{0x47,0x0d},
#ifdef SUBSAMPLE_1_2
	{0x53,0x82},
	{0x54,0x22},
	{0x55,0x03},
	{0x56,0x00},
	{0x57,0x00},
	{0x58,0x00},
	{0x59,0x00},
#endif
	{0x62,0xf7},
	{0x63,0x68},
	{0x64,0xd3},
	{0x65,0xd3},
	{0x66,0x60},
	{0xfe,0x00},

	{0x25,0x0f},

	//ri guang deng
	{0x22,0x55},
	{0x5a,0x40},
	{0x5b,0x42},
	{0x5c,0x50},

	//color effect none
	{0x23,0x00},
	{0x2d,0x0a},
	{0x20,0xff},
	{0xd2,0x90},
	{0x73,0x00},
	{0x77,0x54},
	{0xb3,0x40},
	{0xb4,0x80},
	{0xba,0x00},
	{0xbb,0x00},

};

/**
  * @brief Seclet the light mode.
  * @note This function is used to set the light mode for camera.
  *           The appropriate mode helps to improve the shooting effect.
  * @param light_mode: light mode.
  * @retval None
  */
void GC0308_SetLightMode(SENSOR_LightMode light_mode)
{
	sensor_private *priv = sensor_getpriv();
	uint8_t reg13val = 0XE7, reg01val = 0, reg02val = 0;
	switch(light_mode) {
	case LIGHT_AUTO:
		reg13val = 0XE7;
		reg01val = 0;
		reg02val = 0;
		break;
	case LIGHT_SUNNY:
		reg13val = 0XE5;
		reg01val = 0X5A;
		reg02val = 0X5C;
		break;
	case LIGHT_COLUDY:
		reg13val = 0XE5;
		reg01val = 0X58;
		reg02val = 0X60;
		break;
	case LIGHT_OFFICE:
		reg13val = 0XE5;
		reg01val = 0X84;
		reg02val = 0X4c;
		break;
	case LIGHT_HOME:
		reg13val = 0XE5;
		reg01val = 0X96;
		reg02val = 0X40;
		break;
	}
	sensor_write(priv, 0X13, reg13val);
	sensor_write(priv, 0X01, reg01val);
	sensor_write(priv, 0X02, reg02val);
}

/**
  * @brief Set the color saturation for camera.
  * @param sat: The color saturation.
  * @retval None
  */
void GC0308_SetColorSaturation(SENSOR_ColorSaturation sat)
{
	sensor_private *priv = sensor_getpriv();
	uint8_t reg4f5054val = 0X80, reg52val = 0X22, reg53val = 0X5E;

	switch (sat) {
	case COLOR_SATURATION_0://-2
		reg4f5054val = 0X40;
		reg52val = 0X11;
		reg53val = 0X2F;
		break;
	case COLOR_SATURATION_1://-1
		reg4f5054val = 0X66;
		reg52val = 0X1B;
		reg53val = 0X4B;
		break;
	case COLOR_SATURATION_2:
		reg4f5054val = 0X80;
		reg52val = 0X22;
		reg53val = 0X5E;
		break;
		case COLOR_SATURATION_3:
		reg4f5054val = 0X99;
		reg52val = 0X28;
		reg53val = 0X71;
		break;
	case COLOR_SATURATION_4:
		reg4f5054val = 0XC0;
		reg52val = 0X33;
		reg53val = 0X8D;
		break;
	}

	sensor_write(priv, 0X4F, reg4f5054val);
	sensor_write(priv, 0X50, reg4f5054val);
	sensor_write(priv, 0X51, 0X00);
	sensor_write(priv, 0X52, reg52val);
	sensor_write(priv, 0X53, reg53val);
	sensor_write(priv, 0X54, reg4f5054val);

}

/**
  * @brief Set the sensitivity for camera.
  * @param brihgt: The brightness value.
  * @retval None
  */
void GC0308_SetBrightness(SENSOR_Brightness bright)
{
	sensor_private *priv = sensor_getpriv();
	uint8_t reg55val = 0X00;

	switch (bright) {
	case BRIGHT_0:		//-2
		reg55val = 0XB0;
		break;
	case BRIGHT_1:
		reg55val = 0X98;
			break;
	case BRIGHT_2:
		reg55val = 0X00;
		break;
	case BRIGHT_3:
		reg55val = 0X18;
		break;
	case BRIGHT_4:
		reg55val = 0X30;
		break;
	}

	sensor_write(priv, 0X55,reg55val);
}

/**
  * @brief Set the contarst for camera.
  * @param contrast: The contrast value.
  * @retval None
  */
void GC0308_SetContrast(SENSOR_Contarst contrast)
{
	sensor_private *priv = sensor_getpriv();
	uint8_t reg56val = 0X40;

	switch (contrast) {
	case CONTARST_0:	//-2
		reg56val = 0X30;
		break;
	case CONTARST_1:
		reg56val = 0X38;
		break;
	case CONTARST_2:
		reg56val = 0X40;
		break;
	case CONTARST_3:
		reg56val = 0X50;
		break;
	case CONTARST_4:
		reg56val = 0X60;
		break;
	}
	sensor_write(priv, 0X56,reg56val);
}

/**
  * @brief Set the effects for camera.
  * @param eft: effects.
  * @retval None
  */
void GC0308_SetSpecialEffects(SENSOR_SpecailEffects eft)
{
	sensor_private *priv = sensor_getpriv();
	uint8_t reg3aval = 0X04;
	uint8_t reg67val = 0XC0;
	uint8_t reg68val = 0X80;

	switch (eft) {
	case IMAGE_NOMAL:	//nomal
		reg3aval = 0X04;
		reg67val = 0XC0;
		reg68val = 0X80;
		break;
	case IMAGE_NEGATIVE:
		reg3aval = 0X24;
		reg67val = 0X80;
		reg68val = 0X80;
		break;
	case IMAGE_BLACK_WHITE:
		reg3aval = 0X14;
		reg67val = 0X80;
		reg68val = 0X80;
		break;
	case IMAGE_SLANT_RED:
		reg3aval = 0X14;
		reg67val = 0Xc0;
		reg68val = 0X80;
		break;
	case IMAGE_SLANT_GREEN:
		reg3aval = 0X14;
		reg67val = 0X40;
		reg68val = 0X40;
		break;
	case IMAGE_SLANT_BLUE:
		reg3aval = 0X14;
		reg67val = 0X80;
		reg68val = 0XC0;
		break;
	case IMAGE_VINTAGE:
		reg3aval = 0X14;
		reg67val = 0XA0;
		reg68val = 0X40;
		break;
	}

	sensor_write(priv, 0X3A, reg3aval);
	sensor_write(priv, 0X68, reg67val);
	sensor_write(priv, 0X67, reg68val);
}

/**
  * @brief Set the window for camera.
  * @param sx: Starting coordinates.
  * @param sy: Starting coordinates.
  * @param width: Window width.
  * @param height: Window height.
  * @retval None
  */
void GC0308_SetWindow(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
	sensor_private *priv = sensor_getpriv();
	uint16_t endx;
	uint16_t endy;
	uint8_t temp;

	endx = sx + width * 2;		//V*2
	endy = sy + height * 2;
	if (endy > 784)
	endy -= 784;

	sensor_read(priv, 0X03, (uint16_t *)&temp);
	temp &= 0XF0;
	temp |= ((endx & 0X03) << 2) | (sx & 0X03);
	sensor_write(priv, 0X03, temp);
	sensor_write(priv, 0X19, sx>>2);
	sensor_write(priv, 0X1A, endx>>2);
	sensor_read(priv, 0X32, (uint16_t *)&temp);
	temp &= 0XC0;
	temp |= ((endy & 0X07) << 3) | (sy&0X07);
	sensor_write(priv, 0X32, temp);
	sensor_write(priv, 0X17, sy >> 3);
	sensor_write(priv, 0X18, endy >> 3);
}

//(140,16,640,480) is good for VGA
//(272,16,320,240) is good for QVGA
/* config_GC0308_window */
void GC0308_ConfigWindow(unsigned int startx,unsigned int starty,unsigned int width, unsigned int height)
{
	sensor_private *priv = sensor_getpriv();
	unsigned int endx;
	unsigned int endy;// "v*2"????
	unsigned char temp_reg1, temp_reg2;
	unsigned char temp=0;

	endx=(startx+width*2)%784;
	endy=(starty+height*2);// "v*2"????

	sensor_read(priv, 0x32, (uint16_t *)&temp_reg2);
	temp_reg2 &= 0xc0;

	sensor_read(priv, 0x03, (uint16_t *)&temp_reg1);
	temp_reg1 &= 0xf0;

	// Horizontal
	temp = temp_reg2|((endx&0x7)<<3)|(startx&0x7);
	sensor_write(priv, 0x32, temp);
	temp = (startx&0x7F8)>>3;
	sensor_write(priv, 0x17, temp);
	temp = (endx&0x7F8)>>3;
	sensor_write(priv, 0x18, temp);

	// Vertical
	temp = temp_reg1|((endy&0x3)<<2)|(starty&0x3);
	sensor_write(priv, 0x03, temp);
	temp = starty>>2;
	sensor_write(priv, 0x19, temp);
	temp = endy>>2;
	sensor_write(priv, 0x1A, temp);
}

void GC0308_SetPixelOutFmt(SENSOR_PixelOutFmt pixel_out_fmt)
{
	sensor_private *priv = sensor_getpriv();

	switch (pixel_out_fmt) {
	case YUV422_UYVY:
		sensor_write(priv, 0x24, 0xa0);
		break;
	case YUV422_VYUY:
		sensor_write(priv, 0x24, 0xa1);
		break;
	case YUV422_YUYV:
		sensor_write(priv, 0x24, 0xa2);
		break;
	case YUV422_YVYU:
		sensor_write(priv, 0x24, 0xa3);
		break;
	case RGB565:
		sensor_write(priv, 0x24, 0xa6);
		break;
	default:
		sensor_err("GC0308:untest pixel out fmt %d\n", pixel_out_fmt);
		break;
	}
}

/**
  * @brief Init the io for ctrl the camera power.
  * @param cfg: The io info.
  * @retval None
  */
#if 0
static void GC0308_InitPower(SENSOR_PowerCtrlCfg *cfg)
{
    GPIO_InitParam param;
    param.driving = GPIO_DRIVING_LEVEL_1;
    param.mode = GPIOx_Pn_F1_OUTPUT;
    param.pull = GPIO_PULL_NONE;

    HAL_GPIO_Init(cfg->Pwdn_Port, cfg->Pwdn_Pin, &param);
    HAL_GPIO_Init(cfg->Reset_Port, cfg->Reset_Pin, &param);

    HAL_GPIO_WritePin(cfg->Pwdn_Port, cfg->Pwdn_Pin, GPIO_PIN_LOW);
    HAL_GPIO_WritePin(cfg->Reset_Port, cfg->Reset_Pin, GPIO_PIN_LOW);
    hal_msleep(3);
    HAL_GPIO_WritePin(cfg->Reset_Port, cfg->Reset_Pin, GPIO_PIN_HIGH);
    hal_msleep(100);
}
#endif

static void sensor_power(struct sensor_power_cfg *cfg, unsigned int on)
{
	sensor_private *priv = sensor_getpriv();
	int ret;

	sensor_dbg("[%s:%d]pwdn_pin, reset_pin set!\n", __func__,__LINE__);
	if (on) {
		ret = hal_gpio_pinmux_set_function(cfg->pwdn_pin, GPIO_MUXSEL_OUT);
		ret = hal_gpio_pinmux_set_function(cfg->reset_pin, GPIO_MUXSEL_OUT);
		if (ret)
			sensor_dbg("hal_gpio_pinmux_set_function error!\n");
		ret = hal_gpio_set_driving_level(cfg->pwdn_pin, GPIO_DRIVING_LEVEL1);
		ret = hal_gpio_set_driving_level(cfg->reset_pin, GPIO_DRIVING_LEVEL1);
		if (ret)
			sensor_dbg("hal_gpio_set_driving_level error!\n");
		ret = hal_gpio_set_pull(cfg->pwdn_pin, GPIO_PULL_DOWN_DISABLED);
		ret = hal_gpio_set_pull(cfg->reset_pin, GPIO_PULL_DOWN_DISABLED);
		if (ret)
			sensor_dbg("hal_gpio_set_pull error!\n");

		csi_set_mclk_rate(priv, MCLK);
		csi_set_mclk(priv, 1);
		hal_msleep(100);

		hal_gpio_set_data(cfg->pwdn_pin, GPIO_DATA_LOW);
		hal_msleep(30);
		hal_gpio_set_data(cfg->reset_pin, GPIO_DATA_HIGH);
		hal_msleep(100);
	} else {
		hal_gpio_set_data(cfg->pwdn_pin, GPIO_DATA_HIGH);
		hal_gpio_set_data(cfg->reset_pin, GPIO_DATA_LOW);
		csi_set_mclk(priv, 0);
		hal_msleep(100);
		sensor_dbg("there is no deinit gpio!!!!\n");
	}

}

#if 0
static void GC0308_DeInitPower(SENSOR_PowerCtrlCfg *cfg)
{
    HAL_GPIO_WritePin(cfg->Pwdn_Port, cfg->Pwdn_Pin, GPIO_PIN_HIGH);
    HAL_GPIO_WritePin(cfg->Reset_Port, cfg->Reset_Pin, GPIO_PIN_LOW);
    hal_msleep(3);
    HAL_GPIO_DeInit(cfg->Pwdn_Port, cfg->Pwdn_Pin);
    HAL_GPIO_DeInit(cfg->Reset_Port, cfg->Reset_Pin);
}
#endif

static HAL_Status gc0308_s_stream(void)
{
	sensor_private *priv = sensor_getpriv();
	uint8_t chip_id;
	uint16_t i = 0;
	int cnt = 0;

	/* need write at least 3 times, if not, twi0 failed */
	while (cnt < 5) {
		sensor_write(priv, 0XFE, 0x80);
		hal_msleep(100);
		cnt++;
	}

	sensor_write(priv, 0XFE, 0x80);
	hal_msleep(100);

	sensor_write(priv, 0XFE, 0x00);
	hal_msleep(100);

	if (sensor_read(priv, 0x00, (uint16_t *)&chip_id) != 0) {
		sensor_err("GC0308 sccb read error\n");
		return HAL_ERROR;
	} else {
		if(chip_id != GC0308_CHIP_ID) {
			sensor_err("GC0308 get chip id wrong 0x%02x\n", chip_id);
			return HAL_ERROR;
		} else {
			sensor_dbg("GC0308 chip id read success 0x%02x\n", chip_id);
		}
	}

	sensor_write_array(priv, gc0308_init_reg_tbl, ARRAY_SIZE(gc0308_init_reg_tbl));
	sensor_dbg("GC0308 Init Done \r\n");

	return HAL_OK;
}

HAL_Status hal_gc0308_ioctl(SENSOR_IoctrlCmd attr, uint32_t arg)
{
	switch (attr) {
	case SENSOR_SET_OUTPUT_FMT:
	{
		SENSOR_PixelOutFmt output_fmt;
		output_fmt = *((SENSOR_PixelOutFmt *)arg);
		GC0308_SetPixelOutFmt(output_fmt);
		break;
	}
	default:
		sensor_err("un support camsensor cmd %d\n", attr);
		return HAL_ERROR;
		break;
	}

	return HAL_OK;
}

#ifdef CONFIG_PM
static int sensor_suspend(struct soc_device *dev, enum suspend_state_t state)
{
	sensor_private *priv = (sensor_Private*)dev->platform_data;
	priv->suspend = 1;

	switch (state) {
	case PM_MODE_SLEEP:
	case PM_MODE_STANDBY:
	case PM_MODE_HIBERNATION:
		HAL_GC0308_Suspend();
		break;
	default:
		break;
	}

	return 0;
}

static int sensor_resume(struct soc_device *dev, enum suspend_state_t state)
{
	sensor_private *priv = (sensor_Private*)dev->platform_data;

	switch (state) {
	case PM_MODE_SLEEP:
	case PM_MODE_STANDBY:
	case PM_MODE_HIBERNATION:
		HAL_GC0308_Resume();
		break;
	default:
		break;
	}
	priv->suspend = 0;

	return 0;
}

static const struct soc_device_driver sensor_drv = {
	.name = "sensor",
	.suspend = sensor_suspend,
	.resume = sensor_resume,
};
#endif

struct sensor_win_size hal_gc0308_win_sizes = {
	.width = 640,
	.height = 480,
	.hoffset = 0,
	.voffset = 0,
	.mbus_code = MEDIA_BUS_FMT_YUYV8_2X8,
};

void hal_gc0308_g_mbus_config(unsigned int *flags)
{
	//DC
	*flags = MBUS_SEPARATE_SYNCS | VREF_POL | HREF_POL | CLK_POL;
}

/**
  * @brief Init the GC0308.
  * @retval HAL_Status : The driver status.
  */
HAL_Status hal_gc0308_init(struct sensor_config *cfg)
{
	sensor_private *priv = sensor_getpriv();
	
	priv->twi_port = cfg->twi_port;
	priv->addr_width = ADDR_WIDTH;
	priv->data_width = DATA_WIDTH;
	priv->slave_addr = GC0308_SCCB_ID;
	sensor_twi_init(priv->twi_port, GC0308_SCCB_ID);

#ifdef CONFIG_PM
	if (!priv->suspend) {
		priv->dev.name = "sensor";
		priv->dev.driver = &sensor_drv;
		priv->dev.platform_data = priv;
		pm_register_ops(&priv->dev);
	}
#endif

	sensor_power(&cfg->pwcfg, 1);

	if (gc0308_s_stream() != HAL_OK) {
		sensor_err("GC0308  Init error!!\n");
		return HAL_ERROR;
	}

	sensor_dbg("GC0308 Init ok!!\n");
	hal_msleep(100);
	return HAL_OK;
}

/**
  * @brief Deinit the GC0308.
  * @retval HAL_Status : The driver status.
  */
void hal_gc0308_deinit(struct sensor_config *cfg)
{
	sensor_private *priv = sensor_getpriv();

#ifdef CONFIG_PM
	if (!priv->suspend)
		pm_unregister_ops(&priv->dev);
#endif

	sensor_power(&cfg->pwcfg, 0);
	sensor_twi_exit(priv->twi_port);
	sensor_dbg("%s line: %d", __func__, __LINE__);

}

/**
  * @brief Suspend the GC0308.
  * @retval Void.
  */
void hal_gc0308_suspend(void)
{
	sensor_private *priv = sensor_getpriv();
	uint8_t analog_mode1 = 0;
	uint8_t output_en = 0;

	sensor_read(priv, 0x1a, (uint16_t *)&analog_mode1);
	sensor_read(priv, 0x25, (uint16_t *)&output_en);

	analog_mode1 |= 0x1;
	output_en = 0;

	sensor_write(priv, 0x1a, analog_mode1);
	sensor_write(priv, 0x25, output_en);
}

/**
  * @brief Resume the GC0308.
  * @retval Void.
  */
void hal_gc0308_resume(void)
{
	sensor_private *priv = sensor_getpriv();
	uint8_t analog_mode1 = 0;
	uint8_t output_en = 0;

	sensor_read(priv, 0x1a, (uint16_t *)&analog_mode1);
	sensor_read(priv, 0x25, (uint16_t *)&output_en);

	analog_mode1 &= (~0x1);
	output_en = 0xff;

	sensor_write(priv, 0x1a, analog_mode1);
	sensor_write(priv, 0x25, output_en);
}
