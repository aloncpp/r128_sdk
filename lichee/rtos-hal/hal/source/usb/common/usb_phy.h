#ifndef __USB_PHY_H__
#define __USB_PHY_H__

/*-------------------------------------------------------------*/

#if defined(CONFIG_ARCH_SUN8IW20) || defined(CONFIG_SOC_SUN20IW1) \
 || defined(CONFIG_ARCH_SUN8IW21) || defined(CONFIG_SOC_SUN20IW3) \
 || defined(CONFIG_ARCH_SUN20IW2)
#define USB_PHY_USE_VCBUS		1 //self-search phy use vcbus
#else
#define USB_PHY_USE_VCBUS		0 //purchased phy usb register
#endif

/*-------------------------------------------------------------*/
/* phy register offset*/

#define USB_PHY_OFFSET_CTRL		0x10
#define USB_PHY_OFFSET_TUNE		0x18
#define USB_PHY_OFFSET_STATUS		0x24

/*-------------------------------------------------------------*/
/* self-search phy */

/*phy read/write vc-bus contrl register*/
#define USB_PHY_CTRL_VC_CLK		(0x1 << 0)	//bit0
#define USB_PHY_CTRL_VC_EN		(0x1 << 1)	//bit1
#define USB_PHY_CTRL_VC_DI		(0x1 << 7)	//bit7
#define USB_PHY_CTRL_VC_ADDR		(0xff << 8)	//bit8-15
#define USB_PHY_STATUS_VC_DO		(0x1 << 0)	//bit0

/*phy config mode register address*/
#define USB_PHY_MODE			0x60
#define USB_PHY_VERF_MODE		0
#define USB_PHY_IERF_MODE		1

/*rise/fall adjust register address*/
#define USB_PHY_COMM_VREF_RISE		0x36
#define USB_PHY_COMM_IREF_RISE		0x30
#define USB_PHY_TRAN_IREF_RISE		0x61

/*pre-emphasis adjust register address*/
#define USB_PHY_TRAN_PREEMPAMP		0x64

/*resistance adjust register address*/
#define USB_PHY_TRAN_HARD_RES		0x49
#define USB_PHY_TRAN_SOFT_RES		0x44 //same as USB_PHY_EFUSE_RES

/*ft adjust register address*/
#define USB_PHY_EFUSE_ADDR		0x03006218	//[27:16]
#define USB_PHY_EFUSE_ADJUST		(0x1 << 16)	//bit16
#define USB_PHY_EFUSE_MODE		(0x1 << 17)	//bit17
#define USB_PHY_EFUSE_RES		(0xf << 18)	//bit18-21
#define USB_PHY_EFUSE_VERF_COMMON	(0x7 << 22)	//bit22-24
#define USB_PHY_EFUSE_IREF_USB0TX	(0x7 << 22)	//bit22-24
#define USB_PHY_EFUSE_IREF_USB1TX	(0x7 << 25)	//bit25-27

/*phy_range mapping*/
#define USB_PHY_RANGE_RES		(0xf << 0)	//bit0-3
#define USB_PHY_RANGE_PREEMPAMP		(0x3 << 4)	//bit4-5
#define USB_PHY_RANGE_TRAN_RISE		(0x7 << 6)	//bit6-8
#define USB_PHY_RANGE_COMM_RISE		(0x7 << 9)	//bit9-11
#define USB_PHY_RANGE_MODE		(0x1 << 12)	//bit12

/*-------------------------------------------------------------*/
/* purchased phy */

/*register mapping*/
#define USB_PHY_TUNE_TXPREEMPAMPTUNE	(0x3 << 0)	//bit0-1 预加重
#define USB_PHY_TUNE_TXRESTUNE		(0x3 << 2)	//bit2-3 阻抗
#define USB_PHY_TUNE_TXRISETUNE		(0x3 << 4)	//bit4-5 幅度（波形）
#define USB_PHY_TUNE_TXVREFTUNE		(0xf << 8)	//bit8-11 驱动能力（电压）

/*phy_range mapping*/
#define USB_PHY_RANGE_TXVREFTUNE	(0xf << 0)	//bit0-3
#define USB_PHY_RANGE_TXRISETUNE	(0x3 << 4)	//bit4-5
#define USB_PHY_RANGE_TXPREEMPAMPTUNE	(0x3 << 6)	//bit6-7
#define USB_PHY_RANGE_TXRESTUNE		(0x3 << 8)	//bit8-9

/*-------------------------------------------------------------*/

void usb_phy_init(unsigned long phy_vbase, unsigned int usbc_no);
void usb_phy_range_set(unsigned long phy_vbase, int data);
void usb_phy_range_show(unsigned long phy_vbase);

#endif //__USB_PHY_H__
