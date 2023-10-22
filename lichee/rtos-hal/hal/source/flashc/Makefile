obj-y += hal_spinor.o

obj-y += hal_flash.o
obj-y += hal_flashctrl.o
obj-y += hal_xip.o
obj-y += flashchip/flash_chip.o
obj-y += flashchip/flash_default.o
obj-y += flashchip/flash_EN25QHXXA.o
obj-y += flashchip/flash_XM25QHXXA.o
obj-y += flashchip/flash_P25QXXH.o
obj-y += flashchip/flash_XT25FXXB.o
obj-y += flashchip/flash_W25Q64BXX.o
obj-y += flashchip/flash_MX25LXXXF.o
obj-y += flashchip/flash_chip_cfg.o
obj-y += flashc_drv.o

obj-$(CONFIG_FLASHC_ENC) += hal_flashc_enc.o
obj-$(CONFIG_FLASHC_GET_XIP_PARTITION_FROM_ENV) += env/env.o

subdir-ccflags-y += -I drivers/rtos-hal/hal/source/spinor/
