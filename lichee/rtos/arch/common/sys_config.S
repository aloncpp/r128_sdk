#ifdef CONFIG_SYS_CONFIG_BUILDIN
  .section .blobdata, "a"
.global blob_fexconfig_start
blob_fexconfig_start:
	.incbin SYSCONFG_BIN_PATH
.global blob_fexconfig_end
blob_fexconfig_end:
#endif
