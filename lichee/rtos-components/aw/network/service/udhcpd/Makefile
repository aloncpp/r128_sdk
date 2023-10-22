obj-y += cmd_dhcpd.o

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
    LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
    LWIP = lwip-2.0.3
endif

CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -I components/common/thirdparty/network/udhcp-0.9.8/include
CFLAGS += -I components/common/aw/xradio/cmd
CFLAGS += -I components/common/aw/xradio/include
