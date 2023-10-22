subdir-ccflags-y += -I drivers/rtos-hal/hal/source/usb/core
subdir-ccflags-y += -I drivers/rtos-hal/hal/source/usb/common
subdir-ccflags-y += -I drivers/rtos-hal/hal/source/usb/include
subdir-ccflags-y += -I drivers/rtos-hal/hal/source/usb/uvc/Include
subdir-ccflags-y += -I drivers/rtos-hal/test/usb

obj-y += usb_test.o
obj-y += usb_test_phy.o

obj-$(CONFIG_HAL_TEST_UDC) += usb_test_udc.o
obj-$(CONFIG_HAL_TEST_HCI) += usb_test_hci.o
obj-$(CONFIG_HAL_TEST_UVC) += usb_test_uvc.o

CFLAGS += -Werror