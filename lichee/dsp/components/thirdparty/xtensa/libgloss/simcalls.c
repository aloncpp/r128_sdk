/* Stubs for semihosting calls into the Xtensa simulator.  */

/*
 * Copyright (c) 2003-2017 Tensilica Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <xtensa/config/core.h>
#include <xtensa/hal.h>

/*
 *  Given this typically runs on real hardware, not on a simulator,
 *  most of these calls into the simulator are stubbed (do nothing):
 */

void xt_profile_init (void) { }
void xt_profile_add_memory (void * buf, unsigned int buf_size) { }
void xt_profile_enable (void) { }
void xt_profile_disable (void) { }
void xt_profile_set_frequency (unsigned int sample_frequency) { }
void xt_iss_trace_level (unsigned level) { }
void xt_iss_event_fire (unsigned event_id) { }
void xt_iss_event_wait (unsigned event_id) { }

/* Deprecated names */
void xt_iss_profile_enable (void) { }
void xt_iss_profile_disable (void) { }


/*
 *  Fixed return values:
 */

int
xt_iss_client_command (const char *client, const char *command)
{
  return -1;	/* failure error code */
}

int 
xt_profile_save_and_reset (void)
{
  return -1;	/* failure error code */
}

int
xt_iss_switch_mode (int mode)
{
  return -1;	/* failure error code (?) */
}

int
xt_iss_simcall (/*...*/)
{
  return -1;	/* failure error code (?) */
}

int
xt_iss_reg_read (unsigned target_number, unsigned *regval)
{
  return -1;	/* failure error code */
}

int
xt_iss_reg_write (unsigned target_number, unsigned *regval)
{
  return -1;	/* failure error code */
}

unsigned int 
xt_profile_get_frequency (void)
{
  return 1;
}

int 
xt_profile_num_errors (void)
{
  return 0;
}


/*
 *  NOTE:  For now, this only returns a 32-bit CCOUNT, not a 64-bit count.
 */
unsigned long long
xt_iss_cycle_count (void)
{
  return xthal_get_ccount();
}

