BTSNOOP_BASE:=$(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

BTSNOOP_DIRS += $(shell find $(BTSNOOP_BASE) -type d)

BTSNOOP_SRCS := $(basename $(foreach dir,$(BTSNOOP_DIRS),$(wildcard $(dir)/*.[csS])))

BTSNOOP_OBJS += $(addsuffix .o,$(BTSNOOP_SRCS))

CFLAGS += -I $(BTSNOOP_BASE)
CFLAGS += -I $(BTSNOOP_BASE)/include

CFLAGS += -I components/common/aw/xradio/include

obj-y += $(subst $(BTSNOOP_BASE)/, ,$(BTSNOOP_OBJS))
