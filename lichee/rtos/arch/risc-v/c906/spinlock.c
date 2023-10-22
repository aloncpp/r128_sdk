#include <stdlib.h>
#include <irqflags.h>
#include <spinlock.h>

#include <hal_interrupt.h>

int cur_cpu_id(void)
{
    return 0;
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
    unsigned long flags = 0;
    flags = hal_interrupt_disable_irqsave();
    lock->spin_lock.slock = flags;
    return flags;
}

void freert_spin_unlock_irqrestore(volatile freert_spinlock_t *lock , unsigned long flags)
{
    hal_interrupt_enable_irqrestore(flags);
}

