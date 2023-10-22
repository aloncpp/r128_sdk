#ifndef __STANDBY_SCB_H__
#define __STANDBY_SCB_H__

#define SCB_RW_REGS_NUM		(20U)

#define SCB_ACTLR_REG		(0xe000e008)
#define SCB_ICSR_REG		(0xe000ed04)
#define SCB_CPACR_REG		(0xe000ed88)
#define SCB_NSACR_REG		(0xe000ed8c)

void scb_save(void);
void scb_restore(void);

#endif /* __STANDBY_SCB_H__ */
