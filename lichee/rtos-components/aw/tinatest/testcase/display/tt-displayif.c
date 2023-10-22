#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <hal_mem.h>
#include <video/sunxi_display2.h>
#include <tinatest.h>

#include "displayInterface.h"
#include "displaytest.h"

extern int disp_open(void);
extern int disp_release(void);

struct test_lcd_info {
	int screen_id;
};

struct test_lcd_info test_info;

int tt_displayif(int argc, char **argv)
{
	int ret;

	memset(&test_info, 0, sizeof(struct test_lcd_info));
	ret = disp_open();
	if (ret == -1) {
		DISP_TEST_ERROR(("open display device faild\n"));
		goto err;
	}

	test_info.screen_id = 0;

	if (DispSetBackColor(test_info.screen_id, 0xFFDC143C) < 0) {
		DISP_TEST_FAIL(("DispSetBackColor FAILED"));
	} else {
		DISP_TEST_OK(("DispSetBackColor SUCCESS color is 0xFFDC143C"));
	}

	int width = DispGetScrWidth(test_info.screen_id);
	if (width >= 0) {
		DISP_TEST_OK(("DispGetScrWidth SUCCESS width is %d", width));
	} else {
		DISP_TEST_FAIL(("DispGetScrWidth FAILED"));
	}

	int height = DispGetScrHeight(test_info.screen_id);
	if (height >= 0) {
		DISP_TEST_OK(("DispGetScrHeight SUCCESS height is %d", height));
	} else {
		DISP_TEST_FAIL(("DispGetScrHeight FAILED"));
	}

	int outPutType = DispGetOutPutType(test_info.screen_id);
	if (outPutType > 0) {
		DISP_TEST_OK(
				("DispGetOutPutType SUCCESS outPutType is %d", outPutType));
	} else {
		DISP_TEST_FAIL(("DispGetOutPutType FAILED"));
	}

	ret = DispDeviceSwitch(test_info.screen_id, DISP_OUTPUT_TYPE_LCD,
			DISP_TV_MOD_1080P_60HZ, 1);
	if (ret >= 0) {
		DISP_TEST_OK(("DispDeviceSwitch SUCCESS type is LCD"));
	} else if (ret == DISP_NOT_SUPPORT) {
		DISP_TEST_WARN(("DispSetBlankEnable NOT SUPPORT"));
	} else {
		DISP_TEST_FAIL(("DispDeviceSwitch FAILED"));
	}

	if (DispVsyncEventEnable(test_info.screen_id, 1) < 0) {
		DISP_TEST_FAIL(("DispVsyncEventEnable FAILED"));
	} else {
		DISP_TEST_OK(("DispVsyncEventEnable SUCCESS enable is 1"));
	}

	ret = DispSetBlankEnable(test_info.screen_id, 0);
	if (ret >= 0) {
		DISP_TEST_OK(("DispSetBlankEnable SUCCESS enable is 0"));
	} else if (ret == DISP_NOT_SUPPORT) {
		DISP_TEST_WARN(("DispSetBlankEnable NOT SUPPORT"));
	} else {
		DISP_TEST_FAIL(("DispSetBlankEnable FAILED"));
	}

	if (DispShadowProtect(test_info.screen_id, 1) < 0) {
		DISP_TEST_FAIL(("DispShadowProtect FAILED"));
	} else {
		DISP_TEST_OK(("DispShadowProtect SUCCESS"));
	}

	if (DispSetColorRange(test_info.screen_id, 80) < 0) {
		DISP_TEST_FAIL(("DispSetColorRange FAILED"));
	} else {
		DISP_TEST_OK(("DispSetColorRange SUCCESS color range is 80"));
	}

	int colorRange = DispGetColorRange(test_info.screen_id);
	if (colorRange < 0) {
		DISP_TEST_FAIL(("DispGetColorRange FAILED"));
	} else {
		DISP_TEST_OK(
				("DispGetColorRange SUCCESS color range is %d",colorRange));
	}

	if (DispSetLayerEnable(test_info.screen_id, 0, 1, 1, 1) < 0) {
		DISP_TEST_FAIL(("DispSetLayerEnable FAILED"));
	} else {
		DISP_TEST_OK(("DispSetLayerEnable SUCCESS enable is 1"));
	}

	luapi_layer_config luapiconfig;
	memset(&luapiconfig, 0, sizeof(luapi_layer_config));
	if (DispGetLayerConfig(test_info.screen_id, 0, 1, 1, &luapiconfig)
			< 0) {
		DISP_TEST_FAIL(("DispGetLayerConfig FAILED"));
	} else {
		DISP_TEST_OK(("DispGetLayerConfig SUCCESS"));
	}

	if (DispSetLayerConfig(test_info.screen_id, 0, 1, &luapiconfig)
			< 0) {
		DISP_TEST_FAIL(("DispSetLayerConfig FAILED"));
	} else {
		DISP_TEST_OK(("DispSetLayerConfig SUCCESS"));
	}

	if (DispSetLayerZorder(test_info.screen_id, 0, 1, 1,
			LUAPI_ZORDER_TOP) < 0) {
		DISP_TEST_FAIL(("DispSetLayerZorder FAILED"));
	} else {
		DISP_TEST_OK(("DispSetLayerZorder SUCCESS"));
	}

	int frameId = DispGetLayerFrameId(test_info.screen_id, 0, 1, 1);
	if (frameId < 0) {
		DISP_TEST_FAIL(("DispGetLayerFrameId FAILED"));
	} else {
		DISP_TEST_OK(("DispGetLayerFrameId SUCCESS frameId is %d",frameId));
	}

	if (DispCheckHdmiSupportMode(test_info.screen_id,
			DISP_TV_MOD_1080P_60HZ) < 0) {
		DISP_TEST_FAIL(("DispCheckHdmiSupportMode FAILED"));
	} else {
		DISP_TEST_OK(
				("DispCheckHdmiSupportMode SUCCESS checkout mode is 1080P 60HZ"));
	}

	int brightness = DispGetBrightness(test_info.screen_id);
	if (brightness < 0) {
		DISP_TEST_FAIL(("DispGetBrightness FAILED"));
	} else {
		DISP_TEST_OK(("DispGetBrightness SUCCESS brightness is %d",brightness));
	}

	if (DispSetBrightness(test_info.screen_id, brightness) < 0) {
		DISP_TEST_FAIL(("DispSetBrightness FAILED"));
	} else {
		DISP_TEST_OK(("DispSetBrightness SUCCESS brightness is %d",brightness));
	}

	if (DispSetBackLightEnable(test_info.screen_id, 1) < 0) {
		DISP_TEST_FAIL(("DispSetBackLightEnable FAILED"));
	} else {
		DISP_TEST_OK(("DispSetBackLightEnable SUCCESS enable is 1"));
	}

	if (DispSetEnhanceEnable(test_info.screen_id, 1) < 0) {
		DISP_TEST_FAIL(("DispSetEnhanceEnable FAILED"));
	} else {
		DISP_TEST_OK(("DispSetEnhanceEnable SUCCESS enable is 1"));
	}

	ret = DispSetEnhanceDemoEnable(test_info.screen_id, 1);
	if (ret >= 0) {
		DISP_TEST_OK(("DispSetEnhanceDemoEnable SUCCESS enable is 1"));
	} else if (ret == DISP_NOT_SUPPORT) {
		DISP_TEST_WARN(("DispSetEnhanceDemoEnable NOT SUPPORT"));
	} else {
		DISP_TEST_FAIL(("DispSetEnhanceDemoEnable FAILED"));
	}

	int enhanceMode = DispGetEnhanceMode(test_info.screen_id);
	if (enhanceMode < 0) {
		DISP_TEST_FAIL(("DispGetEnhanceMode FAILED"));
		enhanceMode = 8;
	} else {
		DISP_TEST_OK(
				("DispGetEnhanceMode SUCCESS enhanceMode is %d",enhanceMode));
	}

	if (DispSetEnhanceMode(test_info.screen_id, enhanceMode) < 0) {
		DISP_TEST_FAIL(("DispSetEnhanceMode FAILED"));
	} else {
		DISP_TEST_OK(
				("DispSetEnhanceMode SUCCESS enhanceMode is %d",enhanceMode));
	}

	int enhanceBright = DispGetEnhanceBright(test_info.screen_id);
	if (enhanceBright >= 0) {
		DISP_TEST_OK(
				("DispGetEnhanceBright SUCCESS enhanceBright is %d", enhanceBright));
	} else if (enhanceBright == DISP_NOT_SUPPORT) {
		DISP_TEST_WARN(("DispGetEnhanceBright NOT SUPPORT"));
		enhanceBright = 50;
	} else {
		DISP_TEST_FAIL(("DispGetEnhanceBright FAILED"));
		enhanceBright = 50;
	}

	ret = DispSetEnhanceBright(test_info.screen_id, enhanceBright);
	if (ret >= 0) {
		DISP_TEST_OK(
				("DispSetEnhanceBright SUCCESS enhanceBright is %d",enhanceBright));
	} else if (ret == DISP_NOT_SUPPORT) {
		DISP_TEST_WARN(("DispSetEnhanceBright NOT SUPPORT"));
	} else {
		DISP_TEST_FAIL(("DispSetEnhanceBright FAILED"));
	}

	int enhanceContrast = DispGetEnhanceContrast(test_info.screen_id);
	if (enhanceContrast >= 0) {
		DISP_TEST_OK(
				("DispGetEnhanceContrast SUCCESS enhanceContrast is %d", enhanceContrast));
	} else if (enhanceContrast == DISP_NOT_SUPPORT) {
		DISP_TEST_WARN(("DispGetEnhanceContrast NOT SUPPORT"));
		enhanceContrast = 50;
	} else {
		DISP_TEST_FAIL(("DispGetEnhanceContrast FAILED"));
		enhanceContrast = 50;
	}

	ret = DispSetEnhanceContrast(test_info.screen_id, enhanceContrast);
	if (ret >= 0) {
		DISP_TEST_OK(
				("DispSetEnhanceContrast SUCCESS enhanceContrast is %d",enhanceContrast));
	} else if (ret == DISP_NOT_SUPPORT) {
		DISP_TEST_WARN(("DispSetEnhanceContrast NOT SUPPORT"));
	} else {
		DISP_TEST_FAIL(("DispSetEnhanceContrast FAILED"));
	}

	int enhanceSatuation = DispGetEnhanceSatuation(test_info.screen_id);
	if (enhanceSatuation >= 0) {
		DISP_TEST_OK(
				("DispGetEnhanceSatuation SUCCESS enhanceSatuation is %d", enhanceSatuation));
	} else if (enhanceSatuation == DISP_NOT_SUPPORT) {
		DISP_TEST_WARN(("DispGetEnhanceSatuation NOT SUPPORT"));
		enhanceSatuation = 50;
	} else {
		DISP_TEST_FAIL(("DispGetEnhanceSatuation FAILED"));
		enhanceSatuation = 50;
	}

	ret = DispSetEnhanceSatuation(test_info.screen_id,
			enhanceSatuation);
	if (ret >= 0) {
		DISP_TEST_OK(
				("DispSetEnhanceSatuation SUCCESS enhanceSatuation is %d",enhanceSatuation));
	} else if (ret == DISP_NOT_SUPPORT) {
		DISP_TEST_WARN(("DispSetEnhanceSatuation NOT SUPPORT"));
	} else {
		DISP_TEST_FAIL(("DispSetEnhanceSatuation FAILED"));
	}

	int smblEnable = 0;
	if (DispSetSMBLEnable(test_info.screen_id, smblEnable) < 0) {
		DISP_TEST_FAIL(("DispSetSMBLEnable FAILED"));
	} else {
		DISP_TEST_OK(("DispSetSMBLEnable SUCCESS smblEnable is %d",smblEnable));
	}

	luapi_disp_window smblWindow;
	smblWindow.x = 0;
	smblWindow.y = 0;
	smblWindow.width = width;
	smblWindow.height = height;
	if (DispSetSMBLWindow(test_info.screen_id, smblWindow) < 0){
		DISP_TEST_FAIL(("DispSetSMBLWindow FAILED"));
	} else {
		DISP_TEST_OK(
				("DispSetSMBLWindow SUCCESS x = %d y = %d w = %u h = %u",
						smblWindow.x,smblWindow.y,smblWindow.width,smblWindow.height));
	}

#if defined(CONFIG_SUNXI_DISP2_FB_ROTATION_SUPPORT)
	int rotateDegree = DispGetRotateDegree(test_info.screen_id);
	if (rotateDegree >= 0) {
		DISP_TEST_OK(
				("DispGetRotateDegree SUCCESS rotateDegree is %d", rotateDegree));
	} else if (rotateDegree == DISP_NOT_SUPPORT) {
		DISP_TEST_WARN(("DispGetRotateDegree NOT SUPPORT"));
		rotateDegree = ROTATION_DEGREE_0;
	} else {
		DISP_TEST_FAIL(("DispGetRotateDegree FAILED"));
		rotateDegree = ROTATION_DEGREE_0;
	}

	ret = DispSetRotateDegree(test_info.screen_id, rotateDegree);
	if (ret >= 0) {
		DISP_TEST_OK(("DispSetRotateDegree SUCCESS degree is %d", rotateDegree));
	} else if (ret == DISP_NOT_SUPPORT) {
		DISP_TEST_WARN(("DispSetRotateDegree NOT SUPPORT"));
	} else {
		DISP_TEST_FAIL(("DispSetRotateDegree FAILED"));
	}
#endif

	disp_release();
	DISP_TEST_INFO(("GOOD, display interface test end.\n"));
	return 0;

	err: disp_release();
	DISP_TEST_ERROR(("ERROR, display interface test failed."));
	return -1;
}

testcase_init(tt_displayif, displayif, displayif for tinatest);
