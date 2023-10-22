#include "type.h"
#include "head.h"
#include "link.h"
#include "driver_hpsram.h"

#define HSPSRAM_CLK_CTRL_REG	(0x4003c06c)

#define GPRCM_SYSTEM_PRIV_REG1	(0x40050204)
#define GPRCM_SYSTEM_PRIV_REG2	(0x40050208)
#define HPSRAM_CRC_BEFORE_REG	GPRCM_SYSTEM_PRIV_REG1
#define HPSRAM_CRC_AFTER_REG	GPRCM_SYSTEM_PRIV_REG2

int hpsram_enter_retention(standby_head_t  *head)
{

	hpsram_standby2_enter(0);

	return 0;
}

int hpsram_exit_retention(standby_head_t  *head)
{

	hpsram_standby2_exit(&head->hpsram_para);

	return 0;
}

uint32_t hpsram_crc(standby_head_t *para)
{
	uint32_t *pdata = (uint32_t *)0;
	uint32_t crc = 0;
	uint32_t start = 0;
	pdata = (uint32_t *)para->hpsram_crc.crc_start;
	start = (uint32_t)pdata;
	while (pdata < (uint32_t *)(start + para->hpsram_crc.crc_len)) {
		crc += *pdata;
		pdata++;
	}

	return crc;
}

void hpsram_crc_save(standby_head_t *para, uint8_t type)
{
	if (type == HPSRAM_CRC_BEFORE)
		writel(para->hpsram_crc.crc_before, HPSRAM_CRC_BEFORE_REG);
	else if (type == HPSRAM_CRC_AFTER)
		writel(para->hpsram_crc.crc_after, HPSRAM_CRC_AFTER_REG);
}

uint32_t hpsram_is_running(void)
{
	uint32_t reg_val;

	reg_val = readl(HSPSRAM_CLK_CTRL_REG);
	if (reg_val & (1 << 31)) {
		return 1;
	} else
		return 0;
}
