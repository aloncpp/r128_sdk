obj-$(CONFIG_COMPONENTS_RINGBUFFER) += ringbuffer.o

INCLUDE = -I components/common/aw/ringbuffer/include

ifeq ($(CONFIG_KERNEL_FREERTOS), y)
CFLAGS += $(INCLUDE)
endif

ifeq ($(CONFIG_OS_MELIS), y)
ccflags-y += $(INCLUDE)
endif
