/**
  * @file  hal_flashctrl.c
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
#include <string.h>
#include "hal_flash.h"
#include "hal_flashctrl.h"
#include "hal_xip.h"
#include <hal_mem.h>
#include <hal_atomic.h>
#include <barrier.h>
#include <hal_timer.h>
#include "sunxi_hal_common.h"
#include <hal_gpio.h>
#include <hal_clk.h>
#include <hal_reset.h>
#include <hal_log.h>
#include <hal_mem.h>
#include <hal_cache.h>
#include <hal_dma.h>
#include <task.h>
#define FLASHC_SKIP_CCMU
#ifdef CONFIG_TRUSTZONE
#include "nsc_table.h"
#include "tz_rpc.h"
#endif

#define FLASH_SKIP_CRYPTO

#include "flash_debug.h"

#define __ISB() isb()
#define HAL_EnterCriticalSection()  ({hal_spin_lock_irqsave(&flashctrl_lock);})
#define HAL_ExitCriticalSection(f)  ({hal_spin_unlock_irqrestore(&flashctrl_lock, f);})
#define HAL_ThreadResumeScheduler()  xTaskResumeAll()
#define HAL_ThreadSuspendScheduler() vTaskSuspendAll()
#define HAL_ThreadIsSchedulerRunning OS_ThreadIsSchedulerRunning

static int OS_ThreadIsSchedulerRunning(void)
{
	return (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
}
static hal_spinlock_t flashctrl_lock;

#define HAL_IsIRQDisabled() hal_interrupt_is_disable()

static int nor_sip()
{
#ifdef CONFIG_ARCH_SUN20IW2
	int HAL_PRCM_IsFlashSip(void);
	return HAL_PRCM_IsFlashSip();
#else
	return 0;
#endif
}

static void dma_cb(void *param)
{
    hal_sem_post(((struct flash_controller *)param)->dmaSem);
}

#if (CONFIG_CHIP_ARCH_VER == 3)
void HAL_FlashCrypto_Init(uint8_t *nonce)
{
#ifndef FLASHC_SKIP_CCMU
#ifdef CONFIG_TRUSTZONE
	TEE_FUNC_CALL(FlashCryptoInit_NSC);
#else
	HAL_CCM_BusEnablePeriphClock(CCM_BUS_PERIPH_BIT_FLASH_ENC);
	HAL_CCM_BusReleasePeriphReset(CCM_BUS_PERIPH_BIT_FLASH_ENC);
#endif
	HAL_PRCM_SetFlashCryptoNonce(nonce);
#endif
}

void HAL_FlashCrypto_DeInit(void)
{
#ifndef FLASHC_SKIP_CCMU
#ifdef CONFIG_TRUSTZONE
	TEE_FUNC_CALL(FlashCryptoDeInit_NSC);
#else
	HAL_CCM_BusDisablePeriphClock(CCM_BUS_PERIPH_BIT_FLASH_ENC);
	HAL_CCM_BusForcePeriphReset(CCM_BUS_PERIPH_BIT_FLASH_ENC);
#endif
#endif
}

int32_t FlashCryptoRequest(uint32_t startAddr, uint32_t endAddr, uint8_t *key)
{
#ifdef CONFIG_TRUSTZONE
	return TEE_FUNC_CALL(FlashCryptoRequest_NSC, startAddr, endAddr, key);
#else
	return 0; /* to do */
#endif
}

#if FLASH_CRYPTO_DISABLE_FUNC_EN
void FlashCryptoRelease(uint32_t channel)
{
#ifdef CONFIG_TRUSTZONE
	TEE_FUNC_CALL(FlashCryptoRelease_NSC, channel);
#else
	/* to do */
#endif
}
#endif
#endif

#if (CONFIG_CHIP_ARCH_VER == 3) && (FLASH_CRYPTO_DISABLE_FUNC_EN)
static uint32_t _flashc_read_decrypt = 0;
static uint8_t _flashc_crypto_key[16];

void FC_Crypto_Enable(uint8_t *key)
{
	memcpy(_flashc_crypto_key, key, sizeof(_flashc_crypto_key));
	_flashc_read_decrypt = 1;
}

static inline uint32_t FC_Get_Crypto_Infor(uint8_t *key)
{
	memcpy(key, _flashc_crypto_key, sizeof(_flashc_crypto_key));
	return _flashc_read_decrypt;
}

void FC_Clr_Crypto_Infor(void)
{
	_flashc_read_decrypt = 0UL;
	memset(_flashc_crypto_key, 0, sizeof(_flashc_crypto_key));
}
#endif

static inline int FC_Sbus_GetDebugState(void);

#if 1 //(FC_DEBUG_ON == DBG_ON)
#define FC_WHILE_TIMEOUT(cond, to) \
	do { volatile uint32_t i = to; \
		do { \
			if (--i == 0) { \
				FC_Reg_All(); \
				return HAL_ERROR; \
			} \
		} while (cond); \
	} while (0)
#else
#define FC_WHILE_TIMEOUT(cond, to) \
	while (cond)
#endif

#ifndef CONFIG_BOOTLOADER
uint8_t fc_debug_mask = FC_WRN_FLAG | FC_ERR_FLAG;

void HAL_Flashc_SetDbgMask(uint8_t dbg_mask)
{
	fc_debug_mask = dbg_mask;
}
#endif

#if 0 //(FC_DEBUG_ON == DBG_ON) // not robust implemetation, only for debug
#define FC_DebugCheck(state) __FC_DebugCheck(state, __LINE__)
static int __FC_DebugCheck(int state, uint32_t line)
{
	int debug = FC_Sbus_GetDebugState();
	if (debug != state) {
		hal_udelay(5000);
		debug = FC_Sbus_GetDebugState();
		if (debug != state) {
			FC_ERROR("line: %d, err stat: 0x%x", line, state);
			Regs_Print();
			return -1;
		}
	}
	return 0;
}
#else
#define FC_DebugCheck(state) __FC_DebugCheck(state)
inline int __FC_DebugCheck(int state)
{
	while (FC_Sbus_GetDebugState() != state);
	return 0;
}
#endif

#define FC_REG_ALL() \
    { \
        printf("QPI reg: base addr             0x%8x\n", (uint32_t)&(OPI_MEM_CTRL->MEM_COM_CONFG)); \
        printf("MEM_COM_CONFG:                 0x%8x\n", OPI_MEM_CTRL->MEM_COM_CONFG);\
        printf("SQPI_CTRL_COM_CONFG:           0x%8x\n", OPI_MEM_CTRL->SQPI_CTRL_COM_CONFG);\
        printf("CACHE_RLVT_CONFG:              0x%8x\n", OPI_MEM_CTRL->CACHE_RLVT_CONFG);\
        printf("MEM_AC_CHR_TIMING_CONFG:       0x%8x\n", OPI_MEM_CTRL->MEM_AC_CHR_TIMING_CONFG);\
        printf("CBUS_RD_OPRT_CONFG:            0x%8x\n", OPI_MEM_CTRL->CBUS_RD_OPRT_CONFG);\
        printf("CBUS_WR_OPRT_CONFG:            0x%8x\n", OPI_MEM_CTRL->CBUS_WR_OPRT_CONFG);\
        printf("CBUS_RD_DUMMY_DATA_TOP_HALF:   0x%8x\n", OPI_MEM_CTRL->CBUS_RD_DUMMY_DATA_TOP_HALF);\
        printf("CBUS_RD_DUMMY_DATA_BUTT_HALF:  0x%8x\n", OPI_MEM_CTRL->CBUS_RD_DUMMY_DATA_BUTT_HALF);\
        printf("CBUS_WR_DUMMY_DATA_TOP_HALF:   0x%8x\n", OPI_MEM_CTRL->CBUS_WR_DUMMY_DATA_TOP_HALF);\
        printf("CBUS_WR_DUMMY_DATA_BUTT_HALF:  0x%8x\n", OPI_MEM_CTRL->CBUS_WR_DUMMY_DATA_BUTT_HALF);\
        printf("CBUS_IO_SW_WAIT_TIME:          0x%8x\n", OPI_MEM_CTRL->CBUS_IO_SW_WAIT_TIME);\
        printf("SBUS_RW_OPRT_CONFG:            0x%8x\n", OPI_MEM_CTRL->SBUS_RW_OPRT_CONFG);\
        printf("SBUS_ADDR_CONFG:               0x%8x\n", OPI_MEM_CTRL->SBUS_ADDR_CONFG);\
        printf("SBUS_DUMMY_DATA_TOP_HALF:      0x%8x\n", OPI_MEM_CTRL->SBUS_DUMMY_DATA_TOP_HALF);\
        printf("SBUS_DUMMY_DATA_BUTT_HALF:     0x%8x\n", OPI_MEM_CTRL->SBUS_DUMMY_DATA_BUTT_HALF);\
        printf("SBUS_IO_SW_WAIT_TIME:          0x%8x\n", OPI_MEM_CTRL->SBUS_IO_SW_WAIT_TIME);\
        printf("SBUS_WR_DATA_BYTE_NUM:         0x%8x\n", OPI_MEM_CTRL->SBUS_WR_DATA_BYTE_NUM);\
        printf("SBUS_RD_DATA_BYTE_NUM:         0x%8x\n", OPI_MEM_CTRL->SBUS_RD_DATA_BYTE_NUM);\
        printf("SBUS_START_SEND_REG:           0x%8x\n", OPI_MEM_CTRL->SBUS_START_SEND_REG);\
        printf("FIFO_TRIGGER_LEVEL:            0x%8x\n", OPI_MEM_CTRL->FIFO_TRIGGER_LEVEL);\
        printf("FIFO_STATUS_REG:               0x%8x\n", OPI_MEM_CTRL->FIFO_STATUS_REG);\
        printf("INT_ENABLE_REG:                0x%8x\n", OPI_MEM_CTRL->INT_ENABLE_REG);\
        printf("INT_STATUS_REG:                0x%8x\n", OPI_MEM_CTRL->INT_STATUS_REG);\
        printf("XIP_WARP_MODE_EXE_IDCT:        0x%8x\n", OPI_MEM_CTRL->XIP_WARP_MODE_EXE_IDCT);\
        printf("MEM_CTRL_DEBUG_STATE:          0x%8x\n", OPI_MEM_CTRL->MEM_CTRL_DEBUG_STATE);\
        printf("DEBUG_CNT_SBUS_WR:             0x%8x\n", OPI_MEM_CTRL->DEBUG_CNT_SBUS_WR);\
        printf("DEBUG_CNT_SBUS_RD:             0x%8x\n", OPI_MEM_CTRL->DEBUG_CNT_SBUS_RD);\
        printf("NOP INSTRUCTION:               0x%8x\n", OPI_MEM_CTRL->NOP_INSTRUCTION);\
        printf("C_RW_EN_WAITTIME:              0x%8x\n", OPI_MEM_CTRL->CBUS_RW_EN_WTIME);\
        printf("DEBUG_CNT_CBUS_RD:             0x%8x\n", OPI_MEM_CTRL->DEBUG_CNT_CBUS_RD);\
        printf("DEBUG_CNT_CBUS_WR:             0x%8x\n", OPI_MEM_CTRL->DEBUG_CNT_CBUS_WR);\
        printf("START_ADDR0:                   0x%8x\n", OPI_MEM_CTRL->FLASH_ADDR[0].START_ADDR);\
        printf("END_ADDR0:                     0x%8x\n", OPI_MEM_CTRL->FLASH_ADDR[0].END_ADDR);\
        printf("BIAS_ADDR0:                    0x%8x\n", OPI_MEM_CTRL->FLASH_ADDR[0].BIAS_ADDR);\
    }

static inline void FC_Reg_All()
{
	//FC_REG_ALL();
}

static inline void FC_Reg_CLK()
{
	printf("---------------------------------------------------\n");
	printf("bus_clk_gating(0x%x bit22~23):0x%08x\n",0x4003C000+0x04, *((u32 *)(0x4003C000+0x04)));
	printf("module_reset_ctl(0x%x bit22~23):0x%08x\n",0x4003C000+0x0C, *((u32 *)(0x4003C000+0x0C)));
	printf("flash_spi_clk_ctl(0x%x):0x%08x\n",0x4003C000+0x54, *((u32 *)(0x4003C000+0x54)));

	printf("sys_clk_ctl(0x%x):0x%08x\n",0x4004C400+0xE0, *((u32 *)(0x4004C400+0xE0)));
	printf("dpll1_out_cfg(0x%x):0x%08x\n",0x4004C400+0xA4, *((u32 *)(0x4004C400+0xA4)));

	printf("---------------------------------------------------\n");
}

void FC_GetDelayCycle(Flash_Ctrl_DelayCycle *delay, uint32_t freq)
{
	memset(delay, 0, sizeof(Flash_Ctrl_DelayCycle));
	delay->cs_begin = 1;

	/*delay.cs_deselect = t_shsl_ns * (freq / 1000000) / 1000;*/
	delay->cs_deselect = 5;

	if (freq < 48000000)
		delay->data = 0;
	else if (freq <= 64000000)
		delay->data = 1;
	else
		delay->data = 2;
}

uint32_t FC_GetXipUserEndAddr(void)
{
#ifdef CONFIG_TRUSTZONE
	uint32_t addr = 0;
	addr = TEE_FUNC_CALL(TZ_GetFlashXipStartAddr_NSC);
	addr = (addr == 0) ? PSRAM_START_ADDR : addr;
	return addr;
#else
#define PSRAM_START_ADDR 0x1000000
	return PSRAM_START_ADDR;
#endif
}

int32_t FlashCBUS_AddrRequire(uint8_t field, uint32_t sAddr, uint32_t bias, uint32_t userLen)
{
	int32_t cache_bias;

	if (field > FLASH_ADDR_USER_RD_IDX) {
		//return -1;
	}

#ifdef CONFIG_TRUSTZONE
	tz_remote_call_t rpc_t;
	rpc_t.arg0 = field;
	rpc_t.arg1 = sAddr;
	rpc_t.arg2 = bias;
	rpc_t.retArg0 = userLen;
	cache_bias = TEE_FUNC_CALL(FlashCBUS_AddrRequire_NSC, &rpc_t);
#else
	uint32_t eAddr;

	cache_bias = rounddown2(bias, FLASH_XIP_USER_ADDR_STEP);
	eAddr = roundup2(sAddr + userLen + (bias - cache_bias), FLASH_XIP_USER_ADDR_STEP);

	HAL_MODIFY_REG(OPI_MEM_CTRL->FLASH_ADDR[field].START_ADDR,
		       FLASHC_ADDR_FIELD_START_MASK,
		       (sAddr << FLASHC_ADDR_FIELD_START_SHIFT));
	HAL_MODIFY_REG(OPI_MEM_CTRL->FLASH_ADDR[field].END_ADDR,
		       FLASHC_ADDR_FIELD_END_MASK,
		       (eAddr << FLASHC_ADDR_FIELD_END_SHIFT));
	HAL_MODIFY_REG(OPI_MEM_CTRL->FLASH_ADDR[field].BIAS_ADDR,
		       FLASHC_ADDR_FIELD_BIAS_MASK, cache_bias);
	HAL_SET_BIT(OPI_MEM_CTRL->FLASH_ADDR[field].BIAS_ADDR,
		    FLASHC_ADDR_FIELD_BIAS_EN_MASK);
#endif
	return cache_bias;
}

static inline void FC_Enable_32BitAddr_Mode(void)
{
	HAL_SET_BIT(OPI_MEM_CTRL->MEM_COM_CONFG, FLASHC_ADDR_SIZE_MODE_MASK);
}

void FC_Ibus_ReadConfig(uint8_t read_cmd,
                        FC_CycleBits cmd,
                        FC_CycleBits addr,
                        FC_CycleBits dummy,
                        FC_CycleBits data,
                        uint8_t dummy_byte)
{
	uint8_t dummy_width;

	if (dummy_byte > 8)
		dummy_byte = 8;
	dummy_width = dummy_byte * 8;

	HAL_MODIFY_REG(OPI_MEM_CTRL->CBUS_RD_OPRT_CONFG,
	               FLASHC_BUS_RW_CMD_MASK
	               | FLASHC_BUS_CMD_BIT_MASK
	               | FLASHC_BUS_ADDR_BIT_MASK
	               | FLASHC_BUS_DUMY_BIT_MASK
	               | FLASHC_BUS_DUMMY_WID_MASK
	               | FLASHC_BUS_DATA_BIT_MASK,
	               (read_cmd << FLASHC_BUS_RW_CMD_SHIFT)
	               | (cmd << FLASHC_BUS_CMD_BIT_SHIFT)
	               | (addr << FLASHC_BUS_ADDR_BIT_SHIFT)
	               | (dummy << FLASHC_BUS_DUMY_BIT_SHIFT)
	               | (dummy_width << FLASHC_BUS_DUMMY_WID_SHIFT)
	               | (data << FLASHC_BUS_DATA_BIT_SHIFT));

}

void HAL_Flashc_SetCacheLineLen(FC_RdCacheLineLenCfg rlen, FC_WtCacheLneLenCfg wlen)
{
	HAL_MODIFY_REG(OPI_MEM_CTRL->CACHE_RLVT_CONFG,
	               FLASHC_RD_CACHE_LINE_LEN_CONFG_MASK | FLASHC_WT_CACHE_LINE_LEN_CONFG_MASK,
	               rlen | wlen);
}

void FC_Ibus_DummyData(uint32_t dummyh, uint32_t dummyl)
{
	OPI_MEM_CTRL->CBUS_RD_DUMMY_DATA_TOP_HALF = dummyh;
	OPI_MEM_CTRL->CBUS_RD_DUMMY_DATA_BUTT_HALF = dummyl;
}

void FC_Ibus_TransmitDelay(Flash_Ctrl_DelayCycle *delay)
{
	HAL_MODIFY_REG(OPI_MEM_CTRL->MEM_AC_CHR_TIMING_CONFG,
	               FLASHC_CBUS_SHSL_MASK
	               | FLASHC_SPI_FLASH_CHSH_MASK
	               | FLASHC_SPI_FLASH_SLCH_MASK,
	               (delay->cs_begin << FLASHC_SPI_FLASH_SLCH_SHIFT)
	               | (delay->cs_over << FLASHC_SPI_FLASH_CHSH_SHIFT)
	               | (delay->cs_deselect << FLASHC_CBUS_SHSL_SHIFT));

	HAL_MODIFY_REG(OPI_MEM_CTRL->SQPI_CTRL_COM_CONFG,
		       FLASHC_WAIT_HALF_CYCLE_MASK,
		       delay->data << FLASHC_WAIT_HALF_CYCLE_SHIFT);
}

void FC_SetFlash(FC_Cs cs, FC_TCTRL_Fbs fbs, FC_Sclk_Mode mode)
{
	HAL_MODIFY_REG(OPI_MEM_CTRL->SQPI_CTRL_COM_CONFG,
		       FLASHC_FLASH_CS_POL_MASK
		       | FLASHC_FIRST_RCV_BIT_SLT_MASK
		       | FLASHC_SPI_CPOL_CTRL_MASK
		       | FLASHC_SPI_CPHA_CTRL_MASK,
		       cs | fbs | mode);
}

void FC_SetDefaultOuptHigh(void)
{
	HAL_MODIFY_REG(OPI_MEM_CTRL->MEM_COM_CONFG,
			FLASHC_IO1_VACANCY_OUT_MASK
			| FLASHC_IO2_VACANCY_OUT_MASK
			| FLASHC_IO3_VACANCY_OUT_MASK,
			FLASHC_IO1_VACANCY_OUT_HIGH
			| FLASHC_IO2_VACANCY_OUT_HIGH
			| FLASHC_IO3_VACANCY_OUT_HIGH);

}

static inline void FC_CSModeSel(uint32_t cs_mode)
{
	HAL_MODIFY_REG(OPI_MEM_CTRL->SQPI_CTRL_COM_CONFG,
	               FLASHC_CS_OUTPUT_SEL_MASK, cs_mode);
}

static inline void FC_CbusSbusArbiterEn(uint32_t en)
{
	if (en)
		HAL_SET_BIT(OPI_MEM_CTRL->MEM_COM_CONFG, FLASHC_ARBITER_EN_MASK);
	else
		HAL_CLR_BIT(OPI_MEM_CTRL->MEM_COM_CONFG, FLASHC_ARBITER_EN_MASK);
}

void FC_Sbus_ResetFIFO(bool tx, bool rx)
{
	HAL_MODIFY_REG(OPI_MEM_CTRL->MEM_COM_CONFG,
	               FLASHC_REV_FIFO_RESET_MASK | FLASHC_TRAN_FIFO_RESET_MASK,
	               (tx << FLASHC_TRAN_FIFO_RESET_SHIFT) | (rx << FLASHC_REV_FIFO_RESET_SHIFT));
}

void FC_Cbus_RW_Enable(bool enable)
{
	HAL_MODIFY_REG(OPI_MEM_CTRL->MEM_COM_CONFG,
	               FLASHC_CBUS_RW_ENABLE_MASK,
	               enable << FLASHC_CBUS_RW_ENABLE_SHIFT);
}

void FC_Cbus_Clear_Cache_Buffer(void)
{
#define FLASHC_CBUS_CACHE_CLEAR_BUFFER_SHIFT (16)
#define FLASHC_CBUS_CACHE_CLEAR_BUFFER_MASK (0x1U << FLASHC_CBUS_CACHE_CLEAR_BUFFER_SHIFT)
	HAL_MODIFY_REG(OPI_MEM_CTRL->MEM_COM_CONFG,
	               FLASHC_CBUS_CACHE_CLEAR_BUFFER_MASK,
	               1 << FLASHC_CBUS_CACHE_CLEAR_BUFFER_SHIFT);
}

void FC_WrapMode(bool enable)
{
	HAL_MODIFY_REG(OPI_MEM_CTRL->MEM_COM_CONFG, FLASHC_WRAP_AROUND_ENABLE_MASK, enable << FLASHC_WRAP_AROUND_ENABLE_SHIFT);
}

static inline void FC_Sbus_CommandConfig(FC_CycleBits cmd,
        FC_CycleBits addr,
        FC_CycleBits dummy,
        FC_CycleBits data,
        uint8_t dummy_byte)
{
	uint8_t dummy_width;

	if (dummy_byte > 8)
		dummy_byte = 8;
	dummy_width = dummy_byte * 8;

	HAL_MODIFY_REG(OPI_MEM_CTRL->SBUS_RW_OPRT_CONFG,
	               FLASHC_BUS_CMD_BIT_MASK
	               | FLASHC_BUS_ADDR_BIT_MASK
	               | FLASHC_BUS_DUMY_BIT_MASK
	               | FLASHC_BUS_DUMMY_WID_MASK
	               | FLASHC_BUS_DATA_BIT_MASK,
	               (cmd << FLASHC_BUS_CMD_BIT_SHIFT)
	               | (addr << FLASHC_BUS_ADDR_BIT_SHIFT)
	               | (dummy << FLASHC_BUS_DUMY_BIT_SHIFT)
	               | (dummy_width << FLASHC_BUS_DUMMY_WID_SHIFT)
	               | (data << FLASHC_BUS_DATA_BIT_SHIFT));

}

static inline void FC_Sbus_Command(uint8_t cmd, uint32_t addr, uint32_t dummyh, uint32_t dummyl)
{
	HAL_MODIFY_REG(OPI_MEM_CTRL->SBUS_RW_OPRT_CONFG, FLASHC_BUS_RW_CMD_MASK, cmd << FLASHC_BUS_RW_CMD_SHIFT);
	HAL_MODIFY_REG(OPI_MEM_CTRL->SBUS_ADDR_CONFG, FLASHC_SBUS_ADDR_MASK, addr << FLASHC_SBUS_ADDR_SHIFT);
	HAL_MODIFY_REG(OPI_MEM_CTRL->SBUS_DUMMY_DATA_TOP_HALF, FLASHC_SBUS_DUMMY_TOP_HALF_MASK, dummyh << FLASHC_SBUS_DUMMY_TOP_HALF_SHIFT);
	HAL_MODIFY_REG(OPI_MEM_CTRL->SBUS_DUMMY_DATA_BUTT_HALF, FLASHC_SBUS_DUMMY_BUTTOM_HALF_MASK, dummyl << FLASHC_SBUS_DUMMY_BUTTOM_HALF_SHIFT);
}

static inline void FC_Sbus_WriteSize(uint16_t size)
{
	if (size & (~0x1FF))
		FC_ERROR("wr size error");
	size &= 0x1FF;
	HAL_MODIFY_REG(OPI_MEM_CTRL->SBUS_WR_DATA_BYTE_NUM, FLASHC_SBUS_WR_BYTE_MASK, size << FLASHC_SBUS_WR_BYTE_SHIFT);
}

static inline void FC_Sbus_ReadSize(uint32_t size)
{
	HAL_MODIFY_REG(OPI_MEM_CTRL->SBUS_RD_DATA_BYTE_NUM, FLASHC_SBUS_RD_BYTE_MASK, size << FLASHC_SBUS_RD_BYTE_SHIFT);
}

void FC_Sbus_TransmitDelay(Flash_Ctrl_DelayCycle *delay)
{
	HAL_MODIFY_REG(OPI_MEM_CTRL->MEM_AC_CHR_TIMING_CONFG,
	               FLASHC_SPI_FLASH_CHSH_MASK
	               | FLASHC_SPI_FLASH_SLCH_MASK,
	               (delay->cs_begin << FLASHC_SPI_FLASH_SLCH_SHIFT)
	               | (delay->cs_over << FLASHC_SPI_FLASH_CHSH_SHIFT));

	HAL_MODIFY_REG(OPI_MEM_CTRL->MEM_AC_CHR_TIMING_CONFG,
	               FLASHC_SBUS_SHSL_MASK,
	               delay->cs_deselect << FLASHC_SBUS_SHSL_SHIFT);

	HAL_MODIFY_REG(OPI_MEM_CTRL->SQPI_CTRL_COM_CONFG,
	               FLASHC_WAIT_HALF_CYCLE_MASK,
	               delay->data << FLASHC_WAIT_HALF_CYCLE_SHIFT);
}

static inline void FC_Sbus_FIFOTriggerLevel(uint8_t txfull, uint8_t txempty, uint8_t rxfull, uint8_t rxempty)
{
	HAL_MODIFY_REG(OPI_MEM_CTRL->FIFO_TRIGGER_LEVEL,
	               FLASHC_RD_FIFO_EMPTY_REQ_MASK
	               | FLASHC_RD_FIFO_FULL_REQ_MASK
	               | FLASHC_WR_FIFO_EMPTY_REQ_MASK
	               | FLASHC_WR_FIFO_FULL_REQ_MASK,
	               (txfull << FLASHC_WR_FIFO_FULL_REQ_SHIFT)
	               | (txempty << FLASHC_WR_FIFO_EMPTY_REQ_SHIFT)
	               | (rxfull << FLASHC_RD_FIFO_FULL_REQ_SHIFT)
	               | (rxempty << FLASHC_RD_FIFO_EMPTY_REQ_SHIFT));
}


bool FC_Sbus_IsAvailable(int write)
{
	if (write)
		return !!HAL_GET_BIT(OPI_MEM_CTRL->FIFO_STATUS_REG, FLASHC_WR_BUFF_RD_STATUS_SHIFT);
	else
		return !!HAL_GET_BIT(OPI_MEM_CTRL->FIFO_STATUS_REG, FLASHC_RD_BUFF_RD_STATUS_SHIFT);
}

int FC_Sbus_GetBufCnt(int write)
{
	if (write)
		return HAL_GET_BIT_VAL(OPI_MEM_CTRL->FIFO_STATUS_REG, FLASHC_WR_BUFF_COUNTER_SHIFT, FLASHC_WR_BUFF_COUNTER_VMASK);
	else
		return HAL_GET_BIT_VAL(OPI_MEM_CTRL->FIFO_STATUS_REG, FLASHC_RD_BUFF_COUNTER_SHIFT, FLASHC_RD_BUFF_COUNTER_VMASK);
}

static int FC_Sbus_GetFIFOCnt(int write)
{
	if (write)
		return HAL_GET_BIT_VAL(OPI_MEM_CTRL->FIFO_STATUS_REG, FLASHC_WR_FIFO_COUNTER_SHIFT, FLASHC_WR_FIFO_COUNTER_VMASK);
	else
		return HAL_GET_BIT_VAL(OPI_MEM_CTRL->FIFO_STATUS_REG, FLASHC_RD_FIFO_COUNTER_SHIFT, FLASHC_RD_FIFO_COUNTER_VMASK);
}

/*
	Debug State:
	0x0 Idle or Sbus complete;
	0x2 Send CMD;
	0x4 Send Address;
	0x6 Send Dummy;
	0x8 Send Data;
	0x9 Get Data;
	0xc Ibus read complete;
*/
static inline int FC_Sbus_GetDebugState(void)
{
	return HAL_GET_BIT_VAL(OPI_MEM_CTRL->MEM_CTRL_DEBUG_STATE,
	                       FLASHC_MEM_CTRL_STATUE_DEBUG_SHIFT,
	                       FLASHC_MEM_CTRL_STATUE_DEBUG_VMASK);
}

#if (CONFIG_CHIP_ARCH_VER == 3)
void FC_SbusPrepare(void)
{
	HAL_CLR_BIT(OPI_MEM_CTRL->MEM_COM_CONFG, FLASHC_CBUS_RW_ENABLE_BIT);
	//while (OPI_MEM_CTRL->MEM_CTRL_DEBUG_STATE) /* remove for more efficient */
	//	;
	HAL_SET_BIT(OPI_MEM_CTRL->POPI_CTRL_COM_CFG,
	            PSRAMC_CLEAR_CACHE_BUF_BIT | PSRAMC_FORCE_CACHE_TOUT_BIT);
	while (HAL_GET_BIT(OPI_MEM_CTRL->POPI_CTRL_COM_CFG,
	                   PSRAMC_CLEAR_CACHE_BUF_BIT | PSRAMC_FORCE_CACHE_TOUT_BIT))
		;
	HAL_Flashc_CBUS_Dma_Enable(0);
	HAL_MODIFY_REG(OPI_MEM_CTRL->SBUS_RW_OPRT_CONFG,
	               FLASHC_SBUS_DEV_SEL_MASK, FLASHC_SBUS_DEV_SEL_FLASH);
}

void FC_SbusFinish(void)
{
	//while (FC_Sbus_GetDebugState() != 0x00) /* remove for more efficient */
	//	;
	HAL_Flashc_CBUS_Dma_Enable(0);
	HAL_SET_BIT(OPI_MEM_CTRL->MEM_COM_CONFG, FLASHC_CBUS_RW_ENABLE_BIT);
}
#endif

static inline void FC_Sbus_StartSend(void)
{
	HAL_SET_BIT(OPI_MEM_CTRL->SBUS_START_SEND_REG, FLASHC_ENABLE_SBUS_MASK);
}

static inline bool FC_Sbus_isSending(void)
{
	return !!HAL_GET_BIT(OPI_MEM_CTRL->SBUS_START_SEND_REG, FLASHC_ENABLE_SBUS_MASK);
}

typedef enum FC_Sbus_IntType {
	FC_INT_HREADY_TIMEOUT           = 1 << FLASHC_SBUS_HREADY_TIME_OUT_INT_FLAG_SHIFT,
	FC_INT_TC                       = 1 << FLASHC_TRAN_COMPLETED_INT_FLAG_SHIFT,
	FC_INT_WR_FIFO_UNDERFLOW        = 1 << FLASHC_WR_FIFO_UNDERFLOW_INT_FLAG_SHIFT,
	FC_INT_WR_FIFO_OVERFLOW         = 1 << FLASHC_WR_FIFO_OVERFLOW_INT_FLAG_SHIFT,
	FC_INT_RD_FIFO_UNDERFLOW        = 1 << FLASHC_RD_FIFO_UNDERFLOW_INT_FLAG_SHIFT,
	FC_INT_RD_FIFO_OVERFLOW         = 1 << FLASHC_RD_FIFO_OVERFLOW_INT_FLAG_SHIFT,
	FC_INT_WR_FIFO_FULL             = 1 << FLASHC_WR_FIFO_FULL_INT_FLAG_SHIFT,
	FC_INT_WR_FIFO_EMPTY            = 1 << FLASHC_WR_FIFO_EMPTY_INT_FLAG_SHIFT,
	FC_INT_WR_FIFO_READY            = 1 << FLASHC_WR_FIFO_REQ_INT_FLAG_SHIFT,
	FC_INT_RD_FIFO_FULL             = 1 << FLASHC_RD_FIFO_FULL_INT_FLAG_SHIFT,
	FC_INT_RD_FIFO_EMPTY            = 1 << FLASHC_RD_FIFO_EMPTY_INT_FLAG_SHIFT,
	FC_INT_RD_FIFO_READY            = 1 << FLASHC_RD_FIFO_REQ_INT_FLAG_SHIFT,
} FC_Sbus_IntType;

static inline void FC_Sbus_ClrStatus(FC_Sbus_IntType type)
{
	HAL_CLR_BIT(OPI_MEM_CTRL->INT_STATUS_REG, type);
}

static inline void FC_Sbus_Write(uint8_t data)
{
	*((uint8_t *)&OPI_MEM_CTRL->SBUS_WR_DATA_REG) = data;
}

static inline uint8_t FC_Sbus_Read(void)
{
	/* return 0; //debug, to see if fifo not empty */
	return *((uint8_t *)&OPI_MEM_CTRL->SBUS_RD_DATA_REG);
}

static flashc_caback _flashc_caback;

void HAL_Flashc_RegisterCb(flashc_caback cb)
{
	_flashc_caback = cb;
}

/*
 * @brief
 */
void HAL_Flashc_EnableCCMU(struct flash_controller *ctrl)
{
	hal_clk_status_t ret;

	if (ctrl->ccmu_on++ != 0)
		return;

	FC_DEBUG("Enable CCMU");
	hal_clk_type_t clk_type = HAL_SUNXI_CCU;
	hal_clk_id_t clk_id = CLK_FLASH_SPI;
	hal_clk_t mclk = hal_clock_get(clk_type, clk_id);
	ret = hal_clock_enable(mclk);
	if (ret) {
		printf("couldn't enable mlck! return %d\n", ret);
	}
	/*FC_Reg_CLK();*/
}

/*
 * @brief
 */
void HAL_Flashc_DisableCCMU(struct flash_controller *ctrl)
{
	hal_clk_status_t ret;

	if (--ctrl->ccmu_on != 0)
		return;

	FC_DEBUG("DISABLE CCMU");
	hal_clk_type_t clk_type = HAL_SUNXI_CCU;
	hal_clk_id_t clk_id = CLK_FLASH_SPI;
	hal_clk_t mclk = hal_clock_get(clk_type, clk_id);
	ret = hal_clock_disable(mclk);
	if (ret) {
		printf("couldn't disable mlck! return %d\n", ret);
	}

	/*FC_Reg_CLK();*/
}

void HAL_Flashc_ResetCCMU(void)
{
	hal_clk_status_t ret;
	hal_reset_type_t reset_type = HAL_SUNXI_RESET;
	u32 reset_id = RST_FLASH_CTRL;
	struct reset_control *reset;

	reset = hal_reset_control_get(reset_type, reset_id);
	hal_reset_control_deassert(reset);
	hal_reset_control_put(reset);

	reset_type = HAL_SUNXI_RESET;
	reset_id = RST_FLASH_ENC;

	reset = hal_reset_control_get(reset_type, reset_id);
	hal_reset_control_deassert(reset);
	hal_reset_control_put(reset);
}

/*
 * @brief
 */
bool HAL_Flashc_ConfigCCMU(uint32_t clk)
{
	hal_clk_status_t ret;
	hal_reset_type_t reset_type = HAL_SUNXI_RESET;
	u32 reset_id = RST_FLASH_CTRL;
	hal_clk_type_t clk_type = HAL_SUNXI_CCU;
	hal_clk_id_t clk_id = CLK_FLASH_SPI;
	struct reset_control *reset;

	/*flashctrl rst*/
	reset = hal_reset_control_get(reset_type, reset_id);
	hal_reset_control_deassert(reset);
	hal_reset_control_put(reset);


	/*flashenc rst*/
	reset_type = HAL_SUNXI_RESET;
	reset_id = RST_FLASH_ENC;

	reset = hal_reset_control_get(reset_type, reset_id);
	hal_reset_control_deassert(reset);
	hal_reset_control_put(reset);

	/*clk*/
	hal_clk_type_t clk_type_pclk = HAL_SUNXI_AON_CCU;
	//hal_clk_type_t clk_type_pclk = HAL_SUNXI_CCU;

	hal_clk_t mclk = hal_clock_get(clk_type, clk_id);
	//hal_clk_t bus_clk = hal_clock_get(clk_type, clk_bus_id);
	hal_clk_t pclk = hal_clock_get(clk_type_pclk, CLK_DEVICE);

	ret = hal_clk_set_parent(mclk, pclk);
	if (ret) {
		printf("clk set parent failed! return %d\n", ret);
		return -1;
	}

	uint32_t rate = clk;
	ret = hal_clk_set_rate(mclk, rate);
	if (ret) {
		printf("clk set rate failed! return %d\n", ret);
		return -1;
	}

	rate = hal_clk_get_rate(mclk);
	if (!rate) {
		printf("clk get rate failed! return %ld\n", rate);
		return -1;
	}
	/*
	ret = hal_clock_enable(mclk);
	if (ret) {
		printf("couldn't enable mlck! return %d\n", ret);
		return -1;
	}
	*/
	return 1;
}

void HAL_Flashc_PinInit(struct flash_controller *ctrl)
{
	if (ctrl->pin_inited++ == 0) {
		if (nor_sip()) {
			/* printf("init pb8-pb13 for sip nor, in fpga j6\n"); */
			//FIXME : when func not flashc, don't change it
			gpio_muxsel_t func;
			if (hal_gpio_pinmux_get_function(GPIOB(4), &func) && (func == GPIO_MUXSEL_FUNCTION5))
				hal_gpio_pinmux_set_function(GPIOB(4), 0xf);
			if (hal_gpio_pinmux_get_function(GPIOB(5), &func) && (func == GPIO_MUXSEL_FUNCTION5))
				hal_gpio_pinmux_set_function(GPIOB(5), 0xf);
			if (hal_gpio_pinmux_get_function(GPIOB(6), &func) && (func == GPIO_MUXSEL_FUNCTION5))
				hal_gpio_pinmux_set_function(GPIOB(6), 0xf);
			if (hal_gpio_pinmux_get_function(GPIOB(15), &func) && (func == GPIO_MUXSEL_FUNCTION5))
				hal_gpio_pinmux_set_function(GPIOB(15), 0xf);
			for (int i = 0; i < FLASH_MSG_NUM; i++) {
				hal_gpio_pinmux_set_function(flash_nor_sip_msg[i].flash_pin, flash_nor_sip_msg[i].flash_function);
				hal_gpio_set_driving_level(flash_nor_sip_msg[i].flash_pin, flash_nor_sip_msg[i].flash_drv_level);
				hal_gpio_set_pull(flash_nor_sip_msg[i].flash_pin, flash_nor_sip_msg[i].flash_pull_state);
			}
		} else {
			//for bitfile before 0501, use pb4-pb7
			//for bitfile after 0501, use pb2, pb3, pb6, pb7
			//for bitfile after 0608, use pb4, pb5, pb6, pb15
			/* printf("init pb2-pb7 for nor, in fpga j20\n"); */
			for (int i = 0; i < FLASH_MSG_NUM; i++) {
				hal_gpio_pinmux_set_function(flash_no_nor_sip_msg[i].flash_pin, flash_no_nor_sip_msg[i].flash_function);
				hal_gpio_set_driving_level(flash_no_nor_sip_msg[i].flash_pin, flash_no_nor_sip_msg[i].flash_drv_level);
				hal_gpio_set_pull(flash_no_nor_sip_msg[i].flash_pin, flash_no_nor_sip_msg[i].flash_pull_state);
			}
		}
		//HAL_BoardIoctl(HAL_BIR_PINMUX_INIT, HAL_MKDEV(HAL_DEV_MAJOR_FLASHC, 0), 0);
	}
}

void HAL_Flashc_PinDeinit(struct flash_controller *ctrl)
{
	if ((ctrl->pin_inited > 0) && (--ctrl->pin_inited == 0)) {
		//close io
		//HAL_BoardIoctl(HAL_BIR_PINMUX_DEINIT, HAL_MKDEV(HAL_DEV_MAJOR_FLASHC, 0), 0);
		if (nor_sip()) {
			for (int i = 0; i < FLASH_MSG_NUM; i++) {
				hal_gpio_pinmux_set_function(flash_nor_sip_msg[i].flash_pin, GPIO_MUXSEL_DISABLED);
				if (!flash_nor_sip_msg[i].keep_pull_state_flags)
					hal_gpio_set_pull(flash_nor_sip_msg[i].flash_pin, GPIO_PULL_DOWN_DISABLED);
			}
		} else {
			for (int i = 0; i < FLASH_MSG_NUM; i++) {
				hal_gpio_pinmux_set_function(flash_no_nor_sip_msg[i].flash_pin, GPIO_MUXSEL_DISABLED);
				if (!flash_no_nor_sip_msg[i].keep_pull_state_flags)
					hal_gpio_set_pull(flash_no_nor_sip_msg[i].flash_pin, GPIO_PULL_DOWN_DISABLED);
			}
		}
	}
}

/**
  * @brief Initialize Flash controller IBUS driver (XIP).
  * @param cfg:
  *        @arg cfg->addr: Started address of XIP code in Flash.
  *        @arg cfg->freq: Flash working frequency.
  *        @arg cfg->delay: Delay of hardware.
  *        @arg cfg->ins: Instruction of XIP reading
  *        @arg cfg->cont_mode: Enable continue mode in reading or not.
  * @retval HAL_Status: The status of driver.
  */
HAL_Status HAL_Flashc_Xip_Init(struct flash_controller *ctrl, XIP_Config *cfg)
{
	Flash_Ctrl_DelayCycle delay = {0};

	//HAL_ASSERT_PARAM(!(FLASH_XIP_START_ADDR & 0x0F));
	//HAL_ASSERT_PARAM(FLASH_XIP_END_ADDR > FLASH_XIP_START_ADDR);

	if (!ctrl->suspending)
		memcpy(&ctrl->pm_ibus_cfg, cfg, sizeof(XIP_Config));

	/* enable ccmu */
	HAL_Flashc_EnableCCMU(ctrl);

	/* open io */
	HAL_Flashc_PinInit(ctrl);

	HAL_CLR_BIT(OPI_MEM_CTRL->MEM_COM_CONFG, FLASHC_XIP_ENABLE_BIT | FLASHC_CBUS_RW_ENABLE_BIT);

	/* config flash controller */
	FC_GetDelayCycle(&delay, cfg->freq);
	FC_Ibus_TransmitDelay(&delay);

	FC_SetFlash(FC_TCTRL_CS_LOW_ENABLE, FC_TCTRL_FBS_MSB, FC_SCLK_Mode0);

	FC_Ibus_ReadConfig(cfg->ins.cmd,
	                   cfg->ins.cmd_line,
	                   cfg->ins.addr_line,
	                   cfg->ins.dummy_line,
	                   cfg->ins.data_line,
	                   cfg->ins.dum_btyes);
	if (cfg->cont_mode) {
		//FC_Ibus_DummyData(0xA5000000, 0);
		FC_Ibus_DummyData(cfg->ins.cont_dummyh, cfg->ins.cont_dummyl);
		HAL_SET_BIT(OPI_MEM_CTRL->MEM_COM_CONFG, FLASHC_XIP_ENABLE_BIT | FLASHC_CBUS_RW_ENABLE_BIT);
		ctrl->xip_continue = FLASHC_XIP_ENABLE_BIT;
	} else {
		//FC_WrapMode(1);
		//FC_Ibus_DummyData(0, 0);
		FC_Ibus_DummyData(cfg->ins.ex_cont_dummyh, cfg->ins.ex_cont_dummyl);
		HAL_SET_BIT(OPI_MEM_CTRL->MEM_COM_CONFG, FLASHC_CBUS_RW_ENABLE_BIT);
	}

	ctrl->xip_on = 1;

	FC_Cbus_Clear_Cache_Buffer();
	/* config flash cache */
	FlashCBUS_AddrRequire(FLASH_ADDR_XIP_IDX, FLASH_XIP_START_ADDR, cfg->addr,
	                      FLASH_XIP_END_ADDR - FLASH_XIP_START_ADDR);
	FC_DEBUG("xip start: 0x%0x, len: 0x%0x\n", cfg->addr, FLASH_XIP_END_ADDR - FLASH_XIP_START_ADDR);
	FC_DEBUG("ccmu : %d", ctrl->ccmu_on);
	FD_INFO("xip enable: %d, continue enable: %d\n", 1, cfg->cont_mode);
	//Regs_Print();

	return HAL_OK;
}

/**
  * @brief Deinitialize Flash controller IBUS (XIP).
  * @param None
  * @retval HAL_Status: The status of driver.
  */
HAL_Status HAL_Flashc_Xip_Deinit(struct flash_controller *ctrl)
{
	unsigned long flags = HAL_EnterCriticalSection();

	//config flash controller
	HAL_CLR_BIT(OPI_MEM_CTRL->MEM_COM_CONFG, FLASHC_XIP_ENABLE_BIT | FLASHC_CBUS_RW_ENABLE_BIT);
	ctrl->xip_on = 0;
	HAL_ExitCriticalSection(flags);

	//close io
	HAL_Flashc_PinDeinit(ctrl);

	//disable ccmu
	HAL_Flashc_DisableCCMU(ctrl);

	return HAL_OK;
}

/**
  * @internal
  * @brief Flash controller IBUS (XIP) Enable without Pin initialization.
  * @note Most for Flash controller SBUS. It will resume system schedule.
  * @param None
  * @retval None
  */
void HAL_Flashc_Xip_RawEnable(struct flash_controller *ctrl)
{
	if (!ctrl->xip_on)
		return;

	if (ctrl->optimize_mask & (FLASH_OPTIMIZE_WRITE | FLASH_OPTIMIZE_ERASE)) {
		HAL_ExitCriticalSection(ctrl->irqsaveflag);
		__ISB();
	} else {
		/* if irq disable, do not resume scheduler in case of system error */
		if (!HAL_IsIRQDisabled())
			HAL_ThreadResumeScheduler();
	}
}

/**
  * @internal
  * @brief Flash controller IBUS (XIP) Enable without Pin deinitialization.
  * @note Most for Flash controller SBUS. It will suspend system schedule.
  * @param None
  * @retval None
  */
void HAL_Flashc_Xip_RawDisable(struct flash_controller *ctrl)
{
	if (!ctrl->xip_on)
		return;

	if (ctrl->optimize_mask & (FLASH_OPTIMIZE_WRITE | FLASH_OPTIMIZE_ERASE)) {
		ctrl->irqsaveflag = HAL_EnterCriticalSection();
	} else {
		/* if irq disable, do not suspend scheduler in case of system error */
		if (!HAL_IsIRQDisabled())
			HAL_ThreadSuspendScheduler();
	}

}

#ifdef CONFIG_PM_FLASHC

int flashc_suspend(struct pm_device *dev, suspend_mode_t state)
{
	struct flash_controller *ctrl = dev->data;

	/*
		suspend condition:
			(1) not in sbus opened state
			(2)
	*/
	FC_DEBUG("flashc_suspend\n");
	ctrl->suspending = 1;

	if (ctrl->sbusing)
		return -1;

	if (!ctrl->xip_on) {
		//HAL_CCM_BusEnablePeriphClock(CCM_BUS_PERIPH_BIT_FLASH_CTRL);
		//HAL_CCM_FLASHC_EnableMClock();
		HAL_Flashc_EnableCCMU(ctrl);
	}

	while (FC_Sbus_GetDebugState() != 0x00);

	if (!ctrl->xip_on) {
		//HAL_CCM_BusDisablePeriphClock(CCM_BUS_PERIPH_BIT_FLASH_CTRL);
		//HAL_CCM_FLASHC_DisableMClock();
		HAL_Flashc_DisableCCMU(ctrl);
	}

	switch (state) {
	case PM_MODE_SLEEP:
		if (ctrl->xip_on) {
			HAL_CLR_BIT(OPI_MEM_CTRL->MEM_COM_CONFG, ctrl->xip_continue);
			//HAL_CCM_BusDisablePeriphClock(CCM_BUS_PERIPH_BIT_FLASH_CTRL);
			//HAL_CCM_FLASHC_DisableMClock();
			HAL_Flashc_DisableCCMU(ctrl);
		}
		break;
	case PM_MODE_STANDBY:
	case PM_MODE_HIBERNATION:
		if (ctrl->xip_on) {
			HAL_Flashc_Xip_Deinit(ctrl);
			FC_DEBUG("ccmu : %d", ctrl->ccmu_on);
			ctrl->pm_xip = 1;
		}
		HAL_Flashc_Deinit(ctrl);
		FC_DEBUG("ccmu : %d", ctrl->ccmu_on);
		break;
	default:
		break;
	}

	//FC_Reg_All();
	return 0;
}

int flashc_resume(struct pm_device *dev, suspend_mode_t state)
{
	struct flash_controller *ctrl = dev->data;

	//FC_Reg_All();
	FC_DEBUG("flashc_resume\n");

	switch (state) {
	case PM_MODE_SLEEP:
		if (ctrl->xip_on) {
			//HAL_CCM_BusEnablePeriphClock(CCM_BUS_PERIPH_BIT_FLASH_CTRL);
			//HAL_CCM_FLASHC_EnableMClock();
			HAL_Flashc_EnableCCMU(ctrl);
			HAL_SET_BIT(OPI_MEM_CTRL->MEM_COM_CONFG, ctrl->xip_continue);
		}
		break;
	case PM_MODE_STANDBY:
	case PM_MODE_HIBERNATION:
		HAL_Flashc_Init(ctrl, &ctrl->pm_sbus_cfg);
		if (ctrl->pm_xip) {
			ctrl->pm_xip = 0;
			HAL_Flashc_Xip_Init(ctrl, &ctrl->pm_ibus_cfg);
			hal_udelay(300);
		}
		FC_DEBUG("ccmu: %d, pin: %d", ctrl->ccmu_on, ctrl->pin_inited);
		break;
	default:
		break;
	}

	ctrl->suspending = 0;

	return 0;
}
#endif /* CONFIG_PM */

/**
  * @brief Delay realization in Flash controller IBUS (XIP).
  * @note Delay can be system sleep while it's not in XIP, but must be a while
  *       delay without system interface while it's in XIP.
  * @param us: delay time in microsecond.
  * @retval None
  */
void HAL_Flashc_Delay(struct flash_controller *ctrl, unsigned int us)
{
	if (us == 0)
		return;
	if (ctrl->xip_on || !HAL_ThreadIsSchedulerRunning()) {
		hal_udelay(us);
	} else {
		hal_msleep((us + 1023) >> 10);
	}
}

/**
 * @brief Initialize Flash controller SBUS.
 * @param cfg:
 * 	   @arg cfg->freq: Flash working frequency.
 * @retval HAL_Status: The status of driver.
 */
HAL_Status HAL_Flashc_Init(struct flash_controller *ctrl, const Flashc_Config *cfg)
{
	//this one
	Flash_Ctrl_DelayCycle delay = {0};

	if (_flashc_caback)
		_flashc_caback(ctrl, FC_INIT, 0);

	/* enable ccmu */
	HAL_Flashc_ResetCCMU();
	HAL_Flashc_ConfigCCMU(cfg->freq);
	HAL_Flashc_EnableCCMU(ctrl);
	/* config flash controller */
	FC_GetDelayCycle(&delay, cfg->freq);
	FC_Sbus_TransmitDelay(&delay);

	FC_SetFlash(FC_TCTRL_CS_LOW_ENABLE, FC_TCTRL_FBS_MSB, FC_SCLK_Mode0);

	/* FC_CSModeSel(cfg->cs_mode); */  //r128 no cs mode
	/* ctrl->cs_mode = cfg->cs_mode; */
	FC_CbusSbusArbiterEn(1);

	if (ctrl->externAddr_on == 1) {
		FC_Enable_32BitAddr_Mode();
	}
	FC_Sbus_ResetFIFO(1, 1);
	HAL_Flashc_SetCacheLineLen(FC_RD_CACHE_LINE_LEN_32B, FC_WT_CACHE_LINE_LEN_32B);

	FC_DEBUG("ccmu : %d", ctrl->ccmu_on);
	//Regs_Print();
	FC_SetDefaultOuptHigh();
	HAL_Flashc_DisableCCMU(ctrl);
	if (_flashc_caback)
		_flashc_caback(ctrl, FC_INIT, 1);
#ifdef CONFIG_PM_FLASHC
	if (!ctrl->suspending) {
		memset(&ctrl->flashc_drv, 0, sizeof(struct pm_devops));
		ctrl->flashc_drv.suspend_noirq = flashc_suspend;
		ctrl->flashc_drv.resume_noirq = flashc_resume;
		memset(&ctrl->flashc_dev, 0, sizeof(struct pm_device));
		ctrl->flashc_dev.name = "flashc";
		ctrl->flashc_dev.ops = &ctrl->flashc_drv;
		ctrl->flashc_dev.data = ctrl;
		ctrl->pm_xip = 0;
		memcpy(&ctrl->pm_sbus_cfg, cfg, sizeof(Flashc_Config));
		pm_devops_register(&ctrl->flashc_dev);
	}
#endif

	return HAL_OK;
}

/**
* @brief Deinitialize Flash controller SBUS.
* @param None
* @retval HAL_Status: The status of driver.
*/
HAL_Status HAL_Flashc_Deinit(struct flash_controller *ctrl)
{
#ifdef CONFIG_PM_FLASHC
	if (!ctrl->suspending) {
		pm_devops_unregister(&ctrl->flashc_dev);
		ctrl->flashc_dev.data = NULL;
	}
#endif
	if (_flashc_caback)
		_flashc_caback(ctrl, FC_DEINIT, 0);

	return HAL_OK;
}

/**
 * @brief Open flash controller SBUS.
 * @note At the same time, it will disable XIP and suspend schedule.
 * @param None
 * @retval HAL_Status: The status of driver.
 */
HAL_Status HAL_Flashc_Open(struct flash_controller *ctrl)
{
	ctrl->sbusing = 1;
	if (ctrl->xip_on) {
		if (ctrl->xip_continue) {
			struct FlashDev *dev = getFlashDev(0);
			uint8_t data;
			//FC_Ibus_DummyData(0, 0);
			FC_Ibus_DummyData(ctrl->pm_ibus_cfg.ins.ex_cont_dummyh, ctrl->pm_ibus_cfg.ins.ex_cont_dummyl);
			HAL_CLR_BIT(OPI_MEM_CTRL->MEM_COM_CONFG, FLASHC_XIP_ENABLE_BIT);
			dev->chip->read(dev->chip, dev->rmode, 0, &data, 1);
			dev->chip->continuousReadReset(dev->chip);
		}
		HAL_Flashc_Xip_RawDisable(ctrl);
		FC_Cbus_Clear_Cache_Buffer();
#if (CONFIG_CHIP_ARCH_VER == 3)
		FC_SbusPrepare();
#endif
	}

	if (ctrl->resetMask) {
		ctrl->suspending = 1;

		if (ctrl->xip_on) {
			HAL_Flashc_Xip_Deinit(ctrl);

			FC_DEBUG("ccmu : %d", ctrl->ccmu_on);
			ctrl->xip_on = 1;
		}

		if (_flashc_caback)
			_flashc_caback(ctrl, FC_OPEN, 0);
		HAL_Flashc_Deinit(ctrl);
		HAL_Flashc_Init(ctrl, &ctrl->pm_sbus_cfg);
		if (_flashc_caback)
			_flashc_caback(ctrl, FC_OPEN, 1);
		FC_DEBUG("ccmu : %d", ctrl->ccmu_on);
	}

	HAL_Flashc_EnableCCMU(ctrl);
	HAL_Flashc_PinInit(ctrl);
	FC_Sbus_ResetFIFO(1, 1);

	return HAL_OK;
}

/**
 * @brief Close flash controller SBUS.
 * @param None
 * @retval HAL_Status: The status of driver.
 */
HAL_Status HAL_Flashc_Close(struct flash_controller *ctrl)
{
	HAL_Flashc_PinDeinit(ctrl);
	HAL_Flashc_DisableCCMU(ctrl);
	if (ctrl->resetMask) {
		if (_flashc_caback)
			_flashc_caback(ctrl, FC_CLOSE, 0);
		HAL_Flashc_Deinit(ctrl);
		HAL_Flashc_Init(ctrl, &ctrl->pm_sbus_cfg);
		if (ctrl->xip_on) {
			HAL_Flashc_Xip_Init(ctrl, &ctrl->pm_ibus_cfg);
			hal_udelay(300);
		}
		if (_flashc_caback)
			_flashc_caback(ctrl, FC_CLOSE, 1);
		FC_DEBUG("ccmu: %d, pin: %d", ctrl->ccmu_on, ctrl->pin_inited);
		ctrl->suspending = 0;
	}

	if (ctrl->xip_on) {
#if (CONFIG_CHIP_ARCH_VER == 3)
		FC_SbusFinish();
#endif
		HAL_Flashc_Xip_RawEnable(ctrl);
		if (ctrl->xip_continue) {
			//FC_Ibus_DummyData(0xA5000000, 0);
			FC_Ibus_DummyData(ctrl->pm_ibus_cfg.ins.cont_dummyh, ctrl->pm_ibus_cfg.ins.cont_dummyl);
			HAL_SET_BIT(OPI_MEM_CTRL->MEM_COM_CONFG, FLASHC_XIP_ENABLE_BIT | FLASHC_CBUS_RW_ENABLE_BIT);
		}
	}
	ctrl->sbusing = 0;
	return HAL_OK;
}

/**
 * @brief Flash controller ioctl.
 * @note op : arg
 *       nothing support for now.
 * @param op: ioctl command.
 * @param arg: ioctl arguement
 * @retval HAL_Status: The status of driver.
 */
HAL_Status HAL_Flashc_Ioctl(struct flash_controller *ctrl, uint32_t op, void *arg)
{
	switch (op) {
	case FC_CMD_ENABLE_32BITADDR_MODE:
		FC_Enable_32BitAddr_Mode();
		break;
	case FC_CMD_CONFIG_OPTIMIZE:
		if (ctrl->cacheWriteThroughIdx == -1) {
			//ctrl->cacheWriteThroughIdx = HAL_Dcache_Request_WriteThroughIndex();
			//HAL_Dcache_Config_WriteThrough(ctrl->cacheWriteThroughIdx, FLASH_XIP_USER_START_ADDR, FLASH_XIP_USER_END_ADDR);
			ctrl->cacheWriteThroughIdx = 0;
		}
		if (ctrl->cacheWriteThroughIdx >= 0) {
			ctrl->optimize_mask = FLASH_OPTIMIZE_READ | FLASH_OPTIMIZE_WRITE | FLASH_OPTIMIZE_ERASE;
		} else {
			ctrl->optimize_mask = FLASH_OPTIMIZE_WRITE | FLASH_OPTIMIZE_ERASE;
			FC_WARN("alloc write through faild, use normal read!");
		}
		ctrl->optimize_mask &= *(uint8_t *)arg;
		break;
	case FC_CMD_CONFIG_RESET_MASK:
		ctrl->resetMask = *(uint8_t *)arg;
		break;
	default:
		break;
	}
	return HAL_INVALID;
}

static void HAL_Flashc_DMARelease(void *arg)
{
    hal_sem_post(((struct flash_controller *)arg)->dmaSem);
}

#if (CONFIG_CHIP_ARCH_VER == 3)
void HAL_Flashc_CBUS_Dma_Enable(uint32_t en)
{
	uint32_t rval;

	if (en)
		rval = FLASHC_READ_DMA_EN;
	else
		rval = 0;
	HAL_MODIFY_REG(OPI_MEM_CTRL->CBUS_RD_OPRT_CONFG, FLASHC_READ_DMA_EN, rval);
}
#endif

static HAL_Status HAL_Flashc_DMATransfer(struct flash_controller *ctrl, int write, uint8_t *data, uint32_t size)
{
	HAL_Status ret = HAL_OK;
	int i;
	struct sunxi_dma_chan *hdma = NULL;
	struct dma_slave_config config = {0};
	uint32_t size_len = 0;
	if (size == 0 || data == NULL)
		return HAL_ERROR;
	/* request dma chan */
	ret = hal_dma_chan_request(&hdma);
	if (ret == HAL_DMA_CHAN_STATUS_BUSY) {
		hal_log_err("dma channel busy!");
		goto end;
	}

	/* register dma callback */
	ret = hal_dma_callback_install(hdma, dma_cb, ctrl);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("register dma callback failed!");
		goto end;
	}

	hal_dcache_clean_invalidate((unsigned long)data, size);

    ctrl->dmaSem = hal_sem_create(0);
	if (write) {
		config.direction = DMA_MEM_TO_DEV;
		config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		config.dst_maxburst = DMA_SLAVE_BURST_1;
		config.src_maxburst = DMA_SLAVE_BURST_4;
		config.slave_id = sunxi_slave_id(DRQDST_FLASH_CTRL, DRQSRC_SDRAM);

		ret = hal_dma_slave_config(hdma, &config);

		if (ret != HAL_DMA_STATUS_OK) {
			hal_log_err("dma config error, ret:%d", ret);
			goto end;
		}
	} else {
		config.direction = DMA_DEV_TO_MEM;
		config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		config.dst_maxburst = DMA_SLAVE_BURST_4;
		config.src_maxburst = DMA_SLAVE_BURST_1;
		config.slave_id = sunxi_slave_id(DRQDST_SDRAM, DRQSRC_FLASH_CTRL);

		ret = hal_dma_slave_config(hdma, &config);
		if (ret != HAL_DMA_STATUS_OK) {
			hal_log_err("dma config error, ret:%d", ret);
			goto end;
		}
	}

	if (write)
		ret = hal_dma_prep_device(hdma, (uintptr_t)&OPI_MEM_CTRL->SBUS_WR_DATA_REG, (uintptr_t)data, size, DMA_MEM_TO_DEV);
	else
		ret = hal_dma_prep_device(hdma, (uintptr_t)data, (uintptr_t)&OPI_MEM_CTRL->SBUS_RD_DATA_REG, size, DMA_DEV_TO_MEM);

	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma prep error, ret:%d", ret);
		goto end;
	}

	FC_Sbus_StartSend();
	ret = hal_dma_start(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma start error, ret:%d", ret);
		goto end;
	}

	if ((ret = hal_sem_wait(ctrl->dmaSem)) != HAL_OK)
		FC_ERROR("sem wait failed: %d", ret);

	FC_WHILE_TIMEOUT(FC_Sbus_isSending(), 0x3FFFFFF);
	FC_Sbus_ClrStatus(FC_INT_TC);
	ret = hal_dma_stop(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma stop error, ret:%d", ret);
		goto end;
	}

	ret = hal_dma_chan_free(hdma);
	if (ret != HAL_DMA_STATUS_OK) {
		hal_log_err("dma free error, ret:%d", ret);
		goto end;
	}

	if (!write) {
		hal_dcache_invalidate((unsigned long)data, size);
	}
	hal_sem_delete(ctrl->dmaSem);

	if (FC_DebugCheck(0))
		return HAL_ERROR;

failed:
end:
	return ret;
}

static HAL_Status HAL_Flashc_PollTransfer(struct flash_controller *ctrl, int write, uint8_t *data, uint32_t size)
{
	FC_Sbus_StartSend();

	if (write) {
		while (size--) {
			FC_WHILE_TIMEOUT(FC_Sbus_GetFIFOCnt(write) > 100, 0x3FFFFFF);
			FC_Sbus_Write(*(data++));
		}
	} else {
		while (size--) {
			FC_WHILE_TIMEOUT(FC_Sbus_GetFIFOCnt(write) == 0, 0x3FFFFFF);
			*(data++) = FC_Sbus_Read();
		}
	}

//	FC_WHILE_TIMEOUT(FC_Sbus_GetDebugState() != 0, 0x3FFFFFF);
	FC_WHILE_TIMEOUT(FC_Sbus_isSending(), 0x3FFFFFF);
	FC_Sbus_ClrStatus(FC_INT_TC);

	if (FC_DebugCheck(0))
		return HAL_ERROR;

	return HAL_OK;
}

/**
 * @brief Write or read flash by flash controller SBUS.
 * @note Send a instruction in command + address + dummy + write or read data.
 * @param cmd: Command of instruction.
 *        @arg cmd->pdata: The data is filled with in this field.
 *        @arg cmd->len: The data len of this field.
 *        @arg cmd->line: The number of line transfering this field data.
 * @param addr: Address of instruction
 * @param dummy: Dummy of instruction
 * @param data: Data of instruction
 * @param dma: Transfer data by DMA or not.
 * @retval HAL_Status: The status of driver.
 */
#ifdef FLASH_XIP_OPT_READ
static uint32_t _fc_icache_startAddr;
static uint32_t _fc_icache_endAddr;
#endif

HAL_Status HAL_Flashc_Transfer(struct flash_controller *ctrl, int write,
                                   FC_InstructionField *cmd, FC_InstructionField *addr,
                                   FC_InstructionField *dummy, FC_InstructionField *data, bool dma)
{
	HAL_Status ret;
	uint8_t zero_data = 0;
	FC_InstructionField zero;
	uint8_t fcrypto_key[16];

	memset(&zero, 0, sizeof(zero));
	zero.pdata = &zero_data;

	/* instruction check */
	if (cmd == NULL)
		cmd = &zero;
	if (addr == NULL)
		addr = &zero;
	if (dummy == NULL)
		dummy = &zero;
	if (data == NULL)
		data = &zero;
#ifdef FLASH_XIP_OPT_READ
	if (ctrl->xip_on && !write && (ctrl->optimize_mask & FLASH_OPTIMIZE_READ) && addr->pdata != NULL) { /*cbus*/
		uint32_t icache_bias;
		uint32_t icache_src;
		HAL_ASSERT_PARAM(!(FLASH_XIP_USER_START_ADDR & 0x0F));
		_fc_icache_startAddr = _fc_icache_endAddr;
		if (*((uint32_t *)addr->pdata) > FLASH_XIP_USER_START_ADDR - FLASH_XIP_START_ADDR) {
			_fc_icache_startAddr = FLASH_XIP_START_ADDR + *((uint32_t *)addr->pdata);
		} else {
			if (!_fc_icache_startAddr ||
				((_fc_icache_startAddr + data->len + (2 * FLASH_XIP_USER_ADDR_STEP + 1)) >= FLASH_XIP_USER_END_ADDR)) {
				_fc_icache_startAddr = FLASH_XIP_USER_START_ADDR;
			}
		}
		icache_bias = FlashCBUS_AddrRequire(FLASH_ADDR_USER_RD_IDX, _fc_icache_startAddr,
		                                    *((uint32_t *)addr->pdata), data->len);
		if (icache_bias == -1) {
			return HAL_ERROR;
		}
		icache_src = _fc_icache_startAddr + (*((uint32_t *)addr->pdata)) - icache_bias;
		_fc_icache_endAddr = roundup2(icache_src + data->len, FLASH_XIP_USER_ADDR_STEP);
		hal_dcache_invalidate(icache_src, data->len);
		/* HAL_Dcache_Config_WriteThrough(ctrl->cacheWriteThroughIdx, _fc_icache_startAddr, _fc_icache_endAddr); */
#ifndef FLASH_SKIP_CRYPTO
#if (CONFIG_CHIP_ARCH_VER == 3)
		if (FC_Get_Crypto_Infor(fcrypto_key)) {
			FlashCryptoRequest(_fc_icache_startAddr, _fc_icache_endAddr - 1, fcrypto_key);
		}
#endif
#endif
		memcpy(data->pdata, (uint8_t *)((uintptr_t)(icache_src)), data->len);

		ret = HAL_OK;
	} else
#endif
	{ /*sbus*/
#ifndef FLASH_SKIP_CRYPTO
#if (CONFIG_CHIP_ARCH_VER == 3)
		if (FC_Get_Crypto_Infor(fcrypto_key)) {
			if (addr->pdata) {
				FlashCryptoRequest(*((uint32_t *)addr->pdata),
				   rounddown2(*((uint32_t *)addr->pdata) + data->len, 16) - 1, fcrypto_key);
			}
		}
#endif
#endif
		FC_Sbus_ResetFIFO(1, 1);
		FC_Sbus_CommandConfig(cmd->line, addr->line, dummy->line, data->line, dummy->len);
		if (addr->pdata) {
			FC_Sbus_Command(*(cmd->pdata), *((uint32_t *)addr->pdata), 0, 0);
		}
		else
			FC_Sbus_Command(*(cmd->pdata), 0, 0, 0);
		//FC_Reg_All();
		if (write) {
			FC_Sbus_WriteSize(data->len);
		}
		else {
			FC_Sbus_ReadSize(data->len);
		}
#if 0
		if (dma == 1 && data->len != 0 && !ctrl->xip_on && !hal_interrupt_get_nest()
			&& (HAL_DMA_FlashSbus_Iscoexist())) {
			ret = HAL_Flashc_DMATransfer(ctrl, write, data->pdata, data->len);
		} else {
			ret = HAL_Flashc_PollTransfer(ctrl, write, data->pdata, data->len);
		}
#else
#ifndef CONFIG_FLASHC_CPU_XFER_ONLY
		if (dma == 1 && data->len != 0 && !ctrl->xip_on && !hal_interrupt_get_nest())
			ret = HAL_Flashc_DMATransfer(ctrl, write, data->pdata, data->len);
			/*ret = HAL_Flashc_PollTransfer(ctrl, write, data->pdata, data->len);*/
		else
#endif
			ret = HAL_Flashc_PollTransfer(ctrl, write, data->pdata, data->len);
#endif
	}

	if (ret != HAL_OK)
		FC_ERROR("err on cmd: 0x%x, data len: %d", *cmd->pdata, data->len);

	return ret;
}

void HAL_Flashc_DUMP_SbusCnt(void)
{
	printf("DEBUG_CNT_SBUS RD:0x%08x WR:0x%08x\n",
	       OPI_MEM_CTRL->DEBUG_CNT_SBUS_RD, OPI_MEM_CTRL->DEBUG_CNT_SBUS_WR);
}

void HAL_Flashc_DUMP_CbusCnt(void)
{
	printf("DEBUG_CNT_CBUS RD:0x%08x WR:0x%08x\n",
	       OPI_MEM_CTRL->DEBUG_CNT_CBUS_RD, OPI_MEM_CTRL->DEBUG_CNT_CBUS_WR);
}

struct flash_controller *HAL_Flashc_Create(uint32_t id)
{
	struct flash_controller *ctrl;

	ctrl = hal_malloc(sizeof(struct flash_controller));
	if (ctrl == NULL) {
		FC_ERROR("no mem");
	} else {
		memset(ctrl, 0, sizeof(struct flash_controller));
		ctrl->cacheWriteThroughIdx = -1;
		if (_flashc_caback)
			_flashc_caback(ctrl, FC_CREATE, 0);
	}

	return ctrl;
}

HAL_Status HAL_Flashc_Destory(struct flash_controller *ctrl)
{
	if (ctrl == NULL) {
		FC_ERROR("ctrl %p", ctrl);
	} else {
		if (_flashc_caback)
			_flashc_caback(ctrl, FC_DESTORY, 0);
#if 0
		if (ctrl->cacheWriteThroughIdx >= 0)
			HAL_Dcache_Release_WriteThroughIndex(ctrl->cacheWriteThroughIdx);
#endif
		hal_free(ctrl);
	}

	return HAL_OK;
}

int HAL_Flashc_IncRef(struct flash_controller *ctrl)
{
	return (++ctrl->ref);
}

int HAL_Flashc_DecRef(struct flash_controller *ctrl)
{
	if (ctrl->ref > 0) {
		return (--ctrl->ref);
	} else {
		return -1;
	}
}
