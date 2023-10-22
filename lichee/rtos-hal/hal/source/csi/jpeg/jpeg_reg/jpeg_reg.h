#ifndef JPEG_REG_H
#define JPEG_REG_H

#include "hal_def.h"
#include "hal_atomic.h"
#include "platform_csi.h"

//#define     __IO    volatile             /*!< Defines 'read / write' permissions              */
struct  jpeg_reg {
	__IO uint32_t VE_MODE_REG;			/*!< 0x00 JPEG VE mode control register */
	__IO uint32_t VE_RESET_REG; 		/*!< 0x04 JPEG VE reset control register */
	__IO uint32_t ENC_COUNTER_REG;		/*!< 0x08 JPEG VE counter control register */
	__IO uint32_t ENC_OVERTIME_REG; 	/*!< 0x0C JPEG VE overtime register */
		 uint32_t RESERVED0[3];
	__IO uint32_t VE_INT_STA_REG;		/*!< 0x1C JPEG interrupt status register */
	__IO uint32_t CSI_OUTPUT_ADDR_Y;	/*!< 0x20 JPEG CSI output Y address register */
	__IO uint32_t CSI_OUTPUT_ADDR_UV;	/*!< 0x24 JPEG CSI output UV address register */
	__IO uint32_t CSI_OUTPUT_STRIDE;	/*!< 0x28 JPEG CSI output stride control register */

		 uint32_t RESERVED1[245];
	__IO uint32_t INPUT_PIC_SIZE;		/*!< 0x400 JPEG input size register */
	__IO uint32_t JPE_STRIDE_CTRL;		/*!< 0x404 JPEG input stride control register */
		 uint32_t RESERVED2[3];
	__IO uint32_t JPE_STRIDE_CTRL_1;	/*!< 0x414 JPEG input stride1 control register */
		 uint32_t RESERVED3[24];
	__IO uint32_t JPE_INPUT_ADDR_Y; 	/*!< 0x478 JPEG input Y address register */
	__IO uint32_t JPE_INPUT_ADDR_C; 	/*!< 0x47C JPEG input UV address register */

		 uint32_t RESERVED4[225];
	__IO uint32_t JPEG_PARA0_REG;		/*!< 0x804 JPEG para control register */
	__IO uint32_t JPEG_BITRATE_CTRL;	/*!< 0x808 JPEG bitrate control registe */
		 uint32_t RESERVED5[2];
	__IO uint32_t VE_INT_EN_REG;		/*!< 0x814 JPEG VE interrupt enable register */
	__IO uint32_t VE_START_REG; 		/*!< 0x818 JPEG start trigger register */
	__IO uint32_t VE_STA_REG;			/*!< 0x81C JPEG VE status register */
	__IO uint32_t VE_PUTBITS_DATA;		/*!< 0x820 JPEG putbits data register */
	__IO uint32_t MELEVEL_OVERTIME; 	/*!< 0x824 JPEG macroblock level overtime register */
		 uint32_t RESERVED6[22];
	__IO uint32_t OUTSTM_START_ADDR;	/*!< 0x880 JPEG output stream start address register */
	__IO uint32_t OUTSTM_END_ADDR;		/*!< 0x884 JPEG output stream end address register */
	__IO uint32_t OUTSTM_OFFSET;		/*!< 0x888 JPEG output stream offset register */
	__IO uint32_t OUTSTM_VSIZE; 		/*!< 0x88C JPEG output stream valid size */
	__IO uint32_t HARDWARE_OFFSET;		/*!< 0x890 JPEG output stream length register */ //OUTSTM_LEN
		 uint32_t RESERVED7[19];
	__IO uint32_t QM_INDEX; 			/*!< 0x8E0 JPEG VE quantiser matrix index register */
	__IO uint32_t QM_DATA;				/*!< 0x8E4 JPEG VE quantiser matrix input data register */
} ;

#define	JPEG	((struct jpeg_reg *)JPEG_BASE)

/*
 * Bits definition for JPEG VE mode control register (0x00)
 */
#define JPEG_MEM_PART_TAKE			HAL_BIT(17)
#define JPEG_MEM_PART_MODE			HAL_BIT(16)

#define JPEG_MEM_PART_NUM_SHIFT		(14)
#define JPEG_MEM_PART_NUM_MASK		(0x3U << JPEG_MEM_PART_NUM_SHIFT)
enum jpeg_mempartnum {
	JPEG_MEM_BLOCK2,
	JPEG_MEM_BLOCK4,
	JPEG_MEM_BLOCK8,
};

#define JPEG_AHB_WRITE_BL_8W		HAL_BIT(13)
#define JPEG_AHB_READ_BL_8W			HAL_BIT(12)
#define JPEG_HEIGHT_HALF			HAL_BIT(11)
#define JPEG_WIDTH_HALF				HAL_BIT(10)
#define JPEG_ONLINE_MODE_EN			HAL_BIT(9)	/*!< 0: offline mode 1: online mode */
#define JPEG_INPUT_FMT				HAL_BIT(8)	/*!< 0: JPEG input data format is yuv420 data(NV12) 1: jpeg data */
#define JPEG_ENC_CLK_EN				HAL_BIT(7)
#define JPEG_JPE_CLK_EN				HAL_BIT(6)
#define JPEG_CLK_GATING_DISEN		HAL_BIT(5)

/*
 * Bits definition for JPEG VE reset control register (0x04)
 */
#define JPEG_VE_RESET				HAL_BIT(0)

/*
 * Bits definition for JPEG VE counter control register (0x08)
 */
#define JPEG_VE_CNT_EN				HAL_BIT(31)

#define JPEG_VE_CNT_SHIFT			(0)
#define JPEG_VE_CNT_MASK			(0x7FFFFFFFU << JPEG_VE_CNT_SHIFT)

/*
 * Bits definition for JPEG VE overtime register (0x0c)
 */
#define JPEG_VE_OVERTIME_SHIFT		(8)
#define JPEG_VE_OVERTIME_MASK		(0x7FFFFFU << JPEG_VE_CNT_SHIFT)

/*
 * Bits definition for JPEG interrupt status register (0x1c)
 */
#define JPEG_AHB_SYNC_IDLE			HAL_BIT(25)
#define JPEG_CSI_WB_FINISH			HAL_BIT(24)
#define JPEG_FIFO_OVERFLOW			HAL_BIT(23)
#define JPEG_CSI_ERROR				HAL_BIT(22)
#define JPEG_CSI_TIMEOUT			HAL_BIT(21)
#define JPEG_MEM_PART_OVERFLOW		HAL_BIT(10)
#define JPEG_MEM_PART_INT			HAL_BIT(9)
#define JPEG_VE_TIMEOUT_EN			HAL_BIT(8)
#define JPEG_MB_OVERTIME			HAL_BIT(7)
#define JPEG_BS_STALL				HAL_BIT(6)
#define JPEG_ENC_FINISH				HAL_BIT(3)
#define JPEG_CSI_FRAME_END			HAL_BIT(2)
#define JPEG_CSI_SIZE_CHG			HAL_BIT(1)
#define JPEG_VE_TIMEOUT				HAL_BIT(0)
#define JPEG_VE_INT_ALL				0XFFFFFFFF
#define JPEG_INT_ERR				(JPEG_CSI_ERROR | JPEG_CSI_TIMEOUT | JPEG_VE_TIMEOUT_EN | \
										JPEG_MB_OVERTIME | JPEG_CSI_SIZE_CHG)
/*
 * Bits definition for JPEG CSI output Y address register (0x20)
 */
#define JPEG_CSI_OUT_Y_ADDR_SHIFT			(4)
#define JPEG_CSI_OUT_Y_ADDR_MASK			(0xFFFFFFFU << 4)

/*
 * Bits definition for JPEG CSI output UV address register (0x24)
 */
#define JPEG_CSI_OUT_UV_ADDR_SHIFT			(3)
#define JPEG_CSI_OUT_UV_ADDR_MASK			(0x1FFFFFFFU << 3)

/*
 * Bits definition for JPEG CSI output stride control register (0x28)
 */
#define JPEG_CSI_OUT_Y_STRIDE_DIV16_SHIFT			(16)
#define JPEG_CSI_OUT_Y_STRIDE_DIV16_MASK			(0x7FFU << JPEG_CSI_OUT_Y_STRIDE_DIV16_SHIFT)

#define JPEG_CSI_OUT_UV_STRIDE_DIV8_SHIFT			(0)
#define JPEG_CSI_OUT_UV_STRIDE_DIV8_MASK			(0xFFFU << JPEG_CSI_OUT_UV_STRIDE_DIV8_SHIFT)

/*
 * Bits definition for JPEG input size register (0x400)
 */
#define JPEG_PIC_WIDTH_IN_8X8B_SHIFT			(16)
#define JPEG_PIC_WIDTH_IN_8X8B_MASK				(0x7FFU << JPEG_PIC_WIDTH_IN_8X8B_SHIFT)

#define JPEG_PIC_HEIGHT_IN_8X8B_SHIFT			(0)
#define JPEG_PIC_HEIGHT_IN_8X8B_MASK			(0x7FFU << JPEG_PIC_HEIGHT_IN_8X8B_SHIFT)

/*
 * Bits definition for JPEG input stride control register (0x404)
 */
#define JPEG_IN_STRIDE_DIV16_SHIFT				(16)
#define JPEG_IN_STRIDE_DIV16_MASK				(0x7FFU << JPEG_IN_STRIDE_DIV16_SHIFT)

/*
 * Bits definition for JPEG input stride1 control register (0x414)
 */
#define JPEG_IN_C_STRIDE_DIV8_SHIFT				(0)
#define JPEG_IN_C_STRIDE_DIV8_MASK				(0xFFFU << JPEG_IN_C_STRIDE_DIV8_SHIFT)

/*
 * Bits definition for JPEG input Y address register (0x478)
 */
//16B align

/*
 * Bits definition for JPEG input UV address register (0x47c)
 */
//8B align

/*
 * Bits definition for JPEG para control register (0x804)
 */
#define JPEG_STUFF_ZERO_EN						HAL_BIT(30)

#define JPEG_DC_CHRO_SHIFT						(16)
#define JPEG_DC_CHRO_MASK						(0x7FFU << JPEG_DC_CHRO_SHIFT)

#define JPEG_DC_LUMA_SHIFT						(0)
#define JPEG_DC_LUMA_MASK						(0x7FFU << JPEG_DC_LUMA_SHIFT)

/*
 * Bits definition for JPEG bitrate control register (0x808)
 */
#define JPEG_RUN_LENGTH_OPT_DISEN				HAL_BIT(31)
#define JPEG_COEF_DOWN_SAMPLE_DISEN				HAL_BIT(30)

#define JPEG_RUN_LENGTH_TH_SHIFT				(8)
#define JPEG_RUN_LENGTH_TH_MASK					(0xFU << JPEG_RUN_LENGTH_TH_SHIFT)

#define JPEG_CLASSFY_TH_SHIFT					(0)
#define JPEG_CLASSFY_TH_MASK					(0xFFU << JPEG_CLASSFY_TH_SHIFT)

/*
 * Bits definition for JPEG VE interrupt enable register (0x814)
 */
#define JPEG_MB_X_CURR_SHIFT					(24)
#define JPEG_MB_X_CURR_MASK						(0xFU << JPEG_MB_X_CURR_SHIFT)

#define JPEG_MB_Y_CURR_SHIFT					(16)
#define JPEG_MB_Y_CURR_MASK						(0xFU << JPEG_MB_Y_CURR_SHIFT)

#define JPEG_OVERTIME_INT_EN					HAL_BIT(2)
#define JPEG_BS_STALL_INT_EN					HAL_BIT(1)
#define JPEG_VE_FINISH_INT_EN					HAL_BIT(0)

/*
 * Bits definition for JPEG start trigger register (0x818)
 */
#define JPEG_PUTBITS_LEN_SHIFT					(8)
#define JPEG_PUTBITS_LEN_MASK					(0x3FU << JPEG_PUTBITS_LEN_SHIFT)

#define JPEG_VE_ENC_START_SHIFT					(0)	/*!< 1000: VE encode start */
#define JPEG_VE_ENC_START_MASK					(0xFU << JPEG_VE_ENC_START_SHIFT)

/*
 * Bits definition for JPEG VE status register (0x81c)
 */
#define JPEG_PIC_ENC_BUSY						HAL_BIT(31)
#define JPEG_READ_MB_BUSY						HAL_BIT(23)
#define JPEG_IPTIT_BUSY							HAL_BIT(13)
#define JPEG_VLC_BUSY							HAL_BIT(12)
#define JPEG_PUTBITS_STATUS						HAL_BIT(9)
#define JPEG_ENC_MEM_BUSY						HAL_BIT(7)
#define JPEG_ENC_ISP							HAL_BIT(6)

/*
 * Bits definition for JPEG putbits data register (0x820)
 */

/*
 * Bits definition for JPEG macroblock level overtime register (0x824)
 */
#define JPEG_MBOVERTIME_EN						HAL_BIT(15)

#define JPEG_MBOVERTIME_THRESHOLD_SHIFT			(12)
#define JPEG_MBOVERTIME_THRESHOLD_MASK			(0x7U << JPEG_MBOVERTIME_THRESHOLD_SHIFT)

/*
 * Bits definition for JPEG output stream start address register (0x880)
 */

/*
 * Bits definition for JPEG output stream end address register (0x884)
 */

/*
 * Bits definition for JPEG output stream offset register (0x888)
 */
#define JPEG_STREAM_OFFSET_SHIFT				(0)
#define JPEG_STREAM_OFFSET_MASK					(0xFFFFFFFU << JPEG_STREAM_OFFSET_SHIFT)

/*
 * Bits definition for JPEG output stream valid size register (0x88c)
 */
#define JPEG_STREAM_VALID_SIZE_SHIFT			(16)
#define JPEG_STREAM_VALID_SIZE_MASK				(0xFFFU << JPEG_STREAM_VALID_SIZE_SHIFT)

/*
 * Bits definition for JPEG output stream length register (0x890)
 */
#define JPEG_OUTSTM_LEN_SHIFT					(0)
#define JPEG_OUTSTM_LEN_MASK					(0xFFFFFFFU << JPEG_STREAM_OFFSET_SHIFT)

/*
 * Bits definition for JPEG VE quantiser matrix index register (0x8e0)
 */
#define JPEG_QM_INDEX_SHIFT						(0)
#define JPEG_QM_INDEX_MASK						(0x7FU << JPEG_QM_INDEX_SHIFT)

/*
 * Bits definition for JPEG VE quantiser matrix input data register (0x8e4)
 */

#define JPEG_BUFF_CNT_MAX 	(3)

typedef enum {
	JPEG_MOD_OFFLINE = 0,
	JPEG_MOD_ONLINE,
} JPEG_Mode;

typedef struct {
	uint8_t 		sensor_out_type;
	uint8_t			jpeg_en;
	JPEG_Mode 		jpeg_mode;//online offline
	uint8_t			output_mode;
	uint8_t			jpeg_bitrate_en;
	uint8_t			jpeg_scale;
	uint32_t		csi_output_addr_y;
	uint32_t		csi_output_addr_uv;

	uint32_t		pic_size_width;
	uint32_t		pic_size_height;

	uint32_t		jpeg_input_addr_y;
	uint32_t		jpeg_input_addr_uv;

	uint32_t		outstream_buff_addr[JPEG_BUFF_CNT_MAX];
	uint32_t		outstream_buff_offset;
	uint32_t		outstream_buff_size;
	uint8_t			outstream_buff_num;

	uint8_t 		mem_part_en;
	enum jpeg_mempartnum	mem_part_num;

	uint32_t		quality;
} jpeg_cfg_param;

typedef enum {
	JPEG_MEM_PART_DIS,
	JPEG_MEM_PART_EN,
} jpeg_mempartmode;

/*
typedef struct {
	uint8_t id;
	uint8_t *index;
	uint8_t tail;
	uint32_t size;
} CSI_JPEG_StreamInfo; */

#if 0
typedef struct {
	uint8_t buff_index; 	/* Indicate which buffer the currently encoded part jpeg is stored in */
	uint32_t buff_offset; 	/* Indicate the offset of the current part of jpeg in the buffer */
	uint8_t tail; 			/* Indicates whether it is the last part of a jpeg image */
	uint32_t size; 			/* Indicate the size of the current part of jpeg encoding */
} jpeg_mpartbuffinfo;
#endif

typedef struct {
	uint8_t buff_index; 	/* Indicate which buffer the currently encoded jpeg is stored in */
	uint32_t size; 			/* Indicate the currently encoded jpeg size */
} jpeg_buffinfo;


unsigned int jpeg_ini_get_status(void);
void jpeg_enc_start(void);
void hal_jpeg_clk_en(void);
void jpeg_int_clear_status(unsigned int state);
void hal_ve_rst_ctl_release(void);
void hal_ve_rst_ctl_reset(void);
void hal_jpeg_clk_en(void);
void jpeg_enc_start(void);
void jpeg_mempart_take(void);

#endif  /*JPEG_REG_H*/
