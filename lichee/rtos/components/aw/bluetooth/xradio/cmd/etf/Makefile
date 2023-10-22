ifeq ($(CONFIG_AW_COMPONENT_ETF), y)
XR_ETF_OBJS += btetf.o

CFLAGS        += -I components/common/aw/xradio/include
CFLAGS        += -I components/common/aw/xradio/cmd
CFLAGS        += -I drivers/drv/bluetooth/include/lib
CFLAGS        += -I drivers/drv/bluetooth/include/driver
CFLAGS        += -I components/aw/bluetooth/zephyr/include/xrbtc

obj-y += $(XR_ETF_OBJS)
endif
