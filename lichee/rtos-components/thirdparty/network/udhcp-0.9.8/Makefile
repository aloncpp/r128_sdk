obj-y += usr_dhcpd.o \
	arpping.o \
	files.o \
	leases.o \
	serverpacket.o \
	options.o \
	socket.o \
	packet.o \
	dhcp_time.o \
	dns_server.o

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

VER := 0.9.8
CFLAGS += -DVERSION='"$(VER)"' -DXR_DHCPD
CFLAGS += -DDHCPD_LWIP \
			-DDHCPD_HEAP_REPLACE_STACK \
			-DDHCPD_USRCFG \
			-DDHCPD_FREERTOS \
			-DDHCPD_ICMPPING \
			-DDHCPD_LOW_LEVEL \
			-DDHCPD_DNS \
			-DDHCPD_USE_DEFAULT_INIT \
			-DDHCPD_UPGRADE_STA_MAC

CFLAGS += -Iinclude/drivers
CFLAGS += -Idrivers/drv/wireless/xradio/include
CFLAGS += -Idrivers/drv/wireless/xradio
CFLAGS += -Idrivers/drv/wireless/xradio/driver
CFLAGS += -Idrivers/drv/wireless/xradio/os
CFLAGS += -Icomponents/common/aw/xradio/include
CFLAGS += -Icomponents/common/aw/xradio/include/libc

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
$(DHCPD_OBJS):CFLAGS += -I $(BASE)/components/common/aw/xradio/include/libc
endif

#$(DHCPD_OBJS):CFLAGS +=  -I .
#$(DHCPD_OBJS):CFLAGS +=  -I include
CFLAGS +=  -Iinclude/net

CFLAGS += -Iinclude
CFLAGS += -Iinclude/arch/mach

CFLAGS += -Iinclude/FreeRTOS_POSIX
CFLAGS += -Ikernel/Posix/include
CFLAGS += -Iinclude/drivers/
CFLAGS += -Iinclude/freertos/

CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include

include components/common/aw/xradio/plt_def.mk
CFLAGS += $(PLT_CFLAGS)

$(DHCPD_DEMO_OBJS):MODULE_NAME="components-udhcpd"

OBJECTS += $(DHCPD_OBJS)
