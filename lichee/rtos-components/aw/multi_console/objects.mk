include $(DBUILD_ROOT).dbuild/package.mk

obj-$(CONFIG_MUILTI_CONSOLE) += console.o
obj-$(CONFIG_MUILTI_CONSOLE) += porting.o
obj-$(CONFIG_MUILTI_CONSOLE) += uart_console.o
obj-$(CONFIG_MUILTI_CONSOLE) += doexit.o

$(eval $(call BuildPackage,multi_console))
