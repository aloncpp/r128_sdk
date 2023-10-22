
#HERE1 := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
#BT_BASE:=$(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))


#$(LINKER_SCRIPTS): CFLAGS += -I $(HERE1)zephyr/include/

ifeq ($(CONFIG_ARCH_SUN20IW2P1), y)
$(XRADIO2_OBJS):CFLAGS +=-DCONFIG_OS_TINA
$(XRADIO2_OBJS):CFLAGS += -I $(BASE)/include/arch/arm/armv8m/
$(XRADIO2_OBJS):CFLAGS += -I $(BASE)/include/arch/arm/mach

$(XRADIO2_OBJS):CFLAGS += -I $(BASE)/components/common/aw/xradio
$(XRADIO2_OBJS):CFLAGS += -I $(BASE)/components/common/aw/xradio/os
$(XRADIO2_OBJS):CFLAGS += -I $(BASE)/components/common/aw/xradio/os/include
$(XRADIO2_OBJS):CFLAGS += -I $(BASE)/components/common/aw/xradio/include

else ifeq ($(CONFIG_ARCH_SUN8IW18P1), y)
$(XRADIO2_OBJS):CFLAGS += -I $(BASE)/drivers/drv/wireless/xradio/os1/include
$(XRADIO2_OBJS):CFLAGS += -I $(BASE)/drivers/drv/wireless/xradio
$(XRADIO2_OBJS):CFLAGS += -I $(BASE)/drivers/drv/wireless/xradio/adapter/net_ctrl
$(XRADIO2_OBJS):CFLAGS += -I $(BASE)/components/common/thirdparty/network/lwip/src/include
$(XRADIO2_OBJS):CFLAGS += -I $(BASE)/include/drivers/

endif
