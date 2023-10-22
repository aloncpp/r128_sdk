obj-$(CONFIG_COMPONENTS_AW_OTA_V2) +=	\
			aw_upgrade.o \
			aw_ota_core.o

obj-$(CONFIG_COMPONENTS_AW_OTA_V2_NETWORK) +=	\
			aw_ota_http.o

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

ifeq ($(CONFIG_COMPONENTS_AW_OTA_V2_NETWORK),y)
CFLAGS += -I components/common/aw/xradio/include/
CFLAGS += -I components/common/thirdparty/network/
CFLAGS += -I components/common/thirdparty/network/httpclient/API/
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
endif
