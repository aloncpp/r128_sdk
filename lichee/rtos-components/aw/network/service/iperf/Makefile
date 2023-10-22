obj-y += iperf.o cmd_iperf.o

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/lwip
CFLAGS += -I components/common/aw/network/servie/iperf
CFLAGS += -I components/common/aw/network/tcpip
CFLAGS += -I components/common/aw/xradio/include

ifeq ($(CONFIG_ARCH_SUN20IW2), y)
CFLAGS += -I drivers/drv/wireless/xradio/adapter
CFLAGS += -I drivers/drv/wireless/xradio/include
endif

CFLAGS += -DPRJCONF_NET_EN=1
