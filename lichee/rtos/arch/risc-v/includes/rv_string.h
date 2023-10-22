#ifndef _ASM_RISCV_STRING_H
#define _ASM_RISCV_STRING_H

#define __HAVE_ARCH_MEMCPY

#ifdef __HAVE_ARCH_MEMCPY
extern void *memcpy_vector(void *, const void *, size_t);

extern void *vector_memcpy_size128(void *, const void *, size_t);
extern void *vector_memcpy_size64(void *, const void *, size_t);
extern void *vector_memcpy_size32(void *, const void *, size_t);
extern void *vector_memcpy_size16(void *, const void *, size_t);

extern void *vector_memcpy_size128_tail(void *, const void *, size_t);
extern void *memcpy_O3_rvv(void *, const void *, size_t);
#endif

#ifdef __HAVE_ARCH_MEMSET
extern void *memset(void *, int, size_t);
#endif

#ifdef __HAVE_ARCH_MEMMOVE
extern void *memmove(void *, const void *, size_t);
#endif

#endif /* _ASM_RISCV_STRING_H */
