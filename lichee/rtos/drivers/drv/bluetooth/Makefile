ifeq ($(CONFIG_DRIVERS_BLUETOOTH_XRADIO), y)
BT_DRV_BASE := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

include $(BT_DRV_BASE)/driver/Makefile

ifeq ($(CONFIG_BT_DIST_MODE_CORRESPOND), y)
BT_DRV_AMP_DIRS += $(shell find $(BT_DRV_BASE)/hci_distribute -type d)
endif

BT_DRV_AMP_DIRS += $(shell find $(BT_DRV_BASE)/lib -type d)

BT_DRV_AMP_SRCS = $(basename $(foreach dir,$(BT_DRV_AMP_DIRS),$(wildcard $(dir)/*.[csS])))

BT_DRV_SER_SRCS = $(basename $(foreach dir,$(BT_DRV_SER_DIRS),$(wildcard $(dir)/*.[csS])))

BT_DRV_AMP_OBJS += $(addsuffix .o,$(BT_DRV_AMP_SRCS))

BT_DRV_SER_OBJS += $(addsuffix .o,$(BT_DRV_SER_SRCS))

ifeq ($(CONFIG_COMPONENTS_AMP_BT), y)
ifeq ($(CONFIG_ARCH_RISCV_RV64), y)
BT_DRV_OBJS += $(BT_DRV_AMP_OBJS)
endif
ifeq ($(CONFIG_ARCH_ARM_ARMV8M), y)
BT_DRV_OBJS += $(BT_DRV_SER_OBJS)
endif
else
ifeq ($(CONFIG_ARCH_ARM_ARMV8M), y)
BT_DRV_OBJS += $(BT_DRV_AMP_OBJS)
BT_DRV_OBJS += $(BT_DRV_SER_OBJS)
endif
endif

CFLAGS += -I drivers/drv/bluetooth/driver
CFLAGS += -I drivers/drv/bluetooth/driver/common

CFLAGS += -I drivers/drv/bluetooth/include
CFLAGS += -I drivers/drv/bluetooth/include/lib
CFLAGS += -I drivers/drv/bluetooth/include/driver
CFLAGS += -I drivers/drv/driver/controller/$(XRADIO_CONTROLLER_SELECT)/include
CFLAGS += -I components/common/thirdparty/bluetooth/tools/hcidump/include/hcidump_xr
CFLAGS += -I components/aw/bluetooth/pm
CFLAGS += -I drivers/drv/bluetooth/include/hci_distribute

CFLAGS += -I components/common/aw/xradio/os
CFLAGS += -I components/common/aw/xradio/include

CFLAGS += -I drivers/rtos-hal/hal/source/prcm/
CFLAGS += -I drivers/rtos-hal/hal/source/ccmu/
CFLAGS += -I drivers/rtos-hal/hal/source/ccmu/sunxi-ng/
CFLAGS += -I include/arch/arm/mach/sun20iw2p1/

obj-y += $(subst $(BT_DRV_BASE)/, ,$(BT_DRV_OBJS))
endif