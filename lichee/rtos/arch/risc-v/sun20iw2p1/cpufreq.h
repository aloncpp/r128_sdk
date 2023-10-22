#ifndef __CPU_FREQ_
#define __CPU_FREQ_

#include <stdint.h>

int set_cpu_freq(uint32_t target_freq);
int get_cpu_freq(uint32_t *cpu_freq);

int get_cpu_voltage(uint32_t *cpu_voltage);

uint32_t get_available_cpu_freq_num(void);
int get_available_cpu_freq(uint32_t freq_index, uint32_t *cpu_freq);
int get_available_cpu_freq_info(uint32_t freq_index, uint32_t *cpu_freq, uint32_t *cpu_voltage);

#endif /* __CPU_FREQ_ */
