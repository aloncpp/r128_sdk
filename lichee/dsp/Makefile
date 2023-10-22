SHELL = bash
ifeq ($(V),1)
  Q =
else
  Q = @
endif

srctree   := $(shell pwd)

#include file
-include .config
include ./scripts/build/Build.include

# Init all relevant variables used in build files
INCS      :=
DFLAGS    :=
CFLAGS    :=
LDFLAGS   :=
LSP       :=
prepare   :=
CONFIG_LSP :=
CONFIG_HEAD_DIR :=
# Toolchain
CC          = xt-xcc
CXX         = xt-xcc
AS          = xt-xcc
AR          = xt-ar
LD          = xt-ld
OBJC        = xt-objcopy
OBJS        = xt-strip
#XT_CORE     = $(patsubst %-params,%,$(notdir $(shell xt-xcc --show-config=core)))
CONFIGDIR   = $(shell xt-xcc --show-config=config)

# For platform-specific commands
include $(CONFIGDIR)/misc/hostenv.mk

# Target-specific options
ifeq ($(CONFIG_SIM_PLATFORM),y)
DFLAGS      += -DXT_SIMULATOR
else
DFLAGS      += -DXT_BOARD
endif

# Select LSP
LSP         = $(srctree)/arch/$(CONFIG_ARCH_PLATFORM)/lsp/$(CONFIG_CORE_ID)/$(CONFIG_LSP)

# Build options. Override CFLAGS from the make command line if desired.
# NOTE: Default LDFLAGS are set up to discard unused sections so that
# the final exe size is minimized.
INCS       += -Iarch/common
INCS       += -I$(CONFIG_HEAD_DIR)
INCS       += -Iinclude -Iinclude/hal -Iinclude/hal/sound -Iinclude/osal
INCS       += -Idrivers/rtos-hal/hal/source
INCS       += -Icomponents/thirdparty/freertos
INCS       += -Ikernel/$(CONFIG_KERNEL_VERSION_DIR)/include -Ikernel/$(CONFIG_KERNEL_VERSION_DIR)/include/private
INCS       += -Ikernel/$(CONFIG_KERNEL_VERSION_DIR)/FreeRTOS/portable/XCC/Xtensa
INCS       += -Iarch/include -Iarch/$(CONFIG_ARCH_PLATFORM)/include/
DFLAGS     += -DSTANDALONE=1 -DXTUTIL_NO_OVERRIDE
ifeq ($(CONFIG_PROJECT_SIM_SMALL),y)
DFLAGS     += -DSMALL_TEST
endif
ifeq ($(CONFIG_FPGA_PLATFORM),y)
DFLAGS     += -DXT_CLOCK_FREQ=24000000
endif
ifeq ($(CONFIG_CC_OPTIMIZE_FOR_DEBUG),y)
CFLAGS += -O0
endif
ifeq ($(CONFIG_CC_OPTIMIZE_FOR_PERFORMANCE_O2),y)
CFLAGS += -O2
endif
ifeq ($(CONFIG_CC_OPTIMIZE_FOR_PERFORMANCE_O3),y)
CFLAGS += -O3
endif
ifeq ($(CONFIG_CC_OPTIMIZE_FOR_SIZE),y)
CFLAGS += -Os
endif
CFLAGS     += -g -Wall -Werror
CFLAGS     += -ffunction-sections -fdata-sections -mlongcalls $(DFLAGS) $(INCS)
CFLAGS     += -I$(srctree)/include/generated -include autoconf.h -include map_func.h
LDFLAGS    += -Wl,--gc-sections
LDFLAGS    += -lm -lxtutil

MAKEFLAGS += --no-print-directory

LDFLAGS     +=  -Wl,--start-group

ifeq ($(CONFIG_COMPONENTS_XTENSA_HIFI5_NNLIB_LIBRARY),y)
LDFLAGS    += -L components/thirdparty/xtensa/hifi5_nn_v170_library/
LDFLAGS    += -lHiFi5_libxa_nnlib
endif

ifeq ($(CONFIG_COMPONENTS_XTENSA_HIFI5_VFPU_LIBRARY),y)
LDFLAGS    += -L components/thirdparty/xtensa/hifi5_vfpu_v200_library/
LDFLAGS    += -lHiFi5_vfpu_200_library
endif

LDFLAGS    += -Wl,--end-group

#export xt-xcc params for XtDevTools use
export CC AS AR LD CXX
export CFLAGS

.PHONY : device kernel FORCE menuconfig $(FORCE)

target_name = $(LICHEE_IC)_$(CONFIG_CORE_ID)_$(LICHEE_CHIP_BOARD)
target_out = $(srctree)/out/$(LICHEE_IC)/$(LICHEE_CHIP_BOARD)
target_def = $(srctree)/out/kernel/build/
target_xcc_name = $(target_name)_xcc

DIRS = $(target_def) $(target_out)

LDFLAGS    += -Wl,-Map=$(target_name).map


all: $(DIRS) $(target_name).elf

$(DIRS) :
	mkdir -p $@

sub_dir    := device kernel drivers components arch

ifeq ($(CONFIG_SYS_CONFIG_BUILDIN), y)
prepare += sysconf_prepare

.PHONY:sysconf_prepare
sysconf_prepare: $(LICHEE_CHIP_BOARD_DIR)/sys_config.fex
	$(shell \
		mkdir -p $(target_out); \
		cp $(LICHEE_CHIP_BOARD_DIR)/sys_config.fex $(target_out)/sys_config.fex; \
		busybox unix2dos $(target_out)/sys_config.fex; \
		$(LICHEE_TOP_DIR)/scripts/script $(target_out)/sys_config.fex > /dev/null; \
	)

endif

prepare += include/generated/autoconf.h include/reent.h
prepare += include/generated/version.h
ifeq ($(CONFIG_COMPONENTS_XTENSA_XTOS),y)
prepare += xtensa_xtos
endif
ifeq ($(CONFIG_COMPONENTS_XTENSA_LIBSIM),y)
prepare += xtensa_libsim
endif
ifeq ($(CONFIG_COMPONENTS_XTENSA_LIBGLOSS),y)
prepare += xtensa_libgloss
endif

cur_objs := $(addprefix $(target_def),$(addsuffix /obj-in.o,$(sub_dir)))

$(cur_objs): $(prepare) FORCE
	$(Q)set -e
	$(Q)$(MAKE) -f scripts/build/Makefile.build dir=$(notdir $(patsubst %/,%,$(dir $@))) out=$(target_def)

xtensa_xtos: $(prepare) FORCE
	$(Q)$(MAKE) -C components/thirdparty/xtensa/xtos -f Makefile.src \
		MAKEFILE_SRC=Makefile.src \
		INSTLIBDIR=$(LSP)
	$(Q)$(MAKE) -C components/thirdparty/xtensa/xtos -f Makefile.src install \
		MAKEFILE_SRC=Makefile.src \
		INSTLIBDIR=$(LSP)

xtensa_libsim: $(prepare) FORCE
	$(Q)$(MAKE) -C components/thirdparty/xtensa/sim -f Makefile.src \
		MAKEFILE_SRC=Makefile.src \
		INSTLIBDIR=$(LSP)
	$(Q)$(MAKE) -C components/thirdparty/xtensa/sim -f Makefile.src install \
		MAKEFILE_SRC=Makefile.src \
		INSTLIBDIR=$(LSP)

xtensa_libgloss: $(prepare) FORCE
	$(Q)$(MAKE) -C components/thirdparty/xtensa/libgloss -f Makefile.src \
		MAKEFILE_SRC=Makefile.src \
		INSTLIBDIR=$(LSP)
	$(Q)$(MAKE) -C components/thirdparty/xtensa/libgloss -f Makefile.src install \
		MAKEFILE_SRC=Makefile.src \
		INSTLIBDIR=$(LSP)

clean:
	$(Q)rm -rf $(target_def)
	$(Q)rm -rf $(target_out)
	$(Q)find -L ./ -path './XtDevTools*' -prune -o -name '*.o' -exec rm {} +
	$(Q)find -L ./ -path './XtDevTools*' -prune -o -name '.*.cmd' -exec rm {} +
	$(Q)find -L ./ -path './XtDevTools*' -prune -o -name '.*.d' -exec rm {} +
	$(Q)find -L ./ -path './XtDevTools*' -prune -o -name '*.tmp' -exec rm {} +
	$(Q)find ./ \( -path './XtDevTools*' -o -path './out' \) -prune -o -name '*.elf' -exec rm {} +
	$(Q)find ./ \( -path './XtDevTools*' -o -path './out' \) -prune -o -name '*.bin' -exec rm {} +
	$(Q)find ./ \( -path './XtDevTools*' -o -path './out' \) -prune -o -name '*.map' -exec rm {} +
	$(Q)find -path './include/reent.h' -delete
	$(Q)find -path './include/generated/*' -delete
	$(Q)find -path './include/config/*' -delete
	$(Q)$(MAKE) -C components/thirdparty/xtensa/libgloss -f Makefile.src clean \
		MAKEFILE_SRC=Makefile.src \
		INSTLIBDIR=$(LSP)

# create elf
$(target_name).elf : $(cur_objs)
	$(Q)set -e
	$(CC) $(CFLAGS) $(INCS) -o $@ $^ -mlsp=$(LSP) $(LDFLAGS) -DSTANDALONE=1


# Support for pack bin
pack: FORCE
	$(Q)echo -e "============= start pack for dsp bin ============="
	$(Q)if [ -f "$(srctree)/$(target_name).elf" ]; then \
		$(OBJS) $(srctree)/$(target_name).elf -o $(srctree)/$(target_xcc_name).bin; \
		./pack/dsp_pack -S $(target_xcc_name).bin -D $(target_name).bin; \
		$(OBJC) $(srctree)/$(target_name).elf -g -S -O binary -R .note -R .comment -R COMMON -R .init -R .fini --gap-fill=0xff $(target_name)_raw.bin; \
		mkdir -p $(target_out);\
		mv $(target_xcc_name).bin $(target_out);\
		mv $(target_name).bin $(target_out);\
		mv $(target_name)_raw.bin $(target_out);\
		mv $(target_name).elf $(target_out);\
		mv $(target_name).map $(target_out);\
		echo -e '\033[0;31;1m';\
		echo -e "$(target_out)/$(target_name).bin";\
		echo -e "$(target_out)/$(target_name)_raw.bin";\
		echo -e '\033[0m';\
		echo -e "\n#### make completed successfully ####\n"; \
	else \
		echo -e "ERROR: $(srctree)/$(target_name).elf does not exist\n"; \
	fi

# Support for menuconfig
menuconfig: FORCE
	$(Q)scripts/mconf Kconfig

#create head file
include/reent.h:
	-$(CP) $(subst /,$(S),$(CONFIGDIR)/xtensa-elf/include/sys/reent.h) $(subst /,$(S),./include/reent.h)

include/generated/autoconf.h: .config FORCE
	$(Q)mkdir -p include/generated
	$(Q)mkdir -p include/config
	$(Q)scripts/conf --silentoldconfig Kconfig

include/generated/version.h: FORCE
	$(Q)$(call filechk,version)

define filechk
	$(Q)set -e;				\
	echo '  CHK     $@';		\
	mkdir -p $(dir $@);			\
	$(filechk_$(1)) > $@.tmp;		\
	if [ -r $@ ] && cmp -s $@ $@.tmp; then	\
		rm -f $@.tmp;			\
	else					\
		echo '  UPD     $@';	\
		mv -f $@.tmp $@;		\
	fi
endef

define filechk_version
(echo "/*";\
	echo " * THIS IS CREATE WITH VERSION CHK";\
	echo " * DO NOT CHANGE IT MANUAL";\
	echo " */";\
	echo ;\
	echo "#ifndef _SUB_VER_";\
	echo "#define _SUB_VER_";\
	echo ;\
	echo "#define SUB_VER \"`scripts/setlocalversion`\"";\
	echo ;\
	echo "#endif /* _SUB_VER_ */";)
endef

