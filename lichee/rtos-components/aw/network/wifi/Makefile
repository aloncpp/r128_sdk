obj-y += wifi_glue.o
obj-y += AGWifiImpl.o
obj-y += AGWifiImplTest.o

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include

ifeq ($(CONFIG_DRIVERS_RTL8723DS), y)
CFLAGS += -I drivers/board/wireless/rtl8723ds/include
CFLAGS += -I drivers/board/wireless/rtl8723ds/os/os_dep/include
CFLAGS += -I drivers/board/wireless/rtl8723ds/platform/include
CFLAGS += -I drivers/board/wireless/rtl8723ds/platform/sdio/include
CFLAGS += -I drivers/board/wireless/rtl8723ds/platform/rtwlan_bsp
CFLAGS += -I drivers/board/wireless/rtl8723ds/os/customer_rtos
CFLAGS += -I drivers/board/wireless/rtl8723ds/api
CFLAGS += -I drivers/board/wireless/rtl8723ds/platform/sdio
endif

CFLAGS += -I include/drivers/
CFLAGS += -I include/freertos/
CFLAGS += -I components/common/aw/network/tcpip
