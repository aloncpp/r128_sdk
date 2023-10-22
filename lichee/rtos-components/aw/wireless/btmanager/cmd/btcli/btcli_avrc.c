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
#include <bt_manager.h>
#include <AudioSystem.h>

static enum cmd_status btcli_avrc_help(char *cmd);

#define AVRC_PT_MAX_KEY_CODE 57
#define AVRC_INVALID_VALUE   0xFF

typedef struct key_value {
    char *key;
    uint8_t value;
} kv;

void btcli_avrcp_ct_play_state_cb(const char *bd_addr, btmg_avrcp_play_state_t state)
{
    if (state == BTMG_AVRCP_PLAYSTATE_STOPPED) {
        CMD_DBG("BT playing music stopped with device: %s\n", bd_addr);
    } else if (state == BTMG_AVRCP_PLAYSTATE_PLAYING) {
        CMD_DBG("BT palying music playing with device: %s\n", bd_addr);
    } else if (state == BTMG_AVRCP_PLAYSTATE_PAUSED) {
        CMD_DBG("BT palying music paused with device: %s\n", bd_addr);
    } else if (state == BTMG_AVRCP_PLAYSTATE_FWD_SEEK) {
        CMD_DBG("BT palying music FWD SEEK with device: %s\n", bd_addr);
    } else if (state == BTMG_AVRCP_PLAYSTATE_REV_SEEK) {
        CMD_DBG("BT palying music REV SEEK with device: %s\n", bd_addr);
    } else if (state == BTMG_AVRCP_PLAYSTATE_FORWARD) {
        CMD_DBG("BT palying music forward with device: %s\n", bd_addr);
    } else if (state == BTMG_AVRCP_PLAYSTATE_BACKWARD) {
        CMD_DBG("BT palying music backward with device: %s\n", bd_addr);
    } else if (state == BTMG_AVRCP_PLAYSTATE_ERROR) {
        CMD_DBG("BT palying music ERROR with device: %s\n", bd_addr);
    }
}

void btcli_avrcp_ct_track_changed_cb(const char *bd_addr, btmg_track_info_t *track_info)
{
    CMD_DBG("BT playing music title: %s\n", track_info->title);
    CMD_DBG("BT playing music artist: %s\n", track_info->artist);
    CMD_DBG("BT playing music album: %s\n", track_info->album);
    CMD_DBG("BT playing music track number: %s\n", track_info->track_num);
    CMD_DBG("BT playing music total number of tracks: %s\n", track_info->num_tracks);
    CMD_DBG("BT playing music genre: %s\n", track_info->genre);
    CMD_DBG("BT playing music duration: %s\n", track_info->duration);
}

void btcli_avrcp_ct_play_position_cb(const char *bd_addr, int song_len, int song_pos)
{
    CMD_DBG("dev:[%s],playing song:[len:%d][position:%d]\n", bd_addr, song_len, song_pos);
}

static void set_system_audio_vol(uint32_t volume)
{
    int ret = 0;
    int type = AUDIO_STREAM_SYSTEM;
    uint32_t volume_value = 0;
    uint8_t max_volume = 0;

    ret = softvol_control_with_streamtype(type, &volume_value, 2);
    if (ret != 0) {
        CMD_ERR("get softvol range failed:%d\n", ret);
        return;
    }
    max_volume = (volume_value >> 16) & 0xffff;
    volume_value = (volume * max_volume / 100) & 0xffff;
    ret = softvol_control_with_streamtype(type, &volume_value, 1);
    if (ret != 0) {
        CMD_ERR("set softvol failed:%d\n", ret);
        return;
    }
}

void btcli_avrcp_audio_volume_cb(const char *bd_addr, unsigned int volume)
{
    CMD_DBG("AVRCP audio volume: %d%%\n", volume);
    set_system_audio_vol(volume);
}

static kv pt_map[AVRC_PT_MAX_KEY_CODE] = {
    { "play",                BTMG_AVRC_PT_CMD_PLAY         },
    { "stop",                BTMG_AVRC_PT_CMD_STOP         },
    { "pause",               BTMG_AVRC_PT_CMD_PAUSE        },
    { "forward",             BTMG_AVRC_PT_CMD_FORWARD      },
    { "backward",            BTMG_AVRC_PT_CMD_BACKWARD     },
    { "up",                  BTMG_AVRC_PT_CMD_VOL_UP       },
    { "down",                BTMG_AVRC_PT_CMD_VOL_DOWN     },
    { "mute",                BTMG_AVRC_PT_CMD_MUTE         },
    { "select",              BTMG_AVRC_PT_CMD_SELECT       },
    { "up",                  BTMG_AVRC_PT_CMD_UP           },
    { "down",                BTMG_AVRC_PT_CMD_DOWN         },
    { "left",                BTMG_AVRC_PT_CMD_LEFT         },
    { "right",               BTMG_AVRC_PT_CMD_RIGHT        },
    { "right-up",            BTMG_AVRC_PT_CMD_RIGHT_UP     },
    { "right-down",          BTMG_AVRC_PT_CMD_RIGHT_DOWN   },
    { "left-up",             BTMG_AVRC_PT_CMD_LEFT_UP      },
    { "left-down",           BTMG_AVRC_PT_CMD_LEFT_DOWN    },
    { "root_menu",           BTMG_AVRC_PT_CMD_ROOT_MENU    },
    { "setup_menu",          BTMG_AVRC_PT_CMD_SETUP_MENU   },
    { "contents_menu",       BTMG_AVRC_PT_CMD_CONT_MENU    },
    { "favorite_menu",       BTMG_AVRC_PT_CMD_FAV_MENU     },
    { "exit",                BTMG_AVRC_PT_CMD_EXIT         },
    { "0",                   BTMG_AVRC_PT_CMD_0            },
    { "1",                   BTMG_AVRC_PT_CMD_1            },
    { "2",                   BTMG_AVRC_PT_CMD_2            },
    { "3",                   BTMG_AVRC_PT_CMD_3            },
    { "4",                   BTMG_AVRC_PT_CMD_4            },
    { "5",                   BTMG_AVRC_PT_CMD_5            },
    { "6",                   BTMG_AVRC_PT_CMD_6            },
    { "7",                   BTMG_AVRC_PT_CMD_7            },
    { "8",                   BTMG_AVRC_PT_CMD_8            },
    { "9",                   BTMG_AVRC_PT_CMD_9            },
    { "dot",                 BTMG_AVRC_PT_CMD_DOT          },
    { "enter",               BTMG_AVRC_PT_CMD_ENTER        },
    { "clear",               BTMG_AVRC_PT_CMD_CLEAR        },
    { "channel_up",          BTMG_AVRC_PT_CMD_CHAN_UP      },
    { "channel_down",        BTMG_AVRC_PT_CMD_CHAN_DOWN    },
    { "previous_channel",    BTMG_AVRC_PT_CMD_PREV_CHAN    },
    { "sound_select",        BTMG_AVRC_PT_CMD_SOUND_SEL    },
    { "input_select",        BTMG_AVRC_PT_CMD_INPUT_SEL    },
    { "display_information", BTMG_AVRC_PT_CMD_DISP_INFO    },
    { "help",                BTMG_AVRC_PT_CMD_HELP         },
    { "page_up",             BTMG_AVRC_PT_CMD_PAGE_UP      },
    { "page_down",           BTMG_AVRC_PT_CMD_PAGE_DOWN    },
    { "power",               BTMG_AVRC_PT_CMD_POWER        },
    { "record",              BTMG_AVRC_PT_CMD_RECORD       },
    { "rewind",              BTMG_AVRC_PT_CMD_REWIND       },
    { "fast_forward",        BTMG_AVRC_PT_CMD_FAST_FORWARD },
    { "eject",               BTMG_AVRC_PT_CMD_EJECT        },
    { "angle",               BTMG_AVRC_PT_CMD_ANGLE        },
    { "subpicture",          BTMG_AVRC_PT_CMD_SUBPICT      },
    { "F1",                  BTMG_AVRC_PT_CMD_F1           },
    { "F2",                  BTMG_AVRC_PT_CMD_F2           },
    { "F3",                  BTMG_AVRC_PT_CMD_F3           },
    { "F4",                  BTMG_AVRC_PT_CMD_F4           },
    { "F5",                  BTMG_AVRC_PT_CMD_F5           },
    { "vendor_unique",       BTMG_AVRC_PT_CMD_VENDOR       },
};

/* btcli avrc ptcmd <play/stop/pause/forward/backward/up/down/mute> */
/* passthrough cmd may not take effect if TG not support */
enum cmd_status btcli_ct_send_passthrough(char *cmd)
{
    uint8_t key_code = AVRC_INVALID_VALUE;
    int argc;
    char *argv[2];

    argc = cmd_parse_argv(cmd, argv, 1);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    for (int i = 0; i < AVRC_PT_MAX_KEY_CODE; i++) {
        if (!cmd_strcmp(argv[0], pt_map[i].key)) {
            key_code = pt_map[i].value;
            break;
        }
    }

    if (key_code == AVRC_INVALID_VALUE) {
        CMD_ERR("invalid param %s\n", argv[0]);
        return CMD_STATUS_INVALID_ARG;
    }

    btmg_avrc_ct_send_passthrough_cmd(key_code);

    return CMD_STATUS_OK;
}

/* btcli avrc set_vol */
enum cmd_status btcli_set_absolute_volume(char *cmd)
{
    uint32_t volume = 0;
    int argc;
    char *argv[1];

    argc = cmd_parse_argv(cmd, argv, 1);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    volume = cmd_atoi(argv[0]);
    if (volume < 0 || volume > 100) {
        CMD_ERR("invalid audio vol.Range(0-100)\n");
        return CMD_STATUS_INVALID_ARG;
    }

    btmg_avrc_set_absolute_volume(volume);

    return CMD_STATUS_OK;
}

/* btcli avrc get_vol */
enum cmd_status btcli_get_absolute_volume(char *cmd)
{
    uint32_t volume = 0;

    btmg_avrc_get_absolute_volume(&volume);

    CMD_DBG("now absolute_volume: %d%%\n", volume);

    return CMD_STATUS_OK;
}

static const struct cmd_data avrc_cmds[] = {
    { "ptcmd",   btcli_ct_send_passthrough,  CMD_DESC("<ctrl:play/stop/pause/forward/backward/up/down/mute>") },
    { "set_vol", btcli_set_absolute_volume,  CMD_DESC("<volume: 0-100>")},
    { "get_vol", btcli_get_absolute_volume,  CMD_DESC("No parameters")},
    { "help",    btcli_avrc_help,            CMD_DESC(CMD_HELP_DESC) },
};

/* btcli avrc help */
static enum cmd_status btcli_avrc_help(char *cmd)
{
	return cmd_help_exec(avrc_cmds, cmd_nitems(avrc_cmds), 10);
}

enum cmd_status btcli_avrc(char *cmd)
{
    return cmd_exec(cmd, avrc_cmds, cmd_nitems(avrc_cmds));
}
