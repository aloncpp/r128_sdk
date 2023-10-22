obj-y += MQTTPacket/MQTTConnectClient.o \
			  MQTTPacket/MQTTConnectServer.o \
			  MQTTPacket/MQTTDeserializePublish.o \
			  MQTTPacket/MQTTSerializePublish.o \
			  MQTTPacket/MQTTSubscribeClient.o \
			  MQTTPacket/MQTTSubscribeServer.o \
			  MQTTPacket/MQTTUnsubscribeClient.o \
			  MQTTPacket/MQTTUnsubscribeServer.o \
			  MQTTPacket/MQTTFormat.o \
			  MQTTPacket/MQTTPacket.o \
			  MQTTClient-C/MQTTClient.o \
			  MQTTClient-C/Xr_RTOS/MQTTXrRTOS.o

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

ifeq ($(CONFIG_COMPONENTS_MBEDTLS_2_16_0),y)
MBEDTLS = mbedtls-2.16.0
else ifeq ($(CONFIG_COMPONENTS_MBEDTLS_2_7_0),y)
MBEDTLS = mbedtls-2.7.0
endif

CFLAGS += -Icomponents/common/thirdparty/network/mqtt/include/MQTTPacket
CFLAGS += -Icomponents/common/thirdparty/network/mqtt/include/MQTTClient-C
CFLAGS += -Icomponents/common/thirdparty/network/mqtt/MQTTPacket
CFLAGS += -Icomponents/common/thirdparty/network/mqtt/MQTTClient-C
CFLAGS += -Icomponents/common/thirdparty/network/mqtt/MQTTClient-C/Xr_RTOS
CFLAGS += -Icomponents/common/thirdparty/network/$(MBEDTLS)/include/mbedtls
CFLAGS += -Icomponents/common/thirdparty/network/$(MBEDTLS)/include
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include/lwip
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -Idrivers/drv/wireless/xradio/os
CFLAGS += -Idrivers/drv/wireless/xradio/os/include

include components/common/aw/xradio/plt_def.mk
CFLAGS += $(PLT_CFLAGS)
