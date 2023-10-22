#ifndef _BTCLI_COMMON_H_
#define _BTCLI_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_manager.h"

enum cmd_status btcli_init(char *cmd);
enum cmd_status btcli_deinit(char *cmd);
enum cmd_status btcli_debug(char *cmd);
enum cmd_status btcli_ex_debug(char *cmd);
enum cmd_status btcli_pincode(char *cmd);
enum cmd_status btcli_passkey(char *cmd);
enum cmd_status btcli_passkey_confirm(char *cmd);
enum cmd_status btcli_pairing_confirm(char *cmd);
enum cmd_status btcli_scan_list(char *cmd);
enum cmd_status btcli_paired_list(char *cmd);
enum cmd_status btcli_unpair_dev(char *cmd);
enum cmd_status btcli_set_scanmode(char *cmd);
enum cmd_status btcli_scan(char *cmd);
enum cmd_status btcli_set_io_cap(char *cmd);
enum cmd_status btcli_get_device_name(char *cmd);
enum cmd_status btcli_get_adapter_name(char *cmd);
enum cmd_status btcli_set_adapter_name(char *cmd);
enum cmd_status btcli_get_adapter_mac(char *cmd);
enum cmd_status btcli_a2dp_sink(char *cmd);
enum cmd_status btcli_a2dp_source(char *cmd);
enum cmd_status btcli_gap(char *cmd);
enum cmd_status btcli_avrc(char *cmd);
enum cmd_status btcli_hfp(char *cmd);
enum cmd_status btcli_ag(char *cmd);
enum cmd_status btcli_sppc(char *cmd);
enum cmd_status btcli_spps(char *cmd);
enum cmd_status btcli_ble(char *cmd);
enum cmd_status btcli_gatt(char *cmd);

#ifdef CONFIG_BT_A2DP_ENABLE
void btcli_a2dp_source_connection_state_cb(const char *bd_addr,
                                                  btmg_a2dp_source_connection_state_t state);
void btcli_a2dp_source_audio_state_cb(const char *bd_addr,
                                             btmg_a2dp_source_audio_state_t state);
void btcli_a2dp_source_check_state(void);
void btcli_a2dp_sink_connection_state_cb(const char *bd_addr,
                                                btmg_a2dp_sink_connection_state_t state);
void btcli_a2dp_sink_audio_state_cb(const char *bd_addr, btmg_a2dp_sink_audio_state_t state);
void btcli_a2dp_sink_stream_cb(const char *bd_addr, uint16_t channels, uint16_t sampling,
                                      uint8_t *data, uint32_t len);
void btcli_avrcp_ct_play_state_cb(const char *bd_addr, btmg_avrcp_play_state_t state);
void btcli_avrcp_ct_track_changed_cb(const char *bd_addr, btmg_track_info_t *track_info);
void btcli_avrcp_ct_play_position_cb(const char *bd_addr, int song_len, int song_pos);
void btcli_avrcp_audio_volume_cb(const char *bd_addr, unsigned int volume);
void btcli_avrcp_tg_play_state_cb(const char *bd_addr, btmg_avrcp_play_state_t state);
#endif
#ifdef CONFIG_BT_SPP_ENABLED
void btcli_sppc_conn_status_cb(const char *bd_addr, btmg_spp_connection_state_t status);
void btcli_sppc_recvdata_cb(const char *bd_addr, char *data, int data_len);
void btcli_spps_conn_status_cb(const char *bd_addr, btmg_spp_connection_state_t status);
void btcli_spps_recvdata_cb(const char *bd_addr, char *data, int data_len);
#endif
#ifdef CONFIG_BT_HFP_CLIENT_ENABLE
void btcli_hfp_hf_event_cb(btmg_hfp_hf_event_t event, void *data);
void btcli_hfp_hf_connection_state_cb(const char *bd_addr,
                                             btmg_hfp_hf_connection_state_t state);
#endif
#ifdef CONFIG_BT_HFP_AG_ENABLE
void btcli_hfp_ag_event_cb(btmg_hfp_ag_event_t event, void *data);
void btcli_hfp_ag_connection_state_cb(const char *bd_addr,
                                             btmg_hfp_ag_connection_state_t state);
void btcli_hfp_ag_audio_incoming_cb(const uint8_t *buf, uint32_t sz);
uint32_t btcli_hfp_ag_audio_outgoing_cb(uint8_t *p_buf, uint32_t sz);
#endif
void btcli_ble_scan_cb(le_scan_cb_para_t *data);
void btcli_ble_connection_cb(le_connection_para_t *data);
void btcli_gattc_dis_att_cb(gattc_dis_cb_para_t *data);
void btcli_gattc_notify_indicate_cb(gattc_notify_indicate_cb_para_t *data);
void btcli_gatts_get_db_cb(gatts_get_db_t *data);
void btcli_gatts_char_read_req_cb(gatts_char_read_req_t *data);
void btcli_gatts_char_write_req_cb(gatts_char_write_req_t *data);
void btcli_gatts_ccc_cfg_cb(gatts_ccc_cfg_t *data);
void btcli_gatts_indicate_cb(gatts_indicate_cb_t *data);
void btcli_gattc_read_cb(gattc_read_cb_para_t *data);
void btcli_gattc_write_cb(gattc_write_cb_para_t *data);

void btcli_ble_smp_passkey_display_cb(le_smp_passkey_display_para_t *data);
void btcli_ble_smp_passkey_confirm_cb(le_smp_passkey_confirm_para_t *data);
void btcli_ble_smp_passkey_enter_cb(le_smp_passkey_enter_para_t *data);
void btcli_ble_smp_cancel_cb(le_smp_cancel_para_t *data);
void btcli_ble_smp_pairing_confirm_cb(le_smp_pairing_confirm_para_t *data);
void btcli_ble_smp_pairing_failed_cb(le_smp_pairing_complete_para_t *data);
void btcli_ble_smp_pairing_complete_cb(le_smp_pairing_complete_para_t *data);

int btcli_ble_set_adv_data(void);
int btcli_ble_set_ext_adv_data(void);
int btcli_ble_advertise_on(void);
int btcli_ble_ext_advertise_on(void);
int btcli_gatt_register_test_service(void);
int btcli_gatt_unregister_test_service(void);

void btcli_gatts_attributedata_delete(int conn_id);
int btcli_gatts_attributedata_mtu_exchanged(int conn_id);
void btcli_gatts_attributedata_create(int conn_id, int handle);


#ifdef __cplusplus
}
#endif

#endif /* __BTCLI_COMMON_H_*/

