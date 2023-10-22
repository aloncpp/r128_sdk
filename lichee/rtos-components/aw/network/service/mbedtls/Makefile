obj-y += custom_certs.o \
	server.o \
    tls.o \
    client.o \
    cmd_tls.o

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

CFLAGS += -I include
CFLAGS += -I components/common/aw/xradio/include
CFLAGS += -I components/common/aw/xradio/cmd
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -I components/common/thirdparty/network/$(MBEDTLS)/include/mbedtls
CFLAGS += -I components/common/thirdparty/network/$(MBEDTLS)/include
