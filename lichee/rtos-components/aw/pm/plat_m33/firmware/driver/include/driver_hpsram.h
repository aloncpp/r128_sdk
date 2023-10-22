
#ifndef _DRIVER_HPSRAM_H_
#define _DRIVER_HPSRAM_H_

#include "head.h"

#define HPSRAM_CRC_BEFORE	(0x1)
#define HPSRAM_CRC_AFTER	(0x2)

#define HPSRAM_SUSPENDED	(0xf)
#define HPSRAM_RESUMED		(0x1)

int hpsram_enter_retention(standby_head_t  *head);
int hpsram_exit_retention(standby_head_t  *head);
uint32_t hpsram_crc(standby_head_t *para);
void hpsram_crc_save(standby_head_t *para, uint8_t type);
uint32_t hpsram_is_running(void);

void hpsram_standby2_enter(int hs_flag); /* for standby.bin */
void hpsram_standby2_exit(__dram_para_t *para); /* for standby.bin */

#endif

