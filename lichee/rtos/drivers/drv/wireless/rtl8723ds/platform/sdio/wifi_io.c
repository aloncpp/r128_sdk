//#include "include/common.h"
#define BUILD_SDIO 1
#ifdef BUILD_SDIO
#include <stdio.h>
#include "wifi_io.h"
#include "console.h"
//#include "_sdio.h"
/* test wifi driver */
#define ADDR_MASK 0x10000
#define LOCAL_ADDR_MASK 0x00000

#define WIFIMG_DBUG(fmt,args...) do { printf("AW-WIFI[%s,%d]"fmt,__func__,__LINE__,##args); }while(0)

#define dbg_host(x...) printf(x)
//#define WIFIMG_DBUG(fmt,args...)
#ifdef CONFIG_USE_SDIO
struct sdio_func *wifi_sdio_func = NULL;
extern uint8_t sdio_readb(struct mmc_card *card, uint32_t func_num, uint32_t addr, int32_t *err_ret);
extern void sdio_writeb(struct mmc_card *card, uint32_t func_num, const uint8_t b, uint32_t addr, int32_t *err_ret);
extern int32_t sdio_enable_func(struct mmc_card *card, uint32_t func_num);
extern int32_t sdio_disable_func(struct mmc_card *card, uint32_t func_num);
extern int32_t sdio_set_block_size(struct mmc_card *card, uint32_t fn_num, uint32_t blksz);
extern int sdio_memcpy_fromio(struct mmc_card *card, unsigned int func_num, void *dst, unsigned int addr, int count);
extern int sdio_memcpy_toio(struct mmc_card *card, unsigned int func_num, unsigned int addr, const void *src, int count);
extern void sdio_claim_host(struct mmc_card *card);
extern void sdio_release_host(struct mmc_card *card);
//extern int sdio_claim_irq(struct sdio_func *func, sdio_irq_handler_t *handler);
extern int sdio_release_irq(struct sdio_func *func);
extern uint16_t sdio_readw(struct sdio_func *func, unsigned int addr, int *err_ret);
extern uint32_t sdio_readl(struct sdio_func *func, unsigned int addr, int *err_ret);
extern void sdio_writew(struct sdio_func *func, uint16_t b, unsigned int addr, int *err_ret);
extern void sdio_writel(struct sdio_func *func, uint32_t b, unsigned int addr, int *err_ret);

struct sdio_func ** get_mmc_card_func(uint8_t card_id);

extern int rtw_fake_driver_probe(struct sdio_func *func);
#if 0
struct sdio_func {
	struct mmc_card		*card;		/* the card this device belongs to */
	void	(*irq_handler)(struct sdio_func *); /* IRQ callback */

	unsigned	int	max_blksize;	/* maximum block size */ //add
	unsigned	int	cur_blksize;	/* current block size */	 //add
	unsigned	int	enable_timeout;	/* max enable timeout in msec */ //add
	unsigned int	num;		/* function number *///add
	unsigned short		vendor;		/* vendor id */ //add
	unsigned short		device;		/* device id */ //add
	unsigned		num_info;	/* number of info strings */ //add
	const char		**info;		/* info strings */ //add
	unsigned char		class;		/* standard interface class *///add

	unsigned int			tmpbuf_reserved; //for tmpbuf 4 byte alignment
	unsigned char			tmpbuf[4];	/* DMA:able scratch buffer */

#ifdef CONFIG_READ_CIS
	struct sdio_func_tuple *tuples;
#endif
	void *drv_priv;
};
#endif


static int wifi_io_debug = 0;

int cmd_wifi_io_debug(int argc, char ** argv)
{
    if(argc >= 2) {
		wifi_io_debug = 1;
	}
	if(argc == 1) {
		wifi_io_debug = 0;
	}
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_wifi_io_debug, wgo, Console wifi io debug Command);


int rtl_sdio_probe(struct mmc_card *card)
{
	wifi_sdio_func = card->sdio_func[0];
	sdio_set_block_size(wifi_sdio_func->card, 1, 512);
	return 0;
}

int rtl_sdio_bus_probe(void)
{
	;
}

int rtl_sdio_bus_remove(void)
{
	;
}

int rtl_sdio_enable_func(struct sdio_func *func)
{
	if(wifi_io_debug)
		WIFIMG_DBUG("+-=\n");
	return (int)sdio_enable_func(func->card,func->num);
}

int	rtl_sdio_disable_func(struct sdio_func *func)
{
	if(wifi_io_debug)
		WIFIMG_DBUG("+-=\n");
	return (int)sdio_disable_func(func->card,func->num);
}

int rtl_sdio_claim_irq(struct sdio_func *func, void(*handler)(struct sdio_func *))
{
	if(wifi_io_debug)
		WIFIMG_DBUG("+-=\n");
	sdio_claim_irq(func, handler);
}

int rtl_sdio_release_irq(struct sdio_func *func)
{
	if(wifi_io_debug)
		WIFIMG_DBUG("+-=\n");
	sdio_release_irq(func);
}


void rtl_sdio_claim_host(struct sdio_func *func)
{
	sdio_claim_host(func->card);
}

void rtl_sdio_release_host(struct sdio_func *func)
{
	sdio_release_host(func->card);
}

unsigned char rtl_sdio_readb(struct sdio_func *func, unsigned int addr, int *err_ret)
{
	unsigned char value;
	value = (unsigned char)sdio_readb(func->card, func->num, addr, (int32_t*)err_ret);
	if(wifi_io_debug)
		WIFIMG_DBUG("addr: 0x%x, value = 0x%x\n",addr,value);
	return value;
}

unsigned short rtl_sdio_readw(struct sdio_func *func, unsigned int addr, int *err_ret)
{
	unsigned short value;
	value = (unsigned short)sdio_readw(func,addr, err_ret);
	if(wifi_io_debug)
		WIFIMG_DBUG("addr: 0x%x, value = 0x%x\n",addr,value);
	return value;
}
unsigned int rtl_sdio_readl(struct sdio_func *func, unsigned int addr, int *err_ret)
{
	unsigned int value;
	value = (unsigned int)sdio_readl(func,addr, err_ret);
	if(wifi_io_debug)
		WIFIMG_DBUG("addr: 0x%x, value = 0x%x\n",addr,value);
	return value;
}

void rtl_sdio_writeb(struct sdio_func *func, unsigned char b,unsigned int addr, int *err_ret)
{
	if(wifi_io_debug)
		WIFIMG_DBUG("addr: 0x%x, value = 0x%x\n",addr,b);
	sdio_writeb(func->card, func->num,b, addr, (int32_t*)err_ret);
}

void rtl_sdio_writew(struct sdio_func *func, unsigned short b,unsigned int addr, int *err_ret)
{
	if(wifi_io_debug)
		WIFIMG_DBUG("addr: 0x%x, value = 0x%x\n",addr,b);
	sdio_writew(func,b,addr,err_ret);
}

void rtl_sdio_writel(struct sdio_func *func, unsigned int b,unsigned int addr, int *err_ret)
{
	if(wifi_io_debug)
		WIFIMG_DBUG("addr: 0x%x, value = 0x%x\n",addr,b);
	sdio_writel(func, b, addr, err_ret);
}

int rtl_sdio_memcpy_fromio(struct sdio_func *func, void *dst,unsigned int addr, int count)
{
	if(wifi_io_debug)
		WIFIMG_DBUG("addr: 0x%x, size = 0x%x\n",addr,count);
	return (int)sdio_memcpy_fromio(func->card, func->num, dst, addr, count);
}

int rtl_sdio_memcpy_toio(struct sdio_func *func, unsigned int addr,void *src, int count)
{
	if(wifi_io_debug)
		WIFIMG_DBUG("addr: 0x%x, size = 0x%x\n",addr,count);
	return (int)sdio_memcpy_toio(func->card, func->num, addr, src, count);
}
//extern int rtl_sdio_bus_probe(void);
//extern int rtl_sdio_bus_remove(void);
//extern SDIO_BUS_OPS rtw_sdio_bus_ops = {
SDIO_BUS_OPS rtw_sdio_bus_ops = {
	rtl_sdio_bus_probe,
	rtl_sdio_bus_remove,
	rtl_sdio_enable_func,
	rtl_sdio_disable_func,
	NULL,
	NULL,
	rtl_sdio_claim_irq,
	rtl_sdio_release_irq,
	rtl_sdio_claim_host,
	rtl_sdio_release_host,
	rtl_sdio_readb,
	rtl_sdio_readw,
	rtl_sdio_readl,
	rtl_sdio_writeb,
	rtl_sdio_writew,
	rtl_sdio_writel,
	rtl_sdio_memcpy_fromio,
	rtl_sdio_memcpy_toio
};

int wifi_read(struct sdio_func *func, u32 addr, u32 cnt, void *pdata)
{
	int err;

	rtl_sdio_claim_host(func);

	err = rtl_sdio_memcpy_fromio(func, pdata, addr, cnt);
	if (err) {
		dbg_host("%s: FAIL(%d)! ADDR=%#x Size=%d\n", __func__, err, addr, cnt);
	}

	rtl_sdio_release_host(func);

	return err;
}

int wifi_write(struct sdio_func *func, u32 addr, u32 cnt, void *pdata)
{
	int err;
	u32 size;

	rtl_sdio_claim_host(func);

	size = cnt;
	err = rtl_sdio_memcpy_toio(func, addr, pdata, size);
	if (err) {
		dbg_host("%s: FAIL(%d)! ADDR=%#x Size=%d(%d)\n", __func__, err, addr, cnt, size);
	}

	rtl_sdio_release_host(func);

	return err;
}

u8 wifi_readb(struct sdio_func *func, u32 addr)
{
	int err;
	u8 ret = 0;

	rtl_sdio_claim_host(func);
	ret = rtl_sdio_readb(func, ADDR_MASK | addr, &err);
	rtl_sdio_release_host(func);

	if (err)
		dbg_host("%s: FAIL!(%d) addr=0x%05x\n", __func__, err, addr);

	return ret;
}

u16 wifi_readw(struct sdio_func *func, u32 addr)
{
	int err;
	u16 v;

	rtl_sdio_claim_host(func);
	v = rtl_sdio_readw(func, ADDR_MASK | addr, &err);
	rtl_sdio_release_host(func);
	if (err)
		dbg_host("%s: FAIL!(%d) addr=0x%05x\n", __func__, err, addr);

	return  v;
}

u32 wifi_readl(struct sdio_func *func, u32 addr)
{
	int err;
	u32 v;

	rtl_sdio_claim_host(func);
	v = rtl_sdio_readl(func, ADDR_MASK | addr, &err);
	rtl_sdio_release_host(func);

	return  v;
}

void wifi_writeb(struct sdio_func *func, u32 addr, u8 val)
{
	int err;

	rtl_sdio_claim_host(func);
	rtl_sdio_writeb(func, val, ADDR_MASK | addr, &err);
	rtl_sdio_release_host(func);
	if (err)
		dbg_host("%s: FAIL!(%d) addr=0x%05x val=0x%02x\n", __func__, err, addr, val);
}

void wifi_writew(struct sdio_func *func, u32 addr, u16 v)
{
	int err;

	rtl_sdio_claim_host(func);
	rtl_sdio_writew(func, v, ADDR_MASK | addr, &err);
	rtl_sdio_release_host(func);
	if (err)
		dbg_host("%s: FAIL!(%d) addr=0x%05x val=0x%04x\n", __func__, err, addr, v);
}

void wifi_writel(struct sdio_func *func, u32 addr, u32 v)
{
	int err;

	rtl_sdio_claim_host(func);
	rtl_sdio_writel(func, v, ADDR_MASK | addr, &err);
	rtl_sdio_release_host(func);
}

u8 wifi_readb_local(struct sdio_func *func, u32 addr)
{
	int err;
	u8 ret = 0;

	ret = rtl_sdio_readb(func, LOCAL_ADDR_MASK | addr, &err);

	return ret;
}

void wifi_writeb_local(struct sdio_func *func, u32 addr, u8 val)
{
	int err;

	rtl_sdio_writeb(func, val, LOCAL_ADDR_MASK | addr, &err);
}
void wifi_fake_driver_probe_rtlwifi(struct sdio_func *func)
{
	rtw_fake_driver_probe(func);
}

#endif
#endif
