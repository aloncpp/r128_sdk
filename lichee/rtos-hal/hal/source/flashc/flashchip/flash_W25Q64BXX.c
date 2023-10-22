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
#include "../hal_flash.h"
#include "../hal_xip.h"
#include "../flash_debug.h"

#define W25Q64BXX_JEDEC 0x1740ef

typedef enum {
	FLASH_INSTRUCTION_RDSR1 = 0x05, 			/* read status register-1 */
	FLASH_INSTRUCTION_WRSR = 0X01,				/* write status register */
	FLASH_INSTRUCTION_RDSR2 = 0x35,
	FLASH_INSTRUCTION_READ = 0x03,				/* read data */
	FLASH_INSTRUCTION_FAST_READ = 0x0B, 		/* fast read */
	FLASH_INSTRUCTION_FAST_READ_DO = 0x3B,		/* fast read dual output */
	FLASH_INSTRUCTION_FAST_READ_DIO = 0xBB,
	FLASH_INSTRUCTION_FAST_READ_QO = 0x6B,
	FLASH_INSTRUCTION_FAST_READ_QIO = 0xEB,
	FLASH_INSTRUCTION_EN_QPI = 0x38,
	FLASH_INSTRUCTION_DIS_QPI = 0xFF,
} eSF_Instruction;

/* internal macros for flash chip instruction */
#define FCI_CMD(idx)    instruction[idx]
#define FCI_ADDR(idx)   instruction[idx]
#define FCI_DUMMY(idx)  instruction[idx]
#define FCI_DATA(idx)   instruction[idx]

static int W25Q64BXX_SwitchReadMode(struct FlashChip *chip, FlashReadMode mode)
{
	int ret;
	uint8_t status_buf[2];
	InstructionField instruction[2];

	PCHECK(chip);
	if (!(mode & chip->cfg.mReadSupport)) {
		FLASH_NOTSUPPORT();
		return HAL_INVALID;
	}

	if (!((chip->cfg.mReadStausSupport & FLASH_STATUS2) && (chip->cfg.mWriteStatusSupport & FLASH_STATUS2))) {
		//do not need switch
		return 0;
	}

	memset(&instruction, 0, sizeof(instruction));
	if (mode == FLASH_READ_QUAD_O_MODE || mode == FLASH_READ_QUAD_IO_MODE || mode == FLASH_READ_QPI_MODE) {

		FCI_CMD(0).data = FLASH_INSTRUCTION_RDSR1;
		FCI_CMD(0).line = 1;

		FCI_DATA(1).pdata = (uint8_t *)status_buf;
		FCI_DATA(1).len = 1;
		FCI_DATA(1).line = 1;
		chip->driverRead(chip, &FCI_CMD(0), NULL, NULL, &FCI_DATA(1));

		FCI_CMD(0).data = FLASH_INSTRUCTION_RDSR2;
		FCI_CMD(0).line = 1;

		FCI_DATA(1).pdata = (uint8_t *)&status_buf[1];
		FCI_DATA(1).len = 1;
		FCI_DATA(1).line = 1;
		chip->driverRead(chip, &FCI_CMD(0), NULL, NULL, &FCI_DATA(1));
		printf("state:0x%x\n", status_buf[1]);
		status_buf[1] |= 1 << 1;

		FCI_CMD(0).data = FLASH_INSTRUCTION_WRSR;

		FCI_DATA(1).len = 2;
		FCI_DATA(1).line = 1;

		FCI_DATA(1).pdata = status_buf;

	} else {

		FCI_CMD(0).data = FLASH_INSTRUCTION_RDSR1;
		FCI_CMD(0).line = 1;

		FCI_DATA(1).pdata = (uint8_t *)status_buf;
		FCI_DATA(1).len = 1;
		FCI_DATA(1).line = 1;
		chip->driverRead(chip, &FCI_CMD(0), NULL, NULL, &FCI_DATA(1));

		FCI_CMD(0).data = FLASH_INSTRUCTION_RDSR2;
		FCI_CMD(0).line = 1;

		FCI_DATA(1).pdata = (uint8_t *)&status_buf[1];
		FCI_DATA(1).len = 1;
		FCI_DATA(1).line = 1;
		chip->driverRead(chip, &FCI_CMD(0), NULL, NULL, &FCI_DATA(1));

		status_buf[1] &= ~(1 << 1);

		FCI_CMD(0).data = FLASH_INSTRUCTION_WRSR;

		FCI_DATA(1).len = 2;
		FCI_DATA(1).line = 1;

		FCI_DATA(1).pdata = status_buf;
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

int W25Q64BXX_XipDriverCfg(struct FlashChip *chip, FlashReadMode mode, FlashContinuousRead con_read)
{
	PCHECK(chip);
	InstructionField instruction[4];
	uint32_t continueMode = 0;	/* flashc exit continue mode, needed a read with dummy */

	if (chip->mXip == NULL)
		return -1;

	memset(&instruction, 0, sizeof(instruction));

	if (chip->cfg.mContinuousReadSupport & FLASH_CONTINUOUS_READ_SUPPORT) {
		if (con_read)
			continueMode = 1;
	}

	FCI_CMD(0).len = 1;
	FCI_CMD(0).line = 1;	//not in QPI
	FCI_ADDR(1).len = 3;
	switch (mode) {
	/* !!! NOTICE: m7~m0 is count to dummy byte. !!! */
	case FLASH_READ_NORMAL_MODE:
		FCI_CMD(0).data = FLASH_INSTRUCTION_READ;
		FCI_ADDR(1).line = 1;
		FCI_DATA(3).line = 1;
		FCI_DUMMY(2).len = 0;
		FCI_DUMMY(2).line = 1;
		continueMode = 0;
		break;
	case FLASH_READ_FAST_MODE:
		FCI_CMD(0).data = FLASH_INSTRUCTION_FAST_READ;
		FCI_ADDR(1).line = 1;
		FCI_DATA(3).line = 1;
		FCI_DUMMY(2).len = 1;
		FCI_DUMMY(2).line = 1;
		continueMode = 0;
		break;
	case FLASH_READ_DUAL_O_MODE:
		FCI_CMD(0).data = FLASH_INSTRUCTION_FAST_READ_DO;
		FCI_ADDR(1).line = 1;
		FCI_DATA(3).line = 2;
		FCI_DUMMY(2).len = 1;
		FCI_DUMMY(2).line = 1;
		continueMode = 0;
		break;
	case FLASH_READ_DUAL_IO_MODE:
		FCI_CMD(0).data = FLASH_INSTRUCTION_FAST_READ_DIO;
		FCI_ADDR(1).line = 2;
		FCI_DATA(3).line = 2;
		FCI_DUMMY(2).len = 1;
		FCI_DUMMY(2).line = 2;
		break;
	case FLASH_READ_QUAD_O_MODE:
		FCI_CMD(0).data = FLASH_INSTRUCTION_FAST_READ_QO;
		FCI_ADDR(1).line = 1;
		FCI_DATA(3).line = 4;
		FCI_DUMMY(2).len = 1;
		FCI_DUMMY(2).line = 1;
		continueMode = 0;
		break;
	case FLASH_READ_QUAD_IO_MODE:
		FCI_CMD(0).data = FLASH_INSTRUCTION_FAST_READ_QIO;
		FCI_ADDR(1).line = 4;
		FCI_DATA(3).line = 4;
		FCI_DUMMY(2).len = 3;
		FCI_DUMMY(2).line = 4;
		break;
	case FLASH_READ_QPI_MODE:
		FCI_CMD(0).data = FLASH_INSTRUCTION_FAST_READ_QIO;
		FCI_CMD(0).line = 4;
		FCI_DATA(3).line = 4;
		FCI_DUMMY(2).len = chip->mDummyCount;
		FCI_DUMMY(2).line = 4;
		FCI_ADDR(1).line = 4;
		break;
	default:
		return -1;
	}

	HAL_Xip_setDummyData(chip->mXip, 0xA5000000, 0, 0, 0);
	HAL_Xip_setCmd(chip->mXip, &FCI_CMD(0), &FCI_ADDR(1), &FCI_DUMMY(2), &FCI_DATA(3));
	HAL_Xip_setContinue(chip->mXip, continueMode, NULL);
	/*TODO: xip set delay*/

	return 0;
}

static int W25Q64BXX_FlashInit(struct FlashChip *chip)
{
	PCHECK(chip);
	chip->setBurstWrap = defaultSetBurstWrap;
	chip->continuousReadReset = defaultContinuousReadReset;
	chip->writeEnable = defaultWriteEnable;
	chip->writeDisable = defaultWriteDisable;
	chip->readStatus = defaultReadStatus;
	chip->erase = defaultErase;
	chip->jedecID = defaultGetJedecID;
	chip->pageProgram = defaultPageProgram;
	chip->read = defaultRead;

	chip->driverWrite = defaultDriverWrite;
	chip->driverRead = defaultDriverRead;
	chip->setFreq = defaultSetFreq;
	chip->switchReadMode = W25Q64BXX_SwitchReadMode;
	chip->xipDriverCfg = defaultXipDriverCfg;
	chip->enableXIP = defaultEnableXIP;
	chip->disableXIP = defaultDisableXIP;
	chip->isBusy = defaultIsBusy;
	chip->control = defaultControl;
	chip->minEraseSize = defaultGetMinEraseSize;
	chip->writeStatus = defaultWriteStatus;
	chip->enableQPIMode = defaultEnableQPIMode;
	chip->disableQPIMode = defaultDisableQPIMode;
	//chip->enableReset = defaultEnableReset;
	chip->reset = defaultReset;

	chip->suspendErasePageprogram = defaultSuspendErasePageprogram;
	chip->resumeErasePageprogram = defaultResumeErasePageprogram;
	chip->isSuspend = defaultIsSuspend;
	chip->powerDown = NULL;
	chip->releasePowerDown = defaultReleasePowerDown;
	chip->uniqueID = NULL;
	/*TODO: a NULL interface for showing invalid interface*/

	FLASH_DEBUG("W25Q64BXX_Flash inited");

	return 0;
}

static int W25Q64BXX_FlashDeinit(struct FlashChip *chip)
{
	PCHECK(chip);

//	HAL_Free(chip);

	return 0;
}

static const FlashChipCfg _W25Q64BXX_FlashChipCfg = {
	.mJedec = 0x1740ef,
	.mSize = 8*1024*1024,
	.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
	.mPageProgramSupport = FLASH_PAGEPROGRAM,
	.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
	.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
	.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
			| FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
	.mContinuousReadSupport = FLASH_CONTINUOUS_READ_SUPPORT,
	.mMaxFreq = 100 * 1000 * 1000,
	.mMaxReadFreq = 100 * 1000 * 1000,
	.mSuspendSupport = 0,
	.mSuspend_Latency = 0,
	.mResume_Latency = 0,
};

static struct FlashChip *W25Q64BXX_FlashCtor(struct FlashChip *chip, uint32_t arg)
{
	uint32_t jedec = arg;
	PCHECK(chip);

	if (jedec != W25Q64BXX_JEDEC) {
		return NULL;
	}

	memcpy(&chip->cfg, &_W25Q64BXX_FlashChipCfg, sizeof(FlashChipCfg));

	chip->mPageSize = 256;
	chip->mFlashStatus = 0;
	chip->mDummyCount = 1;

	return chip;
}

FlashChipCtor W25Q64BXX_FlashChip = {
	.mJedecId = W25Q64BXX_JEDEC,
	.enumerate = W25Q64BXX_FlashCtor,
	.init = W25Q64BXX_FlashInit,
	.destory = W25Q64BXX_FlashDeinit,
};
