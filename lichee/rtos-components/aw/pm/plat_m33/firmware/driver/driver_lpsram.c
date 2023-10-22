
#include "type.h"
#include "head.h"
#include "link.h"
#include "driver_lpsram.h"
#include "./lpsram/sunxi_hal_lspsram.h"

#define LSPSRAM_CLK_CTRL_REG	(0x4003c070)

#define GPRCM_SYSTEM_PRIV_REG4	(0x40050210)
#define GPRCM_SYSTEM_PRIV_REG5	(0x40050214)
#define LPSRAM_CRC_BEFORE_REG	GPRCM_SYSTEM_PRIV_REG4
#define LPSRAM_CRC_AFTER_REG	GPRCM_SYSTEM_PRIV_REG5

int lpsram_enter_retention(standby_head_t  *head)
{
	psram_suspend(&head->lpsram_para);

	return 0;
}

int lpsram_exit_retention(standby_head_t  *head)
{
	int ret;

	ret = psram_resume(&head->lpsram_para);
	if (ret) {
	}

	return 0;
}

uint32_t lpsram_crc(standby_head_t *para)
{
	uint32_t *pdata = (uint32_t *)0;
	uint32_t crc = 0;
	uint32_t start = 0;
	pdata = (uint32_t *)para->lpsram_crc.crc_start;
	start = (uint32_t)pdata;
	while (pdata < (uint32_t *)(start + para->lpsram_crc.crc_len)) {
		crc += *pdata;
		pdata++;
	}

	return crc;
}

void lpsram_crc_save(standby_head_t *para, uint8_t type)
{
	if (type == LPSRAM_CRC_BEFORE)
		writel(para->lpsram_crc.crc_before, LPSRAM_CRC_BEFORE_REG);
	else if (type == LPSRAM_CRC_AFTER)
		writel(para->lpsram_crc.crc_after, LPSRAM_CRC_AFTER_REG);
}

uint32_t lpsram_is_running(void)
{
	uint32_t reg_val;

	reg_val = readl(LSPSRAM_CLK_CTRL_REG);
	if (reg_val & (1 << 31)) {
		return 1;
	} else
		return 0;
}
