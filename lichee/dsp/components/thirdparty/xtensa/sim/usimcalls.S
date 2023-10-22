/*******************************************************************************

user_simcall.S	xtensa XTMP user simcall stall

This must be preprocessed by CPP.

Copyright (c) 2006 by Tensilica Inc.  ALL RIGHTS RESERVED.
These coded instructions, statements, and computer programs are the
copyrighted works and confidential proprietary information of Tensilica Inc.
They may not be modified, copied, reproduced, distributed, or disclosed to
third parties in any manner, medium, or form, in whole or in part, without
the prior written consent of Tensilica Inc.

*******************************************************************************/

#include <xtensa/coreasm.h>
#include <xtensa/simcall.h>
#include <xtensa/config/specreg.h>

        .global xt_iss_simcall
	.text

        .align 4
xt_iss_simcall:
	abi_entry
	mov	a8, a2
	movi	a2, SYS_user_simcall
	simcall
        abi_return

