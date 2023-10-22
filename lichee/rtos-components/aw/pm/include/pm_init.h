#ifndef __PM_INIT_H__
#define __PM_INIT_H__

int pm_syscore_init(void);
int pm_devops_init(void);
int pm_wakecnt_init(void);
int pm_wakesrc_init(void);
int pm_init(int argc, char **argv);

#endif
