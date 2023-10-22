/**
  * @file  drv_gc0328c.c
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
#include "hal_timer.h"
#include "sunxi_hal_twi.h"

#include "sensor_helper.h"
#include "sensor/drv_gc0328c.h"

#define MCLK              (24*1000*1000)
#define VREF_POL          MBUS_VSYNC_ACTIVE_HIGH
#define HREF_POL          MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           MBUS_PCLK_SAMPLE_FALLING

#define ADDR_WIDTH        TWI_BITS_8
#define DATA_WIDTH        TWI_BITS_8

#define GC0328C_SCCB_ID			0x21 //GC0328C ID
#define GC0328C_CHIP_ID			0x9d
#define GC0328C_IIC_CLK_FREQ		100000

#define SENSOR_NAME "gc0328"

static sensor_private gsensor_private;

static sensor_private *sensor_getpriv()
{
	return &gsensor_private;
}

static struct regval_list gc0328c_init_reg_tbl[]=
{
	// Initail Sequence Write In.

	/*init registers code.*/
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xfc, 0x16},
	{0xfc, 0x16},
	{0xfc, 0x16},
	{0xfc, 0x16},
	{0xfe, 0x00},
	{0x4f, 0x00},
	{0x42, 0x00},
	{0x03, 0x00},
	{0x04, 0xc0},
	{0x77, 0x62},
	{0x78, 0x40},
	{0x79, 0x4d},

	{0x05, 0x01},
	{0x06, 0x32},
	{0x07, 0x00},
	{0x08, 0x70},
	{0xfe, 0x01},
	{0x29, 0x00},	//anti-flicker step [11:8]
	{0x2a, 0x78},	//anti-flicker step [7:0]
	{0x2b, 0x02},	//exp level 0  20fps
	{0x2c, 0x58},
	{0x2d, 0x02},
	{0x2e, 0x58},
	{0x2f, 0x02},
	{0x30, 0x58},
	{0x31, 0x02},
	{0x32, 0x58},
	{0xfe, 0x00},

	{0xfe, 0x01},
	{0x4f, 0x00},
	{0x4c, 0x01},
	{0xfe, 0x00},
	//////////////////////////////
	///////////AWB///////////
	////////////////////////////////
	{0xfe, 0x01},
	{0x51, 0x80},
	{0x52, 0x12},
	{0x53, 0x80},
	{0x54, 0x60},
	{0x55, 0x01},
	{0x56, 0x06},
	{0x5b, 0x02},
	{0x61, 0xdc},
	{0x62, 0xdc},
	{0x7c, 0x71},
	{0x7d, 0x00},
	{0x76, 0x00},
	{0x79, 0x20},
	{0x7b, 0x00},
	{0x70, 0xFF},
	{0x71, 0x00},
	{0x72, 0x10},
	{0x73, 0x40},
	{0x74, 0x40},
	////AWB//
	{0x50, 0x00},
	{0xfe, 0x01},
	{0x4f, 0x00},
	{0x4c, 0x01},
	{0x4f, 0x00},
	{0x4f, 0x00},
	{0x4f, 0x00},	//wait clr finish
	{0x4d, 0x36},	  ////////////40
	{0x4e, 0x02},	  //SPL D65
	{0x4e, 0x02},	  //SPL D65
	{0x4d, 0x44},	  ////////////40
	{0x4e, 0x02},
	{0x4e, 0x02},
	{0x4e, 0x02},	  //SPL D65
	{0x4e, 0x02},	   //SPL D65
	{0x4d, 0x53},	  /////////////50
	{0x4e, 0x08},	 //SPL CWF
	{0x4e, 0x08},	   //DNP
	{0x4e, 0x02},	//04//pink		//DNP
	{0x4d, 0x63},	  /////////////60
	{0x4e, 0x08},	//TL84
	{0x4e, 0x08},	//TL84
	{0x4d, 0x73},	  //////////////80
	{0x4e, 0x20},	 //SPL	 A
	{0x4d, 0x83},	  //////////////80
	{0x4e, 0x20},	 //SPL	 A
	{0x4f, 0x01},
	{0x50, 0x88},
	{0xfe, 0x00},	  //page0
	////////////////////////////////////////////////
	////////////	 BLK	  //////////////////////
	////////////////////////////////////////////////
	{0x27, 0x00},
	{0x2a, 0x40},
	{0x2b, 0x40},
	{0x2c, 0x40},
	{0x2d, 0x40},
	//////////////////////////////////////////////
	////////// page  0	  ////////////////////////
	//////////////////////////////////////////////
	{0xfe, 0x00},
	{0x0d, 0x01},
	{0x0e, 0xe8},
	{0x0f, 0x02},
	{0x10, 0x88},
	{0x09, 0x00},
	{0x0a, 0x00},
	{0x0b, 0x00},
	{0x0c, 0x00},
	{0x16, 0x00},
	{0x17, 0x14},
	{0x18, 0x0e},
	{0x19, 0x06},
	{0x1b, 0x48},
	{0x1f, 0xC8},
	{0x20, 0x01},
	{0x21, 0x78},
	{0x22, 0xb0},
	{0x23, 0x04}, // 0x06 20140519 lanking GC0328C
	{0x24, 0x11},
	{0x26, 0x00},
	{0x50, 0x01}, //crop mode
	//global gain for range
	{0x70, 0x85},
	////////////////////////////////////////////////
	////////////	 block enable	   /////////////
	////////////////////////////////////////////////
	{0x40, 0x7f},
	{0x41, 0x26},
	{0x42, 0xff},
	{0x45, 0x00},
	{0x44, 0x02},
	{0x46, 0x03},

	{0x4b, 0x01},
	{0x50, 0x01},

	/////DN & EEINTP/////
	{0x7e, 0x0a},
	{0x7f, 0x03},
	{0x80, 0x27}, //  20140915
	{0x81, 0x15},
	{0x82, 0x90},
	{0x83, 0x02},
	{0x84, 0x23}, // 0x22 20140915
	{0x90, 0x2c},
	{0x92, 0x02},
	{0x94, 0x02},
	{0x95, 0x35},

	///////YCP
	{0xd1, 0x32},
	{0xd2, 0x32},
	{0xdd, 0x18},
	{0xde, 0x32},
	{0xe4, 0x88},
	{0xe5, 0x40},
	{0xd7, 0x0e},
	/////////////////////////////
	//////////////// GAMMA //////
	/////////////////////////////
	//rgb gamma
	{0xfe, 0x00},
	{0xbf, 0x10},
	{0xc0, 0x1c},
	{0xc1, 0x33},
	{0xc2, 0x48},
	{0xc3, 0x5a},
	{0xc4, 0x6b},
	{0xc5, 0x7b},
	{0xc6, 0x95},
	{0xc7, 0xab},
	{0xc8, 0xbf},
	{0xc9, 0xcd},
	{0xca, 0xd9},
	{0xcb, 0xe3},
	{0xcc, 0xeb},
	{0xcd, 0xf7},
	{0xce, 0xfd},
	{0xcf, 0xff},
	///Y gamma
	{0xfe, 0x00},
	{0x63, 0x00},
	{0x64, 0x05},
	{0x65, 0x0c},
	{0x66, 0x1a},
	{0x67, 0x29},
	{0x68, 0x39},
	{0x69, 0x4b},
	{0x6a, 0x5e},
	{0x6b, 0x82},
	{0x6c, 0xa4},
	{0x6d, 0xc5},
	{0x6e, 0xe5},
	{0x6f, 0xFF},
	//////ASDE
	{0xfe, 0x01},
	{0x18, 0x02},
	{0xfe, 0x00},
	{0x98, 0x00},
	{0x9b, 0x20},
	{0x9c, 0x80},
	{0xa4, 0x10},
	{0xa8, 0xB0},
	{0xaa, 0x40},
	{0xa2, 0x23},
	{0xad, 0x01},
	//////////////////////////////////////////////
	////////// AEC	  ////////////////////////
	//////////////////////////////////////////////
	{0xfe, 0x01},
	{0x9c, 0x02},
	{0x08, 0xa0},
	{0x09, 0xe8},
	{0x10, 0x00},
	{0x11, 0x11},
	{0x12, 0x10},
	{0x13, 0x80},	 ///  lanking +2
	{0x15, 0xfc},
	{0x18, 0x03},
	{0x21, 0xc0},
	{0x22, 0x60},
	{0x23, 0x30},
	{0x25, 0x00},
	{0x24, 0x14},
	//////////////////////////////////////
	////////////LSC//////////////////////
	//////////////////////////////////////
	{0xfe, 0x01},
	{0xc0, 0x10},
	{0xc1, 0x0c},
	{0xc2, 0x0a},
	{0xc6, 0x0e},
	{0xc7, 0x0b},
	{0xc8, 0x0a},
	{0xba, 0x26},
	{0xbb, 0x1c},
	{0xbc, 0x1d},
	{0xb4, 0x23},
	{0xb5, 0x1c},
	{0xb6, 0x1a},
	{0xc3, 0x00},
	{0xc4, 0x00},
	{0xc5, 0x00},
	{0xc9, 0x00},
	{0xca, 0x00},
	{0xcb, 0x00},
	{0xbd, 0x00},
	{0xbe, 0x00},
	{0xbf, 0x00},
	{0xb7, 0x07},
	{0xb8, 0x05},
	{0xb9, 0x05},
	{0xa8, 0x07},
	{0xa9, 0x06},
	{0xaa, 0x00},
	{0xab, 0x04},
	{0xac, 0x00},
	{0xad, 0x02},
	{0xae, 0x0d},
	{0xaf, 0x05},
	{0xb0, 0x00},
	{0xb1, 0x07},
	{0xb2, 0x03},
	{0xb3, 0x00},
	{0xa4, 0x00},
	{0xa5, 0x00},
	{0xa6, 0x00},
	{0xa7, 0x00},
	{0xa1, 0x3c},
	{0xa2, 0x50},
	{0xfe, 0x00},
	{0xB1, 0x04},
	{0xB2, 0xfd},
	{0xB3, 0xfc},
	{0xB4, 0xf0},
	{0xB5, 0x05},
	{0xB6, 0xf0},
	{0xfe, 0x00},
	{0x27, 0xf7},
	{0x28, 0x7F},
	{0x29, 0x20},
	{0x33, 0x20},
	{0x34, 0x20},
	{0x35, 0x20},
	{0x36, 0x20},
	{0x32, 0x08},
	{0x47, 0x00},
	{0x48, 0x00},
	{0xfe, 0x01},
	{0x79, 0x00},
	{0x7d, 0x00},
	{0x50, 0x88},
	{0x5b, 0x0c},
	{0x76, 0x8f},
	{0x80, 0x70},
	{0x81, 0x70},
	{0x82, 0xb0},
	{0x70, 0xff},
	{0x71, 0x00},
	{0x72, 0x28},
	{0x73, 0x0b},
	{0x74, 0x0b},
	{0xfe, 0x00},
	{0x70, 0x45},
	{0x4f, 0x01},
	{0xf1, 0x07},
	{0xf2, 0x01},

#if 0 //1/2 subsample
	{0xfe, 0x00},
	{0x59, 0x22},
	{0x5a, 0x0e},
	{0x5b, 0x00},
	{0x5c, 0x00},
	{0x5d, 0x00},
	{0x5e, 0x00},
	{0x5f, 0x00},
	{0x60, 0x00},
	{0x61, 0x00},
	{0x62, 0x00},

	//crop 320*240
	{0x50, 0x01},
	{0x51, 0x00},
	{0x52, 0x00},
	{0x53, 0x00},
	{0x54, 0x00},
	{0x55, 0x00},
	{0x56, 0xf0},
	{0x57, 0x01},
	{0x58, 0x40},
#endif
};

/**
  * @brief Set the effects for camera.
  * @param eft: effects.
  * @retval None
  */
void GC0328C_SetSpecialEffects(SENSOR_SpecailEffects eft)
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
  * @param x: Starting coordinates.
  * @param y: Starting coordinates.
  * @param width: Window width.
  * @param height: Window height.
  * @retval None
  */
void GC0328C_SetWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	sensor_private *priv = sensor_getpriv();

	if (x<0 || x>648 || y<0 || y>488 || w<=0 || w>648 || h<=0 || h>488) {
		return ;
	}

	sensor_write(priv, 0xfe, 0x00);            // page0
	sensor_write(priv, 0x09, (y>>8)&0x0001);   // Row_start[8]
	sensor_write(priv, 0x0a, y&0x00FF);        // Row_start[7:0]
	sensor_write(priv, 0x0b, (x>>8)&0x0003);   // Column_start[9:8]
	sensor_write(priv, 0x0c, x&0x00FF);        // Column_start[7:0]
	sensor_write(priv, 0x0d, (h>>8)&0x0001);   // Window_height[8]
	sensor_write(priv, 0x0e, h&0x00FF);        // Window_height[7:0]
	sensor_write(priv, 0x0f, (w>>8)&0x0003);   // Window_width[9:8]
	sensor_write(priv, 0x10, w&0x00FF);        // Window_width[7:0]
}

void GC0328C_SetCropWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	sensor_private *priv = sensor_getpriv();

	if ((x==0) && (y==0) && (w==0) && (h==0)) {
		// disable crop window
		sensor_write(priv, 0x50, 0x00);
		return ;
	}
	if (x<0 || x>648 || y<0 || y>488 || w<=0 || w>648 || h<=0 || h>488) {
		return ;
	}
	sensor_write(priv, 0xfe, 0x00);            // page0
	sensor_write(priv, 0x51, (y>>8)&0x0003);   // out window y1[8,9]
	sensor_write(priv, 0x52, y&0x00FF);        // out window y1[7:0]
	sensor_write(priv, 0x53, (x>>8)&0x0007);   // out window x1[10:8]
	sensor_write(priv, 0x54, x&0x00FF);        // out window x1[7:0]
	sensor_write(priv, 0x55, (h>>8)&0x0001);   // out Window_height[8]
	sensor_write(priv, 0x56, h&0x00FF);        // out Window_height[7:0]
	sensor_write(priv, 0x57, (w>>8)&0x0003);   // out Window_width[9:8]
	sensor_write(priv, 0x58, w&0x00FF);        // out Window_width[7:0]
	sensor_write(priv, 0x50, 0x01);            // crop out window mode
}

void GC0328C_SetSubsample(uint8_t ratio)
{
	sensor_private *priv = sensor_getpriv();

	if (ratio > 6) {
		return ;
	}
	if (ratio == 0) {
		sensor_write(priv, 0x59, 0x11); // subsample
		return ;
	}
	uint8_t row_col;
	uint8_t row_n1, row_n2, row_n3, row_n4;
	uint8_t col_n1, col_n2, col_n3, col_n4;

	uint8_t tmp;
	sensor_write(priv, 0xfe,0x00); // page0
	sensor_read(priv, 0x5a, (uint16_t *)&tmp);
	tmp &= ~(3<<4);
	sensor_write(priv, 0x5a, tmp); // sub_mode[5:4]

	if (ratio == 1) {
		row_col = 0x22;
		row_n1 = row_n2 = row_n3 = row_n4 = 0;
		col_n1 = col_n2 = col_n3 = col_n4 = 0;
	} else if (ratio == 2) {
		row_col = 0x33;
		row_n1 = row_n2 = row_n3 = row_n4 = 0;
		col_n1 = col_n2 = col_n3 = col_n4 = 0;
	} else if (ratio == 3) {
		row_col = 0x44;
		row_n1 = row_n2 = row_n3 = row_n4 = 0;
		col_n1 = col_n2 = col_n3 = col_n4 = 0;
	} else if (ratio == 4) {
		row_col = 0x33;
		row_n2 = row_n3 = row_n4 = 0;
		col_n2 = col_n3 = col_n4 = 0;
		row_n1 = 0x02;
		col_n1 = 0x02;
	} else if (ratio == 5) {
		row_col = 0x55;
		row_n3 = row_n4 = 0;
		col_n3 = col_n4 = 0;
		row_n1 = 0x02;
		col_n1 = 0x02;
		row_n2 = 0x04;
		col_n2 = 0x04;
	} else if (ratio == 6) {
		row_col = 0x77;
		row_n1 = 0x02;
		row_n2 = 0x46;
		row_n3 = 0x00;
		row_n4 = 0x40;
		col_n1 = 0x02;
		col_n2 = 0x46;
		col_n3 = 0x00;
		col_n4 = 0x00;
	} else {
		return ;
	}

	sensor_write(priv, 0x59, row_col); // subsample

	sensor_write(priv, 0x5b, row_n1);   // Sub_row_N1
	sensor_write(priv, 0x5c, row_n2);   // Sub_row_N2
	sensor_write(priv, 0x5d, row_n3);   // Sub_row_N3
	sensor_write(priv, 0x5e, row_n4);   // Sub_row_N4

	sensor_write(priv, 0x5f, col_n1);   // Sub_col_N1
	sensor_write(priv, 0x60, col_n2);   // Sub_col_N2
	sensor_write(priv, 0x61, col_n3);   // Sub_col_N3
	sensor_write(priv, 0x62, col_n4);   // Sub_col_N4
}

/**
 * Meas_win_x0 = Reg[P1:0x06]*4
 * Meas_win_y0 = Reg[P1:0x07]*4
 * Meas_win_x1 = Reg[P1:0x08]*4
 * Meas_win_y1 = Reg[P1:0x09]*4
 */
void GC0328C_SetMeasureWin(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h)
{
	sensor_private *priv = sensor_getpriv();

	if (x0<0 || x0>648 || y0<0 || y0>488 || w<=0 || w>648 || h<=0 || h>488) {
		return ;
	}
	uint16_t x1,y1;
	x1 = x0 + w;
	y1 = y0 + h;
	if (x1 > 640)
		x1 = 640;
	if (y1 > 480)
		y1 = 480;
	sensor_write(priv, 0xfe, 0x01);       // page1
	sensor_write(priv, 0x06, x0/4);       // big_win_x0
	sensor_write(priv, 0x07, y0/4);       // big_win_y0
	sensor_write(priv, 0x08, x1/4);       // big_win_x1
	sensor_write(priv, 0x09, y1/4);       // big_win_y1
}

void GC0328C_SetPixelOutFmt(SENSOR_PixelOutFmt pixel_out_fmt)
{
	sensor_private *priv = sensor_getpriv();

	sensor_write(priv, 0xfe,0x00); // page0
	switch (pixel_out_fmt) {
	case YUV422_UYVY:
		sensor_write(priv, 0x44, 0xa0);
		break;
	case YUV422_VYUY:
		sensor_write(priv, 0x44, 0xa1);
		break;
	case YUV422_YUYV:
		sensor_write(priv, 0x44, 0xa2);
		break;
	case YUV422_YVYU:
		sensor_write(priv, 0x44, 0xa3);
		break;
	case RGB565:
		sensor_write(priv, 0x44, 0xa6);
		break;
	default:
		sensor_err("GC0328C:untest pixel out fmt %d\n", pixel_out_fmt);
		break;
	}
}

/**
  * @brief Init the io for ctrl the camera power.
  * @param cfg: The io info.
  * @retval None
  */
#if 0
static void GC0328C_InitPower(SENSOR_PowerCtrlCfg *cfg)
{
    GPIO_InitParam param;
    param.driving = GPIO_DRIVING_LEVEL_1;
    param.mode = GPIOx_Pn_F1_OUTPUT;
    param.pull = GPIO_PULL_NONE;

    HAL_GPIO_Init(cfg->Pwdn_Port, cfg->Pwdn_Pin, &param);
	HAL_GPIO_WritePin(cfg->Pwdn_Port, cfg->Pwdn_Pin, GPIO_PIN_HIGH);
	OS_MSleep(10);
    HAL_GPIO_WritePin(cfg->Pwdn_Port, cfg->Pwdn_Pin, GPIO_PIN_LOW);
    OS_MSleep(10);
}

static void GC0328C_DeInitPower(SENSOR_PowerCtrlCfg *cfg)
{
    HAL_GPIO_WritePin(cfg->Pwdn_Port, cfg->Pwdn_Pin, GPIO_PIN_HIGH);
    HAL_GPIO_DeInit(cfg->Pwdn_Port, cfg->Pwdn_Pin);
}
#endif
static void sensor_power(struct sensor_power_cfg *cfg, unsigned int on)
{
	if (on) {
		hal_gpio_set_driving_level(cfg->pwdn_pin, GPIO_DRIVING_LEVEL1);
		hal_gpio_set_pull(cfg->pwdn_pin,GPIO_PULL_DOWN_DISABLED);

		hal_gpio_set_data(cfg->pwdn_pin, GPIO_DATA_HIGH);
		//OS_MSleep(10);
		hal_msleep(10);
		hal_gpio_set_data(cfg->pwdn_pin, GPIO_DATA_LOW);
		//OS_MSleep(10);
		hal_msleep(10);
	} else {
		hal_gpio_set_data(cfg->pwdn_pin, GPIO_DATA_HIGH);
		sensor_dbg("there is no deinit gpio");
	}
}

static void GC0328C_SyncEnable(void)
{
	sensor_private *priv = sensor_getpriv();

	sensor_write(priv, 0xf1, 0x07);
	sensor_write(priv, 0xf2, 0x01);
}

HAL_Status hal_gc0328c_ioctl(SENSOR_IoctrlCmd attr, uint32_t arg)
{
	switch (attr) {
	case SENSOR_SET_OUTPUT_FMT:
	{
		SENSOR_PixelOutFmt output_fmt;
		output_fmt = *((SENSOR_PixelOutFmt *)arg);
		GC0328C_SetPixelOutFmt(output_fmt);
		break;
	}
	case SENSOR_SET_SUBSAMP:
	{
		uint8_t radio = (uint8_t)arg;
		GC0328C_SetSubsample(radio);
		break;
	}
	default:
		sensor_err("un support camsensor cmd %d\n", attr);
		return HAL_ERROR;
		break;
	}

	return HAL_OK;
}

static HAL_Status gc0308_s_stream(void)
{
	sensor_private *priv = sensor_getpriv();

	uint8_t chip_id;
	uint16_t i = 0;

	if (sensor_read(priv, 0xf0, (uint16_t *)&chip_id) != 1) {
		sensor_err("GC0328C sccb read error\n");
		return HAL_ERROR;
	} else {
		if(chip_id!= GC0328C_CHIP_ID) {
			sensor_err("GC0328C get chip id wrong 0x%02x\n", chip_id);
			return HAL_ERROR;
		} else {
			sensor_print("GC0328C chip id read success 0x%02x\n", chip_id);
	    }
	}

	sensor_write_array(priv, gc0328c_init_reg_tbl, ARRAY_SIZE(gc0328c_init_reg_tbl));

	sensor_print("GC0328C Init Done \r\n");

	return HAL_OK;
}

#ifdef CONFIG_PM
static int sensor_suspend(struct soc_device *dev, enum suspend_state_t state)
{
	sensor_Private *priv = (sensor_Private*)dev->platform_data;
	priv->suspend = 1;

	switch (state) {
	case PM_MODE_SLEEP:
	case PM_MODE_STANDBY:
	case PM_MODE_HIBERNATION:
		HAL_GC0328C_Suspend();
		break;
	default:
		break;
	}

	return 0;
}

static int sensor_resume(struct soc_device *dev, enum suspend_state_t state)
{
	sensor_Private *priv = (sensor_Private*)dev->platform_data;

	switch (state) {
	case PM_MODE_SLEEP:
	case PM_MODE_STANDBY:
	case PM_MODE_HIBERNATION:
		HAL_GC0328C_Resume();
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

struct sensor_win_size hal_gc0328c_win_sizes = {
	.width = 640,
	.height = 480,
	.hoffset = 0,
	.voffset = 0,
	.mbus_code = MEDIA_BUS_FMT_YUYV8_2X8,
};

void hal_gc0328c_g_mbus_config(unsigned int *flags)
{
	//DC
	*flags = MBUS_SEPARATE_SYNCS | VREF_POL | HREF_POL | CLK_POL;
}

/**
  * @brief Init the GC0328C.
  * @retval Component_Status : The driver status.
  */
HAL_Status hal_gc0328c_init(struct sensor_config *cfg)
{
	sensor_private *priv = sensor_getpriv();

	//priv->i2c_id = cfg->i2c_id;
	priv->twi_port = cfg->twi_port;
	priv->addr_width = ADDR_WIDTH;
	priv->data_width = DATA_WIDTH;
	priv->slave_addr = GC0328C_SCCB_ID;
	sensor_twi_init(priv->twi_port, GC0328C_SCCB_ID);

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
		sensor_err("GC0328C Init error!!\n");
		return HAL_ERROR;
	}

	//OS_MSleep(1000);
	hal_msleep(1000);
	return HAL_OK;
}

/**
  * @brief Deinit the GC0328C.
  * @retval Component_Status : The driver status.
  */
void hal_gc0328c_deinit(struct sensor_config *cfg)
{
	sensor_private *priv = sensor_getpriv();

#ifdef CONFIG_PM
	if (!priv->suspend)
		pm_unregister_ops(&priv->dev);
#endif

	sensor_power(&cfg->pwcfg, 0);
	sensor_twi_exit(priv->twi_port);
}

/**
  * @brief Suspend the GC0328C.
  * @retval Void.
  */
void hal_gc0328c_suspend(void)
{
	sensor_private *priv = sensor_getpriv();
	uint8_t clock_mode = 0;
	uint8_t pad_setting1 = 0;

	sensor_read(priv, 0xfc, (uint16_t *)&clock_mode);
	sensor_read(priv, 0xf1, (uint16_t *)&pad_setting1);

	clock_mode |= 0x1;
	pad_setting1 = 0;

	sensor_write(priv, 0xfc, clock_mode);
	sensor_write(priv, 0xf1, pad_setting1);
}

/**
  * @brief Resume the GC0328C.
  * @retval Void.
  */
void hal_gc0328c_resume(void)
{
	sensor_private *priv = sensor_getpriv();
	uint8_t clock_mode = 0;
	uint8_t pad_setting1 = 0;

	sensor_read(priv, 0xfc, (uint16_t *)&clock_mode);
	sensor_read(priv, 0xf1, (uint16_t *)&pad_setting1);

	clock_mode &= (~(0x1));
	pad_setting1 = 0x7;

	sensor_write(priv, 0xfc, clock_mode);
	sensor_write(priv, 0xf1, pad_setting1);
}
