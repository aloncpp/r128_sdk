#ifndef __AACTD_EQ_HW_H__
#define __AACTD_EQ_HW_H__

#include "aactd/communicate.h"


#if defined(CONFIG_ARCH_SUN20IW2)

#define MAX_MAIN_REG_NUM (202)

#define MAX_POST_REG_NUM (28)


#define MAIN_EQ_REG_MIN 	0x100
#define MAIN_EQ_REG_MAX		(0x310 + 0x14 * (20 - 1))
#define POST_EQ_REG_MIN 	0x500
#define POST_EQ_REG_MAX		(0x514 + 0x14 * (5 - 1))

int eq_hw_local_init(void);
int eq_hw_local_release(void);

int eq_hw_write_com_to_local(const struct aactd_com *com);

#else

static inline int eq_hw_local_init(void)
{
	return 0;
}
static inline int eq_hw_local_release(void)
{
	return 0;
}


static inline int eq_hw_write_com_to_local(const struct aactd_com *com)
{
	return 0;
}


#endif

#endif /* ifndef __AACTD_EQ_HW_H__ */
