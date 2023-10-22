/**
 * Copyright (c) 2017-2020 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File Name: displayInterface.h
 * Description : Display engine interface,compatible DE1 DE2
 *				 Returns -2 if DE1 or DE2 does not support a function
 * History :
 * Author  : allwinnertech
 * Date    : 2017/12/08
 * Comment : first version
 */
#ifndef __DISPLAYINTERFACE_H_
#define __DISPLAYINTERFACE_H_

#include <stdbool.h>
#include <video/sunxi_display2.h>

#define DISP_NOT_SUPPORT -2

typedef enum {
	ROTATION_DEGREE_0 = 0,
	ROTATION_DEGREE_90 = 1,
	ROTATION_DEGREE_180 = 2,
	ROTATION_DEGREE_270 = 3
} luaip_rotate_degree;

typedef enum {
	LUAPI_ZORDER_TOP = 0, LUAPI_ZORDER_MIDDLE = 1, LUAPI_ZORDER_BOTTOM = 2
} luapi_zorder;

typedef struct luapi_layer_config {
	struct disp_layer_config layerConfig;
} luapi_layer_config;

typedef struct luapi_capture_info {
	struct disp_capture_info captureInfo;
} luapi_capture_info;

typedef struct {
	int x;
	int y;
	unsigned int width;
	unsigned int height;
} luapi_disp_window;

/* ----disp global---- */
int DispSetBackColor(unsigned int screenId, unsigned int color);
int DispSetColorKey(unsigned int screenId, unsigned int color); /*DE2 not support*/
int DispGetScrWidth(unsigned int screenId);
int DispGetScrHeight(unsigned int screenId);
int DispGetOutPutType(unsigned int screenId);
int DispVsyncEventEnable(unsigned int screenId, bool enable);
int DispSetBlankEnable(unsigned int screenId, bool enable); /*DE1 not support*/
int DispShadowProtect(unsigned int screenId, bool protect);
int DispDeviceSwitch(unsigned int screenId,
		enum disp_output_type outPutType, enum disp_tv_mode tvMode, bool enable);
int DispSetColorRange(unsigned int screenId,
		unsigned int colorRange); /*DE1 not support*/
int DispGetColorRange(unsigned int screenId); /*DE1 not support*/

/* ----layer---- */
int DispSetLayerEnable(unsigned int screenId, unsigned int layerId,
		unsigned int channelId, unsigned int layerNum, bool enable);
int DispSetLayerConfig(unsigned int screenId, unsigned int layerId,
		unsigned int layerNum, luapi_layer_config *luapiConfig);
int DispGetLayerConfig(unsigned int screenId, unsigned int layerId,
		unsigned int channelId, unsigned int layerNum,
		luapi_layer_config *luapiConfig);
int DispSetLayerZorder(unsigned int screenId, unsigned int layerId,
		unsigned int channelId, unsigned int layerNum, luapi_zorder zorder);
int DispGetLayerFrameId(unsigned int screenId, unsigned int layerId,
		unsigned int channelId, unsigned int layerNum);

/* ----hdmi---- */
int DispCheckHdmiSupportMode(unsigned int screenId,
		enum disp_tv_mode tvMode);
int DispGetHdmiEdid(unsigned int screenId, unsigned char *buf,
		unsigned int bytes);

/* ----lcd---- */
int DispGetBrightness(unsigned int screenId);
int DispSetBrightness(unsigned int screenId,
		unsigned int brightness);
int DispSetBackLightEnable(unsigned int screenId, bool enable);

/* ----capture---- */
int DispCaptureSatrt(unsigned int screenId,
		luapi_capture_info *luapiPapture);
int DispCaptureStop(unsigned int screenId);

/* ---enhance--- */
int DispSetEnhanceEnable(unsigned int screenId, bool enable);
int DispSetEnhanceDemoEnable(unsigned int screenId, bool enable);	/*DE1 not support*/
int DispGetEnhanceEnable(unsigned int screenId);
int DispSetEnhanceWindow(unsigned int screenId,
		luapi_disp_window dispWindow); /*DE2 not support*/
int DispGetEnhanceWindow(unsigned int screenId,
		luapi_disp_window *dispWindow); /*DE2 not support*/
int DispSetEnhanceMode(unsigned int screenId,
		unsigned int mode);
int DispGetEnhanceMode(unsigned int screenId);
int DispSetEnhanceBright(unsigned int screenId,
		unsigned int bright);	/*DE2 not support*/
int DispGetEnhanceBright(unsigned int screenId);	/*DE2 not support*/
int DispSetEnhanceContrast(unsigned int screenId,
		unsigned int contrast);	/*DE2 not support*/
int DispGetEnhanceContrast(unsigned int screenId);	/*DE2 not support*/
int DispSetEnhanceSatuation(unsigned int screenId,
		unsigned int satuation);	/*DE2 not support*/
int DispGetEnhanceSatuation(unsigned int screenId);	/*DE2 not support*/
int DispSetEnhanceHue(unsigned int screenId, unsigned int hue);	/*DE2 not support*/
int DispGetEnhanceHue(unsigned int screenId);	/*DE2 not support*/

/* ---smart backlight--- */
int DispSetSMBLEnable(unsigned int screenId, bool enable);
int DispGetSMBLEnable(unsigned int screenId); /*DE2 not support*/
int DispSetSMBLWindow(unsigned int screenId,
		luapi_disp_window dispWindow);
int DispGetSMBLWindow(unsigned int screenId,
		luapi_disp_window *dispWindow); /*DE2 not support*/

/* ---mem--- */
int DispMemRequest(unsigned int memId, unsigned int memSize);
int DispMemRelease(unsigned int memId);
unsigned long DispMemGetAdrress(unsigned int memId);

/* ---rotate--- */
int DispSetRotateDegree(unsigned int screenId,
		luaip_rotate_degree degree);	/*DE1 not support*/
int DispGetRotateDegree(unsigned int screenId);	/*DE1 not support*/

#endif /* __DISPLAYINTERFACE_H_ */
