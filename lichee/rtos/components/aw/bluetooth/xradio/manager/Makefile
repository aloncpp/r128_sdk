
BT_MANAGER_BASE := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

BT_MANAGER_DIRS += $(shell find $(BT_MANAGER_BASE) -type d)

BT_MANAGER_SRCS = $(basename $(foreach dir,$(BT_MANAGER_DIRS),$(wildcard $(dir)/*.[csS])))

BT_MANAGER_OBJS += $(addsuffix .o,$(BT_MANAGER_SRCS))

CFLAGS += -I components/aw/bluetooth/xradio/manager/include

obj-y += $(subst $(BT_MANAGER_BASE)/, , $(BT_MANAGER_OBJS))
