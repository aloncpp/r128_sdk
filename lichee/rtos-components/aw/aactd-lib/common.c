#include "aactd/common.h"

#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include "lwip/sockets.h"
#include "lwip/netif.h"
#include "libc/errno.h"

#ifndef CONFIG_ARCH_DSP
#include "sys/endian.h"
#else
/* define default endianness */
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN    3456
#endif

#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN    6543
#endif

#ifndef __BYTE_ORDER
#define __BYTE_ORDER    __LITTLE_ENDIAN
#endif

#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define IS_LITTLE_ENDIAN
#elif __BYTE_ORDER == __BIG_ENDIAN
#define IS_BIG_ENDIAN
#else
#error "Unknown byte order"
#endif

void aactd_print_original_buf(const uint8_t *buf, int buf_len, int interval)
{
    int i, j, quotient, remain;
    const uint8_t *p = buf;

    printf("\n");
    printf("Original data (%d bytes):\n", buf_len);
    quotient = buf_len / interval;
    remain = buf_len % interval;
    for (i = 0; i < quotient; ++i) {
		for (j = 0; j < interval; ++j) {
	        printf("  0x%02x", p[i*interval + j]);
		}
		printf("\n");
    }
    p = buf + interval * quotient;
    for (i = 0; i < remain; ++i) {
        printf("  0x%02x", p[i]);
    }
    printf("\n");
    printf("\n");
}

ssize_t aactd_readn(int fd, void *buf, size_t bytes)
{
    size_t left_bytes = bytes;
    uint8_t *ptr = buf;
    ssize_t ret_bytes;

    while (left_bytes > 0) {
        ret_bytes = read(fd, ptr, left_bytes);
        if (ret_bytes < 0) {
            aactd_error("Read error: %d\n", errno);
            return -1;
        } else if (ret_bytes == 0) {
            aactd_info("Read EOF\n");
            break;
        }

        left_bytes -= ret_bytes;
        ptr += ret_bytes;
    }

    return (bytes - left_bytes);
}

ssize_t aactd_writen(int fd, void *buf, size_t bytes)
{
    size_t left_bytes = bytes;
    uint8_t *ptr = buf;
    ssize_t ret_bytes;

    while (left_bytes > 0) {
        ret_bytes = write(fd, ptr, left_bytes);
        if (ret_bytes < 0){
            aactd_error("Write error: %d\n", errno);
            return -1;
        } else if (ret_bytes == 0) {
            /*end of file*/
            break;
        }

        left_bytes -= ret_bytes;
        ptr += ret_bytes;
    }

    return (bytes - left_bytes);
}

uint16_t aactd_le_buf_to_uint16(const uint8_t *buf)
{
    union {
        uint16_t v;
        uint8_t u8[2];
    } u;
#ifdef IS_LITTLE_ENDIAN
    memcpy(u.u8, buf, 2);
#else
    u.u8[0] = buf[1];
    u.u8[1] = buf[0];
#endif
    return u.v;
}

int16_t aactd_le_buf_to_int16(const uint8_t *buf)
{
    union {
        int16_t v;
        uint8_t u8[2];
    } u;
#ifdef IS_LITTLE_ENDIAN
    memcpy(u.u8, buf, 2);
#else
    u.u8[0] = buf[1];
    u.u8[1] = buf[0];
#endif
    return u.v;
}

uint32_t aactd_le_buf_to_uint32(const uint8_t *buf)
{
    union {
        uint32_t v;
        uint8_t u8[4];
    } u;
#ifdef IS_LITTLE_ENDIAN
    memcpy(u.u8, buf, 4);
#else
    u.u8[0] = buf[3];
    u.u8[1] = buf[2];
    u.u8[2] = buf[1];
    u.u8[3] = buf[0];
#endif
    return u.v;
}

int32_t aactd_le_buf_to_int32(const uint8_t *buf)
{
    union {
        int32_t v;
        uint8_t u8[4];
    } u;
#ifdef IS_LITTLE_ENDIAN
    memcpy(u.u8, buf, 4);
#else
    u.u8[0] = buf[3];
    u.u8[1] = buf[2];
    u.u8[2] = buf[1];
    u.u8[3] = buf[0];
#endif
    return u.v;
}

uint32_t aactd_le_buf_to_uint64(const uint8_t *buf)
{
    union {
        uint64_t v;
        uint8_t u8[8];
    } u;
#ifdef IS_LITTLE_ENDIAN
    memcpy(u.u8, buf, 8);
#else
    u.u8[0] = buf[7];
    u.u8[1] = buf[6];
    u.u8[2] = buf[5];
    u.u8[3] = buf[4];
	u.u8[4] = buf[3];
	u.u8[5] = buf[2];
	u.u8[6] = buf[1];
	u.u8[7] = buf[0];
#endif
    return u.v;
}

int32_t aactd_le_buf_to_int64(const uint8_t *buf)
{
    union {
        int64_t v;
        uint8_t u8[8];
    } u;
#ifdef IS_LITTLE_ENDIAN
    memcpy(u.u8, buf, 8);
#else
	u.u8[0] = buf[7];
    u.u8[1] = buf[6];
    u.u8[2] = buf[5];
    u.u8[3] = buf[4];
	u.u8[4] = buf[3];
	u.u8[5] = buf[2];
	u.u8[6] = buf[1];
	u.u8[7] = buf[0];
#endif
    return u.v;
}

void aactd_uint16_to_le_buf(uint16_t value, uint8_t *buf)
{
#ifdef IS_LITTLE_ENDIAN
    memcpy(buf, &value, 2);
#else
    union {
        uint16_t v;
        uint8_t u8[2];
    } u;
    u.v = value;
    buf[0] = u.u8[1];
    buf[1] = u.u8[0];
#endif
}

void aactd_int16_to_le_buf(int16_t value, uint8_t *buf)
{
#ifdef IS_LITTLE_ENDIAN
    memcpy(buf, &value, 2);
#else
    uinon {
        int16_t v;
        uint8_t u8[2];
    } u;
    u.v = value;
    buf[0] = u.u8[1];
    buf[1] = u.u8[0];
#endif
}

void aactd_uint32_to_le_buf(uint32_t value, uint8_t *buf)
{
#ifdef IS_LITTLE_ENDIAN
    memcpy(buf, &value, 4);
#else
    uinon {
        uint32_t v;
        uint8_t u8[4];
    } u;
    u.v = value;
    buf[0] = u.u8[3];
    buf[1] = u.u8[2];
    buf[2] = u.u8[1];
    buf[3] = u.u8[0];
#endif
}

void aactd_int32_to_le_buf(int32_t value, uint8_t *buf)
{
#ifdef IS_LITTLE_ENDIAN
    memcpy(buf, &value, 4);
#else
    uinon {
        int32_t v;
        uint8_t u8[4];
    } u;
    u.v = value;
    buf[0] = u.u8[3];
    buf[1] = u.u8[2];
    buf[2] = u.u8[1];
    buf[3] = u.u8[0];
#endif
}

void aactd_uint64_to_le_buf(uint64_t value, uint8_t *buf)
{
#ifdef IS_LITTLE_ENDIAN
    memcpy(buf, &value, 8);
#else
    uinon {
        uint64_t v;
        uint8_t u8[8];
    } u;
    u.v = value;
    buf[0] = u.u8[7];
    buf[1] = u.u8[6];
    buf[2] = u.u8[5];
    buf[3] = u.u8[4];
    buf[4] = u.u8[3];
    buf[5] = u.u8[2];
    buf[6] = u.u8[1];
    buf[7] = u.u8[0];
#endif
}

void aactd_int64_to_le_buf(int64_t value, uint8_t *buf)
{
#ifdef IS_LITTLE_ENDIAN
    memcpy(buf, &value, 8);
#else
    uinon {
        int64_t v;
        uint8_t u8[8];
    } u;
    u.v = value;
    buf[0] = u.u8[7];
    buf[1] = u.u8[6];
    buf[2] = u.u8[5];
    buf[3] = u.u8[4];
    buf[4] = u.u8[3];
    buf[5] = u.u8[2];
    buf[6] = u.u8[1];
    buf[7] = u.u8[0];

#endif
}

uint8_t aactd_calculate_checksum(const uint8_t *buf, int buf_len)
{
    uint8_t sum = 0;
    int i;

    for (i = 0; i < buf_len; ++i) {
        sum += *(buf + i);
    }
    return sum;
}
