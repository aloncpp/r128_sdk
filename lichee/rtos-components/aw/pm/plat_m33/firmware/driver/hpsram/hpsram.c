/*
 * History:
 * 			20110321		liqiang		V0.10		Initial Version base on "PSRAM FW GUIDE"
 *
 *
 */

#include <common.h>
#include "hpsram_reg.h"
#include "hpsram.h"

#define VERSION		"V2.00"
/* #define DISABLE_AUTO_REFRESH */

#define RET_OK      0 /* for standby.bin */

#define get_wvalue(addr)    (*((volatile unsigned int   *)(addr)))
#define put_wvalue(addr, v) (*((volatile unsigned long  *)(addr)) = (unsigned long)(v))

unsigned int standby_flag = 0;


//extern unsigned int standby_flag;
int hpsram_init(__dram_para_t *para);
void mc_init(__dram_para_t *para);
void phy_init(__dram_para_t *para);
static __inline void hpsram_clk_init(__dram_para_t *para);
//extern void psram_rw_test(void);

// delay for sim
void hspsram_sim_delay(u32 n)
{
	while(n--);
}

/**
* sr32 - clear & set a value in a bit range for a 32 bit address
*/
static void sr32(unsigned int addr, unsigned int start_bit, unsigned int num_bits, unsigned int value)
{
	unsigned int tmp, msk = (1 << num_bits) - 1;
	tmp = readl(addr) & ~(msk << start_bit);
	tmp |= value << start_bit;
	writel(tmp, addr);
}

//hs_flag, 1:half sleep; 0: SR
void hpsram_standby2_enter(int hs_flag)
{
	sr32(HPSRAM_CTRL,3,1,0);
	if(hs_flag){
		standby_flag = 0x1; // half sleep
		sr32(SREF,0,1,0);
		sr32(SREF,1,1,1);
	}else{
		standby_flag = 0x2; // SR
		sr32(SREF,0,1,1);
		sr32(SREF,1,1,0);
	}
	while(((get_wvalue(STATR)>>4)&0xf)<= 1); //LP_STATE
	sr32(INIT,0,1,1);
	sr32(INIT,0,1,0);

	sr32(PHY_VREF_CTRL,0,6,0);//DX VREF // 450
	sr32(PHY_VREF_CTRL,6,1,0);//AC VREF  EN // 900
	sr32(PHY_VREF_CTRL,13,1,0);//ZQ VREF EN // 1127

	sr32(VDD_APP_PWROFF_GATING,0,1,1); //zq pad latch
	sr32(VDD_APP_PWROFF_GATING,1,1,1); //IO pad latch

	sr32(VDD_APP_PWROFF_GATING,2,1,1); //poch

	hspsram_sim_delay(5);
}
void hpsram_standby2_exit(__dram_para_t *para)
{
#if 0
	sr32(VDD_APP_PWROFF_GATING,0,1,0);
	sr32(VDD_APP_PWROFF_GATING,1,1,0);

	sr32(INIT,2,1,1); //闂佽崵濮撮幖顐︽偪閸モ晜宕查柛鎰ㄦ櫔缁憋絾淇婇妶鍛殶闁绘挸鐗撻弻娑㈠箳閹惧墎鐣靛銈庡亝閸旀瑥鐣烽悩缁樻櫢闁跨噦鎷�
	if(hs_flag){
		sr32(INIT,3,1,1);
		}else{
			sr32(INIT,4,1,1); //闂備胶鍘ч〃搴㈢濠婂牆鏄ユ繛鎴欏灩濡﹢鏌ｅΔ锟介悡顒佹媴閸撴彃娈ㄩ梺璺ㄥ櫐閹凤拷
		}
	sr32(INIT,0,1,1);
	while(!(get_wvalue(STATR)&(1<<8)));
	sr32(INIT,0,1,0);
	sr32(INIT,3,1,0);
	sr32(INIT,4,1,0);
	while((get_wvalue(STATR)>>4)&0xf);  //闂備胶鍎甸弲婊冾焽濞嗘挸鏄ラ悘鐐插⒔閳绘棃鏌ㄩ悤鍌涘0
	sr32(HPSRAM_CTRL,3,1,0);
#endif
	sr32(VDD_APP_PWROFF_GATING,0,1,0);
	sr32(VDD_APP_PWROFF_GATING,1,1,0);
	hpsram_init(para);
//	mctl_init();
}

unsigned int dramc_simple_wr_test(unsigned int dram_size, unsigned int test_length)
{
	/* DRAM Simple write_read test
	 * 2 ranks:  write_read rank0 and rank1;
	 * 1rank  : write_read rank0 base and half size;
	 * */
	unsigned int i;
	unsigned int half_size;
	unsigned int val;
	half_size = ((dram_size >> 1) << 20);

	for(i = 0; i < test_length; i++)
	{
		mctl_write_w(0x01234567 + i, (DRAM_BASE_ADDR + i*4));	//rank0
		mctl_write_w(0xfedcba98 + i, (DRAM_BASE_ADDR + half_size + i*4));	//half size (rank1)
	}

	for(i = 0; i < test_length; i++)
	{
		val = mctl_read_w(DRAM_BASE_ADDR + half_size + i*4);
		if(val != (0xfedcba98 + i)) /* Write last,read first */
		{
			dram_dbg_error("DRAM simple test FAIL.\n");
			dram_dbg_error("%x != %x at address %x\n",val,0xfedcba98 + i,DRAM_BASE_ADDR + half_size + i*4);
			return DRAM_RET_FAIL;
		}
		val = mctl_read_w(DRAM_BASE_ADDR + i*4);
		if(val != (0x01234567 + i))
		{
			dram_dbg_error("DRAM simple test FAIL.\n");
			dram_dbg_error("%x != %x at address %x\n",val,0x01234567 + i,DRAM_BASE_ADDR + i*4);
			return DRAM_RET_FAIL;
		}
	}
	dram_dbg_0("DRAM simple test OK.\n");
	return DRAM_RET_OK;
}


static __inline void hpsram_clk_init(__dram_para_t *para)
{
	/* u32 rdata; */
//	unsigned int rval = 0;
//	unsigned int N = 0;
//	unsigned int M=2;
//	unsigned int pll_clk;
//	pll_clk = (para->dram_clk << 1);
//	N = pll_clk *M/24;

	// first close the gating and rst
	sr32(BUS_CLK_GATING_CTRL0,21,1,0); 	// HSPSRAM_CTRL_CLK_GATING,0:gated off,1: running
	sr32(HSPSRAM_CLKCFG,31,1,0);		// clk enable
	sr32(DEV_RST_CTRL0,21,1,0); 		// 0:rst;1:work


	sr32(VDD_APP_PWROFF_GATING,2,1,0);//poch

	//config dpll to 1920M
	/* DPLL1 output frequency has been configurated on early stage */
	//put_wvalue(DPLL1_CTRL,0xe0000301);
//	put_wvalue(DPLL2_CTRL,0xe0000181);
	/* FIXME: need to adapt different external crystal frequency */
	put_wvalue(DPLL3_CTRL,0xe0000281);
/*
 * 鐠佸墽鐤咲PLL3妫版垹宸�
 */
//	sr32(DPLL3_CTRL,29,3,0x7);
//	sr32(DPLL3_CTRL,4,8,N);
//	sr32(DPLL3_CTRL,0,4,(M-1));

	//enable the power for dpll1 and dpll3
	sr32(CLK_LDO_CTRL,0,1,1);
	sr32(CLK_LDO_CTRL,16,1,1);

	//enable dpll
	sr32(DPLL1_CTRL,31,1,1);
	sr32(DPLL2_CTRL,31,1,1);
	sr32(DPLL3_CTRL,31,1,1);

	hspsram_sim_delay(200000);
//	sr32(VDD_APP_PWROFF_GATING,2,1,0);//poch

	//choose DIV for ddr clk
	//CK1_HSPSRAM_DIV 0:M=3; 1:M=2.5; 2:M=2; 3:M=1
	sr32(DPLL1_OUT_CTRL,12,2,2);
	//CK3_HSPSRAM_DIV 00:M=3;01:M=2.5;10:M=2;11:M=1
	sr32(DPLL3_OUT_CTRL,12,2,3);

//	sr32(DPLL3_OUT_CTRL,12,2,0x3);//DIV-M =1
//	if(((para->dram_tpr7>>0) & 0x1) == 0){
//		sr32(DEV_CLK_CTRL,21,1,0);		//choose source DPLL1
//		sr32(DPLL1_OUT_CTRL,15,1,1);
//		sr32(DPLL3_OUT_CTRL,15,1,0);
//	}else{
//		sr32(DEV_CLK_CTRL,21,1,1);		//闁瀚―PLL3閿涘瞼鐤�0閸掓瑤璐烡PLL1 choose source DPLL3
//		sr32(DPLL1_OUT_CTRL,15,1,0);	//disable DPLL1
//		sr32(DPLL3_OUT_CTRL,15,1,1);    //enablDPLL3
//	}

//	sr32(BUS_CLK_GATING_CTRL0,21,1,0);
	sr32(BUS_CLK_GATING_CTRL0,21,1,1);	//enable the ctrl clk ,close gating
	sr32(HSPSRAM_CLKCFG,31,1,1);	//enable the hspsram clk ,close gating
//	sr32(DEV_RST_CTRL0,21,1,0);
	sr32(DEV_RST_CTRL0,21,1,1);	//release the module rst

}
unsigned int auto_cal_timing(unsigned int time_ns,unsigned int clk)
{
	unsigned int value;
	value = (unsigned int)((time_ns * clk) / 1000) + ((((unsigned int)(time_ns * clk) % 1000) != 0) ? 1 : 0);
	return value;
}
static __inline void hpsram_timing_init(__dram_para_t *para){
	unsigned int freq;
//	unsigned int reg_val;
	unsigned int tdqsck = 5;
	unsigned int trefi = 0x30c;
	unsigned int refw_mode= 0;
	unsigned int trfc = 0xc;
	unsigned int trc = 0xc;
	unsigned int tcphr = 0x4;
	unsigned int tcphw = 0x6;
	unsigned int tcem = 0xc;
	unsigned int tpu = 0x7530;
	unsigned int trst = 0x7d0;
	unsigned int txphs = 0x14;
	unsigned int txsr = 0xe;
//	unsigned int txpsr = 0x1;
	unsigned int tsref = 0x14;
	unsigned int txhs = 0x7530;
	unsigned int ths = 0x7530;
	unsigned int tzqcl = 0xc8;
	unsigned int tzqcal = 0xc8;
	unsigned int tzqcrst = 0xc8;
	unsigned int tzqcs = 0x64;
	unsigned int tmrw = 0x15;
	unsigned int trl = 0x1d;
	unsigned int twl = 0xe;
	unsigned int mr00 = 0x7;



	freq = para->dram_clk/4;
	//config tdqsck
	tdqsck = auto_cal_timing(5.5,freq*4);
	tdqsck = 5;
	sr32(TDQSCK,0,4,tdqsck);

	//config trefi
	refw_mode = (get_wvalue(HPSRAM_TMG0)>>28) & 0X1;//????????????????闁板秶鐤唌r4?
	if(refw_mode==0)
		trefi =	(3900*freq)/1000 ;
	if(refw_mode==1)
		trefi =	(7800*freq)/1000 ;
	sr32(HPSRAM_TMG0,0,18,trefi);

	//config trfc
	if (para->dram_type == 0xb){
		trfc = auto_cal_timing(60, freq);
		}else{
		trfc = auto_cal_timing(90, freq);
		}
	trfc = trfc+1;
	sr32(HPSRAM_TMG0,20,7,trfc);


	trc	=auto_cal_timing(60, freq);
	sr32(HPSRAM_TMG1,0,7,trc);

	tcphr=auto_cal_timing(20, freq);
	sr32(HPSRAM_TMG1,8,7,tcphr);
	tcphw=auto_cal_timing(30, freq);
	sr32(HPSRAM_TMG1,16,7,tcphw);
	tcem = auto_cal_timing(60, freq);//
	sr32(HPSRAM_TMG1,24,7,tcem);

	tpu=auto_cal_timing(150000, freq);
	sr32(HPSRAM_TMG2,16,16,tpu);
	trst=auto_cal_timing(10000, freq);
	sr32(HPSRAM_TMG2,0,16,trst);

	txphs = auto_cal_timing(100, freq);
	sr32(HPSRAM_TMG3,24,7,txphs);

	if (para->dram_type == 0xb)
	{
		txsr= auto_cal_timing(70, freq);
	}
	else
	{
		txsr= auto_cal_timing(100, freq);
	}

	sr32(HPSRAM_TMG3,16,7,txsr);
	//txpsr 閺堬拷鐏忔垳璐�1
	sr32(HPSRAM_TMG3,8,7,0x1);
	tsref = auto_cal_timing(100, freq);
	sr32(HPSRAM_TMG3,0,7,tsref);

	txhs=auto_cal_timing(150000, freq);
	sr32(HPSRAM_TMG4,16,16,txhs);
	ths=auto_cal_timing(150000, freq);
	sr32(HPSRAM_TMG4,0,16,ths);

	tzqcl=auto_cal_timing(1000, freq);
	if (para->dram_type == 0xa){
		sr32(HPSRAM_TMG5,16,16,tzqcl);
	}
	tzqcal=auto_cal_timing(1000, freq);
	if (para->dram_type == 0xb){
		sr32(HPSRAM_TMG5,0,16,tzqcal);
	}

	tzqcrst=auto_cal_timing(1000, freq);
	sr32(HPSRAM_TMG6,16,16,tzqcrst);   //for 256M
	tzqcs=auto_cal_timing(500, freq);
	sr32(HPSRAM_TMG6,0,16,tzqcs);		 //for 256M

	tmrw=auto_cal_timing(105, freq);
	sr32(HPSRAM_TMG7,0,7,tmrw);
	if(para->dram_clk<= 200){
	trl= 0x9;
	twl= 0x5;
	mr00 = 0x7;
	}else if(para->dram_clk<= 333){
	trl= 0xd;
	twl= 0x5;
	mr00 = 0x6;
	}else if(para->dram_clk<= 400){
	trl= 0x10;
	twl= 0x6;
	mr00 = 0x5;
	}else if(para->dram_clk<= 533){
	trl= 0x14;
	twl= 0xa;
	if (para->dram_type == 0xb){
		mr00=0x4;
		}else{
		mr00=0x0;
		}
	}else if(para->dram_clk<= 667){
		if (para->dram_type == 0xb){
			trl= 0x18;
			twl= 0xc;
			mr00=0x0;
		}else{
			trl= 0x1d;
			twl= 0xe;
			mr00=0x1;
		}

	}else if(para->dram_clk<= 800){
	trl= 0x1d;
	twl= 0xe;
	mr00=0x1;
	}else if(para->dram_clk<= 933){
	trl= 0x21;
	twl= 0x10;
	mr00=0x2;
	}else if(para->dram_clk<= 1066){
	trl= 0x25;
	twl= 0x12;
	mr00=0x3;
	}else{
	trl= 0x25;
	twl= 0x12;
	mr00=0x3;
	}
	sr32(HPSRAM_TMG7,24,7,trl);
	sr32(HPSRAM_TMG7,8,7,twl);
	sr32(MR_CFG0,0,3,mr00);
}
void mc_init(__dram_para_t *para)
{
	sr32(DEV_RST_CTRL0,21,1,1);
	sr32(BUS_CLK_GATING_CTRL0,21,1,1);

	/////**************************************!!!!!!!!!!!!!!!!!!
	sr32(PHY_INIT_CTRL0,15,1,0);//zqcal
	sr32(PHY_INIT_CTRL0,16,1,1);//ddlcal
	sr32(PHY_INIT_CTRL0,29,1,0);//cfg_phy_reset

	//CONFIG THE VREF
	sr32(PHY_VREF_CTRL,0,6,8);//DX VREF
	sr32(PHY_VREF_CTRL,7,6,8);//AC VREF
	sr32(PHY_VREF_CTRL,14,6,8);	//ZQ VREF
	sr32(PHY_VREF_CTRL,6,1,1);//AC VREF	EN
	sr32(PHY_VREF_CTRL,13,1,1);//ZQ VREF EN
	sr32(PHY_VREF_CTRL,30,1,1);//DX VREF EN

	sr32(PHY_IO_CTRL,31,1,1);	//enable AC normal work mode
	sr32(PHY_IO_CTRL,7,1,1);	//enable DC normal work mode

	put_wvalue(CLKEN,0x7);	//enable the sclk, hdrclk and release the reset hdr
	
	//	sr32(DPLL3_OUT_CTRL,12,2,0x3);//DIV-M =1
	if(((para->dram_tpr7>>0) & 0x1) == 0){
		sr32(DEV_CLK_CTRL,21,1,0);		//choose source DPLL1
		sr32(DPLL1_OUT_CTRL,15,1,1);
		sr32(DPLL3_OUT_CTRL,15,1,0);
	}else{
		sr32(DEV_CLK_CTRL,21,1,1);		//闁瀚―PLL3閿涘瞼鐤�0閸掓瑤璐烡PLL1 choose source DPLL3
		sr32(DPLL1_OUT_CTRL,15,1,0);	//disable DPLL1
		sr32(DPLL3_OUT_CTRL,15,1,1);    //enablDPLL3
	}

	hspsram_sim_delay(1000000);

	sr32(ZQDR1,1,6,35);
	sr32(ZQDR1,9,6,40);
	sr32(ZQDR1,17,6,23);
	sr32(ZQDR1,25,6,27);

	sr32(ZQDR0,1,6,35);
	sr32(ZQDR0,9,6,40);
	sr32(ZQDR0,17,6,35);
	sr32(ZQDR0,25,6,40);

	if (para->dram_type == 0xb){
		put_wvalue(ADRESS_MAP0,0x8b0);
		put_wvalue(ADRESS_MAP1,0x8b0);
	}else{
		put_wvalue(ADRESS_MAP0,0x7e0);
		put_wvalue(ADRESS_MAP1,0x7e0);
	}
	 //ONLY CONFIG HSPRAM MR0
	// added by zss
	if(para->dram_clk<= 200){
		sr32(MR_CFG0,0,8,0x17);//mode stre閿涘L=9 WL=5 FREQ=200 drive strength=48鎯� PD/PU
	}else if(para->dram_clk<= 333){
		sr32(MR_CFG0,0,8,0x16);//mode stre閿涘L=13 WL=5 FREQ=333 drive strength=48鎯� PD/PU
	}else if(para->dram_clk<= 400){
		sr32(MR_CFG0,0,8,0x15);//mode stre閿涘L=16 WL=6 FREQ=333 drive strength=48鎯� PD/PU
	}else if(para->dram_clk<= 533){
		sr32(MR_CFG0,0,8,0x14);//mode stre閿涘L=20 WL=10 FREQ=333 drive strength=48鎯� PD/PU
	}else if(para->dram_clk<= 667){
		sr32(MR_CFG0,0,8,0x10);//mode stre閿涘L=24 WL=12 FREQ=333 drive strength=48鎯� PD/PU
	}else if(para->dram_clk<= 800){
		sr32(MR_CFG0,0,8,0x11);//mode stre閿涘L=29 WL=14 FREQ=333 drive strength=48鎯� PD/PU
	}else if(para->dram_clk<= 933){
		sr32(MR_CFG0,0,8,0x12);//mode stre閿涘L=29 WL=14 FREQ=333 drive strength=48鎯� PD/PU
	}else if(para->dram_clk<= 1066){
		sr32(MR_CFG0,0,8,0x13);//mode stre閿涘L=29 WL=14 FREQ=333 drive strength=48鎯� PD/PU
	}

	//
	if (para->dram_type == 0xb){
		put_wvalue(CMD_CODE0,0xAD95B4CF);//??
	}else{
		put_wvalue(CMD_CODE0,0x8D05E4CF);
		sr32(MR_CFG0, 8, 8, 0x00); // 256
	}
		put_wvalue(CMD_CODE1,0x0);//read

		hpsram_timing_init(para);

		/*
		 * PMU reg set//??????????????????????????
		 */

		sr32(0x40038180,0,12,0xfff);
		sr32(0x40038180,0,1,0x1);

		sr32(REF_MODE,1,3,7);//REF BURST=7+1
		sr32(REF_MODE,0,1,0);//ref burst mode
		sr32(SREF,8,7,0x40);
		
		if (para->dram_type == 0xb){
			sr32(SREF,3,1,0);
		}else{
			sr32(SREF,3,1,1);
		}
		if (para->dram_type == 0xb){
			sr32(SREF,2,1,1);
		}else{
			sr32(SREF,2,1,0);
		}

		sr32(HPSRAM_CTRL,1,2,(para->dram_tpr13>>10)&0x3);//bit2:auto clock gating; bit1:write combine???????????????????

		sr32(WCAM_THR,24,7,0x40);
		sr32(WCAM_THR,20,4,0x0);
		sr32(WCAM_THR,16,4,0xf);
		sr32(WCAM_THR,8,6,0x1c);
		sr32(WCAM_THR,0,6,0x12);

		sr32(PREF_CFG0,1,1,(para->dram_tpr13>>9)&0x1); //prefetch en	????????????
		if (para->dram_type == 0xb){
			sr32(PREF_CFG0,0,1,(para->dram_tpr13>>8)&0x1);// page read access enable????????????????????
		}

		put_wvalue(PREF_CFG1,0x00070007);
		put_wvalue(PREF_IADDR_S,0x0);
		put_wvalue(PREF_IADDR_E,0xFFFFFFFF);	//??????????????MAX


}
void phy_gate_mode_set(__dram_para_t *para)
{
	unsigned int dqs_gating_mode = 0;
	dqs_gating_mode = (para->dram_tpr13 >> 2) & 0x3;
	switch(dqs_gating_mode)
	{
		case 1:	//NL
			sr32(PHY_IO_CTRL,20,8,0xc4);//PHY IO CTRL 娑撳﹣绗呴幏澶屾暩闂冿拷 //add by zss
//			rxen_training(para);

            sr32(PHY_GATE_TRAINING_CTRL0,8,4,0); //gate_open_cnt
            sr32(PHY_GATE_TRAINING_CTRL0,0,1,0); //enable normal mode
            sr32(PHY_GATE_TRAINING_CTRL0,1,1,1); //enable nl dqs gate
            sr32(PHY_GATE_TRAINING_CTRL0,2,1,0); //enable normal

            //gate_open_phase
            sr32(PHY_GATE_TRAINING_CTRL0,26,3,(para->dram_tpr8)& 0xf);
           	//gate_en_post_p0
            sr32(PHY_GATE_TRAINING_CTRL0,23,3,(para->dram_tpr8>>4)& 0xf);
           	//gate_en_post_p1
            sr32(PHY_GATE_TRAINING_CTRL0,20,3,(para->dram_tpr8>>8)& 0xf);
            //gate_lcdl_dly
            sr32(PHY_GATE_TRAINING_CTRL0,12,8,(para->dram_tpr8>>12)& 0xff);

			dram_dbg_8("DRAM DQS gate is NL.\n");
			break;
		case 2:	//auto gating pull up
			sr32(PHY_DLY_CTRL0,25,1,1);//PU AUTO GATE enable
			sr32(PHY_IO_CTRL,20,8,0x4C);//PHY IO CTRL 濠电偞鍨堕幐鎼佹晝閿濆洨绠旈柛娑欐綑缁狀垰顭跨捄鐑樻拱闁哄棌鏅犲濠氬礃閹稿骸顏�
			dram_dbg_8("DRAM DQS gate is PU mode.\n");
			break;
		default:
			//close DQS gating--auto gating pull down
			sr32(PHY_IO_CTRL,20,8,0xc4);//PHY IO CTRL 濠电偞鍨堕幐鎼佹晝閿濆洨绠旈柛娑欐綑缁狀垰顭跨捄鐑樻拱闁哄棌鏅犲濠氬礃閹稿骸顏�
			sr32( PHY_DQS_BDL,30,1,1);
			dram_dbg_8("DRAM DQS gate is PD mode.\n");
			break;
	}
}

void phy_delay_timing(__dram_para_t *para)
{

	int reg_val=0;
	int Delay = 0x0;

	reg_val= (para->dram_tpr8 >>0)&0x3; //0x1閿涙dqs lcdl set 0x2閿涙dq lcdl set

    if(reg_val == 0x1){
	    sr32(PHY_RDQS_LCDL_CTRL,0,8,(para->dram_tpr9 >> 0) &0xff);	//??????????????????????????????
	    sr32(PHY_RDQSN_LCDL_CTRL,0,8,(para->dram_tpr9 >> 0) &0xff);
 	}else{
		Delay=get_wvalue(PHY_RDQS_LCDL_CTRL)&0xff;//default delay by dll open
		para->dram_tpr9 &= ~(0xff<<0);
    	para->dram_tpr9 |= Delay<<0;
	}

    if(reg_val == 0x2){
	    sr32(PHY_WDQ_LCDL_CTRL,0,8,(para->dram_tpr9 >> 8) &0xff);
 	}else{
		Delay=get_wvalue(PHY_WDQ_LCDL_CTRL)&0xff;//default delay by dll open
		para->dram_tpr9 &= ~(0xff<<8);
    	para->dram_tpr9 |= Delay<<8;
	}

    //WRITE DQ DELAY
    reg_val= (para->dram_tpr11)& 0x3f;

	sr32(PHY_WDQ_BDL0,24,6,reg_val);
	sr32(PHY_WDQ_BDL0,16,6,reg_val);
	sr32(PHY_WDQ_BDL0,8,6,reg_val);
	sr32(PHY_WDQ_BDL0,0,6,reg_val);

	sr32(PHY_WDQ_BDL1,24,6,reg_val);
	sr32(PHY_WDQ_BDL1,16,6,reg_val);
	sr32(PHY_WDQ_BDL1,8,6,reg_val);
	sr32(PHY_WDQ_BDL1,0,6,reg_val);

	//READ DQ DELAY
	reg_val= (para->dram_tpr12)& 0x3f;

	sr32(PHY_RDQ_BDL0,24,6,reg_val);
	sr32(PHY_RDQ_BDL0,16,6,reg_val);
	sr32(PHY_RDQ_BDL0,8,6,reg_val);
	sr32(PHY_RDQ_BDL0,0,6,reg_val);

	sr32(PHY_RDQ_BDL1,24,6,reg_val);
	sr32(PHY_RDQ_BDL1,16,6,reg_val);
	sr32(PHY_RDQ_BDL1,8,6,reg_val);
	sr32(PHY_RDQ_BDL1,0,6,reg_val);


	sr32(PHY_DQS_BDL,24,6,((para->dram_tpr12>>16)& 0x3f));		//RDQSN
	sr32(PHY_DQS_BDL,18,6,((para->dram_tpr12>>16)& 0x3f));		//RDQS
	sr32(PHY_DQS_BDL,12,6,((para->dram_tpr11>>16)& 0x3f));		//WDQS
	sr32(PHY_DQS_BDL,6,6,((para->dram_tpr12>>0)& 0x3f));		// read data QM
	sr32(PHY_DQS_BDL,0,6,((para->dram_tpr11>>0)& 0x3f));		//write data DM

    reg_val= (para->dram_tpr10 >> 0)& 0xfff; // CK BDL
    sr32(PHY_ACIO_CTRL, 7, 12, reg_val);
    reg_val= (para->dram_tpr10 >> 12)& 0xfff;// CKE BDL
    sr32(PHY_ACIO_CTRL, 19, 12, reg_val);
}


void  phy_init(__dram_para_t *para)
{
		while((get_wvalue(0x4003a074)&0x7)!=4); //this reg doesn't exist in spec
		hspsram_sim_delay(1000000);
		phy_delay_timing(para);

		sr32(PHY_DLY_CTRL0,29,3,6);//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! PHASE SOURSE CHOICE
		sr32(PHY_DLY_CTRL0,26,3,2);//PHASE VALUE
		phy_gate_mode_set(para);
}
void uhs_psram_init(__dram_para_t *para)
{
////	if((para->dram_tpr7 >> 1)&0x3 != 0x0)
	if(standby_flag != 0x0)
	{
		sr32(INIT,2,1,1); //闁荤姴鎼悿鍥╂崲閸愨晪绱ｆ俊銈呭暟閻撳牓鏌涢幒鎾剁畵妞ゎ偅鍔欏畷鐘诲冀閻㈢數顦梺鍦懗閸♀晜鏂�phy闂佸憡甯楃换鍌烇綖閹版澘绀岄柨鐕傛嫹
		if(standby_flag == 0x1){
			sr32(INIT,3,1,1);// after power-up, in half sleep mode
			sr32(INIT,4,1,0);
		}
		else if(standby_flag == 0x2){
			sr32(INIT,3,1,0);
			sr32(INIT,4,1,1);// after power-up, in SR mode
		}
		sr32(INIT,0,1,1); // start to initialize
		while(!(get_wvalue(STATR)&(1<<8)));
		sr32(INIT,0,1,0);
		sr32(INIT,3,1,0);
		sr32(INIT,4,1,0);
		while((get_wvalue(STATR)>>4)&0xf); // wait for zero
		sr32(HPSRAM_CTRL,3,1,0); // hif闂佸憡绋掗崹婵嬪箮閵堝绠抽柕澶堝劚缂嶆挻鎱ㄥ┑鎾跺埌闁绘牞娉涜灒闁炽儱纾涵锟�
	}
	else
	{
		sr32(INIT,0,1,1); // start to initialize
		while(!(get_wvalue(STATR)&(1<<8)));
		sr32(INIT,0,1,0);
		sr32(HPSRAM_CTRL,3,1,0);
	}
}

int hpsram_init(__dram_para_t *para)
{
	unsigned int ret_val = 0;

	hpsram_clk_init(para);
	mc_init(para);
	phy_init(para);
	uhs_psram_init(para);
//	msg("init done!\n");
//	sr32(MC_CCCR,31,1,1);
	sr32(0x40102014,31,1,1);//clear the credit for msi
//	hspsram_sim_delay(10000000);

	dram_dbg_0("DRAM BOOT DRIVE INFO: %s\n", VERSION);
	dram_dbg_0("DRAM CLK = %d MHZ\n", para->dram_clk);
	dram_dbg_0("dram_tpr11 = 0x%x , dram_tpr12 =0x%x\n",para->dram_tpr11,para->dram_tpr12);
	dram_dbg_0("dram_tpr9 = 0x%x\n",para->dram_tpr9);

	if(((para->dram_tpr13 >> 28) & 0x1))
	{
		ret_val = dramc_simple_wr_test(8, 0x10000);
		if(ret_val)
			return 0;
	}
	if (para->dram_type == 0xb){
		dram_dbg_0("DRAM SIZE =%d MB\n", 8);
	}else{
		dram_dbg_0("DRAM SIZE =%d MB\n", 32);
	}

	return 32;
}

signed int mctl_init(const unsigned int *dram_config)
{
	signed int ret_val=0;
//	signed int reg_val=0;

//	signed int sim_result = 0;
	__dram_para_t dram_para;

	dram_para.dram_clk            = dram_config[0];
	dram_para.dram_type           = dram_config[1]; //a:256M   b:64M
	dram_para.dram_zq             = dram_config[2]; // unused
	dram_para.dram_odt_en         = dram_config[3]; // unused
	dram_para.dram_para1          = dram_config[4]; // unused
	dram_para.dram_para2          = dram_config[5]; // unused
	dram_para.dram_mr0            = dram_config[6];
	dram_para.dram_mr1            = dram_config[7];
	dram_para.dram_mr2            = dram_config[8];
	dram_para.dram_mr3            = dram_config[9];
	dram_para.dram_tpr0           = dram_config[10];
	dram_para.dram_tpr1           = dram_config[11];
	dram_para.dram_tpr2           = dram_config[12];
	dram_para.dram_tpr3           = dram_config[13];
	dram_para.dram_tpr4           = dram_config[14];
	dram_para.dram_tpr5           = dram_config[15];
	dram_para.dram_tpr6           = dram_config[16];
	dram_para.dram_tpr7           = dram_config[17];
	dram_para.dram_tpr8           = dram_config[18];
	dram_para.dram_tpr9           = dram_config[19];
	dram_para.dram_tpr10          = dram_config[20];
	dram_para.dram_tpr11          = dram_config[21];
	dram_para.dram_tpr12          = dram_config[22];
	dram_para.dram_tpr13          = dram_config[23];
/* Initialization */

	put_wvalue(0x40050040,0x83351);
	ret_val = hpsram_init(&dram_para);

	if(ret_val)
		ret_val = 0;
	else
		return 1;

//	ret_val = dramc_ic_test(&dram_para);

/* standby */
#if 0x0
	HSPSRAM_wr_test();
#if 0x0
	hpsram_standby2_enter(1);// half sleep
#else
	hpsram_standby2_enter(0);// SR
#endif
	hpsram_standby2_exit(&dram_para);
	ret_val = HSPSRAM_rd_test_err_ret();
	if(ret_val != RET_OK){
		msg("standby read test, error!");
		return -1;
	}
#endif


	return ret_val;
}





