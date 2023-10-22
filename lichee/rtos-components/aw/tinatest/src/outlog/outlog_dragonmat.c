#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "aw_types.h"
#include "tinatest.h"
#include "outlog.h"
#include "../interact/interact.h"
#include "../socket_db/init_entry.h"
#include "../socket_db/socket_main.h"

// This file is used by DragonMAT to show list.
#define TIMEOUT 60000
#define DRAGONMAT_CONNECT_TIMEOUT_MS	15000

#define MSGTEXT_LEN_MAX		2048

typedef struct{
    long msgtype;
    char msgtext[MSGTEXT_LEN_MAX];
}msgstru;

/*
 * Little Endian to Big Endian.
 */
static inline unsigned int little2big(unsigned int value){
  return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 |
    (value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24;
}

static int dragonmat_show_msg_header(const char *buf){
	const struct head_packet_t *head = (struct head_packet_t *)buf;

	DEBUG("head->magic: %0x\n", head->magic);
	DEBUG("head->version: %d\n", head->version);
	DEBUG("head->type: %d\n", head->type);
	DEBUG("head->payloadsize: %d\n", head->payloadsize);
	DEBUG("head->externsize: %d\n", head->externsize);

	return 0;
}

static int dragonmat_show_msg_packet(const char *buf){
	DEBUG("packet: %s\n",buf + HEAD_SIZE);
	return 0;
}

static int dragonmat_msg_header_little2big(char *buf){
	struct head_packet_t *head = (struct head_packet_t *)buf;

	head->magic = little2big(head->magic);
	head->version = little2big(head->version);
	head->type = little2big(head->type);
	head->payloadsize = little2big(head->payloadsize);
	head->externsize = little2big(head->externsize);

	return 0;
}

/*
 * abstract out the send command public part
 * and it will be called by dragonmat_* interface
 * which need to send msg.
 */
static int dragonmat_snd_common(msgstru *msgs)
{
	int ret = -1;
	char *buf = msgs->msgtext;

	DEBUG("\n");
	DEBUG("==============================\n");
	dragonmat_show_msg_header(buf);
	dragonmat_show_msg_packet(buf);
	DEBUG("==============================\n");

	ret = send_to_sockt(buf, strlen(buf+HEAD_SIZE) + HEAD_SIZE);
	if (ret < 0) {
		ERROR("send to socket fail\n");
		return -1;
	}

	return 0;
}

/*
 * abstract out the revice command public part
 * and it will be called by dragonmat_* interface
 * which need to revice msg.
 */
static int dragonmat_rcv_common(msgstru *msgs)
{
	int ret = -1;
	char *buf = msgs->msgtext;
	
	ret = recv_from_socket(buf, HEAD_SIZE);
	if(ret < 0){
		ERROR("get data fail from socket\n");
		return -1;
	}

	dragonmat_msg_header_little2big(buf);
	DEBUG("\n");
	DEBUG("==============================\n");
	dragonmat_show_msg_header(buf);
	struct head_packet_t *head = (struct head_packet_t *)buf;
	if(head->magic != MAGIC){
		ERROR("head info error\n");
		return -1;
	}

	ret = recv_from_socket(buf+HEAD_SIZE, (head->payloadsize + head->externsize) );
	if(ret < 0){
		ERROR("get data info from socket fail\n");
		return -1;
	}
	dragonmat_show_msg_packet(buf);
	DEBUG("==============================\n");

	return 0;
}
pthread_mutex_t dragonmat_reply_mutex = FREERTOS_POSIX_MUTEX_INITIALIZER;
/*
 * integrated send & revice command
 */
static int dragonmat_reply(msgstru *msgs)
{
	int ret = -1;
    pthread_mutex_lock(&dragonmat_reply_mutex);
	usleep(50*1000);
    if ((dragonmat_snd_common(msgs)) < 0) {
        ERROR("dragonmat_reply: snd failed!\n");
        goto exit;
    }

    if ((dragonmat_rcv_common(msgs)) < 0) {
        ERROR("dragonmat_reply: rcv failed!\n");
        goto exit;
    }
	ret = 0;
exit:
	usleep(50*1000);
    pthread_mutex_unlock(&dragonmat_reply_mutex);
    return 0;
}

static int dragonmat_before_all(struct list_head *TASK_LIST)
{
	DEBUG("=\n");
    return 0;
}

static int dragonmat_before_one(struct tt_struct *tt)
{
	DEBUG("=\n");
    msgstru msgs;
    memset(&msgs, 0, sizeof(msgstru));

    DEBUG("%s start\n", tt->name);
    msgs.msgtype = (long)fill_msg_start(msgs.msgtext, MSGTEXT_LEN_MAX, tt->name);
    if (dragonmat_reply(&msgs) < 0) {
        ERROR("dragonmat reply failed!\n");
        return -1;
    }

    return 0;
}

static int dragonmat_after_one_end(struct tt_struct *tt)
{
	DEBUG("=\n");
    int result = 0;
    msgstru msgs;
    memset(&msgs, 0, sizeof(msgstru));

    result = tt->result;

    DEBUG("%s end with %d\n", tt->name, result);
    msgs.msgtype = (long)fill_msg_end(msgs.msgtext, MSGTEXT_LEN_MAX, tt->name, (result==0)?PASS:FAILED, "");
    if (dragonmat_reply(&msgs) < 0) {
        ERROR("dragonmat reply failed!\n");
        return -1;
    }

    return 0;
}

static int dragonmat_after_all(struct list_head *TASK_LIST)
{
	DEBUG("=\n");
    int result = 0;
    msgstru msgs;
    memset(&msgs, 0, sizeof(msgstru));
    struct tt_struct *tt = NULL;

    list_for_each_entry(tt, TASK_LIST, node) {
	    if (tt->result != 0) {
		    result = -1;
		    break;
	    }
    }

    if(!result){
	int try_times = 5;
        while(try_times--){
            if(access(TINATEST_PATH, F_OK)!=0){
                //TINATEST_PATH not exist, mkdir
                mkdir(TINATEST_PATH, 0777);
            }
            int fd = open(TINATEST_OK_FILE, O_RDONLY);
            if(fd>=0){
                ERROR("flag %s exist!\n", TINATEST_OK_FILE);
                close(fd);
                break;
            }
            fprintf(stdout,"touch %s ...\n", TINATEST_OK_FILE);
            fflush(stdout);
            fd = open(TINATEST_OK_FILE, O_WRONLY | O_CREAT);
            if(fd<0){
                ERROR("touch %s failed!\n", TINATEST_OK_FILE);
                continue;
            }
            fsync(fd);
            close(fd);
            fd = open(TINATEST_OK_FILE, O_RDONLY);
            if(fd<0){
                ERROR("check %s failed!\n", TINATEST_OK_FILE);
                continue;
            }
            close(fd);
            break;
        }
        fprintf(stdout,"touch %s ok\n", TINATEST_OK_FILE);
        fflush(stdout);
    }

    msgs.msgtype = (long)fill_msg_finish(msgs.msgtext, MSGTEXT_LEN_MAX, (result==0)?PASS:FAILED, "");
    if (dragonmat_reply(&msgs) < 0) {
        ERROR("dragonmat reply failed!\n");
        return -1;
    }

    return 0;
}

/*
 * FIXME: dragonmat_ask interface are not complete.
 * It will be achieve until libsocket_db add function
 * that get user input string on dragonmat
 */
static int dragonmat_ask(const char *testcase, const char *tips, char *reply, int len)
{
    int ret = -1;

    msgstru msgs;
    memset(&msgs, 0, sizeof(msgstru));

    msgs.msgtype = (long)fill_msg_edit(msgs.msgtext, MSGTEXT_LEN_MAX, testcase, tips, reply, "", TIMEOUT);
    if (dragonmat_reply(&msgs) < 0) {
        ERROR("dragonmat reply failed!\n");
        return -1;
     }
    ret = getValue_fix(msgs.msgtext + HEAD_SIZE, "editvalue", reply, &len) == 1 ? 0 : -1;
    printf("editvalue %s\n", reply);

    return ret;
}

/*
 * send select CMD to user, people will choose (Yes/No)
 * on dragonmat.
 * @testcase: testcase name
 * @tips: tips message to user
 *
 * return: 1(Yes) / 0(No)
 */
static int dragonmat_istrue(const char *testcase, const char *tips)
{
    int ret = -1;

    msgstru msgs;
    memset(&msgs, 0, sizeof(msgstru));

    msgs.msgtype = (long)fill_msg_select(msgs.msgtext, MSGTEXT_LEN_MAX, testcase, tips, "", TIMEOUT);
    if (dragonmat_reply(&msgs) < 0) {
        ERROR("dragonmat reply failed!\n");
        return -1;
    }

    ret = getReturn_fix(msgs.msgtext + HEAD_SIZE) == 1 ? true : false;

    DEBUG("is true return :%d\n", ret);
    return ret;
}
/*
 * send tips message to user.
 * @testcase: testcase name
 * @tips: tips message to user
 *
 * return: -1(failed) / 0(success)
 */
static int dragonmat_tips(const char *testcase, const char *tips)
{
    msgstru msgs;
    memset(&msgs, 0, sizeof(msgstru));
/*
    msgstru *msgs = (msgstru *)malloc(sizeof(msgstru));
    if (NULL == msgs) {
        ERROR("dragonmat_tips: malloc failed! - %s\n", strerror(errno));
        goto err;
    }
*/
    msgs.msgtype = (long)fill_msg_tip(msgs.msgtext, MSGTEXT_LEN_MAX, testcase, tips);
    if (dragonmat_reply(&msgs) < 0) {
        ERROR("dragonmat reply failed!\n");
        return -1;
    }

    return 0;
}

struct dragonmat_data_t{
	struct socket_data_t *socket_data;
};

struct dragonmat_data_t *dragonmat_module_init(volatile int *exit)
{
	struct dragonmat_data_t *data = malloc(sizeof(struct dragonmat_data_t));
	if(!data){
		ERROR("malloc failed\n");
		goto err1;
	}

	data->socket_data = socket_init(DRAGONMAT_CONNECT_TIMEOUT_MS, exit);
	if (!data->socket_data) {
		ERROR("init socket_db failed\n");
		goto err2;
	}

	outlog_register_ex(
			dragonmat_before_all,
			dragonmat_before_one,
			NULL,
			NULL,
			dragonmat_after_one_end,
			dragonmat_after_all);

	interact_register(
			dragonmat_ask,
			dragonmat_tips,
			dragonmat_istrue);

	return data;
err2:
	free(data);
err1:
	return NULL;
}

int dragonmat_module_exit(struct dragonmat_data_t *data){
	if(data){
		socket_exit(data->socket_data);
		free(data);
	}
	return 0;
}