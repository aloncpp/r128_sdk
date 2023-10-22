#include "type.h"
#include "arch.h"
#include "cache.h"

/**
  \brief   Clean D-Cache
  \details Cleans D-Cache
  */
void standby_CleanDCache(void)
{
	uint32_t ccsidr_val;
	uint32_t sets;
	uint32_t ways;

	volatile uint32_t *dccsw;

	/* select Level 1 data cache */
	*(volatile uint32_t *)M33_CSSELR = 0;
	__DSB();

	ccsidr_val = *(volatile uint32_t *)M33_CCSIDR;
	dccsw = (volatile uint32_t *)M33_DCCSW;

	/* clean D-Cache */
	sets = (uint32_t)(CCSIDR_SETS(ccsidr_val));
	do {
		ways = (uint32_t)(CCSIDR_WAYS(ccsidr_val));
		do {
			*dccsw = (((sets << SCB_DCCSW_SET_Pos) & SCB_DCCSW_SET_Msk) |
				((ways << SCB_DCCSW_WAY_Pos) & SCB_DCCSW_WAY_Msk)  );
		} while (ways-- != 0U);
	} while(sets-- != 0U);

	__DSB();
	__ISB();
}

/**
  \brief   Clean & Invalidate D-Cache
  \details Cleans and Invalidates D-Cache
  */
void standby_CleanInvalidateDCache(void)
{
	uint32_t ccsidr_val;
	uint32_t sets;
	uint32_t ways;

	volatile uint32_t *dccisw;

	/* select Level 1 data cache */
	*(volatile uint32_t *)M33_CSSELR = 0;
	__DSB();

	ccsidr_val = *(volatile uint32_t *)M33_CCSIDR;
	dccisw = (volatile uint32_t *)M33_DCCISW;

	/* clean & Invalid D-Cache */
	sets = (uint32_t)(CCSIDR_SETS(ccsidr_val));
	do {
		ways = (uint32_t)(CCSIDR_WAYS(ccsidr_val));
		do {
			*dccisw = (((sets << SCB_DCCISW_SET_Pos) & SCB_DCCISW_SET_Msk) |
				((ways << SCB_DCCISW_WAY_Pos) & SCB_DCCISW_WAY_Msk)  );
		} while (ways-- != 0U);
	} while(sets-- != 0U);

	__DSB();
	__ISB();
}

void standby_DisableDCache(void)
{
    uint32_t ccsidr_val;
    uint32_t sets;
    uint32_t ways;
    volatile uint32_t *dcisw;

    *(volatile uint32_t *)M33_CSSELR = 0;
    __DSB();

    *(volatile uint32_t *)M33_CCR &= ~(uint32_t)SCB_CCR_DC_Msk;
    __DSB();

    ccsidr_val = *(volatile uint32_t *)M33_CCSIDR;
    dcisw = (volatile uint32_t *)M33_DCISW;

    /* clean & invalidate D-Cache */
    sets = (uint32_t)(CCSIDR_SETS(ccsidr_val));
    do {
        ways = (uint32_t)(CCSIDR_WAYS(ccsidr_val));
        do {
            *dcisw = (((sets << SCB_DCCISW_SET_Pos) & SCB_DCCISW_SET_Msk) |
                       ((ways << SCB_DCCISW_WAY_Pos) & SCB_DCCISW_WAY_Msk)  );
        } while (ways-- != 0U);
    } while(sets-- != 0U);

    __DSB();
    __ISB();
}

/**
  \brief   Enable D-Cache
  \details Turns on D-Cache
  */
void standby_EnableDCache(void)
{
	uint32_t ccsidr_val;
	uint32_t sets;
	uint32_t ways;

	volatile uint32_t *dcisw;

	/* return if DCache is already enabled */
	if ((*(volatile uint32_t *)M33_CCR) & SCB_CCR_DC_Msk)
		return;

	/* select Level 1 data cache */
	*(volatile uint32_t *)M33_CSSELR = 0;
	__DSB();

	ccsidr_val = *(volatile uint32_t *)M33_CCSIDR;
	dcisw = (volatile uint32_t *)M33_DCISW;

	/* invalidate D-Cache */
	sets = (uint32_t)(CCSIDR_SETS(ccsidr_val));
	do {
		ways = (uint32_t)(CCSIDR_WAYS(ccsidr_val));
		do {
			*dcisw = (((sets << SCB_DCISW_SET_Pos) & SCB_DCISW_SET_Msk) |
				((ways << SCB_DCISW_WAY_Pos) & SCB_DCISW_WAY_Msk)  );
		} while (ways-- != 0U);
	} while(sets-- != 0U);
	__DSB();

	/* enable D-Cache */
	*(volatile uint32_t *)M33_CCR = (*(volatile uint32_t *)M33_CCR) | (uint32_t)SCB_CCR_DC_Msk;

	__DSB();
	__ISB();
}

/**
  \brief   Enable I-Cache
  \details Turns on I-Cache
  */
void standby_EnableICache(void)
{
	/* return if ICache is already enabled */
	if ((*(volatile uint32_t *)M33_CCR) & SCB_CCR_IC_Msk)
		return;

	__DSB();
	__ISB();
	/* invalidate I-Cache */
	*(volatile uint32_t *)M33_ICIALLU = 0;
	__DSB();
	__ISB();
	/* enable I-Cache */
	*(volatile uint32_t *)M33_CCR = (*(volatile uint32_t *)M33_CCR) | (uint32_t)SCB_CCR_IC_Msk;
	__DSB();
	__ISB();
}
