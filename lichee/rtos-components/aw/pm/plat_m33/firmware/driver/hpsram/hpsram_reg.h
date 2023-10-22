/*=====================================
 *DRAM驱动专用读写函数
 * ====================================
 */
//#ifndef get_wvalue(addr)
//#define get_wvalue(addr)    (*((volatile unsigned int   *)(addr)))
//#endif
#define put_wvalue(addr, v) (*((volatile unsigned long  *)(addr)) = (unsigned long)(v))
#define mctl_write_w(v, addr)       (*((volatile unsigned long  *)(addr)) = (unsigned long)(v))
#define mctl_read_w(addr)           (*((volatile unsigned long  *)(addr)))

#ifndef MSI_BASE
#define MSI_BASE			(0x40102000)
#endif

#define DRAM_RET_OK                 0
#define DRAM_RET_FAIL               1
#define MC_CCCR				(MSI_BASE + 0x14)

#define DRAM_BASE_ADDR              0x0C000000

#define HSPSRAM_COM_BASE		(0x40038000)
#define HSPSRAM_CTRL_BASE		(0x40038100)
#define HSPSRAM_PHY_BASE		(0x4003A000)

#define CLKEN					(HSPSRAM_COM_BASE + 0x0)
#define ADRESS_MAP0				(HSPSRAM_COM_BASE + 0x4)
#define ADRESS_MAP1				(HSPSRAM_COM_BASE + 0x8)

#define INIT					(HSPSRAM_CTRL_BASE + 0x0)
#define MR_CTRL0				(HSPSRAM_CTRL_BASE + 0x4)
#define MR_CTRL1				(HSPSRAM_CTRL_BASE + 0x8)
#define MR_CFG0					(HSPSRAM_CTRL_BASE + 0xC)
#define MR_CFG1					(HSPSRAM_CTRL_BASE + 0x10)
#define CMD_CODE0				(HSPSRAM_CTRL_BASE + 0x18)
#define CMD_CODE1				(HSPSRAM_CTRL_BASE + 0x1C)
#define REF_MODE				(HSPSRAM_CTRL_BASE + 0x20)
#define SREF					(HSPSRAM_CTRL_BASE + 0x24)
#define STATR					(HSPSRAM_CTRL_BASE + 0x28)
#define HPSRAM_CTRL				(HSPSRAM_CTRL_BASE + 0x2C)
#define TDQSCK					(HSPSRAM_CTRL_BASE + 0x30)
#define HPSRAM_TMG0				(HSPSRAM_CTRL_BASE + 0x34)
#define HPSRAM_TMG1				(HSPSRAM_CTRL_BASE + 0x38)
#define HPSRAM_TMG2				(HSPSRAM_CTRL_BASE + 0x3C)
#define HPSRAM_TMG3				(HSPSRAM_CTRL_BASE + 0x40)
#define HPSRAM_TMG4				(HSPSRAM_CTRL_BASE + 0x44)
#define HPSRAM_TMG5				(HSPSRAM_CTRL_BASE + 0x48)
#define HPSRAM_TMG6				(HSPSRAM_CTRL_BASE + 0x4C)
#define HPSRAM_TMG7				(HSPSRAM_CTRL_BASE + 0x50)
#define WCAM_THR				(HSPSRAM_CTRL_BASE + 0x54)
#define PREF_CFG0				(HSPSRAM_CTRL_BASE + 0x58)
#define PREF_CFG1				(HSPSRAM_CTRL_BASE + 0x5C)
#define PREF_IADDR_S			(HSPSRAM_CTRL_BASE + 0x60)
#define PREF_IADDR_E			(HSPSRAM_CTRL_BASE + 0x64)

#define PHY_ZQ_CTRL				(HSPSRAM_PHY_BASE + 0x0)
#define PHY_TIMING_CTRL			(HSPSRAM_PHY_BASE + 0x4)
#define PHY_WDQ_LCDL_CTRL		(HSPSRAM_PHY_BASE + 0x10)
#define PHY_RDQS_LCDL_CTRL		(HSPSRAM_PHY_BASE + 0x18)
#define PHY_RDQSN_LCDL_CTRL		(HSPSRAM_PHY_BASE + 0x1C)
#define PHY_INIT_CTRL0			(HSPSRAM_PHY_BASE + 0x20)
#define PHY_WDQ_BDL0			(HSPSRAM_PHY_BASE + 0x24)
#define PHY_WDQ_BDL1			(HSPSRAM_PHY_BASE + 0x28)
#define PHY_RDQ_BDL0			(HSPSRAM_PHY_BASE + 0x2C)
#define PHY_RDQ_BDL1			(HSPSRAM_PHY_BASE + 0x30)
#define PHY_DQS_BDL				(HSPSRAM_PHY_BASE + 0x34)


#define PHY_DLY_CTRL0			(HSPSRAM_PHY_BASE + 0x38)
#define PHY_IO_CTRL				(HSPSRAM_PHY_BASE + 0x40)
#define PHY_VREF_CTRL			(HSPSRAM_PHY_BASE + 0x44)
#define PHY_STATUS				(HSPSRAM_PHY_BASE + 0x48)
#define ZQDR0					(HSPSRAM_PHY_BASE + 0x4C)
#define ZQDR1					(HSPSRAM_PHY_BASE + 0x50)
#define PHY_ACIO_CTRL			(HSPSRAM_PHY_BASE + 0x58)
#define FPGA_DEBUG_CTRL			(HSPSRAM_PHY_BASE + 0x60)
#define PHY_GATE_TRAINING_CTRL0			(HSPSRAM_PHY_BASE + 0x80)
#define PHY_GATE_TRAINING_CTRL1			(HSPSRAM_PHY_BASE + 0x84)
#define PHY_GATE_TRAINING_CTRL2			(HSPSRAM_PHY_BASE + 0x88)

#define CCMU_AON_BASE			(0x4004C400)
#define CCMU_BASE				(0x4003C000)

#define DPLL1_CTRL				(CCMU_AON_BASE + 0x8C)
#define DPLL2_CTRL				(CCMU_AON_BASE + 0x90)
#define DPLL3_CTRL				(CCMU_AON_BASE + 0x94)
#define DPLL1_OUT_CTRL			(CCMU_AON_BASE + 0xA4)
#define DPLL3_OUT_CTRL			(CCMU_AON_BASE + 0xA8)
#define CLK_LDO_CTRL			(CCMU_AON_BASE + 0xAC)
#define DEV_CLK_CTRL			(CCMU_AON_BASE + 0xE0)

#define BUS_CLK_GATING_CTRL0	(CCMU_BASE + 0x4)
#define DEV_RST_CTRL0			(CCMU_BASE + 0xC)
#define HSPSRAM_CLKCFG			(CCMU_BASE + 0x6C)


#define PMC_BASE				(0x40051400)
#define VDD_APP_PWROFF_GATING	(PMC_BASE + 0x180)
