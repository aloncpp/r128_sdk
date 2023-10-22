obj-y += loopif_test.o
obj-y += socket_test/loopif_socket_test.o

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/compat/posix/
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/posix/
endif

CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/
CFLAGS += -I include/drivers/
CFLAGS += -I include/freertos/
CFLAGS += -I components/common/aw/xradio/include/
CFLAGS += -I components/common/aw/network/tcpip/
