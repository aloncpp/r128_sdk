#ifndef __HW_SPIN_LOCK__
#define __HW_SPIN_LOCK__

enum hwspinlock_err {
	HWSPINLOCK_OK = 0,
	HWSPINLOCK_ERR = -1,
	HWSPINLOCK_EXCEED_MAX = -2,
	HWSPINLOCK_PM_ERR = -3,
	HWSPINLOCK_TIMEOUT = -4,
};

#define SPINLOCK_CLI_UART_LOCK_BIT (0)

void hal_hwspinlock_init(void);
int hal_hwspinlock_put(int num);
int hal_hwspinlock_get(int num);
int hal_hwspinlock_check_taken(int num);

extern int hal_hwspin_lock(int num);
extern int hal_hwspin_lock_timeout(int num, int ms_timeout);
extern void hal_hwspin_unlock(int num);

#endif
