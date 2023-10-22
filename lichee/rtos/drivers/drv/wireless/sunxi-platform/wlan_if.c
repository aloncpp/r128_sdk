/*
 * Filename:wlan_if.c
 * description: wireless interface adapter.
 * Created: 2019.07.22
 * Author:laumy
 */

#ifdef CONFIG_COMPONENTS_LWIP

#include <lwip/pbuf.h>
#include <lwip/stats.h>
#include <netif/etharp.h>
#include <stdio.h>
#include <stdbool.h>

#include "wlan_rf.h"

#ifdef CONFIG_USE_SDIO

#include <sdmmc/hal_sdhost.h>
#include <sdmmc/card.h>
#include <sdmmc/sdio.h>
#include <sdmmc/sdmmc.h>
#include <sdmmc/sys/sys_debug.h>
#endif

#ifdef CONFIG_RTL8723DS
#include "rtl8723ds.h"
#endif
#ifdef CONFIG_USE_SDIO
#define CONFIG_SDC_ID 1

#define SDIO_DBG printf
static void SDIO_PowerON(struct mmc_card *card)
{
	SDC_InitTypeDef sdc_param = { 0 };

#ifdef CONFIG_DETECT_CARD
	sdc_param.cd_mode = CARD_ALWAYS_PRESENT;
#endif
	sdc_param.debug_mask = ROM_WRN_MASK|ROM_ERR_MASK;
#ifdef CONFIG_SDC_DMA_USED
	sdc_param.dma_use = 1;
#else
	sdc_param.dma_use = 0;
#endif
	card->host = hal_sdc_create(CONFIG_SDC_ID, &sdc_param);
	hal_sdc_init(card->host);
}

static void SDIO_PowerOFF(struct mmc_card *card)
{
	hal_sdc_deinit(CONFIG_SDC_ID);
	hal_sdc_destroy(card->host);
}

static int SDIO_Initialize_Cards(struct mmc_card *card)
{
	int ret;

	ret = mmc_rescan(card, CONFIG_SDC_ID);
	if (ret) {
		return ret;
	}

	return 0;
}

static int SDIO_DeInitialize_Cards(struct mmc_card *card)
{
	int ret;

	ret = mmc_card_deinit(card);
	if (ret) {
	    return ret;
	}

	return 0;
}

struct mmc_card *wifi_sdio_detect(unsigned int id, int enable)
{
	struct mmc_card *card;
	SDCard_InitTypeDef card_param = { 0 };
	card_param.type = MMC_TYPE_SDIO;
	card_param.debug_mask = ROM_INF_MASK | ROM_WRN_MASK | ROM_ERR_MASK;

	mmc_card_create(CONFIG_SDC_ID, &card_param);
	card = mmc_card_open(CONFIG_SDC_ID);

	SDIO_PowerON(card);

	SDIO_Initialize_Cards(card);

	sdio_enable_func(card, FN1);
	OS_MSleep(1);
	mmc_card_close(CONFIG_SDC_ID);

	return card;

}

void wifi_sdio_deinit(struct mmc_card *card)
{
	if (!card)
		return ;
	sdio_disable_func(card, FN1);

	SDIO_DeInitialize_Cards(card);

	SDIO_PowerOFF(card);

	mmc_card_delete(CONFIG_SDC_ID);
}
#endif
int wlan_if_send(struct netif *netif,struct pbuf *p)
{
#ifdef CONFIG_RTL8723DS
	return rtl8723ds_wlan_send(netif,p);
#endif
}

err_t wlan_if_init_station(struct netif *netif)
{
#ifdef CONFIG_RTL8723DS
	return rlt8723ds_wlan_init_statin(netif);
#endif
}

err_t wlan_if_init_ap(struct netif *netif)
{
	;
}

#ifdef CONFIG_RTL8723DS
extern int rtl_sdio_probe(struct mmc_card *card);
#endif
int wlan_if_detect(void)
{
	struct mmc_card *card = NULL;
	int bus_index = -1;

	wlan_set_power(true);

	bus_index = wlan_get_bus_index();

	if(bus_index < 0) {
		printf("bus index error.\n");
		return -1;
	}
#ifdef CONFIG_USE_SDIO
	card = wifi_sdio_detect(1,0);
#endif
#ifdef CONFIG_RTL8723DS
	rtl_sdio_probe(card);
#endif
	return 0;
}

#endif /* CONFIG_COMPONENTS_LWIP */
