# shttpd
obj-y += src/auth.o \
		src/cgi.o \
		src/compat_rtems.o \
		src/compat_rtos.o \
		src/compat_unix.o \
		src/io_cgi.o \
		src/io_dir.o \
		src/io_emb.o \
		src/io_file.o \
		src/io_socket.o \
		src/io_ssi.o \
		src/io_ssl.o \
		src/log.o \
		src/md5.o \
		src/shttpd.o \
		src/string.o
		
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

CFLAGS += -Icomponents/common/thirdparty/network/shttpd/include
CFLAGS += -Icomponents/common/thirdparty/network/$(MBEDTLS)/include/mbedtls
CFLAGS += -Icomponents/common/thirdparty/network/$(MBEDTLS)/include
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include/lwip
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -Idrivers/drv/wireless/xradio/os
CFLAGS += -Idrivers/drv/wireless/xradio/os/include
CFLAGS += -Idrivers/drv/wireless/xradio/include

include components/common/aw/xradio/plt_def.mk
CFLAGS += $(PLT_CFLAGS)
