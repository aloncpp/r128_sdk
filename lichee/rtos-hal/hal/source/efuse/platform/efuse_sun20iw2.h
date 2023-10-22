#ifndef __EFUSE_SUN20IW2_H
#define __EFUSE_SUN20IW2_H


#ifdef __cplusplus
extern "C"
{
#endif

#define SUNXI_SID_BASE		0x4004E400
#define SID_PRCTL		(SUNXI_SID_BASE + 0x40)
#define SID_PRKEY		(SUNXI_SID_BASE + 0x50)
#define SID_RDKEY		(SUNXI_SID_BASE + 0x60)
#define SJTAG_AT0		(SUNXI_SID_BASE + 0x80)
#define SJTAG_AT1		(SUNXI_SID_BASE + 0x84)
#define SJTAG_S			(SUNXI_SID_BASE + 0x88)
#define EFUSE_SECURE_MODE	(SUNXI_SID_BASE + 0xA0)
#define SID_ROTPK_CTRL		(SUNXI_SID_BASE + 0x140)

#define EFUSE_SRAM		(SUNXI_SID_BASE + 0x200)

#define EFUSE_CHIPD             (0x00)
#define EFUSE_MAC_VER		(0x10)
#define EFUSE_FT_ZONE		(0x1C)
#define EFUSE_HW		(0x20)
#define EFUSE_SYSTEM		(0x44)
#define EFUSE_ROTPK		(0x60)
#define EFUSE_SSK		(0x90)
#define EFUSE_OEM1		(0xB0)
#define EFUSE_OEM2		(0xC0)
#define EFUSE_OEM3		(0xD0)

#define EFUSE_LCJS		(0x50)
#define SECURE_BIT_OFFSET	(11)


#define SCC_OEM3_BURNED_FLAG	(16)
#define SCC_OEM2_BURNED_FLAG	(15)
#define SCC_OEM1_BURNED_FLAG	(14)
#define SCC_ROTPK_BURNED_FLAG	(11)
#define SCC_SSK_BURNED_FLAG	(12)
#define SCC_MAC_BURNED_FLAG	(1)
#define SCC_FT_ZONE_BURNED_FLAG	(2)

/* write protect */
#define EFUSE_WRITE_PROTECT     (0x48)
/* read  protect */
#define EFUSE_READ_PROTECT      (0x4C)

#define EFUSE_BIT_NUM		(2048)

#define SID_OP_LOCK		(0xAC)

/*It can not be seen.*/
#define EFUSE_PRIVATE		(0)
/*After burned ,cpu can not access.*/
#define EFUSE_NACCESS		(1)

/* mac offset bit and width*/
#define EFUSE_MAC_OFFSET			(128)
#define EFUSE_MAC_VERSION_OFFSET	(132)
#define EFUSE_MAC1_OFFSET			(135)
#define EFUSE_MAC2_OFFSET			(159)
#define EFUSE_MAC3_OFFSET			(183)
#define EFUSE_MAC_WIDTH				(96)
#define EFUSE_MAC_VERSION_WIDTH		(3)
#define EFUSE_MAC1_WIDTH			(24)
#define EFUSE_MAC2_WIDTH			(24)
#define EFUSE_MAC3_WIDTH			(24)

#define EFUSE_RO		(2)	/* burn read_protect bit disable */
#define EFUSE_RW		(3)	/* burn read/write_protect bit disable */

#define EFUSE_ACL_SET_BRUN_BIT      (1<<29)
#define EFUSE_ACL_SET_RD_FORBID_BIT (1<<30)
#define EFUSE_BRUN_RD_OFFSET_MASK    (0xFFFFFF)

#endif
