#ifndef _EFUSE_H_
#define _EFUSE_H_

int efuse_acl_ck(efuse_key_map_new_t *key_map,int burn);
void efuse_set_cfg_flg(int efuse_cfg_base,int bit_offset);
int efuse_uni_burn_key(unsigned int key_index, unsigned int key_value);
unsigned int efuse_sram_read_key(unsigned int key_index);
unsigned int efuse_read_chip_ver(void);

#endif

