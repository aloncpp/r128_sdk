obj-$(CONFIG_COMPONENTS_AACTD_LIB) += common.o communicate.o

AACED_LIB_INCLUDE_DIR := components/common/aw/aactd-lib/include/

CFLAGS += -I $(AACED_LIB_INCLUDE_DIR)

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/compat/posix/
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/posix/
endif

CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/
CFLAGS += -I components/common/aw/xradio/include/
