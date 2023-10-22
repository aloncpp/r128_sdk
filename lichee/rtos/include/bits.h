#ifndef _RTOS_BITS_H_
#define _RTOS_BITS_H_

#define BITS_PER_LONG 32
#define BITS_PER_LONG_LONG 64

#ifndef BIT
#define BIT(nr)			(1UL << (nr))
#endif

#ifndef BIT_ULL
#define BIT_ULL(nr)			(1ULL << (nr))
#endif

#ifndef BIT_MASK
#define BIT_MASK(nr)		(1UL << ((nr) % BITS_PER_LONG))
#endif

#ifndef BIT_WORD
#define BIT_WORD(nr)		((nr) / BITS_PER_LONG)
#endif

#ifndef BIT_ULL_MASK
#define BIT_ULL_MASK(nr)	(1ULL << ((nr) % BITS_PER_LONG_LONG))
#endif

#ifndef BIT_ULL_WORD
#define BIT_ULL_WORD(nr)	((nr) / BITS_PER_LONG_LONG)
#endif

#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE		8
#endif

#define RTOS_GENMASK(h, l) \
	(((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#define RTOS_GENMASK_ULL(h, l) \
	(((~0ULL) - (1ULL << (l)) + 1) & \
	 (~0ULL >> (BITS_PER_LONG_LONG - 1 - (h))))

#endif  /* _RTOS_BITS_H_ */

