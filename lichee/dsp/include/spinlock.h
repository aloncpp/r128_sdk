/* FreeRTOS */
#include <portmacro.h>

#define spin_lock_irqsave(flags) \
do { \
	flags = portENTER_CRITICAL_NESTED(); \
} while (0)

#define spin_unlock_irqrestore(flags) \
do { \
	portEXIT_CRITICAL_NESTED(flags); \
} while (0)
