#ifndef __DELAY_H__
#define __DELAY_H__


int usleep(unsigned int usec);
void msleep(unsigned int x);

void udelay(unsigned int x);
void arch_freq_update(void);

#endif /* __DELAY_H__ */
