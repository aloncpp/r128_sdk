obj-$(CONFIG_DRIVERS_RTL8723DS) += rtl8723ds/

ifeq ($(CONFIG_DRIVERS_XRADIO), y)
obj-y += xradio/
else ifeq ($(CONFIG_AW_COMPONENT_BLUEDROID), y)
obj-y += xradio/
else ifeq ($(CONFIG_AW_COMPONENT_BLUEDROID_CTRL), y)
obj-y += xradio/
endif

obj-y += sunxi-platform/
