/**
  * @file  cmd_ble_etf.c
  * @author  XRADIO Bluetooth Team
  */

/*
 * Copyright (C) 2019 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
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

/*
 * ===========================================================================
 *
 * -------------------------- temporary --------------------------------------
 *
 * ===========================================================================
 */
// #ifdef PRJCONF_BLE_ETF

// #define CONFIG_BTBLE_ETF_R128 1
#define CONFIG_BTBLE_ETF_XR829 0
#if 1
#include <stdio.h>
#include <console.h>
#include "cmd_util.h"

#include "kernel/os/os.h"
#include "btetf.h"

#include "bt_lib.h"
#include "hal_hci.h"
#include "hal_controller.h"

uint32_t read_start_addr = 0;

static const bt_lib_interface_t *etf_lib_if;
static uint8_t etf_enable;
static int etf_id;

static uint8_t chip_xr829_flag;
static uint8_t bt_rx_flag;

#define BLE_ETF_HCI_CMD_TIMEOUT   2000
#define HCI_TRACE_PRINTK_MAX_SIZE 128
#define DATA_TYPE_COMMAND         1

#define ETF_TRACE_LEVEL_ERROR     (0)
#define ETF_TRACE_LEVEL_WARNING   (1)
#define ETF_TRACE_LEVEL_DEBUG     (2)

#define ETF_DBG_LEVEL             ETF_TRACE_LEVEL_DEBUG
#define ETF_TRACE_ERROR(fmt, args...)       {if (ETF_DBG_LEVEL >= ETF_TRACE_LEVEL_ERROR)   printf(fmt, ##args);}
#define ETF_TRACE_WARNING(fmt, args...)     {if (ETF_DBG_LEVEL >= ETF_TRACE_LEVEL_WARNING) printf(fmt, ##args);}
#define ETF_TRACE_DEBUG(fmt, args...)       {if (ETF_DBG_LEVEL >= ETF_TRACE_LEVEL_DEBUG)   printf(fmt, ##args);}

static XR_OS_Thread_t g_hopping_thread;
#define THREAD_STACK_SIZE       (2 * 1024)

#define BT_VERSION_4_1 7
#define BT_VERSION_4_2 8
#define BT_VERSION_5_0 9

#define ETF_SUPPORT_MAX_POWER_LEVEL    15
#define ETF_DEFAULT_POWER_LEVEL    9
#define ETF_MAX_SETTING_POWER_LEVEL    7

typedef struct {
    uint8_t  chan;
    uint8_t  payload_len;
    uint8_t  payload_type;
    uint8_t  phy;
    uint8_t  mod_index;
    uint8_t  power_level;
    uint8_t  hopping_enable;
} ble_etf_config;

typedef struct {
    bdaddr_t  bdaddr;
    uint8_t  pattern;
    uint16_t  packet_len;
    uint8_t  chan;
    uint8_t  power_level;
    uint8_t  link_type;
    uint8_t  packet_type;
    uint8_t  hopping_mode;
} bt_etf_config;

struct dbg_rd_mem_cmd {
    ///Start address to read
    uint32_t start_addr;
    ///Access size
    uint8_t type;
    ///Length to read
    uint8_t length;
};

struct buffer_tag {
    /// length of buffer
    uint8_t length;
    /// data of 128 bytes length
    uint8_t data[HCI_TRACE_PRINTK_MAX_SIZE];
};

struct dbg_wr_mem_cmd {
    ///Start address to read
    uint32_t start_addr;
    ///Access size
    uint8_t type;
    ///buffer structure to return
    struct buffer_tag buf;
};

static bt_etf_config bt_etf_cfg = {
    .bdaddr.b = {0, 0, 0, 0, 0, 0},
    .pattern = 0,
    .packet_len = 1021,
    .chan = 0,
    .power_level = ETF_DEFAULT_POWER_LEVEL,
    .link_type = 0,
    .packet_type = 3,
    .hopping_mode = 0,
};

static ble_etf_config ble_etf_cfg = {
    .chan = 0,
    .payload_len = 37,
    .payload_type = 0,
    .phy = 0x01,
    .mod_index = 0x00,
    .power_level = ETF_DEFAULT_POWER_LEVEL,
    .hopping_enable = 0,
};

static void etf_send(uint8_t *buf, uint8_t len)
{
    ETF_TRACE_DEBUG("< cmd: ");
    for (uint16_t i = 0; i < len; i++) {
        ETF_TRACE_DEBUG("%02X ", *(buf + i));
    }
    ETF_TRACE_DEBUG("\n");

    etf_lib_if->hci_ops->write(etf_id, *buf, buf + 1, len - 1);
}


static void etf_hci_printf(uint8_t *buf, uint16_t len)
{

    ETF_TRACE_DEBUG("> event: ");
    for (uint8_t i = 0; i < len; i++) {
        ETF_TRACE_DEBUG("%02X ", buf[i]);
    }
    ETF_TRACE_DEBUG("\n");
}

static void test_result(uint8_t *p, uint8_t len)
{
    uint8_t  status;
    uint8_t  offset = 5;//event_type(1) + event_code(1) + para_len(1) + NCP(1) + opcdoe(2);
    uint8_t  mode_status;
    uint32_t receive_packet = 0;
    uint32_t hec_err_packet = 0;
    uint32_t crc_err_packet = 0;
    uint32_t type_err_packet = 0;
    p = p + offset;
    STREAM_TO_UINT8(status, p);
    STREAM_TO_UINT8(mode_status, p);

    if ((mode_status != 0xf0) || (!bt_rx_flag))//rx have been started, now would stop
        return;

    STREAM_TO_UINT32(receive_packet, p);
    STREAM_TO_UINT32(hec_err_packet, p);
    STREAM_TO_UINT32(crc_err_packet, p);
    STREAM_TO_UINT32_NO_CHANGE_P(type_err_packet, p);
    ETF_TRACE_DEBUG("status:0x%2x, mode_status:0x%2x\n", status, mode_status);
    ETF_TRACE_DEBUG("receive_packet:%d, hec_err_packet:%d\n", receive_packet, hec_err_packet);
    ETF_TRACE_DEBUG("crc_err_packet:%d, type_err_packet:%d\n", crc_err_packet, type_err_packet);

    bt_rx_flag = 0;
}

static int etf_hci_c2h(uint8_t hci_type, const uint8_t *buff,
                    uint32_t offset, uint32_t len)
{
    uint16_t opcode = 0;
    int event_status = 0;
    uint16_t rx_pkt_count = 0;
    int8_t rssi = 0;
    int i = 0;
    int read_mem_len = 0;
    uint8_t *read_mem_buff;
    struct hci_version ver;

    // hex_dump("event:", 20, (unsigned char *)buff, len);
    etf_hci_printf(buff, len);

    opcode = *((uint16_t  *)&buff[3]);
    event_status = buff[5];
    ETF_TRACE_DEBUG("opcode is %04x event is status %d\n", opcode, event_status);

    if ((hci_type == ETF_HCI_EVNT_PCKT)) {
        switch (opcode) {
        case READ_LOCAL_VERSION_OPCODE:    //version
            memcpy(&ver, buff + 4,sizeof(ver));
            ETF_TRACE_DEBUG("BtFwv:%d.%d.%d\n", ver.hci_ver, (ver.lmp_subver >> 8) & 0xff,  (ver.lmp_subver) & 0xff);
            if ((ver.hci_ver == BT_VERSION_4_2) || (ver.hci_ver == BT_VERSION_4_1)) {
                chip_xr829_flag = 1;
                ETF_TRACE_DEBUG("This chip is XR829\n");
            } else {
                chip_xr829_flag = 0;
            }
            break;
        case LE_ETF_HCI_OP_LE_TEST_END: // 0x201F
            rx_pkt_count = *((uint16_t  *)&buff[6]);
            ETF_TRACE_DEBUG("rx_pkt_count %u\n", rx_pkt_count);
            break;
        case LE_ETF_HCI_READ_RSSI: // 0xFC15
            rssi = (int8_t)buff[6];
            ETF_TRACE_DEBUG("rssi %u\n", (uint32_t)rssi);
            break;
        case LE_ETF_HCI_DBG_RD_MEM_CMD_OPCODE: // 0xFC01
            read_mem_buff = (uint8_t *)&buff[6];
            read_mem_len = (uint8_t)buff[1]-4;
            ETF_TRACE_DEBUG("read len %d\n", read_mem_len);
            for (i = 0; i < read_mem_len; i += 4) {
                ETF_TRACE_DEBUG("addr:0x%08x, value:0x%08x\n", (read_start_addr + i),
                                *((uint32_t *)(read_mem_buff + i)));
            }
            read_start_addr = 0;
        case ETF_HCI_DBG_RX_STOP_CMD_OPCODE: // 0xFC50
            test_result((uint8_t *)buff, len);
            break;
        case RESET_OPCODE:
            if(event_status == 0)
                ETF_TRACE_DEBUG("reset success\n");
        default:
            break;
        }
    } else {
        ETF_TRACE_DEBUG("err: not hci event\n");
    }

   return 0;
}


static int recv_cb(uint8_t *data, uint16_t len)
{
    return etf_hci_c2h(data[0], data + 1, 0, len - 1);
}

/*
 * The bt controller has 16 levels to control the ble tx power, the user can only
 * set level from 8 to 15. The power level and power value are negatively correlated
 * which means that the greater the power level, the smaller the power value.
 * So wo need to convert the power level and power value into a positive correlation
 */
static int etf_convert_power_level(int power_level)
{
    if (power_level <= ETF_MAX_SETTING_POWER_LEVEL)
        return ETF_SUPPORT_MAX_POWER_LEVEL - power_level;
    else
        return ETF_DEFAULT_POWER_LEVEL;
}

/* Event_code | param_tot_len | num cmd pkt of host to control|opcode | status */
/*   1byte    |       1       |               1               |   2   | 1     */

static const char *ble_etf_help =
    "tx                    start tx test\n"
    "tx_stop               stop tx test\n"
    "rx                    start rx test\n"
    "rx_stop               stop rx test\n"
    "channel <param>       set chan\n"
    "                          0~39 Frequency Range:2402 MHz to 2480 MHz\n"
    "rate <param>          set rate\n"
    "                          1M, 2M, s8, s2\n"
    "payload <param>       set payload type\n"
    "                          0 Pseudo-Random bit sequence 9\n"
    "                          1 Pattern of alternating bits '11110000'\n"
    "                          2 Pattern of alternating bits '10101010'\n"
    "                          3 Pseudo-Random bit sequence 15\n"
    "                          4 Pattern of All '1' bits\n"
    "                          5 Pattern of All '0' bits\n"
    "                          6 Pattern of alternating bits '00001111'\n"
    "                          7 Pattern of alternating bits '0101'\n"
    "len <param>           set payload len\n"
    "                          0~251 Length in bytes of payload data in each packet.\n"
    "hopping               start hopping when tx/rx\n"
    "hopping_stop          stop hopping when tx/rx\n"
    "power_level <param>   set power level\n"
    "                          0~15 power level.\n"
    "read_mem <addr> <len> read mem value,example 0x60110404 4\n"
    "                          len must 4times and 0~128.\n"
    "                          addr must 4 times.\n"
    "write_mem <addr> <len> write mem value,example 0x60110404 0x00000008\n"
    "                          addr must 4 times.\n"
    "set_channel_fec <ch0_pwr_fec> <ch20_pwr_fec> <ch39_pwr_fec> set test power fec.\n"
    "                          pwr_fec must be -128 to 127";

static const char *bt_etf_help =
    "ble                       ble cmd, you can use <btetf ble help> to see cmd\n"
    "bt                        bredr cmd, you can use <btetf bt help> to see cmd\n"
    "hci_reset                 send reset command to fw\n"
    "get_fwv                   get fw version\n"
    "tone <channel:0~78>       start send single tone\n"
    "tone_stop                 stop send single tone\n"
    "connect                   bt fw init\n"
    "disconnect                bt fw deinit, sys reboot\n"
    "freq_cali <freq:0~15>     fix freq cali\n"
    "                              btetf freq_cali 5";

static const char *br_etf_help =
    "tx                        start tx test\n"
    "tx_stop                   stop tx test\n"
    "rx                        start rx test\n"
    "rx_stop                   stop rx test\n"
    "bdaddr                    set bdaddr\n"
    "pattern <param>           set pattern\n"
    "                              Range:0~7 default=0\n"
    "                              0 Transmitter test - 0 pattern\n"
    "                              1 Transmitter test - 1 pattern\n"
    "                              2 Transmitter test - 1010 pattern\n"
    "                              3 Transmitter test - 0101 pattern\n"
    "                              4 Transmitter test 1111 0000 pattern\n"
    "                              5 Transmitter test-0000 1111 pattern\n"
    "                              6 Pseudorandom 9 bit sequence\n"
    "                              7 Pseudorandom 15 bit sequence\n"
    "                              8-254 reserved\n"
    "len <param>               set packet len\n"
    "                              Range:0~65535 default=10\n"
    "channel <param>           set channel\n"
    "                              Range:0~79 default=0\n"
    "power_level <param>       set power level\n"
    "                              Range:0~15 default=9\n"
    "link_type <param>         set link type\n"
    "                              Range:0~3 default=0\n"
    "                              0 ACL/SCO (Basic Rate)\n"
    "                              1 eSCO (Basic Rate)\n"
    "                              2 ACL (EDR)\n"
    "                              3 eSCO (EDR)\n"
    "packet_type <param>       set packet_type\n"
    "                              Range:0~15 default=3\n"
    "                              ACL/SCO (Basic Rate):\n"
    "                                  0 NULL\n"
    "                                  1 POLL\n"
    "                                  2 FHS  (0-17)\n"
    "                                  3 DM1  (0-17)\n"
    "                                  4 DH1  (0-27)\n"
    "                                  5 HV1  (10)\n"
    "                                  6 HV2  (20)\n"
    "                                  7 HV3  (30)\n"
    "                                  8 DV   (10)\n"
    "                                  9 AUX1 (0-29)\n"
    "                                  10 DM3 (0-121)\n"
    "                                  11 DH3 (0-183)\n"
    "                                  14 DM5 (0-224)\n"
    "                                  15 DH5 (0-339)\n"
    "                              eSCO (Basic Rate):\n"
    "                                  0 NULL\n"
    "                                  1 POLL\n"
    "                                  7 EV3  (30)\n"
    "                                  12 EV4 (120)\n"
    "                                  13 EV5 (180)\n"
    "                              ACL (EDR):\n"
    "                                  0 NULL\n"
    "                                  1 POLL\n"
    "                                  2 FHS      (0-17)\n"
    "                                  3 DM1      (0-17)\n"
    "                                  4 2-DH1    (0-54)\n"
    "                                  8 3-DH1    (0-83)\n"
    "                                  9 AUX1     (0-29)\n"
    "                                  10 2-DH3   (0-367)\n"
    "                                  11 3-DH3   (0-552)\n"
    "                                  14 2-DH5   (0-679)\n"
    "                                  15 3-DH5   (0-1021)\n"
    "                            eSCO (EDR):\n"
    "                                  0 NULL\n"
    "                                  1 POLL\n"
    "                                  6 2-EV3    (60)\n"
    "                                  7 3-EV3    (90)\n"
    "                                  12 2-EV5   (360)\n"
    "                                  13 3-EV5   (540)\n"
    "hopping <param>       set hopping mode\n"
    "                           Range:0~1 default=0\n"
    "                           0 Single Frequency\n"
    "                           1 Normal hopping\n";

static int cmd_etf_parse_int(const char *value, int min, int max, int *dst)
{
    int val;
    char *end;

    val = cmd_strtol(value, &end, 10);
    if (*end) {
        ETF_TRACE_ERROR("Invalid number '%s'", value);
        return -1;
    }

    if (val < min || val > max) {
        ETF_TRACE_ERROR("out of range value %d (%s), range is [%d, %d]\n",
             val, value, min, max);
        return -1;
    }

    *dst = val;
    return 0;
}

#if 0
static int ble_etf_tx(uint8_t chan, uint8_t payload_len, uint8_t payload)
{
    int ret = -1;
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = LE_ETF_HCI_OP_LE_TX_TEST;
    uint8_t len = 0x03;

    uint32_t buff_len = 0;
    uint32_t buff_offset = 0;
    uint32_t hci_type = ETF_HCI_CMD_PCKT;
    uint8_t *buff_start = buf;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, chan);
    UINT8_TO_STREAM(ptr, payload_len);
    UINT8_TO_STREAM(ptr, payload);
    buff_len = 7; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return ret;
}

static int ble_etf_rx(uint8_t chan)
{
    int ret = -1;
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = LE_ETF_HCI_OP_LE_RX_TEST;
    uint8_t len = 0x01;

    uint32_t buff_len = 0;
    uint32_t buff_offset = 0;
    uint32_t hci_type = ETF_HCI_CMD_PCKT;
    uint8_t *buff_start = buf;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, chan);
    buff_len = 5; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return ret;

}
#endif

static int ble_etf_tx_rx_stop(void)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = LE_ETF_HCI_OP_LE_TEST_END;
    uint8_t len = 0x00;

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    buff_len = 4; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return 0;
}

static int ble_etf_enhanced_tx(uint8_t chan, uint8_t payload_len, uint8_t payload_type, uint8_t phy)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = LE_ETF_HCI_OP_LE_ENH_TX_TEST;
    uint8_t len = 0x04;

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, chan);
    UINT8_TO_STREAM(ptr, payload_len);
    UINT8_TO_STREAM(ptr, payload_type);
    UINT8_TO_STREAM(ptr, phy);
    buff_len = 8; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return 0;
}

static int ble_etf_tx(uint8_t chan, uint8_t payload_len, uint8_t payload_type)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = LE_ETF_HCI_OP_LE_TX_TEST;
    uint8_t len = 0x03;

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, chan);
    UINT8_TO_STREAM(ptr, payload_len);
    UINT8_TO_STREAM(ptr, payload_type);
    buff_len = 7; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return 0;
}

static int ble_etf_enhanced_rx(uint8_t chan, uint8_t phy, uint8_t mod_index)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = LE_ETF_HCI_OP_LE_ENH_RX_TEST;
    uint8_t len = 0x03;

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;

    if (phy == 0x04) {
        phy = 0x03;
    }

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, chan);
    UINT8_TO_STREAM(ptr, phy);
    UINT8_TO_STREAM(ptr, mod_index); /* fw no use*/
    buff_len = 7; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return 0;
}

static int ble_etf_rx(uint8_t chan)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = LE_ETF_HCI_OP_LE_RX_TEST;
    uint8_t len = 0x01;

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, chan);
    buff_len = 5; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return 0;
}

static int etf_stone(uint8_t chan)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = LE_ETF_HCI_SINGLE_TONE_OPEN;
    uint8_t len = 0x03;
    uint8_t open = 0x01;
    uint8_t power = 0x01; /* power param no use*/

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, open);
    UINT8_TO_STREAM(ptr, chan);
    UINT8_TO_STREAM(ptr, power);
    buff_len = 7; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return 0;
}

static int etf_stone_stop(void)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = LE_ETF_HCI_SINGLE_TONE_OPEN;
    uint8_t len = 0x01;
    uint8_t open = 0x00;

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, open);

    buff_len = 5; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return 0;
}

static int ble_etf_set_pwr_fec(int8_t pwr_fec_ch0, int8_t pwr_fec_ch20, int8_t pwr_fec_ch39)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = LE_ETF_HCI_SET_TEST_PWR_FEC_OPCODE;
    uint8_t len = 0x03;

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, (uint8_t)pwr_fec_ch0);
    UINT8_TO_STREAM(ptr, (uint8_t)pwr_fec_ch20);
    UINT8_TO_STREAM(ptr, (uint8_t)pwr_fec_ch39);
    buff_len = 7; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return 0;
}

static int ble_etf_get_rssi(void)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = LE_ETF_HCI_READ_RSSI;
    uint8_t len = 0x00;

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    buff_len = 4; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return 0;
}

static int ble_etf_set_power(uint8_t power)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = chip_xr829_flag?HCI_LE_TX_POWER_CAL_CMD_OPCODE:LE_ETF_HCI_OP_LE_SET_POWER;
    uint8_t len = 0x01;

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, power);
    buff_len = 5; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return 0;
}

static int ble_etf_read_mem(uint32_t start_addr, uint32_t read_len)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = LE_ETF_HCI_DBG_RD_MEM_CMD_OPCODE;
    uint8_t len = 0;
    struct dbg_rd_mem_cmd param;
    read_start_addr = start_addr;

    param.start_addr = start_addr;
    param.type = _32_Bit;
    param.length = read_len;

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    len = sizeof(struct dbg_rd_mem_cmd);
    UINT8_TO_STREAM(ptr, len);
    memcpy(ptr, &param, sizeof(struct dbg_rd_mem_cmd));
    buff_len = 1 + 2 + 1 + sizeof(struct dbg_rd_mem_cmd); /*acording to up context*/

    etf_send(buff_start, buff_len);

    return 0;
}

static int ble_etf_write_mem(uint32_t start_addr, uint32_t value)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = LE_ETF_HCI_DBG_WR_MEM_CMD_OPCODE;
    uint8_t len = 0;
    struct dbg_wr_mem_cmd param;

    param.start_addr = start_addr;
    param.type = _32_Bit;
    param.buf.length = 4;
    memset(param.buf.data, 0, HCI_TRACE_PRINTK_MAX_SIZE);
    *((uint32_t *)(&param.buf.data[0])) = value;

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    len = sizeof(struct dbg_wr_mem_cmd);
    UINT8_TO_STREAM(ptr, len);
    memcpy(ptr, &param, sizeof(struct dbg_wr_mem_cmd));
    buff_len = 1 + 2 + 1 + sizeof(struct dbg_wr_mem_cmd); /*acording to up context*/

    etf_send(buff_start, buff_len);

    return 0;
}

static int acl_sco_basic_packet_len(uint16_t *packet_len, uint8_t packet_type)
{
    switch (packet_type) {
    case P_NULL:
    case POLL:
    case FHS:
    case DM1:
        *packet_len = (*packet_len > 17) ? 17 : *packet_len;
        break;
    case DH1:
        *packet_len = (*packet_len > 27) ? 27 : *packet_len;
        break;
    case HV1:
        *packet_len = 10;
        break;
    case HV2:
        *packet_len = 20;
        break;
    case HV3:
        *packet_len = 30;
        break;
    case DV:
        *packet_len = 10;
        break;
    case AUX1:
        *packet_len = (*packet_len > 29) ? 29 : *packet_len;
        break;
    case DM3:
        *packet_len = (*packet_len > 121) ? 121 : *packet_len;
        break;
    case DH3:
        *packet_len = (*packet_len > 183) ? 183 : *packet_len;
        break;
    case DM5:
        *packet_len = (*packet_len > 224) ? 224 : *packet_len;
        break;
    case DH5:
        *packet_len = (*packet_len > 339) ? 339 : *packet_len;
        break;
    default:
        ETF_TRACE_DEBUG("Invalid packet_type(:%d) In ACL_EDR_RATE mode", packet_type);
    }
    return 0;
}

static int acl_sco_edr_packet_len(uint16_t *packet_len, uint8_t packet_type)
{
    switch (packet_type) {
    case P_NULL:
    case POLL:
    case FHS:
    case DM1:
        *packet_len = (*packet_len > 17) ? 17 : *packet_len;
        break;
    case P_2DH1:
        *packet_len = (*packet_len > 54) ? 54 : *packet_len;
        break;
    case P_3DH1:
        *packet_len = (*packet_len > 83) ? 83 : *packet_len;
        break;
    case AUX1:
        *packet_len = (*packet_len > 29) ? 29 : *packet_len;
        break;
    case P_2DH3:
        *packet_len = (*packet_len > 367) ? 367 : *packet_len;
        break;
    case P_3DH3:
        *packet_len = (*packet_len > 552) ? 552 : *packet_len;
        break;
    case P_2DH5:
        *packet_len = (*packet_len > 679) ? 679 : *packet_len;
        break;
    case P_3DH5:
        *packet_len = (*packet_len > 1021) ? 1021 : *packet_len;
        break;
    default:
        ETF_TRACE_DEBUG("Invalid packet_type(:%d) In ACL_EDR_RATE mode", packet_type);
    }
    return 0;
}


static int bt_tx_get_packet_len(uint16_t *packet_len, uint8_t link_type, uint8_t packet_type)
{

    if (*packet_len < 0) {
        ETF_TRACE_DEBUG("Invalid packet_len(:%d) para", packet_type);
        return 0;
    }

    switch (link_type) {
    case ACL_SCO_BASIC_RATE:
        acl_sco_basic_packet_len(packet_len, packet_type);
        break;
    case eSCO_BASIC_RATE:
        switch (packet_type) {
        case P_NULL:
        case POLL:
            *packet_len = 10;
            break;
        case EV3:
            *packet_len = 30;
            break;
        case EV4:
            *packet_len = 120;
            break;
        case EV5:
            *packet_len = 180;
            break;
        default:
            ETF_TRACE_DEBUG("Invalid packet_type(:%d) In eSCO_BASIC_RATE mode", packet_type);
            return 0;
        }
    break;
    case ACL_EDR_RATE:
        acl_sco_edr_packet_len(packet_len, packet_type);
        break;
    case eSCO_EDR_RATE:
        switch (packet_type) {
            case P_NULL:
            case POLL:
            case P_2EV3:
            *packet_len = 60;
                break;
    case P_3EV3:
        *packet_len = 90;
        break;
    case P_2EV5:
        *packet_len = 360;
        break;
    case P_3EV5:
        *packet_len = 540;
        break;
    default:
        ETF_TRACE_DEBUG("Invalid packet_type(:%d) In ACL_EDR_RATE mode", packet_type);
        return 0;
    }
    break;
    default:
        ETF_TRACE_DEBUG("Invalid link_type(:%d) In ACL_EDR_RATE mode", link_type);
        return 0;
    }
}


static int bt_etf_tx_rx_stop(void)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint8_t len = 0x01;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, HCI_DBG_TX_RX_TEST_CMD_OPCODE);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, 0xf0);//mean tx mode close
    etf_send(buf, len + 4);

    return 0;
}

static int bt_etf_tx(bdaddr_t bdaddr, uint8_t pattern, uint16_t packet_len, uint8_t channel_num, uint8_t power_level, uint8_t link_packet_type, uint8_t hopping_mode)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint8_t len = 0x0D;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, HCI_DBG_TX_RX_TEST_CMD_OPCODE);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, 0x00);//mean tx mode

    BDADDR_TO_STREAM (ptr, bdaddr.b);
    UINT8_TO_STREAM(ptr, pattern);
    UINT16_TO_STREAM(ptr, packet_len);

    if (hopping_mode == 1) {
        channel_num = channel_num | (1 << 7);
    }
    UINT8_TO_STREAM(ptr, channel_num);
    UINT8_TO_STREAM(ptr, power_level);
    UINT8_TO_STREAM(ptr, link_packet_type);

    etf_send(buf, len + 4);

    return 0;
}

static int bt_etf_rx(bdaddr_t bdaddr, uint8_t channel_num, uint8_t link_packet_type, uint8_t pattern, uint16_t pklen)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint8_t len = 0x0D;

    printf("link_packet_type is %d\n", link_packet_type);
    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, HCI_DBG_TX_RX_TEST_CMD_OPCODE);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, 0x10);//mean rx mode

    BDADDR_TO_STREAM (ptr, bdaddr.b);
    UINT8_TO_STREAM(ptr, pattern);
    UINT16_TO_STREAM(ptr, pklen);
    UINT8_TO_STREAM(ptr, channel_num);
    UINT8_TO_STREAM(ptr, 0x00);
    UINT8_TO_STREAM(ptr, link_packet_type);

    bt_rx_flag = 1;

    etf_send(buf, len + 4);
    return 0;
}

static int bt_etf_stone_stop(void)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = LE_ETF_HCI_SINGLE_TONE_OPEN;
    uint8_t len = 0x01;
    uint8_t open = 0x00;

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, open);

    buff_len = 5; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return 0;
}

static int bt_etf_set_pwr_fec(int8_t pwr_fec_ch0, int8_t pwr_fec_ch20, int8_t pwr_fec_ch39)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = LE_ETF_HCI_SET_TEST_PWR_FEC_OPCODE;
    uint8_t len = 0x03;

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, (uint8_t)pwr_fec_ch0);
    UINT8_TO_STREAM(ptr, (uint8_t)pwr_fec_ch20);
    UINT8_TO_STREAM(ptr, (uint8_t)pwr_fec_ch39);
    buff_len = 7; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return 0;
}

static int bt_etf_get_rssi(void)
{
    int ret = 1074;

    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = LE_ETF_HCI_READ_RSSI;
    uint8_t len = 0x00;

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    buff_len = 4; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return ret;
}

static int bt_etf_set_power(uint8_t power)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = LE_ETF_HCI_OP_LE_SET_POWER;
    uint8_t len = 0x01;

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, power);
    buff_len = 5; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return 0;
}

static enum cmd_status cmd_ble_etf_tx_exec(char *cmd)
{
    int ret = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    if (chip_xr829_flag) {
       ret = ble_etf_tx(ble_etf_cfg.chan, ble_etf_cfg.payload_len,
                   ble_etf_cfg.payload_type);
    } else {
        ret = ble_etf_enhanced_tx(ble_etf_cfg.chan, ble_etf_cfg.payload_len,
                   ble_etf_cfg.payload_type, ble_etf_cfg.phy);
    }

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_ble_etf_tx_stop_exec(char *cmd)
{
    int ret = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = ble_etf_tx_rx_stop();

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_ble_etf_rx_exec(char *cmd)
{
    int ret = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    if (chip_xr829_flag) {
        ret = ble_etf_rx(ble_etf_cfg.chan);
    } else {
        ret = ble_etf_enhanced_rx(ble_etf_cfg.chan, ble_etf_cfg.phy, 00);
    }

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_ble_etf_rx_stop_exec(char *cmd)
{
    int ret = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ble_etf_get_rssi();
    ret = ble_etf_tx_rx_stop();

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_ble_etf_set_chan_exec(char *cmd)
{
    int ret = 0;
    int chan;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = cmd_etf_parse_int(cmd, 0, 39, &chan);
    if (ret == 0) {
        ble_etf_cfg.chan = chan;
    }

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_ble_etf_set_rate_exec(char *cmd)
{
    int ret = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    if (cmd_strcmp(cmd, "1M") == 0) {
        ble_etf_cfg.phy = 0x01;
    } else if (cmd_strcmp(cmd, "2M") == 0) {
        ble_etf_cfg.phy = 0x02;
    } else if (cmd_strcmp(cmd, "S2") == 0) {
        ble_etf_cfg.phy = 0x04;
    } else if (cmd_strcmp(cmd, "S8") == 0) {
        ble_etf_cfg.phy = 0x03;
    } else {
        if (cmd) {
            ETF_TRACE_ERROR("err:not support rate %s\n", cmd);
            ETF_TRACE_ERROR("e.g : btetf ble 1M\n");
            return CMD_STATUS_INVALID_ARG;
        }
    }

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_ble_etf_set_payload_exec(char *cmd)
{
    int ret = 0;
    int payload;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = cmd_etf_parse_int(cmd, 0, 7, &payload);
    if (ret == 0) {
        ble_etf_cfg.payload_type = payload;
    }

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_ble_etf_set_payload_len_exec(char *cmd)
{
    int ret = 0;
    int payload_len;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = cmd_etf_parse_int(cmd, 0, 251, &payload_len);
    if (ret == 0) {
        ble_etf_cfg.payload_len = payload_len;
    }

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_bt_etf_tx_exec(char *cmd)
{
    int ret = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    bt_tx_get_packet_len(&bt_etf_cfg.packet_len, bt_etf_cfg.link_type, bt_etf_cfg.packet_type);

    ret = bt_etf_tx(bt_etf_cfg.bdaddr, bt_etf_cfg.pattern, bt_etf_cfg.packet_len,
                bt_etf_cfg.chan, bt_etf_cfg.power_level, (bt_etf_cfg.link_type << 4 | bt_etf_cfg.packet_type) & 0xff,
                bt_etf_cfg.hopping_mode);

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_bt_etf_tx_stop_exec(char *cmd)
{
    int ret = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = bt_etf_tx_rx_stop();

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_bt_etf_rx_exec(char *cmd)
{
    int ret = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = bt_etf_rx(bt_etf_cfg.bdaddr, bt_etf_cfg.chan, (bt_etf_cfg.link_type << 4) | bt_etf_cfg.packet_type, bt_etf_cfg.pattern, bt_etf_cfg.packet_len);

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_bt_etf_rx_stop_exec(char *cmd)
{
    int ret = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = bt_etf_tx_rx_stop();

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_bt_etf_set_bdaddr_exec(char *cmd)
{
    int ret = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    str2ba(cmd, &bt_etf_cfg.bdaddr);

    ETF_TRACE_DEBUG("bdaddr is %02x:%02x:%02x:%02x:%02x:%02x\n", bt_etf_cfg.bdaddr.b[5], bt_etf_cfg.bdaddr.b[4],
            bt_etf_cfg.bdaddr.b[3], bt_etf_cfg.bdaddr.b[2], bt_etf_cfg.bdaddr.b[1], bt_etf_cfg.bdaddr.b[0]);

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_bt_etf_set_pattern_exec(char *cmd)
{
    int ret = 0;
    int pattern = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = cmd_etf_parse_int(cmd, 0, 7, &pattern);
    bt_etf_cfg.pattern = pattern;

    ETF_TRACE_DEBUG("pattern %d\n", bt_etf_cfg.pattern);
    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_bt_etf_set_payload_len_exec(char *cmd)
{
    int ret = 0;
    int packet_len = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = cmd_etf_parse_int(cmd, 0, 65535, &packet_len);
    bt_etf_cfg.packet_len = packet_len;

    ETF_TRACE_DEBUG("packet_len %d\n", bt_etf_cfg.packet_len);
    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_bt_etf_set_chan_exec(char *cmd)
{
    int ret = 0;
    int chan = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = cmd_etf_parse_int(cmd, 0, 78, &chan);
    bt_etf_cfg.chan = chan;

    ETF_TRACE_DEBUG("chan %d\n", bt_etf_cfg.chan);
    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_bt_etf_set_pwr_level_exec(char *cmd)
{
    int ret = 0;
    int power_level = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = cmd_etf_parse_int(cmd, 0, ETF_MAX_SETTING_POWER_LEVEL, &power_level);
    if (ret == 0) {
        bt_etf_cfg.power_level = etf_convert_power_level(power_level);
    }

    ETF_TRACE_DEBUG("power_level %d\n", bt_etf_cfg.power_level);
    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_bt_etf_set_link_type_exec(char *cmd)
{
    int ret = 0;
    int link_type = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = cmd_etf_parse_int(cmd, 0, 3, &link_type);
    bt_etf_cfg.link_type = link_type;

    ETF_TRACE_DEBUG("link_type %d\n", bt_etf_cfg.link_type);
    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_bt_etf_set_packet_type_exec(char *cmd)
{
    int ret = 0;
    int packet_type = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = cmd_etf_parse_int(cmd, 0, 15, &packet_type);
    bt_etf_cfg.packet_type = packet_type;

    ETF_TRACE_DEBUG("packet_type %d\n", bt_etf_cfg.packet_type);
    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_bt_etf_set_hopping_mode_exec(char *cmd)
{
    int ret = 0;
    int hopping_mode = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = cmd_etf_parse_int(cmd, 0, 1, &hopping_mode);
    bt_etf_cfg.hopping_mode = hopping_mode;

    ETF_TRACE_DEBUG("hopping_mode %d\n", bt_etf_cfg.hopping_mode);
    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

#if 0 //todo
HAL_Status HAL_PRNG_Generate(uint8_t *random, uint32_t size);
#else
int32_t simple_rand(int32_t seed) { return seed * 22695477 + 1;}
#endif
static void hopping_task(void *arg)
{
    uint8_t random_value = 0;
    uint8_t chan = 1;
    while (1) {
#if 0 //todo
        if (HAL_PRNG_Generate(&random_value, 1) == HAL_OK) {
            chan = random_value % 40;
        } else {
            ETF_TRACE_DEBUG("get random value err\n");
        }
#else
        static int32_t seed = 5;
        seed = simple_rand(seed);
        random_value = seed;
        chan = random_value % 40;
#endif
        ble_etf_tx_rx_stop();
        XR_OS_MSleep(10);
        ble_etf_enhanced_tx(chan, ble_etf_cfg.payload_len,
                  ble_etf_cfg.payload_type, ble_etf_cfg.phy);

        XR_OS_MSleep(50);
    }

}

static enum cmd_status cmd_ble_etf_hopping_exec(char *cmd)
{
    int ret = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    memset(&g_hopping_thread, 0, sizeof(XR_OS_Thread_t));
    if (XR_OS_ThreadCreate(&g_hopping_thread,
                        "hopping",
                        hopping_task,
                        NULL,
                        XR_OS_THREAD_PRIO_APP,
                        THREAD_STACK_SIZE) != XR_OS_OK) {
        ETF_TRACE_ERROR("create ak thread failed\n");
        ret = -1;
    }

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_ble_etf_hopping_stop_exec(char *cmd)
{
    int ret = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ble_etf_tx_rx_stop();
    XR_OS_ThreadDelete(&g_hopping_thread);

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_reset_test_exec(char *cmd)
{
    int ret = 0;
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = RESET_OPCODE;
    uint8_t len = 0x00;

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    buff_len = 4; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_read_local_fwv_exec(char *cmd)
{
    int ret = 0;
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = READ_LOCAL_VERSION_OPCODE;
    uint8_t len = 0x00;

    uint32_t buff_len = 0;
    uint8_t *buff_start = buf;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, len);
    buff_len = 4; /*acording to up context*/

    etf_send(buff_start, buff_len);

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_ble_etf_set_power_exec(char *cmd)
{
    int ret = 0;
    int val = 0;
    int power_level;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = cmd_etf_parse_int(cmd, 0, ETF_MAX_SETTING_POWER_LEVEL, &val);
    if (ret == 0) {
        power_level = etf_convert_power_level(val);;
        ret = ble_etf_set_power(power_level);
        if (ret == 0)
            ble_etf_cfg.power_level = power_level;
    }

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_etf_tone_exec(char *cmd)
{
    int ret = 0;
    uint32_t channel = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = cmd_sscanf(cmd, "%d", &channel);
    if (ret != 1) {
        ETF_TRACE_ERROR("example: btetf tone 78\n");
        return CMD_STATUS_INVALID_ARG;
    }

    if (channel > 78) {
        ETF_TRACE_ERROR("channel should in range 0 ~ 78\n");
        return CMD_STATUS_INVALID_ARG;
    }

    ret = etf_stone(channel);

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_etf_tone_stop_exec(char *cmd)
{
    int ret = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = etf_stone_stop();

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_ble_etf_read_mem_exec(char *cmd)
{
    int ret = 0;
    int32_t cnt;
    uint32_t addr;
    int len;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    cnt = cmd_sscanf(cmd, "0x%x %d", &addr, &len);
    if (cnt != 2) {
        ETF_TRACE_ERROR("example: 0x10 0x10\n");
        return CMD_STATUS_INVALID_ARG;
    }

    if (addr % 4 != 0) {
        ETF_TRACE_ERROR("addr must be 4 byte align\n");
        return CMD_STATUS_INVALID_ARG;
    }

    if ((len % 4 != 0) || (len <= 0) || (len > 128)) {
        ETF_TRACE_ERROR("len must be 4 times and 0~128\n");
        return CMD_STATUS_INVALID_ARG;
    }

    ret = ble_etf_read_mem(addr, len);

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_ble_etf_write_mem_exec(char *cmd)
{
    int ret = 0;
    int32_t cnt;
    uint32_t addr;
    uint32_t value;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    cnt = cmd_sscanf(cmd, "0x%x 0x%x", &addr, &value);
    if (cnt != 2) {
        ETF_TRACE_ERROR("example: 0x10 0x10\n");
        return CMD_STATUS_INVALID_ARG;
    }

    if (addr % 4 != 0) {
        ETF_TRACE_ERROR("addr must be 4 byte align\n");
        return CMD_STATUS_INVALID_ARG;
    }

    ret = ble_etf_write_mem(addr, value);

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_ble_etf_set_channel_fec_exec(char *cmd)
{
    int ret = 0;
    int32_t cnt;
    int32_t pwr_fec_ch0;
    int32_t pwr_fec_ch20;
    int32_t pwr_fec_ch39;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    if (chip_xr829_flag) {
        ETF_TRACE_DEBUG("XR829 do not support this command\n");
        return CMD_STATUS_UNKNOWN_CMD;
    }
    cnt = cmd_sscanf(cmd, "%d %d %d", &pwr_fec_ch0, &pwr_fec_ch20, &pwr_fec_ch39);
    if (cnt != 3)
    {
        ETF_TRACE_ERROR("example: -8 12 16\n");
        return CMD_STATUS_INVALID_ARG;
    }

    if ((pwr_fec_ch0 < -128 || pwr_fec_ch0 > 127) ||
        (pwr_fec_ch20 < -128 || pwr_fec_ch20 > 127) ||
        (pwr_fec_ch39 < -128 || pwr_fec_ch39 > 127)) {
        ETF_TRACE_ERROR("value must be form -128 to 127\n");
        return CMD_STATUS_INVALID_ARG;
    }

    ret = ble_etf_set_pwr_fec(pwr_fec_ch0, pwr_fec_ch20, pwr_fec_ch39);

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_br_etf_help_exec(char *cmd)
{
    ETF_TRACE_DEBUG("%s\n", br_etf_help);

    return CMD_STATUS_OK;
}

void hci_set_freq_cali_val(int value)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint8_t len = 1;
    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, HCI_LE_RF_TRIM_CAL_CMD_OPCODE);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, value);

    etf_send(buf, len + 4);
}

void hci_set_pwr_cali_val(int value)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint8_t len = 1;
    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, LE_ETF_HCI_OP_LE_SET_POWER_MAX);
    UINT8_TO_STREAM(ptr, len);
    UINT8_TO_STREAM(ptr, value);

    etf_send(buf, len + 4);
}

static enum cmd_status cmd_etf_get_freq_offset_exec(char *cmd)
{
    ETF_TRACE_DEBUG("freq offset is :%d\n", hal_clk_ccu_aon_get_freq_trim());
    return CMD_STATUS_OK;
}

static enum cmd_status cmd_etf_set_freq_offset_exec(char *cmd)
{
    int32_t cnt;
    uint32_t value;

    /* get param */
    cnt = cmd_sscanf(cmd, "%d", &value);

    /* check param */
    if (cnt != 1) {
        ETF_TRACE_ERROR("invalid param number %d\n", cnt);
        return CMD_STATUS_INVALID_ARG;
    }

    if (value > 127) {
        ETF_TRACE_ERROR("invalid value %d\n", value);
        return CMD_STATUS_INVALID_ARG;
    }

    hal_clk_ccu_aon_set_freq_trim(value);
    ETF_TRACE_DEBUG("freq offset is set to %d!\n", value);

    return CMD_STATUS_OK;
}

static enum cmd_status cmd_etf_pwr_cali(char *cmd)
{
    int ret = 0;
    int pwr = 0;
    if (etf_enable == 0) {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }

    ret = cmd_etf_parse_int(cmd, 0, 255, &pwr);
    if (ret != 0) {
        ETF_TRACE_ERROR("pwr range is 0 ~ 255\n");
        return CMD_STATUS_INVALID_ARG;
    }

    ETF_TRACE_DEBUG("pwr %d\n", pwr);
    hci_set_pwr_cali_val(pwr);

    return (ret == 0) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_bt_etf_help_exec(char *cmd)
{
    ETF_TRACE_DEBUG("%s\n", bt_etf_help);

    return CMD_STATUS_OK;
}

static enum cmd_status cmd_ble_etf_help_exec(char *cmd)
{
    ETF_TRACE_DEBUG("%s\n", ble_etf_help);

    return CMD_STATUS_OK;
}

static enum cmd_status cmd_etf_connect_exec(char *cmd)
{
    int i = 0;
    char mac[6] = {0};
    bt_hc_callbacks_t bt_hc_callbacks;
    if (etf_enable == 1) {
        ETF_TRACE_DEBUG("etf have been enable\n");
        return CMD_STATUS_FAIL;
    }

    etf_lib_if = bt_lib_get_interface();
    for (i = 0; i < 6; ++i) {
        mac[i] = (uint8_t)XR_OS_GetTicks();
    }
    ETF_TRACE_DEBUG("mca is %02x %02x %02x %02x %02x %02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    etf_lib_if->init();
    bt_hc_callbacks.data_ind = &recv_cb;
    etf_id = etf_lib_if->hci_ops->open(BT_FEATURES_DUAL, (void *)&bt_hc_callbacks);
    etf_lib_if->hci_ctrl_ops->set_mac(mac);

    etf_enable = (etf_id == 0) ? 1 : 0;

    cmd_read_local_fwv_exec((char *)NULL);
    /* set default ble tx power level */
    ble_etf_set_power(ble_etf_cfg.power_level);

    return (etf_enable == 1) ? CMD_STATUS_OK : CMD_STATUS_FAIL;
}

static enum cmd_status cmd_etf_disconnect_exec(char *cmd)
{
    const bt_lib_interface_t *bt_lib_interface = NULL;
    if(etf_enable) {
        etf_enable = 0;
    } else {
        ETF_TRACE_DEBUG("etf have not enable\n");
        return CMD_STATUS_FAIL;
    }
    bt_lib_interface = bt_lib_get_interface();
    bt_lib_interface->hci_ops->close(etf_id);
    bt_lib_interface->deinit();
    return CMD_STATUS_OK;
}

static const struct cmd_data g_ble_etf_cmds[] = {
    { "tx",              cmd_ble_etf_tx_exec },
    { "tx_stop",         cmd_ble_etf_tx_stop_exec },
    { "rx",              cmd_ble_etf_rx_exec },
    { "rx_stop",         cmd_ble_etf_rx_stop_exec },
    { "channel",         cmd_ble_etf_set_chan_exec },
    { "rate",            cmd_ble_etf_set_rate_exec },
    { "payload",         cmd_ble_etf_set_payload_exec },
    { "len",             cmd_ble_etf_set_payload_len_exec },
    { "hopping",         cmd_ble_etf_hopping_exec },
    { "hopping_stop",    cmd_ble_etf_hopping_stop_exec },
    { "power_level",     cmd_ble_etf_set_power_exec },
    { "read_mem",        cmd_ble_etf_read_mem_exec },
    { "write_mem",       cmd_ble_etf_write_mem_exec },
    { "set_channel_fec", cmd_ble_etf_set_channel_fec_exec },
    { "help",            cmd_ble_etf_help_exec },
};

enum cmd_status cmd_ble_etf_exec(char *cmd)
{
    return cmd_exec(cmd, g_ble_etf_cmds, cmd_nitems(g_ble_etf_cmds));
}

static const struct cmd_data g_bt_etf_cmds[] = {
    { "tx",              cmd_bt_etf_tx_exec },
    { "tx_stop",         cmd_bt_etf_tx_stop_exec },
    { "rx",              cmd_bt_etf_rx_exec },
    { "rx_stop",         cmd_bt_etf_rx_stop_exec },
    { "bdaddr",          cmd_bt_etf_set_bdaddr_exec },
    { "pattern",         cmd_bt_etf_set_pattern_exec },
    { "len",             cmd_bt_etf_set_payload_len_exec },
    { "channel",         cmd_bt_etf_set_chan_exec },
    { "power_level",     cmd_bt_etf_set_pwr_level_exec },
    { "link_type",       cmd_bt_etf_set_link_type_exec },
    { "packet_type",     cmd_bt_etf_set_packet_type_exec },
    { "hopping",         cmd_bt_etf_set_hopping_mode_exec },
    { "help",            cmd_br_etf_help_exec },
};

enum cmd_status cmd_bt_etf_exec(char *cmd)
{
    return cmd_exec(cmd, g_bt_etf_cmds, cmd_nitems(g_bt_etf_cmds));
}

static const struct cmd_data g_btetf_cmds[] = {
    { "ble",             cmd_ble_etf_exec },
    { "bt",              cmd_bt_etf_exec },
    { "hci_reset",       cmd_reset_test_exec },
    { "get_fwv",         cmd_read_local_fwv_exec },
    { "tone",            cmd_etf_tone_exec },
    { "tone_stop",       cmd_etf_tone_stop_exec },
    { "connect",         cmd_etf_connect_exec },
    { "disconnect",      cmd_etf_disconnect_exec },
    { "get_freq_offset", cmd_etf_get_freq_offset_exec},
    { "set_freq_offset", cmd_etf_set_freq_offset_exec},
    { "pwr_cali",        cmd_etf_pwr_cali,},
    { "help",            cmd_bt_etf_help_exec },
};

enum cmd_status cmd_btetf_exec(char *cmd)
{
    return cmd_exec(cmd, g_btetf_cmds, cmd_nitems(g_btetf_cmds));
}

#define ETF_CMD_BUF_SIZE    128
static char etf_cmd_buf[ETF_CMD_BUF_SIZE];

static void btetf_exec(int argc, char *argv[])
{
    int ret, i, len;
    int left = ETF_CMD_BUF_SIZE;
    char *ptr = etf_cmd_buf;
    memset(etf_cmd_buf, 0, sizeof(etf_cmd_buf));

    for (i = 1; i < argc && left >= 2; ++i) {
        len = cmd_strlcpy(ptr, argv[i], left);
        ptr += len;
        left -= len;
        if (i < argc - 1 && left >= 2) {
            *ptr++ = ' ';
            *ptr = '\0';
            left -= 1;
        }
    }
    ETF_TRACE_DEBUG("net cmd: %s\n", etf_cmd_buf);
    ret = cmd_btetf_exec(etf_cmd_buf);
    if (ret == CMD_STATUS_OK) {
        ETF_TRACE_DEBUG("<ACK> 200 OK\n");
    } else {
        ETF_TRACE_DEBUG("<EVT> %d\n",ret);
    }
}

FINSH_FUNCTION_EXPORT_CMD(btetf_exec, btetf, bluetooth etf command);

#endif
