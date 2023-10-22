#ifndef __AACTD_EQ_SW_H__
#define __AACTD_EQ_SW_H__

#include "aactd/common.h"
#include "aactd/communicate.h"
#include "local.h"

#include "AudioEqual.h"

#define EQ_WANT_RUN_BIT		(0x1<<0)
#define EQ_ALREADY_RUN_BIT	(0x1<<1)
#define EQ_DESTROY_BIT		(0x1<<2)

#define EQ_SW_THREAD_STACK_SIZE	(8*1024)

#define configAPPLICATION_EQ_SW_PRIORITY	(configMAX_PRIORITIES > 20 ? configMAX_PRIORITIES - 8 : configMAX_PRIORITIES - 3)

#if (defined(CONFIG_COMPONENTS_PROCESS_EQ))  && (defined(CONFIG_ARCH_SUN8IW18) || defined(CONFIG_ARCH_SUN8IW19) || defined(CONFIG_ARCH_SUN8IW20))

int eq_sw_local_init(void);
int eq_sw_local_release(void);

int eq_sw_write_com_to_local(const struct aactd_com *com);

#else

static inline int eq_sw_local_init(void)
{
	return 0;
}
static inline int eq_sw_local_release(void)
{
	return 0;
}


static inline int eq_sw_write_com_to_local(const struct aactd_com *com)
{
	return 0;
}


#endif

#endif /* ifndef __AACTD_EQ_SW_H__ */
