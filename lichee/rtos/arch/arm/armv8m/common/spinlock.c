#include <stdio.h>
#include <FreeRTOSConfig.h>
#include <spinlock.h>
#include <hal_interrupt.h>

#define IRQMASK_REG_NAME_R "primask"
#define IRQMASK_REG_NAME_W "primask"

int cur_cpu_id(void)
{
    return 0;
}

void xport_interrupt_enable(uint32_t flags)
{
    __asm volatile("msr    " IRQMASK_REG_NAME_W ", %0":: "r" (flags): "memory", "cc");
}

unsigned long xport_interrupt_disable(void)
{
    unsigned long flags;
    __asm volatile(
        "mrs    %0, " IRQMASK_REG_NAME_R "\n"
        "cpsid  i" : "=r" (flags) : : "memory", "cc");
    return flags;
}

void freert_spin_lock(volatile freert_spinlock_t *lock)
{
    lock->spin_lock.slock = hal_interrupt_disable_irqsave();
    return;
}

void freert_spin_unlock(volatile freert_spinlock_t *lock)
{
    hal_interrupt_enable_irqrestore(lock->spin_lock.slock);
    return;
}

void __freert_spin_lock_init(volatile freert_spinlock_t *lock)
{
    lock->owner = 0;
    lock->counter =  0;
    lock->spin_lock.slock = 0;
}

void freert_spin_init(volatile freert_spinlock_t *lock)
{
    __freert_spin_lock_init(lock);
}

unsigned long freert_spin_lock_irqsave(volatile freert_spinlock_t *lock)
{
    unsigned long flags;
    flags = hal_interrupt_disable_irqsave();
    lock->spin_lock.slock = flags;
    return flags;
}

void freert_spin_unlock_irqrestore(volatile freert_spinlock_t *lock , unsigned long flags)
{
	hal_interrupt_enable_irqrestore(flags);
}

uint32_t freert_cpus_lock(void)
{
    return hal_interrupt_disable_irqsave();
}

void freert_cpus_unlock(uint32_t cpu_sr)
{
    hal_interrupt_enable_irqrestore(cpu_sr);
}

