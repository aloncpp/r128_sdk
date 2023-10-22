obj-$(CONFIG_PROJECT_R128S2_PRO_C906) += pro_c906/
obj-$(CONFIG_PROJECT_R128S2_PRO_M33) += pro_m33/

subdir-ccflags-y += -I components/common/aw/pm/include

ifeq ($(CONFIG_DRIVERS_XRADIO), y)
subdir-ccflags-y += -I components/common/aw/xradio/include/sdd
subdir-ccflags-y += -I drivers/rtos-hal/include/hal
endif
