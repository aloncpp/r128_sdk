#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "libc/errno.h"

#include "console.h"
#include "FreeRTOS.h"

#include "aactd/common.h"
#include "aactd/communicate.h"

#define PORT_DEFAULT 5005

#define INET_ADDR_STR_LEN INET_ADDRSTRLEN
#define COM_BUF_LEN_DEFAULT 1024

#define EQ_SW_FILTER_ARG_NUM 2
#define DRC_HW_REG_ARG_NUM 7
#define DRC_HW_REG_LONG_ARG_NUM 24
#define DRC3_HW_REG_LONG_ARG_NUM 26

#define EQ_HW_REG_ARG_NUM 26

static void help_msg(void)
{
    printf("\n");
    printf("USAGE:\n");
    printf("\taactd_test_client [ARGUMENTS]\n");
    printf("ARGUMENTS:\n");
    printf("\t-d,--drc  : test drchw\n");
    printf("\t-e,--eqsw   : test eqsw\n");
	printf("\t-f,--eqhw   : test eqhw\n");
	printf("\t-g,--drc3   : test drc3hw\n");
}

int cmd_aactd_client(int argc, char *argv[])
{
    int ret;

    int socket_fd;
    struct sockaddr_in server_addr;
    char inet_addr_str[INET_ADDR_STR_LEN];

    uint8_t com_buf[COM_BUF_LEN_DEFAULT];

    struct aactd_com_eq_sw_filter_arg eq_sw_filter_args[EQ_SW_FILTER_ARG_NUM];
    struct aactd_com_drc_hw_reg_arg drc_hw_reg_args[DRC_HW_REG_ARG_NUM];
	struct aactd_com_eq_hw_reg_arg eq_hw_reg_args[EQ_HW_REG_ARG_NUM];
	struct aactd_com_drc_hw_reg_long_arg drc_hw_reg_long_args[DRC_HW_REG_LONG_ARG_NUM];
	struct aactd_com_drc_hw_reg_long_arg drc3_hw_reg_long_args[DRC3_HW_REG_LONG_ARG_NUM];

    struct aactd_com com = {
        .data = com_buf + sizeof(struct aactd_com_header),
        .checksum = 0,
    };
    struct aactd_com_eq_sw_data eq_sw_data = {
        .filter_args = eq_sw_filter_args,
    };
    struct aactd_com_drc_hw_data drc_hw_data = {
        .reg_args = drc_hw_reg_args,
    };
    struct aactd_com_eq_hw_data eq_hw_data = {
        .reg_args = eq_hw_reg_args,
    };
    struct aactd_com_drc_hw_long_data drc_hw_long_data = {
        .reg_args = drc_hw_reg_long_args,
    };
    struct aactd_com_drc_hw_long_data drc3_hw_long_data = {
        .reg_args = drc3_hw_reg_long_args,
    };

    enum aactd_com_type type = AACTD_TYPE_RESERVED;
    unsigned int com_buf_actual_len;
    ssize_t write_bytes;

    const struct option opts[] = {
        { "help", no_argument, NULL, 'h' },
        { "drc", no_argument, NULL, 'd' },
        { "eqsw", no_argument, NULL, 'e' },
        { "eqhw", no_argument, NULL, 'f' },
        { "drc3hw", no_argument, NULL, 'g' },
    };
    int opt;

    if (argc <= 1) {
        help_msg();
        ret = -1;
        goto out;
    }

    while ((opt = getopt_long(argc, argv, "hdefg", opts, NULL)) != -1) {
        switch (opt) {
        case 'h':
            help_msg();
            ret = 0;
            goto out;
        case 'd':
            type = DRC_HW;
            break;
        case 'e':
            type = EQ_SW;
            break;
		case 'f':
            type = EQ_HW;
            break;
		case 'g':
            type = DRC3_HW;
            break;
        default:
            aactd_error("Invalid argument\n");
            help_msg();
            ret = -1;
            goto out;
        }
    }

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        aactd_error("Failed to create socket\n");
        ret = -1;
        goto out;
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_addr.sin_port = htons(PORT_DEFAULT);

    ret = connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        aactd_error("Failed to connect to server %s, port %d\n",
                inet_ntop(AF_INET, &server_addr.sin_addr, inet_addr_str, INET_ADDR_STR_LEN),
                ntohs(server_addr.sin_port));
        goto close_connect_fd;
    }

    switch (type) {
    case EQ_SW:
        com.header.flag = 0xAA;
        com.header.version = 1;
        com.header.command = CMD_WRITE;
        com.header.type = EQ_SW;
        com.header.data_len = sizeof(eq_sw_data.global_enabled)
            + sizeof(eq_sw_data.filter_num)
            + sizeof(struct aactd_com_eq_sw_filter_arg) * EQ_SW_FILTER_ARG_NUM;
        aactd_com_header_to_buf(&com.header, com_buf);

        eq_sw_data.global_enabled = 1;
        eq_sw_data.filter_num = EQ_SW_FILTER_ARG_NUM;
        eq_sw_data.filter_args[0].type = 1;
        eq_sw_data.filter_args[0].frequency = 1000;
        eq_sw_data.filter_args[0].gain = -12;
        eq_sw_data.filter_args[0].quality = (int32_t)(1.5 * 100);
        eq_sw_data.filter_args[0].enabled = 1;
        eq_sw_data.filter_args[1].type = 1;
        eq_sw_data.filter_args[1].frequency = 9000;
        eq_sw_data.filter_args[1].gain = -12;
        eq_sw_data.filter_args[1].quality = (int32_t)(2.2 * 100);
        eq_sw_data.filter_args[1].enabled = 0;
        aactd_com_eq_sw_data_to_buf(&eq_sw_data, com_buf + sizeof(struct aactd_com_header));

        com_buf_actual_len = sizeof(struct aactd_com_header) + com.header.data_len + 1;
        com.checksum = aactd_calculate_checksum(com_buf, com_buf_actual_len - 1);
        *(com_buf + com_buf_actual_len - 1) = com.checksum;
    break;

#if defined(CONFIG_ARCH_SUN20IW2)

	case DRC_HW:
		com.header.flag = 0xBB;
        com.header.version = 1;
        com.header.command = CMD_WRITE;
        com.header.type = DRC_HW;
        com.header.data_len = sizeof(drc_hw_long_data.reg_num)
            + sizeof(struct aactd_com_drc_hw_reg_long_arg) * DRC_HW_REG_LONG_ARG_NUM;
        aactd_com_header_to_buf(&com.header, com_buf);

        drc_hw_long_data.reg_num = DRC_HW_REG_LONG_ARG_NUM;
        drc_hw_long_data.reg_args[0].offset = 0x0600;
        drc_hw_long_data.reg_args[0].value = 0x00000002;
        drc_hw_long_data.reg_args[1].offset = 0x0608;
        drc_hw_long_data.reg_args[1].value = 0x000000FB;
        drc_hw_long_data.reg_args[2].offset = 0x060C;
        drc_hw_long_data.reg_args[2].value = 0x000B77F0;
        drc_hw_long_data.reg_args[3].offset = 0x0610;
        drc_hw_long_data.reg_args[3].value = 0x0001F2B7;

        drc_hw_long_data.reg_args[4].offset = 0x0614;
        drc_hw_long_data.reg_args[4].value = 0x00FFE1F8;
        drc_hw_long_data.reg_args[5].offset = 0x0618;
        drc_hw_long_data.reg_args[5].value = 0x00FFE997;
        drc_hw_long_data.reg_args[6].offset = 0x061C;
        drc_hw_long_data.reg_args[6].value = 0x00012BB0;
		drc_hw_long_data.reg_args[7].offset = 0x0620;
        drc_hw_long_data.reg_args[7].value = 0x00012BB0;

        drc_hw_long_data.reg_args[8].offset = 0x0624;
        drc_hw_long_data.reg_args[8].value = 0x0550A967;
        drc_hw_long_data.reg_args[9].offset = 0x0628;
        drc_hw_long_data.reg_args[9].value = 0x0115555A;
        drc_hw_long_data.reg_args[10].offset = 0x062C;
        drc_hw_long_data.reg_args[10].value = 0xF9144E14;
		drc_hw_long_data.reg_args[11].offset = 0x0630;
        drc_hw_long_data.reg_args[11].value = 0x02A854B4;


        drc_hw_long_data.reg_args[12].offset = 0x0634;
        drc_hw_long_data.reg_args[12].value = 0x000AAAAC;
        drc_hw_long_data.reg_args[13].offset = 0x0638;
        drc_hw_long_data.reg_args[13].value = 0xFBF5548D;
        drc_hw_long_data.reg_args[14].offset = 0x063C;
        drc_hw_long_data.reg_args[14].value = 0x0B599470;
		drc_hw_long_data.reg_args[15].offset = 0x0640;
        drc_hw_long_data.reg_args[15].value = 0x0280001C;


        drc_hw_long_data.reg_args[16].offset = 0x0644;
        drc_hw_long_data.reg_args[16].value = 0xF34414D6;
        drc_hw_long_data.reg_args[17].offset = 0x0648;
        drc_hw_long_data.reg_args[17].value = 0x00F69AFF;
        drc_hw_long_data.reg_args[18].offset = 0x064C;
        drc_hw_long_data.reg_args[18].value = 0x00017665;
		drc_hw_long_data.reg_args[19].offset = 0x0650;
        drc_hw_long_data.reg_args[19].value = 0x00000F04;


        drc_hw_long_data.reg_args[20].offset = 0x0654;
        drc_hw_long_data.reg_args[20].value = 0x03A77477;
        drc_hw_long_data.reg_args[21].offset = 0x0658;
        drc_hw_long_data.reg_args[21].value = 0xFAD9DBE4;
        drc_hw_long_data.reg_args[22].offset = 0x065C;
        drc_hw_long_data.reg_args[22].value = 0x00025600;
		drc_hw_long_data.reg_args[23].offset = 0x0660;
        drc_hw_long_data.reg_args[23].value = 0x11;


        aactd_com_drc_hw_long_data_to_buf(&drc_hw_long_data, com_buf + sizeof(struct aactd_com_header));

        com_buf_actual_len = sizeof(struct aactd_com_header) + com.header.data_len + 1;
        com.checksum = aactd_calculate_checksum(com_buf, com_buf_actual_len - 1);
        *(com_buf + com_buf_actual_len - 1) = com.checksum;
	break;

#elif defined(CONFIG_ARCH_SUN8IW18) || defined(CONFIG_ARCH_SUN8IW19) || defined(CONFIG_ARCH_SUN8IW20)

    case DRC_HW:
        com.header.flag = 0xAA;
        com.header.version = 1;
        com.header.command = CMD_WRITE;
        com.header.type = DRC_HW;
        com.header.data_len = sizeof(drc_hw_data.reg_num)
            + sizeof(struct aactd_com_drc_hw_reg_arg) * DRC_HW_REG_ARG_NUM;
        aactd_com_header_to_buf(&com.header, com_buf);

        drc_hw_data.reg_num = DRC_HW_REG_ARG_NUM;
        drc_hw_data.reg_args[0].offset = 0xF0;
        drc_hw_data.reg_args[0].value = 0xA0000000;
        drc_hw_data.reg_args[1].offset = 0x108;
        drc_hw_data.reg_args[1].value = 0x00FB;
        drc_hw_data.reg_args[2].offset = 0x10C;
        drc_hw_data.reg_args[2].value = 0x000B;
        drc_hw_data.reg_args[3].offset = 0x110;
        drc_hw_data.reg_args[3].value = 0x77F0;
        drc_hw_data.reg_args[4].offset = 0x114;
        drc_hw_data.reg_args[4].value = 0x000B;
        drc_hw_data.reg_args[5].offset = 0x118;
        drc_hw_data.reg_args[5].value = 0x77F0;
        drc_hw_data.reg_args[6].offset = 0x11C;
        drc_hw_data.reg_args[6].value = 0x00FF;
        aactd_com_drc_hw_data_to_buf(&drc_hw_data, com_buf + sizeof(struct aactd_com_header));

        com_buf_actual_len = sizeof(struct aactd_com_header) + com.header.data_len + 1;
        com.checksum = aactd_calculate_checksum(com_buf, com_buf_actual_len - 1);
        *(com_buf + com_buf_actual_len - 1) = com.checksum;
    break;
#endif
    case EQ_HW:
        com.header.flag = 0xBB;
        com.header.version = 1;
        com.header.command = CMD_WRITE;
        com.header.type = EQ_HW;
        com.header.data_len = sizeof(eq_hw_data.reg_num)
            + sizeof(struct aactd_com_eq_hw_reg_arg) * EQ_HW_REG_ARG_NUM;
        aactd_com_header_to_buf(&com.header, com_buf);

        eq_hw_data.reg_num = EQ_HW_REG_ARG_NUM;
        eq_hw_data.reg_args[0].offset = 0x0500;
        eq_hw_data.reg_args[0].value = 0x00000001;
        eq_hw_data.reg_args[1].offset = 0x0504;
        eq_hw_data.reg_args[1].value = 0x0000f16f;
        eq_hw_data.reg_args[2].offset = 0x0508;
        eq_hw_data.reg_args[2].value = 0x001e1d22;
        eq_hw_data.reg_args[3].offset = 0x050C;
        eq_hw_data.reg_args[3].value = 0x0000f16f;

        eq_hw_data.reg_args[4].offset = 0x0510;
        eq_hw_data.reg_args[4].value = 0x001e0086;
        eq_hw_data.reg_args[5].offset = 0x0514;
        eq_hw_data.reg_args[5].value = 0x0000ff7a;
        eq_hw_data.reg_args[6].offset = 0x0518;
        eq_hw_data.reg_args[6].value = 0x0000d765;
        eq_hw_data.reg_args[7].offset = 0x051C;
        eq_hw_data.reg_args[7].value = 0x001e5138;

        eq_hw_data.reg_args[8].offset = 0x0520;
        eq_hw_data.reg_args[8].value = 0x0000d763;
        eq_hw_data.reg_args[9].offset = 0x0524;
        eq_hw_data.reg_args[9].value = 0x001e0004;
        eq_hw_data.reg_args[10].offset = 0x0528;
        eq_hw_data.reg_args[10].value = 0x0000fffd;
        eq_hw_data.reg_args[11].offset = 0x052C;
        eq_hw_data.reg_args[11].value = 0x0000d765;

        eq_hw_data.reg_args[12].offset = 0x0530;
        eq_hw_data.reg_args[12].value = 0x001e513c;
        eq_hw_data.reg_args[13].offset = 0x0534;
        eq_hw_data.reg_args[13].value = 0x0000d761;
        eq_hw_data.reg_args[14].offset = 0x0538;
        eq_hw_data.reg_args[14].value = 0x001e0009;
        eq_hw_data.reg_args[15].offset = 0x053C;
        eq_hw_data.reg_args[15].value = 0x0000fffa;

        eq_hw_data.reg_args[16].offset = 0x0540;
        eq_hw_data.reg_args[16].value = 0x0000d766;
        eq_hw_data.reg_args[17].offset = 0x0544;
        eq_hw_data.reg_args[17].value = 0x001e5157;
        eq_hw_data.reg_args[18].offset = 0x0548;
        eq_hw_data.reg_args[18].value = 0x0000d75f;
        eq_hw_data.reg_args[19].offset = 0x054C;
        eq_hw_data.reg_args[19].value = 0x001e0028;

        eq_hw_data.reg_args[20].offset = 0x0550;
        eq_hw_data.reg_args[20].value = 0x0000fff8;
        eq_hw_data.reg_args[21].offset = 0x0554;
        eq_hw_data.reg_args[21].value = 0x0000d76f;
        eq_hw_data.reg_args[22].offset = 0x0558;
        eq_hw_data.reg_args[22].value = 0x001e648a;
        eq_hw_data.reg_args[23].offset = 0x055C;
        eq_hw_data.reg_args[23].value = 0x0000d6bb;

        eq_hw_data.reg_args[24].offset = 0x0560;
        eq_hw_data.reg_args[24].value = 0x001e16fa;
        eq_hw_data.reg_args[25].offset = 0x0564;
        eq_hw_data.reg_args[25].value = 0x0000ff41;

        aactd_com_eq_hw_data_to_buf(&eq_hw_data, com_buf + sizeof(struct aactd_com_header));

        com_buf_actual_len = sizeof(struct aactd_com_header) + com.header.data_len + 1;
        com.checksum = aactd_calculate_checksum(com_buf, com_buf_actual_len - 1);
        *(com_buf + com_buf_actual_len - 1) = com.checksum;

		aactd_info("com_buf_actual_len %d flag %x command %u type %u data len %lu reg_num %d reg: 0x%08llx, reg_val: 0x%08llx\n", \
					com_buf_actual_len, com_buf[0], com_buf[2], com_buf[3], (uint32_t)com_buf[4],
					(uint16_t)com_buf[8], (uint64_t)com_buf[10], (uint64_t)com_buf[18]);
	break;

	case DRC3_HW:

#if defined(CONFIG_ARCH_SUN20IW2)

		com.header.flag = 0xBB;
		com.header.version = 1;
		com.header.command = CMD_WRITE;
		com.header.type = DRC3_HW;
		com.header.data_len = sizeof(drc3_hw_long_data.reg_num)
			+ sizeof(struct aactd_com_drc_hw_reg_long_arg) * DRC3_HW_REG_LONG_ARG_NUM;
		aactd_com_header_to_buf(&com.header, com_buf);

		drc3_hw_long_data.reg_num = DRC3_HW_REG_LONG_ARG_NUM;
		drc3_hw_long_data.reg_args[0].offset = 0x0700;
		drc3_hw_long_data.reg_args[0].value = 0x00000002;
		drc3_hw_long_data.reg_args[1].offset = 0x0704;
		drc3_hw_long_data.reg_args[1].value = 0x00000703;
		drc3_hw_long_data.reg_args[2].offset = 0x0710;
		drc3_hw_long_data.reg_args[2].value = 0x01442066;
		drc3_hw_long_data.reg_args[3].offset = 0x0714;
		drc3_hw_long_data.reg_args[3].value = 0x01442066;

		drc3_hw_long_data.reg_args[4].offset = 0x0718;
		drc3_hw_long_data.reg_args[4].value = 0x00279073;
		drc3_hw_long_data.reg_args[5].offset = 0x071C;
		drc3_hw_long_data.reg_args[5].value = 0x00279073;
		drc3_hw_long_data.reg_args[6].offset = 0x0720;
		drc3_hw_long_data.reg_args[6].value = 0x6E0A82F4;
		drc3_hw_long_data.reg_args[7].offset = 0x0724;
		drc3_hw_long_data.reg_args[7].value = 0x6E0A82F4;

		drc3_hw_long_data.reg_args[8].offset = 0x0728;
		drc3_hw_long_data.reg_args[8].value = 0x79B63E36;
		drc3_hw_long_data.reg_args[9].offset = 0x072C;
		drc3_hw_long_data.reg_args[9].value = 0x79B63E36;
		drc3_hw_long_data.reg_args[10].offset = 0x0730;
		drc3_hw_long_data.reg_args[10].value = 0x01442066;
		drc3_hw_long_data.reg_args[11].offset = 0x0734;
		drc3_hw_long_data.reg_args[11].value = 0x01442066;


		drc3_hw_long_data.reg_args[12].offset = 0x0738;
		drc3_hw_long_data.reg_args[12].value = 0x00279073;
		drc3_hw_long_data.reg_args[13].offset = 0x073C;
		drc3_hw_long_data.reg_args[13].value = 0x00279073;
		drc3_hw_long_data.reg_args[14].offset = 0x0740;
		drc3_hw_long_data.reg_args[14].value = 0x91F57D0C;
		drc3_hw_long_data.reg_args[15].offset = 0x0744;
		drc3_hw_long_data.reg_args[15].value = 0x91F57D0C;


		drc3_hw_long_data.reg_args[16].offset = 0x0748;
		drc3_hw_long_data.reg_args[16].value = 0x8649C1CA;
		drc3_hw_long_data.reg_args[17].offset = 0x074C;
		drc3_hw_long_data.reg_args[17].value = 0x8649C1CA;
		drc3_hw_long_data.reg_args[18].offset = 0x0750;
		drc3_hw_long_data.reg_args[18].value = 0x01442066;
		drc3_hw_long_data.reg_args[19].offset = 0x0754;
		drc3_hw_long_data.reg_args[19].value = 0x01442066;


		drc3_hw_long_data.reg_args[20].offset = 0x0758;
		drc3_hw_long_data.reg_args[20].value = 0x00279073;
		drc3_hw_long_data.reg_args[21].offset = 0x075C;
		drc3_hw_long_data.reg_args[21].value = 0x00279073;
		drc3_hw_long_data.reg_args[22].offset = 0x0760;
		drc3_hw_long_data.reg_args[22].value = 0x6E0A82F4;
		drc3_hw_long_data.reg_args[23].offset = 0x0764;
		drc3_hw_long_data.reg_args[23].value = 0x6E0A82F4;
		drc3_hw_long_data.reg_args[24].offset = 0x0768;
		drc3_hw_long_data.reg_args[24].value = 0x79B63E36;
		drc3_hw_long_data.reg_args[25].offset = 0x076C;
		drc3_hw_long_data.reg_args[25].value = 0x79B63E36;


		aactd_com_drc_hw_long_data_to_buf(&drc3_hw_long_data, com_buf + sizeof(struct aactd_com_header));

		com_buf_actual_len = sizeof(struct aactd_com_header) + com.header.data_len + 1;
		com.checksum = aactd_calculate_checksum(com_buf, com_buf_actual_len - 1);
		*(com_buf + com_buf_actual_len - 1) = com.checksum;

#endif
		break;

    default:
        aactd_error("Unknown type\n");
        ret = -1;
        goto close_connect_fd;
    }

    write_bytes = aactd_writen(socket_fd, com_buf, com_buf_actual_len);
    if (write_bytes < 0) {
        aactd_error("Write error\n");
        ret = -1;
        goto close_connect_fd;
    } else if (write_bytes < com_buf_actual_len) {
        aactd_error("Write is incomplete\n");
        ret = -1;
        goto close_connect_fd;
    }


close_connect_fd:
    close(socket_fd);
out:
    return ret;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_aactd_client, aactd_client, Console aactd client command);

