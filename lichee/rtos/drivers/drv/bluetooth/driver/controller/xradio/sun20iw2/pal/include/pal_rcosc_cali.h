/******************************************************************************
 *
 * @file    rcosc_cali.h
 * @brief   rcosc cali
 *
 *
 * Copyright (C) 2019 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
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
 *
 ******************************************************************************
 */

#ifndef _PAL_RCOSC_CALI_H_
#define _PAL_RCOSC_CALI_H_

/** Constants -------------------------------------------------------------- */

/** Function Declarations -------------------------------------------------- */
#if 0
void rcosc_cali_init(void); //
void rcosc_cali_deinit(void);//

void rcosc_cali_restart(void);
void rcosc_cali_hosc_set(uint32_t hosc_clk);

void rcosc_cali_wup_set(uint16_t phase_1_wup_time,
                        uint8_t phase_1_wup_cnt,
                        uint8_t phase_2_wup_time,
                        uint8_t phase_2_wup_cnt,
                        uint8_t phase_3_wup_time);

void ble_clk32k_switch(uint8_t switch_to_rcosc);

uint32_t ble_clk32k_switch_loss_get(void);
#endif

/** Constants -------------------------------------------------------------- */
#define RCOCAL_DFT_WUP_TIME                   640
#define RCOCAL_DFT_PHASE_1_CNT                5
#define RCOCAL_DFT_PHASE_2_CNT                3

#define RCOCAL_OUT_CLK                        32000
#define RCOCAL_CNT_TAGT                       8192

typedef enum {
	BLE_32K_CLOCK_LFCLKORRCOSC   = 0,
	BLE_32K_CLOCK_RCCAL          = 1,
	BLE_32K_CLOCK_HOSC32K        = 2,
} BLE_32KClockType;

/** Function Declarations -------------------------------------------------- */
void rcosc_cali_init(void);
void rcosc_cali_deinit(void);

uint32_t ble_clk32k_switch_loss_get(void);
void ble_hosc_32k_start();
void ble_hosc_32k_stop();
void ble_rcosc_cal_32k_start();
void ble_rcosc_cal_32k_stop();
void ble_lfclkOrRcosc_32k_start();

void ble_32k_init(BLE_32KClockType type);
void ble_32k_deinit();

#endif /* _PAL_RCOSC_CALI_H_ */

/******************* (C) COPYRIGHT 2019 XRADIO, INC. ******* END OF FILE *****/
