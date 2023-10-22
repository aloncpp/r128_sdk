/*
 *
 *  Btetf RF Test Tool
 *
 *  Copyright(C), 2015, Xradio Technology Co., Ltd.
 *
 *
 */
#ifndef __HCIDUMP_XRADIO_H
#define __HCIDUMP_XRADIO_H


#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#if 0
void init_vendor_xr(int device);
void Avdtp_coex_start_cmd(uint16_t acl_handle, uint8_t in);
void Avdtp_coex_stop_cmd(uint16_t acl_handle, uint8_t in);
#endif

/* Modes */
enum {
    DISABLE,
    PARSE,
    READ,
    WRITE,
    PPPDUMP,
    AUDIO
};

void hcidump_start_up(int set_mode);
int process_hci_data(int mode, uint8_t *data, uint32_t len);
int hcidump_process(uint8_t type, const uint8_t *packet, uint8_t is_received);
void hcidump_shut_down(void);

/* Do not define the following macro because stack/bt_types has already defined*/
#if 0
#define UINT8_TO_STREAM(p, u8) \
  { *(p)++ = (uint8_t)(u8); }


#define UINT16_TO_STREAM(p, u16)    \
  {                                 \
    *(p)++ = (uint8_t)(u16);        \
    *(p)++ = (uint8_t)((u16) >> 8); \
  }

#define STREAM_TO_UINT8(u8, p) \
  {                               \
    (u8) = (uint8_t)(*(p));       \
    (p) += 1;                     \
  }

#define STREAM_TO_UINT16(u16, p)                                       \
  {                                                                    \
    u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8));        \
    (p) += 2;                                                          \
  }

#define STREAM_TO_UINT32(u32, p)                                       \
  {                                                                    \
    u32 = ((uint32_t)(*(p)) | (((uint32_t)(*((p) + 1))) << 8) | (((uint32_t)(*((p) + 2))) << 16) | (((uint32_t)(*((p) + 3))) << 24));      \
    (p) += 4;                                                        \
  }

#define BDADDR_TO_STREAM(p, a)   {register int ijk; for (ijk = 0; ijk < BD_ADDR_LEN;  ijk++) *(p)++ = (uint8_t) a[BD_ADDR_LEN - 1 - ijk];}
#endif

#endif
