#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sunxi_hal_efuse.h>
#include <hal_cmd.h>

#undef  HEXDUMP_LINE_CHR_CNT
#define HEXDUMP_LINE_CHR_CNT 16

static int sunxi_hexdump(const unsigned char *buf, int bytes)
{
    unsigned char line[HEXDUMP_LINE_CHR_CNT] = {0};
    int addr;

    for (addr = 0; addr < bytes; addr += HEXDUMP_LINE_CHR_CNT)
    {
        int len = ((bytes-addr)<HEXDUMP_LINE_CHR_CNT ? (bytes-addr) : HEXDUMP_LINE_CHR_CNT);
	int i;

        memcpy(line, buf + addr, len);
        memset(line + len, 0, HEXDUMP_LINE_CHR_CNT - len);

        /* print addr */
        printf("0x%.8X: ", addr);
        /* print hex */
        for (i = 0; i < HEXDUMP_LINE_CHR_CNT; i++)
        {
            if (i < len)
            {
                printf("%.2X ", line[i]);
            }
            else { printf("   "); }
        }
        /* print char */
        printf("|");
        for (i = 0; i < HEXDUMP_LINE_CHR_CNT; i++)
        {
            if (i < len)
            {
                if (line[i] >= 0x20 && line[i] <= 0x7E)
                {
                    printf("%c", line[i]);
                }
                else
                {
                    printf(".");
                }
            }
            else
            {
                printf(" ");
            }
        }
        printf("|\n");
    }
    return 0;
}

int cmd_test_efuse(int argc, char **argv)
{
	char buffer[16] = {0};
	char buffer2[12] = {0};
	char buffer3[1] = {0};
	char buffer4[3] = {0};
	char buffer5[3] = {0};
	char buffer6[3] = {0};
	printf("==========TEST READ CHIPID=========\n");
	hal_efuse_get_chipid(buffer);
	sunxi_hexdump(buffer, sizeof(buffer));

	printf("==========TEST READ MAC============\n");
	hal_efuse_get_mac(buffer2);
	printf("MAC:\n");
	sunxi_hexdump(buffer2, sizeof(buffer2));
	hal_efuse_get_mac_version(buffer3);
	printf("MAC_version:\n");
	sunxi_hexdump(buffer3, sizeof(buffer3));
	hal_efuse_get_mac1(buffer4);
	printf("MAC1:\n");
	sunxi_hexdump(buffer4, sizeof(buffer4));
	hal_efuse_get_mac2(buffer5);
	printf("MAC2:\n");
	sunxi_hexdump(buffer5, sizeof(buffer5));
	hal_efuse_get_mac3(buffer6);
	printf("MAC3:\n");
	sunxi_hexdump(buffer6, sizeof(buffer6));

	/* TODO: add more APIs to test */
	printf("===================================\n");
	printf("Test Finished.\n");

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_efuse, hal_efuse, efuse hal APIs tests)
