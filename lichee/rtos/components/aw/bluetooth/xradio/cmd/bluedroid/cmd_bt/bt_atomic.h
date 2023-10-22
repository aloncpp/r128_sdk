/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __BT_ATOMIC_H__
#define __BT_ATOMIC_H__

#include <stdbool.h>
#include <FreeRTOS-orig/Source/include/atomic.h>

typedef uint32_t volatile bt_atomic_t;

#define BT_ATOMIC_BITS (sizeof(bt_atomic_t) * 8)
#define BT_ATOMIC_MASK(bit) (1 << ((uint32_t)(bit) & (BT_ATOMIC_BITS - 1)))

static inline bool bt_atomic_test_bit(bt_atomic_t *target, int bit)
{
	uint32_t val = *target;

	return (1 & (val >> (bit & (BT_ATOMIC_BITS - 1)))) != 0;
}

static inline void bt_atomic_clear_bit(bt_atomic_t *target, int bit)
{
	uint32_t mask = BT_ATOMIC_MASK(bit);
	(void)Atomic_AND_u32(target, ~mask);
}

static inline void bt_atomic_set_bit(bt_atomic_t *target, int bit)
{
	uint32_t mask = BT_ATOMIC_MASK(bit);
	(void)Atomic_OR_u32(target, mask);
}

static portFORCE_INLINE uint32_t bt_atomic_set_val( uint32_t volatile * pulAddend,
                                                  uint32_t ulCount )
{
	uint32_t ulCurrent = 0;

	ATOMIC_ENTER_CRITICAL();
	{
		*pulAddend = ulCurrent;
		*pulAddend |= ulCount;
	}
	ATOMIC_EXIT_CRITICAL();

	return ulCurrent;
}

#endif
