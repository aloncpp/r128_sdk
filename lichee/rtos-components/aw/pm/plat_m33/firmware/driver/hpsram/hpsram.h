#ifndef __HPSRAM_H__
#define __HPSRAM_H__

/* #include <common.h>  for standby.bin*/
/**********************
打印等级宏定义
**********************/

//#define TIMING_DEBUG
/* #define ERROR_DEBUG for standby.bin*/
//#define AUTO_DEBUG

//int printf(const char *fmt, ...);    //boot/standby添加
//#define  printf(fmt,args...) printf(fmt ,##args)  //boot/standby添加

//#define DEBUG_LEVEL_1
//#define DEBUG_LEVEL_4
/* #define DEBUG_LEVEL_8 for standby.bin*/

/* #define msg(fmt,args...)		printf(fmt ,##args) for standby.bin*/

#if defined DEBUG_LEVEL_8
#define dram_dbg_8(fmt,args...)	 printf(fmt ,##args)
#define dram_dbg_4(fmt,args...)	 printf(fmt ,##args)
#define dram_dbg_0(fmt,args...)  printf(fmt ,##args)
#elif defined DEBUG_LEVEL_4
#define dram_dbg_8(fmt,args...)
#define dram_dbg_4(fmt,args...)  printf(fmt ,##args)
#define dram_dbg_0(fmt,args...)  printf(fmt ,##args)
#elif defined DEBUG_LEVEL_1
#define dram_dbg_8(fmt,args...)
#define dram_dbg_4(fmt,args...)
#define dram_dbg_0(fmt,args...)  printf(fmt ,##args)
#else
#define dram_dbg_8(fmt,args...)
#define dram_dbg_4(fmt,args...)
#define dram_dbg_0(fmt,args...)
#endif



#if defined TIMING_DEBUG
#define dram_dbg_timing(fmt,args...)  printf(fmt ,##args)
#else
#define dram_dbg_timing(fmt,args...)
#endif

#if defined ERROR_DEBUG
#define dram_dbg_error(fmt,args...)  printf(fmt ,##args)
#else
#define dram_dbg_error(fmt,args...)
#endif


#if defined AUTO_DEBUG
#define dram_dbg_auto(fmt,args...)  printf(fmt ,##args)
#else
#define dram_dbg_auto(fmt,args...)
#endif


typedef struct __DRAM_PARA
{
	//normal configuration
	unsigned int        dram_clk;
	unsigned int        dram_type;		//dram_type		DDR2: 2	;DDR3: 3;LPDDR2: 6;	LPDDR3: 7		DDR3L: 31
//	unsigned int        lpddr2_type;	//LPDDR2 type		S4:0	S2:1 	NVM:2
    unsigned int        dram_zq;		//do not need
    unsigned int		dram_odt_en;

	//control configuration
	unsigned int		dram_para1;
    unsigned int		dram_para2;

	//timing configuration
	unsigned int		dram_mr0;
    unsigned int		dram_mr1;
    unsigned int		dram_mr2;
    unsigned int		dram_mr3;
    unsigned int		dram_tpr0;	//DRAMTMG0
    unsigned int		dram_tpr1;	//DRAMTMG1
    unsigned int		dram_tpr2;	//DRAMTMG2
    unsigned int		dram_tpr3;	//DRAMTMG3
    unsigned int		dram_tpr4;	//DRAMTMG4
    unsigned int		dram_tpr5;	//DRAMTMG5
   	unsigned int		dram_tpr6;	//DRAMTMG8

    //reserved
    unsigned int		dram_tpr7;
    unsigned int		dram_tpr8; // used
    unsigned int		dram_tpr9; // used
    unsigned int		dram_tpr10;
    unsigned int		dram_tpr11;// used
    unsigned int		dram_tpr12;// used
    unsigned int		dram_tpr13;
}__dram_para_t;


signed int mctl_init(const unsigned int *dram_config);
// static __inline void hpsram_standby2_enter(int hs_flag);
// static __inline void hpsram_standby2_exit(void);

// __inline void hpsram_standby2_enter(int hs_flag);
// __inline void hpsram_standby2_exit(void);

//void hpsram_standby2_enter(int hs_flag);
void hpsram_standby2_enter(int hs_flag); /* for standby.bin */
void hpsram_standby2_exit(__dram_para_t *para); /* for standby.bin */
//void hpsram_standby2_exit(void);
//
//void hpsram_standby3_enter(int hs_flag);
//void hpsram_standby3_exit(void);

#endif

