/**
 * @file  hal_flash.c
  * @author  XRADIO IOT WLAN Team
  */

/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "porting.h"
#include "hal_flash.h"
#include "hal_xip.h"
#include "hal_dev.h"
#include "hal_timer.h"
#include "hal_mem.h"
#include "hal_interrupt.h"
#ifdef CONFIG_FLASH_LOW_POWER_PROTECT
#include "sunxi_hal_power_protect.h"
#endif

//debug on r128, psram not ready
#define PSRAM_START_ADDR  (-1)
#define PSRAM_END_ADDR (-1)

#define __ISB() isb()
#define HAL_ThreadResumeScheduler()  xTaskResumeAll()
#define HAL_ThreadSuspendScheduler() vTaskSuspendAll()
#define HAL_IsIRQDisabled() hal_interrupt_is_disable()

#ifdef CONFIG_PM_FLASHC
static struct pm_devops flash_drv;
#else
#endif

#ifndef CONFIG_BOOTLOADER
uint8_t flash_dbg_mask = FLASH_ALE_FLAG | FLASH_ERR_FLAG | FLASH_NWA_FLAG | FD_ERR_FLAG | FD_INF_FLAG;// | FD_DBG_FLAG | FLASH_DBG_FLAG;

void HAL_Flash_SetDbgMask(uint8_t dbg_mask)
{
	flash_dbg_mask = dbg_mask;
}
#endif

HAL_Status HAL_Flash_WaitCompl(struct FlashDev *dev, int32_t timeout_ms)
{
#define FLASH_WAIT_TIME_US	10
	uint32_t _wait_us = 0;
	uint32_t timeout_temp = timeout_ms * 1000;
	while (dev->chip->isBusy(dev->chip) > 0) {
		//dev->drv->msleep(dev->chip, FLASH_WAIT_TIME);
		hal_udelay(FLASH_WAIT_TIME_US);
		timeout_temp -= FLASH_WAIT_TIME_US;
		if (timeout_temp <= 0) {
			FD_ERROR("%d flash wait clr busy timeout!", __LINE__);
			return HAL_TIMEOUT;
		}
		_wait_us += FLASH_WAIT_TIME_US;
#if 0
		if (_wait_us > (WDG_MIN_TIMEOUT_US / 5)) {
			HAL_Alive();
			_wait_us = 0;
		}
#endif
	}
	return HAL_OK;
#undef FLASH_WAIT_TIME_US
}

#if defined(FLASH_XIP_OPT_ERASR) || defined(FLASH_XIP_OPT_WRITE)
/**
 * check_irqs - check if any interrupts are pending
 */
static uint32_t check_irqstatus(struct FlashDev *dev)
{
	/* not check systick */

	for (int i = 0; i < FLASH_ISPR_MASK_NUM; i++) {
		if (NVIC->ISPR[i] & dev->ispr_mask[i])
			return 1;
	}

	return 0;
}

static int32_t HAL_Flash_Delay_Checkirq(struct FlashDev *dev, uint16_t us)
{
	uint64_t expire;
	int32_t left;

	expire = HAL_RTC_GetFreeRunCnt() + HAL_RTC_FreeRunTimeToCnt(us);
	do {
		if (check_irqstatus(dev)) {
			left = expire - HAL_RTC_GetFreeRunCnt();
			if (left < 0)
				return -1;
			else
				return left;
		}
	} while (expire > HAL_RTC_GetFreeRunCnt());

	return 0;
}

static HAL_Status HAL_Flash_WaitCompl_XIP_Optim(struct FlashDev *dev, int32_t timeout_ms)
{
#define FLASH_WAIT_TIME_US (10000)

	uint32_t _wait_us = 0;
	int32_t time_left;
	int32_t timeout_us = timeout_ms * 1000;
	uint32_t wip_timeout = dev->busy_timeout_us;
	uint64_t expire = HAL_RTC_GetFreeRunCnt() + HAL_RTC_FreeRunTimeToCnt(timeout_us);

	while (dev->chip->isBusy(dev->chip) > 0) {
		time_left = HAL_Flash_Delay_Checkirq(dev, dev->check_busy_us);
		if ((time_left == 0) && (wip_timeout > dev->check_busy_us)) {
			wip_timeout -= dev->check_busy_us;
			_wait_us += dev->check_busy_us;
			if (_wait_us > (WDG_MIN_TIMEOUT_US / 20)) {
				HAL_Alive();
				_wait_us = 0;
			}
			continue;
		}
		wip_timeout = dev->busy_timeout_us;

		dev->chip->suspendErasePageprogram(dev->chip);
		HAL_UDelay(dev->chip->cfg.mSuspend_Latency);
		_wait_us += dev->chip->cfg.mSuspend_Latency;
		if (dev->chip->isBusy(dev->chip)) {
			FD_DEBUG("%d flash suspend command ignored...\n", __LINE__);
			if (dev->chip->isSuspend(dev->chip)) {
				FD_ERROR("%d flash still in suspend...\n", __LINE__);
				return HAL_ERROR;
			} else {
				timeout_us -= time_left;
				dev->drv->msleep(dev->chip, FLASH_WAIT_TIME_US / 1000);
				timeout_us -= FLASH_WAIT_TIME_US;
				if (timeout_us <= 0) {
					FD_ERROR("%d flash wait clr busy timeout!", __LINE__);
					return HAL_TIMEOUT;
				}
				_wait_us += FLASH_WAIT_TIME_US;
				if (_wait_us > (WDG_MIN_TIMEOUT_US / 20)) {
					HAL_Alive();
					_wait_us = 0;
				}
			}
		} else {
			if (dev->chip->isSuspend(dev->chip)) {

				dev->drv->close(dev->chip);

				OS_MSleep(dev->switch_out_ms);

				dev->drv->open(dev->chip);

				dev->chip->resumeErasePageprogram(dev->chip);
				HAL_UDelay(dev->chip->cfg.mResume_Latency);
			}
		}

		if (HAL_RTC_GetFreeRunCnt() >= expire) {
			printf("wait complete timeout....\n");
			dev->chip->reset(dev->chip);
			return HAL_TIMEOUT;
		}
	}
	return HAL_OK;
}
#endif /* FLASH_XIP_OPT_WRITE || FLASH_XIP_OPT_ERASE */
#ifdef CONFIG_FLASH_POWER_DOWN_PROTECT
#define SZ_4K 4 * 1024
#define SZ_32K 32 * 1024
#define SZ_64K 64 * 1024
HAL_Status HAL_Flash_Write_InitLock(struct FlashDev *dev)
{
    int ret = 0;

    if (dev->chip->initLock) {
        dev->drv->open(dev->chip);
        ret = dev->chip->initLock(dev->chip);
        dev->drv->close(dev->chip);
    }
    return ret;
}

HAL_Status HAL_Flash_Write_UnLock(struct FlashDev *dev, uint32_t addr, uint32_t size)
{
    unsigned int align_size;
    unsigned int align_addr;
    int ret = 0;

    if (dev->chip->unLockBlock) {
	//dev->drv->open(dev->chip);
        while (size > 0) {
            /* the first and the last block (64K) is lock on sector (4K) unit */
            if (addr >= dev->chip->cfg.mSize - SZ_64K || addr < SZ_64K)
                align_size = SZ_4K;
            else
                align_size = SZ_64K;
            align_addr = ALIGN_DOWN(addr, align_size);
			dev->chip->writeEnable(dev->chip);
            ret = dev->chip->unLockBlock(dev->chip, align_addr);
            if (ret)
                break;
            size -= MIN(align_size, size);
            addr += MIN(align_size, size);
        }
        //dev->drv->close(dev->chip);
    }
    return ret;
}

HAL_Status HAL_Flash_Write_Lock(struct FlashDev *dev, uint32_t addr, uint32_t size)
{
    unsigned int align_size;
    unsigned int align_addr;
    int ret = 0;

    if (dev->chip->lockBlock) {
	//dev->drv->open(dev->chip);
        while (size > 0) {
            /* the first and the last block (64K) is lock on sector (4K) unit */
            if (addr >= dev->chip->cfg.mSize - SZ_64K || addr < SZ_64K)
                align_size = SZ_4K;
            else
                align_size = SZ_64K;
            align_addr = ALIGN_DOWN(addr, align_size);
			dev->chip->writeEnable(dev->chip);
            ret = dev->chip->lockBlock(dev->chip, align_addr);
            if (ret)
                break;
            size -= MIN(align_size, size);
            addr += MIN(align_size, size);
        }
	//dev->drv->close(dev->chip);
    }
    return ret;
}

HAL_Status HAL_Flash_Write_LockAll(struct FlashDev *dev, uint32_t addr, uint32_t size)
{
    int ret = 0;

    if (dev->chip->setGlobalBlockLockState) {
	dev->drv->open(dev->chip);
	ret = dev->chip->setGlobalBlockLockState(dev->chip, FLASH_BLOCK_STATE_LOCK);
	dev->drv->close(dev->chip);
    }
    return ret;
}
#endif
/**
  * @internal
  * @brief Flash driver open.
  * @param base: driver.
  * @retval HAL_Status: The status of driver
  */
HAL_Status flashcFlashOpen(struct FlashChip *chip)
{
	FD_DEBUG("open");
	return HAL_Flashc_Open(chip->flash_ctrl);
}

/**
  * @internal
  * @brief Flash driver close.
  * @param base: driver.
  * @retval HAL_Status: The status of driver
  */
HAL_Status flashcFlashClose(struct FlashChip *chip)
{
	HAL_Status ret = HAL_Flashc_Close(chip->flash_ctrl);
	FD_DEBUG("close");
	return ret;
}

void insToFcIns(InstructionField *ins, FC_InstructionField *fcins)
{
	if (ins == NULL)
		return;

	if (ins->pdata)
		fcins->pdata = ins->pdata;
	else
		fcins->pdata = (uint8_t *)&ins->data;
	fcins->len = ins->len;
	fcins->line = (FC_CycleBits)((ins->line == 4) ? FC_CYCLEBITS_4 : ins->line);
}

#define INS_CMD  (0)
#define INS_ADDR (1)
#define INS_DUM  (2)
#define INS_DATA (3)
#define INS_MAX  (4)

/**
  * @internal
  * @brief Flash driver write.
  * @param base: Driver.
  * @param cmd: Instruction command field.
  * @param addr: Instruction address field.
  * @param dummy: Instruction dummy field.
  * @param data: Instruction data field.
  * @retval HAL_Status: The status of driver
  */
HAL_Status flashcFlashWrite(struct FlashChip *dev, InstructionField *cmd,
                            InstructionField *addr, InstructionField *dummy, InstructionField *data)
{
	int dma = 0;
	FC_InstructionField tmp[INS_MAX];
	struct FlashDrv *drv = dev->mDriver;

	memset(tmp, 0, sizeof(tmp));

	insToFcIns(cmd, &tmp[INS_CMD]);
	insToFcIns(addr, &tmp[INS_ADDR]);
	insToFcIns(dummy, &tmp[INS_DUM]);
	insToFcIns(data, &tmp[INS_DATA]);

#ifndef CONFIG_FLASHC_CPU_XFER_ONLY
	if (tmp[INS_DATA].len >= (drv->sizeToDma - 1) &&  tmp[INS_DATA].len < 128 * 1024)
		dma = 1;
#endif

	return HAL_Flashc_Transfer(dev->flash_ctrl, 1, &tmp[INS_CMD],
	                           &tmp[INS_ADDR], &tmp[INS_DUM], &tmp[INS_DATA], dma);
}

/**
  * @internal
  * @brief Flash driver read.
  * @param base: Driver.
  * @param cmd: Instruction command field.
  * @param addr: Instruction address field.
  * @param dummy: Instruction dummy field.
  * @param data: Instruction data field.
  * @retval HAL_Status: The status of driver
  */
HAL_Status flashcFlashRead(struct FlashChip *dev, InstructionField *cmd,
                           InstructionField *addr, InstructionField *dummy, InstructionField *data)
{
	int dma = 0;
	FC_InstructionField tmp[INS_MAX];
	struct FlashDrv *drv = dev->mDriver;

	memset(tmp, 0, sizeof(tmp));

	insToFcIns(cmd, &tmp[INS_CMD]);
	insToFcIns(addr, &tmp[INS_ADDR]);
	insToFcIns(dummy, &tmp[INS_DUM]);
	insToFcIns(data, &tmp[INS_DATA]);

#ifndef CONFIG_FLASHC_CPU_XFER_ONLY
	if (tmp[INS_DATA].len >= (drv->sizeToDma - 1) && tmp[INS_DATA].len < 128 * 1024)
		dma = 1;
#endif

	return HAL_Flashc_Transfer(dev->flash_ctrl, 0, &tmp[INS_CMD],
	                           &tmp[INS_ADDR], &tmp[INS_DUM], &tmp[INS_DATA], dma);
}

/**
  * @internal
  * @brief Flash driver set working frequency.
  * @param base: Driver.
  * @param freq: Device and driver working frequency.
  * @retval HAL_Status: The status of driver
  */
HAL_Status flashcFlashSetFreq(struct FlashChip *dev, uint32_t freq)
{
	/* TODO: tbc... */
	return HAL_INVALID;
}

/**
  * @internal
  * @brief Sleep realization by driver.
  * @note For some reason(XIP), system sleep function can't be used in some
  *       case. So the realiztion of sleep should be performed by driver.
  * @param base: Driver.
  * @param ms: Sleep or wait sometime in millisecond.
  * @retval HAL_Status: The status of driver
  */
void flashcFlashMsleep(struct FlashChip *dev, uint32_t ms)
{
	HAL_Flashc_Delay(dev->flash_ctrl, ms * 1000);
}

/**
  * @internal
  * @brief Destroy flash driver.
  * @note This may not be used.
  * @param base: Driver.
  * @retval HAL_Status: The status of driver
  */
void flashcFlashDestroy(struct FlashChip *dev)
{
	HAL_Flashc_Deinit(dev->flash_ctrl);
	hal_free(dev->mDriver);
}

/**
  * @internal
  * @brief flash driver control.
  * @param base: Driver.
  * @param attr: flash control cmd.
  * @param arg: flash control arguement
  * @retval HAL_Status: The status of driver
  */
HAL_Status flashcFlashIoctl(struct FlashChip *dev, FlashControlCmd cmd, uintptr_t arg)
{
	FC_InstructionField fc_ins;
	uint8_t val;

	switch (cmd) {
	case FLASH_ENABLE_32BIT_ADDR:
		val = 0xB7;
		fc_ins.len = 1;
		fc_ins.line = 1;
		fc_ins.pdata = &val;

		HAL_Flashc_Transfer(dev->flash_ctrl, 0, &fc_ins, NULL, NULL, NULL, 1);
		HAL_Flashc_Ioctl(dev->flash_ctrl, FC_CMD_ENABLE_32BITADDR_MODE, (void *)arg);
		break;
	case FLASH_SET_OPTIMIZE_MASK:
		val = arg;
		HAL_Flashc_Ioctl(dev->flash_ctrl, FC_CMD_CONFIG_OPTIMIZE, &val);
		break;
	case FLASH_SET_RESET_MASK:
		val = arg;
		HAL_Flashc_Ioctl(dev->flash_ctrl, FC_CMD_CONFIG_RESET_MASK, &val);
		break;
	default:
		break;
	}
	return HAL_INVALID;
}

/**
  * @internal
  * @brief Create a flash driver.
  * @param dev: Flash device number, but not minor number.
  * @param bcfg: Config from board config.
  * @retval HAL_Status: The status of driver
  */
struct FlashDrv *flashcDriverCreate(int dev, FlashBoardCfg *bcfg)
{
	struct FlashDrv *drv;
	struct flash_controller *flash_ctrl;

	drv = hal_malloc(sizeof(struct FlashDrv));
	if (drv == NULL) {
		FD_ERROR("no mem");
		return drv;
	}
	memset(drv, 0, sizeof(struct FlashDrv));
	flash_ctrl = HAL_Flashc_Create(HAL_DEV_MINOR(dev));
	if (flash_ctrl == NULL) {
		FD_ERROR("flashc create failed");
		goto failed;
	}
	HAL_Flashc_IncRef(flash_ctrl);

	HAL_Flashc_Init(flash_ctrl, &bcfg->flashc.param);

	drv->platform_data = flash_ctrl;
	drv->dev = dev;
	drv->open = flashcFlashOpen;
	drv->close = flashcFlashClose;
	drv->read = flashcFlashRead;
	drv->write = flashcFlashWrite;
	drv->setFreq = flashcFlashSetFreq;
	drv->msleep = flashcFlashMsleep;
	drv->destroy = flashcFlashDestroy;
	drv->ioctl = flashcFlashIoctl;
	drv->sizeToDma = FLASH_DMA_TRANSFER_MIN_SIZE;

	return drv;

failed:
	hal_free(drv);
	return NULL;
}

/**
  * @internal
  * @brief Create a flash driver according board config.
  * @param minor: flash number = flash minor number, from board config.
  * @retval HAL_Status: The status of driver
  */
static struct FlashDrv *flashDriverCreate(int minor, FlashBoardCfg *cfg)
{
	struct FlashDrv *drv = NULL;
	int dev = HAL_MKDEV(HAL_DEV_MAJOR_FLASH, minor);

	if (cfg->type == FLASH_DRV_FLASHC) {
		drv = flashcDriverCreate(dev, cfg);
	}
#if FLASH_SPI_ENABLE
	else if (cfg->type == FLASH_DRV_SPI) {
		//drv = spiDriverCreate(dev, cfg);
	}
#endif

	if (drv == NULL)
		FD_ERROR("create fail");

	/*manage driver gpio. tbc...*/

	return drv;
}

/**
  * @internal
  * @brief Destroy a flash driver.
  * @param base: Driver.
  * @retval HAL_Status: The status of driver
  */
static int flashDriverDestory(struct FlashChip *chip)
{
	if (chip == NULL)
		return -1;

	chip->mDriver->destroy(chip);

	if (chip->dev->type == FLASH_DRV_FLASHC) {
		if (chip->flash_ctrl && HAL_Flashc_DecRef(chip->flash_ctrl) == 0) {
			HAL_Flashc_Destory(chip->flash_ctrl);
		}
	}
#if FLASH_SPI_ENABLE
	else if (chip->dev->type == FLASH_DRV_SPI) {
		; /* do nothing */
	}
#endif

	chip->controller = NULL;

	return 0;
}

struct list_head flashNodeHead = {
	.next = &flashNodeHead,
	.prev = &flashNodeHead
};

struct FlashChip *getFlashChip(struct FlashDev *dev)
{
	return dev->chip;
}

FlashReadMode getFlashMode(struct FlashDev *dev)
{
	return dev->rmode;
}

struct FlashDev *getFlashDev(uint32_t flash)
{
	struct FlashDev *dev = NULL;
	struct FlashDev *itor = NULL;

	list_for_each_entry(itor, &flashNodeHead, node) {
		if (itor->flash == flash) {
			dev = itor;
			break;
		}
	}

	if (dev == NULL)
		FD_ERROR("failed");

	return dev;
}

static int deleteFlashDev(struct FlashDev *dev)
{
	if (dev == NULL) {
		FD_ERROR("NULL flash device");
		return -1;
	}

	list_del(&dev->node);

	return 0;
}

static int addFlashDev(struct FlashDev *dev)
{
	if (dev == NULL) {
		FD_ERROR("NULL flash device");
		return -1;
	}

	list_add_tail(&dev->node, &flashNodeHead);

	return 0;
}

/**
  * @brief Initializes flash Device.
  * @note The flash device configuration is in the board_config g_flash_cfg.
  *       Device number is the g_flash_cfg vector sequency number.
  * @param flash: the flash device number, same as the g_flash_cfg vector
  *               sequency number
  * @retval HAL_Status: The status of driver
  */
HAL_Status HAL_Flash_Init(uintptr_t flash, FlashBoardCfg *cfg)
{
	HAL_Status ret = HAL_OK;
	struct FlashDrv *drv = NULL;
	struct FlashChip *chip = NULL;
	struct FlashDev *dev = NULL;

	if (!cfg) {
		FD_ERROR("cfg is NULL!");
		return HAL_ERROR;
	}

	dev = hal_malloc(sizeof(*dev));
	if (dev == NULL) {
		FD_ERROR("malloc dev failed");
		return HAL_ERROR;
	}
	memset(dev, 0, sizeof(*dev));

	drv = flashDriverCreate(flash, cfg);
	if (drv == NULL) {
		FD_ERROR("flashDriverCreate failed");
		goto failed;
	}

	chip = hal_malloc(sizeof(struct FlashChip));
	if (chip == NULL) {
		FD_ERROR("FlashChip Malloc failed");
		goto failed;
	}
	memset(chip, 0, sizeof(struct FlashChip));
	chip->controller = drv->platform_data;
	chip->mDriver = drv;
	chip->dev = dev;

	FlashChipEnum(chip, drv);
	/*TODO: get read mode from board, and init*/

	dev->chip = chip;
	dev->drv = drv;
	dev->flash = flash;
	dev->type = cfg->type;
	dev->rmode = cfg->mode;
	dev->wmode = FLASH_PAGEPROGRAM;
	/*
	 *disabled continuous read for unstable
	if (dev->rmode == FLASH_READ_QUAD_IO_MODE)
		dev->continuous_read = FLASH_CONTINUOUS_READ_SUPPORT;
	*/
	dev->usercnt = 0;
	INIT_LIST_HEAD(&dev->node);
	dev->lock = hal_mutex_create();
	if (!dev->lock) {
		FD_ERROR("mutex init failed: %d", ret);
		goto failed;
	}

	addFlashDev(dev);
	dev->drv->open(dev->chip);
#if (CONFIG_CHIP_ARCH_VER > 1)
	if (chip->cfg.mSize > 16 * 1024 * 1024) {
		FD_INFO("Enter 32 Bit Address Mode\n");
		dev->drv->ioctl(dev->chip, FLASH_ENABLE_32BIT_ADDR, 0);
		dev->chip->flash_ctrl->externAddr_on = 1;
	}
#endif
	if (dev->rmode & (FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE | FLASH_READ_QPI_MODE)) {
		dev->chip->switchReadMode(dev->chip, dev->rmode);

		HAL_Flash_WaitCompl(dev, 5000);//wait busy

		if (dev->rmode & FLASH_READ_QPI_MODE)
			dev->chip->enableQPIMode(dev->chip);
	}
	dev->drv->close(dev->chip);

	FD_INFO("mode: 0x%x, freq: %dHz, drv: %d", cfg->mode, cfg->clk, cfg->type);
	FD_INFO("jedec: 0x%x, suspend_support: %d", dev->chip->cfg.mJedec, dev->chip->cfg.mSuspendSupport);
#ifdef CONFIG_PM_FLASHC
	struct pm_device *flash_pm = hal_malloc(sizeof(*flash_pm));
	memset(flash_pm, 0, sizeof(*flash_pm));
	flash_pm->name = "Flash";
	flash_pm->ops = &flash_drv;
	flash_pm->data = (void *)flash;
	dev->pm = flash_pm;
	pm_devops_register(flash_pm);
#endif

	return HAL_OK;

failed:
	FD_ERROR("failed");

	if (drv != NULL)
		flashDriverDestory(chip);

	if (chip) {
		hal_free(chip);
	}
	if (dev != NULL)
		hal_free(dev);

	return HAL_ERROR;
}

/**
  * @brief Deinitializes flash Device.
  * @param flash: the flash device number, same as the g_flash_cfg vector
  *               sequency number
  * @retval HAL_Status: The status of driver
  */
HAL_Status HAL_Flash_Deinit(uint32_t flash)
{
	struct FlashDev *dev = getFlashDev(flash);
	struct FlashDrv *drv = dev->drv;
	struct FlashChip *chip = dev->chip;

	/*not thread safe*/
	if (dev->usercnt != 0)
		return HAL_TIMEOUT;

#ifdef CONFIG_PM_FLASHC
	pm_devops_unregister(dev->pm);
	hal_free(dev->pm);
#endif

	drv->open(chip);
	chip->reset(chip);
	drv->close(chip);

	deleteFlashDev(dev);
	hal_mutex_delete(dev->lock);

	if (dev->drv != NULL) {
		flashDriverDestory(dev->chip);
	}

	if (dev->chip) {
		hal_free(dev->chip);
	}

	if (dev != NULL) {
		hal_free(dev);
	}

	return HAL_OK;
}

HAL_Status HAL_Flash_Ioctl(uint32_t flash, FlashControlCmd attr, uintptr_t arg)
{
	HAL_Status ret = HAL_ERROR;
	struct FlashDev *dev = getFlashDev(flash);

	switch (attr) {
		/*TODO: 1.return min erase size */
	case FLASH_GET_MIN_ERASE_SIZE: {
		*((FlashEraseMode *)arg) = dev->chip->minEraseSize(dev->chip);
		ret = HAL_OK;
		break;
	}
	case FLASH_WRITE_STATUS: {
		FlashControlStatus *tmp = (FlashControlStatus *)arg;
		dev->drv->open(dev->chip);
		dev->chip->writeEnable(dev->chip);
		ret = dev->chip->writeStatus(dev->chip, tmp->status, tmp->data);
		HAL_Flash_WaitCompl(dev, 5000);
		dev->chip->writeDisable(dev->chip);
		dev->drv->close(dev->chip);
		break;
	}
	case FLASH_READ_STATUS: {
		FlashControlStatus *tmp = (FlashControlStatus *)arg;
		dev->drv->open(dev->chip);
		ret = dev->chip->readStatus(dev->chip, tmp->status, tmp->data);
		HAL_Flash_WaitCompl(dev, 5000);
		dev->drv->close(dev->chip);
		break;
	}
	case FLASH_SET_READ_MODE: {
		FlashReadMode old_rmode = dev->rmode;

		dev->rmode = (FlashReadMode)arg;
		if(!(dev->rmode & dev->chip->cfg.mReadSupport)) {
			FD_ERROR("not support read mode %d\n", dev->rmode);
			return HAL_INVALID;
		}

		if(old_rmode == FLASH_READ_QPI_MODE) {
			dev->drv->open(dev->chip);
			ret = dev->chip->disableQPIMode(dev->chip);
			if(ret != HAL_OK)
				return ret;
			HAL_Flash_WaitCompl(dev, 5000);//wait busy
			dev->drv->close(dev->chip);
		}

		dev->drv->open(dev->chip);
		ret = dev->chip->switchReadMode(dev->chip, dev->rmode);

		if(ret != HAL_OK)
			return ret;

		HAL_Flash_WaitCompl(dev, 5000);//wait busy

		if (dev->rmode & FLASH_READ_QPI_MODE)
			ret = dev->chip->enableQPIMode(dev->chip);

		dev->drv->close(dev->chip);
		break;
	}
	case FLASH_SET_PAGEPROGRAM_MODE: {
		dev->wmode = (FlashPageProgramMode)arg;
		if(!(dev->wmode & dev->chip->cfg.mPageProgramSupport)) {
			FD_ERROR("not support page program mode %d\n", dev->wmode);
			return HAL_INVALID;
		}
		ret = HAL_OK;
		break;
	}
	case FLASH_SET_SUSPEND_PARAM : {
		Flashc_Config *param = (Flashc_Config *)arg;

		dev->ispr_mask[0] = param->ispr_mask[0];
		dev->ispr_mask[1] = param->ispr_mask[1];
		dev->switch_out_ms = param->switch_out_ms;
		dev->check_busy_us = param->check_busy_us;
		dev->busy_timeout_us = param->busy_timeout_us;
		ret = HAL_OK;
		break;
	}
	/*TODO: tbc...*/
	default:
		return HAL_INVALID;
	}

	return ret;
}

HAL_Status HAL_Flash_InitLater(uint32_t flash, FlashBoardCfg *cfg)
{
	struct FlashDev *dev = getFlashDev(flash);

	if (!cfg) {
		FD_ERROR("cfg is NULL!");
		return HAL_ERROR;
	}

#if (defined(FLASH_XIP_OPT_ERASR))
	HAL_Flash_Ioctl(flash, FLASH_SET_SUSPEND_PARAM, (uint32_t)&cfg->flashc.param);
#endif

	if (!dev->chip->cfg.mSuspendSupport)
		cfg->flashc.param.optimize_mask &= ~(FLASH_OPTIMIZE_WRITE | FLASH_OPTIMIZE_ERASE);
	if (cfg->type == FLASH_DRV_FLASHC)
		dev->drv->ioctl(dev->chip, FLASH_SET_OPTIMIZE_MASK, cfg->flashc.param.optimize_mask);

	return HAL_OK;
}

/**
  * @brief Open flash Device.
  * @note Opened a flash device, other user can't open again, so please
  *       close it while don't need the flash device.
  * @param flash: the flash device number, same as the g_flash_cfg vector
  *               sequency number.
  * @param timeout_ms: timeout in millisecond.
  * @retval HAL_Status: The status of driver
  */
HAL_Status HAL_Flash_Open(uint32_t flash, uint32_t timeout_ms)
{
	struct FlashDev *dev = getFlashDev(flash);
	HAL_Status ret = HAL_ERROR;

	ret = hal_mutex_lock(dev->lock);
	if (ret == HAL_OK)
		dev->usercnt++;

	return ret;
}

/**
  * @brief Close flash Device.
  * @param flash: the flash device number, same as the g_flash_cfg vector
  *               sequency number
  * @retval HAL_Status: The status of driver
  */
HAL_Status HAL_Flash_Close(uint32_t flash)
{
	struct FlashDev *dev = getFlashDev(flash);
	HAL_Status ret = HAL_ERROR;

	ret = hal_mutex_unlock(dev->lock);
	if (ret == HAL_OK)
		dev->usercnt--;

	return ret;
}

#if (defined(FLASH_XIP_OPT_ERASR) || defined(FLASH_XIP_OPT_WRITE))
/**
  * @brief Get flash current status
  * @param flash: the flash device number, same as the g_flash_cfg vector
  *               sequency number
  * @param
  * @retval HAL_Status: The status of driver
  */
HAL_Status HAL_Flash_CurStatus(uint32_t flash)
{
	HAL_Status ret = HAL_OK;
	struct FlashDev *dev = getFlashDev(flash);

	if (dev->chip->flash_ctrl->xip_on) {
		if (dev->usercnt)
			ret = HAL_BUSY;
		else
			ret = HAL_OK;
	}

	return ret;
}
#endif

/**
  * @brief Write flash Device memory, no need to erase first and  other memory
  *        will not be change. Only can be used in the flash supported 4k erase.
  * @note Only the flash supported 4k erase!! FDCM module is much fast than
  *       this function.
  * @param flash: the flash device number, same as the g_flash_cfg vector
  *               sequency number
  * @param addr: the address of memory.
  * @param data: the data needed to write to flash device.
  * @param size: the data size needed to write.
  * @retval HAL_Status: The status of driver
  */
HAL_Status HAL_Flash_Overwrite(uint32_t flash, uint32_t addr, uint8_t *data, uint32_t size)
{
	struct FlashDev *dev = getFlashDev(flash);
	HAL_Status ret = HAL_ERROR;
	uint8_t *buf = NULL;
	uint8_t *ptr = data;
	uint32_t paddr = addr;
	int32_t  left = (int32_t)size;
	uint32_t pp_size;
	uint32_t saddr;

	FD_DEBUG("mEraseSizeSupport 0x%x", dev->chip->cfg.mEraseSizeSupport);
	if (!(dev->chip->cfg.mEraseSizeSupport & FLASH_ERASE_4KB))
		return HAL_INVALID;

	buf = hal_malloc(FLASH_ERASE_4KB);
	if (buf == NULL)
		goto out;

	while (left > 0) {
		ret = HAL_Flash_MemoryOf(flash, FLASH_ERASE_4KB, paddr, &saddr);
		if (ret != HAL_OK)
			goto out;
		ret = HAL_Flash_Read(flash, saddr, buf, FLASH_ERASE_4KB);
		if (ret != HAL_OK)
			goto out;
		ret = HAL_Flash_Erase(flash, FLASH_ERASE_4KB, saddr, 1);
		if (ret != HAL_OK)
			goto out;

		pp_size = MIN(left, FLASH_ERASE_4KB - (paddr - saddr));
		memcpy(buf + (paddr - saddr), ptr, pp_size);

		ret = HAL_Flash_Write(flash, saddr, buf, FLASH_ERASE_4KB);
		if (ret != HAL_OK)
			goto out;

		ptr += pp_size;
		paddr += pp_size;
		left -= pp_size;
	}

out:
	if (buf != NULL)
		hal_free(buf);

	return ret;
}

/**
  * @brief Write flash Device memory, if this memory has been written before,
  *        the memory must be erase first by user. HAL_Flash_Check can check
  *        this memory whether is writable.
  * @note If write a written memory, the memory data will a be error data.
  * @param flash: the flash device number, same as the g_flash_cfg vector
  *               sequency number
  * @param addr: the address of memory.
  * @param data: the data needed to write to flash device.
  * @param size: the data size needed to write.
  * @retval HAL_Status: The status of driver
  */
HAL_Status HAL_Flash_Write(uint32_t flash, uint32_t addr, const uint8_t *data, uint32_t size)
{
	struct FlashDev *dev = getFlashDev(flash);
	HAL_Status ret = HAL_ERROR;
	uint32_t address = addr;
	uint32_t left = size;
	const uint8_t *ptr;
	uint32_t pp_size;
	uint8_t flag = 0;
	uint32_t index = 0;
#ifdef FLASH_REWRITE_CHECK
	uint32_t rewrite_cnt = 0;
#endif

#define PageWritByte	256
	FD_DEBUG("%u: w%u, a: 0x%x", flash, size, addr);

	if ((NULL == dev) || (NULL == dev->chip->pageProgram) || (0 == size) || (addr + size > dev->chip->cfg.mSize)) {
		FD_ERROR("Invalid param");
		if (dev && dev->chip)
			FD_ERROR("pp:%p sz:%d ad:%x ms:%d", dev->chip->pageProgram, size, addr, dev->chip->cfg.mSize);
		return HAL_INVALID;
	}

#if (CONFIG_CHIP_ARCH_VER == 2)
	/* not support data in flash but not in xip, for data is not virtual address */
	if ((data >= (uint8_t *)FLASH_XIP_START_ADDR) && (data < (uint8_t *)FLASH_XIP_END_ADDR)) {
		flag = 1;
	}
#elif (CONFIG_CHIP_ARCH_VER == 3)
	if (((data >= (uint8_t *)PSRAM_START_ADDR) && (data < (uint8_t *)PSRAM_END_ADDR)) ||
	    ((data >= (uint8_t *)FLASH_XIP_START_ADDR) && (data < (uint8_t *)FLASH_XIP_END_ADDR))) {
		flag = 1;
	}
#endif

	if (flag) {
		ptr = (uint8_t *)hal_malloc(256);
		if (NULL == ptr) {
			FD_ERROR("Invalid param");
			return HAL_ERROR;
		}
	} else {
		ptr = data;
	}
	dev->drv->open(dev->chip);
	while (left > 0) {
		//pp_size = MIN(left, dev->chip->mPageSize - (address % dev->chip->mPageSize));
		pp_size = MIN(left, PageWritByte - (address % PageWritByte));

		if (flag)
			memcpy((char *)ptr, (char *)(data + index), pp_size);
#ifdef CONFIG_FLASH_LOW_POWER_PROTECT
		if (get_flash_stat()) {
			FD_ERROR("low power protect!\n");
			ret = HAL_ERROR;
			break;
		}
#endif
#ifdef CONFIG_FLASH_POWER_DOWN_PROTECT
		ret = HAL_Flash_Write_UnLock(dev, address, pp_size);
		if (ret != HAL_OK) {
			break;
		}
#endif
		//dev->drv->open(dev->chip);
		dev->chip->writeEnable(dev->chip);
		//FD_DEBUG("WE");
		ret = dev->chip->pageProgram(dev->chip, dev->wmode, address, ptr, pp_size);
		//FD_DEBUG("PP");
		//dev->chip->writeDisable(dev->chip); /* write disable itself after erase */
		//FD_DEBUG("WD");
		if (ret < 0) {
			//dev->drv->close(dev->chip);
			break;
		}
#ifdef FLASH_XIP_OPT_WRITE
		if (dev->chip->cfg.mSuspendSupport && !dev->chip->flash_ctrl->irqsaveflag) {
			ret = HAL_Flash_WaitCompl_XIP_Optim(dev, 5000);
		} else
#endif
		{
			ret = HAL_Flash_WaitCompl(dev, 5000);
		}
		//dev->drv->close(dev->chip);

		if (ret < 0) {
			FD_ERROR("wr failed: %d", ret);
		}
	#ifdef FLASH_REWRITE_CHECK		/*check write data success*/
		else {
			uint8_t *datatmp = (uint8_t *)hal_malloc(256);
			if (NULL == datatmp) {
				FD_ERROR("malloc faild");
				break;
			}

		#ifdef FLASH_XIP_OPT_READ
			if (dev->chip->flash_ctrl->xip_on) {
				ret = dev->chip->read(dev->chip, dev->rmode, address, datatmp, pp_size);
			} else
		#endif
			{
				//dev->drv->open(dev->chip);
				ret = dev->chip->read(dev->chip, dev->rmode, address, datatmp, pp_size);
				//dev->drv->close(dev->chip);
			}
			if (ret != 0)
				FD_ERROR("read failed");

			if (HAL_Memcmp(datatmp, ptr, pp_size) != 0) {
				FD_ERROR("check failed...\n");
				hal_free(datatmp);
				if (++rewrite_cnt < 3) {
					continue;
				} else {
					ret = HAL_ERROR;
					break;
				}
			} else {
				rewrite_cnt = 0;
				hal_free(datatmp);
			}
		}
	#endif
#ifdef CONFIG_FLASH_POWER_DOWN_PROTECT
		ret = HAL_Flash_Write_Lock(dev, address, pp_size);
		if (ret != HAL_OK) {
			break;
		}
#endif
		address += pp_size;
		index += pp_size;
		if (!flag)
			ptr += pp_size;
		left -= pp_size;
	}
	dev->drv->close(dev->chip);
	if (flag)
		hal_free((char *)ptr);

	if (ret != 0)
		FD_ERROR("wr failed");

	return ret;
}

/**
  * @brief Read flash device memory.
  * @param flash: the flash device number, same as the g_flash_cfg vector
  *               sequency number
  * @param addr: the address of memory.
  * @param data: the data needed to write to flash device.
  * @param size: the data size needed to read.
  * @retval HAL_Status: The status of driver
  * NOTE: should not call this when irq disabled for svc is used in trustzone.
  */
HAL_Status HAL_Flash_Read(uint32_t flash, uint32_t addr, uint8_t *data, uint32_t size)
{
	struct FlashDev *dev = getFlashDev(flash);
	HAL_Status ret;

	FD_DEBUG("%u: r%u, a: 0x%x", flash, size, addr);

	if ((NULL == dev) || (NULL == dev->chip->read) || (size == 0) || (addr + size > dev->chip->cfg.mSize)) {
		FD_ERROR("Invalid param");
		if (dev && dev->chip)
			FD_ERROR("rd:%p sz:%d ad:%x ms:%d", dev->chip->read, size, addr, dev->chip->cfg.mSize);
		return HAL_INVALID;
	}
#ifdef FLASH_XIP_OPT_READ
	if (dev->chip->flash_ctrl->xip_on && (dev->chip->flash_ctrl->optimize_mask & FLASH_OPTIMIZE_READ)) {
		if (!HAL_IsIRQDisabled())
			HAL_ThreadSuspendScheduler();
		ret = dev->chip->read(dev->chip, dev->rmode, addr, data, size);
		if (!HAL_IsIRQDisabled())
			HAL_ThreadResumeScheduler();
	} else
#endif
	{
		dev->drv->open(dev->chip);
		ret = dev->chip->read(dev->chip, dev->rmode, addr, data, size);
		dev->drv->close(dev->chip);
	}

	if (ret != 0)
		FD_ERROR("read failed");

	return ret;
}

/**
  * @brief Erase flash device memory. Flash can only erase sector or block or
  *        chip.
  * @note Some flash is not support some erase mode, for example: FLASH M25P64
  *       is only support FLASH_ERASE_CHIP and FLASH_ERASE_64KB.
  *       The erase address must be aligned to erase mode size, for example:
  *       the address should be n * 0x1000 in the erase 4kb mode, this address
  *       can be calculated in HAL_Flash_MemoryOf.
  * @param flash: the flash device number, same as the g_flash_cfg vector
  *               sequency number.
  * @param blk_size:
  *        @arg FLASH_ERASE_4KB: 4kbyte erase mode.
  *        @arg FLASH_ERASE_32KB: 32kbtye erase mode.
  *        @arg FLASH_ERASE_64KB: 64kbtye erase mode.
  *        @arg FLASH_ERASE_CHIP: erase whole flash chip.
  * @param addr: the address of memory.
  * @param blk_cnt: erase number of block or sector, no use in FLASH_ERASE_CHIP.
  * @retval HAL_Status: The status of driver
  */
HAL_Status HAL_Flash_Erase(uint32_t flash, FlashEraseMode blk_size, uint32_t addr, uint32_t blk_cnt)
{
	struct FlashDev *dev = getFlashDev(flash);
	HAL_Status ret = HAL_ERROR;
	uint32_t esize = blk_size;
	uint32_t eaddr = addr;
	uint32_t reErase_cnt = 0;

	FD_DEBUG("%u: e%u * %u, a: 0x%x\n", flash, (uint32_t)blk_size, blk_cnt, addr);

	if ((addr + blk_size * blk_cnt) > dev->chip->cfg.mSize) {
		FD_ERROR("memory is over flash memory\n");
		return HAL_INVALID;
	}
	if ((blk_size == FLASH_ERASE_CHIP) && (blk_cnt != 1)) {
		FD_ERROR("execute more than 1");
		return HAL_INVALID;
	}
	if (addr % blk_size) {
		FD_ERROR("on a incompatible address");
		return HAL_INVALID;
	}
	dev->drv->open(dev->chip);
	while (blk_cnt-- > 0) {
		//dev->drv->open(dev->chip);
#ifdef CONFIG_FLASH_LOW_POWER_PROTECT
		if (get_flash_stat()) {
			FD_ERROR("low power protect!\n");
			ret = HAL_ERROR;
			break;
		}
#endif
#ifdef CONFIG_FLASH_POWER_DOWN_PROTECT
		uint32_t size = SZ_4K;
		if (blk_size == FLASH_ERASE_32KB)
			size = SZ_32K;
		else if (blk_size == FLASH_ERASE_64KB)
			size = SZ_64K;
		else if (blk_size == FLASH_ERASE_4KB)
			size = SZ_4K;
		ret = HAL_Flash_Write_UnLock(dev, eaddr, size);
		if (ret != HAL_OK) {
			break;
		}
#endif
		dev->chip->writeEnable(dev->chip);
		ret = dev->chip->erase(dev->chip, blk_size, eaddr);
		//dev->chip->writeDisable(dev->chip); /* write disable itself after erase */
		if (ret < 0) {
			FD_ERROR("failed: %d", ret);
			//dev->drv->close(dev->chip);
			break;
		}

#if (defined(FLASH_XIP_OPT_ERASR))
		if (dev->chip->cfg.mSuspendSupport && !dev->chip->flash_ctrl->irqsaveflag) {
			ret = HAL_Flash_WaitCompl_XIP_Optim(dev, 5000);
		} else
#endif
		{
			ret = HAL_Flash_WaitCompl(dev, 5000);
		}
		//dev->drv->close(dev->chip);

		if (ret < 0) {
			if (++reErase_cnt < 3) {
				blk_cnt++;
				continue;
			}
		}
#ifdef FLASH_REERASE_CHECK		/*check earse data*/
		else {
			uint32_t index;
			uint8_t *datatmp = (uint8_t *)hal_malloc(256);
			if (NULL == datatmp) {
				FD_ERROR("malloc faild");
				break;
			}

#ifdef FLASH_XIP_OPT_READ
			if (dev->chip->flash_ctrl->xip_on) {
				ret = dev->chip->read(dev->chip, dev->rmode, eaddr, datatmp, 256);
			} else
#endif
			{
				//dev->drv->open(dev->chip);
				ret = dev->chip->read(dev->chip, dev->rmode, eaddr, datatmp, 256);
				//dev->drv->close(dev->chip);
			}
			if (ret != 0)
				FD_ERROR("read failed");

			for (index = 0; index < 256; index++) {
				if (datatmp[index] != 0xFF) {
					break;
				}
			}
			hal_free(datatmp);

			if (index < 256) {
				if (++reErase_cnt < 3) {
					FD_ERROR("repeat erase %d times...\n", reErase_cnt);
					blk_cnt++;
					continue;
				}
				ret = HAL_ERROR;
			}
		}
#endif
#ifdef CONFIG_FLASH_POWER_DOWN_PROTECT
		ret = HAL_Flash_Write_Lock(dev, eaddr, size);
		if (ret != HAL_OK) {
			break;
		}
#endif
		eaddr += esize;
	}
	dev->drv->close(dev->chip);
	return ret;
}

#ifdef CONFIG_DRIVERS_FLASHC_PANIC_TRANSFER
HAL_Status HAL_Flash_Panic_Write(uint32_t flash, uint32_t addr, const uint8_t *data, uint32_t size)
{
	struct FlashDev *dev = getFlashDev(flash);
	HAL_Status ret = HAL_ERROR;
	uint32_t address = addr;
	uint32_t left = size;
	const uint8_t *ptr;
	uint32_t pp_size;
	uint8_t flag = 0;
	uint32_t index = 0;
	uint8_t buff[256];

#define PageWritByte	256
	FD_DEBUG("%u: w%u, a: 0x%x", flash, size, addr);

	if ((NULL == dev) || (NULL == dev->chip->pageProgram) || (0 == size) || (addr + size > dev->chip->cfg.mSize)) {
		FD_ERROR("Invalid param");
		if (dev && dev->chip)
			FD_ERROR("pp:%p sz:%d ad:%x ms:%d", dev->chip->pageProgram, size, addr, dev->chip->cfg.mSize);
		return HAL_INVALID;
	}

#if (CONFIG_CHIP_ARCH_VER == 2)
	/* not support data in flash but not in xip, for data is not virtual address */
	if ((data >= (uint8_t *)FLASH_XIP_START_ADDR) && (data < (uint8_t *)FLASH_XIP_END_ADDR)) {
		flag = 1;
	}
#elif (CONFIG_CHIP_ARCH_VER == 3)
	if (((data >= (uint8_t *)PSRAM_START_ADDR) && (data < (uint8_t *)PSRAM_END_ADDR)) ||
	    ((data >= (uint8_t *)FLASH_XIP_START_ADDR) && (data < (uint8_t *)FLASH_XIP_END_ADDR))) {
		flag = 1;
	}
#endif

	if (flag) {
		ptr = buff;
		if (NULL == ptr) {
			FD_ERROR("Invalid param");
			return HAL_ERROR;
		}
	} else {
		ptr = data;
	}
	dev->drv->open(dev->chip);
	while (left > 0) {
		//pp_size = MIN(left, dev->chip->mPageSize - (address % dev->chip->mPageSize));
		pp_size = MIN(left, PageWritByte - (address % PageWritByte));

		if (flag)
			memcpy((char *)ptr, (char *)(data + index), pp_size);
#ifdef CONFIG_FLASH_LOW_POWER_PROTECT
		if (get_flash_stat()) {
			FD_ERROR("low power protect!\n");
			dev->drv->close(dev->chip);
			return HAL_ERROR;
		}
#endif
#ifdef CONFIG_FLASH_POWER_DOWN_PROTECT
		ret = HAL_Flash_Write_UnLock(dev, address, pp_size);
		if (ret != HAL_OK) {
			dev->drv->close(dev->chip);
			return ret;
		}
#endif
		//dev->drv->open(dev->chip);
		dev->chip->writeEnable(dev->chip);
		//FD_DEBUG("WE");
		ret = dev->chip->pageProgram(dev->chip, dev->wmode, address, ptr, pp_size);
		//FD_DEBUG("PP");
		//dev->chip->writeDisable(dev->chip); /* write disable itself after erase */
		//FD_DEBUG("WD");
		if (ret < 0) {
			//dev->drv->close(dev->chip);
			break;
		}

		ret = HAL_Flash_WaitCompl(dev, 5000);

		//dev->drv->close(dev->chip);

		if (ret < 0) {
			FD_ERROR("wr failed: %d", ret);
		}

#ifdef CONFIG_FLASH_POWER_DOWN_PROTECT
		ret = HAL_Flash_Write_Lock(dev, address, pp_size);
		if (ret != HAL_OK) {
			break;
		}
#endif
		address += pp_size;
		index += pp_size;
		if (!flag)
			ptr += pp_size;
		left -= pp_size;
	}
	dev->drv->close(dev->chip);
	if (flag)
		hal_free((char *)ptr);

	if (ret != 0)
		FD_ERROR("wr failed");

	return ret;
}

HAL_Status HAL_Flash_Panic_Erase(uint32_t flash, FlashEraseMode blk_size, uint32_t addr, uint32_t blk_cnt)
{
	struct FlashDev *dev = getFlashDev(flash);
	HAL_Status ret = HAL_ERROR;
	uint32_t esize = blk_size;
	uint32_t eaddr = addr;
	uint32_t reErase_cnt = 0;

	FD_DEBUG("%u: e%u * %u, a: 0x%x\n", flash, (uint32_t)blk_size, blk_cnt, addr);

	if ((addr + blk_size * blk_cnt) > dev->chip->cfg.mSize) {
		FD_ERROR("memory is over flash memory\n");
		return HAL_INVALID;
	}
	if ((blk_size == FLASH_ERASE_CHIP) && (blk_cnt != 1)) {
		FD_ERROR("execute more than 1");
		return HAL_INVALID;
	}
	if (addr % blk_size) {
		FD_ERROR("on a incompatible address");
		return HAL_INVALID;
	}
	dev->drv->open(dev->chip);
	while (blk_cnt-- > 0) {
		//dev->drv->open(dev->chip);
#ifdef CONFIG_FLASH_LOW_POWER_PROTECT
		if (get_flash_stat()) {
			FD_ERROR("low power protect!\n");
			dev->drv->close(dev->chip);
			return HAL_ERROR;
		}
#endif
#ifdef CONFIG_FLASH_POWER_DOWN_PROTECT
		uint32_t size = SZ_4K;
		if (blk_size == FLASH_ERASE_32KB)
			size = SZ_32K;
		else if (blk_size == FLASH_ERASE_64KB)
			size = SZ_64K;
		else if (blk_size == FLASH_ERASE_4KB)
			size = SZ_4K;
		ret = HAL_Flash_Write_UnLock(dev, eaddr, size);
		if (ret != HAL_OK) {
			dev->drv->close(dev->chip);
			return ret;
		}
#endif
		dev->chip->writeEnable(dev->chip);
		ret = dev->chip->erase(dev->chip, blk_size, eaddr);
		//dev->chip->writeDisable(dev->chip); /* write disable itself after erase */
		if (ret < 0) {
			FD_ERROR("failed: %d", ret);
			//dev->drv->close(dev->chip);
			break;
		}

		ret = HAL_Flash_WaitCompl(dev, 5000);

		//dev->drv->close(dev->chip);

		if (ret < 0) {
			if (++reErase_cnt < 3) {
				blk_cnt++;
				continue;
			}
		}
#ifdef CONFIG_FLASH_POWER_DOWN_PROTECT
		ret = HAL_Flash_Write_Lock(dev, eaddr, size);
		if (ret != HAL_OK) {
			break;
		}
#endif
		eaddr += esize;
	}
	dev->drv->close(dev->chip);
	return ret;
}
#endif

/**
  * @brief Calculate which block the flash address belong to, and output a
  *        block address to user.
  * @param flash: the flash device number, same as the g_flash_cfg vector
  *               sequency number.
  * @param blk_size:
  *        @arg FLASH_ERASE_4KB: 4kbyte erase mode.
  *        @arg FLASH_ERASE_32KB: 32kbtye erase mode.
  *        @arg FLASH_ERASE_64KB: 64kbtye erase mode.
  *        @arg FLASH_ERASE_CHIP: erase whole flash chip.
  * @param addr: the address of memory.
  * @param start: the address of the block contained the addr.
  * @retval HAL_Status: The status of driver
  */
HAL_Status HAL_Flash_MemoryOf(uint32_t flash, FlashEraseMode size, uint32_t addr, uint32_t *start)
{
	struct FlashDev *dev = getFlashDev(flash);
	uint32_t page_mask;
	HAL_Status ret = HAL_OK;

	if (!(size & dev->chip->cfg.mEraseSizeSupport))
		return HAL_INVALID;

	page_mask = ~((uint32_t)(size - 1));
	*start = addr & page_mask;

	return ret;
}

/**
  * @brief Check the flash memory whether .
  * @note The flash device configuration is in the board_config g_flash_cfg.
  *       Device number is the g_flash_cfg vector sequency number.
  * @param flash: the flash device number, same as the g_flash_cfg vector
  *               sequency number.
  * @param addr: the address of memory.
  * @param data: the data needed to write to flash device.
  * @param size: the data size needed to write.
  * @retval int: 0: same as data, no need to write or erase;
  *              1: write directly, no need to erase;
  *              2: need to erase first;
  */
int HAL_Flash_Check(uint32_t flash, uint32_t addr, uint8_t *data, uint32_t size)
{
#define FLASH_CHECK_BUF_SIZE (128)

	uint8_t *pdata = data;
	uint8_t *pbuf;
	uint8_t *buf;
	uint8_t src;
	uint8_t dst;
	uint32_t left = size;
	uint32_t paddr = addr;
	int32_t ret = 0;

	buf = hal_malloc(FLASH_CHECK_BUF_SIZE);
	if (buf == NULL)
		return -1;
	pbuf = buf + FLASH_CHECK_BUF_SIZE;

	while (left > 0) {
		if ((pbuf - buf) == FLASH_CHECK_BUF_SIZE) {
			HAL_Flash_Read(flash, paddr, buf, FLASH_CHECK_BUF_SIZE);
			pbuf = buf;
		}

		src = *pbuf++;
		dst = *pdata++;
		left--;
		paddr++;

		dst ^= src;
		if (dst == 0)
			continue; /* src == dst */

		ret = 1; /* src != dst */
		if (dst & src) {
			ret = 2; /* src has bit '1', need to erase */
			break;
		}
	}

	hal_free(buf);

	return ret;
}

#ifdef CONFIG_PM_FLASHC
static int PM_FlashSuspend(struct pm_device *dev, suspend_mode_t state)
{
	struct FlashDev *fdev;
	struct FlashDrv *drv;
	struct FlashChip *chip;

	FD_DEBUG("PM_FlashSuspend\n");
	switch (state) {
	case PM_MODE_SLEEP:
	case PM_MODE_STANDBY:
		//break;
	case PM_MODE_HIBERNATION:
		fdev = getFlashDev((uintptr_t)dev->data);
		drv = fdev->drv;
		chip = fdev->chip;
		if (fdev->usercnt != 0)
			return -1;
		drv->open(chip);
		/*chip->reset(chip);*/
		chip->control(chip, DEFAULT_FLASH_POWERDOWN, NULL);
		drv->close(chip);
		break;
	default:
		break;
	}

	return 0;
}

static int PM_FlashResume(struct pm_device *dev, suspend_mode_t state)
{
	struct FlashDev *fdev;
	struct FlashDrv *drv;
	struct FlashChip *chip;

	FD_DEBUG("PM_FlashResume\n");
	switch (state) {
	case PM_MODE_SLEEP:
	case PM_MODE_STANDBY:
		//break;
	case PM_MODE_HIBERNATION:
		/* exit power down mode in brom */
		fdev = getFlashDev((uintptr_t)dev->data);
		drv = fdev->drv;
		chip = fdev->chip;
		drv->open(chip);
		chip->releasePowerDown(chip);
		if (fdev->rmode & (FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE | FLASH_READ_QPI_MODE)) {
			chip->switchReadMode(chip, fdev->rmode);
			if (fdev->rmode & FLASH_READ_QPI_MODE)
				chip->enableQPIMode(chip);
		}
		drv->close(chip);
		break;
	default:
		break;
	}

	return 0;
}

static struct pm_devops flash_drv = {
	.suspend_noirq = PM_FlashSuspend,
	.resume_noirq = PM_FlashResume,
};
#endif /* CONFIG_PM */
