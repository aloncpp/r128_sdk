/*
 *
 *  Btetf RF Test Tool
 *
 *  Copyright(C), 2015, Xradio Technology Co., Ltd.
 *
 *
 */
#ifndef __BTETF_H
#define __BTETF_H

#ifdef __cplusplus
extern "C" {
#endif
#include <ctype.h>
// #include "ttyop.h"
// #include "../lib/bluetooth.h"
// #include "../lib/hci.h"
// #include "../lib/hci_lib.h"

// #ifndef BTETF_DEBUG
// #define BTETF_DEBUG TRUE
// #endif

#if 1
#define BTETF(param, ...) {printf(param, ## __VA_ARGS__);}
#else
#define BTETF(param, ...) {}
#endif

#define BD_ADDR_LEN     6
#define HCI_DBG_TX_RX_TEST_CMD_OPCODE        0xFC50
#define HCI_VSC_CMD_OPCODE                   0xFC07
#define HCI_SINGLE_TONE_OPEN                 0xFC43
#define HCI_LE_RF_TRIM_CAL_CMD_OPCODE        0x2020
#define HCI_LE_TX_POWER_CAL_CMD_OPCODE       0x2021

#define ACL_SCO_BASIC_RATE 0
#define P_NULL  0
#define POLL    1
#define FHS     2
#define DM1     3
#define DH1     4
#define HV1     5
#define HV2     6
#define HV3     7
#define DV      8
#define AUX1    9
#define DM3     10
#define DH3     11
#define DM5     14
#define DH5     15

#define eSCO_BASIC_RATE 1
#define EV3  7
#define EV4 12
#define EV5 15

#define ACL_EDR_RATE 2
#define P_2DH1 4
#define P_3DH1 8
#define P_2DH3 10
#define P_3DH3 11
#define P_2DH5 14
#define P_3DH5 15

#define eSCO_EDR_RATE 3
#define P_2EV3 6
#define P_3EV3 7
#define P_2EV5 12
#define P_3EV5 13

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define HCI_MAX_ACL_SIZE    1024
#define HCI_MAX_SCO_SIZE    255
#define HCI_MAX_EVENT_SIZE  260
#define HCI_MAX_FRAME_SIZE  (HCI_MAX_ACL_SIZE + 4)

/* HCI data types */
#define HCI_COMMAND_PKT     0x01
#define HCI_ACLDATA_PKT     0x02
#define HCI_SCODATA_PKT     0x03
#define HCI_EVENT_PKT       0x04
#define HCI_VENDOR_PKT      0xff

#define OCF_LE_RECEIVER_TEST            0x001D
#define OCF_LE_TRANSMITTER_TEST         0x001E
#define OGF_LE_CTL      0x08

/* 0x01: command, 0x02: acl data, 0x04:event */
#define ETF_HCI_CMD_PCKT 1
#define ETF_HCI_ACL_DATA_PCKT 2
#define ETF_HCI_EVNT_PCKT 4

#define ETF_OP(ogf, ocf)                         ((ocf) | ((ogf) << 10))
#define CONTRO_OGF_COM                              0x03
#define INFO_PARAM_OGF                              0x04
#define ETF_OGF_LE                                  0x08
#define ETF_OGF_VS                                  0x3f

#define RESET_OPCODE                                ETF_OP(CONTRO_OGF_COM, 0x0003)
#define READ_LOCAL_VERSION_OPCODE                   ETF_OP(INFO_PARAM_OGF, 0x0001)
#define LE_ETF_HCI_OP_LE_TX_TEST                    ETF_OP(ETF_OGF_LE, 0x001e)
#define LE_ETF_HCI_OP_LE_RX_TEST                    ETF_OP(ETF_OGF_LE, 0x001d)
#define LE_ETF_HCI_OP_LE_TEST_END                   ETF_OP(ETF_OGF_LE, 0x001f)
#define LE_ETF_HCI_OP_LE_ENH_RX_TEST                ETF_OP(ETF_OGF_LE, 0x0033)
#define LE_ETF_HCI_OP_LE_ENH_TX_TEST                ETF_OP(ETF_OGF_LE, 0x0034)
#define LE_ETF_HCI_SINGLE_TONE_OPEN                 ETF_OP(ETF_OGF_VS, 0x0043)
#define LE_ETF_HCI_SET_TEST_PWR_FEC_OPCODE          ETF_OP(ETF_OGF_VS, 0x0044)
#define LE_ETF_HCI_READ_RSSI                        ETF_OP(ETF_OGF_VS, 0x0015)
#define LE_ETF_HCI_OP_LE_SET_POWER                  ETF_OP(ETF_OGF_VS, 0x0303)
#define LE_ETF_HCI_OP_LE_SET_POWER_MAX              ETF_OP(ETF_OGF_VS, 0x0304)
#define LE_ETF_HCI_DBG_RD_MEM_CMD_OPCODE            ETF_OP(ETF_OGF_VS, 0x0001)
#define LE_ETF_HCI_DBG_WR_MEM_CMD_OPCODE            ETF_OP(ETF_OGF_VS, 0x0002)
#define ETF_HCI_DBG_RX_STOP_CMD_OPCODE              ETF_OP(ETF_OGF_VS, 0x0050)

#define CMD_OGF(pckt)     (((uint8_t)pckt[1]) >> 2)
#define CMD_OCF(pckt)     (((((uint16_t)pckt[1])&0x03) << 8 )| ((uint16_t)pckt[0]))

typedef union _opcode_t_ {
    unsigned int opcode;
    struct {
        unsigned short ocf : 10;
        unsigned short ogf : 6;
    } op;
} opcode_t;

typedef struct {
    uint8_t b[6];
} __attribute__((packed)) bdaddr_t;

struct hci_version {
   uint16_t manufacturer;
   uint8_t  hci_ver;
   uint16_t hci_rev;
   uint8_t  lmp_ver;
   uint16_t lmp_subver;
};

typedef struct hci_event_hdr {
    uint8_t    evt;
    uint8_t    plen;
} __attribute__ ((packed)) hci_event_hdr;

#define for_each_opt(opt, long, short) while ((opt=getopt_long(argc, argv, short ? short:"+", long, NULL)) != -1)

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

#define STREAM_TO_UINT32_NO_CHANGE_P(u32, p)                                       \
  {                                                                    \
    u32 = ((uint32_t)(*(p)) | (((uint32_t)(*((p) + 1))) << 8) | (((uint32_t)(*((p) + 2))) << 16) | (((uint32_t)(*((p) + 3))) << 24));      \
  }

#define BDADDR_TO_STREAM(p, a)   {register int ijk; for (ijk = 0; ijk < BD_ADDR_LEN;  ijk++) *(p)++ = (uint8_t) a[BD_ADDR_LEN - 1 - ijk];}

#define _8_Bit                              8
                 /// 16 bit access types
#define _16_Bit                             16
                 /// 32 bit access types
#define _32_Bit                             32

static int bachk(const char *str)
{
    if (!str)
        return -1;
    if (strlen(str) != 17)
        return -1;
    while (*str) {
        if (!isxdigit(*str++))
            return -1;
        if (!isxdigit(*str++))
            return -1;
        if (*str == 0)
            break;
        if (*str++ != ':')
            return -1;
    }
    return 0;
}

static int str2ba(const char *str, bdaddr_t *ba)
{
    int i;
    if (bachk(str) < 0) {
        memset(ba, 0, sizeof(*ba));
        return -1;
    }
    for (i = 5; i >= 0; i--, str += 3)
        ba->b[i] = strtol(str, NULL, 16);
    return 0;
}

#endif /* __BLUETOOTH_H */