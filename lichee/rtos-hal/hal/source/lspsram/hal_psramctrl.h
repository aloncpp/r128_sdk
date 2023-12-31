/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA,
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MAT
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
#ifndef _HAL_PSRAMCTRL_H_
#define _HAL_PSRAMCTRL_H_

/* The default clock frequency of lpsram is 192M. If you want to set
 * the lpsram clock frequency to 200M, you need to enable the following
 * definition CONFIG_PSRAM_200M.
 */
// #define CONFIG_PSRAM_200M

/* CLK CTRL MODULE */
#define BUS_CLK_GATING_CTRL0  0x4003c004
#define MOD_RESET_CTRL0       0x4003c00c
#define PSRAM_CTRL_CLK_GATING 20
#define PSRAM_CTRL_RST        20

#define PSRAM_CLKCFG	      0x4003c070
#define PSRAM_CLK_ENABLE      31
#define PSRAM_CLK_SRC_SEL     4

/* AON CLK CTRL MODULE */
#define PSRAMC_DPLL1_CTRL     0x4004c48c
#define PSRAMC_DPLL3_CTRL     0x4004c494
#define PSRAMC_DPLL1_OUT_CTRL 0x4004c4a4
#define CK1_PSRAM_DIV         16
#define CK1_PSRAM_EN          19
#define PSRAMC_DPLL3_OUT_CTRL 0x4004c4a8
#define CK3_PSRAM_DIV         16
#define CK3_PSRAM_EN          19
#define PSRAMC_DEV_CLK_CTRL   0x4004c4e0
#define CKPLL_PSRAM_SEL       20
#define PSRAM_DPLL1           1
#define PSRAM_DPLL3           3

#define PSRAMC_PAGE_SZ_512              512
#define PSRAMC_PAGE_SZ_1K               1024

/* MEM_COM_CFG 0x00 */
#define PSRAMC_ADDR_SIZE_SHIFT          (30)
#define PSRAMC_ADDR_SIZE_MASK           (3U << PSRAMC_ADDR_SIZE_SHIFT)
typedef enum {
	PSRAMC_ADDR_SIZE_24BIT = (2U << PSRAMC_ADDR_SIZE_SHIFT),
	PSRAMC_ADDR_SIZE_32BIT = (3U << PSRAMC_ADDR_SIZE_SHIFT),

} PSRAMC_AddrSize;
#define PSRAMC_IOX_DEF_OUTPUT_SHIFT(x)  (30 - 2 * (x))
#define PSRAMC_IOX_DEF_OUTPUT_VMASK     (3U)
#define PSRAMC_IOX_DEF_OUTPUT_MASK(x)   (PSRAMC_IOX_DEF_OUTPUT_VMASK << PSRAMC_IOX_DEF_OUTPUT_SHIFT(x))
typedef enum {
	PSRAMC_IOX_DEF_OUTPUT0 = 0U,
	PSRAMC_IOX_DEF_OUTPUT1 = 1U,
	PSRAMC_IOX_DEF_OUTPUTZ = 2U
} PSRAMC_IoxDefOutPutMode;
#define PSRAMC_IOX_DEF_OUTPUT_MODE(x, m) (((m) & PSRAMC_IOX_DEF_OUTPUT_VMASK) << PSRAMC_IOX_DEF_OUTPUT_SHIFT(x))

#define PSRAMC_SBUS_HRDY_WT_TO_SHIFT    (8)
#define PSRAMC_SBUS_HRDY_WT_TO_VMASK    (0x0FF)
#define PSRAMC_SBUS_HRDY_WT_TO_MASK     (PSRAMC_SBUS_HRDY_WT_TO_VMASK << PSRAMC_SBUS_HRDY_WT_TO_SHIFT)
#define PSRAMC_SBUS_HRDY_WT_TO_VAL(v)   (((v) & PSRAMC_SBUS_HRDY_WT_TO_VMASK) << PSRAMC_SBUS_HRDY_WT_TO_SHIFT)

#define PSRAMC_SBUS_HRDY_TO_EN          HAL_BIT(7)
#define PSRAMC_TRAN_FIFO_RST            HAL_BIT(6)
#define PSRAMC_REV_FIFO_RST             HAL_BIT(5)
#define PSRAM_SELECT                    HAL_BIT(4)
#define PSRAMC_WRAP_AROUND_EN           HAL_BIT(3)
#define PSRAMC_CBUS_RW_EN               HAL_BIT(0)

/* OPICOM_CFG 0x04 */
#define PSRAM_DMA_CROSS_OP_DIS          HAL_BIT(16)
#define PSRAM_SPI_RDWAIT_SHIFT          (12)
#define PSRAM_SPI_RDWAIT_VMASK          (3U)
#define PSRAM_SPI_RDWAIT_Cycle(n)       (((n) & PSRAM_SPI_RDWAIT_VMASK) << PSRAM_SPI_RDWAIT_SHIFT)
#define PSRAM_SPI_CS_SHIFT              (8)
#define PSRAM_SPI_CS_L                  (0U << PSRAM_SPI_CS_SHIFT)
#define PSRAM_SPI_CS_H                  (1U << PSRAM_SPI_CS_SHIFT)
#define PSRAM_SPI_FIRST_BIT_SHIFT       (4)
#define PSRAM_SPI_MSB                   (0U << PSRAM_SPI_FIRST_BIT_SHIFT)
#define PSRAM_SPI_LSB                   (1U << PSRAM_SPI_FIRST_BIT_SHIFT)
#define PSRAM_SPI_MODE_SHIFT            (0)
#define PSRAM_SPI_MODE_MASK             (3U << PSRAM_SPI_MODE_SHIFT)

typedef enum {
	PSRAM_SPI_MODE0 = (0U << PSRAM_SPI_MODE_SHIFT),
	PSRAM_SPI_MODE1 = (1U << PSRAM_SPI_MODE_SHIFT),
	PSRAM_SPI_MODE2 = (2U << PSRAM_SPI_MODE_SHIFT),
	PSRAM_SPI_MODE3 = (3U << PSRAM_SPI_MODE_SHIFT),
} PSRAM_SpiMode;

/* C_READ_CFG 0x10 */
#define PSRAMC_CBUS_RD_CMD_SHIFT        (24)
#define PSRAMC_CBUS_RD_CMD_VMASK        (0x0FF)
#define PSRAMC_CBUS_RD_CMD_MASK(cmd)    (((cmd) & PSRAMC_CBUS_RD_CMD_VMASK) << PSRAMC_CBUS_RD_CMD_SHIFT)
#define PSRAMC_IDBUS_DMA_EN             HAL_BIT(23)

#define PSRAMC_CBUS_CMD_BIT_SHIFT       (20)
#define PSRAMC_CBUS_CMD_BIT_VMASK       (0x7)
typedef enum {
	PSRAMC_CBUS_CMD_0BIT = (0x0U << PSRAMC_CBUS_CMD_BIT_SHIFT),
	PSRAMC_CBUS_CMD_1BIT = (0x1U << PSRAMC_CBUS_CMD_BIT_SHIFT),
	PSRAMC_CBUS_CMD_2BIT = (0x2U << PSRAMC_CBUS_CMD_BIT_SHIFT),
	PSRAMC_CBUS_CMD_4BIT = (0x3U << PSRAMC_CBUS_CMD_BIT_SHIFT),
	PSRAMC_CBUS_CMD_8BIT = (0x4U << PSRAMC_CBUS_CMD_BIT_SHIFT)
} PSRAMC_CbusCmdBit;

#define PSRAMC_CBUS_ADDR_BIT_SHIFT      (16)
#define PSRAMC_CBUS_ADDR_BIT_VMASK      (0x7)
typedef enum {
	PSRAMC_CBUS_ADDR_0BIT = (0x0U << PSRAMC_CBUS_ADDR_BIT_SHIFT),
	PSRAMC_CBUS_ADDR_1BIT = (0x1U << PSRAMC_CBUS_ADDR_BIT_SHIFT),
	PSRAMC_CBUS_ADDR_2BIT = (0x2U << PSRAMC_CBUS_ADDR_BIT_SHIFT),
	PSRAMC_CBUS_ADDR_4BIT = (0x3U << PSRAMC_CBUS_ADDR_BIT_SHIFT),
	PSRAMC_CBUS_ADDR_8BIT = (0x4U << PSRAMC_CBUS_ADDR_BIT_SHIFT)
} PSRAMC_CbusAddrBit;

#define PSRAMC_CBUS_DUMY_BIT_SHIFT      (12)
#define PSRAMC_CBUS_DUMY_BIT_VMASK      (0x7)
typedef enum {
	PSRAMC_CBUS_DUMY_0BIT = (0x0U << PSRAMC_CBUS_DUMY_BIT_SHIFT),
	PSRAMC_CBUS_DUMY_1BIT = (0x1U << PSRAMC_CBUS_DUMY_BIT_SHIFT),
	PSRAMC_CBUS_DUMY_2BIT = (0x2U << PSRAMC_CBUS_DUMY_BIT_SHIFT),
	PSRAMC_CBUS_DUMY_4BIT = (0x3U << PSRAMC_CBUS_DUMY_BIT_SHIFT),
	PSRAMC_CBUS_DUMY_8BIT = (0x4U << PSRAMC_CBUS_DUMY_BIT_SHIFT)
} PSRAMC_CbusDumyBit;

#define PSRAMC_CBUS_DUMY_WID_SHIFT      (4)
#define PSRAMC_CBUS_DUMY_WID_VMASK      (0x7F)
#define PSRAMC_CBUS_DUMY_WID(n)         (((n) & PSRAMC_CBUS_DUMY_WID_VMASK) << PSRAMC_CBUS_DUMY_WID_SHIFT)

#define PSRAMC_CBUS_DATA_BIT_SHIFT      (0)
#define PSRAMC_CBUS_DATA_BIT_VMASK      (0x7)
typedef enum {
	PSRAMC_CBUS_DATA_0BIT = (0x0U << PSRAMC_CBUS_DATA_BIT_SHIFT),
	PSRAMC_CBUS_DATA_1BIT = (0x1U << PSRAMC_CBUS_DATA_BIT_SHIFT),
	PSRAMC_CBUS_DATA_2BIT = (0x2U << PSRAMC_CBUS_DATA_BIT_SHIFT),
	PSRAMC_CBUS_DATA_4BIT = (0x3U << PSRAMC_CBUS_DATA_BIT_SHIFT),
	PSRAMC_CBUS_DATA_8BIT = (0x4U << PSRAMC_CBUS_DATA_BIT_SHIFT)
} PSRAMC_CbusDataBit;

/* C_WRITE_CFG 0x14 */
#define PSRAMC_CBUS_WR_CMD_SHIFT        (24)
#define PSRAMC_CBUS_WR_CMD_VMASK        (0x0FF)
#define PSRAMC_CBUS_WR_CMD(wcmd)        (((wcmd) & PSRAMC_CBUS_WR_CMD_VMASK) << PSRAMC_CBUS_WR_CMD_SHIFT)

#define PSRAMC_CBUS_WR_OP_CMD_BIT_SHIFT (20)
#define PSRAMC_CBUS_WR_OP_CMD_BIT_VMASK (0x7)
typedef enum {
	PSRAMC_CBUS_WR_OP_CMD_0BIT = (0x0U << PSRAMC_CBUS_WR_OP_CMD_BIT_SHIFT),
	PSRAMC_CBUS_WR_OP_CMD_1BIT = (0x1U << PSRAMC_CBUS_WR_OP_CMD_BIT_SHIFT),
	PSRAMC_CBUS_WR_OP_CMD_2BIT = (0x2U << PSRAMC_CBUS_WR_OP_CMD_BIT_SHIFT),
	PSRAMC_CBUS_WR_OP_CMD_4BIT = (0x3U << PSRAMC_CBUS_WR_OP_CMD_BIT_SHIFT),
	PSRAMC_CBUS_WR_OP_CMD_8BIT = (0x4U << PSRAMC_CBUS_WR_OP_CMD_BIT_SHIFT)
} PSRAMC_CbusWrOpCmdBit;

#define PSRAMC_CBUS_WR_OP_ADD_BIT_SHIFT (16)
#define PSRAMC_CBUS_WR_OP_ADD_BIT_VMASK (0x7)
typedef enum {
	PSRAMC_CBUS_WR_OP_ADDR_0BIT = (0x0U << PSRAMC_CBUS_WR_OP_ADD_BIT_SHIFT),
	PSRAMC_CBUS_WR_OP_ADDR_1BIT = (0x1U << PSRAMC_CBUS_WR_OP_ADD_BIT_SHIFT),
	PSRAMC_CBUS_WR_OP_ADDR_2BIT = (0x2U << PSRAMC_CBUS_WR_OP_ADD_BIT_SHIFT),
	PSRAMC_CBUS_WR_OP_ADDR_4BIT = (0x3U << PSRAMC_CBUS_WR_OP_ADD_BIT_SHIFT),
	PSRAMC_CBUS_WR_OP_ADDR_8BIT = (0x4U << PSRAMC_CBUS_WR_OP_ADD_BIT_SHIFT)
} PSRAMC_CbusWrOpAddrBit;

#define PSRAMC_CBUS_WR_OP_DUMY_BIT_SHIFT (12)
#define PSRAMC_CBUS_WR_OP_DUMY_BIT_VMASK (0x7)
typedef enum {
	PSRAMC_CBUS_WR_OP_DUMY_0BIT = (0x0U << PSRAMC_CBUS_WR_OP_DUMY_BIT_SHIFT),
	PSRAMC_CBUS_WR_OP_DUMY_1BIT = (0x1U << PSRAMC_CBUS_WR_OP_DUMY_BIT_SHIFT),
	PSRAMC_CBUS_WR_OP_DUMY_2BIT = (0x2U << PSRAMC_CBUS_WR_OP_DUMY_BIT_SHIFT),
	PSRAMC_CBUS_WR_OP_DUMY_4BIT = (0x3U << PSRAMC_CBUS_WR_OP_DUMY_BIT_SHIFT),
	PSRAMC_CBUS_WR_OP_DUMY_8BIT = (0x4U << PSRAMC_CBUS_WR_OP_DUMY_BIT_SHIFT)
} PSRAMC_CbusWrOpDumyBit;

#define PSRAMC_CBUS_WR_OP_DUMY_NUM_SHIFT (4)
#define PSRAMC_CBUS_WR_OP_DUMY_NUM_VMASK (0x7F)
#define PSRAMC_CBUS_WR_OP_DUMY_NUM(n)   (((n) & PSRAMC_CBUS_WR_OP_DUMY_NUM_VMASK) << PSRAMC_CBUS_WR_OP_DUMY_NUM_SHIFT)

#define PSRAMC_CBUS_WR_OP_DATA_BIT_SHIFT (0)
#define PSRAMC_CBUS_WR_OP_DATA_BIT_VMASK (0x7)

typedef enum {
RAMC_CBUS_WR_OP_DATA_0BIT = (0x0U << PSRAMC_CBUS_WR_OP_DATA_BIT_SHIFT),
	PSRAMC_CBUS_WR_OP_DATA_1BIT = (0x1U << PSRAMC_CBUS_WR_OP_DATA_BIT_SHIFT),
	PSRAMC_CBUS_WR_OP_DATA_2BIT = (0x2U << PSRAMC_CBUS_WR_OP_DATA_BIT_SHIFT),
	PSRAMC_CBUS_WR_OP_DATA_4BIT = (0x3U << PSRAMC_CBUS_WR_OP_DATA_BIT_SHIFT),
	PSRAMC_CBUS_WR_OP_DATA_8BIT = (0x4U << PSRAMC_CBUS_WR_OP_DATA_BIT_SHIFT)
} PSRAMC_CbusWrOpDataBit;

/* S_RW_CFG 0x2c */
#define PSRAMC_SBUS_RW_CMD_SHIFT        (24)
#define PSRAMC_SBUS_RW_CMD_SEND_MASK    (0x3 << PSRAMC_SBUS_RW_CMD_SHIFT)
#define PSRAMC_SBUS_CMD_SEND_BIT_SHIFT  (20)
#define PSRAMC_SBUS_CMD_SEND_BIT_VMASK  (0x7)
#define PSRAMC_SBUS_CMD_SEND_BIT_MASK   (PSRAMC_SBUS_CMD_SEND_BIT_VMASK << PSRAMC_SBUS_CMD_SEND_BIT_SHIFT)
typedef enum {
	PSRAMC_SBUS_CMD_SEND_0BIT = (0x0U << PSRAMC_SBUS_CMD_SEND_BIT_SHIFT),
	PSRAMC_SBUS_CMD_SEND_1BIT = (0x1U << PSRAMC_SBUS_CMD_SEND_BIT_SHIFT),
	PSRAMC_SBUS_CMD_SEND_2BIT = (0x2U << PSRAMC_SBUS_CMD_SEND_BIT_SHIFT),
	PSRAMC_SBUS_CMD_SEND_4BIT = (0x3U << PSRAMC_SBUS_CMD_SEND_BIT_SHIFT),
	PSRAMC_SBUS_CMD_SEND_8BIT = (0x4U << PSRAMC_SBUS_CMD_SEND_BIT_SHIFT)
} PSRAMC_SbusCmdSendBit;

#define PSRAMC_SBUS_ADDR_SEND_BIT_SHIFT (16)
#define PSRAMC_SBUS_ADDR_SEND_BIT_VMASK (0x7)
#define PSRAMC_SBUS_ADDR_SEND_BIT_MASK  (PSRAMC_SBUS_ADDR_SEND_BIT_VMASK << PSRAMC_SBUS_ADDR_SEND_BIT_SHIFT)
typedef enum {
	PSRAMC_SBUS_ADDR_SEND_0BIT = (0x0U << PSRAMC_SBUS_ADDR_SEND_BIT_SHIFT),
	PSRAMC_SBUS_ADDR_SEND_1BIT = (0x1U << PSRAMC_SBUS_ADDR_SEND_BIT_SHIFT),
	PSRAMC_SBUS_ADDR_SEND_2BIT = (0x2U << PSRAMC_SBUS_ADDR_SEND_BIT_SHIFT),
	PSRAMC_SBUS_ADDR_SEND_4BIT = (0x3U << PSRAMC_SBUS_ADDR_SEND_BIT_SHIFT),
	PSRAMC_SBUS_ADDR_SEND_8BIT = (0x4U << PSRAMC_SBUS_ADDR_SEND_BIT_SHIFT)
} PSRAMC_SbusAddrSendBit;

#define PSRAMC_SBUS_DUMY_SEND_BIT_SHIFT  (12)
#define PSRAMC_SBUS_DUMY_SEND_BIT_VMASK  (0x7)
#define PSRAMC_SBUS_DUMY_SEND_BIT_MASK   (PSRAMC_SBUS_DUMY_SEND_BIT_VMASK << PSRAMC_SBUS_DUMY_SEND_BIT_SHIFT)
typedef enum {
	PSRAMC_SBUS_DUMY_SEND_0BIT = (0x0U << PSRAMC_SBUS_DUMY_SEND_BIT_SHIFT),
	PSRAMC_SBUS_DUMY_SEND_1BIT = (0x1U << PSRAMC_SBUS_DUMY_SEND_BIT_SHIFT),
	PSRAMC_SBUS_DUMY_SEND_2BIT = (0x2U << PSRAMC_SBUS_DUMY_SEND_BIT_SHIFT),
	PSRAMC_SBUS_DUMY_SEND_4BIT = (0x3U << PSRAMC_SBUS_DUMY_SEND_BIT_SHIFT),
	PSRAMC_SBUS_DUMY_SEND_8BIT = (0x4U << PSRAMC_SBUS_DUMY_SEND_BIT_SHIFT)
} PSRAMC_SbusDumySendBit;
/* DUMMY Data Bit Number, n/8 Byte */
#define PSRAMC_SBUS_DMY_DATA_WID_SHIFT  (4)
#define PSRAMC_SBUS_DMY_DATA_WID_VMASK  (0x7)
#define PSRAMC_SBUS_DMY_DATA_WID_MASK   (PSRAMC_SBUS_DMY_DATA_WID_VMASK << PSRAMC_SBUS_DMY_DATA_WID_SHIFT)
#define PSRAMC_SBUS_DMY_DATA_WID(n)     (((n) & PSRAMC_SBUS_DMY_DATA_WID_VMASK) << PSRAMC_SBUS_DMY_DATA_WID_SHIFT)

#define PSRAMC_SBUS_DATA_GW_BIT_SHIFT   (0)
#define PSRAMC_SBUS_DATA_GW_BIT_VMASK   (0x7)
#define PSRAMC_SBUS_DATA_GW_BIT_MASK    (PSRAMC_SBUS_DATA_GW_BIT_VMASK << PSRAMC_SBUS_DATA_GW_BIT_SHIFT)
typedef enum {
	PSRAMC_SBUS_DATA_GW_0BIT = (0x0U << PSRAMC_SBUS_DATA_GW_BIT_SHIFT),
	PSRAMC_SBUS_DATA_GW_1BIT = (0x1U << PSRAMC_SBUS_DATA_GW_BIT_SHIFT),
	PSRAMC_SBUS_DATA_GW_2BIT = (0x2U << PSRAMC_SBUS_DATA_GW_BIT_SHIFT),
	PSRAMC_SBUS_DATA_GW_4BIT = (0x3U << PSRAMC_SBUS_DATA_GW_BIT_SHIFT),
	PSRAMC_SBUS_DATA_GW_8BIT = (0x4U << PSRAMC_SBUS_DATA_GW_BIT_SHIFT)
} PSRAMC_SbusDataGwBit;

/* FIFO_TRIG_LEV 0x4c */
#define PSRAMC_FIFO_WR_FULL_SHIFT       (24)
#define PSRAMC_FIFO_WR_FULL_VMASK       (0x0FF)
#define PSRAMC_FIFO_WR_FULL_TRIG(v)     (((v) & PSRAMC_FIFO_WR_FULL_VMASK) << PSRAMC_FIFO_WR_FULL_SHIFT)
#define PSRAMC_FIFO_WR_EMPT_SHIFT       (16)
#define PSRAMC_FIFO_WR_EMPT_VMASK       (0x0FF)
#define PSRAMC_FIFO_WR_EMPT_TRIG(v)     (((v) & PSRAMC_FIFO_WR_EMPT_VMASK)<< PSRAMC_FIFO_WR_EMPT_SHIFT)
#define PSRAMC_FIFO_RD_FULL_SHIFT       (8)
#define PSRAMC_FIFO_RD_FULL_VMASK       (0x0FF)
#define PSRAMC_FIFO_RD_FULL_TRIG(v)     (((v) & PSRAMC_FIFO_RD_FULL_VMASK) << PSRAMC_FIFO_RD_FULL_SHIFT)
#define PSRAMC_FIFO_RD_EMPT_SHIFT       (0)
#define PSRAMC_FIFO_RD_EMPT_VMASK       (0x0FF)
#define PSRAMC_FIFO_RD_EMPT_TRIG(v)     (((v) & PSRAMC_FIFO_RD_EMPT_VMASK) << PSRAMC_FIFO_RD_EMPT_SHIFT)

/* INT_EN_REG 0x54 */
#define PSRAMC_DMA_WR_CROSS_INT_EN      HAL_BIT(15)
#define PSRAMC_RD_TOUT_INT_EN           HAL_BIT(14)
#define PSRAMC_ISR_HEDY_TI_EN           HAL_BIT(13)
#define PSRAMC_IER_TRANS_ENB            HAL_BIT(12)
#define PSRAMC_IER_TRANS_ENB_MASK       HAL_BIT(12)
#define PSRAMC_WR_FIFO_OVER_FLOW_EN     HAL_BIT(10)
#define PSRAMC_WR_FIFO_FULL_EN          HAL_BIT(6)
#define PSRAMC_WR_FIFO_EMP_EN           HAL_BIT(5)
#define PSRAMC_WR_FIFO_READY_RQ_EN      HAL_BIT(4)
#define PSRAMC_RD_FIFO_FULL_EN          HAL_BIT(2)
#define PSRAMC_RD_FIFO_READY_RQ_EN      HAL_BIT(0)

/* INT_STA_REG 0x58 */
#define DMA_WR_CROSS_OP_FLAG		HAL_BIT(15)
#define PSRAMC_RD_TOUT_FLAG             HAL_BIT(14)
#define PSRAMC_ISR_HEDY_TIOT            HAL_BIT(13)
#define PSRAMC_ISR_TRANS_END            HAL_BIT(12)
#define PSRAMC_WR_FIFO_OVER_FLOW        HAL_BIT(10)
#define PSRAMC_WR_FIFO_FULL_FLAG        HAL_BIT(6)
#define PSRAMC_WR_FIFO_EMP_FLAG         HAL_BIT(5)
#define PSRAMC_WR_FIFO_REQ_FLAG         HAL_BIT(4)
#define PSRAMC_RD_FIFO_FULL_FLAG        HAL_BIT(2)
#define PSRAMC_RD_FIFO_REQ_FLAG         HAL_BIT(0)

/* PSRAM_COM_CFG 0x70 */
#define PSRAMC_MAX_RD_LATENCY_SHIFT     (28)
#define PSRAMC_MAX_RD_LATENCY_VMASK     (0x0F)
#define PSRAMC_MAX_READ_LATENCY(l)      (((l) & PSRAMC_MAX_RD_LATENCY_VMASK) << PSRAMC_MAX_RD_LATENCY_SHIFT)

#define PSRAMC_MAX_CEN_LOW_CYC_SHIFT    (16)
#define PSRAMC_MAX_CEN_LOW_CYC_VMASK    (0x0FFF)
#define PSRAMC_MAX_CEN_LOW_CYC_MASK     ((PSRAMC_MAX_CEN_LOW_CYC_VMASK) << PSRAMC_MAX_CEN_LOW_CYC_SHIFT)
#define PSRAMC_MAX_CEN_LOW_CYC_NUM(n)   (((n) & PSRAMC_MAX_CEN_LOW_CYC_VMASK) << PSRAMC_MAX_CEN_LOW_CYC_SHIFT)

#define PSRAMC_MR_REG_ADDR_EN		HAL_BIT(14)
#define PSRAMC_COM_DQS_READ_WAIT_SHIFT  (12)
#define PSRAMC_COM_DQS_READ_WAIT_VMASK  (3U)
#define PSRAMC_COM_DQS_READ_WAIT(w)     (((w) & PSRAMC_COM_DQS_READ_WAIT_VMASK) << PSRAMC_COM_DQS_READ_WAIT_SHIFT)

#define PSRAMC_DUMMY_NUM_SHIFT          (10)
#define PSRAMC_DUMMY_NUM_MASK           (0x3 << PSRAMC_DUMMY_NUM_SHIFT)
typedef enum {
	PSRAMC_DUM_NUM_CLC_1 = (0x0U << PSRAMC_DUMMY_NUM_SHIFT),
	PSRAMC_DUM_NUM_CLC_2 = (0x1U << PSRAMC_DUMMY_NUM_SHIFT),
	PSRAMC_DUM_NUM_CLC_3 = (0x2U << PSRAMC_DUMMY_NUM_SHIFT),
	PSRAMC_DUM_NUM_CLC_4 = (0x3U << PSRAMC_DUMMY_NUM_SHIFT)
} PSRAMC_DumNumClc;

#define PSRAMC_MIN_WR_CLC_SHIFT         (8)
#define PSRAMC_MIN_WR_CLC_MASK          (0x3 << PSRAMC_MIN_WR_CLC_SHIFT)
typedef enum {
	PSRAMC_MIN_WR_CLC_1 = (0x0U << PSRAMC_MIN_WR_CLC_SHIFT),
	PSRAMC_MIN_WR_CLC_2 = (0x1U << PSRAMC_MIN_WR_CLC_SHIFT),
	PSRAMC_MIN_WR_CLC_3 = (0x2U << PSRAMC_MIN_WR_CLC_SHIFT),
	PSRAMC_MIN_WR_CLC_4 = (0x3U << PSRAMC_MIN_WR_CLC_SHIFT)
} PSRAMC_MinWrClc;

#define PSRAMC_CLK_STOP_CE_LOW          HAL_BIT(7)
#define PSRAMC_CEDIS_CLK_VALID          HAL_BIT(6)
#define PSRAMC_WR_AF_DM_DUMMY           HAL_BIT(5)
#define PSRAMC_WR_AF_DQS_DUMMY          HAL_BIT(4)
#define PSRAMC_WR_NEED_DQS              HAL_BIT(3)
#define PSRAMC_CLK_OUTPUT_HLD           HAL_BIT(2)
#define PSRAMC_DDR_MODE_EN              HAL_BIT(1)
#define PSRAMC_CMD_HLD_THCYC            HAL_BIT(0)

/* PSRAM_LC_CFG 0x74 */
#define PSRAMC_RD_LC_TOUT_SHIFT         (16)
#define PSRAMC_RD_LC_TOUT_MASK          (0x0FF << PSRAMC_RD_LC_TOUT_SHIFT)
#define PSRAMC_CBUS_WR_LC_SHIFT         (8)
#define PSRAMC_CBUS_WR_LC_MASK          (0x0FF << PSRAMC_CBUS_WR_LC_SHIFT)
#define PSRAMC_SBUS_WR_LC_SHIFT         (0)
#define PSRAMC_SBUS_WR_LC_MASK          (0x0FF << PSRAMC_SBUS_WR_LC_SHIFT)

/* PSRAM_TIM_CFG 0x78 */
#define PSRAMC_DQS_OUTP_DHCYC_SHIFT     (20)
#define PSRAMC_DQS_OUTP_DHCYC_VMASK     (0x3)
#define PSRAMC_DQS_OUTP_DHCYC_MASK      (PSRAMC_DQS_OUTP_DHCYC_VMASK << PSRAMC_DQS_OUTP_DHCYC_SHIFT)
#define PSRAMC_DQS_OUTP_DHCYC(n)        (((n) & PSRAMC_DQS_OUTP_DHCYC_VMASK) << PSRAMC_DQS_OUTP_DHCYC_SHIFT)
#define PSRAMC_DQS_INP_DHCYC_SHIFT      (16)
#define PSRAMC_DQS_INP_DHCYC_VMASK      (PSRAMC_DQS_INP_DHCYC_VMASK)
#define PSRAMC_DQS_INP_DHCYC_MASK       (0x3 << PSRAMC_DQS_INP_DHCYC_SHIFT)
#define PSRAMC_DQS_INP_DHCYC(n)         (((n) & PSRAMC_DQS_INP_DHCYC_VMASK) << PSRAMC_DQS_INP_DHCYC_SHIFT)
#define PSRAMC_DM_OUTP_DHCYC_SHIFT      (12)
#define PSRAMC_DM_OUTP_DHCYC_VMASK      (0x3)
#define PSRAMC_DM_OUTP_DHCYC_MASK       (PSRAMC_DM_OUTP_DHCYC_VMASK << PSRAMC_DM_OUTP_DHCYC_SHIFT)
#define PSRAMC_DM_OUTP_DHCYC(n)         (((n) & PSRAMC_DM_OUTP_DHCYC_VMASK) << PSRAMC_DM_OUTP_DHCYC_SHIFT)
#define PSRAMC_CS_OUTP_DHCYC_SHIFT      (8)
#define PSRAMC_CS_OUTP_DHCYC_VMASK      (0x3)
#define PSRAMC_CS_OUTP_DHCYC_MASK       (PSRAMC_CS_OUTP_DHCYC_VMASK << PSRAMC_CS_OUTP_DHCYC_SHIFT)
#define PSRAMC_CS_OUTP_DHCYC(n)         (((n) & PSRAMC_CS_OUTP_DHCYC_VMASK) << PSRAMC_CS_OUTP_DHCYC_SHIFT)
#define PSRAMC_CLK_OUTP_DHCYC_SHIFT     (4)
#define PSRAMC_CLK_OUTP_DHCYC_VMASK     (0x3)
#define PSRAMC_CLK_OUTP_DHCYC_MASK      (PSRAMC_CLK_OUTP_DHCYC_VMASK << PSRAMC_CLK_OUTP_DHCYC_SHIFT)
#define PSRAMC_CLK_OUTP_DHCYC(n)        (((n) & PSRAMC_CLK_OUTP_DHCYC_VMASK) << PSRAMC_CLK_OUTP_DHCYC_SHIFT)
#define PSRAMC_ADQ_OUTP_DHCYC_SHIFT     (0)
#define PSRAMC_ADQ_OUTP_DHCYC_VMASK     (0x3)
#define PSRAMC_ADQ_OUTP_DHCYC_MASK      (PSRAMC_ADQ_OUTP_DHCYC_VMASK<< PSRAMC_ADQ_OUTP_DHCYC_SHIFT)
#define PSRAMC_ADQ_OUTP_DHCYC(n)        (((n) & PSRAMC_ADQ_OUTP_DHCYC_VMASK) << 0)

/* PSRAM_DQS_IN_DLY_CFG 0x7c */
#define PSRAMC_OVERWR_CAL               HAL_BIT(24)
#define PSRAMC_OVERWR_CAL_SHIFT         (16)
#define PSRAMC_OVERWR_CAL_VMASK         (0x3F)
#define PSRAMC_OVERWR_CAL_MASK          (0x3F << PSRAMC_OVERWR_CAL_SHIFT)
#define PSRAMC_OVERWR_CAL_MASK2          (0x3F << 4)
#define PSRAMC_OVERWR_CAL_VAL(n)        (((n) & PSRAMC_OVERWR_CAL_VMASK) << PSRAMC_OVERWR_CAL_SHIFT)
#define PSRAMC_CAL_SUCCEED              HAL_BIT(12)
#define PSRAMC_CAL_RESULT_VAL_SHIFT     (4)
#define PSRAMC_CAL_RESULT_VAL_MASK      (0x3F << PSRAMC_CAL_RESULT_VAL_SHIFT)
#define PSRAMC_START_DQS_DELAY_CAL      HAL_BIT(0)


#define PSRAMC_CBUS_WR_SEL_BUS          HAL_BIT(2)

#define PSRAMC_S_RW_CFG_RW_COM_SEND_SHIFT  24

/* psram register base */
#define PSRAM_CTRL_BASE 0x4000d800

/* psarm register */
#define PSRAMC_START_POS(addr)          ((addr) & 0xFFFFFFF0)
#define PSRAMC_END_POS(addr)            ((addr) & 0xFFFFFFF0)
#define PSRAMC_ADDR_BIAS_EN             HAL_BIT(31)

typedef struct {
	__IO uint32_t START_ADDR;	/* Address offset: N * 0x4 + 0x00 */
	__IO uint32_t END_ADDR;		/* Address offset: N * 0x4 + 0x04 */
	__IO uint32_t BIAS_ADDR;	/* Address offset: N * 0x4 + 0x08 */
	__I  uint32_t RESERVE0C;	/* Address offset: N * 0x4 + 0x0c */
} ADDR_T;

typedef struct {
	__IO uint32_t MEM_COM_CFG;	/* Address offset: 0x000 */
	__IO uint32_t OPI_COM_CFG;	/* Address offset: 0x004 */
	__IO uint32_t CACHE_CFG;	/* Address offset: 0x008 */
	__IO uint32_t MEN_AC_CFG;	/* Address offset: 0x00c */
	__IO uint32_t C_READ_CFG;	/* Address offset: 0x010 */
	__IO uint32_t C_WRITE_CFG;	/* Address offset: 0x014 */
	__IO uint32_t RESERVE_18[5];	/* Address offset: 0x018 ~ 0x028 */
	__IO uint32_t S_RW_CFG;		/* Address offset: 0x02c */
	__IO uint32_t S_ADDR_CFG;	/* Address offset: 0x030 */
	__IO uint32_t RESERVE_34[3];	/* Address offset: 0x034 ~ 0x03c */
	__IO uint32_t S_WR_NUM;		/* Address offset: 0x040 */
	__IO uint32_t S_RD_NUM;		/* Address offset: 0x044 */
	__IO uint32_t START_SEND_REG;	/* Address offset: 0x048 */
	__IO uint32_t FIFO_TRIG_LEV;	/* Address offset: 0x04c */
	__I  uint32_t FIFO_STA_REG;	/* Address offset: 0x050 */
	__IO uint32_t INT_EN_REG;	/* Address offset: 0x054 */
	__IO uint32_t INT_STA_REG;	/* Address offset: 0x058 */
	__IO uint32_t RESERVE_5c[1];	/* Address offset: 0x05c */
	__I  uint32_t DEBUG_STA;	/* Address offset: 0x060 */
	__IO uint32_t RESERVE_64[2];	/* Address offset: 0x064 ~ 0x068 */
	__IO uint32_t PSRAM_FORCE_CFG;	/* Address offset: 0x06c */
	__IO uint32_t PSRAM_COM_CFG;	/* Address offset: 0x070 */
	__IO uint32_t PSRAM_LAT_CFG;	/* Address offset: 0x074 */
	__IO uint32_t PSRAM_TIM_CFG;	/* Address offset: 0x078 */
	__IO uint32_t PSRAM_DQS_IN_DLY_CFG; /* Address offset: 0x07c */
	ADDR_T PSRAM_ADDR[4];		/* Address offset: 0x080 ~ 0x0bc */
	__IO uint32_t PSRAM_CLK_OUT_DLY_CFG;/* Address offset: 0x0c0 */
	__IO uint32_t RESERVE_C4[7];	/* Address offset: 0x0c4 ~ 0x0dc */
	__IO uint32_t PSRAM_MISC_CFG;	/* Address offset: 0x0e0 */
	__IO uint32_t RESERVE_E4[3];	/* Address offset: 0x0e4 ~ 0x0ec */
	__IO uint32_t C_W_BYTE_NUM;	/* Address offset: 0x0f0 */
	__IO uint32_t C_R_BYTE_NUM;	/* Address offset: 0x0f4 */
	__IO uint32_t S_W_BYTE_NUM;	/* Address offset: 0x0f8 */
	__IO uint32_t S_R_BYTE_NUM;	/* Address offset: 0x0fc */
	__IO uint32_t S_WDATA_REG;	/* Address offset: 0x100 */
	__IO uint32_t RESERVE_104[63];	/* Address offset: 0x104 ~ 0x1fc */
	__IO uint32_t S_RDATA_REG;	/* Address offset: 0x200 */
} PSRAM_CTRL_T;

#define PSRAM_CTRL ((PSRAM_CTRL_T *)PSRAM_CTRL_BASE)

/*
 * PSRAM Controller initialization parameters
 */
typedef struct {
	uint32_t p_type;
	 uint32_t freq;     /*!< PSRAM working frequency */
	 uint8_t rdata_w;   /*!< PSRAM receive data wait cycle(0~3), base on board line length */
} PSRAMCtrl_InitParam;

struct psram_ctrl {
	volatile uint32_t rd_buf_idx;
	volatile uint32_t Psram_WR_FULL;
	volatile uint32_t wait;

	//OS_Semaphore_t lock;
	uint32_t status_int;
	uint32_t inte;
	uint32_t trans_done;
	uint32_t dma_done;

	//OS_Semaphore_t dmaSem;
	//DMA_ChannelInitParam dmaParam;
	//DMA_Channel dma_ch;
	//uint8_t dma_use;
	uint8_t ref;

	uint32_t busconfig;

	uint32_t p_type;        /* psram type */
	uint32_t freq;          /* psram freq */
	uint8_t rdata_w;
	struct psram_request *mrq;
};

int32_t HAL_PsramCtrl_DQS_Delay_Calibration(void);
void HAL_PsramCtrl_MaxCE_LowCyc(struct psram_ctrl *ctrl, uint32_t clk);
int32_t HAL_PsramCtrl_Request(struct psram_ctrl *ctrl, struct psram_request *mrq, uint8_t standby_flag);
void HAL_PsramCtrl_IDbusCfg(struct psram_ctrl *ctrl, uint32_t write, uint32_t opcfg);
void HAL_PsramCtrl_Set_CBUS_WR_LATENCY(struct psram_ctrl *ctrl, uint32_t lat);
void HAL_PsramCtrl_Set_SBUS_WR_LATENCY(struct psram_ctrl *ctrl, uint32_t lat);
uint32_t HAL_PsramCtrl_Set_BusWidth(struct psram_ctrl *ctrl, uint32_t width);
int32_t HAL_PsramCtrl_ConfigCCMU(uint32_t clk);
void HAL_PLL_FREQ_SET(int pll, uint32_t freq);
void HAL_PsramCtrl_CacheCfg(struct psram_ctrl *ctrl, uint32_t cbus_wsize_bus);
void HAL_PsramCtrl_DMACrossEnable(uint32_t en);
void HAL_PsramCtrl_Set_Address_Field(struct psram_ctrl *ctrl, uint32_t id,
		uint32_t startaddr, uint32_t endaddr, uint32_t bias_addr);

HAL_Status HAL_PsramCtrl_Init(struct psram_ctrl *ctrl, const PSRAMCtrl_InitParam *cfg);
HAL_Status HAL_PsramCtrl_Deinit(struct psram_ctrl *ctrl);
struct psram_ctrl *HAL_PsramCtrl_Open(uint32_t id);
HAL_Status HAL_PsramCtrl_Close(struct psram_ctrl *ctrl);
struct psram_ctrl *HAL_PsramCtrl_Create(uint32_t id, const PSRAMCtrl_InitParam *cfg);
HAL_Status HAL_PsramCtrl_Destory(struct psram_ctrl *ctrl);
#endif
