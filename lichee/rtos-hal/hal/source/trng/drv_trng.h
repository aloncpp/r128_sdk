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

#ifndef __DRV_TRNG_H_
#define __DRV_TRNG_H_

#define     __IO    volatile             /*!< Defines 'read / write' permissions */
/*
 * Bitwise operation
 */
#define HAL_BIT(pos)                        (1U << (pos))

#define HAL_SET_BIT(reg, mask)              ((reg) |= (mask))
#define HAL_CLR_BIT(reg, mask)              ((reg) &= ~(mask))
#define HAL_GET_BIT(reg, mask)              ((reg) & (mask))
#define HAL_GET_BIT_VAL(reg, shift, vmask)  (((reg) >> (shift)) & (vmask))

#define HAL_MODIFY_REG(reg, clr_mask, set_mask) \
    ((reg) = (((reg) & (~(clr_mask))) | (set_mask)))

/**
 * @brief Drv Status value
 */
typedef enum
{
    DRV_OK      = 0,    /* success */
    DRV_ERROR   = -1,   /* general error */
    DRV_BUSY    = -2,   /* device or resource busy */
    DRV_TIMEOUT = -3,   /* wait timeout */
    DRV_INVALID = -4    /* invalid argument */
} DRV_Status;

typedef struct {
	__IO uint32_t CTRL_CFG;							/* ,	 Address offset: 0x00	 */
	__IO uint32_t JITTER_CFG;						/* ,	 Address offset: 0x04	 */
	__IO uint32_t JITTER_CNT_TIMING;				/* ,	 Address offset: 0x08	 */
	__IO uint32_t MONITOR_RCT;						/* ,	 Address offset: 0x0C	 */
	__IO uint32_t MONITOR_APT;						/* ,	 Address offset: 0x10	 */
	__IO uint32_t EXTRACT_CFG;						/* ,	 Address offset: 0x14	 */
	__IO uint32_t RAND_BIT_URN0;					/* ,	 Address offset: 0x18	 */
	__IO uint32_t RAND_BIT_URN1;					/* ,	 Address offset: 0x1C	 */
	__IO uint32_t RAND_BIT_URN2;					/* ,	 Address offset: 0x20	 */
	__IO uint32_t RAND_BIT_URN3;					/* ,	 Address offset: 0x24	 */
	__IO uint32_t JITTER_CNT_RESULT0;				/* ,	 Address offset: 0x28	 */
	__IO uint32_t JITTER_CNT_RESULT1;				/* ,	 Address offset: 0x2C	 */
	__IO uint32_t JITTER_CNT_RESULT2;				/* ,	 Address offset: 0x30	 */
	__IO uint32_t JITTER_CNT_RESULT3;				/* ,	 Address offset: 0x34	 */
	__IO uint32_t JITTER_CNT_RESULT4;				/* ,	 Address offset: 0x38	 */
	__IO uint32_t JITTER_CNT_RESULT5;				/* ,	 Address offset: 0x3C	 */
	__IO uint32_t JITTER_CNT_RESULT6;				/* ,	 Address offset: 0x40	 */
	__IO uint32_t JITTER_CNT_RESULT7;				/* ,	 Address offset: 0x44	 */
	__IO uint32_t JITTER_COUNTER_READY;				/* ,	 Address offset: 0x48	 */
	__IO uint32_t REG_ACCESS_CTR1;					/* ,	 Address offset: 0x4C	 */
	__IO uint32_t REG_ACCESS_CTR2;					/* ,	 Address offset: 0x50	 */
} TRNG_T;

#define TRNG_BASE (0x40048C00)

#define TRNG ((TRNG_T *)TRNG_BASE)

enum CTRL_CFG {
	CC_ENABLE 		= 0,
	CC_RO_DETUNE 	= 1,
	CC_RESERVE3 	= 3,
	CC_RO_CRTL 		= 4,
	CC_READY 		= 12,
	CC_RESERVE15 	= 15,
};
enum CTRL_CFG_MASK {
	CC_ENABLE_MASK 		= 0x1 << CC_ENABLE,
	CC_RO_DETUNE_MASK 	= 0x3 << CC_RO_DETUNE,
	CC_RESERVE3_MASK 	= 0x1 << CC_RESERVE3,
	CC_RO_CRTL_MASK 	= 0xFF << CC_RO_CRTL,
	CC_READY_MASK 		= 0x7 << CC_READY,
	CC_RESERVE15_MASK 	= 0x1FFFF << CC_RESERVE15,
};
enum CTRL_CFG_VMASK {
	CC_ENABLE_VMASK 		= 0x1,
	CC_RO_DETUNE_VMASK 	= 0x3,
	CC_RESERVE3_VMASK 	= 0x1,
	CC_RO_CRTL_VMASK 	= 0xFF,
	CC_READY_VMASK 		= 0x7,
	CC_RESERVE15_VMASK 	= 0x1FFFF,
};
enum JITTER_CFG {
	JC_JITTER_MONTITOR_WORK_EN 		= 0,
	JC_COUNTER_2DIV 				= 1,
	JC_JITTER_COUNTER_START 		= 2,
	JC_RESERVE3 					= 3,
};
enum JITTER_CFG_MASK {
	JC_JITTER_MONTITOR_WORK_EN_MASK 	= 0x1 << JC_JITTER_MONTITOR_WORK_EN,
	JC_COUNTER_2DIV_MASK 				= 0x1 << JC_COUNTER_2DIV,
	JC_JITTER_COUNTER_START_MASK 		= 0x1 << JC_JITTER_COUNTER_START,
	JC_RESERVE3_MASK 					= 0x1FFFFFFF << JC_RESERVE3,
};
enum JITTER_CNT_TIMING {
	JCT_JITTER_COUNTER_TIMING 	= 0,
	JCT_RESERVE21 				= 21,
};
enum JITTER_CNT_TIMING_MASK {
	JCT_JITTER_COUNTER_TIMING_MASK 	= 0x1FFFFF << JCT_JITTER_COUNTER_TIMING,
	JCT_RESERVE21_MASK 				= 0x7FF << JCT_RESERVE21,
};
enum MONITOR_RCT {
	MR_RCT_C 		= 0,
	MR_MONITOR_EN 	= 11,
	MR_RESERVE12 	= 12,
};
enum MONITOR_RCT_MASK {
	MR_RCT_C_MASK 		= 0x7FF << MR_RCT_C,
	MR_MONITOR_EN_MASK 	= 0x1 << MR_MONITOR_EN,
	MR_RESERVE12_MASK 	= 0x3FF << MR_RESERVE12,
};
enum MONITOR_APT {
	MA_APT_C 		= 0,
	MA_RESERVE11 	= 11,
	MA_APT_W 		= 12,
	MA_RESERVE23 	= 23,
};
enum MONITOR_APT_MASK {
	MA_APT_C_MASK		= 0x7FF << MA_APT_C,
	MA_RESERVE11_MASK 	= 0x1 << MA_RESERVE11,
	MA_APT_W_MASK 		= 0x7FF << MA_APT_W,
	MA_RESERVE23_MASK 	= 0x1FF << MA_RESERVE23,
};
enum EXTRACT_CFG {
	EC_RO_SAMPLING_RATION0 	= 0,
	EC_RO_SAMPLING_RATION1 	= 4,
	EC_RESILIENT_TYPE 		= 8,
	EC_EXTRACT_START 		= 9,
	EC_RESERVE10 			= 10,
	EC_RESILIENT_RATIO 		= 12,
};
enum EXTRACT_CFG_MASK {
	EC_RO_SAMPLING_RATION0_MASK 	= 0xFF << EC_RO_SAMPLING_RATION0,
	EC_RO_SAMPLING_RATION1_MASK 	= 0xFF << EC_RO_SAMPLING_RATION1,
	EC_RESILIENT_TYPE_MASK 			= 0x1 << EC_RESILIENT_TYPE,
	EC_EXTRACT_START_MASK 			= 0x1 << EC_EXTRACT_START,
	EC_RESERVE10_MASK 				= 0x3 << EC_RESERVE10,
	EC_RESILIENT_RATIO_MASK 		= 0xFFFFF << EC_RESILIENT_RATIO,
};
enum RAND_BIT_URN0 {
	RBU0_DATA = 0,
};
enum RAND_BIT_URN0_MASK {
	RBU0_DATA_MASK = 0xFFFFFFFF << RBU0_DATA,
};
enum RAND_BIT_URN1 {
	RBU1_DATA = 0,
};
enum RAND_BIT_URN1_MASK {
	RBU1_DATA_MASK = 0xFFFFFFFF << RBU1_DATA,

};
enum RAND_BIT_URN2 {
	RBU2_DATA = 0,
};
enum RAND_BIT_URN2_MASK {
	RBU2_DATA_MASK = 0xFFFFFFFF << RBU2_DATA,
};
enum RAND_BIT_URN3 {
	RBU3_DATA = 0,
};
enum RAND_BIT_URN3_MASK {
	RBU3_DATA_MASK = 0xFFFFFFFF << RBU3_DATA,
};
enum JITTER_CNT_RESULT0 {
	JCR0_DTAT 		= 0,
	JCR0_RESERVE21 	= 21,
};
enum JITTER_CNT_RESULT0_MASK {
	JCR0_DTAT_MASK 			= 0x1FFFFF << JCR0_DTAT,
	JCR0_RESERVE21_MASK 	= 0x7FF << JCR0_RESERVE21,
};
enum JITTER_CNT_RESULT1 {
	JCR1_DTAT 		= 0,
	JCR1_RESERVE21 	= 21,
};
enum JITTER_CNT_RESULT1_MASK {
	JCR1_DTAT_MASK 			= 0x1FFFFF << JCR1_DTAT,
	JCR1_RESERVE21_MASK 	= 0x7FF << JCR1_RESERVE21,
};
enum JITTER_CNT_RESULT2 {
	JCR2_DTAT 		= 0,
	JCR2_RESERVE21 	= 21,
};
enum JITTER_CNT_RESULT2_MASK {
	JCR2_DTAT_MASK 			= 0x1FFFFF << JCR2_DTAT,
	JCR2_RESERVE21_MASK 	= 0x7FF << JCR2_RESERVE21,
};
enum JITTER_CNT_RESULT3 {
	JCR3_DTAT 		= 0,
	JCR3_RESERVE21 	= 21,
};
enum JITTER_CNT_RESULT3_MASK {
	JCR3_DTAT_MASK 			= 0x1FFFFF << JCR3_DTAT,
	JCR3_RESERVE21_MASK 	= 0x7FF << JCR3_RESERVE21,
};
enum JITTER_CNT_RESULT4 {
	JCR4_DTAT 		= 0,
	JCR4_RESERVE21 	= 21,
};
enum JITTER_CNT_RESULT4_MASK {
	JCR4_DTAT_MASK 			= 0x1FFFFF << JCR4_DTAT,
	JCR4_RESERVE21_MASK 	= 0x7FF << JCR4_RESERVE21,
};
enum JITTER_CNT_RESULT5 {
	JCR5_DTAT 		= 0,
	JCR5_RESERVE21 	= 21,
};
enum JITTER_CNT_RESULT5_MASK {
	JCR5_DTAT_MASK 			= 0x1FFFFF << JCR5_DTAT,
	JCR5_RESERVE21_MASK 	= 0x7FF << JCR5_RESERVE21,
};
enum JITTER_CNT_RESULT6 {
	JCR6_DTAT 		= 0,
	JCR6_RESERVE21 	= 21,
};
enum JITTER_CNT_RESULT6_MASK {
	JCR6_DTAT_MASK 			= 0x1FFFFF << JCR6_DTAT,
	JCR6_RESERVE21_MASK 	= 0x7FF << JCR6_RESERVE21,
};
enum JITTER_CNT_RESULT7 {
	JCR7_DTAT 		= 0,
	JCR7_RESERVE21 	= 21,
};
enum JITTER_CNT_RESULT7_MASK {
	JCR7_DTAT_MASK 			= 0x1FFFFF << JCR7_DTAT,
	JCR7_RESERVE21_MASK 	= 0x7FF << JCR7_RESERVE21,
};
enum JITTER_COUNTER_READY {
	JCR_JITTER_COUNTER_READY 	= 0,
	JCR_RESERVE1 				= 1,
};
enum JITTER_COUNTER_READY_MASK {
	JCR_JITTER_COUNTER_READY_MASK 	= 0x1 << JCR_JITTER_COUNTER_READY,
	JCR_RESERVE1_MASK 				= 0x7FFFFFFF << JCR_RESERVE1,
};
enum REG_ACCESS_CTR1 {
	RAC1_CTR = 0,
};
enum REG_ACCESS_CTR1_MASK {
	RAC1_CTR_MASK = 0xFFFFFFFF << RAC1_CTR,
};
enum REG_ACCESS_CTR2 {
	RAC2_CTR 		= 0,
	RAC2_RESERVE8 	= 8,
};
enum REG_ACCESS_CTR2_MASK {
	RAC2_CTR_MASK 		= 0xFF << RAC2_CTR,
	RAC2_RESERVE8_MASK 	= 0xFFFFFF << RAC2_RESERVE8,
};

void DRV_Trng_Init(uint64_t* time);
void DRV_Trng_Deinit();
DRV_Status DRV_Trng_Jitter_Test(uint64_t *actime);
DRV_Status DRV_Trng_Extract(uint8_t type, uint32_t random[4], uint64_t *actime);
uint64_t DRV_Trng_Reg_Access_Time(void);

#endif

