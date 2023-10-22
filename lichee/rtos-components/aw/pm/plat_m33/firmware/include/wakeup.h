#ifndef _FW_WAKEUP_H_
#define _FW_WAKEUP_H_

int wakeup_wait_loop(void);
int wakeup_check_callback(void);

void enable_wuptimer(unsigned int val);
uint64_t pm_gettime_ns(void);

#endif

