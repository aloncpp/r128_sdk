/* Copyright (c) 2017 Cadence Design Systems Inc.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

// User defines - change to fit your application
#define USER_STACK_SIZE    512          //user stack size
#define USER_TEXT_SECTION  .iram0.text  //section for function code
#define USER_DATA_SECTION  .dram0.data  //section for function data
// End user defines

.type	__ddr_init, @function            //dummy DDR init. C code, called from here

#if __XTENSA_CALL0_ABI__
# define CALL	call0
# define ARG1	a2	/* 1st outgoing call argument */
#else
# define CALL	call4
# define ARG1	a6	/* 1st outgoing call argument */
#endif

// Defines for setting C calling conventions below
#define PS_UM		0x00000020
#define PS_WOE_ABI 	0x00040000

	.section   USER_DATA_SECTION
	.space     USER_STACK_SIZE    //allocate space for stack
	.align     128                //allign SP to 128-bits
_user_stack:	   .word  0           //Label top of stack

// Dummy stuff needed to satisfy missing .weak variables
	.globl	_Level4FromVector
	.globl	_Level5FromVector
	.globl	_Pri_5_HandlerAddress
	.globl	_Pri_4_HandlerAddress
_Level5FromVector:  .word 0
_Level4FromVector:  .word 0
_Pri_4_HandlerAddress: .word 0
_Pri_5_HandlerAddress:  .word 0

// Finnally, the function code starts here
	.section   USER_TEXT_SECTION
	.align 4

	.globl __reset_user_init
__reset_user_init:
        movi	sp, _user_stack      //set stack pointer
	addi    sp, sp, -4
	s32i    a0, sp, 0            //save return address
	addi    sp, sp, -4
	movi	a0, 0                //This func. is top of stack for the called C function
	movi	a3, PS_UM|PS_WOE_ABI //C calling conventions: PS.WOE = 0|1, PS.UM = 1, PS.EXCM = 0, PS.INTLEVEL = 0
	wsr.ps	a3
	rsync
	CALL    __ddr_init           //Call dummy DDR initialization C code
	l32i    a0, sp, 4            //restore return address
	ret

	.size	__reset_user_init, . - __reset_user_init

