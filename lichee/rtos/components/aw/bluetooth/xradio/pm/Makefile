
BT_PM_BASE := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

BT_PM_DIRS += $(shell find $(BT_PM_BASE) -type d)

BT_PM_SRCS = $(basename $(foreach dir,$(BT_PM_DIRS),$(wildcard $(dir)/*.[csS])))

BT_PM_OBJS += $(addsuffix .o,$(BT_PM_SRCS))

CFLAGS += -I drivers/drv/bluetooth/xradio/include/driver
CFLAGS += -I drivers/drv/bluetooth/xradio/include/lib

CFLAGS += -I components/common/aw/xradio/include
CFLAGS += -I components/aw/bluetooth/xradio/framework
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/src/host

CFLAGS += -I components/common/aw/pm/include
obj-y += $(subst $(BT_PM_BASE)/, , $(BT_PM_OBJS))
