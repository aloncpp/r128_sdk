#ifndef __INIT_ENTRY_H
#define __INIT_ENTRY_H

#include "xml_packet.h"

#define PASS 1
#define FAILED 0


#define MAGIC 0x54545450
#define VERSION 0x00000001

//20 byte head info
struct head_packet_t{
    unsigned int magic;
    unsigned int version;
    unsigned int type;
    unsigned int payloadsize;
    unsigned int externsize;
};
#define HEAD_SIZE (sizeof(struct head_packet_t))

//head file for init_entry.c
int init_entry();

int fill_msg_start(char *buf, int limit, const char *testname);
int fill_msg_end(char *buf, int limit, const char *testname, int result, const char *mark);
int fill_msg_finish(char *buf, int limit, int result, const char *mark);
int fill_msg_edit(char *buf, int limit, const char *testname, const char *tip, const char *editvalue, const char *mark, int timeout);
int fill_msg_select(char *buf, int limit, const char *testname, const char *tip, const char *mark, int timeout);
int fill_msg_tip(char *buf, int limit, const char *testname, const char *tip);

/* fix bug for testcase */
int sendCMDOperator_fix(const char *testname, const char *plugin, const char * datatype, char * data, char *buf);

int getPacketType_fix(char *buf, char *type);
int getValue_fix(char *buf, const char* szKey, char*szValue, int *nLen);
int getReturn_fix(char *buf);

#endif
