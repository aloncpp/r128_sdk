#include <types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <awlog.h>

#ifndef MIN
#define MIN(a, b) (a > b ? b : a)
#endif

#define HEXDUMP_LINE_CHR_CNT 16
int aw_hexdump(const char *buf, int bytes)
{
    char line[HEXDUMP_LINE_CHR_CNT] = {0};
    int addr;

    for (addr = 0; addr < bytes; addr += HEXDUMP_LINE_CHR_CNT) {
        int len = MIN(bytes, HEXDUMP_LINE_CHR_CNT), i;

        memcpy(line, buf + addr, len);
        memset(line + len, 0, HEXDUMP_LINE_CHR_CNT - len);

        /* print addr */
        printf("0x%.8X: ", (unsigned int)(intptr_t)(buf + addr));
        /* print hex */
        for (i = 0; i < HEXDUMP_LINE_CHR_CNT; i++) {
            if (i < len)
                printf("%.2X ", line[i]);
            else
                printf("  ");
        }
        /* print char */
        printf("|");
        for (i = 0; i < HEXDUMP_LINE_CHR_CNT; i++) {
            if (i < len) {
                if (line[i] >= 0x20 && line[i] <= 0x7E)
                    printf("%c", line[i]);
                else
                    printf(".");
            } else {
                printf(" ");
            }
        }
        printf("|\n");
    }
    return 0;
}
