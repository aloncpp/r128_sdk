obj-y += aactd.o local.o drc_hw.o

ifeq ($(CONFIG_COMPONENTS_PROCESS_EQ),y)
ifneq ($(CONFIG_ARCH_SUN8IW18)$(CONFIG_ARCH_SUN8IW19)$(CONFIG_ARCH_SUN8IW20),)
obj-y += eq_sw.o
endif
endif

ifeq ($(CONFIG_ARCH_SUN20IW2),y)
obj-$(CONFIG_COMPONENTS_AACTD_HW_SET) += eq_hw.o
endif

CFLAGS += -I components/common/aw/aactd-lib/include/

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/compat/posix/
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/posix/
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/
endif

CFLAGS += -I components/common/aw/aactd/daemon/
CFLAGS += -I components/common/aw/xradio/include/
CFLAGS += -I drivers/rtos-hal/hal/source/sound/common
CFLAGS += -I components/common/aw/AudioSystem/include/
