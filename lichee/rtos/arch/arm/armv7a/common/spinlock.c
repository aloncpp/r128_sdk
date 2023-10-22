#include "spinlock.h"
#include "serial.h"
#include <stdio.h>
#include <FreeRTOSConfig.h>
#include <spinlock.h>

volatile freert_spinlock_t bootup_lock;
volatile freert_spinlock_t cpus_lock_obj;

void vTaskScheduerEnable(void);
void vTaskScheduerDisable(void);
void __freert_spin_lock(volatile freert_spinlock_t *lock);
void __freert_spin_unlock(volatile freert_spinlock_t *lock);

percpu_lock_t cpus_lock[configNR_CPUS];

#define IRQ_MASK             0x00000080
#define FIQ_MASK             0x00000040
#define INT_MASK             (IRQ_MASK | FIQ_MASK)


int judge_critical(void)
{
    return cpus_lock[cur_cpu_id()].cpus_lock_nest > 0; 
}
#ifdef USE_C_CPUID
#ifdef configKERNEL_SUPPORT_SMP
int cur_cpu_id(void)
{
    int cpu_id;

    __asm__ __volatile__ (
	"mrc p15, 0, %0, c0, c0, 5"
	:"=r"(cpu_id)
	);
    cpu_id &= 0xf;

    return cpu_id;
}
#else
int cur_cpu_id(void)
{
    return 0;
}
#endif
#endif

#ifdef configKERNEL_SUPPORT_SMP
extern unsigned long xport_interrupt_disable(void);
extern void xport_interrupt_enable(unsigned long);

static inline unsigned long __get_cpsr(void)
{
    unsigned long retval;
    __asm__ __volatile__(
        " mrs  %0, cpsr":"=r"(retval): /* no inputs */);
    return retval;
}

static inline void __set_cpsr(unsigned long val)
{
    __asm__ __volatile__(
        " msr  cpsr, %0": /* no outputs */ :"r"(val));
}

static inline unsigned long awos_arch_disable_interrupt(void)
{
    unsigned long _cpsr;
    _cpsr = __get_cpsr();
    __set_cpsr(_cpsr | INT_MASK);
    return _cpsr;
}

static inline unsigned long awos_arch_restore_interrupt(unsigned long old_cpsr)
{
    unsigned long _cpsr;
    _cpsr = __get_cpsr();
    __set_cpsr((_cpsr & ~INT_MASK) | (old_cpsr & INT_MASK));
    return _cpsr;
}

static uint32_t interrupt_status_nest[configNR_CPUS];
static uint32_t interrupt_status_value[configNR_CPUS];
void freert_cpus_unlock(uint32_t sr_level);
uint32_t freert_cpus_lock(void);
void irqsave_and_cli(void)
{
    int processor_id = cur_cpu_id();
    if(interrupt_status_nest[processor_id] == 0)
    {
        interrupt_status_value[processor_id] = freert_cpus_lock() ;
    }

    interrupt_status_nest[processor_id] ++;
}

void irqrsto_and_ena(void)
{
    int processor_id = cur_cpu_id();

    if(interrupt_status_nest[processor_id])
    {
        interrupt_status_nest[processor_id] --;
        if(interrupt_status_nest[processor_id] == 0)
        {
            freert_cpus_unlock(interrupt_status_value[processor_id]);
        }
    }
}

void task_cpus_lock_callback(void);
void task_cpus_unlock_callback(void);
__attribute__((noinline)) uint32_t freert_cpus_lock(void)
{
    uint32_t sr_level;
    uint32_t old_lck;
    int processor_id = cur_cpu_id();
    
    sr_level = xport_interrupt_disable();

    old_lck = cpus_lock[processor_id].cpus_lock_nest;
    cpus_lock[processor_id].cpus_lock_nest ++;

    if(cpus_lock[processor_id].cpus_lock_nest >= FREERT_SPINLOCK_MAX_NEST_COUNT)
    {
        xport_interrupt_enable(sr_level);
        //failure case of max nested on one core.
        soft_break();
        return -1;
    } 
    else
    {
        if(old_lck == 0)
        {
            vTaskScheduerDisable();
            __freert_spin_lock(&cpus_lock_obj);
            task_cpus_lock_callback();
        }
    }

    return sr_level;
}

__attribute__((noinline)) void freert_cpus_unlock(uint32_t sr_level)
{
    int processor_id = cur_cpu_id();

    cpus_lock[processor_id].cpus_lock_nest --;
    if(cpus_lock[processor_id].cpus_lock_nest == 0)
    {
        task_cpus_unlock_callback();
        __freert_spin_unlock(&cpus_lock_obj);
        vTaskScheduerEnable();
    }

    xport_interrupt_enable(sr_level);
}

static uint32_t ownerid[configNR_CPUS] = {0x30757063, 0x31757063};

void __freert_spin_lock_init(volatile freert_spinlock_t *lock)
{
    lock->owner = 0;
    lock->counter =  0;
    lock->spin_lock.slock = 0;
}

void __freert_spin_lock(volatile freert_spinlock_t *lock)
{
    unsigned long tmp;
    unsigned long newval;
    __freert_spinlock_t lockval;

    //prefetch operations.
    __asm__ __volatile__(
        "pldw [%0]"
        ::"p"(&lock->spin_lock.slock)
    );
   
    __asm__ __volatile__(
        "1: ldrex   %0, [%3]\n"
        "   add %1, %0, %4\n"
        "   strex   %2, %1, [%3]\n"
        "   teq %2, #0\n"
        "   bne 1b"
        : "=&r" (lockval), "=&r" (newval), "=&r" (tmp)
        : "r" (&lock->spin_lock.slock), "I" (1 << 16)
        : "cc");

    while (lockval.tickets.next != lockval.tickets.owner) {
        __asm__ __volatile__("wfe":::"memory");
        lockval.tickets.owner = *(volatile unsigned short *)(&lock->spin_lock.tickets.owner);
    }

    dsb(sy);

    lock->owner = ownerid[cur_cpu_id()];
    lock->counter ++;

    /*spin lock cant be nested locked.*/
    if(lock->counter != 1)
    {
        SMP_DBG("fatal error: nest lock %d times by core %d.\n", lock->counter, lock->owner);
        soft_break();
    }

    dsb(sy);
}

void __freert_spin_unlock(volatile freert_spinlock_t *lock)
{
    int processor_id = cur_cpu_id();
    if(lock->owner != ownerid[processor_id] || lock->counter != 1)
    {
        SMP_DBG("fatal error:locked by core 0x%08x, but unlocked another core, counter %d.\n", \
            lock->owner, lock->counter);
        soft_break();
    }

    lock->owner = SPIN_LOCK_NOT_OWNED;
    lock->counter --;
    dsb(sy);

    lock->spin_lock.tickets.owner++;
    dsb(sy);
    __asm__ __volatile__ ("sev":::"memory", "cc");

}

int in_cpus_lock(void)
{
    int processor_id = cur_cpu_id();

    return cpus_lock_obj.owner == ownerid[processor_id];
}

int freert_spin_is_locked(volatile freert_spinlock_t *lock)
{
    return lock->spin_lock.tickets.next != lock->spin_lock.tickets.owner;
}

int freert_spin_is_contended(volatile freert_spinlock_t *lock)
{
    return (lock->spin_lock.tickets.next - lock->spin_lock.tickets.owner) > 1;
}

void freert_spin_lock(volatile freert_spinlock_t *lock)
{
    if(lock != &bootup_lock)
    {
    	vTaskScheduerDisable();
    }

    __freert_spin_lock(lock);
}

void freert_spin_unlock(volatile freert_spinlock_t *lock)
{
    __freert_spin_unlock(lock);

    if(lock != &bootup_lock)
    {
    	vTaskScheduerEnable();
    }
}

void freert_spin_init(volatile freert_spinlock_t *lock)
{
    __freert_spin_lock_init(lock);
}

/* return 1: get the lock;
 * return 0: failure to get the lock;
 */
int freert_spin_trylock(volatile freert_spinlock_t *lock)
{
    uint32_t contended, res, slock;

    //prefetch operations.
    __asm__ __volatile__(
        "pldw [%0]"
        ::"p"(&lock->spin_lock.slock)
    );

    do {
        __asm__ __volatile__(
        "ldrex   %0, [%3]\n"
        "mov     %2, #0\n"
        "subs    %1, %0, %0, ror #16\n"
        "addeq   %0, %0, %4\n"
        "strexeq %2, %0, [%3]"
        : "=&r" (slock), "=&r" (contended), "=&r" (res)
        : "r" (&lock->spin_lock.slock), "I" (1 << 16)
        : "cc");
    } while (res);

    if (!contended)
    {
        dmb(ish);
        return 1;
    }
    else
    {
        return 0;
    }
}

unsigned long freert_spin_lock_irqsave(volatile freert_spinlock_t *lock)
{
    unsigned long sr_level;

    vTaskScheduerDisable();

    sr_level = xport_interrupt_disable();
    freert_spin_lock(lock);

    return sr_level;
}

void freert_spin_unlock_irqrestore(volatile freert_spinlock_t *lock , unsigned long flags)
{
    freert_spin_unlock(lock);
    xport_interrupt_enable(flags);
    vTaskScheduerEnable();
}

static void __freert_read_lock(freert_rwlock_t *lock)
{
    uint32_t tmp, tmp2;

    //prefetch operations.
    __asm__ __volatile__(
        "pldw [%0]"
        ::"p"(&lock->rw_lock)
    );

    __asm__ __volatile__(
        "1: ldrex   %0, [%2]\n"
        "   adds    %0, %0, #1\n"
        "   strexpl %1, %0, [%2]\n"
        "   wfemi\n"
        "   rsbpls  %0, %1, #0\n"
        "   bmi     1b"
        : "=&r" (tmp), "=&r" (tmp2)
        : "r" (&lock->rw_lock)
        : "cc");

    dmb(ish);
}

static void __freert_read_unlock(freert_rwlock_t *lock)
{
    uint32_t tmp, tmp2;

    dmb(ish);

    __asm__ __volatile__(
        "pldw [%0]"
        ::"p"(&lock->rw_lock)
    );

    __asm__ __volatile__(
        "1: ldrex   %0, [%2]\n"
        "   sub     %0, %0, #1\n"
        "   strex   %1, %0, [%2]\n"
        "   teq     %1, #0\n"
        "   bne     1b"
        : "=&r" (tmp), "=&r" (tmp2)
        : "r" (&lock->rw_lock)
        : "cc");

    if (tmp == 0)
    {
        dsb(ishst);
        __asm__ volatile ("sev":::"memory", "cc");
    }
}

static int __freert_read_trylock(freert_rwlock_t *lock)
{
    uint32_t contended, res;

    __asm__ __volatile__(
        "pldw [%0]"
        ::"p"(&lock->rw_lock)
    );

    do {
        __asm__ __volatile__(
        "ldrex   %0, [%2]\n"
        "mov     %1, #0\n"
        "adds    %0, %0, #1\n"
        "strexpl %1, %0, [%2]"
        : "=&r" (contended), "=&r" (res)
        : "r" (&lock->rw_lock)
        : "cc");
    } while (res);

    /* If the lock is negative, then it is already held for write. */
    if (contended < 0x80000000)
    {
        dmb(ish);
        return 1;
    }
    else
    {
        return 0;
    }
}

static void __freert_write_lock(freert_rwlock_t *lock)
{
    uint32_t tmp;

    __asm__ __volatile__(
        "pldw [%0]"
        ::"p"(&lock->rw_lock)
    );

    __asm__ __volatile__(
        "1: ldrex   %0, [%1]\n"
        "teq     %0, #0\n"
        "wfene\n"
        "strexeq %0, %2, [%1]\n"
        "teq     %0, #0\n"
        "bne     1b"
        : "=&r" (tmp)
        : "r" (&lock->rw_lock), "r" (0x80000000)
        : "cc");

    dmb(ish);
}

static int freert_write_trylock(freert_rwlock_t *lock)
{
    uint32_t contended, res;

    __asm__ __volatile__(
        "pldw [%0]"
        ::"p"(&lock->rw_lock)
    );
    do {
        __asm__ __volatile__(
        "ldrex   %0, [%2]\n"
        "mov     %1, #0\n"
        "teq     %0, #0\n"
        "strexeq %1, %3, [%2]"
        : "=&r" (contended), "=&r" (res)
        : "r" (&lock->rw_lock), "r" (0x80000000)
        : "cc");
    } while (res);

    if (!contended)
    {
        dmb(ish);
        return 1;
    }
    else
    {
        return 0;
    }
}

static void __freert_write_unlock(freert_rwlock_t *lock)
{
    dmb(ish);

    __asm__ __volatile__(
        "str    %1, [%0]\n"
        :
        : "r" (&lock->rw_lock), "r" (0)
        : "cc");

    dsb(ishst);
    __asm__ volatile ("sev":::"memory", "cc");
}

void freert_read_lock(freert_rwlock_t *lock)
{
    vTaskScheduerDisable();
    __freert_read_lock(lock);
}

void freert_read_unlock(freert_rwlock_t *lock)
{
    __freert_read_unlock(lock);
    vTaskScheduerEnable();
}

void freert_write_lock(freert_rwlock_t *lock)
{
    vTaskScheduerDisable();
    __freert_write_lock(lock);
}

void freert_write_unlock(freert_rwlock_t *lock)
{
    __freert_write_unlock(lock);
    vTaskScheduerEnable();
}

uint32_t freert_read_lock_irqsave(freert_rwlock_t *lock)
{
    uint32_t sr_level;

    sr_level = xport_interrupt_disable();
    vTaskScheduerDisable();
    __freert_read_lock(lock);
    return sr_level;
}

void freert_read_unlock_irqsave(freert_rwlock_t *lock, unsigned long sr_level)
{
    __freert_read_unlock(lock);
    vTaskScheduerEnable();
    xport_interrupt_enable(sr_level);
}

unsigned long freert_write_lock_irqsave(freert_rwlock_t *lock)
{
    unsigned long sr_level;

    sr_level = xport_interrupt_disable();
    __freert_write_lock(lock);
    return sr_level;
}

void freert_write_unlock_irqsave(freert_rwlock_t *lock, unsigned long sr_level)
{
    __freert_write_unlock(lock);
    xport_interrupt_enable(sr_level);
}

#else  //NO SMP Support.
void freert_spin_lock(volatile freert_spinlock_t *lock)
{
    return;
}

void freert_spin_unlock(volatile freert_spinlock_t *lock)
{
    return;
}

unsigned long freert_spin_lock_irqsave(volatile freert_spinlock_t *lock)
{
    return 0;
}

void freert_spin_unlock_irqrestore(volatile freert_spinlock_t *lock , uint32_t flags)
{
    return;
}

void freert_read_unlock(freert_rwlock_t *lock)
{
    return;
}

void freert_read_lock(freert_rwlock_t *lock)
{
    return;
}

void freert_write_lock(freert_rwlock_t *lock)
{
    return;
}

void freert_write_unlock(freert_rwlock_t *lock)
{

}

void freert_write_unlock_irqsave(freert_rwlock_t *lock, uint32_t sr_level)
{

}
uint32_t freert_write_lock_irqsave(freert_rwlock_t *lock)
{
	return 0;
}

uint32_t freert_cpus_lock(void)
{
    vTaskScheduerDisable();
    return xport_interrupt_disable();
}

void freert_cpus_unlock(uint32_t cpu_sr)
{
    xport_interrupt_enable(cpu_sr);
    vTaskScheduerEnable();
}

void irqsave_and_cli(void)
{

}
void irqrsto_and_ena(void)
{

}
#endif

void schedule_with_irq_disable(uint32_t lr, uint32_t spsr)
{
	if(spsr & 0xc0)
	{
		/*SMP_DBG("schedule with irq disabled! lr = 0x%08x. spsr = 0x%08x.\n", lr, spsr);*/
	}

	return;
}
