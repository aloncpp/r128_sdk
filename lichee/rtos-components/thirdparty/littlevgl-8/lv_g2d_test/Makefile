include components/common/thirdparty/littlevgl-8/sunxifb.mk

ifeq ($(CONFIG_LVGL8_USE_SUNXIFB_G2D),y)
CFLAGS+=-DLV_USE_SUNXIFB_G2D_FILL \
		-DLV_USE_SUNXIFB_G2D_BLEND \
		-DLV_USE_SUNXIFB_G2D_BLIT \
		-DLV_USE_SUNXIFB_G2D_SCALE
endif

CFLAGS += -I components/common/thirdparty/littlevgl-8/
CFLAGS += -I components/common/thirdparty/littlevgl-8/lv_g2d_test/src/

include components/common/thirdparty/littlevgl-8/lvgl/Makefile
include components/common/thirdparty/littlevgl-8/lv_drivers/Makefile

obj-y += src/lv_main.o
