/* Miscellaneous system calls for the Xtensa semihosting simulator.  */

/*
 * Copyright (c) 2003, 2004 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include <xtensa/config/core.h>
#include <xtensa/simcall.h>

void 
xt_profile_init(void)
{ 
  /* no-op in ISS */
}


void 
xt_profile_add_memory(void * buf, unsigned int buf_size)
{ 
  /* no-op in ISS */
}


void
xt_profile_enable (void)
{
  register int a2 __asm__ ("a2") = SYS_profile_enable;
  register int ret_val __asm__ ("a2");
  register int ret_err __asm__ ("a3");
  __asm__ volatile ("simcall"
		    : "=a" (ret_val), "=a" (ret_err)
		    : "a" (a2));
}


void
xt_profile_disable (void)
{
  register int a2 __asm__ ("a2") = SYS_profile_disable;
  register int ret_val __asm__ ("a2");
  register int ret_err __asm__ ("a3");
  __asm__ volatile ("simcall"
		    : "=a" (ret_val), "=a" (ret_err)
		    : "a" (a2));
}


/* Deprecated name */
void
xt_iss_profile_enable (void)
{
  register int a2 __asm__ ("a2") = SYS_profile_enable;
  register int ret_val __asm__ ("a2");
  register int ret_err __asm__ ("a3");
  __asm__ volatile ("simcall"
		    : "=a" (ret_val), "=a" (ret_err)
		    : "a" (a2));
}


/* Deprecated name */
void
xt_iss_profile_disable (void)
{
  register int a2 __asm__ ("a2") = SYS_profile_disable;
  register int ret_val __asm__ ("a2");
  register int ret_err __asm__ ("a3");
  __asm__ volatile ("simcall"
		    : "=a" (ret_val), "=a" (ret_err)
		    : "a" (a2));
}


void
xt_iss_trace_level (unsigned level)
{
  register int a2 __asm__ ("a2") = SYS_trace_level;
  register unsigned a3 __asm__ ("a3") = level;
  register int ret_val __asm__ ("a2");
  register int ret_err __asm__ ("a3");
  __asm__ volatile ("simcall"
		    : "=a" (ret_val), "=a" (ret_err)
		    : "a" (a2), "a" (a3));
}

int
xt_iss_client_command(const char *client, const char *command)
{
  register int a2 __asm__ ("a2") = SYS_client_command;
  register unsigned a3 __asm__ ("a3") = (unsigned)client;
  register unsigned a4 __asm__ ("a4") = (unsigned)command;
  register int ret_val __asm__ ("a2");
  register int ret_err __asm__ ("a3");
  __asm__ volatile ("simcall"
		    : "=a" (ret_val), "=a" (ret_err)
		    : "a" (a2), "a" (a3), "a" (a4));
  return ret_val;
}

unsigned long long
xt_iss_cycle_count()
{
  register unsigned int a2_in  __asm__ ("a2") = SYS_cycle_count;
  register unsigned int a2_out __asm__ ("a2");
  register unsigned int a3_out __asm__ ("a3");
  __asm__ volatile ("simcall"
		    : "=a" (a2_out), "=a" (a3_out)
		    : "a" (a2_in));
#if XCHAL_HAVE_BE
  return (((unsigned long long)a2_out) << 32) | a3_out;
#else
  return (((unsigned long long)a3_out) << 32) | a2_out;
#endif
}

unsigned int
xt_iss_peek(unsigned long long address)
{
  register int a2  __asm__ ("a2") = SYS_peek;
  /* No need to consider endianness when splitting a 64-bit address
     into two 32-bit registers because in iss4_simcall.cxx the same
     way will be used to join the 64-bit address back.  */
  register unsigned int a3 __asm__ ("a3") = (unsigned int) (address >> 32);
  register unsigned int a4 __asm__ ("a4") = (unsigned int) address ;
  register unsigned int ret_val __asm__ ("a2");
  register int ret_err __asm__ ("a3");
  __asm__ volatile ("simcall"
		    : "=a" (ret_val), "=a" (ret_err)
		    : "a" (a2), "a" (a3), "a" (a4)); 
  return ret_val;
}

int 
xt_iss_poke(unsigned src_value, unsigned long long address)
{
  register int a2  __asm__ ("a2") = SYS_poke;
  register unsigned int a3 __asm__ ("a3") = src_value;
  /* No need to consider endianness when splitting a 64-bit address
     into two 32-bit registers because in iss4_simcall.cxx the same
     way will be used to join the 64-bit address back.  */
  register unsigned int a4 __asm__ ("a4") = (unsigned int) (address >> 32);
  register unsigned int a5 __asm__ ("a5") = (unsigned int) address ;
  register int ret_val __asm__ ("a2");
  register int ret_err __asm__ ("a3");
  __asm__ volatile ("simcall"
		    : "=a" (ret_val), "=a" (ret_err)
		    : "a" (a2), "a" (a3), "a" (a4), "a" (a5)); 
  return ret_val;
}

int
xt_iss_address_to_symbol(void *addr, char *buf)
{
  register int a2 __asm__ ("a2") = SYS_addr_to_sym;
  register unsigned a3 __asm__ ("a3") = (unsigned)addr;
  register unsigned a4 __asm__ ("a4") = (unsigned)buf;
  register int ret_val __asm__ ("a2");
  register int ret_err __asm__ ("a3");
  __asm__ volatile ("simcall"
		    : "=a" (ret_val), "=a" (ret_err)
		    : "a" (a2), "a" (a3), "a" (a4));
  return ret_val;
}

int 
xt_profile_save_and_reset(void)
{
  if (xt_iss_client_command("profile", "save") == 0)
    return xt_iss_client_command("profile", "reset");
  return -1;
}

unsigned int 
xt_profile_get_frequency(void)
{
  /* always returns 1 in ISS */
  return 1;
}


void 
xt_profile_set_frequency(unsigned int sample_frequency)
{
  /* no-op in ISS */
}

int 
xt_profile_num_errors(void)
{
  /* always returns 0 in ISS */
  return 0;
}

