obj-y += src/ff.o
obj-y += src/ffsystem.o
obj-y += src/ffunicode.o
obj-y += vfs/vfs_fat.o
obj-y += diskio/diskio.o

obj-$(CONFIG_DRIVERS_SDMMC) += vfs/aw_vfs_fat_sdmmc.o
obj-$(CONFIG_DRIVERS_SDMMC) += diskio/diskio_sdmmc.o
obj-$(CONFIG_DRIVERS_SDMMC) += diskio/sdmmc_part.o
obj-$(CONFIG_DRIVERS_SDMMC) += diskio/sdmmc_drv.o

obj-$(CONFIG_USB_STORAGE) += vfs/aw_vfs_fat_usb_msc.o
obj-$(CONFIG_USB_STORAGE) += diskio/diskio_usb_msc.o

CFLAGS += -I $(BASE)/include/
CFLAGS += -I $(BASE)/components/thirdparty/elmfat/src/
CFLAGS += -I $(BASE)/components/thirdparty/elmfat/diskio/
