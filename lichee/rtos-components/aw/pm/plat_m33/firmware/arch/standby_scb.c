#include "type.h"
#include "arch.h"
#include "standby_scb.h"

volatile uint32_t scb_bak[SCB_RW_REGS_NUM];

void scb_save(void)
{
	volatile uint32_t *pdata;
	uint32_t start;
	int i;

	scb_bak[0] = *(volatile uint32_t *)SCB_ACTLR_REG;

	pdata = (volatile uint32_t *)SCB_ICSR_REG;
	start = (uint32_t)pdata;
	i = 1;
	while (pdata <= (uint32_t *)(start + 0x38))
	{
		scb_bak[i] = *pdata;
		/* 32bit point */
		pdata++;
		i++;
	}

	scb_bak[i] = *(volatile uint32_t *)SCB_CPACR_REG;
	i++;
	scb_bak[i] = *(volatile uint32_t *)SCB_NSACR_REG;
}

void scb_restore(void)
{
	volatile uint32_t *pdata;
	uint32_t start;
	int i;

	*(volatile uint32_t *)SCB_ACTLR_REG = scb_bak[0];

	pdata = (volatile uint32_t *)SCB_ICSR_REG;
	start = (uint32_t)pdata;
	i = 1;
	while (pdata <= (uint32_t *)(start + 0x38))
	{
		*(volatile uint32_t *)pdata = scb_bak[i];
		/* 32bit point */
		pdata++;
		i++;
	}

	*(volatile uint32_t *)SCB_CPACR_REG = scb_bak[i];
	i++;
	*(volatile uint32_t *)SCB_NSACR_REG = scb_bak[i];
}
