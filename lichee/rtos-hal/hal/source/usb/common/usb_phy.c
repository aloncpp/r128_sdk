/*
********************************************************************************************************************
*                             Copyright(C), 2006-2021, AllWinners Co., Ltd.
*                                           All Rights Reserved
*
* File Name : usb_phy.c
*
* Author :
*
* Version : 1.0
*
* Date : 2022.9.15
*
* Description :
* phy的控制目前一共两种：
* 1.直接读写818/418寄存器，主要针对外购phy
* 2.通过VC总线读写，主要针对自研phy
* 但是也会有例外，没有明确区分手段，每个项目不同，具体参考《正在维护平台的使用情况》，或咨询设计人员
* tips：查看spec的phy ctrl和status寄存器看是否有操作vc总线的位也可大致区分
*
* History :
*
********************************************************************************************************************
*/

#include "usb_phy.h"
#include "usb_os_platform.h"

static int bit_offset(uint32_t mask)
{
	int offset = 0;

	for (offset = 0; offset < 32; offset++)
		if ((mask == 0) || (mask & (0x01 << offset)))
			break;
	return offset;
}

static int bit_len(uint32_t mask)
{
	int len = 0;
	int offset = 0;

	for (offset = 0; offset < 32; offset++)
		if (mask & (0x01 << offset))
			len++;
	return len;
}

static int bit_val(int value, uint32_t mask)
{
	return (value & mask) >> bit_offset(mask);
}

static int bit_map(int value, uint32_t src_mask, uint32_t dst_mask)
{
	return bit_val(value, src_mask) << bit_offset(dst_mask);
}

static int usb_phy_tp_read(unsigned long phy_vbase, int addr, int len);
static int bit_read_map(uint64_t phy_addr, uint32_t reg_addr, uint32_t map_mask)
{
	int len = bit_len(map_mask);
	int val = usb_phy_tp_read(phy_addr, reg_addr, len);

	return val << bit_offset(map_mask);
}

static void bit_cal(int value, uint32_t mask, int *val, int *len)
{
	*val = bit_val(value, mask);
	*len = bit_len(mask);
}

/*-------------------------------------------------------------*/
/* bit write/read for vc_bus */

/**
 * @brief for VC bus write
 *
 * @param phy_vbase phy base register address
 * @param addr the address need to write
 * @param data the date need to write
 * @param len  write data bit length
 * @return int
 */
static int usb_phy_tp_write(unsigned long phy_vbase, int addr, int data, int len)
{
	int j = 0;
	int temp = 0;
	int dtmp = data;

	/*VC_EN enable*/
	temp = hal_readb(phy_vbase + USB_PHY_OFFSET_CTRL);
	temp |= USB_PHY_CTRL_VC_EN;
	hal_writeb(temp, phy_vbase + USB_PHY_OFFSET_CTRL);

	for (j = 0; j < len; j++) {
		/*ensure VC_CLK low*/
		temp = hal_readb(phy_vbase + USB_PHY_OFFSET_CTRL);
		temp &= ~USB_PHY_CTRL_VC_CLK;
		hal_writeb(temp, phy_vbase + USB_PHY_OFFSET_CTRL);

		/*set write address*/
		temp = hal_readl(phy_vbase + USB_PHY_OFFSET_CTRL);
		temp &= ~USB_PHY_CTRL_VC_ADDR;//clear
		temp |= ((addr + j) << bit_offset(USB_PHY_CTRL_VC_ADDR));  // write
		hal_writel(temp, phy_vbase + USB_PHY_OFFSET_CTRL);

		/*write data to VC_DI*/
		temp = hal_readb(phy_vbase + USB_PHY_OFFSET_CTRL);
		temp &= ~USB_PHY_CTRL_VC_DI;//clear
		temp |= ((dtmp & 0x01) << bit_offset(USB_PHY_CTRL_VC_DI));  // write
		hal_writeb(temp, phy_vbase + USB_PHY_OFFSET_CTRL);

		/*set VC_CLK high*/
		temp |= USB_PHY_CTRL_VC_CLK;
		hal_writeb(temp, phy_vbase + USB_PHY_OFFSET_CTRL);

		/*right move one bit*/
		dtmp >>= 1;
	}

	/*set VC_CLK low*/
	temp = hal_readb(phy_vbase + USB_PHY_OFFSET_CTRL);
	temp &= ~USB_PHY_CTRL_VC_CLK;
	hal_writeb(temp, phy_vbase + USB_PHY_OFFSET_CTRL);

	/*VC_EN disable*/
	temp = hal_readb(phy_vbase + USB_PHY_OFFSET_CTRL);
	temp &= ~USB_PHY_CTRL_VC_EN;
	hal_writeb(temp, phy_vbase + USB_PHY_OFFSET_CTRL);

	return 0;
}

/**
 * @brief for VC bus read
 *
 * @param phy_vbase phy base register address
 * @param addr the address need to read
 * @param len read data bit length
 * @return int
 */
static int usb_phy_tp_read(unsigned long phy_vbase, int addr, int len)
{
	int temp = 0;
	int i = 0;
	int j = 0;
	int ret = 0;

	/*VC_EN enable*/
	temp = hal_readb(phy_vbase + USB_PHY_OFFSET_CTRL);
	temp |= USB_PHY_CTRL_VC_EN;
	hal_writeb(temp, phy_vbase + USB_PHY_OFFSET_CTRL);

	for (j = len; j > 0; j--) {
		/*set write address*/
		temp = hal_readl(phy_vbase + USB_PHY_OFFSET_CTRL);
		temp &= ~USB_PHY_CTRL_VC_ADDR;//clear
		temp |= ((addr + j - 1) << bit_offset(USB_PHY_CTRL_VC_ADDR));  // write
		hal_writel(temp, phy_vbase + USB_PHY_OFFSET_CTRL);

		/*delsy 1us*/
		hal_udelay(1);

		/*read data from VC_DO*/
		temp = hal_readb(phy_vbase + USB_PHY_OFFSET_STATUS);
		ret <<= 1;
		ret |= temp & USB_PHY_STATUS_VC_DO;

	}

	/*VC_EN disable*/
	temp = hal_readb(phy_vbase + USB_PHY_OFFSET_CTRL);
	temp &= ~USB_PHY_CTRL_VC_EN;
	hal_writeb(temp, phy_vbase + USB_PHY_OFFSET_CTRL);

	return ret;
}

static void usb_phy_tp_resistance_soft_adjust(unsigned long phy_vbase, int data)
{
	usb_phy_tp_write(phy_vbase, 0x43, 0x0, 0x1);	// rcal_ensoft disable
	usb_phy_tp_write(phy_vbase, 0x41, 0x0, 0x1);	// rcal_en disable
	usb_phy_tp_write(phy_vbase, 0x40, 0x0, 0x1);	// rcal_on disable
	usb_phy_tp_write(phy_vbase, 0x44, data, 0x04);	// write to rcal_soft
	usb_phy_tp_write(phy_vbase, 0x43, 0x1, 0x1);	// rcal_ensoft enable
}

/*-------------------------------------------------------------*/
/* phy range write/read for register
 * 	1.phy_range's range is 0x0~0x3ff.
 * 	2.phy_range paraments:
 * 		bit[9:8]: TXRESTUNE
 * 		bit[7:6]: TXPREEMPAMPTUNE
 * 		bit[5:4]: TXRISETUNE
 * 		bit[3:0]: TXVREFTUNE
 * 	3.mapping to register is discontinuity
 * 		bit[11:8]: TXVREFTUNE
 * 		bit[5:4] : TXRISETUNE
 * 		bit[3:2] : TXRESTUNE
 * 		bit[1:0] : TXPREEMPAMPTUNE
 */

static int usb_phy_range_register_write(unsigned long phy_vbase, int data)
{
	int val = 0;

	val = hal_readl(phy_vbase + USB_PHY_OFFSET_TUNE);

	/*clear TXPREEMPAMPTUNE + TXRESTUNE + TXRISETUNE + TXVREFTUNE */
	val &= ~(USB_PHY_TUNE_TXPREEMPAMPTUNE | USB_PHY_TUNE_TXRESTUNE
		 | USB_PHY_TUNE_TXRISETUNE | USB_PHY_TUNE_TXVREFTUNE);

	/*set TXPREEMPAMPTUNE*/
	val |= bit_map(data, USB_PHY_RANGE_TXPREEMPAMPTUNE, USB_PHY_TUNE_TXPREEMPAMPTUNE);
	/*set TXRESTUNE*/
	val |= bit_map(data, USB_PHY_RANGE_TXRESTUNE, USB_PHY_TUNE_TXRESTUNE);
	/*set TXRISETUNE*/
	val |= bit_map(data, USB_PHY_RANGE_TXRISETUNE, USB_PHY_TUNE_TXRISETUNE);
	/*set TXVREFTUNE*/
	val |= bit_map(data, USB_PHY_RANGE_TXVREFTUNE, USB_PHY_TUNE_TXVREFTUNE);

	hal_writel(val, (phy_vbase + USB_PHY_OFFSET_TUNE));

	printf("phy_range config success!\n");
	return 0;
}

static int usb_phy_range_register_read(unsigned long phy_vbase)
{
	int val = 0;
	int data = 0;

	val = hal_readl(phy_vbase + USB_PHY_OFFSET_TUNE);

	/*read transmitter pre-emphasis*/
	data |= bit_map(val, USB_PHY_TUNE_TXPREEMPAMPTUNE, USB_PHY_RANGE_TXPREEMPAMPTUNE);
	/*read source impedance*/
	data |= bit_map(val, USB_PHY_TUNE_TXRESTUNE, USB_PHY_RANGE_TXRESTUNE);
	/*read rise/fall time range*/
	data |= bit_map(val, USB_PHY_TUNE_TXRISETUNE, USB_PHY_RANGE_TXRISETUNE);
	/*read DC voltage level*/
	data |= bit_map(val, USB_PHY_TUNE_TXVREFTUNE, USB_PHY_RANGE_TXVREFTUNE);

	printf("phy_range = 0x%x, details:\n"\
	       "bit [9:8] = 0x%x, source impedance\n"\
	       "bit [7:6] = 0x%x, transmitter pre-emphasis\n"\
	       "bit [5:4] = 0x%x, rise/fall time\n"\
	       "bit [3:0] = 0x%x, DC voltage level\n\n",
	       data, bit_val(data, USB_PHY_RANGE_TXRESTUNE), bit_val(data, USB_PHY_RANGE_TXPREEMPAMPTUNE),
	       bit_val(data, USB_PHY_RANGE_TXRISETUNE), bit_val(data, USB_PHY_RANGE_TXVREFTUNE));

	return 0;
}

#if defined(CONFIG_ARCH_SUN8IW20) || defined(CONFIG_SOC_SUN20IW1)
/*-------------------------------------------------------------*/
/* phy range write/read for vcbus
 * 	1.phy_range's range is 0x0~0x3ff.
 * 	2.phy_range paraments:
 * 		bit[3:0]  : RES
 * 		bit[5:4]  : PREEMPAMP
 * 		bit[8:6]  : TRAN_RISE
 * 		bit[11:9] : COMM_RISE
 * 		bit[12]   : MODE
 * 	3.mapping to register is discontinuity
 * 		bit[3:0]  : 0x44-0x47
 * 		bit[5:4]  : 0x64-0x65
 * 		bit[8:6]  : 0x61-0x63
 * 		bit[11:9] : 0x30-0x32/0x36-0x38
 * 		bit[12]   : 0x60
 */

static int usb_phy_range_vcbus_write(unsigned long phy_vbase, int data)
{
	uint32_t ret = 0;
	uint32_t len = 0;
	uint32_t value = 0;
	uint32_t phy_mode = 0;

	/* set phy range config mode*/
	bit_cal(data, USB_PHY_RANGE_MODE, &phy_mode, &len);
	usb_phy_tp_write(phy_vbase, USB_PHY_MODE, phy_mode, len);

	switch (phy_mode) {
	case USB_PHY_IERF_MODE:
		/* set common rise range */
		bit_cal(data, USB_PHY_RANGE_COMM_RISE, &value, &len);
		usb_phy_tp_write(phy_vbase, USB_PHY_COMM_IREF_RISE, value, len);

		/* set tranceive rise range */
		bit_cal(data, USB_PHY_RANGE_TRAN_RISE, &value, &len);
		usb_phy_tp_write(phy_vbase, USB_PHY_TRAN_IREF_RISE, value, len);

		break;
	case USB_PHY_VERF_MODE:
		/* set common rise range */
		bit_cal(data, USB_PHY_RANGE_COMM_RISE, &value, &len);
		usb_phy_tp_write(phy_vbase, USB_PHY_COMM_VREF_RISE, value, len);

		/* compare tranceive rise range */
		bit_cal(data, USB_PHY_RANGE_TRAN_RISE, &value, &len);
		if (value != usb_phy_tp_read(phy_vbase, USB_PHY_TRAN_IREF_RISE, len)){
			printf("cann't config trancevie rise range in VERF mode! origin:0x%x, new:0x%x\n",
				usb_phy_tp_read(phy_vbase, USB_PHY_TRAN_IREF_RISE, len),value);
			ret = -1;
		}
		break;
	default:
		break;
	}

	/* set pre-emphasis*/
	bit_cal(data, USB_PHY_RANGE_PREEMPAMP, &value, &len);
	usb_phy_tp_write(phy_vbase, USB_PHY_TRAN_PREEMPAMP, value, len);

	/* set soft resistance*/
	value = bit_val(data, USB_PHY_RANGE_RES);
	usb_phy_tp_resistance_soft_adjust(phy_vbase, value);

	printf("phy_range config in %s mode %s!\n", phy_mode ? "iref" : "vref",
	       ret == 0 ? "success" : "fail");
	return ret;
}

static int usb_phy_range_vcbus_read(unsigned long phy_vbase)
{
	uint32_t len = 0;
	uint32_t value = 0;
	uint32_t phy_mode = 0;

	/* read phy range config mode*/
	len = bit_len(USB_PHY_RANGE_MODE);
	phy_mode = usb_phy_tp_read(phy_vbase, USB_PHY_MODE, len);
	value |= phy_mode << bit_offset(USB_PHY_RANGE_MODE);

	switch (phy_mode) {
	case USB_PHY_IERF_MODE:
		/* read common rise range */
		value |= bit_read_map(phy_vbase, USB_PHY_COMM_IREF_RISE, USB_PHY_RANGE_COMM_RISE);
		break;
	case USB_PHY_VERF_MODE:
		/* read common rise range */
		value |= bit_read_map(phy_vbase, USB_PHY_COMM_VREF_RISE, USB_PHY_RANGE_COMM_RISE);
		break;
	default:
		break;
	}

	/* read tranceive rise range */
	value |= bit_read_map(phy_vbase, USB_PHY_TRAN_IREF_RISE, USB_PHY_RANGE_TRAN_RISE);
	/* read pre-emphasis*/
	value |= bit_read_map(phy_vbase, USB_PHY_TRAN_PREEMPAMP, USB_PHY_RANGE_PREEMPAMP);
	/* read resistance*/
	value |= bit_read_map(phy_vbase, USB_PHY_TRAN_SOFT_RES, USB_PHY_RANGE_RES);

	printf("phy_range = 0x%x, details:\n"\
	       "bit[12]   = 0x%x, mode iref-1 vref-0\n"\
	       "bit[11:9] = 0x%x, rise time(all usb)\n"\
	       "bit[8:6]  = 0x%x, rise time(current usb)\n"\
	       "bit[5:4]  = 0x%x, pre-emphasis\n"\
	       "bit[3:0]  = 0x%x, resistance\n\n",
	       value, phy_mode, bit_val(value, USB_PHY_RANGE_COMM_RISE), bit_val(value, USB_PHY_RANGE_TRAN_RISE),
	       bit_val(value, USB_PHY_RANGE_PREEMPAMP), bit_val(value, USB_PHY_RANGE_RES));

	return 0;
}

#elif defined(CONFIG_ARCH_SUN20IW2)
static int usb_phy_range_vcbus_write(unsigned long phy_vbase, int data)
{
	if(data > 0x3){
		printf("out of range [0x0-0x3]!\n");
		return -1;
	}

	usb_phy_tp_write(phy_vbase, 0x20, data, 2);
	printf("phy_range config success!\n");
	return 0;
}

static int usb_phy_range_vcbus_read(unsigned long phy_vbase)
{
	printf("phy_range = 0x%x, details:\n"\
	       "bit[1:0]  = 0x%x, RISE\n",
	       usb_phy_tp_read(phy_vbase, 0x20, 2),
	       usb_phy_tp_read(phy_vbase, 0x20, 2));
	return 0;
}

#else

static int usb_phy_range_vcbus_write(unsigned long phy_vbase, int data)
{
	printf("phy range write invalid!\n");
	return -1;
}

static int usb_phy_range_vcbus_read(unsigned long phy_vbase)
{
	printf("phy range read invalid\n");
	return -1;
}

#endif
/*-------------------------------------------------------------*/

void usb_phy_range_show(unsigned long phy_vbase)
{
#if USB_PHY_USE_VCBUS
	usb_phy_range_vcbus_read(phy_vbase);
#else
	usb_phy_range_register_read(phy_vbase);
#endif
}

void usb_phy_range_set(unsigned long phy_vbase, int data)
{
#if USB_PHY_USE_VCBUS
	usb_phy_range_vcbus_write(phy_vbase, data);
#else
	usb_phy_range_vcbus_write(phy_vbase, data);
#endif
}

/*-------------------------------------------------------------*/

#if defined(CONFIG_ARCH_SUN8IW20) || defined(CONFIG_SOC_SUN20IW1)
void usb_phy_init(unsigned long phy_vbase, unsigned int usbc_no)
{
	int len = 0, value = 0, efuse = 0, efuse_mode = 0;

	efuse = hal_readl(USB_PHY_EFUSE_ADDR);
	if (!(efuse & USB_PHY_EFUSE_ADJUST))
		return;

	efuse_mode = bit_val(efuse, USB_PHY_EFUSE_MODE);
	usb_phy_tp_write(phy_vbase, USB_PHY_MODE, efuse_mode, 0x01);

	/* set soft resistance */
	value = bit_val(efuse, USB_PHY_EFUSE_RES);
	usb_phy_tp_resistance_soft_adjust(phy_vbase, value);

	/* set common rise range */
	switch (efuse_mode) {
	case USB_PHY_IERF_MODE:
		/* efuse[24:22]map to usb0 (tranceive rise)
		 * efuse[27:25]map to usb1 (tranceive rise) */
		if (usbc_no)
			bit_cal(efuse, USB_PHY_EFUSE_IREF_USB1TX, &value, &len);
		else
			bit_cal(efuse, USB_PHY_EFUSE_IREF_USB0TX, &value, &len);
		usb_phy_tp_write(phy_vbase, USB_PHY_TRAN_IREF_RISE, value, len);
		break;
	case USB_PHY_VERF_MODE:
		/*efuse[24:22]map tranceive rise */
		bit_cal(efuse, USB_PHY_EFUSE_VERF_COMMON, &value, &len);
		usb_phy_tp_write(phy_vbase, USB_PHY_COMM_VREF_RISE, value, len);
		break;
	default:
		break;
	}

	printf("phy_vbase : 0x%lx, usbc_no : %x, efuse : 0x%x\n", phy_vbase, usbc_no, efuse);
}
#elif defined(CONFIG_ARCH_SUN20IW2)
void usb_phy_init(unsigned long phy_vbase, unsigned int usbc_no)
{
	/* 0xC - 0x1, enable calibration */
	usb_phy_tp_write(phy_vbase, 0xC, 0x1, 1);
	/* 0x20~0x21 - 0x3, adjust amplitude */
	usb_phy_tp_write(phy_vbase, 0x20, 0x3, 2);
	usb_phy_tp_write(phy_vbase, 0x3, 0x0, 2);

	/* 0x1a~0x1f, ro, read calibration value */
	printf("calibration finish, val:0x%x, usbc_no:%d\n",
		     usb_phy_tp_read(phy_vbase, 0x1a, 6), usbc_no);
}
#else
void usb_phy_init(unsigned long phy_vbase, unsigned int usbc_no)
{
	printf("using default phy\n");
}
#endif

