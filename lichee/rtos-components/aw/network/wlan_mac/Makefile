obj-y += wlan_mac.o

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/compat/posix
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -I components/common/aw/network/tcpip
CFLAGS += -I components/common/aw/xradio/cmd
CFLAGS += -I components/common/aw/xradio/include
CFLAGS += -I components/common/aw/xradio/include/sysinfo
CFLAGS += -I components/common/aw/network/wlan_mac
CFLAGS += -I components/common/aw/wireless/wifimanager/demo
CFLAGS += -I components/common/aw/wireless/wifimanager/core/include
CFLAGS += -I drivers/drv/wireless/xradio/adapter/net_ctrl
CFLAGS += -I drivers/drv/wireless/xradio/include/net/ethernetif
CFLAGS += -I drivers/drv/wireless/xradio/adapter/cmd
CFLAGS += -I drivers/drv/wireless/xradio/include
CFLAGS += -I drivers/drv/wireless/xradio/include/net/wlan
CFLAGS += -I drivers/drv/wireless/xradio/os/include
CFLAGS += -I drivers/drv/wireless/xradio/os/include/net/wlan
