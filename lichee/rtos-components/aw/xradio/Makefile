ifeq ($(CONFIG_DRIVER_XR819), y)
CFLAGS += -DCONFIG_OS_NUTTX
else ifeq ($(CONFIG_DRIVER_XR829), y)
CFLAGS += -DCONFIG_OS_NUTTX
else ifeq ($(CONFIG_DRIVER_R128), y)
CFLAGS += -DCONFIG_OS_TINA
endif
CFLAGS += $(CFLAGS_COMMON)
obj-y += sdd/sdd.o    # should only on RV?
obj-y += sys/atomic.o
obj-y += os/os_mutex.o
obj-y += os/os_queue.o
obj-y += os/os_semaphore.o
obj-y += os/os_thread.o
obj-y += os/os_time.o
obj-y += os/os_timer.o
obj-y += os/os_error.o
obj-y += os/os_util.o
obj-y += cmd/cmd_util.o
XRADIO_AMP_OBJS += sysinfo/sysinfo.o
XRADIO_AMP_OBJS += sys_ctrl/container.o
XRADIO_AMP_OBJS += sys_ctrl/event_queue.o
XRADIO_AMP_OBJS += sys_ctrl/looper.o
XRADIO_AMP_OBJS += sys_ctrl/observer.o
XRADIO_AMP_OBJS += sys_ctrl/publisher.o
XRADIO_AMP_OBJS += sys_ctrl/sys_ctrl.o

#XRADIO_AMP_OBJS += cmd/cmd_util.o
ifeq ($(CONFIG_ARCH_SUN20IW2), y)
ifneq ($(CONFIG_ETF), y)
XRADIO_AMP_OBJS += rf/cmd_rf.o
endif
endif

##use search:
#_BASE_DIRS += components/common/aw/xradio/sdd    # should only on RV?
#_BASE_DIRS += components/common/aw/xradio/sys
#_BASE_DIRS += components/common/aw/xradio/os
#_BASE__DIRS := $(shell find $(_BASE_DIRS) -type d)
#_BASE_SRCS := $(sort $(basename $(foreach dir,$(_BASE__DIRS),$(wildcard $(dir)/*.[csS]))))
#_BASE_W_OBJS := $(addsuffix .o,$(_BASE_SRCS))
#obj-y += $(_BASE_W_OBJS)
#_AMP_DIRS += components/common/aw/xradio/sysinfo
#_AMP_DIRS += components/common/aw/xradio/sys_ctrl
#_AMP_DIRS += components/common/aw/xradio/cmd
#_AMP__DIRS := $(shell find $(_AMP_DIRS) -type d)
#_AMP_SRCS := $(sort $(basename $(foreach dir,$(_AMP__DIRS),$(wildcard $(dir)/*.[csS]))))
#_AMP_W_OBJS := $(addsuffix .o,$(_AMP_SRCS))
#XRADIO_AMP_OBJS += $(_AMP_W_OBJS)

obj-$(CONFIG_AW_COMPONENT_XBTC_UART) += xbtc_uart/

ifeq ($(CONFIG_COMPONENTS_AMP), y)
ifeq ($(CONFIG_ARCH_RISCV_RV64), y)
obj-y += $(XRADIO_AMP_OBJS)
endif
ifeq ($(CONFIG_ARCH_ARM_ARMV8M), y)
obj-y += $(XRADIO_AMP_SER_OBJS)
endif
else
obj-y += $(XRADIO_AMP_OBJS)
endif

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif

CFLAGS += -I components/common/aw/xradio/cmd
CFLAGS += -I components/common/aw/xradio/include

#ifeq ($(CONFIG_DRIVERS_XRADIO), y)
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -I drivers/drv/wireless/xradio/include
CFLAGS += -I drivers/rtos-hal/include/hal
#endif

CFLAGS += -I include
CFLAGS += -I os
CFLAGS += -I os/include
CFLAGS += -I include/freertos/

include components/common/aw/xradio/plt_def.mk
CFLAGS += $(PLT_CFLAGS)
