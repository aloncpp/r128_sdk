ifeq ($(CONFIG_AW_COMPONENT_BLUEDROID_CTRL), y)
obj-y         += cmd_btc_init.o

include components/common/aw/xradio/plt_def.mk

CFLAGS        += $(PLT_CFLAGS)
CFLAGS        += -I include/drivers/
CFLAGS        += -I include/freertos/
CFLAGS        += -I drivers/drv/wireless/xradio/os/include
CFLAGS        += -I components/common/aw/xradio/cmd/
CFLAGS        += -I drivers/drv/wireless/xradio/

#LDFLAGS      += -Ldrivers/drv/wireless/xradio -lxrbtc
endif
