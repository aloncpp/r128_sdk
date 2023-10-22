#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "msgbuffer.h"

void updateStatus(MsgBuf *msgBuffer, int status)
{
    msgBuffer->status = status;
}

int msg_buf_read(MsgBuf *msgBuffer, void *buffer, int len)
{
    memcpy((char *)buffer, msgBuffer->data + msgBuffer->rpos, len);
    msgBuffer->rpos += len;
    return 0;
}

int msg_buf_write(MsgBuf *msgBuffer, void *buffer, int len)
{
    int ret = 0;
    char *ptr;

    if ((msgBuffer->wpos + len) <= msgBuffer->size)
    {
        memcpy(msgBuffer->data + msgBuffer->wpos, (char *)buffer, len);
        msgBuffer->wpos += len;
        return 0;
    }
    else
    {
        int newsize = (msgBuffer->wpos + len) + MSGBUFFER_STEP_SIZE;
        ptr = realloc(msgBuffer->data, newsize);
        if (!ptr)
        {
            return -1;
        }
        msgBuffer->data = ptr;
        memcpy(msgBuffer->data + msgBuffer->wpos, (char *)buffer, len);
        msgBuffer->wpos += len;
        msgBuffer->size = newsize;
    }
    return ret;
}

int writeBinary(MsgBuf *msgBuffer, int len, uint8_t *data)
{
    msgBuffer->write(msgBuffer, &len, sizeof(len));
    msgBuffer->write(msgBuffer, data, len);
    return 0;
}

int readBinary(MsgBuf *msgBuffer, int *len, uint8_t **data)
{
    msgBuffer->read(msgBuffer, len, 4);
    *data = (uint8_t *)msgBuffer->data + msgBuffer->rpos;
    return 0;
}

int writeData(MsgBuf *msgBuffer, const void *value, uint32_t length)
{
    msgBuffer->write(msgBuffer, (void *)value, length);
    return 0;
}

int readData(MsgBuf *msgBuffer, const void *value, uint32_t length)
{
    msgBuffer->read(msgBuffer, (void *)value, length);
    return 0;
}

int writePtr(MsgBuf *msgBuffer, uintptr_t value)
{
    uint32_t ptrSize = 4;

    msgBuffer->write(msgBuffer, &ptrSize, sizeof(ptrSize));

    writeData(msgBuffer, &value, ptrSize);
    return 0;
}

int readPtr(MsgBuf *msgBuffer, uintptr_t *value)
{
    uint32_t ptrSize = sizeof(*value);

    msgBuffer->read(msgBuffer, &ptrSize, sizeof(ptrSize));

    readData(msgBuffer, value, sizeof(ptrSize));
    return 0;
}

int writeString(MsgBuf *msgBuffer, uint32_t length, const char *value)
{
    writeBinary(msgBuffer, length, (uint8_t *)(value));
    return 0;
}

int readString(MsgBuf *msgBuffer, uint32_t *length, const char **value)
{
    readBinary(msgBuffer, (int *)length, (uint8_t **)(value));
    return 0;
}

int startWriteList(MsgBuf *msgBuffer, uint32_t length)
{
    // Write the list length as a u32.
    msgBuffer->write(msgBuffer, &length, sizeof(length));
    return 0;
}

int startReadList(MsgBuf *msgBuffer, uint32_t *length)
{
    msgBuffer->read(msgBuffer, length, sizeof(*length));
    return 0;
}

int startWriteUnion(MsgBuf *msgBuffer, int32_t discriminator)
{
    // Write the union discriminator as a u32.
    msgBuffer->write(msgBuffer, &discriminator, sizeof(discriminator));
    return 0;
}

int startReadUnion(MsgBuf *msgBuffer, int32_t *discriminator)
{
    msgBuffer->read(msgBuffer, discriminator, sizeof(*discriminator));
    return 0;
}

int readNullFlag(MsgBuf *msgBuffer, bool *isNull)
{
    uint8_t flag;

    msgBuffer->read(msgBuffer, &flag, sizeof(flag));
    return 0;
}

int writeNullFlag(MsgBuf *msgBuffer, bool isNull)
{
    uint8_t flag = isNull ? kIsNull : kNotNull;
    msgBuffer->write(msgBuffer, &flag, sizeof(flag));
    return 0;
}

typedef void *funPtr;          // Pointer to functions
typedef funPtr *arrayOfFunPtr; // Pointer to array of functions

int writeCallback(MsgBuf *msgBuffer, arrayOfFunPtr callbacks, uint8_t callbacksCount, funPtr callback)
{
    uint8_t i;

    // callbacks = callbacks table
    for (i = 0; i < callbacksCount; i++)
    {
        if (callbacks[i] == callback)
        {
            msgBuffer->write(msgBuffer, &i, sizeof(uint8_t));
            break;
        }
        if ((i + 1U) == callbacksCount)
        {
            updateStatus(msgBuffer, kErpcStatus_UnknownCallback);
        }
    }
    return 0;
}

int readCallback(MsgBuf *msgBuffer, arrayOfFunPtr callbacks, uint8_t callbacksCount, funPtr *callback)
{
    uint8_t _tmp_local;

    msgBuffer->read(msgBuffer, &_tmp_local, sizeof(_tmp_local));
    //if (isStatusOk())
    {
        if (_tmp_local < callbacksCount)
        {
            *callback = callbacks[_tmp_local];
        }
        else
        {
            //m_status = kErpcStatus_UnknownCallback;
        }
    }
    return 0;
}

void msg_buf_destroy(MsgBuf *msgBuffer)
{
    if (msgBuffer->data)
    {
        amp_free(msgBuffer->data);
    }
    amp_free(msgBuffer);
}

static int getStatus(MsgBuf *msgBuffer)
{
    return msgBuffer->status;
}

MsgBuf *msg_buf_create(void)
{
    MsgBuf *msgBuffer = amp_malloc(sizeof(MsgBuf));
    if (!msgBuffer)
    {
        return NULL;
    }
    memset(msgBuffer, 0, sizeof(MsgBuf));
    msgBuffer->read = msg_buf_read;
    msgBuffer->write = msg_buf_write;
    msgBuffer->getStatus = getStatus;
    msgBuffer->rpos = sizeof(MsgBufHeader);
    msgBuffer->wpos = sizeof(MsgBufHeader);
    msgBuffer->size = DEFAULT_MSG_BUFFER_SIZE;
    msgBuffer->data = amp_malloc(DEFAULT_MSG_BUFFER_SIZE);
    if (!msgBuffer->data)
    {
        amp_free(msgBuffer);
        return NULL;
    }
    memset(msgBuffer->data, 0, DEFAULT_MSG_BUFFER_SIZE);
    return msgBuffer;
}

MsgBuf *msg_buf_init(MsgBufHeader *header)
{
    MsgBuf *msgBuffer = amp_malloc(sizeof(MsgBuf));
    if (!msgBuffer)
    {
        return msgBuffer;
    }
    memset(msgBuffer, 0, sizeof(MsgBuf));
    msgBuffer->read = msg_buf_read;
    msgBuffer->write = msg_buf_write;
    msgBuffer->getStatus = getStatus;
    msgBuffer->rpos = sizeof(MsgBufHeader);
    msgBuffer->wpos = sizeof(MsgBufHeader);
    msgBuffer->size = header->bufferSize;
    msgBuffer->data = (char *)header;
    return msgBuffer;
}

int msg_buf_deinit(MsgBuf *msgBuffer)
{
    if (msgBuffer)
    {
        amp_free(msgBuffer);
    }
    return 0;
}
