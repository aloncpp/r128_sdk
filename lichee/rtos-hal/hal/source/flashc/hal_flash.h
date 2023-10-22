/**
 * @file  hal_flash.h
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

#ifndef _DRIVER_CHIP_HAL_FLASH_H_
#define _DRIVER_CHIP_HAL_FLASH_H_

#include "./flashchip/flash_chip.h"
#include "flash_debug.h"
#include "hal_flashctrl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FLASH_FLASHC_ENABLE (1)
#define FLASH_SPI_ENABLE (!defined(CONFIG_BOOTLOADER))

typedef enum FlashControlCmd {
	FLASH_GET_MIN_ERASE_SIZE,
	FLASH_WRITE_STATUS,
	FLASH_READ_STATUS,
	FLASH_ENABLE_32BIT_ADDR,
	FLASH_SET_READ_MODE,
	FLASH_SET_PAGEPROGRAM_MODE,
	FLASH_SET_OPTIMIZE_MASK,
	FLASH_SET_SUSPEND_PARAM,
	FLASH_SET_RESET_MASK,
	FLASH_WRITE_STATUS_VOLATILE,
	/*TODO: tbc...*/
} FlashControlCmd;

struct FlashDrv {
	/* attribute */
	int dev;
	uint32_t sizeToDma;

	/* public interface */
	HAL_Status (*write)(struct FlashChip *chip, InstructionField *cmd,
	                    InstructionField *addr, InstructionField *dummy,
	                    InstructionField *data);
	HAL_Status (*read)(struct FlashChip *chip, InstructionField *cmd,
	                   InstructionField *addr, InstructionField *dummy,
	                   InstructionField *data);
	HAL_Status (*open)(struct FlashChip *chip);
	HAL_Status (*close)(struct FlashChip *chip);
	HAL_Status (*setFreq)(struct FlashChip *chip, uint32_t freq);
	void (*msleep)(struct FlashChip *chip, uint32_t ms);
	void (*destroy)(struct FlashChip *);
	HAL_Status (*ioctl)(struct FlashChip *chip, FlashControlCmd attr, uintptr_t arg);
	void *platform_data;
};

/*
	Flash
*/
struct FlashDev {
	struct list_head node;
	hal_mutex_t lock;

	uint16_t flash;
	uint16_t type; /* flash control or spi control */
	struct FlashDrv *drv;
	struct FlashChip *chip;
	FlashReadMode rmode;
	FlashPageProgramMode wmode;
	FlashContinuousRead continuous_read;
	FlashBurstWrapSize wrap_size;
	uint8_t usercnt; /* not thread safe */
	uint32_t ispr_mask[FLASH_ISPR_MASK_NUM];
	uint32_t switch_out_ms; /* time to switch out for run other tasks */
	uint32_t check_busy_us; /* time wheel to exit check flash busy status for feed wdg */
	uint32_t busy_timeout_us; /* timeout to exit write or erase for switch out */

#ifdef CONFIG_PM_FLASHC
	struct pm_device*pm;
#endif
};

struct FlashDrv *FlashDriverCreator(int driver);
int FlashDriverDestory(struct FlashDrv *drv);

/*
	Flash Board Config
*/
enum FlashBoardType {
	FLASH_DRV_FLASHC = 0,   /*!< flash controller driver */
	FLASH_DRV_SPI = 1,      /*!< spi driver */
};

typedef struct FlashcBoardCfg {
	Flashc_Config param;
} FlashcBoardCfg;

//typedef struct SpiBoardCfg {
//	SPI_Port port;	/*!< spi port */
//	SPI_CS cs;      /*!< cs pin */
//	bool cs_level;	/*!< the cs voltage level of chip running */
//} SpiBoardCfg;

typedef struct FlashBoardCfg {
	enum FlashBoardType type;       /*!< control type, use flash or spi control */
	FlashReadMode mode;             /*!< read mode to flash */
	uint32_t clk;                   /*!< flash clock */
	union {
		FlashcBoardCfg flashc;	/*!< flash driver controller configuration. Notice!! flashc support all read mode */
//		SpiBoardCfg spi;        /*!< spi driver configuration. Notice!! spi only support normal read, fast read, dual output mode */
	};
} FlashBoardCfg;

typedef struct FlashControlStatus {
	FlashStatus status;
	uint8_t *data;
} FlashControlStatus;

/**
 * @brief Initializes flash Device.
 * @note The flash device configuration is in the board_config g_flash_cfg.
 *       Device number is the g_flash_cfg vector sequency number.
 * @param flash: the flash device number, same as the g_flash_cfg vector
 *               sequency number
 * @retval HAL_Status: The status of driver
 */
HAL_Status HAL_Flash_Init(uintptr_t flash, FlashBoardCfg *param);

/**
 * @brief Initializes flash Device Later after cache init.
 * @note The flash device configuration is in the board_config g_flash_cfg.
 *       Device number is the g_flash_cfg vector sequency number.
 * @param flash: the flash device number, same as the g_flash_cfg vector
 *               sequency number
 * @retval HAL_Status: The status of driver
 */
HAL_Status HAL_Flash_InitLater(uint32_t flash, FlashBoardCfg *cfg);

/**
 * @brief Deinitializes flash Device.
 * @param flash: the flash device number, same as the g_flash_cfg vector
 *               sequency number
 * @retval HAL_Status: The status of driver
 */
HAL_Status HAL_Flash_Deinit(uint32_t flash);

/**
 * @brief Open flash Device.
 * @note Opened a flash device, other user can't open again, so please
 *       close it while don't need the flash device.
 * @param flash: the flash device number, same as the g_flash_cfg vector
 *               sequency number.
 * @param timeout_ms: timeout in millisecond.
 * @retval HAL_Status: The status of driver
 */
HAL_Status HAL_Flash_Open(uint32_t flash, uint32_t timeout_ms);

/**
 * @brief Close flash Device.
 * @param flash: the flash device number, same as the g_flash_cfg vector
 *               sequency number
 * @retval HAL_Status: The status of driver
 */
HAL_Status HAL_Flash_Close(uint32_t flash);

/**
 * @brief Flash ioctl function.
 * @note attr : arg
 *       others are not support for now.
 * @param flash: the flash device number, same as the g_flash_cfg vector
 *               sequency number
 * @param attr: ioctl command
 * @param arg: ioctl arguement
 * @retval HAL_Status: The status of driver
 */
HAL_Status HAL_Flash_Ioctl(uint32_t flash, FlashControlCmd attr, uintptr_t arg);

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
HAL_Status HAL_Flash_Overwrite(uint32_t flash, uint32_t addr, uint8_t *data, uint32_t size);

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
HAL_Status HAL_Flash_Write(uint32_t flash, uint32_t addr, const uint8_t *data, uint32_t size);

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
HAL_Status HAL_Flash_Read(uint32_t flash, uint32_t addr, uint8_t *data, uint32_t size);

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
HAL_Status HAL_Flash_Erase(uint32_t flash, FlashEraseMode blk_size, uint32_t addr, uint32_t blk_cnt);

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
HAL_Status HAL_Flash_MemoryOf(uint32_t flash, FlashEraseMode size, uint32_t addr, uint32_t *start);

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
int HAL_Flash_Check(uint32_t flash, uint32_t addr, uint8_t *data, uint32_t size);

void HAL_Flash_SetDbgMask(uint8_t dbg_mask);

struct FlashDev;

struct FlashDev *getFlashDev(uint32_t flash);

struct FlashChip *getFlashChip(struct FlashDev *dev);

HAL_Status HAL_Flash_WaitCompl(struct FlashDev *dev, int32_t timeout_ms);
#ifdef CONFIG_FLASH_POWER_DOWN_PROTECT
HAL_Status HAL_Flash_Write_InitLock(struct FlashDev *dev);
HAL_Status HAL_Flash_Write_LockAll(struct FlashDev *dev, uint32_t addr, uint32_t size);
HAL_Status HAL_Flash_Write_UnLock(struct FlashDev *dev, uint32_t addr, uint32_t size);
#endif
/**
 * @internal
 * @brief Create a flash driver.
 * @param dev: Flash device number, but not minor number.
 * @param bcfg: Config from board config.
 * @retval HAL_Status: The status of driver
 */
struct FlashDrv *flashcDriverCreate(int dev, FlashBoardCfg *bcfg);

#if FLASH_SPI_ENABLE
struct FlashDrv *spiDriverCreate(int dev, FlashBoardCfg *bcfg);
#endif

#if (CONFIG_CHIP_ARCH_VER == 3) && (FLASH_CRYPTO_DISABLE_FUNC_EN)
static __inline HAL_Status HAL_Flash_Overwrite_Crypto(uint32_t flash, uint32_t addr, uint8_t *data, uint32_t size, uint8_t *key)
{
	HAL_Status ret;

	FC_Crypto_Enable(key);
	ret = HAL_Flash_Overwrite(flash, addr, data, size);
	FC_Clr_Crypto_Infor();
#ifdef CONFIG_TRUSTZONE
	FlashCryptoRelease(0);
#else
	/* to do */
#endif

	return ret;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_CHIP_HAL_FLASH_H_ */
