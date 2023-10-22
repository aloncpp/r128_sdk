obj-y += bt_test.o
obj-y += bt_os.o
obj-y += bt_adapter/rtl8723ds/bsp.o
obj-y += bt_adapter/rtl8723ds/bt_uart.o
obj-$(CONFIG_AW_COMPONENT_RTLBT) += libbtmanager.o libsbc.o

CFLAGS += -I include/drivers/
CFLAGS += -I include/freertos/

CFLAGS += -I components/aw/sound/aw-alsa-utils
CFLAGS += -I drivers/drv/rfkill
CFLAGS += -I components/aw/bluetooth/bt_adapter/rtl8723ds

CFLAGS += -I include/components/
CFLAGS += -I components/aw/bluetooth/rtlbt
CFLAGS += -I components/common/thirdparty/bluetooth/rtlbt/inc/stack
CFLAGS += -I components/common/thirdparty/bluetooth/rtlbt/inc
CFLAGS += -I components/common/thirdparty/bluetooth/rtlbt
CFLAGS += -I components/common/thirdparty/bluetooth/rtlbt/inc/platform

