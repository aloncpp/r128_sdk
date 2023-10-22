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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flash_chip.h"
#include "../flash_debug.h"
#include "../porting.h"

#define P25Q05H_JEDEC 0x106085 /* not support for too small */
#define P25Q10H_JEDEC 0x116085 /* not support for too small */
#define P25Q20H_JEDEC 0x126085 /* not support for too small */
#define P25Q40H_JEDEC 0x136085
#define P25Q80H_JEDEC 0x146085
#define P25Q16H_JEDEC 0x156085
#define P25Q32H_JEDEC 0x166085
#define P25Q64H_JEDEC 0x176085

typedef enum {
	FLASH_INSTRUCTION_RDSR = 0x05,				/* read status register */
	FLASH_INSTRUCTION_RDSR2 = 0x35,				/* read status register-1 */
	FLASH_INSTRUCTION_RDCR = 0x15,				/* read configure register */
	FLASH_INSTRUCTION_WRSR = 0x01,				/* write status register, P25Q64H 1 byte, other 2 byte */
	FLASH_INSTRUCTION_WRSR2_AB15 = 0x31,        /* write status register-1  1 byte, only used when memory density > 15 */
	FLASH_INSTRUCTION_WRCR_AB15 = 0x11,         /* write configure register, only used when memory density > 15 */
	FLASH_INSTRUCTION_WRCR = 0x31,				/* write configure register */
} eSF_Instruction;

/* internal macros for flash chip instruction */
#define FCI_CMD(idx)    instruction[idx]
#define FCI_ADDR(idx)   instruction[idx]
#define FCI_DUMMY(idx)  instruction[idx]
#define FCI_DATA(idx)   instruction[idx]

static int P25QXXH_WriteStatus(struct FlashChip *chip, FlashStatus reg, uint8_t *status)
{
	int ret;
	uint8_t status_buf[2];
	InstructionField instruction[2];

	PCHECK(chip);

	if (!(reg & chip->cfg.mWriteStatusSupport)) {
		FLASH_NOTSUPPORT();
		return HAL_INVALID;
	}
	/*
		memset(&instruction, 0, sizeof(instruction));
		FCI_CMD(0).data = FLASH_INSTRUCTION_SRWREN;
		FCI_CMD(0).line = 1;
		chip->driverWrite(chip, &FCI_CMD(0), NULL, NULL, NULL);
	*/

	memset(&instruction, 0, sizeof(instruction));

	if (reg == FLASH_STATUS1) {
		if ((chip->cfg.mJedec & 0xFF0000) < 0x160000) {
			FCI_CMD(0).data = FLASH_INSTRUCTION_RDSR2;
			FCI_CMD(0).line = 1;

			FCI_DATA(1).pdata = (uint8_t *)&status_buf[1];
			FCI_DATA(1).len = 1;
			FCI_DATA(1).line = 1;

			chip->driverRead(chip, &FCI_CMD(0), NULL, NULL, &FCI_DATA(1));
		}

		status_buf[0] = *status;

		FCI_CMD(0).data = FLASH_INSTRUCTION_WRSR;

		FCI_DATA(1).pdata = status_buf;
		FCI_DATA(1).len = 2;
		FCI_DATA(1).line = 1;
	} else if (reg == FLASH_STATUS2) {
		if ((chip->cfg.mJedec & 0xFF0000) < 0x160000) {
			FCI_CMD(0).data = FLASH_INSTRUCTION_RDSR;
			FCI_CMD(0).line = 1;

			FCI_DATA(1).pdata = (uint8_t *)status_buf;
			FCI_DATA(1).len = 1;
			FCI_DATA(1).line = 1;

			chip->driverRead(chip, &FCI_CMD(0), NULL, NULL, &FCI_DATA(1));

			status_buf[1] = *status;

			FCI_CMD(0).data = FLASH_INSTRUCTION_WRSR;

			FCI_DATA(1).len = 2;
			FCI_DATA(1).line = 1;
		} else {
			status_buf[0] = *status;

			FCI_CMD(0).data = FLASH_INSTRUCTION_WRSR2_AB15;

			FCI_DATA(1).len = 1;
			FCI_DATA(1).line = 1;
		}
		FCI_DATA(1).pdata = status_buf;
	} else if (reg == FLASH_STATUS3) {
		if ((chip->cfg.mJedec & 0xFF0000) < 0x160000) {
			FCI_CMD(0).data = FLASH_INSTRUCTION_WRCR;
		} else {
			FCI_CMD(0).data = FLASH_INSTRUCTION_WRCR_AB15;
		}

		FCI_DATA(1).pdata = (uint8_t *)status;
		FCI_DATA(1).len = 1;
		FCI_DATA(1).line = 1;
	} else {
		FLASH_NOWAY();
	}

	chip->writeEnable(chip);

	ret = chip->driverWrite(chip, &FCI_CMD(0), NULL, NULL, &FCI_DATA(1));

	chip->writeDisable(chip);
	/*
		while (chip->isBusy(chip)) {
			//printf("busy...\n");
		}
	*/
	return ret;
}

static int P25QXXH_FlashInit(struct FlashChip *chip)
{
	PCHECK(chip);

	chip->writeEnable = defaultWriteEnable;
	chip->writeDisable = defaultWriteDisable;
	chip->readStatus = defaultReadStatus;
	chip->erase = defaultErase;
	chip->jedecID = defaultGetJedecID;
	chip->pageProgram = defaultPageProgram;
	chip->read = defaultRead;

	chip->driverWrite = defaultDriverWrite;
	chip->driverRead = defaultDriverRead;
	chip->xipDriverCfg = defaultXipDriverCfg;
	chip->setFreq = defaultSetFreq;
	chip->switchReadMode = defaultSwitchReadMode;
	chip->enableXIP = defaultEnableXIP;
	chip->disableXIP = defaultDisableXIP;
	chip->isBusy = defaultIsBusy;
	chip->control = defaultControl;
	chip->minEraseSize = defaultGetMinEraseSize;
	//chip->writeStatus = defaultWriteStatus;
	chip->writeStatus = P25QXXH_WriteStatus;
	chip->enableQPIMode = defaultEnableQPIMode;
	chip->disableQPIMode = defaultDisableQPIMode;
//	chip->enableReset = defaultEnableReset;
	chip->reset = defaultReset;

	chip->suspendErasePageprogram = defaultSuspendErasePageprogram;
	chip->resumeErasePageprogram = defaultResumeErasePageprogram;
	chip->isSuspend = defaultIsSuspend;
	chip->powerDown = NULL;
	chip->releasePowerDown = defaultReleasePowerDown;
	chip->uniqueID = NULL;
#ifdef CONFIG_FLASH_POWER_DOWN_PROTECT
	chip->initLock = defaultInitLock;
	chip->setGlobalBlockLockState = defaultSetGlobalBlockLockState;
	chip->lockBlock = defaultLockBlock;
	chip->unLockBlock = defaultUnLockBlock;
	chip->readBlockLockStatus = defaultReadBlockLockStatus;
#endif
	/*TODO: a NULL interface for showing invalid interface*/

	FLASH_DEBUG("P25QXXH_Flash inited");

	return 0;
}

static int P25QXXH_FlashDeinit(struct FlashChip *chip)
{
	PCHECK(chip);

//	HAL_Free(chip);

	return 0;
}

static const FlashChipCfg _P25QXXH_FlashChipCfg = {
	.mJedec = P25Q40H_JEDEC,
	.mSize = 16 * 8 * 4096,
	.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
	.mPageProgramSupport = FLASH_PAGEPROGRAM | FLASH_QUAD_PAGEPROGRAM,
	.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
	.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
	.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE |
	                FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
	.mMaxFreq = 104 * 1000 * 1000,
	.mMaxReadFreq = 55 * 1000 * 1000,
	.mSuspendSupport = 1,
	.mSuspend_Latency = 30,
	.mResume_Latency = 200,
};

static struct FlashChip *P25QXXH_FlashCtor(struct FlashChip *chip, uint32_t arg)
{
	uint32_t jedec = arg;
	uint32_t size;
	PCHECK(chip);

	if (jedec == P25Q64H_JEDEC) {
		size = 8 * 1024 * 1024;
	} else if (jedec == P25Q32H_JEDEC) {
		size = 4 * 1024 * 1024;
	} else if (jedec == P25Q16H_JEDEC) {
		size = 2 * 1024 * 1024;
	} else if (jedec == P25Q80H_JEDEC) {
		size = 1 * 1024 * 1024;
	} else if (jedec == P25Q40H_JEDEC) {
		size = 512 * 1024;
	} else {
		return NULL;
	}

	memcpy(&chip->cfg, &_P25QXXH_FlashChipCfg, sizeof(FlashChipCfg));
	chip->cfg.mJedec = jedec;
	chip->cfg.mSize = size;

	if (jedec == P25Q64H_JEDEC) {
		chip->cfg.mMaxFreq = 96 * 1000 * 1000; /* P25Q64H_JEDEC  2.3~3.6  96M */
		chip->cfg.mReadSupport = chip->cfg.mReadSupport | FLASH_READ_QPI_MODE; /* P25Q64H_JEDEC, qpi */
	}
	chip->mPageSize = 256;
	chip->mFlashStatus = 0;
	chip->mDummyCount = 1;

	return chip;
}

FlashChipCtor P25Q80H_FlashChip = {
	.mJedecId = P25Q80H_JEDEC,
	.enumerate = P25QXXH_FlashCtor,
	.init = P25QXXH_FlashInit,
	.destory = P25QXXH_FlashDeinit,
};

FlashChipCtor P25Q40H_FlashChip = {
	.mJedecId = P25Q40H_JEDEC,
	.enumerate = P25QXXH_FlashCtor,
	.init = P25QXXH_FlashInit,
	.destory = P25QXXH_FlashDeinit,
};

FlashChipCtor P25Q16H_FlashChip = {
	.mJedecId = P25Q16H_JEDEC,
	.enumerate = P25QXXH_FlashCtor,
	.init = P25QXXH_FlashInit,
	.destory = P25QXXH_FlashDeinit,
};

FlashChipCtor  P25Q32H_FlashChip = {
	.mJedecId = P25Q32H_JEDEC,
	.enumerate = P25QXXH_FlashCtor,
	.init = P25QXXH_FlashInit,
	.destory = P25QXXH_FlashDeinit,
};

FlashChipCtor  P25Q64H_FlashChip = {
	.mJedecId = P25Q64H_JEDEC,
	.enumerate = P25QXXH_FlashCtor,
	.init = P25QXXH_FlashInit,
	.destory = P25QXXH_FlashDeinit,
};
