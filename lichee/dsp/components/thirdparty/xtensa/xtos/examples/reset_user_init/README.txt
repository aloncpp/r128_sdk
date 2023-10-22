Xtensa Reset vector provides a hook (named __reset_user_init) for users 
to supply their own code to be executed from  within the reset vector. 
The purpose is to allow user to add any functionality that deals with  
early hardware initialization, such as DDR controller setup or unpacking 
data sections using DMA.

Example in this directory shows a limited use of the __reset_user_init, 
including calling a limited C code for the purpose of initializing the 
DDR memory controller. It's assumed the processor configuration has a 
small on-chip memory where DRAM0 and IRAM0 memories are placed. This is
the only accessible memory until the DDR controller is initialized, 
thus, it's assumed that the LSP sets Reset Vector there as well.

reset_user_init.S implements __reset_user_init function.
Note the placement of text/data section into iram0.text/.dram0.data and 
calling of __ddr_init function.

ddr_init.c implements a dummy DDR initialization code.
This code just sets a value of a global variable.

main.c implements a dummy application which just prints the value 
of the global variable set in dummy DDR controller initialization code.

Makefile file can be used to build the application, containing the user 
code. Note that the makefile does perform exhaustive setup of the tools, 
system,  neither it's concerned with the good organization. It's main 
purpose is to  group all the necessary compile, line and run actions
together. In addition,  makefile shows how to properly place the section 
of the compiled C code:

When compiling the ddr_init.c code, all the program sections are renamed
so they are placed into the existing on-chip memory (iram0 and dram0)
