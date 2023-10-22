#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#include <common.h>
#include <io.h>

#define MSI_BASE			(0x40102000)
#define HSPSRAM_COM_BASE		(0x40038000)
#define HSPSRAM_CTRL_BASE		(0x40038100)
#define HSPSRAM_PHY_BASE		(0x4003A000)

#define CCMU_AON_BASE			(0x4004C400)
#define CCMU_BASE			(0x4003C000)

#define PMC_BASE			(0x40051400)

#define get_wvalue(addr)    (*((volatile unsigned int *)(addr)))
#define put_wvalue(addr, v) (*((volatile unsigned int *)(addr)) = (unsigned long)(v))

typedef uint32_t u32;

/**
 * sr32 - clear & set a value in a bit range for a 32 bit address
 */
/* static __inline void sr32(unsigned int addr, unsigned int start_bit, unsigned int num_bits, unsigned int value) */
/* { */
	/* unsigned int tmp, msk = (1 << num_bits) - 1; */
	/* tmp = readl(addr) & ~(msk << start_bit); */
	/* tmp |= value << start_bit; */
	/* writel(tmp, addr); */
/* } */

static __inline void aw_delay (u32 n)
{
	while(n--);
}

static __inline void hpsram_clk_init(void){
	/* u32 rdata; */
	put_wvalue(CCMU_AON_BASE+0x8C,0xE0000301);
	put_wvalue(CCMU_AON_BASE+0x90,0xE0000181);
	put_wvalue(CCMU_AON_BASE+0x94,0xE0000281);
	aw_delay(1000);
	sr32(PMC_BASE+0x180,2,1,0);//poch
	sr32(CCMU_AON_BASE+0xA8,15,1,1);
	sr32(CCMU_AON_BASE+0xA8,12,2,0x3);//DIV-M =1
	sr32(CCMU_AON_BASE+0xE0,21,1,1);
	sr32(CCMU_BASE+0x4,21,1,0);
	sr32(CCMU_BASE+0x4,21,1,1);
	sr32(CCMU_BASE+0x6C,31,1,1);
	sr32(CCMU_BASE+0xC,21,1,0);
	sr32(CCMU_BASE+0xC,21,1,1);

}

int hpsram_init(void)
{
	printf("hpsram init\n");
	u32 rdata;
	hpsram_clk_init();
	sr32(HSPSRAM_PHY_BASE+0x0020,16,1,0);
	//COM
	put_wvalue(HSPSRAM_COM_BASE+0x0000,0x7);
	put_wvalue(HSPSRAM_COM_BASE+0x0004,0x8f0);
	put_wvalue(HSPSRAM_COM_BASE+0x0008,0x8f0);
	//phy gate

	aw_delay(100);
//	sr32(HSPSRAM_PHY_BASE+0x0038,25,1,1);//PU 自动gate模式enable
//	sr32(HSPSRAM_PHY_BASE+0x0040,20,8,0x4C);//PHY IO CTRL 上下拉电阻
	sr32(HSPSRAM_PHY_BASE+0x60,0,4,1);//trdata_en=1
	rdata=get_wvalue(HSPSRAM_PHY_BASE+0x60);

	sr32(HSPSRAM_PHY_BASE+0x60,4,3,2);  //no level shift  rl =9
//  sr32(HSPSRAM_PHY_BASE+0x60,4,3,6);//level shift rl=7
	sr32(HSPSRAM_PHY_BASE+0x60,14,6,0x1);
	//CTRL
	sr32(HSPSRAM_CTRL_BASE+0xC,0,8,0x1f);//mode stre
//	sr32(HSPSRAM_CTRL_BASE+0xC,0,8,0x9f);//mode stre


	put_wvalue(HSPSRAM_CTRL_BASE+0x0018,0xAD95B4CF);//颗粒各个命令值
	put_wvalue(HSPSRAM_CTRL_BASE+0x001C,0x2);//命令值
	sr32(HSPSRAM_CTRL_BASE+0x0020,1,3,7);//REF BURST=1
	sr32(HSPSRAM_CTRL_BASE+0x0020,0,1,1);//ref burst mode

	sr32(HSPSRAM_CTRL_BASE+0x24,8,7,0x4);

	sr32(HSPSRAM_CTRL_BASE+0x30,0,4,2);
	sr32(HSPSRAM_CTRL_BASE+0x34,0,18,0x16);
	sr32(HSPSRAM_CTRL_BASE+0x34,20,7,0x1);
	sr32(HSPSRAM_CTRL_BASE+0x38,0,7,0x1);
	sr32(HSPSRAM_CTRL_BASE+0x38,8,7,0x1);
	sr32(HSPSRAM_CTRL_BASE+0x38,16,7,0x1);
	sr32(HSPSRAM_CTRL_BASE+0x38,24,7,0x1);
	put_wvalue(HSPSRAM_CTRL_BASE+0x3C,0x05DC0064);
	sr32(HSPSRAM_CTRL_BASE+0x40,0,7,0x1);
	sr32(HSPSRAM_CTRL_BASE+0x40,8,7,0x1);
	sr32(HSPSRAM_CTRL_BASE+0x40,16,7,0x1);
	sr32(HSPSRAM_CTRL_BASE+0x40,24,7,0x1);
	put_wvalue(HSPSRAM_CTRL_BASE+0x44,0x05DC05DC);
	put_wvalue(HSPSRAM_CTRL_BASE+0x48,0x000A000A);
	put_wvalue(HSPSRAM_CTRL_BASE+0x4C,0x000A0005);
	sr32(HSPSRAM_CTRL_BASE+0x50,0,7,0x2);
	sr32(HSPSRAM_CTRL_BASE+0x50,8,7,0x5); //5->4
	sr32(HSPSRAM_CTRL_BASE+0x50,24,7,0x9);
	put_wvalue(HSPSRAM_CTRL_BASE+0x0054,0x010F1C12);

	put_wvalue(HSPSRAM_CTRL_BASE+0x0058,0x0);//prefetch pra enable


	sr32(HSPSRAM_CTRL_BASE+0x0000,0,1,1);
	sr32(HSPSRAM_CTRL_BASE+0x0000,0,1,0);
	aw_delay(1000);
	rdata=get_wvalue(HSPSRAM_CTRL_BASE+0x0000);
//	while(1);
	while(!(get_wvalue(HSPSRAM_CTRL_BASE+0x0028)&(1<<8)));
//	while(1){
		put_wvalue(HSPSRAM_CTRL_BASE+0x0004,0x7);
		while(1){
			rdata=get_wvalue(HSPSRAM_CTRL_BASE+0x0008);
			if(rdata&0x2) break;
		}
		aw_delay(10);
//	}

	sr32(HSPSRAM_CTRL_BASE+0x002C,0,2,0);
	sr32(HSPSRAM_CTRL_BASE+0x002C,3,1,0);
	sr32(HSPSRAM_CTRL_BASE+0x002C,0,1,0);

	sr32(MSI_BASE+0x14,31,1,1);
	sr32(MSI_BASE+0x14,31,1,0);
	return 0;
}
