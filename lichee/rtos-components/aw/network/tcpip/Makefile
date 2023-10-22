obj-y += tcpip_adapter.o

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

CFLAGS += -I include/drivers/
CFLAGS += -I include/freertos/

CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -I components/common/aw/xradio/include
CFLAGS += -I drivers/drv/wireless/sunxi-platform

CFLAGS += -DCONFIG_LWIP
