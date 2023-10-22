obj-$(CONFIG_COMPONENTS_LIBMETAL) += libmetal/
obj-$(CONFIG_COMPONENTS_OPENAMP)  += open-amp/ sunxi_helper/
obj-$(CONFIG_COMPONENTS_OPENAMP)  += rpmsg_demo/
obj-$(CONFIG_COMPONENTS_RPBUF)    += rpbuf_demo/
obj-y += trace_log/

INCLUDE += -I components/common/thirdparty/openamp/include

ifeq ($(CONFIG_KERNEL_FREERTOS), y)
CFLAGS += $(INCLUDE)
subdir-ccflags-y += -I components/common/thirdparty/openamp/include
endif

ifeq ($(CONFIG_OS_MELIS), y)
ccflags-y += $(INCLUDE)
endif
