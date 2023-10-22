/*
 *
 *
 *  Copyright(C), 2015, Xradio Technology Co., Ltd.
 *
 *
 */
 #if 0
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <signal.h>

#include "hcidump_xr/hcidump_xr.h"

#define MAX_BUF_LEN                          128
#define HCI_VSC_COEX_A2DP_CMDS_PARA_SIZE      4

#define HCI_VSC_COEX_CMDS                    0x0022
#define HCI_VSC_COEX_A2DP_SOURCE_ID               0x01
#define HCI_VSC_COEX_A2DP_SINK_ID                 0x03

#define HCI_GRP_VENDOR_SPECIFIC (0x3F << 10) /* 0xFC00 */
static int dev_id = -1;

enum {
  A2DP_COEX_START = 0x01,
  A2DP_COEX_STOP
};

typedef union _opcode_t_ {
    unsigned int opcode;
    struct {
        unsigned short ocf : 10;
        unsigned short ogf : 6;
    } op;}
opcode_t;

opcode_t HCI_OP;


static void vendor_hex_dump(char *pref, int width, unsigned char *buf, int len)
{
    register int i,n;

    for (i = 0, n = 1; i < len; i++, n++) {
        if (n == 1)
            printf("%s ", pref);
        printf("%2.2X ", buf[i]);
        if (n == width) {
            printf("\n");
            n = 0;
        }
    }
    if (i && n!=1)
        printf("\n");
}

static void send_vendor_cmd(char *cmdbuf)
{
    if (dev_id == -1)
        return;

    unsigned char *ptr = cmdbuf;
    struct hci_filter flt;
    hci_event_hdr *hdr;
    int len, dd;
    uint16_t opcode, ocf;
    uint8_t ogf;

    ptr++;
    STREAM_TO_UINT16(opcode, ptr);
    STREAM_TO_UINT8(len, ptr);
    HCI_OP.opcode = opcode;

    //printf("< HCI Command: opcode:(0x%4x) ogf-ocf:(0x%02x-0x%04x) plen:(%d)\n", opcode, HCI_OP.op.ogf, HCI_OP.op.ocf, len);
    //vendor_hex_dump("  ", 20, ptr, len);

    dd = hci_open_dev(dev_id);
    if (dd < 0) {
        perror("Device open failed");
        exit(EXIT_FAILURE);
    }

    hci_filter_clear(&flt);
    hci_filter_set_ptype(HCI_EVENT_PKT, &flt);
    hci_filter_all_events(&flt);
    if (setsockopt(dd, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
        perror("HCI filter setup failed");
        exit(EXIT_FAILURE);
    }

    if (hci_send_cmd(dd, HCI_OP.op.ogf, HCI_OP.op.ocf, len, ptr) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    char buf[HCI_MAX_EVENT_SIZE];
    len = read(dd, buf, sizeof(buf));
    if (len < 0) {
        perror("Read failed");
        exit(EXIT_FAILURE);
    }

    hdr = (void *)(buf + 1);
    ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
    len -= (1 + HCI_EVENT_HDR_SIZE);

    //printf("> HCI Event: 0x%02x plen %d\n", hdr->evt, hdr->plen);
    //vendor_hex_dump("  ", 20, ptr, len);
    fflush(stdout);

    hci_close_dev(dd);

}

void Avdtp_coex_start_cmd(uint16_t acl_handle, uint8_t in)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = (HCI_VSC_COEX_CMDS | HCI_GRP_VENDOR_SPECIFIC);
    uint8_t HCI_VSC_COEX_A2DP_ID = (in != 0) ? (HCI_VSC_COEX_A2DP_SOURCE_ID) : (HCI_VSC_COEX_A2DP_SINK_ID);

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr, opcode);
    UINT8_TO_STREAM(ptr, HCI_VSC_COEX_A2DP_CMDS_PARA_SIZE);
    UINT16_TO_STREAM(ptr, acl_handle);
    UINT8_TO_STREAM(ptr, HCI_VSC_COEX_A2DP_ID);
    UINT8_TO_STREAM(ptr, A2DP_COEX_START);

    send_vendor_cmd(buf);
}

void Avdtp_coex_stop_cmd(uint16_t acl_handle, uint8_t in)
{
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    uint8_t *ptr = buf;
    uint16_t opcode = (HCI_VSC_COEX_CMDS | HCI_GRP_VENDOR_SPECIFIC);
    uint8_t HCI_VSC_COEX_A2DP_ID = (in != 0) ? (HCI_VSC_COEX_A2DP_SOURCE_ID) : (HCI_VSC_COEX_A2DP_SINK_ID);

    UINT8_TO_STREAM(ptr, HCI_COMMAND_PKT);
    UINT16_TO_STREAM(ptr,opcode);
    UINT8_TO_STREAM(ptr, HCI_VSC_COEX_A2DP_CMDS_PARA_SIZE);
    UINT16_TO_STREAM(ptr, acl_handle);
    UINT8_TO_STREAM(ptr, HCI_VSC_COEX_A2DP_ID);
    UINT8_TO_STREAM(ptr, A2DP_COEX_STOP);

    send_vendor_cmd(buf);
}

void init_vendor_xr(int device)
{
    dev_id = device;
}
#endif

