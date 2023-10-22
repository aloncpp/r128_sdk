obj-y += cmd_sntp.o

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
    LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
    LWIP = lwip-2.0.3
endif

include $(BASE)/components/common/aw/xradio/plt_def.mk
CFLAGS += $(PLT_CFLAGS)
CFLAGS += -I include
CFLAGS += -I components/common/aw/xradio/cmd
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -I components/common/thirdparty/network/sntp
