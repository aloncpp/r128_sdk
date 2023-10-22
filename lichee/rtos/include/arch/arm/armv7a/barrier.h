
#ifndef __BARRIER_H_
#define __BARRIER_H_

#define isb(option)             __asm__ __volatile__ ("isb " #option : : : "memory")
#define dsb(option)             __asm__ __volatile__ ("dsb " #option : : : "memory")
#define dmb(option)             __asm__ __volatile__ ("dmb " #option : : : "memory")

#define soft_break(...)     	do { __asm__ __volatile__("bkpt #0" ::: "memory", "cc"); } while(0)

#endif
