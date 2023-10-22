obj-y += wlan_if.o

ifeq ($(CONFIG_DRIVERS_RTL8723DS), y)
obj-y += rtl8723ds.o
CFLAGS += -I drivers/drv/wireless/rtl8723ds/os/os_dep/include
CFLAGS += -I drivers/drv/wireless/rtl8723ds/include
CFLAGS += -DCONFIG_RTL8723DS
endif

ifeq ($(CONFIG_ARCH_SUN8IW18P1), y)
CFLAGS += -I drivers/rtos-hal/include/hal/sdmmc/sys
endif

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -I drivers/drv/wireless/sunxi-platform
CFLAGS += -I drivers/drv/rfkill
CFLAGS += -I components/common/aw/network/tcpip
CFLAGS += -I components/common/aw/xradio/include
