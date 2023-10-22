WIRELESS_VIDEO_OBJS += main.o \
					wv_log.o \
					wv_network.o \
					wv_camera.o \
					wv_communication.o \

obj-y += $(WIRELESS_VIDEO_OBJS)

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
    LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
    LWIP = lwip-2.0.3
endif

CFLAGS += -I components/aw/wireless_video
CFLAGS += -I drivers/hal/include/hal/
CFLAGS += -I include/hal/sdmmc/osal
CFLAGS += -I include/hal/sdmmc/hal
CFLAGS += -I drivers/rtos-hal/hal/source/csi/jpeg/
CFLAGS += -I components/common/aw/xradio/os/include
CFLAGS += -I components/common/aw/xradio/include
CFLAGS += -I components/common/aw/wireless/wifimanager/demo
CFLAGS += -I components/common/aw/wireless/wifimanager/core/include
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/compat/posix

CFLAGS += -I include/freertos/
CFLAGS += -I include
