BASE_PATH:=$(dir $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))
BASE:=$(patsubst %/,%,$(BASE_PATH))
BUILD_BASE:=$(BASE)
MODULE_NAME:="Tina-RT-Builder"

export PROJECT_CONFIG
ifndef PROJECT_DIR
PROJECT_DIR:=$(shell pwd -P)
PROJECT_CONFIG:=n
else
PROJECT_CONFIG:=y
endif

IMAGE_NAME ?= rt_system
BUILD_DIR:=$(PROJECT_DIR)/build/$(RTOS_PROJECT_NAME)
SYSCONF_DIR:=$(PROJECT_DIR)/../../board/$(RTOS_TARGET_BOARD_PATH)/configs
IMG_DIR:=$(BUILD_DIR)/img
TARGET:=$(IMG_DIR)/${IMAGE_NAME}.bin
TARGET_XIP:=$(IMG_DIR)/${IMAGE_NAME}_xip.bin
TARGET_PSRAM:=$(IMG_DIR)/${IMAGE_NAME}_psram.bin
TARGET_HPSRAM:=$(IMG_DIR)/${IMAGE_NAME}_hpsram.bin
TARGET_DEPS:=$(IMG_DIR)/${IMAGE_NAME}.elf
TARGET_SYMS:=$(IMG_DIR)/$(IMAGE_NAME).syms
TARGET_LIST:=$(IMG_DIR)/$(IMAGE_NAME).list
TARGET_MAP:=$(IMG_DIR)/$(IMAGE_NAME).map
TARGET_EXPORTS:=$(IMG_DIR)/$(IMAGE_NAME).exports


CONFIG_:=CONFIG_
#CONFIG_HEADER_NAME:=bsp_config.h
ifneq ($(PROJECT_CONFIG), y)
CONFIG_PATH:=$(PROJECT_DIR)
#CONFIG_HEADER_PATH:=$(BASE)
else
CONFIG_PATH:=$(PROJECT_DIR)
#CONFIG_HEADER_PATH:=$(PROJECT_DIR)/include
endif

include $(BASE)/.dbuild/dbuild.mk

$(PROJECT_DIR)/.config:
	$(Q)echo " ******************************************************"
	$(Q)echo "   >>>> No .config file found, run make menuconfig"
	$(Q)echo " ******************************************************"
	@false;

OBJCOPYFLAGS := -g -S -O binary -R .note -R .comment -R COMMON -R .ARM.attributes -R .en_frame -R .init -R .fini --gap-fill=0x00
ifeq ($(CONFIG_XIP), y)
	OBJCOPY_R_XIP := -R .xip
	OBJCOPY_J_XIP := -j .xip
endif
ifeq ($(CONFIG_PSRAM), y)
	OBJCOPY_R_PSRAM := -R .psram_text -R .psram_data -R .psram_bss
	OBJCOPY_J_PSRAM := -j .psram_text -j .psram_data
endif
ifeq ($(CONFIG_HPSRAM), y)
	OBJCOPY_R_HPSRAM := -R .hpsram_text -R .hpsram_data -R .hpsram_bss
	OBJCOPY_J_HPSRAM := -j .hpsram_text -j .hpsram_data
endif

ifeq ($(CONFIG_COMPONENTS_TFM), y)
	TFM_S_VENEERS := $(BASE)/components/aw/tfm/lib/s_veneers.o
else
	TFM_S_VENEERS :=
endif

all: $(TARGET) $(TARGET_DEPS) $(TARGET_SYMS)
	$(Q)$(SIZE) $(subst $(BASE)/,,$(TARGET_DEPS))

ifeq ($(BT_CONFIG_BUILD_DISASSEMBLE), y)
all: $(PROJECT_DIR)/${IMAGE_NAME}.list
endif

list: $(PROJECT_DIR)/${IMAGE_NAME}.list
.PHONY: list

$(TARGET): $(TARGET_DEPS)
	$(Q)$(PRETTY) IMAGE $(MODULE_NAME) $(subst $(PROJECT_DIR)/,"",$@)
	$(Q)$(OBJCOPY) $(subst $(BASE)/,,$(TARGET_DEPS)) $(OBJCOPYFLAGS) $(OBJCOPY_R_XIP) $(OBJCOPY_R_PSRAM) $(OBJCOPY_R_HPSRAM) $(subst $(BASE)/,,$@)
ifeq ($(CONFIG_XIP), y)
	$(Q)$(OBJCOPY) $(subst $(BASE)/,,$(TARGET_DEPS)) -O binary $(OBJCOPY_J_XIP) $(subst $(BASE)/,,$(TARGET_XIP))
endif
ifeq ($(CONFIG_PSRAM), y)
	$(Q)$(OBJCOPY) $(subst $(BASE)/,,$(TARGET_DEPS)) -O binary $(OBJCOPY_J_PSRAM) $(subst $(BASE)/,,$(TARGET_PSRAM))
endif
ifeq ($(CONFIG_HPSRAM), y)
	$(Q)$(OBJCOPY) $(subst $(BASE)/,,$(TARGET_DEPS)) -O binary $(OBJCOPY_J_HPSRAM) $(subst $(BASE)/,,$(TARGET_HPSRAM))
endif

$(LINKER_SCRIPTS): $(CONFIG_HEADER_PATH)/$(CONFIG_HEADER_NAME)

# Usage:
#   $(call link_execable_file, input.o, output.o)
define link_execable_file
$(Q)$(CC) $(CC_MARCH) $(CC_MTUNE) \
		$(CC_TCFLAGS) $(CC_MACHFLAGS) \
		$(CC_MFPU) $(CC_FPU_ABI) \
		-o $(subst $(BASE)/,,$2) -T $(subst $(BASE)/,,$(LINKER_SCRIPT)) \
		-Wl,-Map=$(subst $(BASE)/,,$(TARGET_MAP)) -Wl,--whole-archive  \
		$(subst $(BASE)/,, $1) -Wl,--no-whole-archive $(subst $(BASE)/,,$(LDFLAGS)) $(subst $(BASE)/,,$(LDFLAGS-y)) \
		-Wl,--start-group $(subst $(BASE)/,,$(LDLIBS)) \
		-Wl,--end-group -lc -lm -lgcc
endef

define cc_kallsyms
	gcc -o scripts/kallsyms/kallsyms -DBITS_PER_LONG=$(CONFIG_BITS_PER_LONG) \
		scripts/kallsyms/kallsyms.c
endef
# Usage:
#   $(call gen_ksym_o, input.o, output.o)
define gen_ksym_o
nm $1 | ./scripts/kallsyms/kallsyms $(if $(CONFIG_COMPONENTS_KALLSYMS_ALL), --all-symbols,) > sym.S; \
	$(CC) $(CC_MARCH) $(CC_MTUNE) -c -o $2  sym.S -MD -MP $(CFLAGS); \
	rm sym.S;
endef

define link_execable_file_by_cmd
	allfiles=(build/$(RTOS_PROJECT_NAME)/arch/.obj-in.o.cmd build/$(RTOS_PROJECT_NAME)/drivers/.obj-in.o.cmd build/$(RTOS_PROJECT_NAME)/components/.obj-in.o.cmd build/$(RTOS_PROJECT_NAME)/kernel/.obj-in.o.cmd build/$(RTOS_PROJECT_NAME)/projects/.obj-in.o.cmd); \
	function parse_cmd_files() \
	{ \
		local line; \
		local var; \
		local array; \
		local cmd_files; \
		\
		while read line; \
		do \
			array=($${line// / }); \
			for var in $${array[@]}; \
			do \
			if [[ $$var == *.o  ]]; then \
				if [[ $$var != */obj-in.o  ]]; then \
					objects+="$$var "; \
				else \
					if [[ $$var != cmd_*  ]]; then \
						local newfile=$${var//obj-in.o/.obj-in.o.cmd}; \
						if [[ $$1 != $$newfile ]]; then \
							cmd_files=($${cmd_files[@]} $$newfile); \
						fi \
					fi \
				fi \
			fi \
		done; \
		done < $$1; \
		for sfile in $${cmd_files[@]}; \
		do \
			parse_cmd_files $$sfile; \
		done; \
	}; \
	for sfile in $${allfiles[@]}; \
	do \
		parse_cmd_files $$sfile; \
	done; \
	$(call link_execable_file, $$objects $1, $2)
endef

$(TARGET_DEPS): $(OBJECTS) $(TFM_S_VENEERS) $(LINKER_SCRIPTS) $(KBUILD_OBJECTS)
	$(Q)$(PRETTY) --dbuild "LD" $(MODULE_NAME) $(subst $(PROJECT_DIR)/,"",$@)
	if [ -n $(IMG_DIR)/  ]; then mkdir -p $(IMG_DIR); fi

ifeq ($(CONFIG_COMPONENTS_KALLSYMS), y)
	$(Q)$(PRETTY) --dbuild "CC" KSYM scripts/kallsyms/kallsyms.c
	@$(call cc_kallsyms)
	@# step 1 Link mm1.o it has all symbols buf kallsyms is empty
	$(Q)$(PRETTY) --dbuild "SYMS" GEN .tmp_ksyms1
	@#$(call link_execable_file, $(OBJECTS) $(KBUILD_OBJECTS) $(TFM_S_VENEERS), .tmp_aw_rtos1.o)
	@$(call link_execable_file_by_cmd, $(TFM_S_VENEERS), .tmp_aw_rtos1.o)
	@$(call gen_ksym_o, .tmp_aw_rtos1.o, .tmp_ksyms1.o)

	@# step 2 Libk mm2 to geve kallsyms with right size
	$(Q)$(PRETTY) --dbuild "SYMS" GEN .tmp_ksyms2
	@# $(call link_execable_file, $(OBJECTS) $(KBUILD_OBJECTS) $(TFM_S_VENEERS) .tmp_ksyms1.o, .tmp_aw_rtos2.o)
	@$(call link_execable_file_by_cmd, $(TFM_S_VENEERS) .tmp_ksyms1.o , .tmp_aw_rtos2.o)
	@$(call gen_ksym_o, .tmp_aw_rtos2.o, .tmp_ksyms2.o)

	@# step 3 The correct kallsyms is linked to finale obj file
	@# $(call link_execable_file, $(OBJECTS) $(KBUILD_OBJECTS) $(TFM_S_VENEERS) .tmp_ksyms2.o, $@)
	@$(call link_execable_file_by_cmd, $(TFM_S_VENEERS) .tmp_ksyms2.o , $@)
	$(Q)rm .tmp_aw_rtos*
	$(Q)rm .tmp_ksyms*
else
	@$(call link_execable_file, $(OBJECTS) $(KBUILD_OBJECTS) $(TFM_S_VENEERS), $@)
endif

$(TARGET_LIST): $(TARGET_DEPS)
	$(Q)$(PRETTY) LIST $(MODULE_NAME) $(subst $(PROJECT_DIR)/,"",$@)
ifeq ($(BT_CONFIG_BUILD_DISASSEMBLE_SOURCE), y)
	$(Q)$(OBJDUMP) -D -S $(subst $(BASE)/,,$(TARGET_DEPS)) > $(subst $(BASE)/,,$@)
else
	$(Q)$(OBJDUMP) -D $(subst $(BASE)/,,$(TARGET_DEPS)) > $(subst $(BASE)/,,$@)
endif

$(TARGET_SYMS): $(TARGET_DEPS)
	$(Q)$(PRETTY) SYMS $(MODULE_NAME) $(subst $(PROJECT_DIR)/,"",$@)
	$(Q)$(OBJDUMP) -t $(subst $(BASE)/,,$(TARGET_DEPS)) > $(subst $(BASE)/,,$@)

#$(TARGET_EXPORTS: $(TARGET_DEPS)
#	$(Q)$(PRETTY) SYMS $(MODULE_NAME) $(subst $(PROJECT_DIR)/,"",$@)
#	$(Q)$(BASE)/scripts/symbol_status.sh $(TARGET_DEPS) $(BASE) > $@

.PHONY:checksyms
checksyms:$(TARGET_EXPORTS)

.PHONY:linktree
linktree:
	nm $(OBJECTS) > linktree

$(OBJECTS) $(OBJECTS-y): $(PROJECT_DIR)/.config

ifeq ($(CONFIG_SYS_CONFIG_BUILDIN), y)
$(LINKER_SCRIPTS): sysconf_prepare

.PHONY:sysconf_prepare
sysconf_prepare: $(SYSCONF_DIR)/sys_config.fex
	$(Q)$(PRETTY) CC SCRIPT $(subst $(PROJECT_DIR)/,"", $(IMG_DIR)/sys_config.fex)
	$(shell \
		mkdir -p $(IMG_DIR); \
		cp $(SYSCONF_DIR)/sys_config.fex $(IMG_DIR)/sys_config.fex; \
		busybox unix2dos $(IMG_DIR)/sys_config.fex; \
		$(PROJECT_DIR)/../../tools/tool/script $(IMG_DIR)/sys_config.fex > /dev/null; \
	)
endif

project.init:
	$(Q)touch $(PROJECT_DIR)/Kconfig
	$(Q)touch $(PROJECT_DIR)/objects.mk
	$(Q)touch $(PROJECT_DIR)/README.md
	$(Q)touch $(PROJECT_DIR)/main.c
	-$(Q)mkdir $(PROJECT_DIR)/include
	$(Q)echo "export PROJECT_DIR=\$$(shell pwd -P)" >> $(PROJECT_DIR)/Makefile
	$(Q)echo "export PROJECT_CONFIG=y" >> $(PROJECT_DIR)/Makefile
	$(Q)echo "include $(shell $(RELPATH) $(BASE) $(PROJECT_DIR))/Makefile" >> $(PROJECT_DIR)/Makefile
	$(Q)echo "objs += \$$(APP)/main.o" >> $(PROJECT_DIR)/objects.mk

project.git.init:
	-$(Q)cd $(PROJECT_DIR) && git init .
	-$(Q)cd $(PROJECT_DIR) && git submodule add git://github.com/jameswalmsley/bitthunder.git bitthunder
	$(Q)touch $(PROJECT_DIR)/Kconfig
	$(Q)touch $(PROJECT_DIR)/objects.mk
	$(Q)touch $(PROJECT_DIR)/README.md
	$(Q)touch $(PROJECT_DIR)/main.c
	-$(Q)mkdir $(PROJECT_DIR)/include
	$(Q)echo "export PROJECT_DIR=\$$(shell pwd -P)" >> $(PROJECT_DIR)/Makefile
	$(Q)echo "export PROJECT_CONFIG=y" >> $(PROJECT_DIR)/Makefile
	$(Q)echo "include bitthunder/Makefile" >> $(PROJECT_DIR)/Makefile
	$(Q)echo "objs += \$$(APP)/main.o" >> $(PROJECT_DIR)/objects.mk

project.info:
	@echo "PROJECT_DIR      : $(PROJECT_DIR)"
	@echo "BUILD_DIR        : $(BUILD_DIR)"
	@echo "BASE             : $(BASE)"
	@echo "PROJECT_CONFIG   : $(PROJECT_CONFIG)"

.PHONY:defconfig
defconfig:
ifdef BT_CONFIG_BOARD_DEFCONFIG_FILE
	@echo "Copying $(ARCH)/mach/$(SUBARCH)/boards/$(BT_CONFIG_BOARD_DEFCONFIG_FILE) to .config"
	@cp $(BASE)/arch/$(ARCH)/mach/$(SUBARCH)/boards/$(BT_CONFIG_BOARD_DEFCONFIG_FILE) $(PROJECT_DIR)/.config
else
	@echo "No default board configuration for $(ARCH)/$(SUBARCH)"
endif

.PHONY:upconfig
upconfig:
ifdef BT_CONFIG_BOARD_DEFCONFIG_FILE
	@echo "Updating $(ARCH)/mach/$(SUBARCH)/boards/$(BT_CONFIG_BOARD_DEFCONFIG_FILE) from .config"
	@cp $(PROJECT_DIR)/.config $(BASE)/arch/$(ARCH)/mach/$(SUBARCH)/boards/$(BT_CONFIG_BOARD_DEFCONFIG_FILE)
else
	# Print the defconfig error message.
	@$(MAKE) defconfig
endif

mrproper: clean
ifneq ($(PROJECT_CONFIG),y)
	$(Q)rm -rf $(PRM_FLAGS) $(PROJECT_DIR)/.config $(BASE)/include/generated/$(RTOS_PROJECT_NAME)/ $(PRM_PIPE)
else
	$(Q)rm -rf $(PRM_FLAGS) $(PROJECT_DIR)/.config $(PROJECT_DIR)/include/generated/$(RTOS_PROJECT_NAME)/ $(PRM_PIPE)
endif

distclean: mrproper

clean: clean_images
clean_images: | dbuild_splash
	$(Q)rm -rf $(PROJECT_DIR)/build/$(RTOS_PROJECT_NAME)
	$(Q)rm $(PRM_FLAGS) $(TARGET) $(TARGET_DEPS) $(TARGET_LIST) $(TARGET_SYMS) $(TARGET_EXPORTS) $(TARGET_MAP) $(PRM_PIPE)
	$(Q)rm -rf $(CONFIG_HEADER_PATH)/$(CONFIG_HEADER_NAME) $(PRM_PIPE)
	$(Q)rm -rf $(PROJECT_DIR)/linktree

$(CONFIG_HEADER_PATH)/$(CONFIG_HEADER_NAME): MODULE_NAME:=$(MODULE_NAME)
