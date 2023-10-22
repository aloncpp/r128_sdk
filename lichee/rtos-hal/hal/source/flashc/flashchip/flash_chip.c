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

typedef enum {
	FLASH_INSTRUCTION_WREN = 0x06,				/* write enable */
	FLASH_INSTRUCTION_WRDI = 0x04,				/* write disable */
	FLASH_INSTRUCTION_RDID = 0x9F,				/* jedec id */
	FLASH_INSTRUCTION_RDSR1 = 0x05, 			/* read status register-1 */
	FLASH_INSTRUCTION_WRSR1 = 0x01, 			/* write status register-1 */
	FLASH_INSTRUCTION_READ = 0x03,				/* read data */
	FLASH_INSTRUCTION_FAST_READ = 0x0B, 		/* fast read */
	FLASH_INSTRUCTION_PP = 0x02,					/* page program */
	FLASH_INSTRUCTION_ERASE_64KB = 0xD8,			/* erase block(sector) 64k */
	FLASH_INSTRUCTION_ERASE_32KB = 0x52,			/* erase block(sector) 32k */
	FLASH_INSTRUCTION_ERASE_4KB = 0x20, 		/* erase sector 4k */
	FLASH_INSTRUCTION_ERASE_CHIP = 0xC7,			/* chip erase */
	FLASH_INSTRUCTION_WRSR = 0X01,				/* write status register */
	FLASH_INSTRUCTION_FAST_READ_DO = 0x3B,		/* fast read dual output */
	FLASH_INSTRUCTION_RDSR2 = 0x35,
	FLASH_INSTRUCTION_RDSR3 = 0x15,
	FLASH_INSTRUCTION_WRSR2 = 0x31,
	FLASH_INSTRUCTION_WRSR3 = 0x11,
	FLASH_INSTRUCTION_SRWREN = 0x50,
	FLASH_INSTRUCTION_CE = 0x60,
	FLASH_INSTRUCTION_EPSP = 0x75,
	FLASH_INSTRUCTION_BURST_WRAP = 0x77,
	FLASH_INSTRUCTION_EPRS = 0x7A,
	FLASH_INSTRUCTION_PWDN = 0xB9,
	FLASH_INSTRUCTION_REL = 0xAB,
	FLASH_INSTRUCTION_FAST_READ_DIO = 0xBB,
	FLASH_INSTRUCTION_FAST_READ_QO = 0x6B,
	FLASH_INSTRUCTION_FAST_READ_QIO = 0xEB,
	FLASH_INSTRUCTION_EN_QPI = 0x38,
	FLASH_INSTRUCTION_DIS_QPI = 0xFF,
	FLASH_INSTRUCTION_RSEN = 0x66,
	FLASH_INSTRUCTION_RESET = 0x99,
	FLASH_INSTRUCTION_QPP = 0x32,
	FLASH_INSTRUCTION_SRP = 0xC0,
	FLASH_INSTRUCTION_BLOCK_LOCK = 0x36,
	FLASH_INSTRUCTION_BLOCK_UNLOCK = 0x39,
	FLASH_INSTRUCTION_ALL_BLOCK_LOCK = 0x7E,
	FLASH_INSTRUCTION_ALL_BLOCK_UNLOCK = 0x98,
	FLASH_INSTRUCTION_RD_BLOCKLOCK = 0x3D,
} eSF_Instruction;

/* internal macros for flash chip instruction */
#define FCI_CMD(idx)    instruction[idx]
#define FCI_ADDR(idx)   instruction[idx]
#define FCI_DUMMY(idx)  instruction[idx]
#define FCI_DATA(idx)   instruction[idx]

#ifdef FLASH_DEFAULTCHIP
extern FlashChipCtor DefaultFlashChip;
#endif
#ifdef FLASH_XT25F16B
extern FlashChipCtor  XT25F16B_FlashChip;
#endif
#ifdef FLASH_XT25F32B
extern FlashChipCtor  XT25F32B_FlashChip;
#endif
#ifdef FLASH_XT25F64B
extern FlashChipCtor  XT25F64B_FlashChip;
#endif
#ifdef FLASH_XT25F128B
extern FlashChipCtor  XT25F128B_FlashChip;
#endif
#ifdef FLASH_P25Q80H
extern FlashChipCtor  P25Q80H_FlashChip;
#endif
#ifdef FLASH_P25Q40H
extern FlashChipCtor  P25Q40H_FlashChip;
#endif
#ifdef FLASH_P25Q16H
extern FlashChipCtor  P25Q16H_FlashChip;
#endif
#ifdef FLASH_P25Q32H
extern FlashChipCtor  P25Q32H_FlashChip;
#endif
#ifdef FLASH_P25Q64H
extern FlashChipCtor  P25Q64H_FlashChip;
#endif
#ifdef FLASH_EN25QH64A
extern FlashChipCtor  EN25QH64A_FlashChip;
#endif
#ifdef FLASH_EN25QX128A
extern FlashChipCtor  EN25QX128A_FlashChip;
#endif
#ifdef FLASH_XM25QH64A
extern FlashChipCtor  XM25QH64A_FlashChip;
#endif
#ifdef FLASH_XM25QH128C
extern FlashChipCtor  XM25QH128C_FlashChip;
#endif
#ifdef FLASH_W25Q64BXX
extern FlashChipCtor  W25Q64BXX_FlashChip;
#endif
#ifdef FLASH_MX25L12835F
extern FlashChipCtor  MX25LXXXF_FlashChip;
#endif
#ifdef FLASH_MX25L6433F
extern FlashChipCtor  MX25L6433F_FlashChip;
#endif
#ifdef FLASH_MX25L25645G
extern FlashChipCtor  MX25L25645G_FlashChip;
#endif
FlashChipCtor *flashChipList[] = {
#ifdef FLASH_DEFAULTCHIP
	&DefaultFlashChip, /*default chip must be at the first*/
#endif
#ifdef FLASH_XT25F16B
	&XT25F16B_FlashChip,
#endif
#ifdef FLASH_XT25F32B
	&XT25F32B_FlashChip,
#endif
#ifdef FLASH_XT25F64B
	&XT25F64B_FlashChip,
#endif
#ifdef FLASH_P25Q80H
	&P25Q80H_FlashChip,
#endif
#ifdef FLASH_XT25F128B
	&XT25F128B_FlashChip,
#endif
#ifdef FLASH_P25Q40H
	&P25Q40H_FlashChip,
#endif
#ifdef FLASH_P25Q16H
	&P25Q16H_FlashChip,
#endif
#ifdef FLASH_P25Q32H
	&P25Q32H_FlashChip,
#endif
#ifdef FLASH_P25Q64H
	&P25Q64H_FlashChip,
#endif
#ifdef FLASH_EN25QH64A
	&EN25QH64A_FlashChip,
#endif
#ifdef FLASH_EN25QX128A
	&EN25QX128A_FlashChip,
#endif
#ifdef FLASH_XM25QH64A
	&XM25QH64A_FlashChip,
#endif
#ifdef FLASH_XM25QH128C
	&XM25QH128C_FlashChip,
#endif
#ifdef FLASH_W25Q64BXX
	&W25Q64BXX_FlashChip,
#endif
#ifdef FLASH_MX25L12835F
	&MX25LXXXF_FlashChip,
#endif

#ifdef FLASH_MX25L6433F
	&MX25L6433F_FlashChip,
#endif
#ifdef FLASH_MX25L25645G
	&MX25L25645G_FlashChip,
#endif
};

const FlashChipCtor **FlashChipGetChipList(int *len)
{
	*len = HAL_ARRAY_SIZE(flashChipList);
	return (const FlashChipCtor **)flashChipList;
}

static uint32_t getJedecID(struct FlashChip *chip, struct FlashDrv *drv)
{
	int ret;
	PCHECK(drv);
	InstructionField instruction[2];

	memset(&instruction, 0, sizeof(instruction));
	FCI_CMD(0).data = FLASH_INSTRUCTION_RDID;
	FCI_CMD(0).len = 1;
	FCI_CMD(0).line = 1;
	FCI_DATA(1).pdata = (uint8_t *)&FCI_DATA(1).data;
	FCI_DATA(1).line = 1;
	FCI_DATA(1).len = 3;

	drv->open(chip);
	ret = drv->read(chip, &FCI_CMD(0), NULL, NULL, &FCI_DATA(1));
	if (ret != HAL_OK)
		FLASH_ERROR("driver err: %d", ret);
	drv->close(chip);

	return FCI_DATA(1).data;
}

int FlashChipEnum(struct FlashChip *chip, struct FlashDrv *drv)
{
	int32_t list_len;
	uint32_t jedec;
	const FlashChipCtor *ctor;
	const FlashChipCtor **chiplist;

	jedec = getJedecID(chip, drv);
	chiplist = FlashChipGetChipList(&list_len);

	FLASH_DEBUG("jedec: 0x%x, list len %d", jedec, list_len);

	if (list_len <= 0) {
		return -1;
	}

	while (list_len > 0) {
		ctor = chiplist[--list_len];
		FLASH_DEBUG("check %x = %x ?\n", ctor->mJedecId, jedec);
		if (ctor->mJedecId == jedec)
			break;
	}

	ctor->enumerate(chip, jedec);
	ctor->init(chip);

	return 0;
}


/*
	Default Flash Chip Interface
*/
typedef struct {
	uint8_t SRP0: 1;
	uint8_t SEC: 1;
	uint8_t TB: 1;
	uint8_t BP2: 1;
	uint8_t BP1: 1;
	uint8_t BP0: 1;
	uint8_t WEL: 1;
	uint8_t BUSY: 1;
} DefaultFlash_StatusRegister1;	//this can be different in different flash chip.

typedef struct {
	union {
		struct {
			uint8_t SRP1: 1;
			uint8_t QE: 1;
			uint8_t RESERVED: 1;
			uint8_t LB1: 1;
			uint8_t LB2: 1;
			uint8_t LB3: 1;
			uint8_t CMP: 1;
			uint8_t SUS: 1;
		};
		uint8_t status;
	};
} DefaultFlash_StatusRegister2;	//this can be different in different flash chip.

int defaultSetBurstWrap(struct FlashChip *chip, FlashBurstWrapSize size)
{
	PCHECK(chip);
	InstructionField instruction[3];
	uint8_t set_mode;

	memset(&instruction, 0, sizeof(instruction));

	if (size == FLASH_BURST_WRAP_8B) {
		set_mode = 0x00;
	} else if (size == FLASH_BURST_WRAP_16B) {
		set_mode = 0x20;
	} else if (size == FLASH_BURST_WRAP_32B) {
		set_mode = 0x40;
	} else if (size == FLASH_BURST_WRAP_64B) {
		set_mode = 0x60;
	}else if (size == FLASH_BURST_WRAP_NOSUPPORT) {
		set_mode = 0x10;
	}

	FCI_CMD(0).data = FLASH_INSTRUCTION_BURST_WRAP;
	FCI_DUMMY(1).len = 3;
	FCI_DUMMY(1).line = 4;

	FCI_DATA(2).pdata = &set_mode;
	FCI_DATA(2).len = 1;
	FCI_DATA(2).line = 4;

	return chip->driverWrite(chip, &FCI_CMD(0), NULL, &FCI_DUMMY(1), &FCI_DATA(2));
}

void defaultContinuousReadReset(struct FlashChip *chip)
{
	PCHECK(chip);
	InstructionField cmd;

	memset(&cmd, 0, sizeof(cmd));

	cmd.data = 0xFF;
	cmd.line = 1;

	chip->driverWrite(chip, &cmd, NULL, NULL, NULL);
}

void defaultWriteEnable(struct FlashChip *chip)
{
	PCHECK(chip);
	InstructionField cmd;

	memset(&cmd, 0, sizeof(cmd));

	cmd.data = FLASH_INSTRUCTION_WREN;
	cmd.line = 1;

	chip->driverWrite(chip, &cmd, NULL, NULL, NULL);
}

void defaultWriteDisable(struct FlashChip *chip)
{
	PCHECK(chip);
	InstructionField cmd;

	memset(&cmd, 0, sizeof(cmd));

	cmd.data = FLASH_INSTRUCTION_WRDI;
	cmd.line = 1;

	chip->driverWrite(chip, &cmd, NULL, NULL, NULL);
}

int defaultReadStatus(struct FlashChip *chip, FlashStatus reg, uint8_t *status)
{
	PCHECK(chip);
	InstructionField instruction[2];

	memset(&instruction, 0, sizeof(instruction));

	if (!(reg & chip->cfg.mReadStausSupport)) {
		FLASH_NOTSUPPORT();
		return -1;
	}

	if (reg == FLASH_STATUS1) {
		FCI_CMD(0).data = FLASH_INSTRUCTION_RDSR1;
	} else if (reg == FLASH_STATUS2) {
		FCI_CMD(0).data = FLASH_INSTRUCTION_RDSR2;
	} else if (reg == FLASH_STATUS3) {
		FCI_CMD(0).data = FLASH_INSTRUCTION_RDSR3;
	} else {
		FLASH_NOWAY();
	}

	FCI_DATA(1).pdata = (uint8_t *)status;
	FCI_DATA(1).len = 1;
	FCI_DATA(1).line = 1;

	return chip->driverRead(chip, &FCI_CMD(0), NULL, NULL, &FCI_DATA(1));
}

int defaultWriteStatus(struct FlashChip *chip, FlashStatus reg, uint8_t *status)
{
	PCHECK(chip);
	InstructionField instruction[2];

	memset(&instruction, 0, sizeof(instruction));

	if (!(reg & chip->cfg.mWriteStatusSupport)) {
		FLASH_NOTSUPPORT();
		return HAL_INVALID;
	}

	FCI_CMD(0).data = FLASH_INSTRUCTION_SRWREN;
	FCI_CMD(0).line = 1;

	chip->driverWrite(chip, &FCI_CMD(0), NULL, NULL, NULL);

	memset(&instruction, 0, sizeof(instruction));

	if (reg == FLASH_STATUS1) {
		FCI_CMD(0).data = FLASH_INSTRUCTION_WRSR1;
	} else if (reg == FLASH_STATUS2) {
		FCI_CMD(0).data = FLASH_INSTRUCTION_WRSR2;
	} else if (reg == FLASH_STATUS3) {
		FCI_CMD(0).data = FLASH_INSTRUCTION_WRSR3;
	} else {
		FLASH_NOWAY();
	}

	FCI_DATA(1).pdata = (uint8_t *)status;
	FCI_DATA(1).len = 1;
	FCI_DATA(1).line = 1;

	return chip->driverWrite(chip, &FCI_CMD(0), NULL, NULL, &FCI_DATA(1));
}

int defaultErase(struct FlashChip *chip, FlashEraseMode mode, uint32_t eaddr)
{
	PCHECK(chip);
	InstructionField instruction[2];

	memset(&instruction, 0, sizeof(instruction));

	if (!(mode & chip->cfg.mEraseSizeSupport)) {
		FLASH_NOTSUPPORT();
		return HAL_INVALID;
	}

	FLASH_DEBUG("mode: 0x%x EraseSizeSupport: 0x%x", mode, chip->cfg.mEraseSizeSupport);

	if (mode == FLASH_ERASE_CHIP) {
		FCI_CMD(0).data = FLASH_INSTRUCTION_ERASE_CHIP;
		chip->driverWrite(chip, &FCI_CMD(0), NULL, NULL, NULL);
		return 0;
	} else if (mode == FLASH_ERASE_32KB) {
		FCI_CMD(0).data = FLASH_INSTRUCTION_ERASE_32KB;
	} else if (mode == FLASH_ERASE_64KB) {
		FCI_CMD(0).data = FLASH_INSTRUCTION_ERASE_64KB;
	} else if (mode == FLASH_ERASE_4KB) {
		FCI_CMD(0).data = FLASH_INSTRUCTION_ERASE_4KB;
	} else {
		FLASH_NOWAY();
	}

	FCI_ADDR(1).data = eaddr;
	FCI_ADDR(1).line = 1;

	return chip->driverWrite(chip, &FCI_CMD(0), &FCI_ADDR(1), NULL, NULL);
}

int defaultSuspendErasePageprogram(struct FlashChip *chip)
{
	PCHECK(chip);
	InstructionField cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.data = FLASH_INSTRUCTION_EPSP;
	return chip->driverWrite(chip, &cmd, NULL, NULL, NULL);
}

int defaultResumeErasePageprogram(struct FlashChip *chip)
{
	PCHECK(chip);
	InstructionField cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.data = FLASH_INSTRUCTION_EPRS;
	return chip->driverWrite(chip, &cmd, NULL, NULL, NULL);
}

int defaultPowerDown(struct FlashChip *chip)
{
	PCHECK(chip);
	InstructionField cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.data = FLASH_INSTRUCTION_PWDN;
	return chip->driverWrite(chip, &cmd, NULL, NULL, NULL);
}

int defaultReleasePowerDown(struct FlashChip *chip)
{
	PCHECK(chip);
	InstructionField instruction[3];

	memset(&instruction, 0, sizeof(instruction));
	FCI_CMD(0).data = FLASH_INSTRUCTION_REL;
	FCI_DUMMY(1).len = 3;
	FCI_DUMMY(1).line = 1;
	FCI_DATA(2).len = 1;
	FCI_DATA(2).line = 1;

	return chip->driverWrite(chip, &FCI_CMD(0), NULL, &FCI_DUMMY(1), &FCI_DATA(2));
}

int defaultGetJedecID(struct FlashChip *chip, uint32_t *jedec)
{
	PCHECK(chip);
	InstructionField instruction[2];

	memset(&instruction, 0, sizeof(instruction));
	FCI_CMD(0).data = FLASH_INSTRUCTION_RDID;
	FCI_DATA(1).pdata = (uint8_t *)jedec;
	FCI_DATA(1).line = 1;
	FCI_DATA(1).len = 3;
	return chip->driverRead(chip, &FCI_CMD(0), NULL, NULL, &FCI_DATA(1));
}

int defaultEnableQPIMode(struct FlashChip *chip)
{
	int ret;
	uint32_t tmp;
	PCHECK(chip);
	InstructionField cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.data = FLASH_INSTRUCTION_EN_QPI;
	ret = chip->driverWrite(chip, &cmd, NULL, NULL, NULL);

	if (ret >= 0)
		chip->mFlashStatus |= FLASH_READ_QPI_MODE;
	else
		return ret;

	tmp = 0x03;
	ret = chip->control(chip, DEFAULT_FLASH_SET_QPI_READ_P5P4, &tmp);
	if (ret >= 0)
		chip->mDummyCount = 4;

	return ret;
}

int defaultDisableQPIMode(struct FlashChip *chip)
{
	int ret;
	PCHECK(chip);
	InstructionField cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.data = FLASH_INSTRUCTION_DIS_QPI;
	cmd.line = 4;
	ret = chip->driverWrite(chip, &cmd, NULL, NULL, NULL);
	if (ret == 0)
		chip->mFlashStatus &= ~FLASH_READ_QPI_MODE;
	return ret;
}

/*
int defaultEnableReset(struct FlashChip *chip)
{
	PCHECK(chip);
	InstructionField cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.data = FLASH_INSTRUCTION_RSEN;
	return chip->driverWrite(chip, &cmd, NULL, NULL, NULL);
}
*/

int defaultReset(struct FlashChip *chip)
{
	int ret;
	PCHECK(chip);
	InstructionField cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.data = FLASH_INSTRUCTION_RSEN;
	ret = chip->driverWrite(chip, &cmd, NULL, NULL, NULL);
	if (ret < 0)
		return ret;

	memset(&cmd, 0, sizeof(cmd));
	cmd.data = FLASH_INSTRUCTION_RESET;
	return chip->driverWrite(chip, &cmd, NULL, NULL, NULL);
}

int defaultGetUniqueID(struct FlashChip *chip, uint8_t uid[8])
{
	PCHECK(chip);
	InstructionField instruction[3];

	memset(&instruction, 0, sizeof(instruction));
	FCI_CMD(0).data = FLASH_INSTRUCTION_RESET;
	FCI_DUMMY(1).line = 1;
	FCI_DUMMY(1).len = 4;
	FCI_DATA(2).pdata = uid;
	FCI_DATA(2).line = 1;
	FCI_DATA(2).len = 8;
	return chip->driverRead(chip, &FCI_CMD(0), NULL, &FCI_DUMMY(1), &FCI_DATA(2));
}

int defaultPageProgram(struct FlashChip *chip, FlashPageProgramMode mode, uint32_t waddr, const uint8_t *wdata, uint32_t size)
{
	PCHECK(chip);
	InstructionField instruction[3];

	memset(&instruction, 0, sizeof(instruction));

	if (size > chip->mPageSize)
		return -1;

	if (!(mode & chip->cfg.mPageProgramSupport)) {
		FLASH_NOTSUPPORT();
		return HAL_INVALID;
	}

	if (((waddr + size) > chip->cfg.mSize) || (size > chip->mPageSize))
		return -1;

	if (mode == FLASH_PAGEPROGRAM) {
		FCI_CMD(0).data = FLASH_INSTRUCTION_PP;
		FCI_DATA(2).line = 1;
	} else if (mode == FLASH_QUAD_PAGEPROGRAM) {
		FCI_CMD(0).data = FLASH_INSTRUCTION_QPP;
		FCI_DATA(2).line = 4;
	} else {
		FLASH_NOWAY();
	}

	FCI_ADDR(1).data = waddr;
	FCI_ADDR(1).line = 1;
	FCI_DATA(2).pdata = (uint8_t *)wdata;
	FCI_DATA(2).len = size;
	return chip->driverWrite(chip, &FCI_CMD(0), &FCI_ADDR(1), NULL, &FCI_DATA(2));
}

int defaultRead(struct FlashChip *chip, FlashReadMode mode, uint32_t raddr, uint8_t *rdata, uint32_t size)
{
	PCHECK(chip);
	InstructionField instruction[4];

	memset(&instruction, 0, sizeof(instruction));

	if (!(mode & chip->cfg.mReadSupport)) {
		FLASH_DEBUG("not support read mode: %d", mode);
		//FLASH_NOTSUPPORT(chip, );
		return HAL_INVALID;
	}

	if ((raddr + size) > chip->cfg.mSize)
		return -1;

	FLASH_DEBUG("size: %u mode: %u", size, (uint32_t)mode);

	switch (mode) {
	/* !!! NOTICE: m7~m0 is count to dummy byte. !!! */
	case FLASH_READ_NORMAL_MODE:
		FCI_CMD(0).data = FLASH_INSTRUCTION_READ;
		FCI_ADDR(1).line = 1;
		FCI_DATA(3).line = 1;
		FCI_DUMMY(2).len = 0;
		break;
	case FLASH_READ_FAST_MODE:
		FCI_CMD(0).data = FLASH_INSTRUCTION_FAST_READ;
		FCI_ADDR(1).line = 1;
		FCI_DATA(3).line = 1;
		FCI_DUMMY(2).len = 1;
		FCI_DUMMY(2).line = 1;
		break;
	case FLASH_READ_DUAL_O_MODE:
		FCI_CMD(0).data = FLASH_INSTRUCTION_FAST_READ_DO;
		FCI_ADDR(1).line = 1;
		FCI_DATA(3).line = 2;
		FCI_DUMMY(2).len = 1;
		FCI_DUMMY(2).line = 1;
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
		break;
	default:
		return -1;
	}

	FCI_ADDR(1).data = raddr;
	FCI_DATA(3).pdata = rdata;
	FCI_DATA(3).len = size;
	return chip->driverRead(chip, &FCI_CMD(0), &FCI_ADDR(1), &FCI_DUMMY(2), &FCI_DATA(3));
}

int defaultDriverWrite(struct FlashChip *chip, InstructionField *cmd, InstructionField *addr, InstructionField *dummy, InstructionField *data)
{
	if (chip == NULL || cmd == NULL)
		return -1;

	if (!(chip->mFlashStatus & FLASH_READ_QPI_MODE)) {
		cmd->len = 1;
		cmd->line = 1;	//not in QPI

		if (addr != NULL)
			addr->len = 3;
		if (data != NULL && data->pdata == NULL && data->len <= 4)
			data->pdata = (uint8_t *)&data->data;
	} else {
		cmd->len = 1;
		cmd->line = 4;	//not in QPI

		if (addr != NULL) {
			addr->len = 3;
			addr->line = 4;
		}
		if (dummy != NULL)
			dummy->line = 4;
		if (data != NULL)
			data->line = 4;
		if (data != NULL && data->pdata == NULL && data->len <= 4)
			data->pdata = (uint8_t *)&data->data;
	}

	if (chip->mDriver == NULL || chip->mDriver->write == NULL)
		return -1;

	FLASH_DEBUG("cmd: 0x%x", cmd->data);

	return chip->mDriver->write(chip, cmd, addr, dummy, data);
}

int defaultDriverRead(struct FlashChip *chip, InstructionField *cmd, InstructionField *addr, InstructionField *dummy, InstructionField *data)
{
	if (chip == NULL || cmd == NULL)
		return -1;

	if (!(chip->mFlashStatus & FLASH_READ_QPI_MODE)) {
		cmd->len = 1;
		cmd->line = 1;	//not in QPI

		if (addr != NULL)
			addr->len = 3;
		if (data != NULL && data->pdata == NULL && data->len <= 4)
			data->pdata = (uint8_t *)&data->data;
	} else { /* in QPI mode */
		cmd->len = 1;
		cmd->line = 4;	//not in QPI

		if (addr != NULL) {
			addr->len = 3;
			addr->line = 4;
		}
		if (dummy != NULL)
			dummy->line = 4;
		if (data != NULL)
			data->line = 4;
		if (data != NULL && data->pdata == NULL && data->len <= 4)
			data->pdata = (uint8_t *)&data->data;
	}

	if (chip->mDriver == NULL || chip->mDriver->read == NULL)
		return -1;

	FLASH_DEBUG("cmd: 0x%x", cmd->data);

	return chip->mDriver->read(chip, cmd, addr, dummy, data);
}

int defaultXipDriverCfg(struct FlashChip *chip, FlashReadMode mode, FlashContinuousRead con_read)
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

	HAL_Xip_setDummyData(chip->mXip, 0x20000000, 0, 0, 0);
	HAL_Xip_setCmd(chip->mXip, &FCI_CMD(0), &FCI_ADDR(1), &FCI_DUMMY(2), &FCI_DATA(3));
	HAL_Xip_setContinue(chip->mXip, continueMode, NULL);
	/*TODO: xip set delay*/

	return 0;
}

int defaultSetFreq(struct FlashChip *chip, uint32_t freq)
{
	PCHECK(chip);

	return chip->mDriver->setFreq(chip, freq);
}

int defaultSwitchReadMode(struct FlashChip *chip, FlashReadMode mode)
{
	PCHECK(chip);
	uint8_t status;
	int ret;

	if (!(mode & chip->cfg.mReadSupport)) {
		FLASH_NOTSUPPORT();
		return HAL_INVALID;
	}

	if (!((chip->cfg.mReadStausSupport & FLASH_STATUS2) && (chip->cfg.mWriteStatusSupport & FLASH_STATUS2))) {
		//do not need switch
		return 0;
	}

	if (mode == FLASH_READ_QUAD_O_MODE || mode == FLASH_READ_QUAD_IO_MODE || mode == FLASH_READ_QPI_MODE) {
		ret = chip->readStatus(chip, FLASH_STATUS2, &status);
		if (ret < 0)
			return -1;
		status |= 1 << 1;
		ret = chip->writeStatus(chip, FLASH_STATUS2, &status);
	} else {
		ret = chip->readStatus(chip, FLASH_STATUS2, &status);
		if (ret < 0)
			return -1;
		status &= ~(1 << 1);
		ret = chip->writeStatus(chip, FLASH_STATUS2, &status);
	}

	return ret;
}

int defaultEnableXIP(struct FlashChip *chip)
{
	/* TODO: it should mean the continue mode, so it would not use for now. */
	return 0;
}

int defaultDisableXIP(struct FlashChip *chip)
{
	/* TODO: it should mean the continue mode, so it would not use for now. */
	return 0;
}

int defaultIsBusy(struct FlashChip *chip)
{
	PCHECK(chip);
	uint8_t status;
	int ret;

	ret = chip->readStatus(chip, FLASH_STATUS1, &status);
	if (ret < 0)
		return -1;

	return !!(status & (1 << 0));
}

int defaultIsSuspend(struct FlashChip *chip)
{
	PCHECK(chip);
	uint8_t status;
	int ret;

	ret = chip->readStatus(chip, FLASH_STATUS2, &status);
	if (ret < 0)
		return -1;
	if ((status & (1 << 2)) || (status & (1 << 7)))
		return 1;
	else
		return 0;
}

static int defaultSetReadParam(struct FlashChip *chip, uint8_t param)
{
	PCHECK(chip);
	InstructionField instruction[2];

	memset(&instruction, 0, sizeof(instruction));
	FCI_CMD(0).data = FLASH_INSTRUCTION_SRP;
	FCI_DATA(1).pdata = (uint8_t *)&param;
	FCI_DATA(1).line = 4;
	FCI_DATA(1).len = 1;
	return chip->driverWrite(chip, &FCI_CMD(0), NULL, NULL, &FCI_DATA(1));
}

int defaultControl(struct FlashChip *chip, int op, void *param)
{
	PCHECK(chip);
	InstructionField cmd;

	memset(&cmd, 0, sizeof(cmd));
	switch (op) {
	case DEFAULT_FLASH_SET_QPI_READ_P5P4:
		if (*(uint8_t *)param > 0x03)
			return -1;
		defaultSetReadParam(chip, (*(uint8_t *)param) << 4);
		break;
	case DEFAULT_FLASH_POWERDOWN:
		cmd.data = FLASH_INSTRUCTION_PWDN;
		return chip->driverWrite(chip, &cmd, NULL, NULL, NULL);
		break;
	}
	return 0;
}

FlashEraseMode defaultGetMinEraseSize(struct FlashChip *chip)
{
	PCHECK(chip);

	if (chip->cfg.mEraseSizeSupport & FLASH_ERASE_4KB)
		return FLASH_ERASE_4KB;
	else if (chip->cfg.mEraseSizeSupport & FLASH_ERASE_32KB)
		return FLASH_ERASE_32KB;
	else if (chip->cfg.mEraseSizeSupport & FLASH_ERASE_64KB)
		return FLASH_ERASE_64KB;
	else
		return FLASH_ERASE_NOSUPPORT;
}

/* need not to write disable */
int defaultWriteSREnableForVolatile(struct FlashChip *chip)
{
	PCHECK(chip);
	InstructionField cmd;

	memset(&cmd, 0, sizeof(cmd));

	cmd.data = FLASH_INSTRUCTION_SRWREN;
	cmd.line = 1;

	return chip->driverWrite(chip, &cmd, NULL, NULL, NULL);
}

#ifdef CONFIG_FLASH_POWER_DOWN_PROTECT
int defaultUnLockBlock(struct FlashChip *chip, uint32_t addr)
{
	PCHECK(chip);
	int ret;
	uint8_t status = 1;
	InstructionField instruction[2];

	memset(&instruction, 0, sizeof(instruction));

	FD_DEBUG("UnLock addr is 0x%x\n", addr);
	FCI_CMD(0).data = FLASH_INSTRUCTION_BLOCK_UNLOCK;
	FCI_ADDR(1).data = addr;
	FCI_ADDR(1).line = 1;
	ret = chip->driverWrite(chip, &FCI_CMD(0), &FCI_ADDR(1), NULL, NULL);
	return ret;
}

int defaultInitLock(struct FlashChip *chip)
{
	PCHECK(chip);
	uint8_t status;
	int ret;

	ret = chip->readStatus(chip, FLASH_STATUS3, &status);
	if (ret < 0)
		return -1;
	status |= 1 << 2;
	ret = chip->writeStatus(chip, FLASH_STATUS3, &status);

	return ret;
}

int defaultLockBlock(struct FlashChip *chip, uint32_t addr)
{
	PCHECK(chip);
	InstructionField instruction[2];

	memset(&instruction, 0, sizeof(instruction));

	FD_DEBUG("Lock addr is 0x%x\n", addr);
	FCI_CMD(0).data = FLASH_INSTRUCTION_BLOCK_LOCK;
	FCI_ADDR(1).data = addr;
	FCI_ADDR(1).line = 1;

	return chip->driverWrite(chip, &FCI_CMD(0), &FCI_ADDR(1), NULL, NULL);
}

int defaultSetGlobalBlockLockState(struct FlashChip *chip, FlashBlockLockState state)
{
	PCHECK(chip);
	InstructionField cmd;

	memset(&cmd, 0, sizeof(cmd));

	if (state == FLASH_BLOCK_STATE_LOCK) {
		cmd.data = FLASH_INSTRUCTION_ALL_BLOCK_LOCK;
	} else if (state == FLASH_BLOCK_STATE_UNLOCK) {
		cmd.data = FLASH_INSTRUCTION_ALL_BLOCK_UNLOCK;
	} else {
		FLASH_NOWAY();
	}
	cmd.line = 1;

	return chip->driverWrite(chip, &cmd, NULL, NULL, NULL);
}

/* 1:  */
int defaultReadBlockLockStatus(struct FlashChip *chip, uint32_t raddr, uint8_t *rdata)
{
	PCHECK(chip);
	InstructionField instruction[3];

	memset(&instruction, 0, sizeof(instruction));
	if (raddr > chip->cfg.mSize) {
		return -1;
	}

	FCI_CMD(0).data = FLASH_INSTRUCTION_RD_BLOCKLOCK;
	FCI_ADDR(1).line = 1;
	FCI_ADDR(1).data = raddr;
	FCI_DATA(2).line = 1;
	FCI_DATA(2).pdata = rdata;
	FCI_DATA(2).len = 1;

	return chip->driverRead(chip, &FCI_CMD(0), &FCI_ADDR(1), NULL, &FCI_DATA(2));
}
#endif /* CONFIG_FLASH_POWER_DOWN_PROTECT */

