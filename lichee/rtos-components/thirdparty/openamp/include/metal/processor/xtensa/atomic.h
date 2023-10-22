/*
 * @file	xtensa/atomic.h
 * @brief	Xtensa specific atomic primitives for libmetal.
 */

#ifndef __METAL_XTENSA_ATOMIC__H__
#define __METAL_XTENSA_ATOMIC__H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * TODO:
 *   For convenience, currently we implement the atomic primitives by
 *   enabling/disabling interrupt to enter/exit the critical section.
 *   For better performance, we had better implement them with the xtensa
 *   atomic instructions.
 */

#include <hal_interrupt.h>

typedef int atomic_flag;
typedef char atomic_char;
typedef unsigned char atomic_uchar;
typedef short atomic_short;
typedef unsigned short atomic_ushort;
typedef int atomic_int;
typedef unsigned int atomic_uint;
typedef atomic_uint atomic_uintptr_t;
typedef long atomic_long;
typedef unsigned long atomic_ulong;
typedef long long atomic_llong;
typedef unsigned long long atomic_ullong;

#define ATOMIC_FLAG_INIT	0
#define ATOMIC_VAR_INIT(VAL)	(VAL)

typedef enum {
	memory_order_relaxed,
	memory_order_consume,
	memory_order_acquire,
	memory_order_release,
	memory_order_acq_rel,
	memory_order_seq_cst,
} memory_order;

#define atomic_init(OBJ, VAL)						\
	do { *(OBJ) = (VAL); } while (0)
#define atomic_is_lock_free(OBJ)					\
	(sizeof(*(OBJ)) <= sizeof(long))

#define atomic_store(OBJ, VAL)						\
	do { *(OBJ) = (VAL); _Pragma("flush_memory") } while (0)
#define atomic_store_explicit(OBJ, VAL, MO)				\
	atomic_store((OBJ), (VAL))
#define atomic_load(OBJ)						\
	({ _Pragma("flush_memory") *(OBJ); })
#define atomic_load_explicit(OBJ, MO)					\
	atomic_load(OBJ)

#define atomic_flag_test_and_set(FLAG)					\
	({								\
		uint32_t irq_flag = hal_interrupt_disable_irqsave();		\
		__typeof__(FLAG) flag = (FLAG);				\
		__typeof__(*flag) oldval = atomic_load(flag);		\
		atomic_store(flag, 1);					\
		hal_interrupt_enable_irqrestore(irq_flag);			\
		oldval;							\
	})
#define atomic_flag_test_and_set_explicit(FLAG, MO)			\
	atomic_flag_test_and_set(FLAG)
#define atomic_flag_clear(FLAG)						\
	do {								\
		uint32_t irq_flag = hal_interrupt_disable_irqsave();		\
		__typeof__(FLAG) flag = (FLAG);				\
		atomic_store(flag, 0);					\
		hal_interrupt_enable_irqrestore(irq_flag);			\
	} while (0)
#define atomic_flag_clear_explicit(FLAG, MO)				\
	atomic_flag_clear(FLAG)


#define ATOMIC_FETCH_OP(PFX_OP, INF_OP, OBJ, VAL)			\
	({								\
		uint32_t irq_flag = hal_interrupt_disable_irqsave();		\
		__typeof__(OBJ) obj = (OBJ);				\
		__typeof__(VAL) val = (VAL);				\
		__typeof__(*obj) tmp = atomic_load(obj);		\
		atomic_store(obj, PFX_OP (tmp INF_OP val));		\
		hal_interrupt_enable_irqrestore(irq_flag);			\
		tmp;							\
	})
#define atomic_fetch_add(OBJ, VAL)					\
	ATOMIC_FETCH_OP(  , +, (OBJ), (VAL))
#define atomic_fetch_add_explicit(OBJ, VAL, MO)				\
	atomic_fetch_add((OBJ), (VAL))
#define atomic_fetch_sub(OBJ, VAL)					\
	ATOMIC_FETCH_OP(  , -, (OBJ), (VAL))
#define atomic_fetch_sub_explicit(OBJ, VAL, MO)				\
	atomic_fetch_sub((OBJ), (VAL))
#define atomic_fetch_or(OBJ, VAL)					\
	ATOMIC_FETCH_OP(  , |, (OBJ), (VAL))
#define atomic_fetch_or_explicit(OBJ, VAL, MO)				\
	atomic_fetch_or((OBJ), (VAL))
#define atomic_fetch_xor(OBJ, VAL)					\
	ATOMIC_FETCH_OP(  , ^, (OBJ), (VAL))
#define atomic_fetch_xor_explicit(OBJ, VAL, MO)				\
	atomic_fetch_xor((OBJ), (VAL))
#define atomic_fetch_and(OBJ, VAL)					\
	ATOMIC_FETCH_OP(  , &, (OBJ), (VAL))
#define atomic_fetch_and_explicit(OBJ, VAL, MO)				\
	atomic_fetch_and((OBJ), (VAL))

#define atomic_exchange(OBJ, DES)					\
	({								\
		uint32_t irq_flag = hal_interrupt_disable_irqsave();		\
		__typeof__(OBJ) obj = (OBJ);				\
		__typeof__(*obj) des = (DES);				\
		__typeof__(*obj) oldval = atomic_load(obj);		\
		atomic_store(obj, des);					\
		hal_interrupt_enable_irqrestore(irq_flag);			\
		oldval;							\
	})
#define atomic_exchange_explicit(OBJ, DES, MO)				\
	atomic_exchange((OBJ), (DES))
#define atomic_compare_exchange_strong(OBJ, EXP, DES)			\
	({								\
		int ret;						\
		uint32_t irq_flag = hal_interrupt_disable_irqsave();		\
		__typeof__(OBJ) obj = (OBJ);				\
		__typeof__(EXP) exp = (EXP);				\
		__typeof__(*obj) desval = (DES);			\
		__typeof__(*obj) oldval = atomic_load(obj);		\
		__typeof__(*obj) expval = atomic_load(exp);		\
		if (oldval == expval) {					\
			atomic_store(obj, desval);			\
			ret = 1;   					\
		} else {						\
			atomic_store(exp, oldval);			\
			ret = 0;					\
		}							\
		hal_interrupt_enable_irqrestore(irq_flag);			\
		ret;							\
	})
#define atomic_compare_exchange_strong_explicit(OBJ, EXP, DES, MO)	\
	atomic_compare_exchange_strong((OBJ), (EXP), (DES))
#define atomic_compare_exchange_weak(OBJ, EXP, DES)			\
	atomic_compare_exchange_strong((OBJ), (EXP), (DES))
#define atomic_compare_exchange_weak_explicit(OBJ, EXP, DES, MO)	\
	atomic_compare_exchange_weak((OBJ), (EXP), (DES))

#define atomic_thread_fence(MO)						\
	do { _Pragma("flush_memory") } while (0)
#define atomic_signal_fence(MO)						\
	do { _Pragma("flush_memory") } while (0)

#ifdef __cplusplus
}
#endif

#endif /* __METAL_XTENSA_ATOMIC__H__ */
