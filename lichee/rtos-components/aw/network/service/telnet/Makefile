obj-y += telnet.o telnet_console.o

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

include components/common/aw/xradio/plt_def.mk
CFLAGS += $(PLT_CFLAGS)

CFLAGS += -I components/common/aw/ringbuffer/include
CFLAGS += -I components/common/thirdparty/finsh_cli
CFLAGS += -I components/common/aw/network/service/telnet
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -I include
CFLAGS += -DRT_USING_MUTEX -DRT_USING_SEMAPHORE -DRT_USING_DEVICE
