obj-y += syscalls.o
obj-y += time.o
obj-y += ctype.o

ifeq ($(CONFIG_ARCH_SUN8IW18P1), y)
obj-y += gmtime_r.o
obj-y += lcltime_r.o
obj-y += gettzinfo.o
obj-y += mktime.o
endif

ifeq ($(CONFIG_ARCH_ARM_CORTEX_M33), y)
obj-$(CONFIG_ARCH_MEMCPY_OPTIMIZE) += wrap_memcpy-armv7m-ldm.o
endif
ifeq ($(CONFIG_ARCH_RISCV), y)
obj-$(CONFIG_ARCH_MEMCPY_OPTIMIZE) += wrap_memcpy-riscv.o
endif

ifneq ($(CONFIG_KASAN), y)
obj-$(CONFIG_ARCH_MEMCPY_OPTIMIZE) += wrap_memmove.o
# cygwin enviroment work failed!!!
#obj-$(CONFIG_ARCH_MEMCPY_OPTIMIZE) += wrap_memset.o
endif

CFLAGS += -I components/common/aw/virt_log/
