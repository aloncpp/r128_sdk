#include <stdlib.h>
#include <string.h>

#include "bt_manager.h"
#include "btmg_common.h"
#include "btmg_log.h"
#include "xr_avrc_api.h"
#include <AudioSystem.h>

#define APP_RC_CT_TL_GET_CAPS                 (0)
#define APP_RC_CT_TL_RN_VOLUME_CHANGE         (1)
#define APP_RC_CT_TL_GET_META_DATA            (1)
#define APP_RC_CT_TL_RN_TRACK_CHANGE          (2)
#define APP_RC_CT_TL_RN_PLAYBACK_CHANGE       (3)
#define APP_RC_CT_TL_RN_PLAY_POS_CHANGE       (4)
#define APP_RC_CT_TL_RN_BATTERY_STATUS_CHANGE (5)

static uint8_t s_volume = 0;
static int song_len = 0;
static xr_avrc_rn_evt_cap_mask_t s_avrc_peer_rn_cap;
static btmg_track_info_t track_info;

static int set_system_abs_vol(unsigned int value)
{
    int ret = 0;
    int type = AUDIO_STREAM_SYSTEM;
    uint32_t volume_value = 0;
    uint8_t max_volume = 0;

    ret = softvol_control_with_streamtype(type, &volume_value, 2);
    if (ret != 0) {
        BTMG_ERROR("get softvol range failed:%d", ret);
        return -1;
    }
    max_volume = (volume_value >> 16) & 0xffff;
    volume_value = (value * max_volume / 100) & 0xffff;
    ret = softvol_control_with_streamtype(type, &volume_value, 1);
    if (ret != 0) {
        BTMG_ERROR("set softvol failed:%d", ret);
        return -1;
    }

    return ret;
}

static int get_system_abs_vol(void)
{
    int ret = 0;
    int type = AUDIO_STREAM_SYSTEM;
    uint32_t volume_value = 0;
    uint8_t max_volume = 0;

    ret = softvol_control_with_streamtype(type, &volume_value, 2);
    if (ret != 0) {
        BTMG_ERROR("get softvol range failed:%d", ret);
        return -1;
    }

    max_volume = (volume_value >> 16) & 0xffff;
    ret = softvol_control_with_streamtype(type, &volume_value, 0);
    if (ret != 0) {
        BTMG_ERROR("get softvol failed:%d", ret);
        return -1;
    }

    volume_value = volume_value & 0xffff;

    return volume_value * 100 / max_volume;
}

static void bt_av_reg_new_track(void)
{
    xr_avrc_ct_send_metadata_cmd(APP_RC_CT_TL_GET_META_DATA, 0);
    if (xr_avrc_rn_evt_bit_mask_operation(XR_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                          XR_AVRC_RN_TRACK_CHANGE)) {
        xr_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_TRACK_CHANGE,
                                                  XR_AVRC_RN_TRACK_CHANGE, 0);
    }
}

static void bt_av_reg_playback_changed(void)
{
    if (xr_avrc_rn_evt_bit_mask_operation(XR_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                          XR_AVRC_RN_PLAY_STATUS_CHANGE)) {
        xr_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_PLAYBACK_CHANGE,
                                                  XR_AVRC_RN_PLAY_STATUS_CHANGE, 0);
    }
}

static void bt_av_reg_play_pos_changed(void)
{
    if (xr_avrc_rn_evt_bit_mask_operation(XR_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                          XR_AVRC_RN_PLAY_POS_CHANGED)) {
        xr_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_PLAY_POS_CHANGE,
                                                  XR_AVRC_RN_PLAY_POS_CHANGED, 10);
    }
}

static void bt_av_reg_battery_status_changed(void)
{
    if (xr_avrc_rn_evt_bit_mask_operation(XR_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                          XR_AVRC_RN_BATTERY_STATUS_CHANGE)) {
        xr_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_BATTERY_STATUS_CHANGE,
                                                  XR_AVRC_RN_BATTERY_STATUS_CHANGE, 10);
    }
}

static void bt_av_reg_volume_changed(void)
{
    if (xr_avrc_rn_evt_bit_mask_operation(XR_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                          XR_AVRC_RN_VOLUME_CHANGE)) {
        xr_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_VOLUME_CHANGE,
                                                  XR_AVRC_RN_VOLUME_CHANGE, 0);
    }
}

void bt_av_notify_evt_handler(uint8_t event_id, xr_avrc_rn_param_t *event_parameter)
{
    switch (event_id) {
    case XR_AVRC_RN_TRACK_CHANGE:
        bt_av_reg_new_track();
        break;
    case XR_AVRC_RN_PLAY_STATUS_CHANGE:
        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_ct_play_state_cb) {
            if (event_parameter->playback == XR_AVRC_PLAYBACK_STOPPED) {
                btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_ct_play_state_cb(bda_str, BTMG_AVRCP_PLAYSTATE_STOPPED);
            } else if (event_parameter->playback == XR_AVRC_PLAYBACK_PLAYING) {
                btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_ct_play_state_cb(bda_str, BTMG_AVRCP_PLAYSTATE_PLAYING);
            } else if (event_parameter->playback == XR_AVRC_PLAYBACK_PAUSED) {
                btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_ct_play_state_cb(bda_str, BTMG_AVRCP_PLAYSTATE_PAUSED);
            } else if (event_parameter->playback == XR_AVRC_PLAYBACK_FWD_SEEK) {
                btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_ct_play_state_cb(bda_str,
                                                             BTMG_AVRCP_PLAYSTATE_FWD_SEEK);
            } else if (event_parameter->playback == XR_AVRC_PLAYBACK_REV_SEEK) {
                btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_ct_play_state_cb(bda_str,
                                                             BTMG_AVRCP_PLAYSTATE_REV_SEEK);
            } else if (event_parameter->playback == 5) {
                btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_ct_play_state_cb(bda_str, BTMG_AVRCP_PLAYSTATE_FORWARD);
            } else if (event_parameter->playback == 6) {
                btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_ct_play_state_cb(bda_str,
                                                             BTMG_AVRCP_PLAYSTATE_BACKWARD);
            } else if (event_parameter->playback == XR_AVRC_PLAYBACK_ERROR) {
                btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_ct_play_state_cb(bda_str, BTMG_AVRCP_PLAYSTATE_ERROR);
            }
        }
        bt_av_reg_playback_changed();
        break;
    case XR_AVRC_RN_PLAY_POS_CHANGED:
        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_ct_play_position_cb) {
            btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_ct_play_position_cb(bda_str, song_len,
                                                            event_parameter->play_pos);
        }
        bt_av_reg_play_pos_changed();
        break;
    case XR_AVRC_RN_BATTERY_STATUS_CHANGE:
        BTMG_DEBUG("Remote device battery status: %d\n", event_parameter->batt);
        bt_av_reg_battery_status_changed();
        break;
    case XR_AVRC_RN_VOLUME_CHANGE: //作为source时用到
        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_audio_volume_cb) {
            unsigned int value = (unsigned int)event_parameter->volume * 100 / 0x7f;
            btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_audio_volume_cb(bda_str, value);
        }
        bt_av_reg_volume_changed();
        break;
    default:
        BTMG_ERROR("%s unhandled evt %d", __func__, event_id);
        break;
    }
}

static void bt_avrc_ct_cb(xr_avrc_ct_cb_event_t event, xr_avrc_ct_cb_param_t *rc)
{
    switch (event) {
    case XR_AVRC_CT_CONNECTION_STATE_EVT: {
        bda2str(rc->conn_stat.remote_bda, bda_str);
        if (rc->conn_stat.connected) {
            BTMG_INFO("avrcp ct Connected, dev:%s", bda_str);
            // get remote supported event_ids of peer AVRCP Target
            xr_avrc_ct_send_get_rn_capabilities_cmd(APP_RC_CT_TL_GET_CAPS);
        } else {
            BTMG_INFO("avrcp ct Disconnected, dev:%s", bda_str);
            // clear peer notification capability record
            s_avrc_peer_rn_cap.bits = 0;
        }
        break;
    }
    case XR_AVRC_CT_PASSTHROUGH_RSP_EVT: { //发送暂停pass through命令回复是否接受，没啥用
        BTMG_DEBUG("AVRC passthrough rsp: key_code 0x%x, key_state %d", rc->psth_rsp.key_code,
                   rc->psth_rsp.key_state);
        break;
    }
    case XR_AVRC_CT_METADATA_RSP_EVT: {
        if (rc->meta_rsp.attr_id != 0x40 ) {
            BTMG_INFO("AVRC metadata rsp:attribute id 0x%x:%s", rc->meta_rsp.attr_id, rc->meta_rsp.attr_text);
        }

        if (rc->meta_rsp.attr_id == 0x40) {
            song_len = atoi(rc->meta_rsp.attr_text);
        }
        break;
    }
    case XR_AVRC_CT_CHANGE_NOTIFY_EVT: {
        bt_av_notify_evt_handler(rc->change_ntf.event_id, &rc->change_ntf.event_parameter);
        break;
    }
    case XR_AVRC_CT_REMOTE_FEATURES_EVT: {
        BTMG_DEBUG("AVRC remote features %x, TG features %x", rc->rmt_feats.feat_mask,
                   rc->rmt_feats.tg_feat_flag);
        break;
    }
    case XR_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT: {
        BTMG_DEBUG("remote rn_cap: count %d, bitmask 0x%x", rc->get_rn_caps_rsp.cap_count,
                   rc->get_rn_caps_rsp.evt_set.bits);
        s_avrc_peer_rn_cap.bits = rc->get_rn_caps_rsp.evt_set.bits;
        if (bt_pro_info->a2dp_sink_enabled == true) {
            bt_av_reg_new_track();
            bt_av_reg_playback_changed();
            bt_av_reg_play_pos_changed();
        } else if (bt_pro_info->a2dp_source_enabled == true) {
            bt_av_reg_volume_changed(); //作为source时只注册绝对音量变化
        }
        break;
    }
    case XR_AVRC_CT_SET_ABSOLUTE_VOLUME_RSP_EVT: { //作为source时发送绝对音量命令后，对方给的回复
        unsigned int value = rc->set_volume_rsp.volume * 100 / 0x7f;
        BTMG_WARNG("AVRC set_volume_rsp:%d", value);
        set_system_abs_vol(value);
        break;
    }
    default:
        BTMG_ERROR("unhandled evt %d", event);
        break;
    }
}

static void handle_passthrough_event(uint8_t key_code, uint8_t key_state)
{
    if (!key_state)
        return;

    if (!btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_tg_play_state_cb == NULL) {
        return;
    }

    switch (key_code) {
        case BTMG_AVRC_PT_CMD_PLAY:
            btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_tg_play_state_cb(bda_str, BTMG_AVRCP_PLAYSTATE_PLAYING);
            break;
        case BTMG_AVRC_PT_CMD_STOP:
            btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_tg_play_state_cb(bda_str, BTMG_AVRCP_PLAYSTATE_STOPPED);
            break;
        case BTMG_AVRC_PT_CMD_PAUSE:
            btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_tg_play_state_cb(bda_str, BTMG_AVRCP_PLAYSTATE_PAUSED);
            break;
        case BTMG_AVRC_PT_CMD_FORWARD:
            btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_tg_play_state_cb(bda_str, BTMG_AVRCP_PLAYSTATE_FORWARD);
            break;
        case BTMG_AVRC_PT_CMD_BACKWARD:
            btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_tg_play_state_cb(bda_str, BTMG_AVRCP_PLAYSTATE_BACKWARD);
            break;
        default:
            break;
    }
}

static void bt_avrc_tg_cb(xr_avrc_tg_cb_event_t event, xr_avrc_tg_cb_param_t *rc)
{
    switch (event) {
    case XR_AVRC_TG_CONNECTION_STATE_EVT: {
        bda2str(rc->conn_stat.remote_bda, bda_str);
        if (rc->conn_stat.connected) {
            BTMG_INFO("avrcp tg Connected, dev:%s", bda_str);
        } else {
            BTMG_INFO("avrcp tg Disconnected, dev:%s", bda_str);
        }
        break;
    }
    case XR_AVRC_TG_PASSTHROUGH_CMD_EVT: {
        BTMG_INFO("AVRC passthrough cmd: key_code 0x%x, key_state %d", rc->psth_cmd.key_code,
                  rc->psth_cmd.key_state);
        handle_passthrough_event(rc->psth_cmd.key_code, rc->psth_cmd.key_state);
        break;
    }
    case XR_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT: { //接收到source端控制的绝对音量命令
        if (rc->set_abs_vol.volume != s_volume) {
            if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_audio_volume_cb) {
                btmg_cb_p[CB_MAIN]->btmg_avrcp_cb.avrcp_audio_volume_cb(
                        bda_str, (uint32_t)rc->set_abs_vol.volume * 100 / 0x7f);
            }
            s_volume = rc->set_abs_vol.volume;
        }
        break;
    }
    case XR_AVRC_TG_REGISTER_NOTIFICATION_EVT: { //接收到对端发来的注册，目前只处理了绝对音量的注册
        BTMG_DEBUG("AVRC register event notification: %d, param: 0x%x", rc->reg_ntf.event_id,
                   rc->reg_ntf.event_parameter);
        if (rc->reg_ntf.event_id == XR_AVRC_RN_VOLUME_CHANGE) {
            int get_volume = get_system_abs_vol();
            BTMG_DEBUG("get system abs vol: %d%%", get_volume);
            xr_avrc_rn_param_t rn_param;
            rn_param.volume = get_volume * 0x7F / 100;
            xr_avrc_tg_send_rn_rsp(XR_AVRC_RN_VOLUME_CHANGE, XR_AVRC_RN_RSP_INTERIM, &rn_param);
        }
        break;
    }
    case XR_AVRC_TG_REMOTE_FEATURES_EVT: {
        BTMG_DEBUG("AVRC remote features %x, CT features %x", rc->rmt_feats.feat_mask,
                   rc->rmt_feats.ct_feat_flag);
        break;
    }
    default:
        BTMG_ERROR("unhandled evt %d", event);
        break;
    }
}

btmg_err bt_avrc_init(int a2dp_type)
{
    xr_err_t ret;
    xr_avrc_psth_bit_mask_t cmd_set = {
        0x0000, /* bit mask: 0=SELECT, 1=UP, 2=DOWN, 3=LEFT,
                             4=RIGHT, 5=RIGHT_UP, 6=RIGHT_DOWN, 7=LEFT_UP,
                             8=LEFT_DOWN, 9=ROOT_MENU, 10=SETUP_MENU, 11=CONT_MENU,
                             12=FAV_MENU, 13=EXIT */
        0x0000, /* not used */
        0x1FFF, /* bit mask: 0=0, 1=1, 2=2, 3=3,
                             4=4, 5=5, 6=6, 7=7,
                             8=8, 9=9, 10=DOT, 11=ENTER,
                             12=CLEAR */
        0x0078, /* bit mask: 0=CHAN_UP, 1=CHAN_DOWN, 2=PREV_CHAN, 3=SOUND_SEL,
                             4=INPUT_SEL, 5=DISP_INFO, 6=HELP, 7=PAGE_UP,
                             8=PAGE_DOWN */
        0x1b7F, /* bit mask: 0=POWER, 1=VOL_UP, 2=VOL_DOWN, 3=MUTE,
                             4=PLAY, 5=STOP, 6=PAUSE, 7=RECORD,
                             8=REWIND, 9=FAST_FOR, 10=EJECT, 11=FORWARD,
                             12=BACKWARD */
        0x0000, /* bit mask: 0=ANGLE, 1=SUBPICT */
        0x0000, /* not used */
        0x003E  /* bit mask: 0=not used, 1=F1, 2=F2, 3=F3,
                             4=F4, 5=F5 */
    };

    BTMG_INFO("init avrc, a2dp_type:%d", a2dp_type);

    if ((ret = xr_avrc_ct_init()) != XR_OK) {
        BTMG_ERROR("xr_avrc_ct_init return failed: %d", ret);
        return BT_FAIL;
    }
    if ((ret = xr_avrc_ct_register_callback(bt_avrc_ct_cb)) != XR_OK) {
        BTMG_ERROR("xr_avrc_ct_init return failed: %d", ret);
        return BT_FAIL;
    }

    if ((ret = xr_avrc_tg_init()) != XR_OK) {
        BTMG_ERROR("xr_avrc_ct_init return failed: %d", ret);
        return BT_FAIL;
    }

    if ((ret = xr_avrc_tg_register_callback(bt_avrc_tg_cb)) != XR_OK) {
        BTMG_ERROR("xr_avrc_ct_init return failed: %d", ret);
        return BT_FAIL;
    }

    if (bt_pro_info->a2dp_source_enabled == true) {
        if ((ret = xr_avrc_tg_set_psth_cmd_filter(
                    XR_AVRC_PSTH_FILTER_SUPPORTED_CMD, &cmd_set)) != XR_OK) {
            BTMG_ERROR("set psth cmd ret failed: %d", ret);
            return BT_FAIL;
        }
    }

    if (bt_pro_info->a2dp_sink_enabled == true) {
        xr_avrc_rn_evt_cap_mask_t evt_set = { 0 };
        xr_avrc_rn_evt_bit_mask_operation(XR_AVRC_BIT_MASK_OP_SET, &evt_set,
                                          XR_AVRC_RN_VOLUME_CHANGE);
        if ((ret = xr_avrc_tg_set_rn_evt_cap(&evt_set)) != XR_OK) {
            BTMG_ERROR("xr_avrc_tg_set_rn_evt_cap return failed: %d", ret);
            return BT_FAIL;
        }
    }
    return BT_OK;
}

btmg_err bt_avrc_deinit(int a2dp_type)
{
    xr_err_t ret;

    if ((ret = xr_avrc_ct_deinit()) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }
    if ((ret = xr_avrc_tg_deinit()) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }
    return BT_OK;
}

btmg_err bt_avrc_ct_send_passthrough_cmd(uint8_t key_code)
{
    xr_err_t ret;
    uint8_t tl = 0;
    uint8_t key_state = XR_AVRC_PT_CMD_STATE_PRESSED;

    if ((ret = xr_avrc_ct_send_passthrough_cmd(tl, key_code, key_state)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    key_state = XR_AVRC_PT_CMD_STATE_RELEASED;
    if ((ret = xr_avrc_ct_send_passthrough_cmd(tl, key_code, key_state)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }
    return BT_OK;
}

btmg_err bt_avrc_set_absolute_volume(uint32_t volume)
{
    xr_err_t ret;
    uint8_t tl = 0;

    if (bt_pro_info->a2dp_source_enabled == true) {
        if ((ret = xr_avrc_ct_send_set_absolute_volume_cmd(tl, volume * 0x7F / 100 + 1)) != XR_OK) {
            BTMG_ERROR("return failed: %d", ret);
            return BT_FAIL;
        }
    } else if (bt_pro_info->a2dp_sink_enabled == true) {
        set_system_abs_vol(volume);
        xr_avrc_rn_param_t rn_param;
        rn_param.volume = volume * 0x7F / 100 + 1;
        xr_avrc_tg_send_rn_rsp(XR_AVRC_RN_VOLUME_CHANGE, XR_AVRC_RN_RSP_CHANGED, &rn_param);
    }
    return BT_OK;
}

btmg_err bt_avrc_get_absolute_volume(uint32_t *value)
{
    int ret;

    ret = get_system_abs_vol();
    if (ret < 0) {
        return BT_FAIL;
    }
    *value = (uint32_t)ret;
    return BT_OK;
}
