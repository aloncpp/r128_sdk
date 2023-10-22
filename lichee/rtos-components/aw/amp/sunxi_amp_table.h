#ifndef _SUNXI_AMP_TABLE_H
#define _SUNXI_AMP_TABLE_H

#include "sunxi_amp_msg.h"

#include "service/demo/demo_service.h"

typedef struct
{
    sunxi_amp_func_table open;
    sunxi_amp_func_table close;
    sunxi_amp_func_table read;
    sunxi_amp_func_table write;
    sunxi_amp_func_table lseek;
    sunxi_amp_func_table fstat;
    sunxi_amp_func_table stat;
    sunxi_amp_func_table unlink;
    sunxi_amp_func_table rename;
    sunxi_amp_func_table opendir;
    sunxi_amp_func_table readdir;
    sunxi_amp_func_table closedir;
    sunxi_amp_func_table mkdir;
    sunxi_amp_func_table rmdir;
    sunxi_amp_func_table access;
    sunxi_amp_func_table truncate;
    sunxi_amp_func_table statfs;
    sunxi_amp_func_table fstatfs;
    sunxi_amp_func_table fsync;
} RPCHandler_FSYS_t;

typedef struct
{
#ifdef CONFIG_ARCH_RISCV_RV64
	sunxi_amp_func_table wpa_ctrl_request;
#if (defined(CONFIG_WLAN_STA) && defined(CONFIG_WLAN_AP))
	sunxi_amp_func_table wlan_start;
	sunxi_amp_func_table wlan_stop;
#elif (defined(CONFIG_WLAN_STA))
	sunxi_amp_func_table wlan_start_sta;
	sunxi_amp_func_table wlan_stop_sta;
#elif (defined(CONFIG_WLAN_AP))
	sunxi_amp_func_table wlan_start_hostap;
	sunxi_amp_func_table wlan_stop_hostap;
#endif
	sunxi_amp_func_table wlan_get_mac_addr;
	sunxi_amp_func_table wlan_set_mac_addr;
	sunxi_amp_func_table wlan_set_ip_addr;
	sunxi_amp_func_table wlan_set_ps_mode;
	sunxi_amp_func_table wlan_set_appie;
	sunxi_amp_func_table wlan_set_channel;
	sunxi_amp_func_table net80211_mail_init;
	sunxi_amp_func_table net80211_mail_deinit;
	sunxi_amp_func_table net80211_ifnet_setup;
	sunxi_amp_func_table net80211_ifnet_release;
	sunxi_amp_func_table net80211_monitor_enable_rx;
	sunxi_amp_func_table wlan_send_raw_frame;
	sunxi_amp_func_table wlan_if_create;
	sunxi_amp_func_table wlan_if_delete;
	sunxi_amp_func_table wlan_linkoutput;
	sunxi_amp_func_table wlan_ext_request;
	sunxi_amp_func_table wlan_ext_low_power_param_set_default;
	/* lmac */
	sunxi_amp_func_table xradio_drv_cmd;
	/* mbuf */
	sunxi_amp_func_table mb_get;
	sunxi_amp_func_table mb_free;
#elif (defined CONFIG_ARCH_ARM_ARMV8M)
	sunxi_amp_func_table wlan_ap_get_default_conf;
	sunxi_amp_func_table wlan_event_notify;
	sunxi_amp_func_table wlan_monitor_input;
	sunxi_amp_func_table lwip_htons;
	sunxi_amp_func_table ethernetif_raw_input;
	sunxi_amp_func_table ethernetif_get_mode;
	sunxi_amp_func_table ethernetif_get_state;
	sunxi_amp_func_table wlan_ext_temp_volt_event_input;
#endif
} RPCHandler_NET_t;

typedef struct
{
#ifdef CONFIG_ARCH_RISCV_RV64
	sunxi_amp_func_table xrbtc_init;
	sunxi_amp_func_table xrbtc_deinit;
	sunxi_amp_func_table xrbtc_enable;
	sunxi_amp_func_table xrbtc_disable;
	sunxi_amp_func_table xrbtc_hci_init;
	sunxi_amp_func_table xrbtc_hci_h2c;
	sunxi_amp_func_table xrbtc_hci_c2h_cb;
	sunxi_amp_func_table xrbtc_sdd_init;
	sunxi_amp_func_table xrbtc_sdd_write;
#elif (defined CONFIG_ARCH_ARM_ARMV8M)
	sunxi_amp_func_table bt_event_notify;
#endif
} RPCHandler_BT_t;

typedef struct
{
	sunxi_amp_func_table _rpc_pm_wakelocks_getcnt_dsp;
	sunxi_amp_func_table _rpc_pm_msgtodsp_trigger_notify;
	sunxi_amp_func_table _rpc_pm_msgtodsp_trigger_suspend;
	sunxi_amp_func_table _rpc_pm_msgtodsp_check_subsys_assert;
	sunxi_amp_func_table _rpc_pm_msgtodsp_check_wakesrc_num;
} RPCHandler_PMOFDSP_t;

typedef struct
{
	sunxi_amp_func_table _rpc_pm_wakelocks_getcnt_riscv;
	sunxi_amp_func_table _rpc_pm_msgtorv_trigger_notify;
	sunxi_amp_func_table _rpc_pm_msgtorv_trigger_suspend;
	sunxi_amp_func_table _rpc_pm_msgtorv_check_subsys_assert;
	sunxi_amp_func_table _rpc_pm_msgtorv_check_wakesrc_num;
} RPCHandler_PMOFRV_t;

typedef struct
{
	sunxi_amp_func_table _rpc_pm_set_wakesrc;
	sunxi_amp_func_table _rpc_pm_trigger_suspend;
	sunxi_amp_func_table _rpc_pm_report_subsys_action;
	sunxi_amp_func_table _rpc_pm_subsys_soft_wakeup;
} RPCHandler_PMOFM33_t;

typedef struct
{
    sunxi_amp_func_table exe_cmd;
} RPCHandler_ARM_CONSOLE_t;

typedef struct
{
    sunxi_amp_func_table exe_cmd;
} RPCHandler_DSP_CONSOLE_t;

typedef struct
{
    sunxi_amp_func_table exe_cmd;
} RPCHandler_RV_CONSOLE_t;

#endif

typedef struct
{
    sunxi_amp_func_table nor_read;
    sunxi_amp_func_table nor_write;
    sunxi_amp_func_table nor_erase;
    sunxi_amp_func_table nor_ioctrl;
} RPCHandler_FLASHC_t;

typedef struct
{
    sunxi_amp_func_table misc_ioctrl;
} RPCHandler_RV_MISC_t;

typedef struct
{
    sunxi_amp_func_table misc_ioctrl;
} RPCHandler_M33_MISC_t;

typedef struct
{
    sunxi_amp_func_table misc_ioctrl;
} RPCHandler_DSP_MISC_t;

typedef struct
{
	sunxi_amp_func_table audio_test1;
	sunxi_amp_func_table audio_test2;
	sunxi_amp_func_table audio_test3;
	sunxi_amp_func_table audio_test4;
	/* AudioTrack */
	sunxi_amp_func_table AudioTrackCreateRM;
	sunxi_amp_func_table AudioTrackDestroyRM;
	sunxi_amp_func_table AudioTrackControlRM;
	sunxi_amp_func_table AudioTrackSetupRM;
	sunxi_amp_func_table AudioTrackWriteRM;
	/* AudioRecord */
	sunxi_amp_func_table AudioRecordCreateRM;
	sunxi_amp_func_table AudioRecordDestroyRM;
	sunxi_amp_func_table AudioRecordControlRM;
	sunxi_amp_func_table AudioRecordSetupRM;
	sunxi_amp_func_table AudioRecordReadRM;
} RPCHandler_AUDIO_t;

typedef struct
{
	sunxi_amp_func_table rpdata_test1;
	sunxi_amp_func_table rpdata_ioctl;
} RPCHandler_RPDATA_t;

typedef struct
{
	sunxi_amp_func_table tfm_sunxi_flashenc_set_region;
	sunxi_amp_func_table tfm_sunxi_flashenc_set_key;
	sunxi_amp_func_table tfm_sunxi_flashenc_set_ssk_key;
	sunxi_amp_func_table tfm_sunxi_flashenc_enable;
	sunxi_amp_func_table tfm_sunxi_flashenc_disable;
} RPCHandler_TFM_t;
