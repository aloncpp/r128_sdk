/*
 * =====================================================================================
 *
 *       Filename:  spinlock.h
 *
 *    Description:  spinlock impl.
 *
 *        Version:  1.0
 *        Created:  2019年07月11日 15时09分20秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  czl,
 *   Organization:
 *
 * =====================================================================================
 */

#ifndef SPINLOCK_H
#define SPINLOCK_H
#include <stddef.h>
#include <stdint.h>
#include <FreeRTOSConfig.h>

#define FREERT_SPINLOCK_MAX_NEST_COUNT      256UL

#define SPIN_LOCK_NOT_OWNED                 (0xdeadbeef)

typedef union
{
    unsigned long slock;
    struct __arch_tickets
    {
        unsigned short owner;
        unsigned short next;
    } tickets;
} __freert_spinlock_t;

typedef struct
{
    __freert_spinlock_t spin_lock;
    uint32_t owner;
    uint32_t counter;
} freert_spinlock_t;

typedef struct
{
    uint32_t cpus_lock_nest;
} percpu_lock_t;

typedef struct
{
    uint32_t rw_lock;
} freert_rwlock_t;

void freert_spin_init(volatile freert_spinlock_t *lock);
void freert_spin_lock(volatile freert_spinlock_t *lock);
void freert_spin_unlock(volatile freert_spinlock_t *lock);
int freert_spin_trylock(volatile freert_spinlock_t *lock);
int freert_spin_is_locked(volatile freert_spinlock_t *lock);
int freert_spin_is_contended(volatile freert_spinlock_t *lock);
void freert_spin_unlock_irqrestore(volatile freert_spinlock_t *lock, unsigned long flags);
unsigned long freert_spin_lock_irqsave(volatile freert_spinlock_t *lock);
int cur_cpu_id(void);
void freert_read_lock(freert_rwlock_t *lock);
void freert_read_unlock(freert_rwlock_t *lock);
void freert_write_lock(freert_rwlock_t *lock);
void freert_write_unlock(freert_rwlock_t *lock);
uint32_t freert_read_lock_irqsave(freert_rwlock_t *lock);
void freert_read_unlock_irqsave(freert_rwlock_t *lock, unsigned long sr_level);
unsigned long freert_write_lock_irqsave(freert_rwlock_t *lock);
void freert_write_unlock_irqsave(freert_rwlock_t *lock, unsigned long sr_level);
extern volatile freert_spinlock_t bootup_lock;

#endif
