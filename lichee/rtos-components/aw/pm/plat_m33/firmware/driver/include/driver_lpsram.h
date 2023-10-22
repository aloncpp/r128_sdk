
#ifndef _DRIVER_LPSRAM_H_
#define _DRIVER_LPSRAM_H_

#include "head.h"

#define LPSRAM_CRC_BEFORE	(0x1)
#define LPSRAM_CRC_AFTER	(0x2)

#define LPSRAM_SUSPENDED	(0xf)
#define LPSRAM_RESUMED		(0x1)

int lpsram_enter_retention(standby_head_t  *head);
int lpsram_exit_retention(standby_head_t  *head);
uint32_t lpsram_crc(standby_head_t *para);
void lpsram_crc_save(standby_head_t *para, uint8_t type);
uint32_t lpsram_is_running(void);

#endif

