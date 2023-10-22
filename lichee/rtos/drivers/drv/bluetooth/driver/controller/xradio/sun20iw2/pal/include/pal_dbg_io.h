/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      Debug GPIO definition.
 *
 *  \author     Xradio BT Team
 *
 *  \Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
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
/*************************************************************************************************/

#ifndef PAL_DBG_IO_H
#define PAL_DBG_IO_H

#include "pal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup PAL_DBG_IO
 *  \{ */
/**************************************************************************************************
  Data Types
**************************************************************************************************/
/*! \brief      Reserved DBG IO IDs. */
enum {
	/* System signals. */
	PAL_DBG_IO_ID_H_TASK        = 0x00,  /*!< High priority Task Debug. */
	PAL_DBG_IO_ID_L_TASK        = 0x01,  /*!< Low priority Task Debug. */
	PAL_DBG_IO_ID_BLE_INTRP     = 0x02,  /*!< BLE Interrupt Debug. */
	PAL_DBG_IO_ID_ERROR         = 0x03,  /*!< Error Debug. */
	PAL_DBG_IO_ID_LTASK_BB      = 0x04,
	PAL_DBG_IO_ID_LTASK_LL      = 0x05,
};

#define palBtPmSleepHandler_dbgio               (7)//up and down
#define ld_clock_isr_wl_dbgio                   (7)//up and down

#define btc_suspend_noirq_dbgio                 (10)//up
#define btc_resume_noirq_dbgio                  (10)//down

#define btc_bb_sleep_check_dbgio                (11)
#define btc_bb_sleep_check_out1_dbgio           (29)
#define btc_bb_sleep_check_out2_dbgio           (18)
#define btc_bb_sleep_check_out3_dbgio           (19)
#define palBtPmSleepTimerISR_dbgio              (24)

#define ld_clock_isr_dbgio                      (99)
#define ld_clock_isr_sniff_dbgio1               (25)
#define ld_clock_isr_sniff_dbgio2               (26)
#define ld_clock_isr_sniff_dbgio3               (27)
#define ld_clock_isr_sniff_dbgio4               (28)
#define ld_rx_clock_sniff1_dbgio                (99)
#define ld_rx_clock_sniff2_dbgio                (99)

#define ld_sleep_enter_dbgio                    (99)


/**************************************************************************************************
  Macros
**************************************************************************************************/

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/* Initialization */
void PalIoInit(void);
void PalIoDeInit(void);

/* Control and Status */
void PalIoCtl(uint8_t io_num, uint8_t io_state);
void PalIoOn(uint8_t id);
void PalIoOff(uint8_t id);
void PalIoOnAndOff(uint8_t id, uint8_t delay);

/*! \} */    /* PAL_DBG_IO */

#ifdef __cplusplus
};
#endif

#endif /* PAL_DBG_IO_H */
