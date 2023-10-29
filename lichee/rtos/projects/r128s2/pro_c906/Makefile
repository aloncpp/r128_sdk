obj-y += src/main.o
obj-y += src/assert.o
obj-y += src/hooks.o
obj-y += src/alsa_config.o
obj-y += src/card_default.o
obj-y += src/wifi_test.o
obj-y += src/ledc_test.o


ifeq ($(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM),y)
CFLAGS += -I $(BASE)/components/common/aw/AudioSystem/include
endif

ifeq ($(CONFIG_COMPONENTS_WIRELSEE_COMMON),y)
CFLAGS += -I $(BASE)/components/common/aw/wireless/wifimanager/core/include
endif

ifeq ($(CONFIG_COMPONENTS_HTTPCLIENT), y)
CFLAGS += -I $(BASE)/components/common/aw/network/service/httpclient
endif

ifeq ($(CONFIG_COMPONENTS_LWIP),y)
ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif
CFLAGS += -I $(BASE)/components/common/aw/xradio/include
CFLAGS += -I $(BASE)/components/common/thirdparty/network/$(LWIP)/src/include
endif

ifeq ($(CONFIG_COMPONENTS_HTTPCLIENT),y)
CFLAGS += -I $(BASE)/components/common/thirdparty/network/httpclient
CFLAGS += -I $(BASE)/components/common/thirdparty/network/httpclient/API
endif


ifeq ($(CONFIG_COMPONENTS_WLAN_CSFC),y)
CFLAGS += -I $(BASE)/components/common/aw/network/wlan_csfc
endif
