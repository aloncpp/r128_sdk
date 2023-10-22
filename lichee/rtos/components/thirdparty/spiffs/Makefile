obj-$(CONFIG_COMPONENT_SPIFFS) += spiffs.o
obj-$(CONFIG_COMPONENT_SPIFFS) += spiffs_api.o
obj-$(CONFIG_COMPONENT_SPIFFS) += spiffs/src/spiffs_cache.o
obj-$(CONFIG_COMPONENT_SPIFFS) += spiffs/src/spiffs_check.o
obj-$(CONFIG_COMPONENT_SPIFFS) += spiffs/src/spiffs_gc.o
obj-$(CONFIG_COMPONENT_SPIFFS) += spiffs/src/spiffs_hydrogen.o
obj-$(CONFIG_COMPONENT_SPIFFS) += spiffs/src/spiffs_nucleus.o

CFLAGS += -Icomponents/thirdparty/spiffs/
$(eval $(call BuildPackage,spiffs))
