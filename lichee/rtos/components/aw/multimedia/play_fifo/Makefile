obj-y += play_fifo.o
obj-y += player_app.o
obj-y += audiofifo.o
obj-y += kfifoqueue.o
obj-y += kfifo.o
#obj-y += components/aw/xradio/sys_ctrl/sys_ctrl.o
RTPLAYER_INCLUDE_DIR := components/aw/multimedia/include/
PLAY_FIFO_DIR := components/aw/multimedia/play_fifo/

CFLAGS += -I $(PLAY_FIFO_DIR)
CFLAGS += -I $(RTPLAYER_INCLUDE_DIR)
CFLAGS += -I $(RTPLAYER_INCLUDE_DIR)/include/cedarx/xplayer/include/
CFLAGS += -I $(RTPLAYER_INCLUDE_DIR)/libcore/base/include/
CFLAGS += -I $(RTPLAYER_INCLUDE_DIR)/libcore/playback/include/
CFLAGS += -I $(RTPLAYER_INCLUDE_DIR)/libcore/parser/include/
CFLAGS += -I $(RTPLAYER_INCLUDE_DIR)/libcore/stream/include/
CFLAGS += -I $(RTPLAYER_INCLUDE_DIR)/Cdx2.0Plugin/include/
CFLAGS += -I components/thirdparty/console/
CFLAGS += -I components/common/aw/AudioSystem/include/
#CFLAGS += -I components/aw/xradio/include/sys_ctrl/
#CFLAGS += -I components/aw/xradio/include/
#CFLAGS += -I components/thirdparty/elmfat/src/
#CFLAGS += -I $(PLAY_FIFO_DIR)/include/
CFLAGS += -D_POSIX_TIMERS


