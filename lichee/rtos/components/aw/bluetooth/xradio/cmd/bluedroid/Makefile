
BLUEDROID_CMD_BASE:=$(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

BLUEDROID_CMD_DIRS := $(BLUEDROID_CMD_BASE)

BLUEDROID_CMD_DIRS += $(shell find $(BLUEDROID_CMD_BASE)/cmd_bt -type d)

ifneq ($(CONFIG_BT_DUAL_HOST), y)
BLUEDROID_CMD_DIRS += $(shell find $(BLUEDROID_CMD_BASE)/cmd_mesh -type d)
endif

BLUEDROID_CMD_SRCS := $(basename $(foreach dir,$(BLUEDROID_CMD_DIRS),$(wildcard $(dir)/*.[csS])))

BLUEDROID_CMD_OBJS += $(addsuffix .o,$(BLUEDROID_CMD_SRCS))

ifeq ($(CONFIG_BT_DUAL_HOST), y)
FIRTER_OBJS := $(BLUEDROID_CMD_BASE)/cmd_bt/cmd_gattc.o $(BLUEDROID_CMD_BASE)/cmd_bt/cmd_gatts.o $(BLUEDROID_CMD_BASE)/cmd_bt/cmd_ble.o
BLUEDROID_CMD_OBJS := $(filter-out $(FIRTER_OBJS),$(BLUEDROID_CMD_OBJS))
endif

ifeq ($(CONFIG_COMPONENTS_AW_TINY_ALSA_LIB), y)
FIRTER_OBJS := $(BLUEDROID_CMD_BASE)/bt_audio_fake.o
BLUEDROID_CMD_OBJS := $(filter-out $(FIRTER_OBJS),$(BLUEDROID_CMD_OBJS))
endif

BLUEDROID_BASE_DIR :=components/common/thirdparty/bluetooth/bluedroid

CFLAGS += -I components/common/aw/xradio/cmd
CFLAGS += -I components/aw/bluetooth/xradio/cmd/bluedroid
CFLAGS += -I components/aw/bluetooth/xradio/cmd/bluedroid/cmd_bt
CFLAGS += -I drivers/drv/wireless/xradio/adapter
CFLAGS += -I components/aw/bluetooth/xradio/framework

CFLAGS += -I components/common/aw/xradio
CFLAGS += -I components/common/aw/xradio/include
CFLAGS += -I components/common/aw/xradio/os
CFLAGS += -I components/common/aw/xradio/os/include

CFLAGS += -I $(BLUEDROID_BASE_DIR)/common/include
CFLAGS += -I $(BLUEDROID_BASE_DIR)/common/osi/include
CFLAGS += -I $(BLUEDROID_BASE_DIR)/host/bluedroid/hci/include
CFLAGS += -I $(BLUEDROID_BASE_DIR)/host/bluedroid/hci/include/hci
CFLAGS += -I $(BLUEDROID_BASE_DIR)/host/bluedroid/common/include/common
CFLAGS += -I $(BLUEDROID_BASE_DIR)/host/bluedroid/stack/include
CFLAGS += -I $(BLUEDROID_BASE_DIR)/host/bluedroid/common/include
CFLAGS += -I components/common/thirdparty/bluetooth/tools/hcidump/include

CFLAGS += -I $(BLUEDROID_BASE_DIR)/host/bluedroid/api/include/api
ifeq ($(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM), y)
CFLAGS += -I components/common/aw/AudioSystem/include
endif

ifneq ($(CONFIG_BT_DUAL_HOST), y)
# ifeq ($(CONFIG_BLE_MESH), y)
CFLAGS += -I $(BLUEDROID_BASE_DIR)/xr_ble_mesh/api/core/include
CFLAGS += -I $(BLUEDROID_BASE_DIR)/xr_ble_mesh/api/models/include
CFLAGS += -I $(BLUEDROID_BASE_DIR)/xr_ble_mesh/api
# endif
endif

CFLAGS += -I include/freertos/

#include components/common/aw/xradio/plt_def.mk
#CFLAGS += $(PLT_CFLAGS)

obj-y += $(subst $(BLUEDROID_CMD_BASE)/, , $(BLUEDROID_CMD_OBJS))
