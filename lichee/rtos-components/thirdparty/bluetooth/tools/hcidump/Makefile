HCIDUMP_BASE:=$(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

HCIDUMP_DIRS += $(shell find $(HCIDUMP_BASE) -type d)

HCIDUMP_SRCS := $(basename $(foreach dir,$(HCIDUMP_DIRS),$(wildcard $(dir)/*.[csS])))

HCIDUMP_OBJS += $(addsuffix .o,$(HCIDUMP_SRCS))

CFLAGS += -I $(HCIDUMP_BASE)
CFLAGS += -I $(HCIDUMP_BASE)/include/hcidump_xr
CFLAGS += -I $(HCIDUMP_BASE)/include
CFLAGS += -I $(HCIDUMP_BASE)/parser

CFLAGS += -I $(BASE)/components/common/aw/xradio/include
CFLAGS += -I $(BASE)/drivers/drv/wireless/xradio/include

obj-y += $(subst $(HCIDUMP_BASE)/, ,$(HCIDUMP_OBJS))
