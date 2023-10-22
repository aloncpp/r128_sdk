#include "btmg_log.h"
#include "bt_manager.h"
#include "btmg_audio.h"
#include "btmg_common.h"
#include "xr_hf_client_api.h"

static bool is_device_connected = false;
static bool is_hf_sco_connected = false;
static uint32_t bt_hfp_hf_outgoing_cb(uint8_t *buf, uint32_t len)
{
    if (bt_audio_read_unblock((uint8_t *)buf, len, BT_AUDIO_TYPE_HFP) == len) {
        return len;
    }

    return 0;
}

static void bt_hfp_hf_incoming_cb(const uint8_t *buf, uint32_t len)
{
    bt_audio_write_unblock((uint8_t *)buf, len, BT_AUDIO_TYPE_HFP);
    xr_hf_client_outgoing_data_ready();
}

void hfp_hf_event_ind(btmg_hfp_hf_event_t event, void *data)
{
    if (btmg_cb_p[CB_MAIN]->btmg_hfp_hf_cb.event_cb)
        btmg_cb_p[CB_MAIN]->btmg_hfp_hf_cb.event_cb(event, data);
}

uint32_t peer_feat; /*!< AG supported features */
uint32_t chld_feat; /*!< AG supported features on call hold and multiparty services */

/* callback for hfp hf */
static void bt_hfp_hf_cb(xr_hf_client_cb_event_t event, xr_hf_client_cb_param_t *param)
{
    BTMG_INFO("hfp hf event %d", event);
    dev_node_t *dev_node = NULL;

    switch (event) {
    case XR_HF_CLIENT_CONNECTION_STATE_EVT: {
        bda2str(param->conn_stat.remote_bda, bda_str);
        BTMG_DEBUG("XR_HF_CLIENT_CONNECTION_STATE_EVT: con-state(%d), peer(%d), chld(%d), dev:%s",
                   param->conn_stat.state, param->conn_stat.peer_feat, param->conn_stat.chld_feat,
                   bda_str);
        peer_feat = param->conn_stat.peer_feat;
        chld_feat = param->conn_stat.chld_feat;

        if (param->conn_stat.state == XR_HF_CLIENT_CONNECTION_STATE_CONNECTED) {
            is_device_connected = true;
            dev_node = btmg_dev_list_find_device(connected_devices, bda_str);
            if (dev_node == NULL) {
                btmg_dev_list_add_device(connected_devices, NULL, bda_str, HFP_HF_DEV);
            } else {
                dev_node->profile |= HFP_HF_DEV;
            }
            BTMG_DEBUG("add device %s into connected_devices", bda_str);
        } else if (param->conn_stat.state == XR_HF_CLIENT_CONNECTION_STATE_DISCONNECTED) {
            if (connected_devices->sem_flag) {
                XR_OS_SemaphoreRelease(&(connected_devices->sem));
            }
            is_device_connected = false;
            dev_node = btmg_dev_list_find_device(connected_devices, bda_str);
            if (dev_node != NULL) {
                dev_node->profile &= ~HFP_HF_DEV;
                btmg_dev_list_remove_device(connected_devices, bda_str);
            }
            BTMG_DEBUG("remove device %s from connected_devices", bda_str);
        } else if (param->conn_stat.state == XR_HF_CLIENT_CONNECTION_STATE_DISCONNECTING) {
            sys_handler_send(bt_audio_event_handle, BT_AUDIO_EVENT_HFP_STOP, 1);
        }

        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_hfp_hf_cb.conn_state_cb) {
            btmg_cb_p[CB_MAIN]->btmg_hfp_hf_cb.conn_state_cb(
                    bda_str, (btmg_hfp_hf_connection_state_t)param->conn_stat.state);
        }
        break;
    }
    case XR_HF_CLIENT_AUDIO_STATE_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_AUDIO_STATE_EVT: audio-state(%d), " XR_BD_ADDR_STR,
                   param->audio_stat.state, XR_BD_ADDR_HEX(param->audio_stat.remote_bda));
        if (param->audio_stat.state == XR_HF_CLIENT_AUDIO_STATE_CONNECTED ||
            param->audio_stat.state == XR_HF_CLIENT_AUDIO_STATE_CONNECTED_MSBC) {
            is_hf_sco_connected = true;
            sys_handler_send(bt_audio_event_handle, BT_AUDIO_EVENT_HFP_START, 1);
            xr_hf_client_register_data_callback(bt_hfp_hf_incoming_cb, bt_hfp_hf_outgoing_cb);
        } else if (param->audio_stat.state == XR_HF_CLIENT_AUDIO_STATE_DISCONNECTED) {
            is_hf_sco_connected = false;
            sys_handler_send(bt_audio_event_handle, BT_AUDIO_EVENT_HFP_STOP, 1);
            if (connected_devices->sem_flag) {
                XR_OS_SemaphoreRelease(&(connected_devices->sem));
            }
        }
        break;
    case XR_HF_CLIENT_BVRA_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_BVRA_EVT: recog-state(%d)", param->bvra.value);
        btmg_hf_bvra_t hf_bvra;
        hf_bvra.value = param->bvra.value;
        hfp_hf_event_ind(BTMG_HFP_HF_BVRA_EVT, &hf_bvra);
        break;
    case XR_HF_CLIENT_CIND_CALL_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_CIND_CALL_EVT: call-state(%d)", param->call.status);
        btmg_hf_call_ind_t call_ind;
        call_ind.status = param->call.status;
        hfp_hf_event_ind(BTMG_HFP_HF_CIND_CALL_EVT, &call_ind);
        break;
    case XR_HF_CLIENT_CIND_CALL_SETUP_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_CIND_CALL_SETUP_EVT: call-setup-state(%d)",
                   param->call_setup.status);
        btmg_hf_call_setup_ind_t call_setup_ind;
        call_setup_ind.status = param->call_setup.status;
        hfp_hf_event_ind(BTMG_HFP_HF_CIND_CALL_SETUP_EVT, &call_setup_ind);
        break;
    case XR_HF_CLIENT_CIND_CALL_HELD_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_CIND_CALL_HELD_EVT: held-state(%d)", param->call_held.status);
        btmg_hf_call_held_ind_t call_held_ind;
        call_held_ind.status = param->call_held.status;
        hfp_hf_event_ind(BTMG_HFP_HF_CIND_CALL_HELD_EVT, &call_held_ind);
        break;
    case XR_HF_CLIENT_CIND_SERVICE_AVAILABILITY_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_CIND_SERVICE_AVAILABILITY_EVT: service-avail(%d)",
                   param->service_availability.status);
        btmg_hf_service_availability_t service_availability;
        service_availability.status = param->service_availability.status;
        hfp_hf_event_ind(BTMG_HFP_HF_CIND_SERVICE_AVAILABILITY_EVT, &service_availability);
        break;
    case XR_HF_CLIENT_CIND_SIGNAL_STRENGTH_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_CIND_SIGNAL_STRENGTH_EVT: signal_strength(%d)",
                   param->signal_strength.value);
        btmg_hf_signal_strength_ind_t signal_strength_ind;
        signal_strength_ind.value = param->signal_strength.value;
        hfp_hf_event_ind(BTMG_HFP_HF_CIND_SIGNAL_STRENGTH_EVT, &signal_strength_ind);
        break;
    case XR_HF_CLIENT_CIND_ROAMING_STATUS_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_CIND_ROAMING_STATUS_EVT: roaming(%d)", param->roaming.status);
        btmg_hf_network_roaming_t network_roaming;
        network_roaming.status = param->roaming.status;
        hfp_hf_event_ind(BTMG_HFP_HF_CIND_ROAMING_STATUS_EVT, &network_roaming);
        break;
    case XR_HF_CLIENT_CIND_BATTERY_LEVEL_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_CIND_BATTERY_LEVEL_EVT: battery_level(%d)",
                   param->battery_level.value);
        btmg_hf_battery_level_ind_t battery_level_ind;
        battery_level_ind.value = param->battery_level.value;
        hfp_hf_event_ind(BTMG_HFP_HF_CIND_BATTERY_LEVEL_EVT, &battery_level_ind);
        break;
    case XR_HF_CLIENT_COPS_CURRENT_OPERATOR_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_COPS_CURRENT_OPERATOR_EVT: cops(%s)", param->cops.name);
        btmg_hf_current_operator_t current_operator;
        current_operator.name = param->cops.name;
        hfp_hf_event_ind(BTMG_HFP_HF_COPS_CURRENT_OPERATOR_EVT, &current_operator);
        break;
    case XR_HF_CLIENT_BTRH_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_BTRH_EVT: btrh(%d)", param->btrh.status);
        btmg_hf_btrh_t btrh;
        btrh.status = param->btrh.status;
        hfp_hf_event_ind(BTMG_HFP_HF_BTRH_EVT, &btrh);
        break;
    case XR_HF_CLIENT_CLIP_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_CLIP_EVT: number(%s)", param->clip.number);
        btmg_hf_clip_t clip;
        clip.number = param->clip.number;
        hfp_hf_event_ind(BTMG_HFP_HF_CLIP_EVT, &clip);
        break;
    case XR_HF_CLIENT_CCWA_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_CCWA_EVT: number(%s)", param->ccwa.number);
        btmg_hf_ccwa_t ccwa;
        ccwa.number = param->ccwa.number;
        hfp_hf_event_ind(BTMG_HFP_HF_CCWA_EVT, &ccwa);
        break;
    case XR_HF_CLIENT_CLCC_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_CLCC_EVT: idx(%d), dir(%d), status(%d), mpty(%d), number(%s)",
                   param->clcc.idx, param->clcc.dir, param->clcc.status, param->clcc.mpty,
                   param->clcc.number);
        btmg_hf_clcc_t clcc;
        clcc.idx = param->clcc.idx;
        clcc.dir = param->clcc.dir;
        clcc.status = param->clcc.status;
        clcc.mpty = param->clcc.mpty;
        clcc.number = param->clcc.number;
        hfp_hf_event_ind(BTMG_HFP_HF_CLCC_EVT, &clcc);
        break;
    case XR_HF_CLIENT_VOLUME_CONTROL_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_VOLUME_CONTROL_EVT: target(%d), vol(%d)",
                   param->volume_control.type, param->volume_control.volume);
        btmg_hf_volume_control_t volume_control;
        volume_control.type = param->volume_control.type;
        volume_control.volume = param->volume_control.volume;
        hfp_hf_event_ind(BTMG_HFP_HF_VOLUME_CONTROL_EVT, &volume_control);
        break;
    case XR_HF_CLIENT_CNUM_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_CNUM_EVT: number(%s), type(%d)", param->cnum.number,
                   param->cnum.type);
        btmg_hf_cnum_t cnum;
        cnum.number = param->cnum.number;
        cnum.type = param->cnum.type;
        hfp_hf_event_ind(BTMG_HFP_HF_CNUM_EVT, &cnum);
        break;
    case XR_HF_CLIENT_BSIR_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_BSIR_EVT: bsir-provide(%d)", param->bsir.state);
        btmg_hf_bsir_t bsir;
        bsir.state = param->bsir.state;
        hfp_hf_event_ind(BTMG_HFP_HF_BSIR_EVT, &bsir);
        break;
    case XR_HF_CLIENT_BINP_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_BINP_EVT: number(%s)", param->binp.number);
        btmg_hf_binp_t binp;
        binp.number = param->binp.number;
        hfp_hf_event_ind(BTMG_HFP_HF_BINP_EVT, &binp);
        break;
    case XR_HF_CLIENT_RING_IND_EVT:
        BTMG_DEBUG("XR_HF_CLIENT_RING_IND_EVT");
        hfp_hf_event_ind(BTMG_HFP_HF_RING_IND_EVT, NULL);
        break;
    case XR_HF_CLIENT_AT_RESPONSE_EVT:
        BTMG_DEBUG("AT_RESPONSE_OK");
        break;
    default:
        BTMG_DEBUG("Invalid hfp event: %d", event);
        break;
    }
}

btmg_err bt_hfp_hf_init(void)
{
    xr_err_t ret;

    if ((ret = xr_hf_client_register_callback(bt_hfp_hf_cb)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    if ((ret = xr_hf_client_init()) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    bt_audio_init(BT_AUDIO_TYPE_HFP, 0);

    return BT_OK;
}

btmg_err bt_hfp_hf_deinit(void)
{
    xr_err_t ret;

    if ((ret = xr_hf_client_deinit()) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    bt_audio_deinit(BT_AUDIO_TYPE_HFP);

    return BT_OK;
}

//btmg_err bt_hfp_hf_connect(const char *addr);

btmg_err bt_hfp_hf_disconnect(const char *addr)
{
    xr_err_t ret;
    xr_bd_addr_t remote_bda = { 0 };

    str2bda(addr, remote_bda);

    if ((ret = xr_hf_client_disconnect(remote_bda)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_hf_disconnect_audio(const char *addr)
{
    if (!is_hf_sco_connected) {
        return BT_ERR_NOT_CONNECTED;
    }

    xr_err_t ret;
    xr_bd_addr_t remote_bda = { 0 };

    str2bda(addr, remote_bda);

    if ((ret = xr_hf_client_disconnect_audio(remote_bda)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_hf_start_voice_recognition(void)
{
    xr_err_t ret;

    if ((ret = xr_hf_client_start_voice_recognition()) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_hf_stop_voice_recognition(void)
{
    xr_err_t ret;

    if ((ret = xr_hf_client_stop_voice_recognition()) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_hf_spk_vol_update(int volume)
{
    xr_err_t ret;

    if ((ret = xr_hf_client_volume_update(XR_HF_VOLUME_CONTROL_TARGET_SPK, volume)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_hf_mic_vol_update(int volume)
{
#ifdef XRADIO_HFP_ADJUST_VOLUME
    int ret = -1;
    int vol = volume;

    BTMG_INFO("adjusting the volume by ourselves");
    vol = XRADIO_AMIXER_HFP_MAX - (uint32_t)vol * 4;
    BTMG_DEBUG("volume is: %d", vol);
    ret = bluedroid_amixer(vol);
    if (ret != XR_OK) {
        BTMG_ERROR("Set vol by ourself failed");
        return BT_FAIL;
    }
#endif

    return BT_OK;
}

btmg_err bt_hfp_hf_dial(const char *number)
{
    xr_err_t ret;

    if ((ret = xr_hf_client_dial(number)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_hf_dial_memory(int location)
{
    xr_err_t ret;

    if ((ret = xr_hf_client_dial_memory(location)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_hf_send_chld_cmd(btmg_hf_chld_type_t chld, int idx)
{
    xr_err_t ret;

    if (chld == BTMG_HF_CHLD_TYPE_REL_X || chld == BTMG_HF_CHLD_TYPE_PRIV_X) {
        BTMG_ERROR("invalid param");
        return BT_ERR_INVALID_ARG;
    }

    if ((ret = xr_hf_client_send_chld_cmd((xr_hf_chld_type_t)chld, idx)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_hf_send_btrh_cmd(btmg_hf_btrh_cmd_t btrh)
{
    xr_err_t ret;

    if ((ret = xr_hf_client_send_btrh_cmd((xr_hf_btrh_cmd_t)btrh)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_hf_answer_call(void)
{
    xr_err_t ret;

    if ((ret = xr_hf_client_answer_call()) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_hf_reject_call(void)
{
    xr_err_t ret;

    if ((ret = xr_hf_client_reject_call()) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_hf_query_calls(void)
{
    xr_err_t ret;

    if ((ret = xr_hf_client_query_current_calls()) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_hf_query_operator(void)
{
    xr_err_t ret;

    if ((ret = xr_hf_client_query_current_operator_name()) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_hf_query_number(void)
{
    xr_err_t ret;

    if ((ret = xr_hf_client_retrieve_subscriber_info()) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_hf_send_dtmf(char code)
{
    xr_err_t ret;

    if ((ret = xr_hf_client_send_dtmf(code)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_hf_request_last_voice_tag_number(void)
{
    xr_err_t ret;

    if ((ret = xr_hf_client_request_last_voice_tag_number()) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_hf_send_nrec(void)
{
    xr_err_t ret;

    if ((ret = xr_hf_client_request_last_voice_tag_number()) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}
