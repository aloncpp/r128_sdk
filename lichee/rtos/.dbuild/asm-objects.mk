
$(BUILD_DIR)/%.o: $(BASE)/%.s
	$(Q)$(PRETTY) --dbuild "AS" $(MODULE_NAME) $(subst $(BUILD_DIR)/,"",$@)
	@mkdir -p $(dir $@)
	$(Q)$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(BASE)/%.S
	$(Q)$(PRETTY) --dbuild "AS" $(MODULE_NAME) $(subst $(BUILD_DIR)/,"",$@)
	@mkdir -p $(dir $@)
	$(Q)$(CC) -c $(ASFLAGS) $< -o $@


$(BUILD_DIR)/application/%.o: $(PROJECT_DIR)/%.s
	$(Q)$(PRETTY) --dbuild "AS" $(MODULE_NAME) $(subst $(BUILD_DIR)/,"",$@)
	@mkdir -p $(dir $@)
	$(Q)$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/application/%.o: $(PROJECT_DIR)/%.S
	$(Q)$(PRETTY) --dbuild "AS" $(MODULE_NAME) $(subst $(BUILD_DIR)/,"",$@)
	@mkdir -p $(dir $@)
	$(Q)$(CC) -c $(ASFLAGS) $< -o $@
