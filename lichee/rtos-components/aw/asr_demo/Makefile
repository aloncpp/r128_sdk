
obj-y += src/asr_rws_client.o \
         src/asr_result_parser.o \
         src/player_mgr.o \
         src/asr_input.o \
         src/asr_resources.o \
         src/dsp_controller.o \
         src/ring.o \
         src/data_save.o \
         src/crc32.o \
         src/iot_controller.o \
         src/file_opt.o \
         src/asr_demo.o

# websocket
CFLAGS += -Icomponents/common/thirdparty/network/websocket/include/

# rtplayer
RTPLAYER_INCLUDE_DIR := components/aw/multimedia/include/
CFLAGS += -I $(RTPLAYER_INCLUDE_DIR)
CFLAGS += -I $(RTPLAYER_INCLUDE_DIR)/include/cedarx/xplayer/include/
CFLAGS += -I $(RTPLAYER_INCLUDE_DIR)/libcore/base/include/
CFLAGS += -I $(RTPLAYER_INCLUDE_DIR)/libcore/playback/include/
CFLAGS += -I $(RTPLAYER_INCLUDE_DIR)/libcore/parser/include/
CFLAGS += -I $(RTPLAYER_INCLUDE_DIR)/libcore/stream/include/
CFLAGS += -I $(RTPLAYER_INCLUDE_DIR)/Cdx2.0Plugin/include/

#audio system
CFLAGS += -Icomponents/common/aw/AudioSystem/include/

#rpdata
CFLAGS += -Icomponents/common/aw/rpdata/include/

#cjson

#wifi & net
ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif
CFLAGS += -I components/common/aw/wireless/wifimanager/core/include
CFLAGS += -I components/aw/smartlink/soft_ap_config/include
CFLAGS += -I drivers/drv/wireless/xradio/adapter/net_ctrl
CFLAGS += -I drivers/drv/wireless/xradio/include
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -I components/common/aw/xradio/include

#self
CFLAGS += -Icomponents/common/aw/asr_demo/inc/

CFLAGS += $(CC_FLAGS)
