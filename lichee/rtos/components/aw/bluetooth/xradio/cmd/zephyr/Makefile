
ZEPHYR_CMD_BASE := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

ZEPHYR_CMD_DIRS += $(shell find $(ZEPHYR_CMD_BASE) -type d)
#$(info $(foreach dir,$(ZEPHYR_CMD_DIRS),$(wildcard $(dir)/*.[csS])))

ZEPHYR_CMD_SRCS = $(basename $(foreach dir,$(ZEPHYR_CMD_DIRS),$(wildcard $(dir)/*.[csS])))
#$(info "!!!!!!!!!!!!!!!!!!!" $(ZEPHYR_CMD_SRCS))

ZEPHYR_CMD_OBJS += $(addsuffix .o,$(ZEPHYR_CMD_SRCS))
#$(info "zephyr objects------>")

CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/src
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include/ble

CFLAGS += -I drivers/drv/bluetooth/include/driver

CFLAGS += -I components/aw/bluetooth/xradio/framework
CFLAGS += -I components/aw/bluetooth/xradio/cmd/zephyr

CFLAGS += -I components/common/aw/xradio/cmd
CFLAGS += -I components/common/aw/xradio/include

obj-y += $(subst $(ZEPHYR_CMD_BASE)/, , $(ZEPHYR_CMD_OBJS))
