#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "tinatest.h"
#include "socket_main.h"
#include "init_entry.h"

#define PACKET_HEADLEN  20

static inline int fill_head_new(char *buf, int payloadsize, int externsize, int type)
{
	struct head_packet_t *head = (struct head_packet_t *)buf;
    head->magic = MAGIC;
    head->externsize = externsize;
    head->payloadsize = payloadsize;
    head->type = 1;
    head->version = 0x00000001;

    return 0;
}

pthread_mutex_t msg_id_mutex = FREERTOS_POSIX_MUTEX_INITIALIZER;
static inline int getMsgid(void)
{
	static int id_num =100 ;
	int msgid;

	pthread_mutex_lock(&msg_id_mutex);
	msgid = id_num;
	id_num++;
	pthread_mutex_unlock(&msg_id_mutex);

	return msgid;
}

int fill_msg_start(char *buf, int limit, const char *testname){
	if(!testname || !buf || (limit<=HEAD_SIZE))
		return -1;
    int msgid = getMsgid();
	int len = fill_packet_start(buf + HEAD_SIZE, limit - HEAD_SIZE, testname, msgid);
	fill_head_new(buf, len, 0, 1);
    return msgid;
}

int fill_msg_end(char *buf, int limit, const char *testname, int result, const char *mark){
	if(!testname || !buf || (limit<=HEAD_SIZE))
		return -1;
    int msgid = getMsgid();
	int len = fill_packet_end(buf + HEAD_SIZE, limit - HEAD_SIZE, testname, msgid, result, mark);
	fill_head_new(buf, len, 0, 1);
    return msgid;
}

int fill_msg_finish(char *buf, int limit, int result, const char *mark){
	if(!buf || (limit<=HEAD_SIZE))
		return -1;
    int msgid = getMsgid();
	int len = fill_packet_finish(buf + HEAD_SIZE, limit - HEAD_SIZE, msgid, result, mark);
	fill_head_new(buf, len, 0, 1);
    return msgid;
}

int fill_msg_edit(char *buf, int limit, const char *testname, const char *tip, const char *editvalue, const char *mark, int timeout){
	if(!testname || !buf || (limit<=HEAD_SIZE))
		return -1;
    int msgid = getMsgid();
	int len = fill_packet_edit(buf + HEAD_SIZE, limit - HEAD_SIZE, testname, msgid, tip, editvalue, mark, timeout);
	fill_head_new(buf, len, 0, 1);
    return msgid;
}

int fill_msg_select(char *buf, int limit, const char *testname, const char *tip, const char *mark, int timeout){
	if(!testname || !buf || (limit<=HEAD_SIZE))
		return -1;
    int msgid = getMsgid();
	int len = fill_packet_select(buf + HEAD_SIZE, limit - HEAD_SIZE, testname, msgid, tip, mark, timeout);
	fill_head_new(buf, len, 0, 1);
    return msgid;
}

int fill_msg_tip(char *buf, int limit, const char *testname, const char *tip){
	if(!testname || !buf || (limit<=HEAD_SIZE))
		return -1;
    int msgid = getMsgid();
	int len = fill_packet_tip(buf + HEAD_SIZE, limit - HEAD_SIZE, testname, msgid, tip);
	fill_head_new(buf, len, 0, 1);
    return msgid;
}


int getReturn_fix(char *buf)
{
    int ret;
    char szVal[1024];

    GetXmlNode(buf, "response", "result", szVal);

    if (strcasecmp(szVal, "ok") != 0)
    {
	    DEBUG("ttrue result : fail\n");
        return 0;
    }
    else
    {
	    DEBUG("ttrue result : ok\n");
        return 1;
    }
}


int getValue_fix(char* buf, const char* szKey, char*szValue, int *nLen)
{
    int ret;
    char value[512];
    char node[32];

    memset(value,0x0,sizeof(value));
    strcpy(node, szKey);
    ret = GetXmlNode(buf, "response", node, value);
    if(ret == -1)
    {
        ERROR("get value fialure\n");
        return ret;
    }

    strcpy(szValue, value);
    DEBUG("szValue : %s\n",szValue);

    *nLen = strlen(szValue);

    return 1;
}

