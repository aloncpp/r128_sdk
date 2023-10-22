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

#ifndef _ROM_DRIVER_CHIP_HAL_LPUART_H_
#define _ROM_DRIVER_CHIP_HAL_LPUART_H_


#endif
/*
 *
 * Register definitions for UART
 */
#define LPUART_GSC (0x0000)
#define LPUART_BCR (0x0004)
#define LPUART_RCR (0x0010)
#define LPUART_RDR (0x0014)
#define LPUART_IER (0x0020)
#define LPUART_ISR (0x0024)
#define LPUART_ICR (0x0028)
#define LPUART_RCP1 (0x002C)
#define LPUART_RCP2 (0x0030)
#define LPUART0_AON (0x00D0)
#define LPUART1_AON (0x00D4)

/*
 * register bit field define
 */
/* LPUART Control Register */
#define GP_SR_CON (BIT(8))
/* LPUART Baudrate Config Register */
#define LPUART_BCR_DE_MASK	(0xff)
#define LPUART_BCR_DENO		(3 << 24)
#define LPUART_BCR_REMA		(2 << 16)
#define LPUART_BCR_QUOT		(6)
/* LPAURT Enable Register */
#define LPUART_IER_RDC		(BIT(7))
#define LPUART_IER_RD		(BIT(9))
/* LPUART RX Config Register */
#define LPUART_RCR_DWID_MASK	(BIT(10)|BIT(9)|BIT(8))
#define LPUART_RCR_DWID4      (0 << 8)
#define LPUART_RCR_DWID5      (1 << 8)
#define LPUART_RCR_DWID6      (2 << 8)
#define LPUART_RCR_DWID7      (3 << 8)
#define LPUART_RCR_DWID8      (4 << 8)
#define LPUART_RCR_DWID9      (5 << 8)
#define LPUART_RCR_PARITY_MASK	(BIT(6)|BIT(5)|BIT(4))
#define EVEN                  (1 << 4)
#define ODD                   (2 << 4)
#define SPACE                 (3 << 4)
#define MARK                  (4 << 4)
#define EN_RX                 (BIT(0))
#define LPUART_RCR_MSB	      (1 << 12)

#define LPUART_MULT_16		(1 << 16)
#define LPUART_MULT_17		(1 << 17)

/* LPUART RX Compare Register */

