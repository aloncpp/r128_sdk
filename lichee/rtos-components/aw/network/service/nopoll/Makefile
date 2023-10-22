obj-y += cmd_nopoll.o

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

CFLAGS += -I components/common/thirdparty/network/nopoll/include
CFLAGS += -I components/common/thirdparty/network/nopoll/src
CFLAGS += -I components/common/thirdparty/network/$(MBEDTLS)/include/mbedtls
CFLAGS += -I components/common/thirdparty/network/$(MBEDTLS)/include
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/lwip
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -I components/common/aw/xradio/cmd
CFLAGS += -I drivers/drv/wireless/xradio/os
CFLAGS += -I drivers/drv/wireless/xradio/os/include
CFLAGS += -I drivers/drv/wireless/xradio/adapter/cmd

include components/common/aw/xradio/plt_def.mk
CFLAGS += $(PLT_CFLAGS)
