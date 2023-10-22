#ifndef __CAL_RC_LF_H__
#define __CAL_RC_LF_H__

#include <stdint.h>

int get_corrected_counter_value_for_rc_lf(uint32_t target_value, uint32_t *corrected_value);
int get_corrected_time_for_rc_lf(uint32_t target_time, uint32_t *corrected_time);

#endif /* __CAL_RC_LF_H__ */