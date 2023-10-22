#ifndef __BITOPTS_H
#define __BITOPTS_H

#define BIT(nr)			(1UL << (nr))
#define SET_BIT(data, idx)	((data) |= 1 << (idx))
#define CLR_BIT(data, idx)	((data) &= ~(1 << (idx)))



#endif /* __BITOPTS_H */

