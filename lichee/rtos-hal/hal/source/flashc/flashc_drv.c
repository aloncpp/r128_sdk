#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <console.h>
#include <sunxi_hal_common.h>
#include <hal_atomic.h>
#include <hal_sem.h>
#include <hal_log.h>
#include <hal_cmd.h>
#include "hal_flash.h"
#include "hal_flashctrl.h"
#include <blkpart.h>
#include <hal_flashc_enc.h>
#include "hal_xip.h"
#include <image_header.h>

#define SPINOR_FMT(fmt) "spinor: "fmt
#define SPINOR_ERR(fmt, arg...) hal_log_err(SPINOR_FMT(fmt), ##arg)
#define SPINOR_INFO(fmt, arg...) hal_log_info(SPINOR_FMT(fmt), ##arg)

#define NOR_CMD_READ 0x03
#define NOR_CMD_FAST_READ 0x0B
#define NOR_CMD_DUAL_READ 0x3B
#define NOR_CMD_QUAD_READ 0x6B
#define NOR_CMD_DUAL_IO_READ 0xBB
#define NOR_CMD_QUAD_IO_READ 0xEB
#define NOR_CMD_PROG 0x02
#define NOR_CMD_QUAD_PROG 0x32
#define NOR_CMD_QUAD_IO_PROG 0x38
#define NOR_CMD_ERASE_BLK4K 0x20
#define NOR_CMD_ERASE_BLK32K 0x52
#define NOR_CMD_ERASE_BLK64K 0xD8
#define NOR_CMD_ERASE_CHIP 0x60
#define NOR_CMD_WREN 0x06
#define NOR_CMD_WRDI 0x04
#define NOR_CMD_READ_SR 0x05
#define NOR_CMD_WRITE_SR 0x01
#define NOR_CMD_READ_CR 0x15
#define NOR_CMD_READ_SCUR 0x2B
#define NOR_CMD_RESET_EN 0x66
#define NOR_CMD_RESET 0x99
#define NOR_CMD_RDID 0x9F

#define cmd_4bytes(cmd) (nor->addr_width == 4 ? cmd ## 4B : cmd)
#define NOR_CMD_EN4B 0xB7
#define NOR_CMD_EX4B 0xE9
#define NOR_CMD_MXC_EX4B 0x29
#define NOR_CMD_READ4B 0x13
#define NOR_CMD_FAST_READ4B 0x0C
#define NOR_CMD_DUAL_READ4B 0x3C
#define NOR_CMD_QUAD_READ4B 0x6C
#define NOR_CMD_DUAL_IO_READ4B 0xBC
#define NOR_CMD_QUAD_IO_READ4B 0xEC
#define NOR_CMD_PROG4B 0x12
#define NOR_CMD_QUAD_PROG4B 0x34
#define NOR_CMD_QUAD_IO_PROG4B 0x3E
#define NOR_CMD_ERASE_BLK4K4B 0x21
#define NOR_CMD_ERASE_BLK32K4B 0x5C
#define NOR_CMD_ERASE_BLK64K4B 0xDC

#define NOR_BUSY_MASK BIT(0)
#define NOR_SR_BIT_WEL BIT(1)
#define IH_SIZE 128

extern int get_xip_part_offset(void);
extern HAL_Status HAL_Xip_Init(uint32_t flash, uint32_t xaddr);
extern int nor_blkpart_init(void);
extern void HAL_PRCM_SetFlashCryptoNonce(uint8_t *nonce);
#define MAX_ID_LEN 3

enum {
    NOR_IOCTRL_GET_PAGE_SIZE = 0,
    NOR_IOCTRL_GET_BLK_SIZE,
    NOR_IOCTRL_GET_TOTAL_SIZE,
};

struct nor_info
{
    char *model;
    unsigned char id[MAX_ID_LEN];
    unsigned int total_size;

    int flag;
#define SUPPORT_4K_ERASE_BLK        BIT(0)
#define SUPPORT_32K_ERASE_BLK       BIT(1)
#define SUPPORT_64K_ERASE_BLK       BIT(2)
#define SUPPORT_DUAL_READ           BIT(3)
#define SUPPORT_QUAD_READ           BIT(4)
#define SUPPORT_QUAD_WRITE          BIT(5)
#define SUPPORT_INDIVIDUAL_PROTECT  BIT(6)
#define SUPPORT_ALL_ERASE_BLK       (SUPPORT_4K_ERASE_BLK | \
                                     SUPPORT_32K_ERASE_BLK | \
                                     SUPPORT_64K_ERASE_BLK)
#define SUPPORT_GENERAL             (SUPPORT_ALL_ERASE_BLK | \
                                     SUPPORT_QUAD_WRITE | \
                                     SUPPORT_QUAD_READ | \
                                     SUPPORT_DUAL_READ)
#define USE_4K_ERASE                BIT(20)
#define USE_IO_PROG_X4              BIT(21)
#define USE_IO_READ_X2              BIT(22)
#define USE_IO_READ_X4              BIT(23)
};

struct nor_flash
{
    unsigned char cmd_read;
    unsigned char cmd_write;

    unsigned int r_cmd_slen: 3;
    unsigned int w_cmd_slen: 3;
    unsigned int total_size;
    unsigned int blk_size;
    unsigned int page_size;
    unsigned int addr_width;

    struct nor_info *info;
    struct nor_factory *factory;

    hal_sem_t hal_sem;
};

struct nor_flash g_nor, *nor = &g_nor;
#ifdef CONFIG_XIP
static int xip_start_addr = 0;
static char xip_enc = 0;
#endif

static inline int nor_lock_init(void)
{
    nor->hal_sem = hal_sem_create(1);
    if (!nor->hal_sem) {
        SPINOR_ERR("create hal_sem lock for nor_flash failed");
        return -1;
    }
    return 0;
}

static inline int nor_lock(void)
{
    return hal_sem_wait(nor->hal_sem);
}

static inline int nor_unlock(void)
{
    return hal_sem_post(nor->hal_sem);
}

static const FlashBoardCfg g_flash_cfg[] = {
    {
        /* default flashc */
        .type = FLASH_DRV_FLASHC,
        //.mode = FLASH_READ_FAST_MODE,
        .mode = FLASH_READ_QUAD_IO_MODE,
        .clk = (96 * 1000 * 1000),
    },
};

FlashBoardCfg *get_flash_cfg(int devNum)
{
    return (FlashBoardCfg *)&g_flash_cfg[0];
}
#ifdef CONFIG_DRIVERS_SPINOR_PANIC_WRITE
int nor_panic_erase(unsigned int addr, unsigned int size)
{
    HAL_Status status = HAL_ERROR;

    status = HAL_Flash_Panic_Erase(0 , FLASH_ERASE_4KB, addr, size / 4096);
    if (status != HAL_OK) {
        printf("erase %d fail\n", 0);
	return status;
    }
    return status;
}

int nor_panic_write(unsigned int addr, char *buf, unsigned int size)
{
    HAL_Status status = HAL_ERROR;

    status = HAL_Flash_Panic_Write(0 , addr, buf, size);
    if (status != HAL_OK) {
	printf("erase %d fail\n", 0);
	return status;
    }
    return status;
}

int nor_panic_read(unsigned int addr, char *buf, unsigned int size)
{
    HAL_Status status = HAL_ERROR;

    status = HAL_Flash_Read(0 , addr, buf, size);
    if (status != HAL_OK) {
        printf("read %d fail\n", 0);
        return status;
    }
    return status;
}

#endif

int nor_erase(unsigned int addr, unsigned int size)
{
    HAL_Status status = HAL_ERROR;
    status = HAL_Flash_Open(0, 5000);
    if (status != HAL_OK) {
        printf("open %d fail\n", 0);
        return status;
    }

    status = HAL_Flash_Erase(0 , FLASH_ERASE_4KB, addr, size / 4096);
    if (status != HAL_OK) {
        printf("erase %d fail\n", 0);
        return status;
    }

    HAL_Flash_Close(0);
    if (status != HAL_OK) {
        printf("close %d fail\n", 0);
        return status;
    }
    return status;
}

int nor_write(unsigned int addr, char *buf, unsigned int size)
{
    HAL_Status status = HAL_ERROR;
    status = HAL_Flash_Open(0, 5000);
    if (status != HAL_OK) {
        printf("open %d fail\n", 0);
        return status;
    }

    status = HAL_Flash_Write(0 , addr, buf, size);
    if (status != HAL_OK) {
        printf("write %d fail\n", 0);
        return status;
    }

    HAL_Flash_Close(0);
    if (status != HAL_OK) {
        printf("close %d fail\n", 0);
        return status;
    }
    return status;
}

int nor_read(unsigned int addr, char *buf, unsigned int size)
{
    HAL_Status status = HAL_ERROR;
    status = HAL_Flash_Open(0, 5000);
    if (status != HAL_OK) {
        printf("open %d fail\n", 0);
        return status;
    }

    status = HAL_Flash_Read(0 , addr, buf, size);
    if (status != HAL_OK) {
        printf("read %d fail\n", 0);
        return status;
    }

    HAL_Flash_Close(0);
    if (status != HAL_OK) {
        printf("close %d fail\n", 0);
        return status;
    }
    return status;
}

int nor_ioctrl(int cmd, void *buf, unsigned int size)
{
    switch(cmd) {
        case NOR_IOCTRL_GET_PAGE_SIZE:
            *(unsigned int *) buf = nor->page_size;
            break;
        case NOR_IOCTRL_GET_BLK_SIZE:
            *(unsigned int *) buf = nor->blk_size;
            break;
        case NOR_IOCTRL_GET_TOTAL_SIZE:
            *(unsigned int *) buf = nor->total_size;
        default:
            break;
    }
    return 0;
}

static int cmd_bit(unsigned char cmd)
{
    switch (cmd)
    {
        case NOR_CMD_DUAL_READ:
        case NOR_CMD_DUAL_IO_READ:
        case NOR_CMD_DUAL_READ4B:
        case NOR_CMD_DUAL_IO_READ4B:
            return 2;
        case NOR_CMD_QUAD_READ:
        case NOR_CMD_QUAD_IO_READ:
        case NOR_CMD_QUAD_READ4B:
        case NOR_CMD_QUAD_IO_READ4B:
        case NOR_CMD_QUAD_PROG:
        case NOR_CMD_QUAD_IO_PROG:
        case NOR_CMD_QUAD_PROG4B:
        case NOR_CMD_QUAD_IO_PROG4B:
            return 4;
        default:
            return 1;
    }
}

struct nor_flash *get_nor_flash(void)
{
    return nor;
}

static struct nor_info default_info =
{
	.model = "w25q64jv",
	.id = {0xef, 0x40, 0x17},
	.total_size = 4*1024*1024,
};

int flashc_enc_init(uint8_t *nonce_key, uint32_t *aes_key)
{
    Flashc_Enc_Cfg enc_set;
    Flashc_Enc_Cfg *enc_cfg;

#ifdef CONFIG_XIP
    hal_flashc_enc_init(nor->total_size + FLASH_XIP_START_ADDR, 2);
    if (xip_enc) {
        enc_set.ch = hal_flashc_enc_alloc_ch();
        if (enc_set.ch < 0) {
	        printf("err: alloc channel failed.\n");
	        return -1;
        }
        enc_set.start_addr = xip_start_addr;
        enc_set.end_addr = xip_start_addr + FLASH_XIP_END_ADDR - FLASH_XIP_START_ADDR - IH_SIZE;
        enc_set.key_0 = aes_key[0];
        enc_set.key_1 = aes_key[0];
        enc_set.key_2 = aes_key[0];
        enc_set.key_3 = aes_key[0];
        enc_set.enable = 1;
        hal_flashc_set_enc(&enc_set);
    }
#else
    hal_flashc_enc_init(nor->total_size, 1);
#endif

    HAL_PRCM_SetFlashCryptoNonce(nonce_key);
    enc_cfg = get_flashc_enc_cfg();
    printf_enc_config(enc_cfg);
}

static int flashc_nor_init(void)
{
    int ret = -1;

    if (nor->info)
        return 0;

    ret = nor_lock_init();
    if (ret) {
        SPINOR_ERR("create hal_sem lock for nor_flash failed");
        goto out;
    }
    nor_lock();

    FlashBoardCfg *cfg = (FlashBoardCfg *)&g_flash_cfg[0];
    cfg->flashc.param.freq = cfg->clk; //for flashc
    cfg->flashc.param.cs_mode = FLASHC_FLASHCS1_PSRAMCS0; //sip flash
    HAL_Flash_Init(0, cfg);

    struct FlashDev *dev = getFlashDev(0);
    if (dev->chip->cfg.mJedec == 0) {
	    printf("no flash, skip init nor\n");
	    ret = -1;
	    goto out;
    }
    nor->info = malloc (sizeof(struct nor_info));
    nor->info->model = malloc(16);
    if (nor->info == NULL || nor->info->model == NULL) {
	    printf("malloc failed\n");
	    ret = -1;
	    goto out;
    }
    strcpy(nor->info->model, "flashc nor"); //FIXME
    nor->info->id[0] = (dev->chip->cfg.mJedec >> 0) & 0xff;
    nor->info->id[1] = (dev->chip->cfg.mJedec >> 8) & 0xff;
    nor->info->id[2] = (dev->chip->cfg.mJedec >> 16) & 0xff;
    nor->info->total_size = dev->chip->cfg.mSize;
    nor->total_size = dev->chip->cfg.mSize;
    nor->page_size = 256; //FIXME
    nor->blk_size = 4096; //FIXME

    SPINOR_INFO("Nor Flash %s size %uMB write %dbit read %dbit blk size %uKB",
            nor->info->model, nor->total_size / 1024 / 1024,
            cmd_bit(nor->cmd_write), cmd_bit(nor->cmd_read), nor->blk_size / 1024);
    SPINOR_INFO("Nor Flash ID (hex): %02x %02x %02x", nor->info->id[0],
            nor->info->id[1], nor->info->id[2]);

#ifdef CONFIG_FLASH_POWER_DOWN_PROTECT
    ret = HAL_Flash_Write_InitLock(dev);
    if (ret) {
        SPINOR_ERR("write lock init failed");
        goto unlock;
    }

    /* ensure all block is locked default */
    ret = HAL_Flash_Write_LockAll(dev, 0, 0);
    if (ret) {
        SPINOR_ERR("write lock all failed");
        goto unlock;
    }
    SPINOR_ERR("Nor Flash Lock All");
#endif

#ifdef CONFIG_DRIVERS_SPINOR_CACHE
    ret = nor_cache_init(nor);
    if (ret) {
        SPINOR_ERR("cache init failed");
        goto unlock;
    }
#endif

    ret = 0;
unlock:
    nor_unlock();
out:
    if (ret) {
	if (nor->info != NULL)
		free(nor->info);
	if (nor->info->model != NULL)
		free(nor->info->model);
        SPINOR_ERR("init nor flash failed");
    } else
        SPINOR_INFO("nor flash init ok");
    return ret;
}

int hal_flashc_init(void)
{
    int ret = 1;
    int xip_start = 0;
    FlashBoardCfg *cfg = NULL;

    ret = flashc_nor_init();
    if (ret)
        return ret;
#ifdef CONFIG_XIP
    xip_start = get_xip_part_offset();
    if (xip_start >= 0) {
        printf("rtos-xip:0x%x\n", xip_start);
    } else {
        printf("no part rtos-xip\n");
#ifdef CONFIG_COMPONENTS_AW_BLKPART_NO_GPT
        #define HARDCODE_XIP_OFFSET (152*1024)
        printf("when no gpt, hardcode xip offset in 0x%x\n", HARDCODE_XIP_OFFSET);
        xip_start = HARDCODE_XIP_OFFSET;
#endif
    }

#ifdef CONFIG_FLASHC_CPU_XFER_ONLY
    char *ih_buf = malloc(IH_SIZE);
#else
    char *ih_buf = hal_malloc_coherent(IH_SIZE);
#endif
    if (ih_buf == NULL) {
        printf("malloc  failed\n");
        return -1;
    }
    memset(ih_buf, 0, IH_SIZE);
    nor_read(xip_start, ih_buf, IH_SIZE);
    if (memcmp(ih_buf, "AWIH", 4) == 0) {
        printf("xip with image header\n");
        xip_start += IH_SIZE;
    }
    xip_start_addr = xip_start;
    image_header_t *hdr = (image_header_t *)ih_buf;
    if (hdr->ih_imgattr & 0x08) {
        xip_enc = 1;
        printf("encrypted xip image\n");
    }
#ifdef CONFIG_FLASHC_CPU_XFER_ONLY
    free(ih_buf);
#else
    hal_free_coherent(ih_buf);
#endif

    HAL_Xip_Init(0, xip_start);
#endif
    cfg = get_flash_cfg(0);
    if (cfg == NULL) {
        printf("getFlashBoardCfg failed\n");
        return -1;
    }
#ifdef FLASH_XIP_OPT_READ
    cfg->flashc.param.optimize_mask = FLASH_OPTIMIZE_READ;
#else
    cfg->flashc.param.optimize_mask = 0;
#endif
#ifdef FLASH_XIP_OPT_WRITE
    cfg->flashc.param.optimize_mask |= FLASH_OPTIMIZE_WRITE;
#endif
    HAL_Flash_InitLater(0, cfg);
#ifndef CONFIG_COMPONENTS_AMP
    ret = nor_blkpart_init();
#elif defined(CONFIG_COMPONENTS_AMP) && !defined(CONFIG_AMP_FLASHC_SERVICE)
    ret = nor_blkpart_init();
#endif
    return ret;
}
