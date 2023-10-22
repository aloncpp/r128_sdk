/*
 * Copyright 2018 AllWinnertech  Co., Ltd
 * wangwei@allwinnertech.com
 */
#include <stdio.h>
#include <string.h>

#include <sunxi_hal_efuse.h>
#include <sunxi_hal_common.h>
#include <hal_mem.h>

#include "platform_efuse.h"
#include "efuse.h"

#if (EFUSE_DBG_EN & (CONFIG_LOG_DEFAULT_LEVEL != 0))
#define EFUSE_DBG(fmt,args...)	printf(fmt ,##args)
#define EFUSE_DUMP_LEN 16
#define EFUSE_DBG_DUMP(key_name, key_data, key_len)  efuse_dump(key_name, key_data, key_len, EFUSE_DUMP_LEN)
#else
#define EFUSE_DBG(fmt,args...)
#define EFUSE_DBG_DUMP(...) do{} while(0);
#endif
#define EFUSE_ERR(fmt,args...)	printf(fmt ,##args)

#if EFUSE_DBG_EN
static  void efuse_dump(char *str,unsigned char *data,\
	int len, int align)
{
	int i = 0;
	if(str)
		printf("\n%s: ",str);
	for(i = 0; i<len; i++)
	{
		if((i%align) == 0)
		{
			printf("\n");
		}
		printf("%02x ",*(data++));
	}
	printf("\n");
}
#endif

int hal_efuse_write(char *key_name, unsigned char *key_data, size_t key_bit_len)
{
	unsigned long k_src = 0;
	unsigned int niddle = 0,tmp_data = 0,k_d_lft = 0 ;
	size_t key_len = key_bit_len>>3;
	efuse_key_map_new_t *key_map;
	int ret;

	if ((strlen(key_name) == 0)|| (key_len == 0) || ((unsigned long)key_data%4 != 0))
		return EFUSE_ERR_ARG;

	key_map = efuse_search_key_by_name(key_name);

	if (((unsigned int)key_map->size >> 3) < key_len) {
		EFUSE_DBG("key name = %s\n", key_map->name);
		return EFUSE_ERR_KEY_SIZE_TOO_BIG;
	}

	if (key_map->size == 0) {
		EFUSE_DBG("%s: error: unknow key\n", __func__);
		return EFUSE_ERR_KEY_NAME_WRONG;
	}

	ret = efuse_acl_ck(key_map, 1);
	if(ret) {
		EFUSE_DBG("%s: __efuse_acl_ck check failed\n", __func__);
		return ret;
	}
	EFUSE_DBG_DUMP(key_name, key_data, key_len);

	/*rotpk need double check before burn*/
	if (strcmp(key_name, "rotpk") == 0) {
	}

	niddle = key_map->offset;
	k_d_lft = key_len;
	k_src = (unsigned long)key_data;

	while(k_d_lft >= 4) {
		tmp_data = *(unsigned int*)k_src;
		EFUSE_DBG("offset:0x%x val:0x%x\n",niddle,tmp_data);
		if(tmp_data)
		{
			if(efuse_uni_burn_key(niddle, tmp_data))
			{
				return  EFUSE_ERR_BURN_TIMING;
			}
		}
		k_d_lft-=4;
		niddle += 4;
		k_src +=4;
	}

	if(k_d_lft) {
		uint mask = (1UL << (k_d_lft << 3)) - 1;
		tmp_data = *(unsigned int*)k_src;
		mask &= tmp_data;
		EFUSE_DBG("offset:0x%x val:0x%x\n",niddle,mask);
		if(mask)
		{
			if(efuse_uni_burn_key(niddle,mask))
			{
				return  EFUSE_ERR_BURN_TIMING;
			}
		}
	}
	/*Already burned bit: Set this bit to indicate it is already burned.*/
	if ((key_map->burned_flg_offset >= 0) &&
	   (key_map->burned_flg_offset <= EFUSE_BRUN_RD_OFFSET_MASK)) {
		efuse_set_cfg_flg(EFUSE_WRITE_PROTECT,key_map->burned_flg_offset);
	}

	/*Read forbidden bit: Set to indicate cpu can not access this key again.*/
	if ((key_map->rd_fbd_offset >= 0) &&
	   (key_map->rd_fbd_offset <= EFUSE_BRUN_RD_OFFSET_MASK)) {
		efuse_set_cfg_flg(EFUSE_READ_PROTECT,key_map->rd_fbd_offset);
	}

	return 0;
}

/*This API assume the caller already
*prepared enough buffer to receive data.
*Because the lenth of key is exported as MACRO*/
#define EFUSE_ROUND_UP(x,y)  ((((x) + ((y) - 1)) / (y)) * (y))
int hal_efuse_read(char *key_name, unsigned char *key_data, size_t key_bit_len)
{
	efuse_key_map_new_t *key_map;
	uint tmp=0,i=0,k_u32_l=0,bit_lft = 0;
	int offset =0,tmp_sz = 0;
	int ret;
	/*if key_data not aligned ,u32_p will not be accessed*/
	unsigned int *u32_p = (unsigned int *)key_data;
	unsigned char *u8_p = (unsigned char *)key_data;
	if(!(key_name && key_data)) {
		EFUSE_DBG("[efuse] error arg: key_name is %p, key_data is %p\n",key_name, key_data);
		return EFUSE_ERR_ARG;
	}

	key_map = efuse_search_key_by_name(key_name);

	if (key_map->size == 0 || key_map->size > key_bit_len) {
		EFUSE_DBG("[efuse] error: unknow key name\n");
		return EFUSE_ERR_KEY_NAME_WRONG;
	}

	ret = efuse_acl_ck(key_map, 0);
	if(ret) {
		EFUSE_DBG("[sunxi_efuse_write] error: acl check fail\n");
		return ret;
	}
	EFUSE_DBG("key name:%s key size:%d key offset:%d\n",\
		key_map->name,key_map->size,key_map->offset);

	if (key_map->size > key_bit_len) {
		EFUSE_DBG("[efuse] error: key size\n");
		return EFUSE_ERR_ARG;
	}

	k_u32_l = key_map->size / 32;
	bit_lft = key_map->size % 32;
	offset = key_map->offset;

	for(i = 0;i<k_u32_l;i++)
	{
		tmp = efuse_sram_read_key(offset);
		if(((unsigned long)key_data & 0x3) == 0)
		{
			u32_p[i] = tmp;
		}
		else
		{
			memcpy((void*)(u8_p + i * 4),(void*)(&tmp),4);
		}
		offset += 4;
		tmp_sz += 4;
	}

	if(bit_lft)
	{
		EFUSE_DBG("bit lft is %d\n",bit_lft);
		tmp = efuse_sram_read_key(offset);
		memcpy((void*)(u8_p + k_u32_l * 4),(void*)(&tmp),
			   EFUSE_ROUND_UP(bit_lft,8) / 8);
		tmp_sz += (EFUSE_ROUND_UP(bit_lft,8) / 8);
	}

	EFUSE_DBG_DUMP(key_name, key_data, key_bit_len/8);

	return tmp_sz;
}

int hal_efuse_read_ext(uint32_t start_bit, uint32_t bit_num, uint8_t *data)
{
	if ((data == NULL) ||
		(start_bit >= EFUSE_BIT_NUM) ||
		(bit_num == 0) ||
		(bit_num > EFUSE_BIT_NUM) ||
		(start_bit + bit_num > EFUSE_BIT_NUM)) {
		EFUSE_ERR("arg error: start bit %u, bit num %u, data 0x%p\n", start_bit, bit_num, data);
		return EFUSE_ERR_ARG;
	}

	uint32_t start_word = start_bit / 32;
	uint8_t word_shift = start_bit % 32;
	uint8_t word_cnt = (start_bit + bit_num) / 32 - start_word;
	if ((start_bit + bit_num) % 32)
		word_cnt ++;

	uint32_t *sid_data = hal_malloc(word_cnt * 4);
	if (!sid_data) {
		EFUSE_ERR("efuse malloc %d bytes error\n", word_cnt * 4);
		return EFUSE_ERR_ARG;
	}
	int i;
	for (i = 0; i < word_cnt; i++) {
		sid_data[i] = efuse_sram_read_key((start_word + i) * 4);
	}
	for (i = 0; i < word_cnt - 1; i++) {
		sid_data[i] = (sid_data[i] >> word_shift) | (sid_data[i + 1] << (32 - word_shift));
	}
	sid_data[i] = (sid_data[i] >> word_shift);
	((uint8_t*)sid_data)[(bit_num - 1) / 8] &= ((1 << (bit_num % 8 == 0 ? 8 : bit_num % 8)) - 1);
	memcpy(data, (uint8_t*)sid_data, (bit_num + 7) / 8);

	hal_free(sid_data);

	return 0;
}
int hal_efuse_write_ext(uint32_t start_bit, uint32_t bit_num, uint8_t *data)
{
	int tmp = 0;
	unsigned long k_src = 0;

	if ((data == NULL) ||
		(start_bit >= EFUSE_BIT_NUM) ||
		(bit_num == 0) ||
		(bit_num > EFUSE_BIT_NUM) ||
		(start_bit + bit_num > EFUSE_BIT_NUM)) {
		EFUSE_ERR("arg error: start bit %u, bit num %u, data 0x%p\n", start_bit, bit_num, data);
		return EFUSE_ERR_ARG;
	}

	uint32_t start_word = start_bit / 32;
	uint8_t word_shift = start_bit % 32;
	uint8_t word_size = bit_num/8;
	uint8_t word_cnt = (start_bit + bit_num) / 32 - start_word;
	if ((start_bit + bit_num) % 32){
		word_cnt ++;
		word_size ++;
	}

	uint8_t *sid_data = hal_malloc(word_cnt * 4);
	if (!sid_data) {
		EFUSE_ERR("efuse malloc %d bytes error\n", word_cnt * 4);
		return EFUSE_ERR_ARG;
	}
	memset(sid_data, 0, word_cnt * 4);
	memcpy(sid_data, data, word_size);

	int i;
	for (i = 1; i < word_size; i++) {
		sid_data[word_size - i] = (sid_data[word_size - i] << word_shift) | (sid_data[word_size - i - 1] >> (8 - word_shift));
	}
	sid_data[0] = sid_data[0] << word_shift;
	k_src = (unsigned long)sid_data;

	for (i = 0; i < word_cnt; i++) {
		tmp = *(int *)k_src;
		printf("tmp:%x\n", tmp);

		if(efuse_uni_burn_key((start_word + i) * 4, tmp))
			return -1;
		k_src += 4;
	}
	return 0;
}

int hal_efuse_set_security_mode(void)
{
#ifdef EFUSE_LCJS
	return efuse_uni_burn_key(EFUSE_LCJS, 0x1 << SECURE_BIT_OFFSET);
#else
	return 0;
#endif
}


int hal_efuse_get_security_mode(void)
{
#ifdef EFUSE_SECURE_MODE
	return hal_readl(EFUSE_SECURE_MODE) & 0x1;
#else
	return 0;
#endif
}

int hal_efuse_get_rotpk_flag(void)
{
#ifdef SID_ROTPK_CTRL
	return (hal_readl(SID_ROTPK_CTRL) & 0x2) >> 1;
#else
	return 0;
#endif
}

int hal_efuse_get_chipid(unsigned char *buffer)
{
       return hal_efuse_read("chipid", buffer, 128);
}

//get mac api
int hal_efuse_get_mac(unsigned char *buffer)
{
#ifdef EFUSE_MAC_VER
		return hal_efuse_read_ext(EFUSE_MAC_OFFSET, EFUSE_MAC_WIDTH, buffer);
#else
		return 0;
#endif
}

int hal_efuse_get_mac_version(unsigned char *buffer)
{
#ifdef EFUSE_MAC_VER
    return hal_efuse_read_ext(EFUSE_MAC_VERSION_OFFSET, EFUSE_MAC_VERSION_WIDTH, buffer);
#else
	return 0;
#endif

}

int hal_efuse_get_mac1(unsigned char *buffer)
{
#ifdef EFUSE_MAC_VER
	return hal_efuse_read_ext(EFUSE_MAC1_OFFSET, EFUSE_MAC1_WIDTH, buffer);
#else
	return 0;
#endif
}

int hal_efuse_get_mac2(unsigned char *buffer)
{
#ifdef EFUSE_MAC_VER
	return hal_efuse_read_ext(EFUSE_MAC2_OFFSET, EFUSE_MAC2_WIDTH, buffer);
#else
	return 0;
#endif
}

int hal_efuse_get_mac3(unsigned char *buffer)
{
#ifdef EFUSE_MAC_VER
	return hal_efuse_read_ext(EFUSE_MAC3_OFFSET, EFUSE_MAC3_WIDTH, buffer);
#else
	return 0;
#endif
}

int hal_efuse_get_thermal_cdata(unsigned char *buffer)
{
       return hal_efuse_read("thermal_sensor", buffer, 64);
}

int hal_efuse_get_chip_ver(void)
{
	u32 val = 0;
	u32 ver = -1;

	val = efuse_read_chip_ver();
	if (val == 0x0)
		ver = CHIP_VER_A;
	else if (val == 0x1)
		ver =  CHIP_VER_B;
	else if (val == 0x2)
		ver =  CHIP_VER_C;
	else if (val == 0x3)
		ver =  CHIP_VER_D;
	else {
		EFUSE_ERR("no support this version\n");
		return -1;
	}

	return ver;
}

/**
 * get module_param:
 * argc[0]---dst buf
 * argc[1]---the sid offset
 * argc[2]---len(btye)
 */
int hal_get_module_param_from_sid(uint32_t *dst, uint32_t offset, uint32_t len)
{
	int i = 0;

	if (dst == NULL) {
		EFUSE_ERR("intput buf is NULL\n");
		return -1;
	}

	if ((len == 0) || (len & 0x3)) {
		EFUSE_ERR("len must be word align\n");
		return -2;
	}

	if (offset & 0x3) {
		EFUSE_ERR("offset must be word align\n");
		return -3;
	}

	for (i = 0; i < len; i += 4) {
		dst[i] = efuse_sram_read_key(offset + i);
	}

	return 0;
}

