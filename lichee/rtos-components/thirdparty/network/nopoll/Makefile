# nopoll
obj-y += src/nopoll.o \
	  	 src/nopoll_msg.o \
		 src/nopoll_loop.o \
		 src/nopoll_log.o \
		 src/nopoll_listener.o \
		 src/nopoll_io.o \
		 src/nopoll_FreeRTOS.o \
		 src/nopoll_decl.o \
		 src/nopoll_ctx.o \
		 src/nopoll_conn_opts.o \
		 src/nopoll_conn.o

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

ifeq ($(CONFIG_COMPONENTS_MBEDTLS_2_16_0),y)
MBEDTLS = mbedtls-2.16.0
else ifeq ($(CONFIG_COMPONENTS_MBEDTLS_2_7_0),y)
MBEDTLS = mbedtls-2.7.0
endif

CFLAGS += -Icomponents/common/thirdparty/network/nopoll/include
CFLAGS += -Icomponents/common/thirdparty/network/nopoll/src
CFLAGS += -Icomponents/common/thirdparty/network/$(MBEDTLS)/include/mbedtls
CFLAGS += -Icomponents/common/thirdparty/network/$(MBEDTLS)/include
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include/lwip
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -Idrivers/drv/wireless/xradio/os
CFLAGS += -Idrivers/drv/wireless/xradio/os/include

include components/common/aw/xradio/plt_def.mk
CFLAGS += $(PLT_CFLAGS)
