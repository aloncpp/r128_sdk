#makefile for standby.bin

CROSS_RTOSDIR ?= $(shell cd ../../../../../rtos && pwd)
CROSS_COMPILE ?= $(CROSS_RTOSDIR)/tools/gcc-arm-none-eabi-8-2019-q3-update/bin/arm-none-eabi-
CROSS_CONFIG  := $(CROSS_RTOSDIR)/.config
STANDBY_LDFILE := standby.lds
STANDBY_TARGET := standby.bin

RTOS_PROJECT_NAME ?= r128_evb1_m33
RTOS_AUTOCONF_HEAD_FILE ?= $(CROSS_RTOSDIR)/include/generated/$(RTOS_PROJECT_NAME)/autoconf.h

include $(CROSS_RTOSDIR)/.config

INCFLAGS += -I$(CROSS_RTOSDIR)/include/
INCFLAGS += -I$(CROSS_RTOSDIR)/include/drivers/
INCFLAGS += -I$(CROSS_RTOSDIR)/include/arch/arm/mach/sun20iw2p1/
INCFLAGS += -I$(CROSS_RTOSDIR)/arch/arm/armv8m/include/cmsis
INCFLAGS += -I$(CROSS_RTOSDIR)/components/common/aw/pm/
INCFLAGS += -I$(CROSS_RTOSDIR)/components/common/aw/pm/common/
INCFLAGS += -I$(CROSS_RTOSDIR)/components/common/aw/pm/include/
INCFLAGS += -I$(CROSS_RTOSDIR)/components/common/aw/pm/plat_m33/
INCFLAGS += -I$(CROSS_RTOSDIR)/components/common/aw/pm/plat_m33/include
INCFLAGS += -Iinclude -Iarch/include -Idriver/include -Iclk/include -Ipower/include
INCFLAGS += -include $(RTOS_AUTOCONF_HEAD_FILE)

LIBFLAGS += -Ldriver/hpsram -lhpsram -Ldriver/lpsram -llpsram

SRCFLAGS := -g -c -nostdlib -march=armv8-m.main -mfpu=fpv5-sp-d16 \
		-fno-unwind-tables -fno-jump-tables \
		-fno-asynchronous-unwind-tables -mlittle-endian \
		-O2 -mno-unaligned-access


GIT_COMMIT_ID := $(strip $(shell git rev-parse --verify HEAD 2>/dev/null))
GIT_DIRTY     := $(strip $(shell git diff --quiet -- $(CROSS_RTOSDIR)/components/common/aw/pm || echo "-dirty"))
GIT_COMMIT_ID_DIRTY_FLAG := "$(GIT_COMMIT_ID)$(GIT_DIRTY)"
GIT_CHANGE_ID := "$(strip $(shell git log -1 HEAD | grep "^ \{4,5\}Change-Id:" | sed 's/Change-Id://g' | tail -1))"
BUILD_DATE   := "$(strip $(shell date "+%Y-%m-%d %H:%M:%S"))"
BUILD_AUTHOR := "$(strip $(USER))"

HEADCFLAGS := -DEXT_HEAD_COMMIT_ID=\"$(GIT_COMMIT_ID_DIRTY_FLAG)\" \
        -DEXT_HEAD_CHANGE_ID=\"$(GIT_CHANGE_ID)\" \
	-DEXT_HEAD_BUILD_DATE=\"$(BUILD_DATE)\" \
	-DEXT_HEAD_BUILD_AUTHOR=\"$(BUILD_AUTHOR)\"

CFLAGS  += $(SRCFLAGS)
CFLAGS  += $(INCFLAGS)
CFLAGS  += $(HEADCFLAGS)
CFLAGS  += -MMD -MP
LDFLAGS := -nostdlib -static

standby-c := $(wildcard *.c lib/*.c driver/*.c clk/*.c power/*.c arch/*.c)
standby-s := $(filter-out link.S, $(wildcard *.S arch/*.S))

standby-y :=
standby-y += $(patsubst %.c,%.o,$(standby-c))
standby-y += $(patsubst %.S,%.o,$(standby-s))

standby-dep := $(standby-y:.o=.d)



all: $(STANDBY_TARGET)

-include $(standby-dep)

%.o:%.c
	$(CROSS_COMPILE)gcc $(CFLAGS) $< $(LIBFLAGS) -o $@

%.o:%.S
	$(CROSS_COMPILE)gcc $(CFLAGS) $< $(LIBFLAGS) -o $@

$(STANDBY_LDFILE): link.S
	$(CROSS_COMPILE)gcc -E -P $(CFLAGS) \
		link.S > $(STANDBY_LDFILE)

standby.elf: $(standby-y) $(STANDBY_LDFILE)
	$(CROSS_COMPILE)ld -T $(STANDBY_LDFILE)\
		$(LDFLAGS) $(standby-y) -o $@ $(LIBFLAGS) -Map standby.map
	$(CROSS_COMPILE)objdump -D standby.elf > standby.lst

standby.bin: standby.elf
	$(CROSS_COMPILE)objcopy -O binary standby.elf standby.bin

clean:
	-rm -f $(standby-y) $(standby-dep) *.d *.lds *.lst *.map

distclean:
	-rm -f $(standby-y) $(standby-dep) standby.* *.d *.lds *.lst *.map

