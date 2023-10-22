#ifdef CONFIG_SYS_CONFIG_BUILDIN
.global blob_fexconfig_start
blob_fexconfig_start:
.align 4
.incbin SYSCONFG_BIN_PATH
.global blob_fexconfig_end
blob_fexconfig_end:
#endif
