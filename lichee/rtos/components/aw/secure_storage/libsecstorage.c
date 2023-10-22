#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

#include <drivers/hal_aes.h>
#include <drivers/hal_efuse.h>

#include <FreeRTOS.h>
#include <task.h>
#include <portable.h>
#include <aw_types.h>

#include "libsecstorage.h"

extern hal_aes_status_t hal_aes_cbc_encrypt(hal_aes_buffer_t *src_text,
						hal_aes_buffer_t *dst_text,
						hal_aes_buffer_t *key,
						u8 *iv);
extern hal_aes_status_t hal_aes_cbc_decrypt(hal_aes_buffer_t *src_text,
						hal_aes_buffer_t *dst_text,
						hal_aes_buffer_t *key,
						u8 *iv);

static u8 iv[16] = {
	0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};


#if 0
static void ss_dump(u8 *buf, int ttl_len)
{
	int len;
	printf("inbuf: 0x%x\n", buf);
	for (len = 0; len < ttl_len; len++) {
		printf("0x%02x ", ((char *)buf)[len]);
		if (len % 8 == 7) {
			printf("\n");
		}
	}
	printf("\n");
}
#endif

int ss_read(char *file, u8* dst_buffer)
{
	int ret, fd;
	struct stat s;
	u8 *src_buffer = NULL;
	int key_size = 16;
	u8 *key_data = NULL;
	hal_aes_buffer_t src_text;
	hal_aes_buffer_t dst_text;
	hal_aes_buffer_t key_text;

	/* 1. init src text */
	fd = open(file, O_RDONLY);
	if (fd < 0) {
		printf("open %s failed - %s\n", file, strerror(errno));
		ret = -1;
		goto close;
	}

	if (stat(file, &s)) {
		printf("stat %s failed - %s\n", file, strerror(errno));
		ret = -1;
		goto close;
	}

	src_buffer = (u8 *)pvPortMallocAlign(s.st_size, CE_ALIGN_SIZE);
	if (!src_buffer) {
		printf("malloc align %d failed\n", s.st_size);
		ret = -1;
		goto close;
	}
	memset(src_buffer, 0 , s.st_size);

	ret = read(fd, src_buffer, s.st_size);
	if (ret < 0) {
		printf("read %s failed - %s\n", file, strerror(errno));
		ret = -1;
		goto close;
	}

	src_text.buffer = src_buffer;
	src_text.length = s.st_size;

	/* 2. init dst text */
	dst_text.buffer = dst_buffer;
	dst_text.length = s.st_size;

	/* 3. init key text */
	key_data = (u8 *)pvPortMallocAlign(key_size, CE_ALIGN_SIZE);
	if (!key_data) {
		printf("malloc align %d failed\n", key_size);
		ret = -1;
		goto close;
	}
	memset(key_data, 0 , key_size);
	ret = hal_efuse_read("chipid", key_data, key_size << 3);
	if (ret != key_size) {
		printf("read chipid error!\n");
		ret = -1;
		goto close;
	}
	key_text.buffer = key_data;
	key_text.length = 16;

	/* 4. decrypt */
	ret = hal_aes_cbc_decrypt(&src_text, &dst_text, &key_text, iv);
	if (ret < 0) {
		printf ("aes cbc_encrypt fail %d\n", ret);
		ret = -1;
		goto close;
	}

close:
	if (key_data)
		free(key_data);
	if (src_buffer)
		free(src_buffer);
	if (fd)
		close(fd);
	return 0;
}

int ss_write(char *file, u8 *src_buffer, int length)
{
	int ret = 0;
	int fd;
	u8 *dst_buffer = NULL;
	int key_size = 16;
	char *key_data = NULL;
	hal_aes_buffer_t src_text;
	hal_aes_buffer_t dst_text;
	hal_aes_buffer_t key_text;

	if ((length % 16) != 0) {
		printf("ERROR: file %s length (%d) MUST be 16 bytes aligned\n", file, length);
		return -1;
	}

	/* 1. init src text */
	src_text.buffer = src_buffer;
	src_text.length = length;

	/* 2. init dst text */
	dst_buffer = (u8 *)pvPortMallocAlign(length, CE_ALIGN_SIZE);
	if (!dst_buffer) {
		printf("malloc align %d failed\n", length);
		ret = -1;
		goto close;
	}
	memset(dst_buffer, 0 , length);
	dst_text.buffer = dst_buffer;
	dst_text.length = length;

	/* 3. init key text */
	key_data = (u8 *)pvPortMallocAlign(key_size, CE_ALIGN_SIZE);
	if (!key_data) {
		printf("malloc align %d failed\n", key_size);
		ret = -1;
		goto close;
	}
	memset(key_data, 0 , key_size);

	ret = hal_efuse_read("chipid", key_data, key_size << 3);
	if (ret != key_size) {
		printf("read chipid error!\n");
		ret = -1;
		goto close;
	}
	key_text.buffer = key_data;
	key_text.length = 16;

	/* 4. encrypt */
	ret = hal_aes_cbc_encrypt(&src_text, &dst_text, &key_text, iv);
	if (ret < 0) {
		printf ("aes cbc_encrypt fail %d\n", ret);
		ret = -1;
		goto close;
	}

	/* 5. write encrypt data to file */
	fd = open(file, O_WRONLY | O_CREAT | O_TRUNC);
	if (fd < 0) {
		printf("open %s failed - %s\n", file, strerror(errno));
		ret = -1;
		goto close;
	}

	ret = write(fd, dst_buffer, dst_text.length);
	if (ret != dst_text.length) {
		printf("write %s failed - %s\n", file, strerror(errno));
		ret = -1;
		goto close;
	}

close:
	if (key_data)
		free(key_data);
	if (dst_buffer)
		free(dst_buffer);
	if (fd)
		close(fd);

	return ret;
}
