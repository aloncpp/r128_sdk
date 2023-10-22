/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cmd_util.h"
#include "xr_avrc_api.h"
#include "xr_a2dp_api.h"
#include "xr_log.h"
#include "bt_app_core.h"
#include "bt_app_audio.h"
#include <aw-alsa-lib/control.h>

#define XR_AVRC_MAX_VOLUME 0x7F
#define CLO 0

#define BT_RC_CT_TAG "AVRC_CT "
#define BT_RC_TG_TAG "AVRC_TG "
#define BT_AV_TAG "AVRC_SNK "

#define VOL_ADJ(vol) (vol * 100 / XR_AVRC_MAX_VOLUME)

static xTaskHandle s_vcs_task_hdl = NULL;

#if 1
static const char *s_avrc_conn_state_str[] = {"Disconnected", "Connected"};
#define AVRC_PS_MAX_ATTR_VALUE          4
#define AVRC_PT_MAX_KEY_CODE            57
#define AVRC_INVALID_VALUE              0xFF
#define AVRC_SOURCE                     (0)
#define APP_RC_CT_TL_GET_CAPS            (0)
#define APP_RC_CT_TL_RN_VOLUME_CHANGE    (1)
#define APP_RC_CT_TL_GET_META_DATA       (1)
#define APP_RC_CT_TL_RN_TRACK_CHANGE     (2)
#define APP_RC_CT_TL_RN_PLAYBACK_CHANGE  (3)
#define APP_RC_CT_TL_RN_PLAY_POS_CHANGE  (4)
#define APP_RC_CT_TL_RN_BATTERY_STATUS_CHANGE (5)
static xr_avrc_rn_evt_cap_mask_t s_avrc_peer_rn_cap;
static uint8_t s_volume = 50;
static bool s_volume_notify;
static enum cmd_status cmd_avrc_help_exec(char *cmd);

typedef struct key_value {
	char *key;
	uint8_t value;
} kv;

static void bt_av_new_track(void)
{
	// request metadata
	uint8_t attr_mask = XR_AVRC_MD_ATTR_TITLE | XR_AVRC_MD_ATTR_ARTIST | XR_AVRC_MD_ATTR_ALBUM | XR_AVRC_MD_ATTR_GENRE;
	xr_avrc_ct_send_metadata_cmd(APP_RC_CT_TL_GET_META_DATA, attr_mask);

	// register notification if peer support the event_id
	if (xr_avrc_rn_evt_bit_mask_operation(XR_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
	                                       XR_AVRC_RN_TRACK_CHANGE)) {
		xr_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_TRACK_CHANGE, XR_AVRC_RN_TRACK_CHANGE, 0);
	}
}

static void bt_av_playback_changed(void)
{
	if (xr_avrc_rn_evt_bit_mask_operation(XR_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
	                                       XR_AVRC_RN_PLAY_STATUS_CHANGE)) {
		xr_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_PLAYBACK_CHANGE, XR_AVRC_RN_PLAY_STATUS_CHANGE, 0);
	}
}

static void bt_av_play_pos_changed(void)
{
	if (xr_avrc_rn_evt_bit_mask_operation(XR_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
	                                       XR_AVRC_RN_PLAY_POS_CHANGED)) {
		xr_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_PLAY_POS_CHANGE, XR_AVRC_RN_PLAY_POS_CHANGED, 10);
	}
}

static void bt_av_battery_status_changed(void)
{
	if (xr_avrc_rn_evt_bit_mask_operation(XR_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
	                                       XR_AVRC_RN_BATTERY_STATUS_CHANGE)) {
		xr_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_BATTERY_STATUS_CHANGE, XR_AVRC_RN_BATTERY_STATUS_CHANGE, 10);
	}
}

void bt_av_notify_evt_handler_snk(uint8_t event_id, xr_avrc_rn_param_t *event_parameter)
{
	switch (event_id) {
	case XR_AVRC_RN_TRACK_CHANGE:
		bt_av_new_track();
		break;
	case XR_AVRC_RN_PLAY_STATUS_CHANGE:
		CMD_DBG(BT_AV_TAG "Playback status changed: 0x%x\n", event_parameter->playback);
		bt_av_playback_changed();
		break;
	case XR_AVRC_RN_PLAY_POS_CHANGED:
		CMD_DBG(BT_AV_TAG "Play position changed: %d-ms\n", event_parameter->play_pos);
		bt_av_play_pos_changed();
		break;
	case XR_AVRC_RN_BATTERY_STATUS_CHANGE:
		CMD_DBG(BT_AV_TAG "Remote device battery status: %d\n", event_parameter->batt);
		bt_av_battery_status_changed();
		break;
	}
}

static void bt_av_hdl_avrc_ct_evt_snk(uint16_t event, void *p_param)
{
	CMD_DBG("%s evt %d\n", __func__, event);
	xr_avrc_ct_cb_param_t *rc = (xr_avrc_ct_cb_param_t *)(p_param);
	switch (event) {
	case XR_AVRC_CT_CONNECTION_STATE_EVT: {
		uint8_t *bda = rc->conn_stat.remote_bda;
		CMD_DBG(BT_RC_CT_TAG "AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]\n",
	             rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

		if (rc->conn_stat.connected) {
			// get remote supported event_ids of peer AVRCP Target
			xr_avrc_ct_send_get_rn_capabilities_cmd(APP_RC_CT_TL_GET_CAPS);
	} else {
			// clear peer notification capability record
			s_avrc_peer_rn_cap.bits = 0;
		}
		break;
	}
	case XR_AVRC_CT_PASSTHROUGH_RSP_EVT: {
		CMD_DBG(BT_RC_CT_TAG "AVRC passthrough rsp: key_code 0x%x, key_state %d\n", rc->psth_rsp.key_code, rc->psth_rsp.key_state);
		break;
	}
	case XR_AVRC_CT_METADATA_RSP_EVT: {
		CMD_DBG(BT_RC_CT_TAG "AVRC metadata rsp: attribute id 0x%x, %s\n", rc->meta_rsp.attr_id, rc->meta_rsp.attr_text);
		//free(rc->meta_rsp.attr_text);
		break;
	}
	case XR_AVRC_CT_CHANGE_NOTIFY_EVT: {
		CMD_DBG(BT_RC_CT_TAG "AVRC event notification: %d\n", rc->change_ntf.event_id);
		bt_av_notify_evt_handler_snk(rc->change_ntf.event_id, &rc->change_ntf.event_parameter);
		break;
	}
	case XR_AVRC_CT_REMOTE_FEATURES_EVT: {
		CMD_DBG(BT_RC_CT_TAG "AVRC remote features %x, TG features %x\n", rc->rmt_feats.feat_mask, rc->rmt_feats.tg_feat_flag);
		break;
	}
	case XR_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT: {
		CMD_DBG(BT_RC_CT_TAG "remote rn_cap: count %d, bitmask 0x%x\n", rc->get_rn_caps_rsp.cap_count,
				 rc->get_rn_caps_rsp.evt_set.bits);
		s_avrc_peer_rn_cap.bits = rc->get_rn_caps_rsp.evt_set.bits;
		bt_av_new_track();
		bt_av_playback_changed();
		bt_av_play_pos_changed();
		break;
	}
	case XR_AVRC_CT_SET_ABSOLUTE_VOLUME_RSP_EVT: {
		CMD_DBG(BT_RC_CT_TAG "Set absolute volume rsp: volume %d%%\n", (int)VOL_ADJ(rc->set_volume_rsp.volume));
		s_volume = VOL_ADJ(rc->set_volume_rsp.volume);
		break;
	}
	default:
		CMD_DBG("%s unhandled evt %d\n", __func__, event);
		break;
	}
}

#if (CLO == 1)
static void volume_set_by_controller(uint8_t volume)
{
	CMD_DBG("Volume is set by remote controller %d%%\n", VOL_ADJ(volume));
	//_lock_acquire(&s_volume_lock);
	s_volume = volume;
	//_lock_release(&s_volume_lock);
}

static void volume_set_by_local_host(uint8_t volume)
{
	int ret = -1;
	CMD_DBG("Volume is set locally to: %d%%\n", VOL_ADJ(volume));
	//_lock_acquire(&s_volume_lock);
	s_volume = volume;
	//_lock_release(&s_volume_lock);
	printf("s_volume_notify is %d\n", s_volume_notify);
	if (1) {//s_volume_notify
		xr_avrc_rn_param_t rn_param;
		rn_param.volume = s_volume;
		ret = xr_avrc_tg_send_rn_rsp(XR_AVRC_RN_VOLUME_CHANGE, XR_AVRC_RN_RSP_CHANGED, &rn_param);

#ifdef XRADIO_SINK_ADJUST_VOLUME
		if (ret == XRADIO_ADJ_BYMYSELF) {
			CMD_DBG("The other side doesn't support adjusting the volume, we adjust it ourselves\n");
			volume = XRADIO_AMIXER_MAX - (uint32_t)volume / 2;
			printf("volume is %d\n", volume);
			ret = bluedroid_amixer(volume);
			if (ret != XR_OK) {
				CMD_DBG("Set vol by ourself failed\n");
			}
		}
		else if (ret != XR_OK){
			CMD_DBG("Set vol failed reason is %d\n", ret);
		}
#else
		if (ret != XR_OK){
			CMD_DBG("Set vol failed reason is %d\n", ret);
		}
#endif

		s_volume_notify = false;
	}
}

static void volume_change_simulation(void *arg)
{
	CMD_DBG("start volume change simulation\n");

	for (;;) {
		vTaskDelay(10000 / portTICK_RATE_MS);

		uint8_t volume = (s_volume + 5) & 0x7f;
		volume_set_by_local_host(volume);
	}
}
#endif

//mode 0:sink 1:src
static void handle_passthrough_event(uint8_t key_code, uint8_t key_state, uint8_t mode)
{
	bt_av_media_state state;
	if (!key_state)//0 means press,we only handle 1
		return;

	CMD_DBG("handle ps cmd from %s\n", mode ? "sink" : "src");

	if (mode) {
		switch (key_code) {
			case XR_AVRC_PT_CMD_PLAY:
				if (bt_app_get_media_state() == APP_AV_MEDIA_STATE_SUSPEND)
					state = APP_AV_MEDIA_STATE_STARTED;
				else
					state = APP_AV_MEDIA_STATE_STARTING;
				bt_app_av_media_state_change(state);
				xr_a2d_media_ctrl(XR_A2D_MEDIA_CTRL_START);
				break;
			case XR_AVRC_PT_CMD_STOP:
				state = APP_AV_MEDIA_STATE_STOPPED;
				bt_app_av_media_state_change(state);
				xr_a2d_media_ctrl(XR_A2D_MEDIA_CTRL_STOP);
				break;
			case XR_AVRC_PT_CMD_PAUSE:
				state = APP_AV_MEDIA_STATE_SUSPEND;
				bt_app_av_media_state_change(state);
				xr_a2d_media_ctrl(XR_A2D_MEDIA_CTRL_SUSPEND);
				break;
			case XR_AVRC_PT_CMD_FORWARD:
				bt_app_av_media_play_state_change(APP_AV_MEDIA_FORWARD);
				break;
			case XR_AVRC_PT_CMD_BACKWARD:
				bt_app_av_media_play_state_change(APP_AV_MEDIA_BACKWARD);
				break;
			default:
				break;
		}
	}else {
		switch (key_code) {
			case XR_AVRC_PT_CMD_VOL_UP:
				s_volume = (s_volume > 95) ? 100 : (s_volume + 5);
				CMD_DBG("up vol to %d%%\n", s_volume);
				bt_app_set_system_audio_vol(s_volume);
				break;
			case XR_AVRC_PT_CMD_VOL_DOWN:
				s_volume = (s_volume < 5) ? 0 : (s_volume - 5);
				CMD_DBG("down vol to %d%%\n", s_volume);
				bt_app_set_system_audio_vol(s_volume);
				break;
			default:
				break;
		}
	}

}

static void bt_av_hdl_avrc_tg_evt_snk(uint16_t event, void *p_param)
{
	CMD_DBG("%s evt %d\n", __func__, event);
	xr_avrc_tg_cb_param_t *rc = (xr_avrc_tg_cb_param_t *)(p_param);
	switch (event) {
	case XR_AVRC_TG_CONNECTION_STATE_EVT: {
		uint8_t *bda = rc->conn_stat.remote_bda;
		CMD_DBG("AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]\n",
		        rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
#if (CLO == 1)
		if (rc->conn_stat.connected) {
			//create task to simulate volume change
			xTaskCreate(volume_change_simulation, "vcsT", 2048, NULL, 5, &s_vcs_task_hdl);
		} else {
			vTaskDelete(s_vcs_task_hdl);
			CMD_DBG("Stop volume change simulation\n");
		}
#endif
		break;
	}
	case XR_AVRC_TG_PASSTHROUGH_CMD_EVT: {
		CMD_DBG("sink avrc passthrough cmd: key_code 0x%x, key_state %d\n", rc->psth_cmd.key_code, rc->psth_cmd.key_state);
		handle_passthrough_event(rc->psth_cmd.key_code, rc->psth_cmd.key_state, 0);
		break;
	}
	case XR_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT: {
		CMD_DBG("AVRC set absolute volume: %d%%\n", (int)VOL_ADJ(rc->set_abs_vol.volume));
		if (VOL_ADJ(rc->set_abs_vol.volume) != s_volume) {
			s_volume = VOL_ADJ(rc->set_abs_vol.volume);
			bt_app_set_system_audio_vol(VOL_ADJ(rc->set_abs_vol.volume));
		} else {
			CMD_DBG("AVRC volume is alread %d%%\n", s_volume);
		}
		break;
	}
	case XR_AVRC_TG_REGISTER_NOTIFICATION_EVT: {
		CMD_DBG("AVRC register event notification: %d, param: 0x%x\n", rc->reg_ntf.event_id, rc->reg_ntf.event_parameter);
		if (rc->reg_ntf.event_id == XR_AVRC_RN_VOLUME_CHANGE) {
			s_volume_notify = true;
			xr_avrc_rn_param_t rn_param;
			rn_param.volume = s_volume;
			xr_avrc_tg_send_rn_rsp(XR_AVRC_RN_VOLUME_CHANGE, XR_AVRC_RN_RSP_INTERIM, &rn_param);
		}
		break;
	}
	case XR_AVRC_TG_REMOTE_FEATURES_EVT: {
		CMD_DBG("AVRC remote features %x, CT features %x\n", rc->rmt_feats.feat_mask, rc->rmt_feats.ct_feat_flag);
		break;
	}
	default:
		CMD_DBG("%s unhandled evt %d\n", __func__, event);
		break;
	}
}

static void bt_av_hdl_avrc_tg_evt_src(uint16_t event, void *p_param)
{
	CMD_DBG("%s evt %d\n", __func__, event);
	xr_avrc_tg_cb_param_t *rc = (xr_avrc_tg_cb_param_t *)(p_param);
	switch (event) {
	case XR_AVRC_TG_CONNECTION_STATE_EVT: {
		uint8_t *bda = rc->conn_stat.remote_bda;
		CMD_DBG("AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]\n",
		        rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
		break;
	}
	case XR_AVRC_TG_PASSTHROUGH_CMD_EVT: {
		CMD_DBG("src avrc passthrough cmd: key_code 0x%x, key_state %d\n", rc->psth_cmd.key_code, rc->psth_cmd.key_state);
		handle_passthrough_event(rc->psth_cmd.key_code, rc->psth_cmd.key_state, 1);
		break;
	}
	case XR_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT: {
		CMD_DBG(BT_RC_CT_TAG "Set absolute volume rsp: volume %d.Now we dont't support this feature.\
		                      Maybe we should handle the music that we read.\n", rc->set_abs_vol.volume);
		break;
	}
	case XR_AVRC_TG_REGISTER_NOTIFICATION_EVT: {
		CMD_DBG("AVRC register event notification: %d, param: 0x%x\n", rc->reg_ntf.event_id, rc->reg_ntf.event_parameter);
		if (rc->reg_ntf.event_id == XR_AVRC_RN_VOLUME_CHANGE) {
			s_volume_notify = true;
			xr_avrc_rn_param_t rn_param;
			rn_param.volume = s_volume;
			xr_avrc_tg_send_rn_rsp(XR_AVRC_RN_VOLUME_CHANGE, XR_AVRC_RN_RSP_INTERIM, &rn_param);
		}
		break;
	}
	case XR_AVRC_TG_REMOTE_FEATURES_EVT: {
		CMD_DBG("AVRC remote features %x, CT features %x\n", rc->rmt_feats.feat_mask, rc->rmt_feats.ct_feat_flag);
		break;
	}
	default:
		CMD_DBG("%s unhandled evt %d\n", __func__, event);
		break;
	}
}

void bt_app_rc_ct_cb_snk(xr_avrc_ct_cb_event_t event, xr_avrc_ct_cb_param_t *param)
{
	switch (event) {
	case XR_AVRC_CT_METADATA_RSP_EVT:
	/* fall through */
	case XR_AVRC_CT_CONNECTION_STATE_EVT:
	case XR_AVRC_CT_PASSTHROUGH_RSP_EVT:
	case XR_AVRC_CT_CHANGE_NOTIFY_EVT:
	case XR_AVRC_CT_REMOTE_FEATURES_EVT:
	case XR_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT: {
		bt_app_work_dispatch(bt_av_hdl_avrc_ct_evt_snk, event, param, sizeof(xr_avrc_ct_cb_param_t), NULL);
		break;
	}
	default:
		CMD_DBG("Invalid AVRC event: %d\n", event);
		break;
	}
}

void bt_app_rc_tg_cb_snk(xr_avrc_tg_cb_event_t event, xr_avrc_tg_cb_param_t *param)
{
	switch (event) {
	case XR_AVRC_TG_CONNECTION_STATE_EVT:
	case XR_AVRC_TG_REMOTE_FEATURES_EVT:
	case XR_AVRC_TG_PASSTHROUGH_CMD_EVT:
	case XR_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
	case XR_AVRC_TG_REGISTER_NOTIFICATION_EVT:
	case XR_AVRC_TG_SET_PLAYER_APP_VALUE_EVT:
		bt_app_work_dispatch(bt_av_hdl_avrc_tg_evt_snk, event, param, sizeof(xr_avrc_tg_cb_param_t), NULL);
		break;
	default:
		CMD_DBG("Invalid AVRC event: %d\n", event);
		break;
	}
}

void bt_app_rc_tg_cb_src(xr_avrc_tg_cb_event_t event, xr_avrc_tg_cb_param_t *param)
{
	switch (event) {
	case XR_AVRC_TG_CONNECTION_STATE_EVT:
	case XR_AVRC_TG_REMOTE_FEATURES_EVT:
	case XR_AVRC_TG_PASSTHROUGH_CMD_EVT:
	case XR_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
	case XR_AVRC_TG_REGISTER_NOTIFICATION_EVT:
	case XR_AVRC_TG_SET_PLAYER_APP_VALUE_EVT:
		bt_app_work_dispatch(bt_av_hdl_avrc_tg_evt_src, event, param, sizeof(xr_avrc_tg_cb_param_t), NULL);
		break;
	default:
		CMD_DBG("Invalid AVRC event: %d\n", event);
		break;
	}
}

static void bt_av_volume_changed(void)
{
	if (xr_avrc_rn_evt_bit_mask_operation(XR_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
	                                      XR_AVRC_RN_VOLUME_CHANGE)) {
		xr_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_VOLUME_CHANGE, XR_AVRC_RN_VOLUME_CHANGE, 0);
	}
}

void bt_av_notify_evt_handler_src(uint8_t event_id, xr_avrc_rn_param_t *event_parameter)
{
	switch (event_id) {
	case XR_AVRC_RN_VOLUME_CHANGE:
		CMD_DBG(BT_RC_CT_TAG "Volume changed: %d\n", event_parameter->volume);
		CMD_DBG(BT_RC_CT_TAG "Set absolute volume: volume %d\n", event_parameter->volume);
		xr_avrc_ct_send_set_absolute_volume_cmd(APP_RC_CT_TL_RN_VOLUME_CHANGE, event_parameter->volume);
		bt_av_volume_changed();
		break;
	}
}

static void bt_av_hdl_avrc_ct_evt_src(uint16_t event, void *p_param)
{
	CMD_DBG("%s evt %d\n", __func__, event);
	xr_avrc_ct_cb_param_t *rc = (xr_avrc_ct_cb_param_t *)(p_param);
	switch (event) {
	case XR_AVRC_CT_CONNECTION_STATE_EVT: {
		uint8_t *bda = rc->conn_stat.remote_bda;
		CMD_DBG(BT_RC_CT_TAG "AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]\n",
		        rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

		if (rc->conn_stat.connected) {
			// get remote supported event_ids of peer AVRCP Target
			xr_avrc_ct_send_get_rn_capabilities_cmd(APP_RC_CT_TL_GET_CAPS);
		} else {
			// clear peer notification capability record
			s_avrc_peer_rn_cap.bits = 0;
		}
		break;
	}
	case XR_AVRC_CT_PASSTHROUGH_RSP_EVT: {
		CMD_DBG(BT_RC_CT_TAG "AVRC passthrough rsp: key_code 0x%x, key_state %d\n", rc->psth_rsp.key_code, rc->psth_rsp.key_state);
		break;
	}
	case XR_AVRC_CT_METADATA_RSP_EVT: {
		CMD_DBG(BT_RC_CT_TAG "AVRC metadata rsp: attribute id 0x%x, %s\n", rc->meta_rsp.attr_id, rc->meta_rsp.attr_text);
		//free(rc->meta_rsp.attr_text);
		break;
	}
	case XR_AVRC_CT_CHANGE_NOTIFY_EVT: {
		CMD_DBG(BT_RC_CT_TAG "AVRC event notification: %d\n", rc->change_ntf.event_id);
		bt_av_notify_evt_handler_src(rc->change_ntf.event_id, &rc->change_ntf.event_parameter);
		break;
	}
	case XR_AVRC_CT_REMOTE_FEATURES_EVT: {
		CMD_DBG(BT_RC_CT_TAG "AVRC remote features %x, TG features %x\n", rc->rmt_feats.feat_mask, rc->rmt_feats.tg_feat_flag);
		break;
	}
	case XR_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT: {
		CMD_DBG(BT_RC_CT_TAG "remote rn_cap: count %d, bitmask 0x%x\n", rc->get_rn_caps_rsp.cap_count,
		        rc->get_rn_caps_rsp.evt_set.bits);
		s_avrc_peer_rn_cap.bits = rc->get_rn_caps_rsp.evt_set.bits;

		bt_av_volume_changed();
		break;
	}
	case XR_AVRC_CT_SET_ABSOLUTE_VOLUME_RSP_EVT: {
		CMD_DBG(BT_RC_CT_TAG "Set absolute volume rsp: volume %d%%\n", (int)VOL_ADJ(rc->set_volume_rsp.volume));
		s_volume = VOL_ADJ(rc->set_volume_rsp.volume);
		break;
	}
	default:
		CMD_DBG("%s unhandled evt %d\n", __func__, event);
		break;
	}
}

static void bt_app_rc_ct_cb_src(xr_avrc_ct_cb_event_t event, xr_avrc_ct_cb_param_t *param)
{
	switch (event) {
	case XR_AVRC_CT_METADATA_RSP_EVT:
	case XR_AVRC_CT_CONNECTION_STATE_EVT:
	case XR_AVRC_CT_PASSTHROUGH_RSP_EVT:
	case XR_AVRC_CT_CHANGE_NOTIFY_EVT:
	case XR_AVRC_CT_REMOTE_FEATURES_EVT:
	case XR_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT:
	case XR_AVRC_CT_SET_ABSOLUTE_VOLUME_RSP_EVT: {
		bt_app_work_dispatch(bt_av_hdl_avrc_ct_evt_src, event, param, sizeof(xr_avrc_ct_cb_param_t), NULL);
		break;
	}
	default:
		CMD_DBG("Invalid AVRC event: %d\n", event);
		break;
	}
}

/* $avrc init*/
enum cmd_status cmd_avrc_init_exec(char *cmd)
{
	xr_err_t ret;
	int argc, i;
	char *argv[1];
	bool flag;

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	flag = (strcmp(argv[0], "snk") == 0) ? true : false;

	CMD_DBG(BT_RC_CT_TAG "init %s\n", flag ? "sink" : "source");

	if ((ret = xr_avrc_ct_init()) != XR_OK) {
		CMD_ERR("xr_avrc_ct_init return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	if ((ret = xr_avrc_ct_register_callback(flag ? bt_app_rc_ct_cb_snk : bt_app_rc_ct_cb_src)) != XR_OK) {
		CMD_ERR("xr_avrc_ct_init return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}


	if ((ret = xr_avrc_tg_init()) != XR_OK) {
		CMD_ERR("xr_avrc_ct_init return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	if ((ret = xr_avrc_tg_register_callback(flag ? bt_app_rc_tg_cb_snk : bt_app_rc_tg_cb_src)) != XR_OK) {
		CMD_ERR("xr_avrc_ct_init return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

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
		0x003E	/* bit mask: 0=not used, 1=F1, 2=F2, 3=F3,
				             4=F4, 5=F5 */
	};

	if (ret = xr_avrc_tg_set_psth_cmd_filter(XR_AVRC_PSTH_FILTER_SUPPORTED_CMD, &cmd_set) != XR_OK) {
		CMD_ERR("xr_avrc_tg_set_psth_cmd_filter: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	xr_avrc_rn_evt_cap_mask_t evt_set = {0};
	xr_avrc_rn_evt_bit_mask_operation(XR_AVRC_BIT_MASK_OP_SET, &evt_set, XR_AVRC_RN_VOLUME_CHANGE);
	if ((ret = xr_avrc_tg_set_rn_evt_cap(&evt_set)) != XR_OK) {
		CMD_ERR("xr_avrc_tg_set_rn_evt_cap return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;

}

/* $avrc deinit*/
enum cmd_status cmd_ct_deinit_exec(char *cmd)
{
	xr_err_t ret;
	if ((ret = xr_avrc_ct_deinit()) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

const char* ps_map[XR_AVRC_PS_MAX_ATTR][AVRC_PS_MAX_ATTR_VALUE] = {
	{ "null",       "null",     "null",     "null" },
	{ "equalizer",  "off",      "on",       "null" },
	{ "repeat",     "off",      "single",   "group"},
	{ "shuffle",    "off",      "all",      "group"},
	{ "scan",       "off",      "all",      "group"},
};

int ps_value_map[XR_AVRC_PS_MAX_ATTR][AVRC_PS_MAX_ATTR_VALUE] = {
	{ 0,  0,  0,  0 },
	{ XR_AVRC_PS_EQUALIZER,    XR_AVRC_PS_EQUALIZER_OFF,  XR_AVRC_PS_EQUALIZER_ON,    AVRC_INVALID_VALUE         },
	{ XR_AVRC_PS_REPEAT_MODE,  XR_AVRC_PS_REPEAT_OFF,     XR_AVRC_PS_REPEAT_SINGLE,   XR_AVRC_PS_REPEAT_GROUP   },
	{ XR_AVRC_PS_SHUFFLE_MODE, XR_AVRC_PS_SHUFFLE_OFF,    XR_AVRC_PS_SHUFFLE_ALL,     XR_AVRC_PS_SHUFFLE_GROUP  },
	{ XR_AVRC_PS_SCAN_MODE,    XR_AVRC_PS_SCAN_OFF,       XR_AVRC_PS_SCAN_ALL,        XR_AVRC_PS_SCAN_GROUP     },
};

/* $avrc pscmd <equalizer> <off>*/
enum cmd_status cmd_ct_set_player_setting_exec(char *cmd)
{
	xr_err_t ret;
	uint8_t tl = 0;
	uint8_t attr_id = AVRC_INVALID_VALUE;
	uint8_t value_id = AVRC_INVALID_VALUE;

	int argc, i, j;
	char *argv[2];

	argc = cmd_parse_argv(cmd, argv, 2);
	if (argc < 2) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	for(i = 1; i < XR_AVRC_PS_MAX_ATTR; i++) {
		if (!cmd_strcmp(argv[0], ps_map[i][0])) {
			for(j = 1; j < AVRC_PS_MAX_ATTR_VALUE; j++) {
				if (!cmd_strcmp(argv[1], ps_map[i][j])) {
					attr_id = ps_value_map[i][0];
					value_id = ps_value_map[i][j];
					break;
				}
			}
		}
	}

	if ((attr_id == AVRC_INVALID_VALUE) || (value_id == AVRC_INVALID_VALUE)) {
		CMD_ERR("invalid param %s %s\n", argv[0], argv[1]);
		return CMD_STATUS_INVALID_ARG;
	}

	if ((ret = xr_avrc_ct_send_set_player_value_cmd(tl, attr_id, value_id)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

kv rn_map[XR_AVRC_RN_MAX_EVT] = {
	{ "null",               AVRC_INVALID_VALUE                 },
	{ "play_status",        XR_AVRC_RN_PLAY_STATUS_CHANGE     },
	{ "track_change",       XR_AVRC_RN_TRACK_CHANGE           },
	{ "track_end",          XR_AVRC_RN_TRACK_REACHED_END      },
	{ "track_start",        XR_AVRC_RN_TRACK_REACHED_START    },
	{ "pos_change",         XR_AVRC_RN_PLAY_POS_CHANGED       },
	{ "battery_change",     XR_AVRC_RN_BATTERY_STATUS_CHANGE  },
	{ "system_change",      XR_AVRC_RN_SYSTEM_STATUS_CHANGE   },
	{ "setting_change",     XR_AVRC_RN_APP_SETTING_CHANGE     },
	{ "volume_change",      XR_AVRC_RN_VOLUME_CHANGE          },
};

/* $avrc rncmd <pos_change>*/
enum cmd_status cmd_ct_register_notify_exec(char *cmd)
{
	xr_err_t ret;
	uint8_t tl = 0;
	uint8_t event_id = AVRC_INVALID_VALUE;
	int event_parameter = 0;

	int argc, i;
	char *argv[2];

	argc = cmd_parse_argv(cmd, argv, 2);
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	for(i = 1; i < XR_AVRC_RN_MAX_EVT; i++) {
		if (!cmd_strcmp(argv[0], rn_map[i].key)) {
			event_id = rn_map[i].value;
			break;
		}
	}

	if (event_id == AVRC_INVALID_VALUE) {
		CMD_ERR("invalid param %s\n", argv[0]);
		return CMD_STATUS_INVALID_ARG;
	}

	if (event_id == XR_AVRC_RN_PLAY_POS_CHANGED) {
		if (argc > 1) {
			event_parameter = cmd_atoi(argv[1]);
		} else {
			/* default param 1 second*/
			event_parameter = 1;
		}
	}

	if ((ret = xr_avrc_ct_send_register_notification_cmd(tl, event_id, event_parameter)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

/* $avrc rnrsp <volume_change> */
enum cmd_status cmd_tg_register_notify_rsp_exec(char *cmd)
{
	xr_err_t ret;
	uint8_t event_id = AVRC_INVALID_VALUE;

	int argc, i;
	char *argv[2];

	argc = cmd_parse_argv(cmd, argv, 2);
	if (argc < 2) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	for(i = 1; i < XR_AVRC_RN_MAX_EVT; i++) {
		if (!cmd_strcmp(argv[0], rn_map[i].key)) {
			event_id = rn_map[i].value;
			break;
		}
	}

	if (event_id == AVRC_INVALID_VALUE) {
		CMD_ERR("invalid param %s\n", argv[0]);
		return CMD_STATUS_INVALID_ARG;
	}
	xr_avrc_rn_rsp_t rsp_param;
	int param = cmd_atoi(argv[1]);

	if (event_id == XR_AVRC_RN_PLAY_POS_CHANGED) {
		if (param <= 0 || param > 5) {
			CMD_ERR("invalid media pos change.Range(1-4)\n");
			return CMD_STATUS_INVALID_ARG;
		}
		// rsp_param.param = param;
	} else if (event_id == XR_AVRC_RN_VOLUME_CHANGE) {
		// rsp_param.volume = xr_avrc_tg_handle_abs_vol(BT_AV_GET_ABS_VOL, 0);
		if (param < 0 || param > XR_AVRC_MAX_VOLUME) {
			CMD_ERR("invalid audio vol.Range(0-128)\n");
			return CMD_STATUS_INVALID_ARG;
		}
		CMD_DBG(BT_AV_TAG "vol will set to %d\n", param);
		bt_app_set_system_audio_vol(param);
		//volume_set_by_local_host(param);
		//xr_avrc_rn_param_t rn_param;
		//rn_param.volume = param;
		//ret = xr_avrc_tg_send_rn_rsp(XR_AVRC_RN_VOLUME_CHANGE, XR_AVRC_RN_RSP_CHANGED, &rn_param);
		//if (ret != XR_OK) {
			//CMD_ERR("set voice failed %d\n", ret);
			//return CMD_STATUS_FAIL;
		//}
	} else {
		CMD_ERR("not support invalid param %s\n", argv[0]);
		return CMD_STATUS_INVALID_ARG;
	}

	return CMD_STATUS_OK;
}

/* $avrc mdcmd */
/* only support GetElemAttrs for now */
enum cmd_status cmd_ct_send_metadata_exec(char *cmd)
{
	int argc;
	char *argv[8];
	xr_err_t ret;
	uint8_t tl = 0;
	uint8_t attr_mask = 0;

	argc = cmd_parse_argv(cmd, argv, cmd_nitems(argv));
	if (argc < 1) {
		CMD_DBG("bt avrc mdcmd TITLE ARTIST ALBUM TRACK_POSITION TRACK_NUM TRACK_GENRE PLAYING_TIME\n");
		CMD_DBG("default get ITLE ARTIST ALBUM TRACK_GENRE\n");
		attr_mask = XR_AVRC_MD_ATTR_TITLE | XR_AVRC_MD_ATTR_ARTIST | XR_AVRC_MD_ATTR_ALBUM | XR_AVRC_MD_ATTR_GENRE;
	} else {
		for (int i = 0; i < argc; i++) {
			const char *arg = argv[i];
			if (!strcmp(arg, "TITLE")) {
				attr_mask |= XR_AVRC_MD_ATTR_TITLE;
			} else if (!strcmp(arg, "ARTIST")) {
				attr_mask |= XR_AVRC_MD_ATTR_ARTIST;
			} else if (!strcmp(arg, "ALBUM")) {
				attr_mask |= XR_AVRC_MD_ATTR_ALBUM;
			} else if (!strcmp(arg, "TRACK_POSITION")) {
				attr_mask |= XR_AVRC_MD_ATTR_TRACK_NUM;
			} else if (!strcmp(arg, "TRACK_NUM")) {
				attr_mask |= XR_AVRC_MD_ATTR_NUM_TRACKS;
			} else if (!strcmp(arg, "TRACK_GENRE")) {
				attr_mask |= XR_AVRC_MD_ATTR_GENRE;
			} else if (!strcmp(arg, "PLAYING_TIME")) {
				attr_mask |= XR_AVRC_MD_ATTR_PLAYING_TIME;
			} else {
				CMD_ERR("invalid param %s\n", argv[i]);
			}
		}
	}

	if ((ret = xr_avrc_ct_send_metadata_cmd(tl, attr_mask)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;
}

kv pt_map[AVRC_PT_MAX_KEY_CODE] = {
	{ "play",               XR_AVRC_PT_CMD_PLAY           },
	{ "stop",               XR_AVRC_PT_CMD_STOP           },
	{ "pause",              XR_AVRC_PT_CMD_PAUSE          },
	{ "forward",            XR_AVRC_PT_CMD_FORWARD        },
	{ "backward",           XR_AVRC_PT_CMD_BACKWARD       },
	{ "up",                 XR_AVRC_PT_CMD_VOL_UP         },
	{ "down",               XR_AVRC_PT_CMD_VOL_DOWN       },
	{ "mute",               XR_AVRC_PT_CMD_MUTE           },
	{ "select",             XR_AVRC_PT_CMD_SELECT         },
	{ "up",                 XR_AVRC_PT_CMD_UP             },
	{ "down",               XR_AVRC_PT_CMD_DOWN           },
	{ "left",               XR_AVRC_PT_CMD_LEFT           },
	{ "right",              XR_AVRC_PT_CMD_RIGHT          },
	{ "right-up",           XR_AVRC_PT_CMD_RIGHT_UP       },
	{ "right-down",         XR_AVRC_PT_CMD_RIGHT_DOWN     },
	{ "left-up",            XR_AVRC_PT_CMD_LEFT_UP        },
	{ "left-down",          XR_AVRC_PT_CMD_LEFT_DOWN      },
	{ "root_menu",          XR_AVRC_PT_CMD_ROOT_MENU      },
	{ "setup_menu",         XR_AVRC_PT_CMD_SETUP_MENU     },
	{ "contents_menu",      XR_AVRC_PT_CMD_CONT_MENU      },
	{ "favorite_menu",      XR_AVRC_PT_CMD_FAV_MENU       },
	{ "exit",               XR_AVRC_PT_CMD_EXIT           },
	{ "0",                  XR_AVRC_PT_CMD_0              },
	{ "1",                  XR_AVRC_PT_CMD_1              },
	{ "2",                  XR_AVRC_PT_CMD_2              },
	{ "3",                  XR_AVRC_PT_CMD_3              },
	{ "4",                  XR_AVRC_PT_CMD_4              },
	{ "5",                  XR_AVRC_PT_CMD_5              },
	{ "6",                  XR_AVRC_PT_CMD_6              },
	{ "7",                  XR_AVRC_PT_CMD_7              },
	{ "8",                  XR_AVRC_PT_CMD_8              },
	{ "9",                  XR_AVRC_PT_CMD_9              },
	{ "dot",                XR_AVRC_PT_CMD_DOT            },
	{ "enter",              XR_AVRC_PT_CMD_ENTER          },
	{ "clear",              XR_AVRC_PT_CMD_CLEAR          },
	{ "channel_up",         XR_AVRC_PT_CMD_CHAN_UP        },
	{ "channel_down",       XR_AVRC_PT_CMD_CHAN_DOWN      },
	{ "previous_channel",  XR_AVRC_PT_CMD_PREV_CHAN       },
	{ "sound_select",      XR_AVRC_PT_CMD_SOUND_SEL       },
	{ "input_select",      XR_AVRC_PT_CMD_INPUT_SEL       },
	{ "display_information",XR_AVRC_PT_CMD_DISP_INFO      },
	{ "help",              XR_AVRC_PT_CMD_HELP            },
	{ "page_up",           XR_AVRC_PT_CMD_PAGE_UP         },
	{ "page_down",         XR_AVRC_PT_CMD_PAGE_DOWN       },
	{ "power",             XR_AVRC_PT_CMD_POWER           },
	{ "record",            XR_AVRC_PT_CMD_RECORD          },
	{ "rewind",            XR_AVRC_PT_CMD_REWIND          },
	{ "fast_forward",      XR_AVRC_PT_CMD_FAST_FORWARD    },
	{ "eject",             XR_AVRC_PT_CMD_EJECT           },
	{ "angle",             XR_AVRC_PT_CMD_ANGLE           },
	{ "subpicture",        XR_AVRC_PT_CMD_SUBPICT         },
	{ "F1",                XR_AVRC_PT_CMD_F1              },
	{ "F2",                XR_AVRC_PT_CMD_F2              },
	{ "F3",               XR_AVRC_PT_CMD_F3             },
	{ "F4",                XR_AVRC_PT_CMD_F4              },
	{ "F5",                XR_AVRC_PT_CMD_F5              },
	{ "vendor_unique",     XR_AVRC_PT_CMD_VENDOR          },
};

/* $avrc ptcmd <play/stop/pause/forward/backward/up/down/mute> */
/* passthrough cmd may not take effect if TG not support */
enum cmd_status cmd_ct_send_passthrough_exec(char *cmd)
{
	xr_err_t ret;
	uint8_t tl = 0;
	uint8_t key_code = AVRC_INVALID_VALUE;
	uint8_t key_state = XR_AVRC_PT_CMD_STATE_PRESSED;

	int argc, i;
	char *argv[2];

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	for(i = 0; i < AVRC_PT_MAX_KEY_CODE; i++) {
		if (!cmd_strcmp(argv[0], pt_map[i].key)) {
			key_code = pt_map[i].value;
			break;
		}
	}

	if (key_code == AVRC_INVALID_VALUE) {
		CMD_ERR("invalid param %s\n", argv[0]);
		return CMD_STATUS_INVALID_ARG;
	}

	if ((ret = xr_avrc_ct_send_passthrough_cmd(tl, key_code, key_state)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	key_state = XR_AVRC_PT_CMD_STATE_RELEASED;
	if ((ret = xr_avrc_ct_send_passthrough_cmd(tl, key_code, key_state)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/* $avrc avcmd <volume>(0~128) */
enum cmd_status cmd_ct_set_absolute_volume_exec(char *cmd)
{
	xr_err_t ret;
	uint8_t tl = 0;
	uint8_t volume = 0;

	int argc, i;
	char *argv[1];

	argc = cmd_parse_argv(cmd, argv, 1);
	if (argc < 1) {
		CMD_ERR("invalid param number %d\n", argc);
		return CMD_STATUS_INVALID_ARG;
	}

	volume = cmd_atoi(argv[0]);
	printf("volume %d\n", volume);
	if (volume < 0 || volume > XR_AVRC_MAX_VOLUME) {
		CMD_ERR("invalid audio vol.Range(0-128)\n");
		return CMD_STATUS_INVALID_ARG;
	}

	if ((ret = xr_avrc_ct_send_set_absolute_volume_cmd(tl, volume)) != XR_OK) {
		CMD_ERR("return failed: %d\n", ret);
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/*
    $avrc init
    $avrc deinit
    $avrc pscmd <attr_id> <value_id>
    $avrc rncmd <event_id>
    $avrc mdcmd
    $avrc ptcmd <key_code>
    $avrc avcmd <volume>(0~128)
*/
static const struct cmd_data g_avrc_cmds[] = {
	{ "init",         cmd_avrc_init_exec,              CMD_DESC("<mode:snk/src>") },
	{ "deinit",       cmd_ct_deinit_exec,              CMD_DESC("No parameters") },
	{ "pscmd",        cmd_ct_set_player_setting_exec,  CMD_DESC("<mode:null/equalizer/repeat/shuffle/scan> <state:off/on/null/all/single/group>") },
	{ "rncmd",        cmd_ct_register_notify_exec,     CMD_DESC("<event:play_status/track_change/track_end/track_start/pos_change/battery_change/system_change/setting_change/volume_change>") },
	{ "rnrsp",        cmd_tg_register_notify_rsp_exec, CMD_DESC("<event:play_status/track_change/track_end/track_start/pos_change/battery_change/system_change/setting_change/volume_change><param>") },
	{ "mdcmd",        cmd_ct_send_metadata_exec,       CMD_DESC("No parameters") },
	{ "ptcmd",        cmd_ct_send_passthrough_exec,    CMD_DESC("<ctrl:play/stop/pause/forward/backward/up/down/mute>") },
	{ "avcmd",        cmd_ct_set_absolute_volume_exec, CMD_DESC("<volume>(0~128)") },
	{ "help",         cmd_avrc_help_exec,              CMD_DESC(CMD_HELP_DESC) },

};

static enum cmd_status cmd_avrc_help_exec(char *cmd)
{
	return cmd_help_exec(g_avrc_cmds, cmd_nitems(g_avrc_cmds), 8);
}

enum cmd_status cmd_avrc_exec(char *cmd)
{
	return cmd_exec(cmd, g_avrc_cmds, cmd_nitems(g_avrc_cmds));
}

#endif
