/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>

#include "bt_types.h"
#include "gap_legacy.h"

#include "bt_sdp.h"

#define RTK_COMPANY_ID      0x005D

static const uint8_t did_sdp_record[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x4D,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PNP_INFORMATION >> 8),
    (uint8_t)(UUID_PNP_INFORMATION),

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP,

    //attribute SDP_ATTR_LANG_BASE_ATTR_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_LANG_BASE_ATTR_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_LANG_BASE_ATTR_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x09,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_LANG_ENGLISH >> 8),
    (uint8_t)SDP_LANG_ENGLISH,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_CHARACTER_UTF8 >> 8),
    (uint8_t)SDP_CHARACTER_UTF8,
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_BASE_LANG_OFFSET >> 8),
    (uint8_t)SDP_BASE_LANG_OFFSET,

    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PNP_INFORMATION >> 8),
    (uint8_t)UUID_PNP_INFORMATION,
    SDP_UNSIGNED_TWO_BYTE,
    0x01,//version 1.3
    0x03,

    //attribute SDP_ATTR_DIP_SPECIFICATION_ID
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_SPECIFICATION_ID >> 8),
    (uint8_t)SDP_ATTR_DIP_SPECIFICATION_ID,
    SDP_UNSIGNED_TWO_BYTE,
    0x01,
    0x03,

    //attribute SDP_ATTR_DIP_VENDOR_ID
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_VENDOR_ID >> 8),
    (uint8_t)SDP_ATTR_DIP_VENDOR_ID,
    SDP_UNSIGNED_TWO_BYTE,
    //0x00,//0x005D : RealTek
    //0x5D,
    (uint8_t)(RTK_COMPANY_ID >> 8),
    (uint8_t)RTK_COMPANY_ID,

    //attribute SDP_ATTR_DIP_PRODUCT_ID
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_PRODUCT_ID >> 8),
    (uint8_t)SDP_ATTR_DIP_PRODUCT_ID,
    SDP_UNSIGNED_TWO_BYTE,
    0x22,//8763
    0x3B,

    //attribute SDP_ATTR_DIP_PRODUCT_VERSION
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_PRODUCT_VERSION >> 8),
    (uint8_t)SDP_ATTR_DIP_PRODUCT_VERSION,
    SDP_UNSIGNED_TWO_BYTE,
    0x01,// 1.0.0
    0x00,

    //attribute SDP_ATTR_DIP_PRIMARY_RECORD
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_PRIMARY_RECORD >> 8),
    (uint8_t)SDP_ATTR_DIP_PRIMARY_RECORD,
    SDP_BOOL_ONE_BYTE,
    true,

    //attribute SDP_ATTR_DIP_VENDOR_ID_SOURCE
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_DIP_VENDOR_ID_SOURCE >> 8),
    (uint8_t)SDP_ATTR_DIP_VENDOR_ID_SOURCE,
    SDP_UNSIGNED_TWO_BYTE,
    0x00,//Bluetooth SIG
    0x01
};

static const uint8_t avrcp_ct_sdp_record[] =
{
    //Total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x3B,

    //Attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Attribute length: 6 bytes
    //Service Class #0: A/V Remote Control
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL),
    //Service Class #1: A/V Remote Control Controller
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL_CONTROLLER >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL_CONTROLLER),

    //Attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x10, //Attribute length: 12 bytes
    //Protocol #0: L2CAP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 3 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    //Parameter #0 for Protocol #0: PSM = AVCTP
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_AVCTP >> 8),
    (uint8_t)PSM_AVCTP,
    //Protocol #1: AVCTP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 5 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AVCTP >> 8),
    (uint8_t)(UUID_AVCTP),
    //Parameter #0 for Protocol #1: 0x0104 (v1.4)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0104 >> 8),
    (uint8_t)(0x0104),

    //Attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08, //Attribute length: 8 bytes
    //Profile #0: A/V Remote Control
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 6 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL),
    //Parameter #0 for Profile #0: 0x0106 (v1.6)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0106 >> 8),
    (uint8_t)(0x0106),

    //Attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SUPPORTED_FEATURES) >> 8),
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0001 >> 8), //Category 1 Player / Recorder
    (uint8_t)(0x0001),

    //Attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP
};

static const uint8_t avrcp_tg_sdp_record[] =
{
    //Total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x38,//0x46,//0x5F,

    //Attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03, //Attribute length: 6 bytes
    //Service Class #0: A/V Remote Control Target
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL_TARGET >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL_TARGET),

    //Attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x10, //Attribute length: 12 bytes
    //Protocol #0: L2CAP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 3 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    //Parameter #0 for Protocol #0: PSM = AVCTP
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_AVCTP >> 8),
    (uint8_t)PSM_AVCTP,
    //Protocol #1: AVCTP
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 5 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AVCTP >> 8),
    (uint8_t)(UUID_AVCTP),
    //Parameter #0 for Protocol #1: 0x0104 (v1.4)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0104 >> 8),
    (uint8_t)(0x0104),

    //Attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08, //Attribute length: 8 bytes
    //Profile #0: A/V Remote Control
    SDP_DATA_ELEM_SEQ_HDR,
    0x06, //Element length: 6 bytes
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AV_REMOTE_CONTROL >> 8),
    (uint8_t)(UUID_AV_REMOTE_CONTROL),
    //Parameter #0 for Profile #0: 0x0106 (v1.6)
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0106 >> 8),
    (uint8_t)(0x0106),

    //Attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SUPPORTED_FEATURES) >> 8),
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x0002 >> 8), //Category 2 Amplifier
    (uint8_t)(0x0002),

    //Attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP
};

static const uint8_t hsp_sdp_record[] =
{
    //total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x39,//0x58,

    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x09,                                   //6bytes
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_HEADSET >> 8),       //0x1108
    (uint8_t)(UUID_HEADSET),
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_HEADSET_HS >> 8),    //0x1131
    (uint8_t)(UUID_HEADSET_HS),
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_GENERIC_AUDIO >> 8),  //0x1203
    (uint8_t)(UUID_GENERIC_AUDIO),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x0C,                                   //12 bytes
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x03,                               ///3 bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_L2CAP >> 8),     //0x0100
    (uint8_t)(UUID_L2CAP),
    SDP_DATA_ELEM_SEQ_HDR,
    0x05,                               //5 bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_RFCOMM >> 8),    //0x0003
    (uint8_t)(UUID_RFCOMM),
    SDP_UNSIGNED_ONE_BYTE,           //0x08
    RFC_HSP_CHANN_NUM,      //0x01

    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x08,                                   //8 bytes
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x06,                               //6 bytes
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_HEADSET >> 8),   //0x1108
    (uint8_t)(UUID_HEADSET),
    SDP_UNSIGNED_TWO_BYTE,               //0x09
    (uint8_t)(0x0102 >> 8),         //version number default hs1.2
    (uint8_t)(0x0102),

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP,

    //attribute SDP_ATTR_RMT_AUDIO_VOL_CTRL
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_RMT_AUDIO_VOL_CTRL >> 8),
    (uint8_t)SDP_ATTR_RMT_AUDIO_VOL_CTRL,
    SDP_BOOL_ONE_BYTE,                       //0x28
    0x01                                    //True
};

static const uint8_t hfp_sdp_record[] =
{
    //total length
    SDP_DATA_ELEM_SEQ_HDR,
    0x37,//0x59,

    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x06,                                   //6bytes
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_HANDSFREE >> 8), //0x111E
    (uint8_t)(UUID_HANDSFREE),
    SDP_UUID16_HDR,                     //0x19
    (uint8_t)(UUID_GENERIC_AUDIO >> 8),  //0x1203
    (uint8_t)(UUID_GENERIC_AUDIO),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x0C,                                   //12bytes
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x03,                               //3bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_L2CAP >> 8),     //0x0100
    (uint8_t)(UUID_L2CAP),
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x05,                               //5bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_RFCOMM >> 8),   //0x0003
    (uint8_t)(UUID_RFCOMM),
    SDP_UNSIGNED_ONE_BYTE,           //0x08
    RFC_HFP_CHANN_NUM,  //0x02

    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,          //0x35
    0x08,                                   //8 bytes
    SDP_DATA_ELEM_SEQ_HDR,      //0x35
    0x06,                               //6 bytes
    SDP_UUID16_HDR,                 //0x19
    (uint8_t)(UUID_HANDSFREE >> 8), //0x111E
    (uint8_t)(UUID_HANDSFREE),
    SDP_UNSIGNED_TWO_BYTE,           //0x09
    (uint8_t)(0x0107 >> 8),     //version number default hf1.7
    (uint8_t)(0x0107),

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)UUID_PUBLIC_BROWSE_GROUP,

    //attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)((SDP_ATTR_SUPPORTED_FEATURES) >> 8),
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(0x003F >> 8),
    (uint8_t)(0x003F)
};

static const uint8_t a2dp_sdp_record[] =
{
    SDP_DATA_ELEM_SEQ_HDR,
    0x39,//0x55,
    //attribute SDP_ATTR_SRV_CLASS_ID_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SRV_CLASS_ID_LIST >> 8),
    (uint8_t)SDP_ATTR_SRV_CLASS_ID_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AUDIO_SINK >> 8),
    (uint8_t)(UUID_AUDIO_SINK),

    //attribute SDP_ATTR_PROTO_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROTO_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROTO_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x10,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_L2CAP >> 8),
    (uint8_t)(UUID_L2CAP),
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(PSM_AVDTP >> 8),
    (uint8_t)(PSM_AVDTP),
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_AVDTP >> 8),
    (uint8_t)(UUID_AVDTP),
    SDP_UNSIGNED_TWO_BYTE,
    0x01,
    0x03,

    //attribute SDP_ATTR_BROWSE_GROUP_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_BROWSE_GROUP_LIST >> 8),
    (uint8_t)SDP_ATTR_BROWSE_GROUP_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x03,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP >> 8),
    (uint8_t)(UUID_PUBLIC_BROWSE_GROUP),

    //attribute SDP_ATTR_PROFILE_DESC_LIST
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_PROFILE_DESC_LIST >> 8),
    (uint8_t)SDP_ATTR_PROFILE_DESC_LIST,
    SDP_DATA_ELEM_SEQ_HDR,
    0x08,
    SDP_DATA_ELEM_SEQ_HDR,
    0x06,
    SDP_UUID16_HDR,
    (uint8_t)(UUID_ADVANCED_AUDIO_DISTRIBUTION >> 8),
    (uint8_t)(UUID_ADVANCED_AUDIO_DISTRIBUTION),
    SDP_UNSIGNED_TWO_BYTE,
    0x01,//version 1.3
    0x03,

    //attribute SDP_ATTR_SUPPORTED_FEATURES
    SDP_UNSIGNED_TWO_BYTE,
    (uint8_t)(SDP_ATTR_SUPPORTED_FEATURES >> 8),
    (uint8_t)SDP_ATTR_SUPPORTED_FEATURES,
    SDP_UNSIGNED_TWO_BYTE,
    0x00,
    0x03
};

void bt_sdp_init(void)
{
    legacy_add_sdp_record((void *)did_sdp_record);
    legacy_add_sdp_record((void *)a2dp_sdp_record);
    legacy_add_sdp_record((void *)avrcp_ct_sdp_record);
    legacy_add_sdp_record((void *)avrcp_tg_sdp_record);
    legacy_add_sdp_record((void *)hfp_sdp_record);
    legacy_add_sdp_record((void *)hsp_sdp_record);
}

