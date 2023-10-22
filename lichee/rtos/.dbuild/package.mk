include $(BASE)/.config

define BuildPackage

$(1)_SUBPATH := $$(shell echo $(MAKEFILE_LIST) | awk '{print $$$$(NF-2)}')
$(1)_SUBPATH := $$(subst $(BASE)/,,$$(shell dirname $$($(1)_SUBPATH)))
$(1)_MNAME := $$(shell echo $$($(1)_SUBPATH) | tr '[a-z]' '[A-Z]' | sed 's/[^A-Z]/_/g')
$(1)_CFLAGS := $$(addprefix -I ,$$(obj-header)) $$(obj-cflags)
$(1)_SRCPATH := $$(BUILD_DIR)/$$($(1)_SUBPATH)/

$$($(1)_MNAME) := $$(addprefix $$($(1)_SRCPATH),$$(obj-y))

$$($$($(1)_MNAME)):MODULE_NAME := $$(shell echo $$($(1)_SUBPATH) | tr '/' '-')
$$($$($(1)_MNAME)):CFLAGS += $$($1_CFLAGS)
$$($$($(1)_MNAME)):CFLAGS += -I $(BASE)/drivers/rtos-hal/include/hal/
$$($$($(1)_MNAME)):CFLAGS += -I $(BASE)/drivers/rtos-hal/include/osal/
$$($$($(1)_MNAME)):CFLAGS += -I $(BASE)/include/
$$($$($(1)_MNAME)):CFLAGS += -I $(BASE)/include/drivers
$$($$($(1)_MNAME)):CFLAGS += -I $(BASE)/kernel/Posix/include/
$$($$($(1)_MNAME)):CFLAGS += -I $(BASE)/include/FreeRTOS_POSIX/

OBJECTS += $$($$($(1)_MNAME))
obj-y :=
obj-header :=
obj-cflags :=
#CXX config
$(1)_CXXFLAGS := $$(addprefix -I ,$$(obj-cxxheader))$$(obj-cxxflags)
$$($$($(1)_MNAME)):CXXFLAGS += $$($1_CXXFLAGS)
obj-cxxheader :=
obj-cxxflags :=

endef
