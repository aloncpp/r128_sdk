#ifndef __BARRIER_H__
#define __BARRIER_H__

static inline void isb(void)
{
	__asm__ volatile("isync" ::: "memory");
}

static inline void dsb(void)
{
	__asm__ volatile("dsync" ::: "memory");
}

static inline void dmb(void)
{
	__asm__ volatile("dsync" ::: "memory");
}

#endif /* __BARRIER_H__ */

