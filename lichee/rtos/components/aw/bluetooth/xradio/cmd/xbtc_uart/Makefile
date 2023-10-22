ifeq ($(CONFIG_AW_COMPONENT_XBTC_UART), y)

obj-y  += cmd_xbtc_uart.o

CFLAGS        += $(PLT_CFLAGS)
CFLAGS        += -I include/drivers/
CFLAGS        += -I include/freertos/

CFLAGS        += -I drivers/drv/wireless/xradio/os/include
CFLAGS        += -I components/common/aw/xradio/cmd/
CFLAGS        += -I components/common/aw/xradio/xbtc_uart/
CFLAGS        += -I drivers/drv/wireless/xradio/
CFLAGS        += -I drivers/drv/bluetooth/xradio/include/driver
CFLAGS        += -I ../rtos-components/aw/xradio/include
endif
