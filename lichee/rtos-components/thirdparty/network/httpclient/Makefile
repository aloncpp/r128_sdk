obj-y += HTTPCUsr_api.o
obj-y += HTTPMbedTLSWrapper.o
obj-y += API/HTTPClientAuth.o
obj-y += API/HTTPClient.o
obj-y += API/HTTPClientString.o
obj-y += API/HTTPClientWrapper.o

obj-y += Sample/HTTPClientSample.o
obj-y += Sample/cmd_httpclient_sample.o
obj-y += http_client.o

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

CFLAGS += -Iinclude/FreeRTOS_POSIX/

CFLAGS += -Icomponents/thirdparty/include
CFLAGS += -Icomponents/common/thirdparty/network/httpclient
CFLAGS += -Icomponents/common/thirdparty/network/$(MBEDTLS)/include/mbedtls
CFLAGS += -Icomponents/common/thirdparty/network/$(MBEDTLS)/include
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include/lwip
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -Icomponents/common/aw/xradio/include
