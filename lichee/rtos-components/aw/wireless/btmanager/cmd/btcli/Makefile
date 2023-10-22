#BLUEDROID_CMD_BASE:=$(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
#BLUEDROID_CMD_DIRS := $(BLUEDROID_CMD_BASE)
#BLUEDROID_CMD_DIRS += $(shell find $(BLUEDROID_CMD_BASE)/cmd_bt -type d)
#BLUEDROID_CMD_SRCS := $(basename $(foreach dir,$(BLUEDROID_CMD_DIRS),$(wildcard $(dir)/*.[csS])))
#BTCLI_CMD_OBJS += $(patsubst $(abspath $(BASE))/%, $(abspath $(BUILD_DIR))/%, $(addsuffix .o,$(BLUEDROID_CMD_SRCS)))

BTCLI_CMD_OBJS +=  btcli_cmd.o \
				   btcli.o \
				   btcli_a2dp_source.o \
				   btcli_a2dp_sink.o \
				   btcli_avrc.o \
				   btcli_sppc.o \
				   btcli_spps.o \

ifeq ($(CONFIG_BLEHOST), y)
BTCLI_CMD_OBJS +=  btcli_gatt.o \
                   btcli_ble.o
endif

ifeq ($(CONFIG_BT_HFP_CLIENT_ENABLE), y)
BTCLI_CMD_OBJS += btcli_hfp.o
endif
ifeq ($(CONFIG_BT_HFP_AG_ENABLE), y)
BTCLI_CMD_OBJS += btcli_ag.o
endif

obj-y += $(BTCLI_CMD_OBJS)

BLUEDROID_BASE_DIR :=components/common/thirdparty/bluetooth/bluedroid

CFLAGS += -I components/common/aw/xradio/cmd
CFLAGS += -I components/common/aw/wireless/btmanager/cmd/btcli
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
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include/ble
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/src/host
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include/ble/drivers/bluetooth

CFLAGS += -I components/common/aw/AudioSystem/include
