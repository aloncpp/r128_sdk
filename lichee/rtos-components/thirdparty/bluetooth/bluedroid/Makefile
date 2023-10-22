
BLUEDROID_BASE:=$(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

#BLUEDROID_DIRS_ALL := $(shell find $(BLUEDROID_BASE) -type d)
#BLUEDROID_DIRS_IGNORE := $(shell find $(BLUEDROID_BASE)/xr_ble_mesh -type d)
#BLUEDROID_DIRS := $(filter-out $(BLUEDROID_DIRS_IGNORE),$(BLUEDROID_DIRS_ALL))

BLUEDROID_DIRS := $(BLUEDROID_BASE)
BLUEDROID_DIRS += $(shell find $(BLUEDROID_BASE)/common $(BLUEDROID_BASE)/host -type d)
ifeq ($(CONFIG_BLE_MESH), y)
BLUEDROID_DIRS += $(shell find $(BLUEDROID_BASE)/xr_ble_mesh -type d)
endif

BLUEDROID_SRCS := $(basename $(foreach dir,$(BLUEDROID_DIRS),$(wildcard $(dir)/*.[csS])))

BLUEDROID_OBJS += $(addsuffix .o,$(BLUEDROID_SRCS))

#CFLAGS += $(addprefix -I,$(BLUEDROID_DIRS))

CFLAGS += -I components/common/aw/xradio/include
CFLAGS += -I components/common/thirdparty/bluetooth/tools/hcidump/include

ifeq ($(CONFIG_BLUEDROID_USE_SETTINGS), y)
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include/ble
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include
endif

CFLAGS += -I $(BLUEDROID_BASE)/common
CFLAGS += -I $(BLUEDROID_BASE)/common/osi/include
CFLAGS += -I $(BLUEDROID_BASE)/common/btc/include
CFLAGS += -I $(BLUEDROID_BASE)/common/include
CFLAGS += -I $(BLUEDROID_BASE)/include/xradio
CFLAGS += -I components/aw/bluetooth/xradio/pm

CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/bta/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/bta/ar/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/bta/av/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/bta/dm/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/bta/gatt/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/bta/hf_ag/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/bta/hf_client/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/bta/hh/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/bta/jv/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/bta/sdp/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/bta/sys/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/device/include

CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/hci/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/hci/include/hci

CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/external/sbc/decoder/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/external/sbc/encoder/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/external/sbc/plc/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/btc/profile/std/a2dp/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/btc/profile/std/hid/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/btc/profile/std/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/btc/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/stack/btm/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/stack/gap/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/stack/gatt/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/stack/l2cap/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/stack/sdp/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/stack/smp/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/stack/avct/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/stack/avrc/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/stack/avdt/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/stack/a2dp/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/stack/rfcomm/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/stack/include
CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/common/include

CFLAGS += -I $(BLUEDROID_BASE)/host/bluedroid/api/include/api

ifeq ($(CONFIG_BLE_MESH), y)
CFLAGS += -I $(BLUEDROID_BASE)/xr_ble_mesh/mesh_common/include
CFLAGS += -I $(BLUEDROID_BASE)/xr_ble_mesh/mesh_common/tinycrypt/include

CFLAGS += -I $(BLUEDROID_BASE)/xr_ble_mesh/mesh_core
CFLAGS += -I $(BLUEDROID_BASE)/xr_ble_mesh/mesh_core/include
CFLAGS += -I $(BLUEDROID_BASE)/xr_ble_mesh/mesh_core/storage

CFLAGS += -I $(BLUEDROID_BASE)/xr_ble_mesh/btc/include

CFLAGS += -I $(BLUEDROID_BASE)/xr_ble_mesh/mesh_models/common/include
CFLAGS += -I $(BLUEDROID_BASE)/xr_ble_mesh/mesh_models/client/include
CFLAGS += -I $(BLUEDROID_BASE)/xr_ble_mesh/mesh_models/server/include

CFLAGS += -I $(BLUEDROID_BASE)/xr_ble_mesh/api/core/include
CFLAGS += -I $(BLUEDROID_BASE)/xr_ble_mesh/api/models/include
CFLAGS += -I $(BLUEDROID_BASE)/xr_ble_mesh/api
endif

obj-y += $(subst $(BLUEDROID_BASE), ,$(BLUEDROID_OBJS))
