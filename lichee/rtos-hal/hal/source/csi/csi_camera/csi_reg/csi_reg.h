
#ifndef __CSI_REG_H_
#define __CSI_REG_H_

#include "hal_def.h"
#include "hal_atomic.h"

#include "platform_csi.h"

/**
  * @brief The register for CSI.
  */
struct csi_reg {
	__IO uint32_t CSI_EN_REG;          /*!< 0x00 CSI enable register */
	__IO uint32_t CSI_CFG_REG;         /*!< 0x04 CSI configuration register */
	__IO uint32_t CSI_CAP_REG;         /*!< 0x08 CSI capture control register */
	__I  uint32_t CSI_SIGNAL_STA_REG;  /*!< 0x0C CSI signal status register */
	__IO uint32_t CSI_HSIZE_REG;    /*!< 0x10 CSI horizontal size register */
	__IO uint32_t CSI_VSIZE_REG;    /*!< 0x14 CSI vertical size register */
	__IO uint32_t CSI_IN_SIZE_REG;  /*!< 0x18 CSI input size register */
	__IO uint32_t CSI_INT_EN_REG;   /*!< 0x1C CSI interrupt enable register */
	__IO uint32_t CSI_INT_STA_REG;  /*!< 0x20 CSI interrupt status register */
};

#define	CSI  ((struct csi_reg *)CSI_BASE)

/*
 * Bits definition for CSI enable register (0x0000)
 */
#define CSI_PCLK_EN				HAL_BIT(2)
#define CSI_NCSIC_EN				HAL_BIT(1)
#define CSI_PRS_EN				HAL_BIT(0)

/*
 * Bits definition for CSI configuration register (0x0004)
 */
#define CSI_YUV420_MASK_SHIFT			(12) /*!< valid when output mode set YUV422 to YUV420 */
#define CSI_YUV420_MASK_MASK			(0x1U << CSI_YUV420_MASK_SHIFT)
enum csi_yuv420mask {
	CSI_YUV420_MASK_UV_ODD = 0,
	CSI_YUV420_MASK_UV_EVEN,
};

#define CSI_OUTPUT_MODE_SHIFT			(11) /*!< 0: original output 1: YUV422 to YUV420 */
#define CSI_OUTPUT_MODE_MASK			(0x1U << CSI_OUTPUT_MODE_SHIFT)
enum csi_outputmode {
	CSI_OUT_MODE_ORIGINAL = 0,
	CSI_OUT_MODE_YUV422_TO_YUV420,
};

#define CSI_YUV420_LINE_ORDER_SHIFT		(10) /*!< YUV420 line order */
#define CSI_YUV420_LINE_ORDER_MASK		(0x1U << CSI_YUV420_LINE_ORDER_SHIFT)
enum csi_yuv420lineorder {
	CSI_LINE_ORDER_Y_YC_Y_YC = 0,
	CSI_LINE_ORDER_YC_Y_YC_Y,
};

#define CSI_INPUT_SEQ_SHIFT			(8) /*!< input data sequence, only valid for YUV422 and YUV420 input format */
#define CSI_INPUT_SEQ_MASK			(0x3U << CSI_INPUT_SEQ_SHIFT)
enum csi_inputseq{
	CSI_IN_SEQ_YUYV = 0,
	CSI_IN_SEQ_YVYU,
	CSI_IN_SEQ_UYVY,
	CSI_IN_SEQ_VYUY,
};

#define CSI_INPUT_FMT_SHIFT			(6) /*!< input data format */
#define CSI_INPUT_FMT_MASK			(0x3U << CSI_INPUT_FMT_SHIFT)
enum csi_inputfmt {
	CSI_IN_FMT_RAW = 0,
	CSI_IN_FMT_YUV422 = 0x2,
	CSI_IN_FMT_YUV420 = 0x3,
};

#define CSI_VREF_POL_SHIFT			(3)
#define CSI_VREF_POL_MASK			(0x1U << CSI_VREF_POL_SHIFT) /*!< Vref polarity */
#define CSI_HREF_POL_SHIFT			(2)
#define CSI_HREF_POL_MASK			(0x1U << CSI_HREF_POL_SHIFT) /*!< Href polarity */
enum csi_signalpol {
	CSI_POL_NEGATIVE = 0, /*!< Negative*/
	CSI_POL_POSITIVE, /*!< Positive*/
};

#define CSI_CLK_POL_SHIFT			(1)
#define CSI_CLK_POL_MASK			(0x1U << CSI_CLK_POL_SHIFT)  /*!< data clock type */
enum csi_clkpol {
	CSI_ACTIVE_RISING = 0,
	CSI_ACTIVE_FALLING,
};

#define CSI_SYNC_TYPE_SHIFT			(0)
#define CSI_SYNC_TYPE_MASK			(0x1U << CSI_SYNC_TYPE_SHIFT) /*!< sync type */
enum csi_synctype {
	CSI_SYNC_SEPARARE = 0, /*!< Negative*/
	CSI_SYNC_CCIR656,  /*!< Positive*/
};
/*
 * Bits definition for CSI capture control register (0x0008)
 */
#define CSI_FRATE_HALF_SHIFT			(6) /*!< frame rate half down */
#define CSI_FRATE_HALF_MASK			(0x1U << CSI_FRATE_HALF_SHIFT)

#define CSI_FRAME_MASK_SHIFT			(2)
#define CSI_FRAME_MASK_MASK			(0xFU << CSI_FRAME_MASK_SHIFT) /*!< frame number masked before capture */

#define CSI_FRATE_HALF_EN				HAL_BIT(6)
#define CSI_VCAP_EN				HAL_BIT(1) /*!< video capture control: capture the video image data stream on channel 0 */
#define CSI_SCAP_EN				HAL_BIT(0) /*!< still capture control: capture a single still image frame on channel 0 */
enum csi_captype {
	CSI_CAP_STILL,
	CSI_CAP_VIDEO,
};

/*
 * Bits definition for CSI signal status register (0x000c)
 */
#define CSI_SIGNAL_PCLK_STA_SHIFT		(24)
#define CSI_SIGNAL_PCLK_MASK			(0xFU << CSI_SIGNAL_PCLK_STA_SHIFT)
#define CSI_SIGNAL_DATA_STA_SHIFT		(0)
#define CSI_SIGNAL_DATA_MASK			(0xFFFFFFU << CSI_SIGNAL_DATA_STA_SHIFT)

/*
 * Bits definition for CSI horizontal size register (0x0010)
 */
#define CSI_HOR_LEN_SHIFT			(16)
#define CSI_HOR_LEN_MASK			(0x3FFFU << CSI_HOR_LEN_SHIFT) /*!< horizontal pixel unit length. valid pixel of a line */
#define CSI_HOR_START_SHIFT			(0)
#define CSI_HOR_START_MASK			(0x3FFFU << CSI_HOR_START_SHIFT) /*!< horizontal pixel unit start. pixel is valid from this pixel */

/*
 * Bits definition for CSI vertical size register (0x0014)
 */
#define CSI_VER_LEN_SHIFT			(16)
#define CSI_VER_LEN_MASK			(0x1FFFU << CSI_VER_LEN_SHIFT) /*!< valid line number of a line */
#define CSI_VER_START_SHIFT			(0)
#define CSI_VER_START_MASK			(0x1FFFU << CSI_VER_START_SHIFT) /*!< vertical line start. data is valid from this line */

/*
 * Bits definition for CSI input size register (0x0018)
 */
#define CSI_INPUT_SIZE_Y_SHIFT		(16)
#define CSI_INPUT_SIZE_Y_MASK		(0x1FFFU << CSI_INPUT_SIZE_Y_SHIFT)
#define CSI_INPUT_SIZE_X_SHIFT		(16)
#define CSI_INPUT_SIZE_X_MASK		(0x1FFFU << CSI_INPUT_SIZE_X_SHIFT)

/*
 * Bits definition for CSI interrupt enable register (0x001c)
 */
#define CSI_INT_FRAME_END_EN			HAL_BIT(1)
#define CSI_INT_INPUT_SIZE_CHG_EN		HAL_BIT(0)

/*
 * Bits definition for CSI interrupt status register (0x0020)
 */
#define CSI_INT_STA_FRM_END_PD			HAL_BIT(1)
#define CSI_INT_STA_INPUT_SIZE_PD		HAL_BIT(0)
#define CSI_INT_STA_ALL_PD				0XFFFFFFFF

enum csi_irqstate {
	CSI_IRQ_INPUT_SIZE_CHG = 0x1,
	CSI_IRQ_FRM_END = 0x2,
	CSI_IRQ_ALL = 0x3,
};

/**
  * struct for csi.
  */

#if 0
hal_spinlock_t csi_jpeg_lock;
#define HAL_EnterCriticalSection()  ({hal_spin_lock_irqsave(&csi_jpeg_lock);})
#define HAL_ExitCriticalSection(f)  ({hal_spin_unlock_irqrestore(&csi_jpeg_lock, f);})
#endif

enum csi_state{
	CSI_STATE_INVALID	= 0,
	CSI_STATE_INIT		= 1, /* Initializing */
	CSI_STATE_DEINIT	= 2, /* Deinitializing */
	CSI_STATE_READY		= 3,
	CSI_STATE_BUSY		= 4
};

/**
  * @brief Enable or disable func.
  */
enum csi_ctrl {
	CSI_DISABLE,    /*!< Enable*/
	CSI_ENABLE,     /*!< Disable*/
};

struct csi_cfg_param {
	enum csi_yuv420mask yuv420_mask;
	enum csi_outputmode out_mode;
	enum csi_yuv420lineorder yuv420_line_order;
	enum csi_inputseq input_seq;
	enum csi_inputfmt input_fmt;
	enum csi_signalpol vref_pol;
	enum csi_signalpol href_pol;
	enum csi_clkpol clk_pol;
	enum csi_synctype sync_type;

	uint16_t hor_len;
	uint16_t hor_start;
	uint16_t ver_len;
	uint16_t ver_start;
};

void csi_top_enable(void);
void csi_top_disable(void);
void csi_ini_enable(unsigned int en);
void csi_ini_disable(unsigned int en);
unsigned int  csi_ini_get_status(void);
void csi_ini_clear_status(enum csi_irqstate state);
void csi_capture_start(enum csi_captype mode);
void csi_capture_stop(void);
void csi_set_cfg(struct csi_cfg_param *cfg);

#endif
