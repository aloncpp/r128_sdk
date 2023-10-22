#
# Copyright (C) 2006-2010 OpenWrt.org
# Copyright (C) 2016-2016 tracewong
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

CFLAGS+=-DLV_CONF_INCLUDE_SIMPLE
CFLAGS+=-DLV_DEMO_CONF_INCLUDE_SIMPLE
CFLAGS+=-DCONF_G2D_VERSION_NEW

ifeq ($(CONFIG_LVGL8_EXAMPLES),y)
CFLAGS += -I $(BASE)/components/common/thirdparty/littlevgl-8/lv_examples/src/
endif

ifeq ($(CONFIG_LVGL8_G2D_TEST),y)
CFLAGS += -I $(BASE)/components/common/thirdparty/littlevgl-8/lv_g2d_test/src/
endif

ifeq ($(CONFIG_LVGL8_USE_SUNXIFB_DOUBLE_BUFFER),y)
CFLAGS+=-DUSE_SUNXIFB_DOUBLE_BUFFER
endif

ifeq ($(CONFIG_LVGL8_USE_SUNXIFB_CACHE),y)
CFLAGS+=-DUSE_SUNXIFB_CACHE
endif

ifeq ($(CONFIG_LVGL8_USE_SUNXIFB_G2D),y)
CFLAGS+=-DUSE_SUNXIFB_G2D
TARGET_LDFLAGS+=-luapi
endif

ifeq ($(CONFIG_LVGL8_USE_SUNXIFB_G2D_ROTATE),y)
CFLAGS+=-DUSE_SUNXIFB_G2D_ROTATE
endif

ifeq ($(CONFIG_DISP2_SUNXI),y)
CFLAGS+=-DUSE_DISP2
endif

ifeq ($(CONFIG_DRIVERS_SPILCD),y)
CFLAGS+=-DUSE_SPILCD
endif

ifeq ($(CONFIG_LVGL8_USE_FREETYPE),y)
CFLAGS+=-I $(BASE)/components/common/thirdparty/freetype/include/
endif
