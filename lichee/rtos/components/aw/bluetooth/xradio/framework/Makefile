
FRAMEWORK_BASE := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

FRAMEWORK_DIRS += $(shell find $(FRAMEWORK_BASE) -type d)

FRAMEWORK_SRCS = $(basename $(foreach dir,$(FRAMEWORK_DIRS),$(wildcard $(dir)/*.[csS])))

FRAMEWORK_OBJS += $(addsuffix .o,$(FRAMEWORK_SRCS))

CFLAGS += -I drivers/drv/bluetooth/include/driver
CFLAGS += -I drivers/drv/bluetooth/include/lib

CFLAGS += -I components/common/aw/xradio/include
CFLAGS += -I components/aw/bluetooth/xradio/framework
CFLAGS += -I components/aw/bluetooth/xradio/pm
CFLAGS += -I components/common/aw/pm/include

CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include/ble/drivers/bluetooth
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include/ble/
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include/

obj-y += $(subst $(FRAMEWORK_BASE)/, , $(FRAMEWORK_OBJS))
