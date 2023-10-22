#common
include $(BT_DRV_BASE)/driver/common/Makefile

#hci
include $(BT_DRV_BASE)/driver/interface/Makefile

#controller
ifeq ($(CONFIG_XRADIO_BT_CONTROLLER), y)
BT_CONTROLLER_SELECT := xradio
else
BT_CONTROLLER_SELECT := xradio
endif

include $(BT_DRV_BASE)/driver/controller/$(BT_CONTROLLER_SELECT)/Makefile

CFLAGS += -I drivers/drv/bluetooth/driver/controller/$(XRADIO_CONTROLLER_SELECT)/hal/include
CFLAGS += -I drivers/drv/bluetooth/driver/controller/$(XRADIO_CONTROLLER_SELECT)/pal/include
