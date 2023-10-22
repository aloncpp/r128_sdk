obj-y += cmd_shttpd.o \
		web_server_demo.o

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

CFLAGS += -I components/common/aw/xradio/cmd
CFLAGS += -I drivers/drv/wireless/xradio/include
CFLAGS += -I components/common/thirdparty/network/shttpd/include
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -I components/common/thirdparty/network/$(MBEDTLS)/include

include components/common/aw/xradio/plt_def.mk
CFLAGS += $(PLT_CFLAGS)
