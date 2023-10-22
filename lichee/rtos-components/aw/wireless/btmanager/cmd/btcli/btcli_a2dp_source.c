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
#include "string.h"
#include "bt_manager.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <dirent.h>

static enum cmd_status btcli_source_help(char *cmd);

#define A2DP_SRC_BUFF_SIZE (512)

XR_OS_Thread_t a2dp_play_thread;
int music_fd = -1;

typedef enum {
    STOP = 0,
    PLAY,
    PAUSE,
    FORWARD,
    BACKWARD,
} play_state_t;

typedef struct {
    char music_path[128];
    int number;
} wav_musiclist_t;

typedef struct {
    char type;
    char parameter[128];
} a2dp_src_thread_args_t;

typedef struct {
    unsigned int riff_type;
    unsigned int riff_size;
    unsigned int wave_type;
    unsigned int format_type;
    unsigned int format_size;
    unsigned short compression_code;
    unsigned short num_channels;
    unsigned int sample_rate;
    unsigned int bytes_per_second;
    unsigned short block_align;
    unsigned short bits_per_sample;
    unsigned int data_type;
    unsigned int data_size;
} wav_header_t;

static bool a2dp_src_thread_run = false;
static bool a2dp_src_loop = false;
static bool is_folder_play = false;
wav_musiclist_t *musiclist = NULL;
static int musiclist_number = 1;
static a2dp_src_thread_args_t *thread_args = NULL;
static play_state_t play_state;
static bool dev_connnect = false;
static bool is_pause = true;

static play_state_t get_play_state(void)
{
    return play_state;
}

static void set_play_state(play_state_t state)
{
    play_state = state;
}

void btcli_avrcp_tg_play_state_cb(const char *bd_addr, btmg_avrcp_play_state_t state)
{
    if (state == BTMG_AVRCP_PLAYSTATE_STOPPED) {
        CMD_DBG("Receive event[STOP] from devices device: %s\n", bd_addr);
        set_play_state(STOP);
    } else if (state == BTMG_AVRCP_PLAYSTATE_PLAYING) {
        CMD_DBG("Receive event[PLAY] from devices device: %s\n", bd_addr);
        set_play_state(PLAY);
    } else if (state == BTMG_AVRCP_PLAYSTATE_PAUSED) {
        CMD_DBG("Receive event[PAUSE] from devices device: %s\n", bd_addr);
        set_play_state(PAUSE);
    } else if (state == BTMG_AVRCP_PLAYSTATE_FORWARD) {
        CMD_DBG("Receive event[FORWARD] from devices device: %s\n", bd_addr);
        if (!is_folder_play) {
            CMD_WRN("need specify the folder with -p to start\n");
            return;
        }
        set_play_state(FORWARD);
    } else if (state == BTMG_AVRCP_PLAYSTATE_BACKWARD) {
        CMD_DBG("Receive event[BACKWARD] from devices device: %s\n", bd_addr);
        if (!is_folder_play) {
            CMD_WRN("need specify the folder with -p to start\n");
            return;
        }
        set_play_state(BACKWARD);
    }
}

static int open_file(char *file_path)
{
    return open(file_path, O_RDONLY);
}

static void close_file(int fd)
{
    close(fd);
}

static int traverse_musiclist(char *foldpath)
{
    char file_format[8] = { 0 };
    DIR *record_dir = NULL;
    struct dirent *de = NULL;
    FILE *file = NULL;
    int file_count = 0;

    record_dir = opendir(foldpath);
    if (record_dir == NULL) {
        CMD_ERR("Path OPEN error \n");
        return -1;
    }

    if (musiclist != NULL) {
        free(musiclist);
    }

    musiclist_number = 1;
    musiclist = (wav_musiclist_t *)malloc(1 * sizeof(wav_musiclist_t));

    while ((de = readdir(record_dir)) != 0) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        } else if (de->d_type == 1) { /* file */
            int filelen = strlen(de->d_name);
            memset(file_format, '\0', sizeof(file_format));
            strncpy(file_format, de->d_name + filelen - 3, 3); /* 记录文件格式 */
            if (!strcmp("wav", file_format)) {
                wav_musiclist_t *ml = &musiclist[musiclist_number - 1];
                if (foldpath[strlen(foldpath) - 1] != '/')
                    sprintf(ml->music_path, "%s/%s", foldpath, de->d_name);
                else
                    sprintf(ml->music_path, "%s%s", foldpath, de->d_name);

                ml->number = musiclist_number;
                musiclist_number++;
                CMD_DBG("find path:%s\n", ml->music_path);

                wav_musiclist_t *new_musiclist;
                new_musiclist = (wav_musiclist_t *)realloc(
                        musiclist, musiclist_number * sizeof(wav_musiclist_t));
                if (new_musiclist == NULL) {
                    CMD_ERR("realloc fail\n");
                    free(musiclist);
                    musiclist = NULL;
                    return -1;
                } else {
                    musiclist = new_musiclist;
                }
            }
        }
    }

    closedir(record_dir);
    return 0;
}

static void btcli_a2dp_src_thread(void *arg)
{
    int ret = -1;
    int send_len = -1;
    int path_length = 0;
    wav_header_t wav_header;
    static int wav_number = 0;
    char buffer[A2DP_SRC_BUFF_SIZE] = { 0 };
    char muisc_path[128] = { 0 };
    unsigned int c = 0, written = 0, count = 0, len = 0;
    a2dp_src_thread_args_t *thread_arg = (a2dp_src_thread_args_t *)arg;
    char type = thread_arg->type;

    if (type == 'f') {
        strcpy(muisc_path, thread_arg->parameter);
    } else if (type == 'p' | type == 'P') {
        ret = traverse_musiclist(thread_arg->parameter);
        if (ret == -1) {
            CMD_ERR("traverse musiclist fail \n");
            goto failed;
        }
        strcpy(muisc_path, (const char *)&musiclist[wav_number].music_path);
    }

start:
    c = 0, written = 0, count = 0, len = 0;
    path_length = strlen(muisc_path);
    if (path_length < 5) { //File path meets at least length 5
        CMD_ERR("Please enter the correct file path \n");
        goto failed;
    }

    if (strcmp(".wav", &muisc_path[path_length - 4])) {
        CMD_ERR("Please enter the correct audio format - 'wav'\n");
        goto failed;
    }

    if (music_fd > 0) {
        CMD_DBG("close music fd\n");
        close_file(music_fd);
    }

    music_fd = open_file(muisc_path);
    if (music_fd < 0) {
        CMD_ERR("Cannot open input file\n");
    }

    ret = read(music_fd, &wav_header, sizeof(wav_header_t));
    if (ret != sizeof(wav_header_t)) {
        CMD_ERR("read wav file header failed\n");
        close_file(music_fd);
        goto failed;
    }

    CMD_DBG("btcli_a2dp_src_thread start !\n");
    memset(buffer, 0, sizeof(buffer));
    btmg_a2dp_source_set_audio_param(wav_header.num_channels, wav_header.sample_rate);
    count = wav_header.data_size;
    CMD_DBG("start a2dp src loop, data size:%d, ch:%d, sample:%d ,path: %s \n",
            wav_header.data_size, wav_header.num_channels, wav_header.sample_rate, muisc_path);

    a2dp_src_loop = true;
    set_play_state(PLAY);

    while (a2dp_src_loop) {
        switch (get_play_state()) {
        case PLAY:
            if (is_pause) {
                btmg_a2dp_source_play_start();
                is_pause = false;
            }
            if (send_len != 0) {
                c = count - written;
                if (c > A2DP_SRC_BUFF_SIZE) {
                    c = A2DP_SRC_BUFF_SIZE;
                }
                len = read(music_fd, buffer, c);
                if (len == 0) {
                    lseek(music_fd, 0, SEEK_SET);
                    written = 0;
                    CMD_DBG("audio file read complete\n");
                    continue;
                }
                if (len < 0) {
                    CMD_DBG("read file error,ret:%d,c=%d\n", len, c);
                    break;
                }
            } else {
                usleep(5);
            }
            if (len > 0) {
                if (get_play_state() != PLAY) {
                    CMD_DBG("!= PLAY\n");
                    usleep(10);
                    continue;
                }
                send_len = btmg_a2dp_source_send_data(buffer, len);
                written += send_len;
            }
            break;
        case PAUSE:
            if (!is_pause) {
                btmg_a2dp_source_play_stop(false);
                is_pause = true;
            }
            usleep(10 * 1000);
            break;
        case FORWARD:
            wav_number++;
            if (wav_number >= musiclist_number - 1) {
                wav_number = 0;
            }
            memset(muisc_path, 0, sizeof(muisc_path));
            strcpy(muisc_path, (const char *)&musiclist[wav_number].music_path);
            close_file(music_fd);
            music_fd = 0;
            a2dp_src_loop = false;
            goto start;
            break;
        case BACKWARD:
            wav_number--;
            if (wav_number < 0) {
                wav_number = musiclist_number - 1 - 1;
            }
            memset(muisc_path, 0, sizeof(muisc_path));
            strcpy(muisc_path, (const char *)&musiclist[wav_number].music_path);
            close_file(music_fd);
            music_fd = 0;
            a2dp_src_loop = false;
            goto start;
            break;
        case STOP:
            CMD_DBG("A2dp source play stop\n");
            a2dp_src_loop = false;
            break;
        default:
            break;
        }
    }

    close_file(music_fd);
    music_fd = 0;
failed:
    CMD_DBG("Delete a2dp source play thread\n");
    is_pause = true;
    btmg_a2dp_source_play_stop(true);
    if (musiclist != NULL) {
        free(musiclist);
        musiclist = NULL;
    }
    if (thread_args != NULL) {
        free(thread_args);
        thread_args = NULL;
    }
    a2dp_src_thread_run = false;
    XR_OS_ThreadDelete(&a2dp_play_thread);
    return;
}

void btcli_a2dp_source_audio_state_cb(const char *bd_addr, btmg_a2dp_source_audio_state_t state)
{
    if (state == BTMG_A2DP_SOURCE_AUDIO_STARTED) {
        set_play_state(PLAY);
        CMD_DBG("audio start %s\n", bd_addr);
    } else if (state == BTMG_A2DP_SOURCE_AUDIO_STOPPED) {
        set_play_state(PAUSE); //need PAUSE not STOP
        CMD_DBG("audio stop %s\n", bd_addr);
    } else if (state == BTMG_A2DP_SOURCE_AUDIO_SUSPENDED) {
        /*SUSPENDED protocol stack not used???*/
        set_play_state(PAUSE);
        CMD_DBG("audio suspend %s\n", bd_addr);
    }
}

void btcli_a2dp_source_connection_state_cb(const char *bd_addr,
                                           btmg_a2dp_source_connection_state_t state)
{
    if (state == BTMG_A2DP_SOURCE_DISCONNECTED) {
        /* It should be noted that there will be no disconnecting callback
        when the other party disconnects abnormally, so it is necessary to
        ensure that the thread can be released */
        set_play_state(STOP);
        dev_connnect = false;
        btmg_adapter_set_scanmode(BTMG_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        CMD_DBG("A2DP source disconnected with device: %s\n", bd_addr);
    } else if (state == BTMG_A2DP_SOURCE_CONNECTING) {
        CMD_DBG("A2DP source connecting with device: %s\n", bd_addr);
    } else if (state == BTMG_A2DP_SOURCE_CONNECTED) {
        dev_connnect = true;
        btmg_adapter_set_scanmode(BTMG_SCAN_MODE_NONE);
        CMD_DBG("A2DP source connected with device: %s\n", bd_addr);
    } else if (state == BTMG_A2DP_SOURCE_DISCONNECTING) {
        //The disconnecting stage should exit the playback thread
        set_play_state(STOP);
        CMD_DBG("A2DP source disconnecting with device: %s\n", bd_addr);
    }
}

/* btcli a2dp_src connect <device mac> */
enum cmd_status btcli_source_connect(char *cmd)
{
    int argc;
    char *argv[1];

    argc = cmd_parse_argv(cmd, argv, 1);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    btmg_a2dp_source_connect(argv[0]);

    return CMD_STATUS_OK;
}

/* btcli a2dp_src disconnect <device mac> */
enum cmd_status btcli_source_disconnect(char *cmd)
{
    int argc;
    char *argv[1];

    argc = cmd_parse_argv(cmd, argv, 1);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    btmg_a2dp_source_disconnect(argv[0]);

    return CMD_STATUS_OK;
}

/* btcli a2dp_src start -f xxx.wav or btcli a2dp_src start -p /data */
enum cmd_status btcli_source_start(char *cmd)
{
    int ret = -1, i = 0, argc = 0, len = 0;
    char *argv[2];

    if (!dev_connnect) {
        CMD_ERR("Please complete the device connection first\n");
        return CMD_STATUS_FAIL;
    }

    if (a2dp_src_thread_run == true) {
        CMD_WRN("a2dp_src_play_t thread already started\n");
        return CMD_STATUS_FAIL;
    }

    is_folder_play = false;

    argc = cmd_parse_argv(cmd, argv, 2);

    if (a2dp_src_loop == false) {
        if (thread_args)
            free(thread_args);
        thread_args = (a2dp_src_thread_args_t *)malloc(sizeof(a2dp_src_thread_args_t));
        if (strcmp("-f", argv[0]) == 0 || strcmp("-F", argv[0]) == 0) {
            thread_args->type = 'f';
            memset(thread_args->parameter, 0, sizeof(thread_args->parameter));
            strcpy(thread_args->parameter, argv[1]);
        } else if (strcmp("-p", argv[0]) == 0 || strcmp("-P", argv[0]) == 0) {
            thread_args->type = 'p';
            is_folder_play = true;
            memset(thread_args->parameter, 0, sizeof(thread_args->parameter));
            strcpy(thread_args->parameter, argv[1]);
        } else {
            CMD_ERR("please enter the parameters correctly \n");
            free(thread_args);
            thread_args = NULL;
            return CMD_STATUS_FAIL;
        }
    }
    XR_OS_ThreadCreate(&a2dp_play_thread, "a2dp_src_play_t", btcli_a2dp_src_thread,
                        thread_args, XR_OS_PRIORITY_NORMAL, 1024 * 6);
    a2dp_src_thread_run = true;

    return CMD_STATUS_OK;
}

/* btcli a2dp_src stop */
enum cmd_status btcli_source_stop(char *cmd)
{
    set_play_state(STOP);

    return CMD_STATUS_OK;
}

enum cmd_status btcli_source_control(char *cmd)
{
    int argc;
    char *argv[1];

    argc = cmd_parse_argv(cmd, argv, 1);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }

    if (!dev_connnect) {
        CMD_ERR("Please complete the device connection first\n");
        return CMD_STATUS_FAIL;
    }

    if (get_play_state() == STOP) {
        CMD_ERR("Please run first a2dp_src:  btcli a2dp_src start xxxx\n");
        return CMD_STATUS_FAIL;
    }

    if (strcmp("play", argv[0]) == 0) {
        set_play_state(PLAY);
    } else if (strcmp("pause", argv[0]) == 0) {
        set_play_state(PAUSE);
    } else if (strcmp("forward", argv[0]) == 0) {
        if (!is_folder_play) {
            CMD_ERR("need specify the folder with -p to start\n");
            CMD_STATUS_FAIL;
        }
        set_play_state(FORWARD);
    } else if (strcmp("backward", argv[0]) == 0) {
        if (!is_folder_play) {
            CMD_ERR("need specify the folder with -p to start\n");
            CMD_STATUS_FAIL;
        }
        set_play_state(BACKWARD);
    } else {
        CMD_ERR("This operation is not supported\n");
    }

    return CMD_STATUS_OK;
}

/* btcli a2dp_src vol */
enum cmd_status btcli_source_vol(char *cmd)
{
    int argc;
    char *argv[1];

    argc = cmd_parse_argv(cmd, argv, 1);
    if (argc < 1) {
        CMD_ERR("invalid param number %d\n", argc);
        return CMD_STATUS_INVALID_ARG;
    }
    int val = cmd_atoi(argv[0]);
    CMD_WRN("val is %d \n", val);
    if (val < 0 || val > 99) {
        CMD_ERR("return failed\n");
        return CMD_STATUS_FAIL;
    }

    return CMD_STATUS_OK;
}

void btcli_a2dp_source_check_state(void)
{
    if (get_play_state() == PLAY) {
        CMD_WRN("Ready to exit a2dp_src_play_t\n");
        set_play_state(STOP);
    }
}

/*
    $btcli a2dp_src connect <mac>
    $btcli a2dp_src disconnect <mac>
    $btcli a2dp_src start
    $btcli a2dp_src stop
    $btcli a2dp_src control
    $btcli a2dp_src vol <val>
*/
static const struct cmd_data a2dp_source_cmds[] = {
    { "connect",    btcli_source_connect,    CMD_DESC("<device mac>")},
    { "disconnect", btcli_source_disconnect, CMD_DESC("<device mac>")},
    { "start",      btcli_source_start,      CMD_DESC("<-p [folder path] or -f [file path]>")},
    { "control",    btcli_source_control,    CMD_DESC("<cmd>(pause play forward backward)")},
    { "stop",       btcli_source_stop,       CMD_DESC("No parameters")},
    { "vol",        btcli_source_vol,        CMD_DESC("<val>(val:0~100)")},
    { "help",       btcli_source_help,       CMD_DESC(CMD_HELP_DESC)},
};

/* btcli a2dp_src help */
static enum cmd_status btcli_source_help(char *cmd)
{
    return cmd_help_exec(a2dp_source_cmds, cmd_nitems(a2dp_source_cmds), 10);
}

enum cmd_status btcli_a2dp_source(char *cmd)
{
    return cmd_exec(cmd, a2dp_source_cmds, cmd_nitems(a2dp_source_cmds));
}
