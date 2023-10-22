
#include "hal_gpio.h"
#include "csi_reg.h"

void csi_top_enable(void)
{
	HAL_SET_BIT(CSI->CSI_EN_REG, CSI_PCLK_EN | CSI_NCSIC_EN | CSI_PRS_EN);
}

void csi_top_disable(void)
{
	HAL_CLR_BIT(CSI->CSI_EN_REG, CSI_PCLK_EN | CSI_NCSIC_EN | CSI_PRS_EN);
}

void csi_ini_enable(unsigned int en)
{
	HAL_SET_BIT(CSI->CSI_INT_EN_REG, en);
}

void csi_ini_disable(unsigned int en)
{
	HAL_CLR_BIT(CSI->CSI_INT_EN_REG, en);
}

unsigned int csi_ini_get_status(void)
{
	return CSI->CSI_INT_STA_REG;
}

void csi_ini_clear_status(enum csi_irqstate state)
{
	CSI->CSI_INT_STA_REG = state;
}

void csi_capture_start(enum csi_captype mode)
{
	if (CSI_CAP_STILL == mode)
		HAL_SET_BIT(CSI->CSI_CAP_REG, CSI_SCAP_EN);
	else
		HAL_SET_BIT(CSI->CSI_CAP_REG, CSI_VCAP_EN);
}

void csi_capture_stop(void)
{
	HAL_CLR_BIT(CSI->CSI_CAP_REG, CSI_VCAP_EN | CSI_SCAP_EN);
}

void csi_set_cfg(struct csi_cfg_param *cfg)
{
	uint32_t reg_val;

#if 0
	reg_val = ((cfg->yuv420_mask & 0x1) << CSI_YUV420_MASK_SHIFT) |\
		    ((cfg->out_mode & 0x1) << CSI_OUTPUT_MODE_SHIFT) |\
		    ((cfg->yuv420_line_order  & 0x1) << CSI_YUV420_LINE_ORDER_SHIFT) |\
		    ((cfg->input_seq & 0x3) << CSI_INPUT_SEQ_SHIFT) |\
		    ((cfg->input_fmt & 0x3) << CSI_INPUT_FMT_SHIFT) |\
		    ((cfg->vref_pol & 0x1) << CSI_VREF_POL_SHIFT) |\
		    ((cfg->href_pol & 0x1) << CSI_HREF_POL_SHIFT) |\
		    ((cfg->clk_pol & 0x1) << CSI_CLK_POL_SHIFT) |\
		    ((cfg->sync_type & 0x1) << CSI_SYNC_TYPE_SHIFT);
	CSI->CSI_CFG_REG = reg_val;

	reg_val = ((cfg->hor_len & 0x3fff) << CSI_HOR_LEN_SHIFT) |\
			  ((cfg->hor_start & 0x3fff) << CSI_HOR_START_SHIFT);
	CSI->CSI_C0_HSIZE_REG = reg_val;

	reg_val = ((cfg->ver_len & 0x1fff) << CSI_VER_LEN_SHIFT) |\
			  ((cfg->ver_start & 0x1fff) << CSI_VER_START_SHIFT);
	CSI->CSI_C0_VSIZE_REG = reg_val;

	reg_val = ((0 & 0x1) << CSI_C0_FRATE_HALF_SHIFT) |\
			  ((0 & 0xf) << CSI_C0_FRAME_MASK_SHIFT);
	CSI->CSI_CAP_REG = reg_val;
#endif
	HAL_MODIFY_REG(CSI->CSI_CFG_REG, CSI_SYNC_TYPE_MASK,
			(cfg->sync_type << CSI_SYNC_TYPE_SHIFT) & CSI_SYNC_TYPE_MASK);
	HAL_MODIFY_REG(CSI->CSI_CFG_REG, CSI_CLK_POL_MASK,
			(cfg->clk_pol << CSI_CLK_POL_SHIFT) & CSI_CLK_POL_MASK);
	HAL_MODIFY_REG(CSI->CSI_CFG_REG, CSI_HREF_POL_MASK,
			(cfg->href_pol << CSI_HREF_POL_SHIFT) & CSI_HREF_POL_MASK);
	HAL_MODIFY_REG(CSI->CSI_CFG_REG, CSI_VREF_POL_MASK,
			(cfg->vref_pol << CSI_VREF_POL_SHIFT) & CSI_VREF_POL_MASK);
	HAL_MODIFY_REG(CSI->CSI_CFG_REG, CSI_INPUT_FMT_MASK,
			(cfg->input_fmt << CSI_INPUT_FMT_SHIFT) & CSI_INPUT_FMT_MASK);
	HAL_MODIFY_REG(CSI->CSI_CFG_REG, CSI_INPUT_SEQ_MASK,
			(cfg->input_seq << CSI_INPUT_SEQ_SHIFT) & CSI_INPUT_SEQ_MASK);
	HAL_MODIFY_REG(CSI->CSI_CFG_REG, CSI_YUV420_LINE_ORDER_MASK,
			(cfg->yuv420_line_order << CSI_YUV420_LINE_ORDER_SHIFT) & CSI_YUV420_LINE_ORDER_MASK);
	HAL_MODIFY_REG(CSI->CSI_CFG_REG, CSI_OUTPUT_MODE_MASK,
			(cfg->out_mode << CSI_OUTPUT_MODE_SHIFT) & CSI_OUTPUT_MODE_MASK);
	HAL_MODIFY_REG(CSI->CSI_CFG_REG, CSI_YUV420_MASK_MASK,
			(cfg->yuv420_mask << CSI_YUV420_MASK_SHIFT) & CSI_YUV420_MASK_MASK);

	HAL_MODIFY_REG(CSI->CSI_HSIZE_REG, CSI_HOR_LEN_MASK,
			(cfg->hor_len << CSI_HOR_LEN_SHIFT) & CSI_HOR_LEN_MASK);
	HAL_MODIFY_REG(CSI->CSI_HSIZE_REG, CSI_HOR_START_MASK,
			(cfg->hor_start << CSI_HOR_START_SHIFT) & CSI_HOR_START_MASK);

	HAL_MODIFY_REG(CSI->CSI_VSIZE_REG, CSI_VER_LEN_MASK,
			(cfg->ver_len << CSI_VER_LEN_SHIFT) & CSI_VER_LEN_MASK);
	HAL_MODIFY_REG(CSI->CSI_VSIZE_REG, CSI_VER_START_MASK,
			(cfg->ver_start << CSI_VER_START_SHIFT) & CSI_VER_START_MASK);

	HAL_MODIFY_REG(CSI->CSI_CAP_REG, CSI_FRAME_MASK_MASK,
			(0 << CSI_FRAME_MASK_SHIFT) & CSI_FRAME_MASK_MASK);
	HAL_MODIFY_REG(CSI->CSI_CAP_REG, CSI_FRATE_HALF_MASK,
			(0 << CSI_FRATE_HALF_SHIFT) & CSI_FRATE_HALF_MASK);

}

