BT_DRV_AMP_DIRS += $(shell find $(BT_DRV_BASE)/driver/controller/$(XRADIO_CONTROLLER_SELECT) -type d)

ifeq ($(CONFIG_ARCH_ARM_ARMV8M), y)
obj-y += driver/controller/$(BT_CONTROLLER_MODE_SELECT)/libxrbtc.o
endif

CFLAGS += -I $(BT_DRV_BASE)/driver/controller/$(XRADIO_CONTROLLER_SELECT)
