
ADAPTER_BASE := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

ifeq ($(CONFIG_COMPONENTS_ZEPHYR), y)
ADAPTER_DIRS += $(shell find $(ADAPTER_BASE)/zephyr -type d)
endif

ifeq ($(CONFIG_COMPONENTS_BLUEDROID), y)
ADAPTER_DIRS += $(shell find $(ADAPTER_BASE)/bluedroid -type d)
endif

ADAPTER_SRCS = $(basename $(foreach dir,$(ADAPTER_DIRS),$(wildcard $(dir)/*.[csS])))

ADAPTER_OBJS += $(addsuffix .o,$(ADAPTER_SRCS))

CFLAGS += -I drivers/drv/bluetooth/include/lib
CFLAGS += -I drivers/drv/bluetooth/include/driver

CFLAGS += -I components/common/aw/xradio/include

ifeq ($(CONFIG_COMPONENTS_ZEPHYR), y)
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include/ble
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include/ble/drivers/bluetooth
endif

ifeq ($(CONFIG_COMPONENTS_BLUEDROID), y)
CFLAGS += -I components/common/thirdparty/bluetooth/bluedroid/common
CFLAGS += -I components/common/thirdparty/bluetooth/bluedroid/common/include
CFLAGS += -I components/common/thirdparty/bluetooth/bluedroid/common/osi/include
CFLAGS += -I components/common/thirdparty/bluetooth/bluedroid/host/bluedroid/common/include
CFLAGS += -I components/common/thirdparty/bluetooth/bluedroid/host/bluedroid/hci/include/hci
CFLAGS += -I components/common/thirdparty/bluetooth/bluedroid/host/bluedroid/stack/include
endif

CFLAGS += -I components/common/thirdparty/bluetooth/tools/hcidump/include

obj-y += $(subst $(ADAPTER_BASE)/, , $(ADAPTER_OBJS))
