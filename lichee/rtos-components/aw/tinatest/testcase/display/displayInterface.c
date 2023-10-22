#include <stdio.h>
#include <string.h>
#include "displayInterface.h"

extern int disp_ioctl(int cmd, void *arg);

/* ----disp global---- */
/* ----Set the background color---- */
int DispSetBackColor(unsigned int screenId, unsigned int color)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	struct disp_color ck;
	unsigned int r;
	unsigned int g;
	unsigned int b;
	r = (color >> 16) & 0xff;
	g = (color >> 8) & 0xff;
	b = (color >> 0) & 0xff;
	ck.alpha = 0xff;
	ck.red = r;
	ck.green = g;
	ck.blue = b;
	ioctlParam[1] = (unsigned long) &ck;
	return disp_ioctl(DISP_SET_BKCOLOR, ioctlParam);
}

/* ----Set filter color---- */
int DispSetColorKey(unsigned int screenId, unsigned int color)
{
	/*DE2 does not support this cmd*/
	return DISP_NOT_SUPPORT;
}

/* ----Get the screen width---- */
int DispGetScrWidth(unsigned int screenId)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	return disp_ioctl(DISP_GET_SCN_WIDTH, ioctlParam);
}

/* ----Get the screen height---- */
int DispGetScrHeight(unsigned int screenId)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	return disp_ioctl(DISP_GET_SCN_HEIGHT, ioctlParam);
}

/* ----Get the out put type---- */
int DispGetOutPutType(unsigned int screenId)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	return disp_ioctl(DISP_GET_OUTPUT_TYPE, ioctlParam);
}

/* ----Set Vsync event enable---- */
int DispVsyncEventEnable(unsigned int screenId, bool enable)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	ioctlParam[1] = (unsigned long) enable;
	return disp_ioctl(DISP_VSYNC_EVENT_EN, ioctlParam);
}

/* ----Set blank enable---- */
int DispSetBlankEnable(unsigned int screenId, bool enable)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	ioctlParam[1] = (unsigned long) enable;
	return disp_ioctl(DISP_BLANK, ioctlParam);
}

/* ----Set shadow protect---- */
int DispShadowProtect(unsigned int screenId, bool protect)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	ioctlParam[1] = (unsigned long) protect;
	return disp_ioctl(DISP_SHADOW_PROTECT, ioctlParam);
}

/**
 * Device switch
 * You can set lcd/tv/hdmi/vga enable/disable, and set tv mode
 *
 * @outPutType lcd/tv/hdmi/vga, DE1 not support vga
 * @tvMode Screen support for the tv mode, DE1 lcd not support set tv mode
 * @enable 0 is disable, 1 is enable
 */
int DispDeviceSwitch(unsigned int screenId,
		enum disp_output_type outPutType, enum disp_tv_mode tvMode, bool enable)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	if (!enable)
		outPutType = (unsigned long) DISP_OUTPUT_TYPE_NONE;
	ioctlParam[1] = (unsigned long) outPutType;
	ioctlParam[2] = (unsigned long) tvMode;
	return disp_ioctl(DISP_DEVICE_SWITCH, ioctlParam);
}

/* ----Set color range---- */
int DispSetColorRange(unsigned int screenId,
		unsigned int colorRange)
{
	unsigned long ioctlParam[4] = {0};
	ioctlParam[0] = (unsigned long) screenId;
	ioctlParam[1] = (unsigned long) colorRange;
	return disp_ioctl(DISP_SET_COLOR_RANGE, ioctlParam);
}

/* ----Get color range---- */
int DispGetColorRange(unsigned int screenId)
{
	unsigned long ioctlParam[4] = {0};
	ioctlParam[0] = (unsigned long) screenId;
	return disp_ioctl(DISP_GET_COLOR_RANGE, ioctlParam);
}

/* ----layer---- */
/* ----Set layer enable---- */
int DispSetLayerEnable(unsigned int screenId, unsigned int layerId,
		unsigned int channelId, unsigned int layerNum, bool enable)
{
	luapi_layer_config luapiconfig;
	memset(&luapiconfig, 0, sizeof(luapi_layer_config));
	int ret = DispGetLayerConfig(screenId, layerId, channelId, layerNum,
			&luapiconfig);
	if (ret < 0)
		return ret;

	luapiconfig.layerConfig.enable = enable;

	ret = DispSetLayerConfig(screenId, layerId, layerNum, &luapiconfig);

	return ret;
}

/**
 * Set layer config
 * You need to call DispGetLayerConfig before calling DispSetLayerConfig
 *
 * @layerId DE1 is layerId, DE2 is layer num
 */
int DispSetLayerConfig(unsigned int screenId, unsigned int layerId,
		unsigned int layerNum, luapi_layer_config *luapiconfig)
{
	unsigned long ioctlParam[4] = {0};
	ioctlParam[0] = (unsigned long) screenId;
	ioctlParam[1] = (unsigned long) &luapiconfig->layerConfig;
	ioctlParam[2] = (unsigned long) layerNum;
	return disp_ioctl(DISP_LAYER_SET_CONFIG, ioctlParam);
}

/* ----Get layer config---- */
int DispGetLayerConfig(unsigned int screenId, unsigned int layerId,
		unsigned int channelId, unsigned int layerNum,
		luapi_layer_config *luapiConfig)
{
	luapiConfig->layerConfig.channel = channelId;
	luapiConfig->layerConfig.layer_id = layerId;

	unsigned long ioctlParam[4] = {0};
	ioctlParam[0] = (unsigned long) screenId;
	ioctlParam[1] = (unsigned long) &luapiConfig->layerConfig;
	ioctlParam[2] = (unsigned long) layerNum;
	return disp_ioctl(DISP_LAYER_GET_CONFIG, ioctlParam);
}

/* ----Get layer zorder---- */
int DispSetLayerZorder(unsigned int screenId, unsigned int layerId,
		unsigned int channelId, unsigned int layerNum, luapi_zorder zorder)
{
	luapi_layer_config luapiconfig;
	memset(&luapiconfig, 0, sizeof(luapi_layer_config));
	int ret = DispGetLayerConfig(screenId, layerId, channelId, layerNum,
			&luapiconfig);
	if (ret < 0)
		return ret;

	switch (zorder) {
		case LUAPI_ZORDER_TOP:
			luapiconfig.layerConfig.info.zorder = 11;
		break;
		case LUAPI_ZORDER_MIDDLE:
			luapiconfig.layerConfig.info.zorder = 5;
		break;
		case LUAPI_ZORDER_BOTTOM:
			luapiconfig.layerConfig.info.zorder = 0;
		break;
			default:
		break;
	}
	ret = DispSetLayerConfig(screenId, layerId, layerNum, &luapiconfig);
	return ret;
}

/* ----Get layer frame id---- */
int DispGetLayerFrameId(unsigned int screenId, unsigned int layerId,
		unsigned int channelId, unsigned int layerNum)
{
	luapi_layer_config luapiconfig;
	memset(&luapiconfig, 0, sizeof(luapi_layer_config));
	int ret = DispGetLayerConfig(screenId, layerId, channelId, layerNum,
			&luapiconfig);
	if (ret < 0)
		return ret;
	else
		return luapiconfig.layerConfig.info.id;
}

/* ----hdmi---- */
/**
 * Checkout hdmi support mode
 *
 * @tvMode The mode to check
 */
int DispCheckHdmiSupportMode(unsigned int screenId,
		enum disp_tv_mode tvMode)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	ioctlParam[1] = (unsigned long) tvMode;
	return disp_ioctl(DISP_HDMI_SUPPORT_MODE, ioctlParam);
}

/**
 * Get hdmi edid
 * DE1 Only supported in versions using the linux3.4/linux3.10 kernel
 * DE2 Only supported in versions using the linux3.4 kernel
 *
 * @buf Data
 * @bytes Data length, Maximum value 1024
 */
int DispGetHdmiEdid(unsigned int screenId, unsigned char *buf,
		unsigned int bytes)
{
#if 0
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	ioctlParam[1] = (unsigned long) buf;
	ioctlParam[2] = (unsigned long) bytes;
	return disp_ioctl(DISP_HDMI_GET_EDID, ioctlParam);
#else
	return DISP_NOT_SUPPORT;
#endif
}

/* ----lcd---- */
/* ----Get the brightness---- */
int DispGetBrightness(unsigned int screenId)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	return disp_ioctl(DISP_LCD_GET_BRIGHTNESS, ioctlParam);
}

/* ----Set the brightness---- */
int DispSetBrightness(unsigned int screenId,
		unsigned int brightness)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	ioctlParam[1] = (unsigned long) brightness;
	return disp_ioctl(DISP_LCD_SET_BRIGHTNESS, ioctlParam);
}

/**
 * Set back light enable
 * DE1 Only supported in versions using the linux3.4/linux3.10 kernel
 * DE2 Only supported in versions using the linux3.4 kernel
 */
int DispSetBackLightEnable(unsigned int screenId, bool enable)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	if (enable)
		return disp_ioctl(DISP_LCD_BACKLIGHT_ENABLE, ioctlParam);
	else
		return disp_ioctl(DISP_LCD_BACKLIGHT_DISABLE, ioctlParam);
}

/* ----capture---- */
/**
 * Start screen capture
 * DE2 linux3.4 kernel not supported and return -1
 */
int DispCaptureSatrt(unsigned int screenId,
		luapi_capture_info *luapiPapture)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	int ret = disp_ioctl(DISP_CAPTURE_START, ioctlParam);
	if(ret < 0) {
		return ret;
	} else {
		ioctlParam[1] = (unsigned long) &luapiPapture->captureInfo;
		ret = disp_ioctl(DISP_CAPTURE_COMMIT, ioctlParam);
		if(ret < 0)
			disp_ioctl(DISP_CAPTURE_STOP, ioctlParam);
		return ret;
	}
}

/* ----Stop screen capture---- */
int DispCaptureStop(unsigned int screenId)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	return disp_ioctl(DISP_CAPTURE_STOP, ioctlParam);
}

/* ---enhance --- */
/* ----Set enhance enable---- */
int DispSetEnhanceEnable(unsigned int screenId, bool enable)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	if (enable)
		return disp_ioctl(DISP_ENHANCE_ENABLE, ioctlParam);
	else
		return disp_ioctl(DISP_ENHANCE_DISABLE, ioctlParam);
}

/* ----Set enhance demo enable---- */
int DispSetEnhanceDemoEnable(unsigned int screenId, bool enable)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	if (enable)
		return disp_ioctl(DISP_ENHANCE_DEMO_ENABLE, ioctlParam);
	else
		return disp_ioctl(DISP_ENHANCE_DEMO_DISABLE, ioctlParam);
}

/* ----Get enhance enable---- */
int DispGetEnhanceEnable(unsigned int screenId)
{
	return DISP_NOT_SUPPORT;
}

/* ----Set enhance window---- */
int DispSetEnhanceWindow(unsigned int screenId,
		luapi_disp_window dispWindow)
{
	return DISP_NOT_SUPPORT;
}

/* ----Get enhance window---- */
int DispGetEnhanceWindow(unsigned int screenId,
		luapi_disp_window *dispWindow)
{
	return DISP_NOT_SUPPORT;
}

/* ----Set enhance mode---- */
int DispSetEnhanceMode(unsigned int screenId, unsigned int mode)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	ioctlParam[1] = (unsigned long) mode;
	return disp_ioctl(DISP_ENHANCE_SET_MODE, ioctlParam);
}

/* ----Get enhance mode---- */
int DispGetEnhanceMode(unsigned int screenId)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	return disp_ioctl(DISP_ENHANCE_GET_MODE, ioctlParam);
}

/**
 * Set enhance bright
 * Only supported in versions using the linux3.10 kernel
 */
int DispSetEnhanceBright(unsigned int screenId, unsigned int bright)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	ioctlParam[1] = (unsigned long) bright;
	return disp_ioctl(DISP_ENHANCE_SET_BRIGHT, ioctlParam);
}

/**
 * Get enhance bright
 * Only supported in versions using the linux3.10 kernel
 */
int DispGetEnhanceBright(unsigned int screenId)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	return disp_ioctl(DISP_ENHANCE_GET_BRIGHT, ioctlParam);
}

/**
 * Set enhance contrast
 * Only supported in versions using the linux3.10 kernel
 */
int DispSetEnhanceContrast(unsigned int screenId,
		unsigned int contrast)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	ioctlParam[1] = (unsigned long) contrast;
	return disp_ioctl(DISP_ENHANCE_SET_CONTRAST, ioctlParam);
}

/**
 * Get enhance contrast
 * Only supported in versions using the linux3.10 kernel
 */
int DispGetEnhanceContrast(unsigned int screenId)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	return disp_ioctl(DISP_ENHANCE_GET_CONTRAST, ioctlParam);
}

/**
 * Set enhance satuation
 * Only supported in versions using the linux3.10 kernel
 */
int DispSetEnhanceSatuation(unsigned int screenId,
		unsigned int satuation)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	ioctlParam[1] = (unsigned long) satuation;
	return disp_ioctl(DISP_ENHANCE_SET_SATURATION, ioctlParam);
}

/**
 * Get enhance satuation
 * Only supported in versions using the linux3.10 kernel
 */
int DispGetEnhanceSatuation(unsigned int screenId)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	return disp_ioctl(DISP_ENHANCE_GET_SATURATION, ioctlParam);
}

/**
 * Set enhance hue
 * Only supported in versions using the linux3.10 kernel
 */
int DispSetEnhanceHue(unsigned int screenId, unsigned int hue)
{
#if 0
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	ioctlParam[1] = (unsigned long) hue;
	return disp_ioctl(DISP_ENHANCE_GET_HUE, ioctlParam);
#endif
	return DISP_NOT_SUPPORT;
}

/**
 * Get enhance hue
 * Only supported in versions using the linux3.10 kernel
 */
int DispGetEnhanceHue(unsigned int screenId)
{
#if 0
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	return disp_ioctl(DISP_ENHANCE_SET_HUE, ioctlParam);
#endif
	return DISP_NOT_SUPPORT;
}

/* ----smart backlight---- */
/* ----Set smart backlight enable---- */
int DispSetSMBLEnable(unsigned int screenId, bool enable)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	if(enable)
		return disp_ioctl(DISP_SMBL_ENABLE, ioctlParam);
	else
		return disp_ioctl(DISP_SMBL_DISABLE, ioctlParam);
}

/* ----Get smart backlight enable---- */
int DispGetSMBLEnable(unsigned int screenId)
{
	return DISP_NOT_SUPPORT;
}

/* ----Set smart backlight window---- */
int DispSetSMBLWindow(unsigned int screenId,
		luapi_disp_window dispWindow)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	struct disp_rect window;
	window.x = dispWindow.x;
	window.y = dispWindow.y;
	window.width = dispWindow.width;
	window.height = dispWindow.height;
	ioctlParam[1] = (unsigned long) &window;
	return disp_ioctl(DISP_SMBL_SET_WINDOW, ioctlParam);
}

/* ----Get smart backlight window---- */
int DispGetSMBLWindow(unsigned int screenId,
		luapi_disp_window *dispWindow)
{
	return DISP_NOT_SUPPORT;
}

/* ---mem--- */
/* ----Mem Request---- */
int DispMemRequest(unsigned int memId, unsigned int memSize)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) memId;
	ioctlParam[1] = (unsigned long) memSize;
	return disp_ioctl(DISP_MEM_REQUEST, ioctlParam);
}

/* ----Mem Release---- */
int DispMemRelease(unsigned int memId)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) memId;
	return disp_ioctl(DISP_MEM_RELEASE, ioctlParam);
}

/* ----Mem Get Adrress---- */
unsigned long DispMemGetAdrress(unsigned int memId)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) memId;
	return disp_ioctl(DISP_MEM_GETADR, ioctlParam);
}

/* ---rotate--- */
/**
 * Set Rotate Degree
 * Only supported in versions using the linux3.4 kernel
 * You have to modify sys_config.fex, enable rotate_sw, this function to work
 */
int DispSetRotateDegree(unsigned int screenId,
		luaip_rotate_degree degree)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	ioctlParam[1] = (unsigned long) degree;
	return disp_ioctl(DISP_ROTATION_SW_SET_ROT, ioctlParam);
}

/**
 * Get Rotate Degree
 * Only supported in versions using the linux3.4 kernel
 * You have to modify sys_config.fex, enable rotate_sw, this function to work
 */
int DispGetRotateDegree(unsigned int screenId)
{
	unsigned long ioctlParam[4] = { 0 };
	ioctlParam[0] = (unsigned long) screenId;
	return disp_ioctl(DISP_ROTATION_SW_GET_ROT, ioctlParam);
}
