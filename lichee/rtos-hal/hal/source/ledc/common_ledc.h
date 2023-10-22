/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __COMMON_LEDC_I_H__
#define __COMMON_LEDC_I_H__

#ifdef __cplusplus
extern "C" {
#endif

/* LEDC register offset */
#define LEDC_CTRL_REG       		(0x00) 	/* LEDC Control Register */
#define LED_T01_TIMING_CTRL_REG		(0x04) 	/* LED T0 & 1 Timing Control Register */
#define LEDC_DATA_FINISH_CNT_REG   	(0x08) 	/* LEDC Data Finish Counter Register */
#define LED_RST_TIMING_CTRL_REG    	(0x0c) 	/* LED Reset Timing Control Register */
#define LEDC_WAIT_TIME0_CTRL_REG	(0x10)	/* LEDC Wait Time0 Control Register */
#define LEDC_DATA_REG    		(0x14) 	/* LEDC Data Register */
#define LEDC_DMA_CTRL_REG      		(0X18) 	/* LEDC Dma Control Register */
#define LEDC_INTC_REG   		(0x1c)	/* LEDC Interrupt Control Register */
#define LEDC_INTS_REG   		(0x20)	/* LEDC Interrupt Status Register */
#define LEDC_WAIT_TIME1_CTRL_REG   	(0x28) 	/* LEDC Wait Time1 Control Register */
#define LEDC_VER_NUM_REG   		(0x2C) 	/* LEDC Version Number Register */
#define LEDC_FIFO_DATA0_REG   		(0x30) 	/* LEDC Fifo Data0 Register */
#define LEDC_FIFO_DATA1_REG   		(0x34) 	/* LEDC Fifo Data1 Register */
#define LEDC_FIFO_DATA2_REG   		(0x38) 	/* LEDC Fifo Data2 Register */

#define LEDC_MAX_LED_COUNT 1024

#define LEDC_DEFAULT_LED_COUNT 8

#define LEDC_RESET_TIME_MIN_NS 84
#define LEDC_RESET_TIME_MAX_NS 327000

#define LEDC_T1H_MIN_NS 84
#define LEDC_T1H_MAX_NS 2560

#define LEDC_T1L_MIN_NS 84
#define LEDC_T1L_MAX_NS 1280

#define LEDC_T0H_MIN_NS 84
#define LEDC_T0H_MAX_NS 1280

#define LEDC_T0L_MIN_NS 84
#define LEDC_T0L_MAX_NS 2560

#define LEDC_WAIT_TIME0_MIN_NS 84
#define LEDC_WAIT_TIME0_MAX_NS 10000

#define LEDC_WAIT_TIME1_MIN_NS 84
#define LEDC_WAIT_TIME1_MAX_NS 85000000000ULL

#define LEDC_WAIT_DATA_TIME_MIN_NS 84
#define LEDC_WAIT_DATA_TIME_MAX_NS_IC 655000
#define LEDC_WAIT_DATA_TIME_MAX_NS_FPGA 20000000

#define LEDC_LEDC_FIFO_DEPTH 32 /* 32 * 4 bytes */
#define LEDC_LEDC_FIFO_TRIG_LEVEL 15


#ifdef __cplusplus
}
#endif
#endif /* __COMMON_LEDC_I_H__ */