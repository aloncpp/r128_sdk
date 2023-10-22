ifeq ($(CONFIG_TESTCASE_G2D), y)
ttg2d_dir=$(RTOS_TOP)/lichee/rtos-components/aw/tinatest/testcase/g2d/res
UDISK_PATH = $(RTOS_TOP)/board/$(TARGET_PLATFORM)/data/UDISK

$(shell mkdir -p $(UDISK_PATH)/tt_g2d/ || exit 1)
$(shell cp $(ttg2d_dir)/*.bin $(UDISK_PATH)/tt_g2d/ || exit 1)
endif

obj-$(CONFIG_TESTCASE_G2D) += tt-g2d.o

CFLAGS += -I drivers/rtos-hal/hal/source/g2d_rcq/
