obj-$(CONFIG_COMPONENTS_AMP) += amp/
obj-$(CONFIG_COMPONENTS_RINGBUFFER) += ringbuffer/
obj-$(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM) += AudioSystem/
obj-$(CONFIG_COMPONENTS_LPVOICE) += lpvoice/
obj-$(CONFIG_COMPONENTS_RPDATA) += rpdata/
obj-$(CONFIG_COMPONENTS_OMX_SYSTEM) += OpenMax/
obj-$(CONFIG_COMPONENTS_TEST_PRINTF) += test_printf/
obj-$(CONFIG_COMPONENTS_RPBUF) += rpbuf/
obj-$(CONFIG_COMPONENTS_VIRT_LOG) += virt_log/
obj-$(CONFIG_COMPONENTS_STACK_PROTECTOR) += stackp/
obj-$(CONFIG_COMPONENTS_AW_SYS_CONFIG_SCRIPT) +=sys_config_script/
obj-$(CONFIG_COMPONENTS_AW_IOBOX) += iobox/
obj-$(CONFIG_COMPONENTS_SYSINFO) += sysinfo/
obj-$(CONFIG_COMPONENTS_MULTI_CONSOLE) += console/
obj-$(CONFIG_MULTI_CONSOLE) += multi_console/
obj-$(CONFIG_COMPONENT_THERMAL) += thermal/
obj-$(CONFIG_COMPONENT_PWM_TEM) += pwm_tem/
obj-$(CONFIG_COMPONENTS_BOOT_REASON) += boot_reason/
obj-$(CONFIG_COMPONENTS_AW_TINATEST) += tinatest/
obj-$(CONFIG_COMPONENTS_MSGBOX_DEMO) += msgbox_demo/
obj-$(CONFIG_COMPONENTS_AACTD_LIB) += aactd-lib/
obj-$(CONFIG_COMPONENTS_AACTD) += aactd/
obj-y += ota/
obj-y += ota_v2/
obj-y += usb/
obj-y += network/
obj-y += wireless/
obj-y += process_module/

ifeq ($(CONFIG_DRIVERS_XRADIO), y)
obj-y += xradio/
else ifeq ($(CONFIG_AW_COMPONENT_BLUEDROID), y)
obj-y += xradio/
else ifeq ($(CONFIG_AW_COMPONENT_BLUEDROID_CTRL), y)  #wait to remove
obj-y += xradio/
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2), y)
obj-y += xradio/
endif

obj-$(CONFIG_COMPONENTS_CAL_RC_LF) += cal_rc_lf/
obj-$(CONFIG_COMPONENTS_TEST_CPUFREQ) += test_cpufreq/
obj-$(CONFIG_COMPONENTS_ASR_DEMO) += asr_demo/

ifeq ($(CONFIG_COMPONENTS_PM), y)
obj-$(CONFIG_COMPONENTS_PM) += pm/
else
obj-y += pm/pm_empty.o
endif
