#ifndef _SUNXI_AMP_MSG_H
#define _SUNXI_AMP_MSG_H

#define SUNXI_AMP_MAX_ARGS_NUM (8)

enum MSG_TYPE
{
    MSG_SERIAL_FUNC_CALL = 0,
    MSG_SERIAL_FUNC_RET,
    MSG_SERIAL_FREE_BUFFER,
    MSG_DIRECT_FUNC_CALL,
    MSG_DIRECT_FUNC_RET,
    MSG_TYPE_NUM,
};

enum FUNC_RETURN_TYPE
{
    RET_NULL = 0,
    RET_POINTER,
    RET_NUMBER_32,
    RET_NUMBER_64,
};

enum RPC_MSG_DIRECTION
{
    RPC_MSG_DIR_UNKNOWN = 0,
    RPC_MSG_DIR_CM33 = 1,
    RPC_MSG_DIR_RV,
    RPC_MSG_DIR_DSP,
};

typedef struct _sunxi_amp_msg_args_t
{
    uint32_t args_num: 8;
    uint32_t reserved;
    uint32_t args[SUNXI_AMP_MAX_ARGS_NUM];
} __attribute__((packed)) sunxi_amp_msg_args;

typedef struct _sunxi_amp_msg_t
{
    uint32_t rpcid: 16;
    uint32_t prio: 5;
    uint32_t type: 5;
    uint32_t src: 3;
    uint32_t dst: 3;
    uint32_t data;
    uint32_t flags;
} __attribute__((packed)) sunxi_amp_msg;

typedef struct _MsgBuf MsgBuf;
struct _MsgBuf
{
    int rpos;
    int wpos;
    int size;
    char *data;
    int status;
    int (*read)(MsgBuf *msgBuf, void *buffer, int len);
    int (*write)(MsgBuf *msgBuf, void *buffer, int len);
    int (*getStatus)(MsgBuf *msgBuf);
} __attribute__((packed));

typedef struct _MsgBufHead
{
    sunxi_amp_msg msg;
    int msgSize;
    int bufferSize;
    unsigned int crc16;
} __attribute__((packed)) MsgBufHeader;

typedef int (*sunxi_amp_service_func)(sunxi_amp_msg *msg, MsgBuf *msgbuf, MsgBuf **sendBuffer);

typedef struct _sunxi_amp_func_table
{
    uint32_t args_num: 8;
    uint32_t return_type: 8;
    sunxi_amp_service_func func;
} sunxi_amp_func_table;

MsgBuf *msg_buf_init(MsgBufHeader *header);
int msg_buf_deinit(MsgBuf *msgBuffer);

#endif
