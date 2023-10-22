#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <FreeRTOS.h>
#include <task.h>
#include <portable.h>
#include <console.h>
#include <aw_types.h>

#include "libsecstorage.h"

extern int sunxi_ce_init(void);

static void ss_usage(void)
{
	printf("Usage: \n");
	printf("  ss -r [file]                Read encrypted file data\n");
	printf("  ss -w [file] [hex-string]   Encrypt hex-string and write to file\n");
	printf("  ss -W [file] [src-file]     read and encrypt src-file data and write to file\n");
}

static void ss_dump(u8 *buf, int ttl_len)
{
	int len;
	printf("inbuf: 0x%x\n", (unsigned int)buf);
	for (len = 0; len < ttl_len; len++) {
		printf("0x%02x ", ((char *)buf)[len]);
		if (len % 8 == 7) {
			printf("\n");
		}
	}
	printf("\n");
}

static int hexstr_to_byte(const char* source, u8* dest, int sourceLen)
{
	int i;
	u8 highByte, lowByte;

	for (i = 0; i < sourceLen; i += 2) {
		highByte = toupper(source[i]);
		lowByte  = toupper(source[i + 1]);

		if (highByte < '0' || (highByte > '9' && highByte < 'A' ) || highByte > 'F') {
			printf("input buf[%d] is %c, not in 0123456789ABCDEF\n", i, source[i]);
			return -1;
		}

		if (lowByte < '0' || (lowByte > '9' && lowByte < 'A' ) || lowByte > 'F') {
			printf("input buf[%d] is %c, not in 0123456789ABCDEF\n", i+1, source[i+1]);
			return -1;
		}

		if (highByte > 0x39)
			highByte -= 0x37;
		else
			highByte -= 0x30;


		if (lowByte > 0x39)
			lowByte -= 0x37;
		else
			lowByte -= 0x30;

		dest[i / 2] = (highByte << 4) | lowByte;
	}
	return 0;
}

int ss_main(int argc, char *argv[])
{
	int ret = 0;
	int fd = 0;
	struct stat s;
	u8 *buffer = NULL;
	char *cmd = NULL;
	char full_name[256] = {0};
	char ori_name[256] = {0};
	u8 *hex_string = NULL;
	int hex_length = 0;

	if (argc != 3 && argc != 4) {
		ss_usage();
		return -1;
	}

	cmd = argv[1];
	snprintf(full_name, 256, "/secret/%s", argv[2]);

	ret = sunxi_ce_init();
	if (ret) {
		printf("ce init error\n");
		return ret;
	}

	if (!strcmp(cmd, "-r")) {
		if (stat(full_name, &s)) {
			printf("stat %s failed - %s\n", full_name, strerror(errno));
			ret = -1;
			goto close;
		}

		if (s.st_size % 16 != 0) {
			printf("ERROR: file %s length (%d) MUST be 16 bytes aligned\n", full_name, s.st_size);
			ret = -1;
			goto close;
		}

		buffer = (u8 *)pvPortMallocAlign(s.st_size, CE_ALIGN_SIZE);
		if (!buffer) {
			printf("malloc align %d failed\n", s.st_size);
			ret = -1;
			goto close;
		}
		memset(buffer, 0 , s.st_size);
		ret = ss_read(full_name, buffer);
		if (!ret)
			ss_dump(buffer, s.st_size);

	} else if (!strcmp(cmd, "-w")) {
		if (argc != 4) {
			ss_usage();
			ret = -1;
			goto close;
		}

		hex_string = argv[3];
		hex_length = strlen(hex_string);

		if (hex_length % 2 != 0) {
			printf("ERROR: hex_string length should be divided exactly by two\n");
			ret = -1;
			goto close;
		}

		buffer = (u8 *)pvPortMallocAlign(hex_length / 2, CE_ALIGN_SIZE);
		if (!buffer) {
			printf("malloc align %d failed\n", s.st_size);
			ret = -1;
			goto close;
		}
		memset(buffer, 0, hex_length / 2);

		ret = hexstr_to_byte(hex_string, buffer, hex_length);
		if (ret) {
			printf("hexstr_to_byte error\n");
			ret = -1;
			goto close;
		}

		ret = ss_write(full_name, buffer, hex_length / 2);
	} else if (!strcmp(cmd, "-W")) {
		if (argc != 4) {
			ss_usage();
			return -1;
		}

		snprintf(ori_name, 256, "/secret/%s", argv[3]);

		if (stat(ori_name, &s)) {
			printf("stat %s failed - %s\n", ori_name, strerror(errno));
			ret = -1;
			goto close;
		}

		if (s.st_size % 16 != 0) {
			printf("ERROR: file %s length (%d) MUST be 16 bytes aligned\n", ori_name, s.st_size);
			ret = -1;
			goto close;
		}

		buffer = (u8 *)pvPortMallocAlign(s.st_size, CE_ALIGN_SIZE);
		if (!buffer) {
			printf("malloc align %d failed\n", s.st_size);
			return -1;
		}
		memset(buffer, 0 , s.st_size);

		fd = open(ori_name, O_RDONLY);
		if (fd < 0) {
			printf("open %s failed - %s\n", ori_name, strerror(errno));
			ret = -1;
			goto close;
		}

		ret = read(fd, buffer, s.st_size);
		if (ret != s.st_size) {
			printf("read %s failed - %s\n", ori_name, strerror(errno));
			ret = -1;
			goto close;
		}

		ret = ss_write(full_name, buffer, s.st_size);
	} else {
		ss_usage();
	}

close:
	if (buffer)
		free(buffer);
	if (fd)
		close(fd);

	return ret;
}
FINSH_FUNCTION_EXPORT_CMD(ss_main, ss, secure storage demo);
