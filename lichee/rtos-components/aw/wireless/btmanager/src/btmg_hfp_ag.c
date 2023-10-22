#include "btmg_log.h"
#include "bt_manager.h"
#include "btmg_audio.h"
#include "btmg_common.h"
#include "xr_hf_ag_api.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "xr_log.h"
#include "xr_bt_main.h"
#include "xr_bt_device.h"
#include "xr_gap_bt_api.h"
#include "time.h"
#include "sys/time.h"
#include "cmd_util.h"
#include <math.h>

static const char *c_hf_evt_str[] = {
	"CONNECTION_STATE_EVT",              /*!< SERVICE LEVEL CONNECTION STATE CONTROL */
	"AUDIO_STATE_EVT",                   /*!< AUDIO CONNECTION STATE CONTROL */
	"VR_STATE_CHANGE_EVT",               /*!< VOICE RECOGNITION CHANGE */
	"VOLUME_CONTROL_EVT",                /*!< AUDIO VOLUME CONTROL */
	"UNKNOW_AT_CMD",                     /*!< UNKNOW AT COMMAND RECIEVED */
	"IND_UPDATE",                        /*!< INDICATION UPDATE */
	"CIND_RESPONSE_EVT",                 /*!< CALL & DEVICE INDICATION */
	"COPS_RESPONSE_EVT",                 /*!< CURRENT OPERATOR EVENT */
	"CLCC_RESPONSE_EVT",                 /*!< LIST OF CURRENT CALL EVENT */
	"CNUM_RESPONSE_EVT",                 /*!< SUBSCRIBER INFORTMATION OF CALL EVENT */
	"DTMF_RESPONSE_EVT",                 /*!< DTMF TRANSFER EVT */
	"NREC_RESPONSE_EVT",                 /*!< NREC RESPONSE EVT */
	"ANSWER_INCOMING_EVT",               /*!< ANSWER INCOMING EVT */
	"REJECT_INCOMING_EVT",               /*!< AREJECT INCOMING EVT */
	"DIAL_EVT",                          /*!< DIAL INCOMING EVT */
	"WBS_EVT",                           /*!< CURRENT CODEC EVT */
	"BCS_EVT",                           /*!< CODEC NEGO EVT */
};

//xr_hf_connection_state_t
static const char *c_connection_state_str[] = {
	"DISCONNECTED",
	"CONNECTING",
	"CONNECTED",
	"SLC_CONNECTED",
	"DISCONNECTING",
};

// xr_hf_audio_state_t
static const char *c_audio_state_str[] = {
	"disconnected",
	"connecting",
	"connected",
	"connected_msbc",
};

// xr_hf_vr_state_t
static const char *c_vr_state_str[] = {
	"Disabled",
	"Enabled",
};

// xr_hf_nrec_t
static const char *c_nrec_status_str[] = {
	"NREC DISABLE",
	"NREC ABLE",
};

// xr_hf_control_target_t
static const char *c_volume_control_target_str[] = {
	"SPEAKER",
	"MICROPHONE",
};

// xr_hf_subscriber_service_type_t
static const char *c_operator_name_str[] = {
	"中国移动",
	"中国联通",
	"中国电信",
};

// xr_hf_subscriber_service_type_t
static const char *c_subscriber_service_type_str[] = {
	"UNKNOWN",
	"VOICE",
	"FAX",
};

// xr_hf_nego_codec_status_t
static const char *c_codec_mode_str[] = {
	"CVSD Only",
	"Use CVSD",
	"Use MSBC",
};

#if CONFIG_BT_HFP_AUDIO_DATA_PATH_HCI
// Produce a sine audio
static uint32_t bt_app_hf_outgoing_cb(uint8_t *p_buf, uint32_t sz)
{
    if (btmg_cb_p[CB_MAIN]->btmg_hfp_ag_cb.audio_outgoing_cb)
        return btmg_cb_p[CB_MAIN]->btmg_hfp_ag_cb.audio_outgoing_cb(p_buf, sz);
}

static void bt_app_hf_incoming_cb(const uint8_t *buf, uint32_t sz)
{
    if (btmg_cb_p[CB_MAIN]->btmg_hfp_ag_cb.audio_incoming_cb)
        btmg_cb_p[CB_MAIN]->btmg_hfp_ag_cb.audio_incoming_cb(buf, sz);
	xr_hf_outgoing_data_ready();
}
#endif /* #if CONFIG_BT_HFP_AUDIO_DATA_PATH_HCI */

void hfp_hf_ag_event_ind(btmg_hfp_ag_event_t event, void *data)
{
    if (btmg_cb_p[CB_MAIN]->btmg_hfp_ag_cb.event_cb)
        btmg_cb_p[CB_MAIN]->btmg_hfp_ag_cb.event_cb(event, data);
}

uint32_t peer_feat; /*!< AG supported features */
uint32_t chld_feat; /*!< AG supported features on call hold and multiparty services */

/* callback for hfp ag */

static bool is_device_connected = false;
static bool is_ag_sco_connected = false;

static xr_bd_addr_t hf_peer_addr;
static void bt_app_hf_cb(xr_hf_cb_event_t event, xr_hf_cb_param_t *param)
{
	if (event <= XR_HF_BCS_RESPONSE_EVT) {
		BTMG_DEBUG("APP HFP event: %s\n", c_hf_evt_str[event]);
	} else {
		CMD_ERR("APP HFP invalid event %d\n", event);
	}

	switch (event) {
	case XR_HF_CONNECTION_STATE_EVT: {
        dev_node_t *dev_node = NULL;
        bda2str(param->conn_stat.remote_bda, bda_str);
		BTMG_DEBUG("HF connection state %s, peer feats 0x%x, chld_feats 0x%x, dev:%s\n",
		    c_connection_state_str[param->conn_stat.state],
		    param->conn_stat.peer_feat,
		    param->conn_stat.chld_feat, bda_str);
		memcpy(hf_peer_addr, param->conn_stat.remote_bda, XR_BD_ADDR_LEN);

        if (param->conn_stat.state == XR_HF_CONNECTION_STATE_CONNECTED) {
            is_device_connected = true;
            dev_node = btmg_dev_list_find_device(connected_devices, bda_str);
            if (dev_node == NULL) {
                btmg_dev_list_add_device(connected_devices, NULL, bda_str, HFP_AG_DEV);
            } else {
                dev_node->profile |= HFP_AG_DEV;
            }
            BTMG_DEBUG("add device %s into connected_devices", bda_str);
        } else if (param->conn_stat.state == XR_HF_CONNECTION_STATE_DISCONNECTED) {
            if (connected_devices->sem_flag) {
                XR_OS_SemaphoreRelease(&(connected_devices->sem));
            }
            is_device_connected = false;
            dev_node = btmg_dev_list_find_device(connected_devices, bda_str);
            if (dev_node != NULL) {
                BTMG_DEBUG("remove device %s from connected_devices", bda_str);
                dev_node->profile &= ~HFP_AG_DEV;
                btmg_dev_list_remove_device(connected_devices, bda_str);
            }
        } else if (param->conn_stat.state == XR_HF_CONNECTION_STATE_DISCONNECTING) {
            sys_handler_send(bt_audio_event_handle, BT_AUDIO_EVENT_HFP_STOP, 1);
        }

        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_hfp_hf_cb.conn_state_cb) {
            btmg_cb_p[CB_MAIN]->btmg_hfp_hf_cb.conn_state_cb(
                    bda_str, (btmg_hfp_hf_connection_state_t)param->conn_stat.state);
        }
		btmg_ag_conn_stat_t conn_stat;
		conn_stat.state = param->conn_stat.state;
		conn_stat.peer_feat = param->conn_stat.peer_feat;
		conn_stat.chld_feat = param->conn_stat.chld_feat;
		hfp_hf_ag_event_ind(BTMG_HFP_AG_CONNECTION_STATE_EVT, &conn_stat);
		break;
	}

	case XR_HF_AUDIO_STATE_EVT: {
		BTMG_DEBUG("HF Audio State %s\n", c_audio_state_str[param->audio_stat.state]);
#if CONFIG_BT_HFP_AUDIO_DATA_PATH_HCI
		if (param->audio_stat.state == XR_HF_AUDIO_STATE_CONNECTED ||
		    param->audio_stat.state == XR_HF_AUDIO_STATE_CONNECTED_MSBC) {
			is_ag_sco_connected = true;
			xr_bt_hf_register_data_callback(bt_app_hf_incoming_cb, bt_app_hf_outgoing_cb);
		} else if (param->audio_stat.state == XR_HF_AUDIO_STATE_DISCONNECTED) {
			is_ag_sco_connected = false;
			BTMG_DEBUG("HF Audio Connection Disconnected.\n");
		}
#endif /* #if CONFIG_BT_HFP_AUDIO_DATA_PATH_HCI */
		btmg_ag_audio_stat_t audio_stat;
		audio_stat.state = param->audio_stat.state;
		hfp_hf_ag_event_ind(BTMG_HFP_AG_AUDIO_STATE_EVT, &audio_stat);
		break;
	}

	case XR_HF_BVRA_RESPONSE_EVT: {
		BTMG_DEBUG("HF Voice Recognition is %s\n", c_vr_state_str[param->vra_rep.value]);
		btmg_ag_vra_rep_t vra_rep;
		vra_rep.value = param->vra_rep.value;
		hfp_hf_ag_event_ind(BTMG_HFP_AG_BVRA_RESPONSE_EVT, &vra_rep);
		break;
	}

	case XR_HF_VOLUME_CONTROL_EVT: {
		BTMG_DEBUG("HF Volume Target: %s, Volume %d\n", c_volume_control_target_str[param->volume_control.type], param->volume_control.volume);
		btmg_ag_volume_control_t volume_control;
		volume_control.type = param->volume_control.type;
		volume_control.volume = param->volume_control.volume;
		hfp_hf_ag_event_ind(BTMG_HFP_AG_VOLUME_CONTROL_EVT, &volume_control);
		break;
	}

	case XR_HF_UNAT_RESPONSE_EVT: {
		BTMG_DEBUG("HF UNKOW AT CMD: %s\n", param->unat_rep.unat);
		xr_hf_unat_response(hf_peer_addr, NULL);
		btmg_ag_unat_rep_t unat_rep;
		unat_rep.unat = param->unat_rep.unat;
		hfp_hf_ag_event_ind(BTMG_HFP_AG_UNAT_RESPONSE_EVT, &unat_rep);
		break;
	}

	case XR_HF_IND_UPDATE_EVT: {
		BTMG_DEBUG("HF UPDATE INDCATOR!\n");
		xr_hf_call_status_t call_state = 1;
		xr_hf_call_setup_status_t call_setup_state = 2;
		xr_hf_network_state_t ntk_state = 1;
		int signal = 2;
		xr_bt_hf_indchange_notification(hf_peer_addr, call_state, call_setup_state, ntk_state, signal);
		// hfp_hf_ag_event_ind(BTMG_HFP_AG_IND_UPDATE_EVT,);
		break;
	}

	case XR_HF_CIND_RESPONSE_EVT: {
		BTMG_DEBUG("HF CIND Start.\n");
		btmg_ag_cind_t cind = {0};
		hfp_hf_ag_event_ind(BTMG_HFP_AG_CIND_RESPONSE_EVT, &cind);
		xr_bt_hf_cind_response(hf_peer_addr,
								cind.call_status,
								cind.call_setup_status,
								cind.svc,
								cind.signal_strength,
								cind.roam,
								cind.battery_level,
								cind.call_held_status);
		break;
	}

	case XR_HF_COPS_RESPONSE_EVT: {
		const int svc_type = 1;
		xr_bt_hf_cops_response(hf_peer_addr, (char *)c_operator_name_str[svc_type]);
		break;
	}

	case XR_HF_CLCC_RESPONSE_EVT: {
		int index = 1;
		//mandatory
		xr_hf_current_call_direction_t dir = 1;
		xr_hf_current_call_status_t current_call_status = 0;
		xr_hf_current_call_mode_t mode = 0;
		xr_hf_current_call_mpty_type_t mpty = 0;
		//option
		char *number = {"123456"};
		xr_hf_call_addr_type_t type = XR_HF_CALL_ADDR_TYPE_UNKNOWN;

		BTMG_DEBUG("HF Calling Line Identification.\n");
		xr_bt_hf_clcc_response(hf_peer_addr, index, dir, current_call_status, mode, mpty, number, type);
		break;
	}

	case XR_HF_CNUM_RESPONSE_EVT: {
		char *number = {"123456"};
		xr_hf_subscriber_service_type_t type = 1;
		BTMG_DEBUG("HF Current Number is %s ,Type is %s.\n", number, c_subscriber_service_type_str[type]);
		xr_bt_hf_cnum_response(hf_peer_addr, number, type);
		break;
	}

	case XR_HF_VTS_RESPONSE_EVT: {
		BTMG_DEBUG("HF DTMF code is: %s.\n", param->vts_rep.code);
		btmg_ag_vts_rep_t vts_rep;
		vts_rep.code = param->vts_rep.code;
		hfp_hf_ag_event_ind(BTMG_HFP_AG_VTS_RESPONSE_EVT, &vts_rep);
		break;
	}

	case XR_HF_NREC_RESPONSE_EVT: {
		BTMG_DEBUG("HF NREC status is: %s.\n", c_nrec_status_str[param->nrec.state]);
		memset(&param, 0, sizeof(xr_hf_cb_param_t));
		btmg_ag_nrec_t nrec;
		nrec.state = param->nrec.state;
		hfp_hf_ag_event_ind(BTMG_HFP_AG_NREC_RESPONSE_EVT, &nrec);
		break;
	}

	case XR_HF_ATA_RESPONSE_EVT: {
		BTMG_DEBUG("HF Asnwer Incoming Call.\n");
		char number[32] = { 0 };
		hfp_hf_ag_event_ind(BTMG_HFP_AG_ATA_RESPONSE_EVT, &number);
		xr_bt_hf_answer_call(hf_peer_addr, 1, 0, 1, 0, number, 0);
		break;
	}

	case XR_HF_CHUP_RESPONSE_EVT: {
		BTMG_DEBUG("HF Reject Incoming Call.");
		char number[32] = { 0 };
		hfp_hf_ag_event_ind(BTMG_HFP_AG_ATA_RESPONSE_EVT, &number);
		xr_bt_hf_reject_call(hf_peer_addr, 0, 0, 0, 0, number, 0);
		break;
	}

	case XR_HF_DIAL_EVT: {
		if (param->out_call.num_or_loc) {
			//dia_num_or_mem
			BTMG_DEBUG("HF Dial \"%s\".\n", param->out_call.num_or_loc);
			xr_bt_hf_out_call(hf_peer_addr, 1, 0, 1, 0, param->out_call.num_or_loc, 0);
		} else {
			//dia_last
			BTMG_DEBUG("HF Dial last number.\n");
		}
		btmg_ag_out_call_t out_call;
		out_call.num_or_loc = param->out_call.num_or_loc;
		hfp_hf_ag_event_ind(BTMG_HFP_AG_DIAL_EVT, &out_call);
		break;
	}
#if (BTM_WBS_INCLUDED == TRUE)
	case XR_HF_WBS_RESPONSE_EVT: {
		BTMG_DEBUG("HF Current codec: %s\n",c_codec_mode_str[param->wbs_rep.codec]);
		btmg_ag_wbs_rep_t wbs_rep;
		wbs_rep.codec = param->wbs_rep.codec;
		hfp_hf_ag_event_ind(BTMG_HFP_AG_WBS_RESPONSE_EVT, &wbs_rep);
		break;
	}
#endif
	case XR_HF_BCS_RESPONSE_EVT: {
		BTMG_DEBUG("HF Consequence of codec negotiation: %s\n",c_codec_mode_str[param->bcs_rep.mode]);
		btmg_ag_bcs_rep_t bcs_rep;
        bcs_rep.mode = param->bcs_rep.mode;
		hfp_hf_ag_event_ind(BTMG_HFP_AG_BCS_RESPONSE_EVT, &bcs_rep);
		break;
	}

	default:
		BTMG_DEBUG("Unsupported HF_AG EVT: %d.\n", event);
		break;
	}
}

btmg_err bt_hfp_ag_init(void)
{
    xr_err_t ret;
	xr_bd_addr_t remote_addr={ 0x00 };

    if ((ret = xr_bt_hf_register_callback(bt_app_hf_cb)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

	if ((ret = xr_bt_hf_init(remote_addr)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    bt_audio_init(BT_AUDIO_TYPE_HFP, 0);

    return BT_OK;
}

btmg_err bt_hfp_ag_deinit(void)
{
    xr_err_t ret;
	xr_bd_addr_t remote_addr = { 0 };

	if ((ret = xr_bt_hf_deinit(remote_addr)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_ag_connect(const char *addr)
{
	xr_err_t ret;
    xr_bd_addr_t remote_bda = { 0 };

    str2bda(addr, remote_bda);

	if ((ret = xr_bt_hf_connect(remote_bda)) != XR_OK) {
		CMD_ERR("xr_bt_hf_connect failed: %d\n", ret);
		return BT_FAIL;
	}
	return BT_OK;
}

btmg_err bt_hfp_ag_disconnect(const char *addr)
{
    xr_err_t ret;
    xr_bd_addr_t remote_bda = { 0 };

    str2bda(addr, remote_bda);

    if ((ret = xr_bt_hf_disconnect(remote_bda)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_ag_connect_audio(const char *addr)
{
    xr_err_t ret;
    xr_bd_addr_t remote_bda = { 0 };

    str2bda(addr, remote_bda);

    if ((ret = xr_bt_hf_connect_audio(remote_bda)) != XR_OK) {
        BTMG_ERROR("return failed: %d\n", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_ag_disconnect_audio(const char *addr)
{
    if (!is_ag_sco_connected) {
        return BT_ERR_NOT_CONNECTED;
    }

    xr_err_t ret;
    xr_bd_addr_t remote_bda = { 0 };

    str2bda(addr, remote_bda);

    if ((ret = xr_bt_hf_disconnect_audio(remote_bda)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}

btmg_err bt_hfp_ag_spk_vol_update(const char *addr, int volume)
{
    xr_err_t ret;
    xr_bd_addr_t remote_bda = { 0 };

    str2bda(addr, remote_bda);
    if ((ret = xr_bt_hf_volume_control(remote_bda, XR_HF_VOLUME_CONTROL_TARGET_SPK, volume)) != XR_OK) {
        BTMG_ERROR("return failed: %d", ret);
        return BT_FAIL;
    }

    return BT_OK;
}
