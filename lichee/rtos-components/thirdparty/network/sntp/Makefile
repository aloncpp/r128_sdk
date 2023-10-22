obj-y += sntp.o
ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

CFLAGS += -Iinclude/

include components/common/aw/xradio/plt_def.mk
CFLAGS += $(PLT_CFLAGS)

CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -Icomponents/common/thirdparty/network/sntp
