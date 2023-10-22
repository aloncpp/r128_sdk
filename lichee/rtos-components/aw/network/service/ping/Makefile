ifeq ($(CONFIG_ARCH_SUN20IW2P1),y)
obj-y += cmd_xr_ping.o
else
obj-y += cmd_ping.o
endif

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

ifeq ($(CONFIG_ARCH_SUN20IW2P1),y)
CFLAGS += -I components/common/aw/xradio/cmd
endif

CFLAGS += -I include
CFLAGS += -I components/common/aw/xradio/include
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -I components/common/thirdparty/network/ping

CFLAGS += -DLWIP_IPV4 \
	                            -DLWIP_COMPAT_SOCKETS \
	                            -DLWIP_DNS \
	                            -DLWIP_SOCKET \
	                            -DLWIP_MDNS_RESPONDER \
