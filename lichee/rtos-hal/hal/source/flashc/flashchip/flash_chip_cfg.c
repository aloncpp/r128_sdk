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

#include "flash_chip.h"
#include "../porting.h"

static const FlashChipCfg simpleFlashChipCfg[] = {
/*
 * #ifdef FLASH_xxxxx
 *	{
 *		.mJedec = 0x ID7-ID0 ID15-ID8 M7-M0 from jedec id,
 *		.mSize = total flash memory size,
 *		.mEraseSizeSupport = the flash erase commands is supported,
 *		.mPageProgramSupport = the flash pageprogram commands is supported,
 *		.mReadStausSupport = the flash status registers can be read,
 *		.mWriteStatusSupport = the flash status registers can be written,
 *		.mReadSupport = the flash read commands (modes) is supported,
 *		.mMaxFreq = max operation frequency to flash,
 *		.mMaxReadFreq = max read command frequency(only read command: 0x03h),
 *	},
 * #endif
 */
	{
		/* default config must be at first */
		.mJedec = 0,	/* ID7-ID0 ID15-ID8 M7-M0 */
		.mSize = 128*1024*1024,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE | FLASH_READ_QPI_MODE,
		.mMaxFreq = -1,
		.mMaxReadFreq = -1,
		.mSuspendSupport = 1,
		.mSuspend_Latency = 30,
		.mResume_Latency = 200,
	},
	{
		.mJedec = 0x0c20e4,
		.mSize = 16*1024*1024,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE,
		.mMaxFreq = 100 * 1000 * 1000,
		.mMaxReadFreq = 100 * 1000 * 1000,
		.mSuspendSupport = 0,
		.mSuspend_Latency = 0,
		.mResume_Latency = 0,
	},
	{
		.mJedec = 0x1840c8,
		.mSize = 16*1024*1024,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE,
		.mMaxFreq = 100 * 1000 * 1000,
		.mMaxReadFreq = 100 * 1000 * 1000,
		.mSuspendSupport = 0,
		.mSuspend_Latency = 0,
		.mResume_Latency = 0,
	},
	{
		.mJedec = 0x1840ef,
		.mSize = 16*1024*1024,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
				| FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
		.mContinuousReadSupport = FLASH_CONTINUOUS_READ_SUPPORT,
		.mBurstWrapReadSupport = FLASH_BURST_WRAP_16B,
		.mMaxFreq = 100 * 1000 * 1000,
		.mMaxReadFreq = 100 * 1000 * 1000,
		.mSuspendSupport = 0,
		.mSuspend_Latency = 0,
		.mResume_Latency = 0,
	},
	{
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
	},

	{	/* FM25W128 */
		.mJedec = 0x1828A1,
		.mSize = 16*1024*1024,
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
	},
	{	/* FM25Q128A */
		.mJedec = 0x1840A1,
		.mSize = 16*1024*1024,
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
	},
#ifdef FLASH_PN25F16B
	{
		/* FLASH_PN25F16B */
		.mJedec = 0x15405E,
		.mSize = 32 * 16 * 0x1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2,
		.mWriteStatusSupport = FLASH_STATUS1,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE,
		.mMaxFreq = 100 * 1000 * 1000,
		.mMaxReadFreq = 55 * 1000 * 1000,
		.mSuspendSupport = 0,
		.mSuspend_Latency = 0,
		.mResume_Latency = 0,
	},
#endif
#ifdef FLASH_M25P64
	{
		/* FLASH_M25P64 */
		.mJedec = 0x172020,
		.mSize = 128 * 0x10000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1,
		.mWriteStatusSupport = 0,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE,
		.mMaxFreq = 50 * 1000 * 1000,
		.mMaxReadFreq = 20 * 1000 * 1000,
		.mSuspendSupport = 1,
		.mSuspend_Latency = 30,
		.mResume_Latency = 200,
	},
#endif
#ifdef FLASH_W25Q16FW
	{
		/* FLASH_W25Q16FW */
		.mJedec = 0x1560EF,
		.mSize = 32 *16 * 0x1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
		.mMaxFreq = 80 * 1000 * 1000,
		.mMaxReadFreq = 50 * 1000 * 1000,
		.mSuspendSupport = 1,
		.mSuspend_Latency = 30,
		.mResume_Latency = 200,
	},
#endif
#ifdef FLASH_PN25F08
	{
		/* FLASH_PN25F08 */
		.mJedec = 0x14405E,
		.mSize = 16 * 16 * 0x1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2,
		.mWriteStatusSupport = FLASH_STATUS1 /* write status2 need to rewrite writeStatus */,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
		.mMaxFreq = 108 * 1000 * 1000,
		.mMaxReadFreq = 55 * 1000 * 1000,
		.mSuspendSupport = 1,
		.mSuspend_Latency = 30,
		.mResume_Latency = 200,
	},
#endif
#ifdef FLASH_PN25F16
	{
		/* FLASH_PN25F16 */
		.mJedec = 0x1540E0,
		.mSize = 32 * 16 * 0x1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2,
		.mWriteStatusSupport = FLASH_STATUS1 /* write status2 need to rewrite writeStatus */,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
		.mMaxFreq = 108 * 1000 * 1000,
		.mMaxReadFreq = 55 * 1000 * 1000,
		.mSuspendSupport = 1,
		.mSuspend_Latency = 30,
		.mResume_Latency = 200,
	},
#endif
#ifdef FLASH_MX25L1636E
	{
		/* FLASH_MX25L1636E */
		.mJedec = 0x1525C2,
		.mSize = 32 * 16 * 0x1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM /* QPP need to rewrite pageProgram */,
		.mReadStausSupport = FLASH_STATUS1,
		.mWriteStatusSupport = FLASH_STATUS1,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_IO_MODE,
		.mMaxFreq = 108 * 1000 * 1000,
		.mMaxReadFreq = 50 * 1000 * 1000,
		.mSuspendSupport = 0,
		.mSuspend_Latency = 0,
		.mResume_Latency = 0,
	},
#endif
#ifdef FLASH_MX25L1633E
	{
		/* FLASH_MX25L1633E */
		.mJedec = 0x1524C2,
		.mSize = 32 * 16 * 0x1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM /* QPP need to rewrite pageProgram */,
		.mReadStausSupport = FLASH_STATUS1,
		.mWriteStatusSupport = FLASH_STATUS1,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_IO_MODE,
		.mMaxFreq = 85 * 1000 * 1000,
		.mMaxReadFreq = 33 * 1000 * 1000,
		.mSuspendSupport = 0,
		.mSuspend_Latency = 0,
		.mResume_Latency = 0,
	},
#endif
#ifdef FLASH_MX25L12835F
	{
		/* FLASH_MX25L12835F */
		.mJedec = 0x1820C2,
		.mSize = 16 * 1000 * 1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM /* QPP need to rewrite pageProgram */,
		.mReadStausSupport = FLASH_STATUS1,
		.mWriteStatusSupport = FLASH_STATUS1,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_IO_MODE,
		.mContinuousReadSupport = FLASH_CONTINUOUS_READ_SUPPORT,
		.mMaxFreq = 100 * 1000 * 1000,
		.mMaxReadFreq = 50 * 1000 * 1000,
		.mSuspendSupport = 0,
		.mSuspend_Latency = 0,
		.mResume_Latency = 0,
	},
#endif
#ifdef FLASH_XM25QH16B
	{
		/* FLASH_XM25QH16B */
		.mJedec = 0x154020,
		.mSize = 32 * 16 * 0x1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE | FLASH_READ_QPI_MODE,
		.mMaxFreq = 104 * 1000 * 1000,
		.mMaxReadFreq = 80 * 1000 * 1000,
		.mSuspendSupport = 1,
		.mSuspend_Latency = 30,
		.mResume_Latency = 200,
	},
#endif
#ifdef FLASH_XM25QH32B
	{
		/* FLASH_XM25QH32B */
		.mJedec = 0x164020,
		.mSize = 64 * 16 * 0x1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE | FLASH_READ_QPI_MODE,
		.mMaxFreq = 104 * 1000 * 1000,
		.mMaxReadFreq = 80 * 1000 * 1000,
		.mSuspendSupport = 1,
		.mSuspend_Latency = 30,
		.mResume_Latency = 200,
	},
#endif
#ifdef FLASH_BY25Q64AS
	{
		/* FLASH_BY25Q64AS */
		.mJedec = 0x174068,
		.mSize = 128 * 16 * 0x1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
		.mMaxFreq = 120 * 1000 * 1000,
		.mMaxReadFreq = 55 * 1000 * 1000,
		.mSuspendSupport = 1,
		.mSuspend_Latency = 30,
		.mResume_Latency = 200,
	},
#endif

	{
		/* FLASH_BY25Q128AS */
		.mJedec = 0x184068,
		.mSize = 256 * 16 * 0x1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
		.mMaxFreq = 120 * 1000 * 1000,
		.mMaxReadFreq = 55 * 1000 * 1000,
		.mSuspendSupport = 1,
		.mSuspend_Latency = 30,
		.mResume_Latency = 200,
	},
	{
		/* FLASH_BY25Q256FS */
		.mJedec = 0x194968,
		.mSize = 512 * 16 * 0x1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
		.mMaxFreq = 120 * 1000 * 1000,
		.mMaxReadFreq = 55 * 1000 * 1000,
		.mSuspendSupport = 1,
		.mSuspend_Latency = 30,
		.mResume_Latency = 200,
	},

#ifdef FLASH_BY25Q32BS
	{
		/* FLASH_BY25Q32BS */
		.mJedec = 0x164068,
		.mSize = 64 * 16 * 0x1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
		.mMaxFreq = 108 * 1000 * 1000,
		.mMaxReadFreq = 55 * 1000 * 1000,
		.mSuspendSupport = 1,
		.mSuspend_Latency = 30,
		.mResume_Latency = 200,
	},
#endif
#ifdef FLASH_BY25D16
	{
		/* FLASH_BY25D16 */
		.mJedec = 0x154068,
		.mSize = 32 * 16 * 0x1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1,
		.mWriteStatusSupport = FLASH_STATUS1,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE,
		.mMaxFreq = 108 * 1000 * 1000,
		.mMaxReadFreq = 55 * 1000 * 1000,
		.mSuspendSupport = 0,
		.mSuspend_Latency = 0,
		.mResume_Latency = 0,
	},
#endif
#ifdef FLASH_BY25D80
	{
		/* FLASH_BY25D80 */
		.mJedec = 0x144068,
		.mSize = 16 * 16 * 0x1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1,
		.mWriteStatusSupport = FLASH_STATUS1,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE,
		.mMaxFreq = 108 * 1000 * 1000,
		.mMaxReadFreq = 55 * 1000 * 1000,
		.mSuspendSupport = 0,
		.mSuspend_Latency = 0,
		.mResume_Latency = 0,
	},
#endif
#ifdef FLASH_EN25Q80B
	{
		/* FLASH_EN25Q80B */
		.mJedec = 0x14301C,
		.mSize = 16 * 16 * 0x1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1,
		.mWriteStatusSupport = FLASH_STATUS1,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | /*FLASH_READ_QUAD_O_MODE |*/ FLASH_READ_QUAD_IO_MODE,
		.mMaxFreq = 104 * 1000 * 1000,
		.mMaxReadFreq = 50 * 1000 * 1000,
		.mSuspendSupport = 0,
		.mSuspend_Latency = 0,
		.mResume_Latency = 0,
	},
#endif
#ifdef FLASH_EN25QH16A
	{
		/* FLASH_EN25QH16A */
		.mJedec = 0x15701C,
		.mSize = 32 * 16 * 0x1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1,
		.mWriteStatusSupport = FLASH_STATUS1,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | /*FLASH_READ_QUAD_O_MODE |*/ FLASH_READ_QUAD_IO_MODE,
		.mMaxFreq = 104 * 1000 * 1000,
		.mMaxReadFreq = 50 * 1000 * 1000,
		.mSuspendSupport = 0,
		.mSuspend_Latency = 0,
		.mResume_Latency = 0,
	},
#endif
#ifdef FLASH_EN25Q32C
	{
		/* FLASH_EN25Q32C */
		.mJedec = 0x16301C,
		.mSize = 64 * 16 * 0x1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1,
		.mWriteStatusSupport = FLASH_STATUS1,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
		.mMaxFreq = 104 * 1000 * 1000,
		.mMaxReadFreq = 50 * 1000 * 1000,
		.mSuspendSupport = 0,
		.mSuspend_Latency = 0,
		.mResume_Latency = 0,
	},
#endif
#if 0
#ifdef FLASH_EN25QX128A
	{
		/* FLASH_EN25QX128C */
		.mJedec = 0x18711C,
		.mSize = 16 * 1000 * 1000,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1,
		.mWriteStatusSupport = FLASH_STATUS1,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
		//.mContinuousReadSupport = FLASH_CONTINUOUS_READ_SUPPORT,
		.mMaxFreq = 104 * 1000 * 1000,
		.mMaxReadFreq = 50 * 1000 * 1000,
		.mSuspendSupport = 0,
		.mSuspend_Latency = 0,
		.mResume_Latency = 0,
	},
#endif
#endif
	{
		/* FLASH_GD25Q127C */
		.mJedec = 0x1840C8,
		.mSize = 16 * 1024 * 1024,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
		.mContinuousReadSupport = FLASH_CONTINUOUS_READ_SUPPORT,
		.mMaxFreq = 104 * 1000 * 1000,
		.mMaxReadFreq = 50 * 1000 * 1000,
		.mSuspendSupport = 1,
		.mSuspend_Latency = 30,
		.mResume_Latency = 200,
	},
	{
		/* FLASH_GD25B512ME */
		.mJedec = 0x1A47C8,
		.mSize = 64 * 1024 * 1024,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
		.mContinuousReadSupport = FLASH_CONTINUOUS_READ_SUPPORT,
		.mMaxFreq = 104 * 1000 * 1000,
		.mMaxReadFreq = 50 * 1000 * 1000,
		.mSuspendSupport = 1,
		.mSuspend_Latency = 30,
		.mResume_Latency = 200,
	},
#ifdef FLASH_GD25Q256D
	{
		/* FLASH_GD25Q256D */
		.mJedec = 0x1940C8,
		.mSize = 32 * 1024 * 1024,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
		.mMaxFreq = 104 * 1000 * 1000,
		.mMaxReadFreq = 50 * 1000 * 1000,
		.mSuspendSupport = 1,
		.mSuspend_Latency = 30,
		.mResume_Latency = 200,
	},
#endif

	{
		/* FLASH_PY25Q64HA */
		.mJedec = 0x172085,
		.mSize = 8 * 1024 * 1024,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
		.mMaxFreq = 104 * 1000 * 1000,
		.mMaxReadFreq = 50 * 1000 * 1000,
		.mSuspendSupport = 1,
		.mSuspend_Latency = 30,
		.mResume_Latency = 200,
	},
	{
		/* FLASH_PY25Q128HA */
		.mJedec = 0x182085,
		.mSize = 16 * 1024 * 1024,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
		.mMaxFreq = 104 * 1000 * 1000,
		.mMaxReadFreq = 50 * 1000 * 1000,
		.mSuspendSupport = 1,
		.mSuspend_Latency = 30,
		.mResume_Latency = 200,
	},
	{
		/* FLASH_PY25Q256HA */
		.mJedec = 0x192085,
		.mSize = 32 * 1024 * 1024,
		.mEraseSizeSupport = FLASH_ERASE_64KB | FLASH_ERASE_32KB | FLASH_ERASE_4KB | FLASH_ERASE_CHIP,
		.mPageProgramSupport = FLASH_PAGEPROGRAM,
		.mReadStausSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mWriteStatusSupport = FLASH_STATUS1 | FLASH_STATUS2 | FLASH_STATUS3,
		.mReadSupport = FLASH_READ_NORMAL_MODE | FLASH_READ_FAST_MODE | FLASH_READ_DUAL_O_MODE
		                | FLASH_READ_DUAL_IO_MODE | FLASH_READ_QUAD_O_MODE | FLASH_READ_QUAD_IO_MODE,
		.mMaxFreq = 104 * 1000 * 1000,
		.mMaxReadFreq = 50 * 1000 * 1000,
		.mSuspendSupport = 1,
		.mSuspend_Latency = 30,
		.mResume_Latency = 200,
	},
};

#ifdef CONFIG_FLASH_POWER_DOWN_PROTECT
/* FLASH_W25Q16JL */
#ifdef FLASH_W25Q16JL
static const FlashChipBlockLockModeCfg w25q16jlFlashChipblockLockCfg = {
	.mWpSectorSize = 4096,
	.mWpBlockSize = 65536,
	.mBtmBlockBoundaryAddr = 65536 * 1,
	.mTopBlockBoundaryAddr = 65536 * 31,
};
#endif

static const FlashChipWpCfg simpleFlashChipWpCfg[] = {
#ifdef FLASH_W25Q16JL
	{
		/* FLASH_W25Q16JL */
		.mJedec = 0x1540EF,
		.mWpBlockLockCfg = &w25q16jlFlashChipblockLockCfg,
		.mWpAreaLockCfg = NULL,/* TODO: add to support flash area protect mode */
	},
#endif
};

const FlashChipWpCfg *FlashChipGetWpCfgList(int32_t *len)
{
	*len = HAL_ARRAY_SIZE(simpleFlashChipWpCfg);
	return simpleFlashChipWpCfg;
}
#endif /* CONFIG_FLASH_POWER_DOWN_PROTECT */

const FlashChipCfg *FlashChipGetCfgList(int32_t *len)
{
	*len = HAL_ARRAY_SIZE(simpleFlashChipCfg);
	return simpleFlashChipCfg;
}
