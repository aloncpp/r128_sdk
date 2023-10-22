#ifndef __AACTD_COMMON_H__
#define __AACTD_COMMON_H__

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define AACTD_LIB_VERSION "AACTDLIB-V0.0.1"


#ifdef __cplusplus
extern "C" {
#endif

#define aactd_info(format, arg...) \
    do { \
        printf("[INFO](%s:%d): "format "\n" , __func__, __LINE__, ##arg); \
    } while (0)

#define aactd_error(format, arg...) \
    do { \
        printf("[ERROR](%s:%d): "format "\n" , __func__, __LINE__, ##arg); \
    } while (0)

void aactd_print_original_buf(const uint8_t *buf, int buf_len, int interval);

ssize_t aactd_readn(int fd, void *buf, size_t bytes);
ssize_t aactd_writen(int fd, void *buf, size_t bytes);

uint16_t aactd_le_buf_to_uint16(const uint8_t *buf);
int16_t aactd_le_buf_to_int16(const uint8_t *buf);
uint32_t aactd_le_buf_to_uint32(const uint8_t *buf);
int32_t aactd_le_buf_to_int32(const uint8_t *buf);
uint32_t aactd_le_buf_to_uint64(const uint8_t *buf);

void aactd_uint16_to_le_buf(uint16_t value, uint8_t *buf);
void aactd_int16_to_le_buf(int16_t value, uint8_t *buf);
void aactd_uint32_to_le_buf(uint32_t value, uint8_t *buf);
void aactd_int32_to_le_buf(int32_t value, uint8_t *buf);
void aactd_uint64_to_le_buf(uint64_t value, uint8_t *buf);

uint8_t aactd_calculate_checksum(const uint8_t *buf, int buf_len);

#ifdef __cplusplus
}
#endif

#endif /* ifndef __AACTD_COMMON_H__ */
