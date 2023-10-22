BLUEDROID_BASE_DIR := components/common/thirdparty/bluetooth/bluedroid

#BLUEDROID_CMD_BASE:=$(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
#BLUEDROID_CMD_DIRS := $(BLUEDROID_CMD_BASE)
#BLUEDROID_CMD_DIRS += $(shell find $(BLUEDROID_CMD_BASE)/cmd_bt -type d)
#BLUEDROID_CMD_SRCS := $(basename $(foreach dir,$(BLUEDROID_CMD_DIRS),$(wildcard $(dir)/*.[csS])))
#BTMG_CORE_OBJS += $(patsubst $(abspath $(BASE))/%, $(abspath $(BUILD_DIR))/%, $(addsuffix .o,$(BLUEDROID_CMD_SRCS)))

BTMG_CORE_OBJS += btmg_a2dp_sink.o \
				bt_manager.o \
				btmg_gap.o \
				btmg_log.o \
				btmg_spp_client.o \
				btmg_common.o \
				btmg_a2dp_source.o \
				btmg_spp_server.o \
				ringbuff.o \
				btmg_audio.o \
				btmg_avrc.o \
				btmg_dev_list.o \
				btmg_weak.o \

ifeq ($(CONFIG_BLEHOST), y)
BTMG_CORE_OBJS += btmg_gatt_db.o \
				  btmg_gatt.o \
				  btmg_ble.o
endif

ifeq ($(CONFIG_BT_HFP_CLIENT_ENABLE), y)
BTMG_CORE_OBJS += btmg_hfp_hf.o
endif
ifeq ($(CONFIG_BT_HFP_AG_ENABLE), y)
BTMG_CORE_OBJS += btmg_hfp_ag.o
endif

obj-y += $(BTMG_CORE_OBJS)

CFLAGS += -I components/common/aw/xradio/cmd
CFLAGS += -I components/common/aw/wireless/btmanager/src
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

CFLAGS += -I include/freertos/
CFLAGS += -I include
CFLAGS += -I components/common/aw/wireless/btmanager/src

CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include/ble
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/src/host
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include/ble/drivers/bluetooth

CFLAGS +=  -I components/common/thirdparty/speexdsp/
CFLAGS += -I components/common/aw/AudioSystem/include
