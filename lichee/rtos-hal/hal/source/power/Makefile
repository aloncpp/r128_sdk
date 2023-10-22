subdir-ccflags-y += -I$(obj)/ -I drivers/include

obj-$(CONFIG_AXP2585_POWER) := axp_twi.o axp.o axp2585.o
obj-$(CONFIG_GPADC_POWER) := gpadc_power.o axp.o sun20iw2p1/gpadc_power_core.o

obj-$(if $(CONFIG_ARCH_SUN20IW2P1),$(CONFIG_AXP2585_POWER)) += sun20iw2p1/core.o
