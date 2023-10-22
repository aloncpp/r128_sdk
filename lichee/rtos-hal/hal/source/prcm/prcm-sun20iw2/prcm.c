/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 *the the People's Republic of China and other countries.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hal_clk.h>
#include <hal_time.h>
#include "prcm.h"

/* Voltage Calibration */
static uint32_t HAL_PRCM_GetDCDCCalibrationValue(void)
{
	return HAL_GET_BIT_VAL(*(volatile uint32_t *)(long)EFUSE_SMPS_VSEL_ADDR, EFUSE_SMPS_VSEL_SHIFT, EFUSE_SMPS_VSEL_MASK);
}

static uint32_t HAL_PRCM_GetAONLDOCalibrationValue(void)
{
	return HAL_GET_BIT_VAL(*(volatile uint32_t *)(long)EFUSE_LDO_ADDR, EFUSE_LDO_AON_SHIFT, EFUSE_LDO_MASK);
}

static uint32_t HAL_PRCM_GetAPPLDOCalibrationValue(void)
{
	return HAL_GET_BIT_VAL(*(volatile uint32_t *)(long)EFUSE_LDO_ADDR, EFUSE_LDO_APP_SHIFT, EFUSE_LDO_MASK);
}

static uint32_t HAL_PRCM_GetDSPLDOCalibrationValue(void)
{
	return HAL_GET_BIT_VAL(*(volatile uint32_t *)(long)EFUSE_LDO_ADDR, EFUSE_LDO_DSP_SHIFT, EFUSE_LDO_MASK);
}


/*
 * Power
 *   - DCDC
 *   - Power switch
 *   - LDO
 */
/*
 * bit field definition of PRCM->DCDC_CTRL0
 */


uint32_t HAL_PRCM_GetDCDCVoltage(void)
{
	return PRCM_DCDC_VOLT_VAL2MV_DELTA(
		HAL_GET_BIT_VAL(PRCM->DCDC_CTRL0, PRCM_DCDC_VOLT_SHIFT, \
			PRCM_DCDC_VOLT_VMASK), HAL_PRCM_GetDCDCCalibrationValue());
}

int HAL_PRCM_SetDCDCVoltage(uint32_t volt)
{
	if (volt > PRCM_DCDC_VOLT_MAX_MV || volt < PRCM_DCDC_VOLT_MIN_MV)
		return -4;

	if (HAL_PRCM_GetDCDCCalibrationValue()) {
		HAL_MODIFY_REG(PRCM->DCDC_CTRL0, PRCM_DCDC_VOLT_MASK,
				(HAL_PRCM_GetDCDCCalibrationValue() + PRCM_DCDC_VOLT_MV2VAL_DELTA(volt)) << PRCM_DCDC_VOLT_SHIFT);
	} else {
		HAL_MODIFY_REG(PRCM->DCDC_CTRL0, PRCM_DCDC_VOLT_MASK,
			PRCM_DCDC_VOLT_MV2VAL(volt)<<PRCM_DCDC_VOLT_SHIFT);
	}

	return 0;
}

void HAL_PRCM_SetDCDCOffFlowAONLDOEnable(uint8_t enable)
{
	/*
	 * Note: the reverse operation(!),
	 * because the function has the opposite meaning of the register bit.
	 */
	if (!enable)
		HAL_SET_BIT(PRCM->DCDC_CTRL0, PRCM_TOPLDO_OFF_DCDC_ON_BIT);
	else
		HAL_CLR_BIT(PRCM->DCDC_CTRL0, PRCM_TOPLDO_OFF_DCDC_ON_BIT);
}


uint32_t HAL_PRCM_GetDCDCOffFlowAONLDOStatus(void)
{
	/*
	 * Note: the reverse operation(!),
	 * because the function has the opposite meaning of the register bit.
	 */
	return !HAL_GET_BIT(PRCM->DCDC_CTRL0, PRCM_TOPLDO_OFF_DCDC_ON_BIT);
}

void HAL_PRCM_SetDCDCOffWhenSYSStandbyEnable(uint8_t enable)
{
	if (enable)
		HAL_SET_BIT(PRCM->DCDC_CTRL0, PRCM_SYS_STANDBY_DCDC_OFF_BIT);
	else
		HAL_CLR_BIT(PRCM->DCDC_CTRL0, PRCM_SYS_STANDBY_DCDC_OFF_BIT);
}


uint32_t HAL_PRCM_GetDCDCOffWhenSYSStandbyStatus(void)
{
	return !!HAL_GET_BIT(PRCM->DCDC_CTRL0, PRCM_SYS_STANDBY_DCDC_OFF_BIT);
}

void HAL_PRCM_SetDCDCPwmSelEnable(uint8_t enable)
{
	if (enable) {
		HAL_SET_BIT(PRCM->DCDC_CTRL0, PRCM_DCDC_PWM_SEL_EN_BIT);
	} else {
		HAL_CLR_BIT(PRCM->DCDC_CTRL0, PRCM_DCDC_PWM_SEL_EN_BIT);
	}
}

uint32_t HAL_PRCM_IsDCDCPwmSelEnabled(void)
{
	return !!HAL_GET_BIT(PRCM->DCDC_CTRL0, PRCM_DCDC_PWM_SEL_EN_BIT);
}

void HAL_PRCM_SelectDCDCPwmSrc(PRCM_DCDCPwmSrc src)
{
	HAL_MODIFY_REG(PRCM->DCDC_CTRL0, PRCM_DCDC_PWM_SRC_MASK, src);
}

uint32_t HAL_PRCM_GetOvrDCDCDetctStatus(void)
{
	return !!HAL_GET_BIT(PRCM->DCDC_CTRL0, PRCM_OVR_DCDC_DETECT_BIT);
}

void HAL_PRCM_SetOvrDCDCDetctEnable(uint8_t enable)
{
	if (enable)
		HAL_SET_BIT(PRCM->DCDC_CTRL0, PRCM_OVR_DCDC_DETECT_BIT);
	else
		HAL_CLR_BIT(PRCM->DCDC_CTRL0, PRCM_OVR_DCDC_DETECT_BIT);
}

uint32_t HAL_PRCM_GetOvrDCDCDetctValueStatus(void)
{
	return !!HAL_GET_BIT(PRCM->DCDC_CTRL0, PRCM_DCDC_DETECT_VALUE_BIT);
}


uint32_t HAL_PRCM_SetOvrDCDCDetctValueEnable(uint8_t enable)
{
	if (enable)
		HAL_SET_BIT(PRCM->DCDC_CTRL0, PRCM_DCDC_DETECT_VALUE_BIT);
	else
		HAL_CLR_BIT(PRCM->DCDC_CTRL0, PRCM_DCDC_DETECT_VALUE_BIT);
}

uint32_t HAL_PRCM_GetDCDCDetctStatus(void)
{
	return !!HAL_GET_BIT(PRCM->DCDC_CTRL0, PRCM_DCDC_DETECT_BIT);
}

/*
 * bit field definition of PRCM->DCDC_CTRL1
 */

uint32_t HAL_PRCM_GetDCDCWorkMode(void)
{
	/*
	 * return 0: PWM Mode, 1: PFM Mode
	 */
	return !!HAL_GET_BIT(PRCM->DCDC_CTRL1, PRCM_DCDC_WORK_MODE_BIT);
}



/*
 * bit field definition of PRCM->DCDC_LDO_MODE_SW_SEL
 */
void HAL_PRCM_SetDSPLDOLQModeEnable(uint8_t enable)
{
	if (enable)
		HAL_SET_BIT(PRCM->DCDC_LDO_MODE_SW_SEL, PRCM_DSP_LDO_LQ_MODE_BIT);
	else
		HAL_CLR_BIT(PRCM->DCDC_LDO_MODE_SW_SEL, PRCM_DSP_LDO_LQ_MODE_BIT);
}


void HAL_PRCM_SetAPPLDOLQModeEnable(uint8_t enable)
{
	if (enable)
		HAL_SET_BIT(PRCM->DCDC_LDO_MODE_SW_SEL, PRCM_APP_LDO_LQ_MODE_BIT);
	else
		HAL_CLR_BIT(PRCM->DCDC_LDO_MODE_SW_SEL, PRCM_APP_LDO_LQ_MODE_BIT);
}


void HAL_PRCM_SetAONLDOLQModeEnable(uint8_t enable)
{
	if (enable)
		HAL_SET_BIT(PRCM->DCDC_LDO_MODE_SW_SEL, PRCM_AON_LDO_LQ_MODE_BIT);
	else
		HAL_CLR_BIT(PRCM->DCDC_LDO_MODE_SW_SEL, PRCM_AON_LDO_LQ_MODE_BIT);
}

void HAL_PRCM_SetLDOModeSWSelEnable(uint8_t enable)
{
	if (enable)
		HAL_SET_BIT(PRCM->DCDC_LDO_MODE_SW_SEL, PRCM_LDO_MODE_SW_OVERIDE_BIT);
	else
		HAL_CLR_BIT(PRCM->DCDC_LDO_MODE_SW_SEL, PRCM_LDO_MODE_SW_OVERIDE_BIT);
}

void HAL_PRCM_SetDCDCPFMModeEnable(uint8_t enable)
{
	if (enable)
		HAL_SET_BIT(PRCM->DCDC_LDO_MODE_SW_SEL, PRCM_DCDC_PFM_MODE_BIT);
	else
		HAL_CLR_BIT(PRCM->DCDC_LDO_MODE_SW_SEL, PRCM_DCDC_PFM_MODE_BIT);
}

void HAL_PRCM_SetDCDCModeSWSelEnable(uint8_t enable)
{
	if (enable)
		HAL_SET_BIT(PRCM->DCDC_LDO_MODE_SW_SEL, PRCM_DCDC_MODE_SW_OVERIDE_BIT);
	else
		HAL_CLR_BIT(PRCM->DCDC_LDO_MODE_SW_SEL, PRCM_DCDC_MODE_SW_OVERIDE_BIT);
}


/*
 * bit field definition of PRCM->RTC_LDO_CTRL
 */
void HAL_PRCM_SetRTCLDOSlpVolt(PRCM_RTCLDOSlpVolt volt)
{
	HAL_MODIFY_REG(PRCM->RTC_LDO_CTRL, PRCM_RTC_LDO_SLEEP_VOLT_MASK, volt);
}

void HAL_PRCM_SetRTCLDOActVolt(PRCM_RTCLDOActVolt volt)
{
	HAL_MODIFY_REG(PRCM->RTC_LDO_CTRL, PRCM_RTC_LDO_ACTIVE_VOLT_MASK, volt);
}


/*
 * bit field definition of PRCM->EXT_LDO_CTRL
 */

void HAL_PRCM_SetEXTLDOSwTrim(uint32_t trim)
{
	/* only 2 bits*/
	HAL_MODIFY_REG(PRCM->EXT_LDO_CTRL, PRCM_EXT_LDO_SW_TRIM_MASK, \
			(trim & PRCM_EXT_LDO_SW_TRIM_VMASK) << PRCM_EXT_LDO_SW_TRIM_SHIFT);
}

void HAL_PRCM_SetEXTLDOSwModeEnable(uint8_t enable)
{
	if (enable)
		HAL_SET_BIT(PRCM->EXT_LDO_CTRL, PRCM_EXT_LDO_SW_MODE_BIT);
	else
		HAL_CLR_BIT(PRCM->EXT_LDO_CTRL, PRCM_EXT_LDO_SW_MODE_BIT);
}

void HAL_PRCM_SelectEXTLDOVolt(PRCM_EXTLDOVolt volt)
{
	HAL_MODIFY_REG(PRCM->EXT_LDO_CTRL, PRCM_EXT_LDO_VOLT_MASK, volt);
}

void HAL_PRCM_SelectEXTLDOMode(PRCM_ExtLDOMode mode)
{
	HAL_MODIFY_REG(PRCM->EXT_LDO_CTRL, PRCM_EXT_LDO_MODE_MASK, mode);
}

/*
 * bit field definition of PRCM->TOP_LDO_CTRL
 */

/* NOT TODO */

/*
 * bit field definition of PRCM->AON_LDO_CTRL
 */

uint32_t HAL_PRCM_GetAONLDOWorkVoltage(void)
{
	return PRCM_AON_LDO_WORK_VOLT_VAL2MV_DELTA( \
		HAL_GET_BIT_VAL(PRCM->AON_LDO_CTRL, \
			PRCM_AON_LDO_WORK_VOLT_SHIFT, \
			PRCM_AON_LDO_WORK_VOLT_VMASK), HAL_PRCM_GetAONLDOCalibrationValue());
}

int HAL_PRCM_SetAONLDOWorkVoltage(uint32_t volt)
{
	if (volt > PRCM_AON_LDO_WORK_VOLT_MAX_MV || volt < PRCM_AON_LDO_WORK_VOLT_MIN_MV)
		return -4;

	if (HAL_PRCM_GetAONLDOCalibrationValue()) {
		HAL_MODIFY_REG(PRCM->AON_LDO_CTRL, \
				PRCM_AON_LDO_WORK_VOLT_MASK,
				(HAL_PRCM_GetAONLDOCalibrationValue() + PRCM_AON_LDO_WORK_VOLT_MV2VAL_DELTA(volt)) << PRCM_AON_LDO_WORK_VOLT_SHIFT);
	} else {
		HAL_MODIFY_REG(PRCM->AON_LDO_CTRL, \
			PRCM_AON_LDO_WORK_VOLT_MASK,
			PRCM_AON_LDO_WORK_VOLT_MV2VAL(volt) << PRCM_AON_LDO_WORK_VOLT_SHIFT);
	}

	return 0;
}

uint32_t HAL_PRCM_GetAONLDOSlpVoltage(void)
{
	return PRCM_AON_LDO_SLEEP_VOLT_VAL2MV_DELTA( \
		HAL_GET_BIT_VAL(PRCM->AON_LDO_CTRL, \
			PRCM_AON_LDO_SLEEP_VOLT_SHIFT, \
			PRCM_AON_LDO_SLEEP_VOLT_VMASK), HAL_PRCM_GetAONLDOCalibrationValue());
}

int HAL_PRCM_SetAONLDOSlpVoltage(uint32_t volt)
{
	if (volt > PRCM_AON_LDO_SLEEP_VOLT_MAX_MV || volt < PRCM_AON_LDO_SLEEP_VOLT_MIN_MV)
		return -4;

	if (HAL_PRCM_GetAONLDOCalibrationValue()) {
		HAL_MODIFY_REG(PRCM->AON_LDO_CTRL, \
				PRCM_AON_LDO_SLEEP_VOLT_MASK,
				(HAL_PRCM_GetAONLDOCalibrationValue() + PRCM_AON_LDO_SLEEP_VOLT_MV2VAL_DELTA(volt)) << PRCM_AON_LDO_SLEEP_VOLT_SHIFT);
	} else {
	        HAL_MODIFY_REG(PRCM->AON_LDO_CTRL, \
	                        PRCM_AON_LDO_SLEEP_VOLT_MASK,
	                        PRCM_AON_LDO_SLEEP_VOLT_MV2VAL(volt) << PRCM_AON_LDO_SLEEP_VOLT_SHIFT);
	}

	return 0;
}

uint32_t HAL_PRCM_GetAONLDOLQMode(void)
{
	/*
	 * return 0: active mode, 1: lq mode
	 */
	return !!HAL_GET_BIT(PRCM->AON_LDO_CTRL, PRCM_AON_LDO_LQB_MODE_BIT);
}

int HAL_PRCM_IsAONLDOEnabled(void)
{
	return !!HAL_GET_BIT(PRCM->AON_LDO_CTRL, PRCM_AON_LDO_EN_BIT);
}


/*
 * bit field definition of PRCM->APP_LDO_CTRL
 */
uint32_t HAL_PRCM_GetAPPLDOWorkVoltage(void)
{
	return PRCM_APP_LDO_WORK_VOLT_VAL2MV_DELTA( \
		HAL_GET_BIT_VAL(PRCM->APP_LDO_CTRL, \
			PRCM_APP_LDO_WORK_VOLT_SHIFT, \
			PRCM_APP_LDO_WORK_VOLT_VMASK), HAL_PRCM_GetAPPLDOCalibrationValue());
}

int HAL_PRCM_SetAPPLDOWorkVoltage(uint32_t volt)
{
	if (volt > PRCM_APP_LDO_WORK_VOLT_MAX_MV || volt < PRCM_APP_LDO_WORK_VOLT_MIN_MV)
		return -4;

	if (HAL_PRCM_GetAPPLDOCalibrationValue()) {
		HAL_MODIFY_REG(PRCM->APP_LDO_CTRL, \
				PRCM_APP_LDO_WORK_VOLT_MASK,
				(HAL_PRCM_GetAPPLDOCalibrationValue() + PRCM_APP_LDO_WORK_VOLT_MV2VAL_DELTA(volt)) << PRCM_APP_LDO_WORK_VOLT_SHIFT);
	} else {
	        HAL_MODIFY_REG(PRCM->APP_LDO_CTRL, \
	                        PRCM_APP_LDO_WORK_VOLT_MASK,
	                        PRCM_APP_LDO_WORK_VOLT_MV2VAL(volt) << PRCM_APP_LDO_WORK_VOLT_SHIFT);
	}

	return 0;
}

uint32_t HAL_PRCM_GetAPPLDOSlpVoltage(void)
{
	return PRCM_APP_LDO_SLEEP_VOLT_VAL2MV_DELTA( \
		HAL_GET_BIT_VAL(PRCM->APP_LDO_CTRL, \
			PRCM_APP_LDO_SLEEP_VOLT_SHIFT, \
			PRCM_APP_LDO_SLEEP_VOLT_VMASK), HAL_PRCM_GetAPPLDOCalibrationValue());
}

int HAL_PRCM_SetAPPLDOSlpVoltage(uint32_t volt)
{
	if (volt > PRCM_APP_LDO_SLEEP_VOLT_MAX_MV || volt < PRCM_APP_LDO_SLEEP_VOLT_MIN_MV)
		return -4;

	if (HAL_PRCM_GetAPPLDOCalibrationValue()) {
		HAL_MODIFY_REG(PRCM->APP_LDO_CTRL, \
				PRCM_APP_LDO_SLEEP_VOLT_MASK,
				(HAL_PRCM_GetAPPLDOCalibrationValue() + PRCM_APP_LDO_SLEEP_VOLT_MV2VAL_DELTA(volt)) << PRCM_APP_LDO_SLEEP_VOLT_SHIFT);
	} else {
	        HAL_MODIFY_REG(PRCM->APP_LDO_CTRL, \
	                        PRCM_APP_LDO_SLEEP_VOLT_MASK,
	                        PRCM_APP_LDO_SLEEP_VOLT_MV2VAL(volt) << PRCM_APP_LDO_SLEEP_VOLT_SHIFT);
	}

	return 0;
}

uint32_t HAL_PRCM_GetAPPLDOLQMode(void)
{
	/*
	 * return 0: active mode, 1: lq mode
	 */
	return !!HAL_GET_BIT(PRCM->APP_LDO_CTRL, PRCM_APP_LDO_LQB_MODE_BIT);
}

int HAL_PRCM_IsAPPLDOEnabled(void)
{
	return !!HAL_GET_BIT(PRCM->APP_LDO_CTRL, PRCM_APP_LDO_EN_BIT);
}


/*
 * bit field definition of PRCM->DSP_LDO_CTRL
 */
uint32_t HAL_PRCM_GetDSPLDOWorkVoltage(void)
{
	return PRCM_DSP_LDO_WORK_VOLT_VAL2MV_DELTA( \
		HAL_GET_BIT_VAL(PRCM->DSP_LDO_CTRL, \
			PRCM_DSP_LDO_WORK_VOLT_SHIFT, \
			PRCM_DSP_LDO_WORK_VOLT_VMASK), HAL_PRCM_GetDSPLDOCalibrationValue());
}

int HAL_PRCM_SetDSPLDOWorkVoltage(uint32_t volt)
{
	if (volt > PRCM_DSP_LDO_WORK_VOLT_MAX_MV || volt < PRCM_DSP_LDO_WORK_VOLT_MIN_MV)
		return -4;

	if (HAL_PRCM_GetDSPLDOCalibrationValue()) {
		HAL_MODIFY_REG(PRCM->DSP_LDO_CTRL, \
				PRCM_DSP_LDO_WORK_VOLT_MASK,
				(HAL_PRCM_GetDSPLDOCalibrationValue() + PRCM_DSP_LDO_WORK_VOLT_MV2VAL_DELTA(volt)) << PRCM_DSP_LDO_WORK_VOLT_SHIFT);
	} else {
	        HAL_MODIFY_REG(PRCM->DSP_LDO_CTRL, \
	                        PRCM_DSP_LDO_WORK_VOLT_MASK,
	                        PRCM_DSP_LDO_WORK_VOLT_MV2VAL(volt) << PRCM_DSP_LDO_WORK_VOLT_SHIFT);
	}


	return 0;
}

uint32_t HAL_PRCM_GetDSPLDOSlpVoltage(void)
{
	return PRCM_DSP_LDO_SLEEP_VOLT_VAL2MV_DELTA( \
		HAL_GET_BIT_VAL(PRCM->DSP_LDO_CTRL, \
			PRCM_DSP_LDO_SLEEP_VOLT_SHIFT, \
			PRCM_DSP_LDO_SLEEP_VOLT_VMASK), HAL_PRCM_GetDSPLDOCalibrationValue());
}

int HAL_PRCM_SetDSPLDOSlpVoltage(uint32_t volt)
{
	if (volt > PRCM_DSP_LDO_SLEEP_VOLT_MAX_MV || volt < PRCM_DSP_LDO_SLEEP_VOLT_MIN_MV)
		return -4;

	if (HAL_PRCM_GetDSPLDOCalibrationValue()) {
		HAL_MODIFY_REG(PRCM->DSP_LDO_CTRL, \
				PRCM_DSP_LDO_SLEEP_VOLT_MASK,
				(HAL_PRCM_GetDSPLDOCalibrationValue() + PRCM_DSP_LDO_SLEEP_VOLT_MV2VAL_DELTA(volt)) << PRCM_DSP_LDO_SLEEP_VOLT_SHIFT);
	} else {
	       HAL_MODIFY_REG(PRCM->DSP_LDO_CTRL, \
	                        PRCM_DSP_LDO_SLEEP_VOLT_MASK,
	                        PRCM_DSP_LDO_SLEEP_VOLT_MV2VAL(volt) << PRCM_DSP_LDO_SLEEP_VOLT_SHIFT);
	}

	return 0;
}

uint32_t HAL_PRCM_GetDSPLDOLQMode(void)
{

	/*
	 * return 0: active mode, 1: lq mode
	 */

	return HAL_GET_BIT(PRCM->DSP_LDO_CTRL, PRCM_DSP_LDO_LQB_MODE_BIT);
}

int HAL_PRCM_IsDSPLDOEnabled(void)
{
	return !!HAL_GET_BIT(PRCM->DSP_LDO_CTRL, PRCM_DSP_LDO_EN_BIT);
}



/*
 * bit field definition of PRCM->VBAT_MON_CTRL
 */

uint32_t HAL_PRCM_GetVBATMonLowVoltage(void)
{
	return PRCM_VBAT_MON_VSEL_VAL2MV( \
		HAL_GET_BIT_VAL(PRCM->VBAT_MON_CTRL, \
			PRCM_VBAT_MON_VSEL_SHIFT, \
			PRCM_VBAT_MON_VSEL_VMASK));
}

int HAL_PRCM_SetVBATMonLowVoltage(uint32_t volt)
{
	if (volt > PRCM_VBAT_MON_VSEL_MAX_MV || volt < PRCM_VBAT_MON_VSEL_MIN_MV)
		return -4;

	HAL_MODIFY_REG(PRCM->VBAT_MON_CTRL, \
			PRCM_VBAT_MON_VSEL_MASK,
			PRCM_VBAT_MON_VSEL_MV2VAL(volt) << PRCM_VBAT_MON_VSEL_SHIFT);

	return 0;
}

void HAL_PRCM_SetVBATLowVoltMode(PRCM_VbatLowVolMode mode)
{
	if (mode == PRCM_VBAT_LOW_VOL_MODE_RESET) {
		HAL_SET_BIT(PRCM->VBAT_MON_CTRL, PRCM_VBAT_LOW_VOL_MODE_SEL_BIT);
	} else {
		HAL_CLR_BIT(PRCM->VBAT_MON_CTRL, PRCM_VBAT_LOW_VOL_MODE_SEL_BIT);
	}
}

void HAL_PRCM_SetVBATMonSwModeEnable(uint8_t enable)
{
	if (enable) {
		HAL_SET_BIT(PRCM->VBAT_MON_CTRL, PRCM_VBAT_MON_SW_MODE_BIT);
	} else {
		HAL_CLR_BIT(PRCM->VBAT_MON_CTRL, PRCM_VBAT_MON_SW_MODE_BIT);
	}
}

void HAL_PRCM_SetVBATMonitorEnable(uint8_t enable)
{
	if (enable) {
		HAL_SET_BIT(PRCM->VBAT_MON_CTRL, PRCM_VBAT_MON_EN_BIT);
	} else {
		HAL_CLR_BIT(PRCM->VBAT_MON_CTRL, PRCM_VBAT_MON_EN_BIT);
	}
}


/*
 * bit field definition of PRCM->USB_BIAS_CTRL
 */

void HAL_PRCM_SetUSBBiasTrim(uint32_t trim)
{
	HAL_MODIFY_REG(PRCM->USB_BIAS_CTRL, PRCM_USB_BIAS_TRIM_MASK, \
			(trim & PRCM_USB_BIAS_TRIM_VMASK) << PRCM_USB_BIAS_TRIM_SHIFT);
}


void HAL_PRCM_SetUSBBiasEnable(uint8_t enable)
{
	if (enable) {
		HAL_SET_BIT(PRCM->USB_BIAS_CTRL, PRCM_USB_BIAS_EN_BIT);
	} else {
		HAL_CLR_BIT(PRCM->USB_BIAS_CTRL, PRCM_USB_BIAS_EN_BIT);
	}
}



/*
 * bit field definition of PRCM->SYS_LFCLK_CTRL
 */


void HAL_PRCM_SetPadClkOut(uint8_t enable)
{
	if (enable)
		HAL_SET_BIT(PRCM->SYS_LFCLK_CTRL, PRCM_PAD_CLK_OUT_EN_BIT);
	else
		HAL_CLR_BIT(PRCM->SYS_LFCLK_CTRL, PRCM_PAD_CLK_OUT_EN_BIT);
}

void HAL_PRCM_SetPadClkOutSource(PRCM_PadClkOutSource source)
{
	HAL_MODIFY_REG(PRCM->SYS_LFCLK_CTRL, PRCM_PAD_CLK_OUT_SOURCE_MASK, source);
}

void HAL_PRCM_SetPadClkOutFactorM(uint16_t value)
{
	HAL_MODIFY_REG(PRCM->SYS_LFCLK_CTRL, PRCM_PAD_CLK_OUT_FACTOR_M_MASK,
	               PRCM_PAD_CLK_OUT_FACTOR_M_VAL(value));
}

/*
 * Clock
 */
void HAL_PRCM_SetLFCLKBaseSource(PRCM_LFCLKBaseSrc src)
{
	/* always enable inter 32K for external 32K is not ready at startup */
	uint32_t clr_mask = PRCM_LFCLK_BASE_SRC_MASK | PRCM_LFCLK_EXT32K_EN_BIT;
	uint32_t set_mask = src | PRCM_LFCLK_INTER32K_EN_BIT;
	if (src == PRCM_LFCLK_BASE_SRC_EXT32K) {
		set_mask |= PRCM_LFCLK_EXT32K_EN_BIT;
	}
	HAL_MODIFY_REG(PRCM->SYS_LFCLK_CTRL, clr_mask, set_mask);
}

uint32_t HAL_PRCM_GetInter32KFreq(void)
{
	return (10 * HAL_GET_BIT_VAL(PRCM->SYS_RCOSC_FREQ_DET,
	                             PRCM_RCOSC_CALIB_FREQ_SHIFT,
	                             PRCM_RCOSC_CALIB_FREQ_VMASK));
}

uint32_t HAL_PRCM_EnableInter32KCalib(void)
{
	return HAL_SET_BIT(PRCM->SYS_RCOSC_FREQ_DET, PRCM_RCOSC_CALIB_EN_BIT);
}

uint32_t HAL_PRCM_DisableInter32KCalib(void)
{
	return HAL_CLR_BIT(PRCM->SYS_RCOSC_FREQ_DET, PRCM_RCOSC_CALIB_EN_BIT);
}

int HAL_PRCM_IsInter32KCalibEnabled(void)
{
	return HAL_GET_BIT(PRCM->SYS_RCOSC_FREQ_DET, PRCM_RCOSC_CALIB_EN_BIT);
}

/*
 *fix that inter32kfreq regs is 0 before 24M clock complete checking the inter32k clock.
 *32768 is recommended value.
 */
uint32_t HAL_PRCM_GetBaseLFClock(void)
{
	uint32_t val = HAL_GET_BIT(PRCM->SYS_LFCLK_CTRL, PRCM_LFCLK_BASE_SRC_MASK);

	if (val == PRCM_LFCLK_BASE_SRC_INTER32K &&
	    HAL_GET_BIT(PRCM->SYS_RCOSC_FREQ_DET, PRCM_RCOSC_CALIB_EN_BIT)) {
		if (!HAL_PRCM_GetInter32KFreq()) {
			return SYS_LFCLOCK;
		} else {
			return HAL_PRCM_GetInter32KFreq();
		}
	} else {
		return SYS_LFCLOCK;
	}
}

void HAL_PRCM_SetCLK32kDiv(uint32_t en, PRCM_BLE_CLK32K_DivSrcSel sel)
{
	uint32_t div;

	if (en) {
		if (sel == PRCM_BLE_CLK32K_DIV_SRC_SEL_32M) {
			div = PRCM_BLE_CLK32K_DIV_HALFCYCLE_32M;
		} else {
			div = HAL_GetHFClock() / (32 * 1000) / 2 - 1;
		}
		HAL_MODIFY_REG(PRCM->BLE_CLK32K_SWITCH0, PRCM_BLE_CLK32K_DIV_MASK,
			       PRCM_BLE_CLK32K_DIV_VALUE(div));
		HAL_MODIFY_REG(PRCM->BLE_CLK32K_SWITCH0, PRCM_BLE_CLK32K_DIV_SRC_SEL_MASK, sel);
		HAL_SET_BIT(PRCM->BLE_CLK32K_SWITCH0, PRCM_BLE_CLK32K_DIV_CLK_EN_BIT);
	} else {
		HAL_CLR_BIT(PRCM->BLE_CLK32K_SWITCH0, PRCM_BLE_CLK32K_DIV_CLK_EN_BIT);
	}
}

void HAL_PRCM_SetCLK32kAutoSw(uint32_t en)
{
	if (en) {
		HAL_SET_BIT(PRCM->BLE_CLK32K_SWITCH0, PRCM_BLE_CLK32K_AUTO_SW_EN_BIT);
	} else {
		HAL_CLR_BIT(PRCM->BLE_CLK32K_SWITCH0, PRCM_BLE_CLK32K_AUTO_SW_EN_BIT);
	}
}

void HAL_PRCM_SetRcoCalib(uint32_t en, PRCM_RCOSC_WkModSel mode, PRCM_RCOSC_NormalWkTimesSel sel,
                          uint32_t phase, uint32_t wk_time_en, uint32_t wk_time)
{
	if (en) {
		uint32_t mask;

		/* method */
		mask = PRCM_RCOSC_WK_MODE_SEL_MASK | PRCM_RCOSC_NORMAL_WK_TIMES_SEL_MASK |
		       PRCM_RCOSC_SCALE_PHASE1_NUM_MASK | PRCM_RCOSC_SCALE_PHASE2_NUM_SHIFT |
		       PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_MASK | PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_MASK;
		HAL_MODIFY_REG(PRCM->BLE_RCOSC_CALIB_CTRL1, mask, mode | sel | phase);

		HAL_SET_BIT(PRCM->BLE_RCOSC_CALIB_CTRL0, PRCM_BLE_RCOSC_CALIB_EN_BIT);
	} else {
		HAL_CLR_BIT(PRCM->BLE_RCOSC_CALIB_CTRL0, PRCM_BLE_RCOSC_CALIB_EN_BIT);
	}

	if (wk_time_en) {
		/* eg. phase3 = wup_time * PRCM_RCOSC_SCALE_PHASE3_WUP_TIMES_10 = 2.5ms,
		 *     8 / 32000 = 0.25ms
		 */
		HAL_MODIFY_REG(PRCM->BLE_RCOSC_CALIB_CTRL0, PRCM_RCOSC_WK_TIME_MASK, wk_time << PRCM_RCOSC_WK_TIME_SHIFT);
		HAL_SET_BIT(PRCM->BLE_RCOSC_CALIB_CTRL0, PRCM_RCOSC_WK_TIME_EN_BIT);
	} else {
		HAL_CLR_BIT(PRCM->BLE_RCOSC_CALIB_CTRL0, PRCM_RCOSC_WK_TIME_EN_BIT);
	}
}


uint32_t HAL_PRCM_GetWLANRstStatus(void)
{
	return !!HAL_GET_BIT(PRCM->BT_WLAN_CONN_MODE_CTRL, PRCM_WLAN_RESET_BIT);
}

uint32_t HAL_PRCM_GetBLERstStatus(void)
{
	return !!HAL_GET_BIT(PRCM->BT_WLAN_CONN_MODE_CTRL, PRCM_BLE_RESET_BIT);
}

void HAL_PRCM_SetWlanSramShare(PRCM_WLAN_ShareSramType type)
{
	HAL_MODIFY_REG(PRCM->BT_WLAN_CONN_MODE_CTRL, PRCM_WLAN_SHARE_SRAM_MASK, type);
}

uint32_t HAL_PRCM_GetConnectModeFlag(void)
{
	return !!HAL_GET_BIT(PRCM->BT_WLAN_CONN_MODE_CTRL, PRCM_CONN_BOOT_FLAG_BIT);
}

static uint8_t __ext_flash_flg = 0;
int HAL_PRCM_IsFlashSip(void)
{
	if (__ext_flash_flg) {
		return 0;
	}
	return HAL_GET_BIT(PRCM->CFG_IO_STATUS, PRCM_FLASH_SIP_EN_BIT);
}

void HAL_PRCM_SetFlashExt(uint8_t ext)
{
	__ext_flash_flg = ext;
}

void HAL_PRCM_EnableWlanCPUClk(uint8_t enable)
{
	if (enable)
		HAL_CLR_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_DISABLE_CPU_CLK_BIT); /* 0 is enable */
	else
		HAL_SET_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_DISABLE_CPU_CLK_BIT);
}

void HAL_PRCM_ReleaseWlanCPUReset(void)
{
	HAL_CLR_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_RESET_CPU_BIT);
}

void HAL_PRCM_ForceWlanCPUReset(void)
{
	HAL_SET_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_RESET_CPU_BIT);
}

void HAL_PRCM_WakeUpWlan(uint8_t wakeup)
{
	if (wakeup)
		HAL_SET_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_WUP_BIT);
	else
		HAL_CLR_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_WUP_BIT);
}

void HAL_PRCM_EnableWlanCPUClkOvrHIF(void)
{
	HAL_SET_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_DISABLE_CPU_CLK_OVR_HIF_BIT);
}

void HAL_PRCM_DisableWlanCPUClkOvrHIF(void)
{
	HAL_CLR_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_DISABLE_CPU_CLK_OVR_HIF_BIT);
}

void HAL_PRCM_ReleaseWlanCPUOvrHIF(void)
{
	HAL_SET_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_RESET_CPU_OVR_HIF_BIT);
}

void HAL_PRCM_ResetWlanCPUOvrHIF(void)
{
	HAL_CLR_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_RESET_CPU_OVR_HIF_BIT);
}

void HAL_PRCM_EnableWlanWUPOvrHIF(void)
{
	HAL_SET_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_WUP_OVR_HIF_BIT);
}

void HAL_PRCM_DisableWlanWUPOvrHIF(void)
{
	HAL_CLR_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_WUP_OVR_HIF_BIT);
}

void HAL_PRCM_EnableWlanIRQOvrHIF(void)
{
	HAL_SET_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_IRQ_OVR_HIF_BIT);
}

void HAL_PRCM_DisableWlanIRQOvrHIF(void)
{
	HAL_CLR_BIT(PRCM->WLAN_HIF_OV_CTRL, PRCM_WLAN_IRQ_OVR_HIF_BIT);
}


uint32_t HAL_PRCM_GetRcoscCalSwitchLoseClk()
{
	uint32_t loss_clk;
	uint32_t src_clock;

	if(HAL_GET_BIT(PRCM->BLE_CLK32K_SWITCH0, PRCM_BLE_CLK32K_DIV_SRC_SEL_MASK) == PRCM_BLE_CLK32K_DIV_SRC_SEL_HFCLK) {
		src_clock = HAL_GetHFClock();
	} else {
		src_clock = 32000000;//32MHz
	}
	loss_clk = HAL_GET_BIT_VAL(PRCM->BLE_CLK32K_SWITCH1, PRCM_BLE_CLK32K_SW_OFFSET_DOWN_SHIFT, PRCM_BLE_CLK32K_SW_OFFSET_DOWN_VMASK);
	loss_clk += HAL_GET_BIT_VAL(PRCM->BLE_CLK32K_SWITCH1, PRCM_BLE_CLK32K_SW_OFFSET_ON_SHIFT, PRCM_BLE_CLK32K_SW_OFFSET_ON_VMASK);
	loss_clk /= (src_clock / 1000000);

	return loss_clk;
}

void HAL_PRCM_SetCLK32KSwitchRCCal(uint8_t enable)
{
	if(enable) {
		HAL_SET_BIT(PRCM->BLE_CLK32K_SWITCH0, PRCM_CLK32K_SW_FORCE_EN_BIT | PRCM_CLK32K_SW_FORCE_DOWN_BIT | PRCM_CLK32K_SW_FORCE_READY_BIT);
		hal_udelay(100);
		HAL_MODIFY_REG(PRCM->BLE_CLK32K_SWITCH0, PRCM_CLK32K_SW_FORCE_EN_BIT | PRCM_CLK32K_SW_FORCE_DOWN_BIT | PRCM_CLK32K_SW_FORCE_READY_BIT,
			PRCM_CLK32K_SW_FORCE_EN_BIT & (~PRCM_CLK32K_SW_FORCE_READY_BIT) & (~PRCM_CLK32K_SW_FORCE_DOWN_BIT));
	} else {
		HAL_MODIFY_REG(PRCM->BLE_CLK32K_SWITCH0, PRCM_CLK32K_SW_FORCE_EN_BIT | PRCM_CLK32K_SW_FORCE_DOWN_BIT | PRCM_CLK32K_SW_FORCE_READY_BIT,
			PRCM_CLK32K_SW_FORCE_EN_BIT | PRCM_CLK32K_SW_FORCE_READY_BIT);
		hal_udelay(100);
	}
}

void HAL_PRCM_SetFlashCryptoNonce(uint8_t *nonce)
{
	PRCM->FLASH_ENCRYPT_AES_NONCE1 = HAL_REG_16BIT(nonce);
	PRCM->FLASH_ENCRYPT_AES_NONCE0 = HAL_REG_32BIT(&nonce[2]);
}

void HAL_PRCM_SetCPUBootFlag(PRCM_CPUTypeIndex cpu, PRCM_CPUBootFlag flag)
{
	switch (cpu) {
	case PRCM_CPU_TYPE_INDEX_AR800A:
		PRCM->AR800A_BOOT_FLAG = PRCM_CPU_BOOT_FLAG_WR_LOCK | flag;
		break;
	case PRCM_CPU_TYPE_INDEX_DSP:
		PRCM->DSP_BOOT_FLAG = PRCM_CPU_BOOT_FLAG_WR_LOCK | flag;
		break;
	case PRCM_CPU_TYPE_INDEX_RSICV:
		PRCM->RV_BOOT_FLAG = PRCM_CPU_BOOT_FLAG_WR_LOCK | flag;
		break;
	default:
		break;
	}
}

uint32_t HAL_PRCM_GetCPUBootFlag(PRCM_CPUTypeIndex cpu)
{
	volatile uint32_t temp = 0xFFFFFFFF;

	switch (cpu) {
	case PRCM_CPU_TYPE_INDEX_AR800A:
		temp = HAL_GET_BIT(PRCM->AR800A_BOOT_FLAG, PRCM_CPU_BOOT_FLAG_MASK);
		break;
	case PRCM_CPU_TYPE_INDEX_DSP:
		temp = HAL_GET_BIT(PRCM->DSP_BOOT_FLAG, PRCM_CPU_BOOT_FLAG_MASK);
		break;
	case PRCM_CPU_TYPE_INDEX_RSICV:
		temp = HAL_GET_BIT(PRCM->RV_BOOT_FLAG, PRCM_CPU_BOOT_FLAG_MASK);
		break;
	default:
		break;
	}

	return temp;
}

void HAL_PRCM_SetCPUBootAddr(PRCM_CPUTypeIndex cpu, uint32_t addr)
{
	switch (cpu) {
	case PRCM_CPU_TYPE_INDEX_AR800A:
		PRCM->AR800A_BOOT_ADDR = addr;
		break;
	case PRCM_CPU_TYPE_INDEX_DSP:
		PRCM->DSP_BOOT_ADDR = addr;
		break;
	case PRCM_CPU_TYPE_INDEX_RSICV:
		PRCM->RV_BOOT_ADDR = addr;
		break;
	default:
		break;
	}
}

uint32_t HAL_PRCM_GetCPUBootAddr(PRCM_CPUTypeIndex cpu)
{
	volatile uint32_t temp = 0xFFFFFFFF;

	switch (cpu) {
	case PRCM_CPU_TYPE_INDEX_AR800A:
		temp =  PRCM->AR800A_BOOT_ADDR;
		break;
	case PRCM_CPU_TYPE_INDEX_DSP:
		temp =  PRCM->DSP_BOOT_ADDR;
		break;
	case PRCM_CPU_TYPE_INDEX_RSICV:
		temp =  PRCM->RV_BOOT_ADDR;
		break;
	default:
		break;
	}

	return temp;
}

void HAL_PRCM_SetCPUBootArg(PRCM_CPUTypeIndex cpu, uint32_t arg)
{
	switch (cpu) {
	case PRCM_CPU_TYPE_INDEX_AR800A:
		PRCM->AR800A_BOOT_ARG = arg;
		break;
	case PRCM_CPU_TYPE_INDEX_DSP:
		PRCM->DSP_BOOT_ARG = arg;
		break;
	case PRCM_CPU_TYPE_INDEX_RSICV:
		PRCM->RV_BOOT_ARG = arg;
		break;
	default:
		break;
	}
}

uint32_t HAL_PRCM_GetCPUBootArg(PRCM_CPUTypeIndex cpu)
{
	volatile uint32_t temp = 0xFFFFFFFF;

	switch (cpu) {
	case PRCM_CPU_TYPE_INDEX_AR800A:
		temp =  PRCM->AR800A_BOOT_ARG;
		break;
	case PRCM_CPU_TYPE_INDEX_DSP:
		temp =  PRCM->DSP_BOOT_ARG;
		break;
	case PRCM_CPU_TYPE_INDEX_RSICV:
		temp =  PRCM->RV_BOOT_ARG;
		break;
	default:
		break;
	}

	return temp;
}


void HAL_PRCM_SetSystemPrivateData(uint32_t id, uint32_t data)
{
	if (id < PRCM_SYSTEM_PRIV_DATA_ID_NUM) {
		PRCM->SYSTEM_PRIV_REG0T7[id] = data;
	}
}

uint32_t HAL_PRCM_GetSystemPrivateData(uint32_t id)
{
	if (id < PRCM_SYSTEM_PRIV_DATA_ID_NUM) {
		return PRCM->SYSTEM_PRIV_REG0T7[id];
	} else {
		return 0;
	}
}




