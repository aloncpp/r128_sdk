/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
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

#include <stdlib.h>
#include <string.h>

#include "sdd/sdd.h"
#include <fcntl.h>
#include <sys/unistd.h>

#ifdef CONFIG_CHIP_XRADIO
	#include "image/image.h"
	#include "image/flash.h"
	#include "sys/xr_debug.h"
#endif

#define CONFIG_SDD_DEBUG

#define SDD_LOGD_ON 0
#define SDD_LOGN_ON 0
#define SDD_LOGW_ON 1
#define SDD_LOGE_ON 1

#define SDD_DEBUG(flags, fmt, arg...)   \
	do {                                \
		if(flags)                       \
			printf("SDD: "fmt, ##arg);  \
	} while (0)

#define SDD_LOGD(format, args...) SDD_DEBUG(SDD_LOGD_ON, format, ##args)
#define SDD_LOGN(format, args...) SDD_DEBUG(SDD_LOGN_ON, format, ##args)
#define SDD_LOGW(format, args...) SDD_DEBUG(SDD_LOGW_ON, format, ##args)
#define SDD_LOGE(format, args...) SDD_DEBUG(SDD_LOGE_ON, format, ##args)

#define FIELD_OFFSET(type, field) ((uint8_t *)&((type *)0)->field - (uint8_t *)0)
#define FIND_NEXT_ELT(e) (struct sdd_ie *)((uint8_t *)&e->data + e->length)
#define FIND_NEXT_SEC(e) (struct sdd_sec_header *)((uint8_t *)&e->type + e->sec_length)
#define FIND_SEC_ELT(s) (struct sdd_ie *)((uint8_t *)&s->type + 2 + s->ie_length)

#define image_read(...) 1
#define image_check_header(a) 1
#define image_get_checksum(...) 1
#define flash_read(...) 1
#define flash_write(...) 1

#if defined(CONFIG_ARCH_SUN20IW2) && defined(CONFIG_COMPONENTS_AMP) && \
    defined(CONFIG_ARCH_ARM_ARMV8M)
#include "hal/sunxi_hal_common.h"
#endif

uint32_t sdd_request(struct sdd *sdd)
{
#ifdef CONFIG_CHIP_XRADIO
	section_header_t sh;
#else
	int f = -1;
	uint32_t size;
#endif

	if (!sdd) {
		SDD_LOGE("sdd in NULL!\n");
		return 0;
	}

#ifdef CONFIG_CHIP_XRADIO
	if (image_read(IMAGE_SYS_SDD_ID, IMAGE_SEG_HEADER, 0, &sh, IMAGE_HEADER_SIZE)
	    != IMAGE_HEADER_SIZE) {
		SDD_LOGE("load sdd header failed!\n");
		return 0;
	}
	if (image_check_header(&sh) == IMAGE_INVALID) {
		SDD_LOGE("check sdd header failed!\n");
		return 0;
	}

	if (SDD_MAX_SIZE < sh.body_len) {
		SDD_LOGE("define SDD_MAX_SIZE:%d bigger than sh.body_len:%d!\n", SDD_MAX_SIZE, sh.body_len);
		return 0;
	}

	sdd->size = sh.body_len;
#else
	struct stat f_statbuff;
	f = open(SYS_SDD_FILE, O_RDONLY);
	if (f == -1) {
		SDD_LOGE("open %s fail\n", SYS_SDD_FILE);
		return 0;
	}
	if (stat(SYS_SDD_FILE, &f_statbuff) != 0) {
		SDD_LOGE("get sddfile stat fail\n");
	}
	sdd->size = f_statbuff.st_size;
#endif

	if (SDD_MAX_SIZE < sdd->size) {
		SDD_LOGE("define SDD_MAX_SIZE:%d bigger than sdd->size:%d!\n", SDD_MAX_SIZE, sdd->size);
		sdd->size = 0;
		goto out;
	}

#if defined(CONFIG_ARCH_SUN20IW2) && defined(CONFIG_COMPONENTS_AMP) && \
    defined(CONFIG_ARCH_ARM_ARMV8M)
	sdd->data = (unsigned int *)amp_align_malloc(SDD_MAX_SIZE);
#else
	sdd->data = (unsigned int *)malloc(SDD_MAX_SIZE);
#endif
	if (!sdd->data) {
		SDD_LOGE("%s malloc faild!\n", __func__);
		sdd->size = 0;
		goto out;
	}

#ifdef CONFIG_CHIP_XRADIO
	if (image_read(IMAGE_SYS_SDD_ID, IMAGE_SEG_BODY, 0, sdd->data, sh.body_len) != sh.body_len) {
		SDD_LOGE("load sdd body failed!\n");
		return 0;
	}
#else
	size = read(f, (void*)sdd->data, sdd->size);
	if (size != sdd->size) {
		SDD_LOGE("read file:%s sz:%d need:%d\n", SYS_SDD_FILE, size, sdd->size);
		free(sdd->data);
		sdd->size = 0;
		goto out;
	}
#endif

out:
#ifdef CONFIG_CHIP_XRADIO
#else
	close(f);
#endif
	return sdd->size;
}

void sdd_release(struct sdd *sdd)
{
	if (!sdd || !sdd->data) {
		SDD_LOGE("sdd in NULL!\n");
		return ;
	}
#if defined(CONFIG_ARCH_SUN20IW2) && defined(CONFIG_COMPONENTS_AMP) && \
    defined(CONFIG_ARCH_ARM_ARMV8M)
	amp_align_free((void *)sdd->data);
#else
	free((void *)sdd->data);
#endif
	sdd->data = NULL;
}

/*
 * This function is called to save target ie to the SDD file in flash.
 */
int sdd_save(struct sdd *sdd)
{
#ifdef CONFIG_CHIP_XRADIO
	section_header_t sh;
	uint32_t type;
	uint32_t bin_start_addr, block_start_addr, write_addr, write_len;
	uint32_t block_len, block_cnt;
	uint16_t checksum;
	uint8_t *pWriteBuf;
	uint8_t *buf;

	/* 1.calculate address and length*/
	type = IMAGE_SYS_SDD_ID; /* only write sdd file now */
	write_len = sdd->size;
	if (write_len > 4 * 1024) {
		SDD_LOGE("sdd file length can not be more than 4096:%d\n", write_len);
		return -1;
	}
	buf = (uint8_t *)(sdd->data);
	bin_start_addr = image_get_section_addr(type);
	block_start_addr = bin_start_addr & 0xfffff000;
	block_len = (bin_start_addr & 0x00000fff) + write_len;
	block_cnt = block_len >> 12;
	if (block_len & 0x00000fff)
		block_cnt++;
	block_len = block_cnt << 12;//should be 4k
	SDD_LOGD("bin_start_addr:0x%08x\n", bin_start_addr);
	SDD_LOGD("block_start_addr:0x%08x\n", block_start_addr);
	SDD_LOGD("block_cnt:0x%08x\n", block_cnt);
	SDD_LOGD("block_len:0x%08x\n", block_len);
	SDD_LOGD("write_len:0x%08x\n", write_len);
	if (block_cnt > 1) {
		SDD_LOGE("error sdd file length or file is not aligned!block cnt:%d\n", block_cnt);
		return -1;
	}
	pWriteBuf = malloc(4 * 1024);
	if (pWriteBuf == NULL) {
		SDD_LOGE("failed to malloc write buffer\n");
		return -1;
	}

	/* 2.update header checksum */
	if (image_read(type, IMAGE_SEG_HEADER, 0, &sh,
	               IMAGE_HEADER_SIZE) != IMAGE_HEADER_SIZE) {
		SDD_LOGE("load section (id: %#08x) header failed\n", type);
		free(pWriteBuf);
		return -1;
	}
	if (image_check_header(&sh) == IMAGE_INVALID) {
		SDD_LOGE("check section (id: %#08x) header failed\n", type);
		free(pWriteBuf);
		return -1;
	}
	checksum = image_get_checksum(buf, write_len);
	sh.data_chksum = checksum;
	sh.header_chksum = 0;
	checksum = image_get_checksum(&sh, IMAGE_HEADER_SIZE);
	sh.header_chksum = 0xffff - checksum;
	SDD_LOGD("data_chksum:0x%08x\n", sh.data_chksum);
	SDD_LOGD("header_chksum:0x%08x\n", sh.header_chksum);

	/* 3.read old data first */
	if (flash_read(0, block_start_addr, pWriteBuf, block_len) != block_len) {
		SDD_LOGE("fail to read old data before!\n");
		free(pWriteBuf);
		return -1;
	}

	/* 4.erase the block */
	if (flash_erase(0, block_start_addr, block_len)) {
		SDD_LOGE("fail to erase block!\n");
		free(pWriteBuf);
		return -1;
	}

	/* 5.write data before header(if address is not 4k aligned) */
	write_addr = block_start_addr;
	if (bin_start_addr != block_start_addr) {
		int len_before = bin_start_addr - block_start_addr;
		SDD_LOGD("write_addr:0x%08x, len_before:0x%08x\n", write_addr, len_before);
		if (flash_write(0, write_addr, pWriteBuf, len_before) != len_before) {
			SDD_LOGE("fail to write old data!\n");
			free(pWriteBuf);
			return -1;
		}
		write_addr = bin_start_addr;
	}

	/* 6.write header 64byte */
	SDD_LOGD("write_addr:0x%08x, len:0x%08x\n", write_addr, IMAGE_HEADER_SIZE);
	if (flash_write(0, write_addr, &sh, IMAGE_HEADER_SIZE) != IMAGE_HEADER_SIZE) {
		SDD_LOGE("fail to write header data!\n");
		free(pWriteBuf);
		return -1;
	}
	write_addr += IMAGE_HEADER_SIZE;

	/* 7.write bin data */
	SDD_LOGD("write_addr:0x%08x, len:0x%08x\n", write_addr, write_len);
	if (flash_write(0, write_addr, buf, write_len) != write_len) {
		SDD_LOGE("fail to write bin data!\n");
		free(pWriteBuf);
		return -1;
	}
	write_addr += write_len;
	/* 8.write reset of old data */
	if (write_addr != (block_start_addr + block_len)) {
		int len_after = block_start_addr + block_len - write_addr;
		SDD_LOGD("write_addr:0x%08x, len_after:0x%08x\n", write_addr, len_after);
		if (flash_write(0, write_addr, pWriteBuf + IMAGE_HEADER_SIZE + write_len, len_after) != len_after) {
			SDD_LOGE("fail to write old data after!\n");
			free(pWriteBuf);
			return -1;
		}
		write_addr = bin_start_addr;
	}
	free(pWriteBuf);
#else
	int f = -1;
	uint32_t size;

	f = open(SYS_SDD_FILE, O_RDWR);
	if (f == -1) {
		SDD_LOGE("open %d fail\n", SYS_SDD_FILE);
		return 0;
	}
	size = write(f, (void *)sdd->data, sdd->size);
	if (size != sdd->size) {
		SDD_LOGE("write file:%s sz:%d need:%d\n", SYS_SDD_FILE, size, sdd->size);
	}
	close(f);
#endif

	return 0;
}

/*
 * This function is called to Parse the SDD file to extract target ie.
 */
struct sdd_ie *sdd_find_ie(struct sdd *sdd, int isec, int ies)
{
	uint32_t parsedLength = 0;
	uint32_t secLength = 0;
	struct sdd_ie *pElement = NULL;
	struct sdd_sec_header *pSection = NULL;

	if (!sdd || !sdd->data) {
		SDD_LOGE("sdd in NULL!\n");
		return NULL;
	}

	/* parse SDD config. */
	pElement = (struct sdd_ie *)sdd->data;
	parsedLength = (FIELD_OFFSET(struct sdd_ie, data) + pElement->length);
	pElement = FIND_NEXT_ELT(pElement);
	pSection = (struct sdd_sec_header *)pElement;

	while (parsedLength < sdd->size) {
		if (pSection->id == isec) {
			secLength = (FIELD_OFFSET(struct sdd_sec_header, type) + pSection->ie_length);
			for (pElement = FIND_SEC_ELT(pSection); secLength < pSection->sec_length;) {
				if (pElement->id == ies)
					return pElement;
				secLength += (FIELD_OFFSET(struct sdd_ie, data) + pElement->length);
				pElement = FIND_NEXT_ELT(pElement);
			}
		}
		parsedLength += pSection->sec_length;
		pSection = FIND_NEXT_SEC(pSection);

		if (pSection->id == SDD_LAST_SECT_ID)
			return NULL;
	}

	return NULL;
}

/*
 * This function is called to Parse the SDD file to update ie.
 */
int sdd_set_ie(struct sdd *sdd, int isec, int ies, uint8_t *data)
{
	int ret = 0;
	int parsedLength = 0;
	struct sdd_ie *pElement = NULL;

	if (!sdd || !sdd->data) {
		SDD_LOGE("sdd in NULL!\n");
		return -1;
	}

	pElement = sdd_find_ie(sdd, isec, ies);
	if (pElement != NULL) {
		parsedLength = pElement->length;
		SDD_LOGD("sdd ies:0x%02x, ie len:%d\n", ies, parsedLength);
		memcpy(pElement->data, data, parsedLength);
		ret = sdd_save(sdd);
	} else {
		SDD_LOGE("sdd ies:0x%02x not found!\n", ies);
		ret = -1;
	}

	return ret;
}

/*
 * This function is called to Print the SDD file
 * print_type:0-ASCII type, 1-HEX type
 */
int sdd_dump_file(int print_type)
{
	int ret;
	int i, sdd_len;
	uint8_t *data;
	struct sdd sdd;

	ret = sdd_request(&sdd);
	if (ret == 0) {
		SDD_LOGE("%s: can't load sdd file\n", __func__);
		return 0;
	}
	sdd_len = sdd.size;
	data = (uint8_t *)(sdd.data);
	printf("sdd file length:%d\n", sdd_len);
	printf("begin dump sdd file");
	if (print_type) {
		printf("\n");
		for (i = 0; i < sdd_len; i++) {
			printf("%c", data[i]);
		}
	} else {
		for (i = 0; i < sdd_len; i++) {
			if (((unsigned int)i & 0x0f) == 0x0) {
				printf("\n");
			}
			printf("%02x ", data[i]);
		}
		printf("\n");
	}
	printf("end dump sdd file\n");
	sdd_release(&sdd);

	return sdd_len;
}
