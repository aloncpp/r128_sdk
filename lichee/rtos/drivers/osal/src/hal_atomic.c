#include <string.h>
#include <hal_osal.h>
#include <hal_atomic.h>

unsigned long hal_spin_lock_irqsave(hal_spinlock_t *lock)
{
	unsigned long cpu_sr;

	cpu_sr = freert_spin_lock_irqsave((freert_spinlock_t *)lock);
	if (hal_interrupt_get_nest() == 0)
		hal_thread_scheduler_suspend();

	return cpu_sr;
}

int hal_spin_lock_init(hal_spinlock_t *lock)
{
    memset(lock, 0, sizeof(hal_spinlock_t));
    return HAL_OK;
}

int hal_spin_lock_deinit(hal_spinlock_t *lock)
{
    return HAL_OK;
}

void hal_spin_unlock_irqrestore(hal_spinlock_t *lock, unsigned long __cpsr)
{
	if (hal_interrupt_get_nest() == 0)
		hal_thread_scheduler_resume();
	freert_spin_unlock_irqrestore((freert_spinlock_t *)lock, __cpsr);
}

void hal_spin_lock(hal_spinlock_t *lock)
{
	freert_spin_lock((freert_spinlock_t *)lock);
}

void hal_spin_unlock(hal_spinlock_t *lock)
{
	freert_spin_unlock((freert_spinlock_t *)lock);
}
