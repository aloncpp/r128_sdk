
include $(BASE)/components/common/thirdparty/bluetooth/zephyr/src/makefile

ifeq ($(CONFIG_ARCH_SUN20IW2P1), y)
CONFIG_BT_VHCI=y
endif

ZEPHYR_OBJS += $(XRADIO_ZEPHYR_OBJS)

CFLAGS +=-DCONFIG_ARM -DZTEST_UNITTEST -D$(HCIOPTION)

#ifeq ($(HCIOPTION), H4)
CFLAGS +=-DCONFIG_UART_0_INTERRUPT_DRIVEN -DCONFIG_UART_INTERRUPT_DRIVEN -DCONFIG_BT_H4
#endif
ifeq ($(CONFIG_ARCH_SUN20IW2P1), y)
CFLAGS +=-DCONFIG_BT_VHCI
endif

CFLAGS += $(INCLUDE_PATHS)

ifeq ($(CONFIG_ARCH_SUN20IW2P1), y)
CFLAGS +=-DCONFIG_OS_TINA
CFLAGS += -I include/arch/arm/armv8m/
CFLAGS += -I include/arch/arm/mach

CFLAGS += -I components/common/aw/xradio/include
CFLAGS += -I components/aw/bluetooth/xradio/pm
else ifeq ($(CONFIG_ARCH_SUN8IW18P1), y)
CFLAGS += -I drivers/drv/wireless/xradio/os1/include
CFLAGS += -I drivers/drv/wireless/xradio
CFLAGS += -I include/drivers/
endif

#LDFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include/ble -I components/common/thirdparty/bluetooth/zephyr/include/

obj-y += $(ZEPHYR_OBJS)
